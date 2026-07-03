#!/usr/bin/env python3
"""
MK OS - Intelligent Web Scraper
Extracts facts from web pages and outputs in .mk pipe-delimited format.
Respects robots.txt via urllib.robotparser. Rate limited.

Usage:
    python3 web_scraper.py --help
    python3 web_scraper.py --url "https://en.wikipedia.org/wiki/Python_(programming_language)"
    python3 web_scraper.py --topic "machine learning"
"""

import argparse
import json
import re
import sys
import time
import urllib.parse
import urllib.request
import urllib.error
import urllib.robotparser
from html.parser import HTMLParser

# Rate limiting
_last_request_time = 0
MIN_REQUEST_INTERVAL = 3.0  # seconds between requests


def rate_limit_wait():
    """Enforce minimum delay between requests."""
    global _last_request_time
    now = time.time()
    elapsed = now - _last_request_time
    if elapsed < MIN_REQUEST_INTERVAL:
        time.sleep(MIN_REQUEST_INTERVAL - elapsed)
    _last_request_time = time.time()


def check_robots_txt(url):
    """Check if URL is allowed by robots.txt."""
    try:
        parsed = urllib.parse.urlparse(url)
        robots_url = f"{parsed.scheme}://{parsed.netloc}/robots.txt"
        rp = urllib.robotparser.RobotFileParser()
        rp.set_url(robots_url)
        rp.read()
        return rp.can_fetch("MK-OS-Scraper", url)
    except Exception:
        # If we cannot read robots.txt, assume allowed
        return True


class TextExtractor(HTMLParser):
    """Simple HTML parser that extracts text content."""

    def __init__(self):
        super().__init__()
        self.text_parts = []
        self.current_tag = ""
        self.skip_tags = {"script", "style", "nav", "footer", "header", "aside"}
        self.in_skip = 0
        self.headings = []
        self.paragraphs = []

    def handle_starttag(self, tag, attrs):
        self.current_tag = tag
        if tag in self.skip_tags:
            self.in_skip += 1

    def handle_endtag(self, tag):
        if tag in self.skip_tags:
            self.in_skip -= 1
        self.current_tag = ""

    def handle_data(self, data):
        if self.in_skip > 0:
            return
        text = data.strip()
        if not text:
            return
        if self.current_tag in ("h1", "h2", "h3", "h4"):
            self.headings.append(text)
        elif self.current_tag in ("p", "li", "td", "span", "div"):
            if len(text) > 20:
                self.paragraphs.append(text)
        self.text_parts.append(text)


def fetch_page(url, timeout=30):
    """Fetch a web page with proper headers."""
    rate_limit_wait()
    try:
        req = urllib.request.Request(
            url,
            headers={
                "User-Agent": "MK-OS-Scraper/1.0 (Knowledge gathering bot)",
                "Accept": "text/html,application/xhtml+xml",
            },
        )
        with urllib.request.urlopen(req, timeout=timeout) as response:
            charset = response.headers.get_content_charset() or "utf-8"
            return response.read().decode(charset, errors="replace")
    except urllib.error.HTTPError as e:
        sys.stderr.write(f"[ERROR] HTTP {e.code}: {e.reason} for {url}\n")
        return None
    except urllib.error.URLError as e:
        sys.stderr.write(f"[ERROR] URL error: {e.reason}\n")
        return None
    except Exception as e:
        sys.stderr.write(f"[ERROR] Failed to fetch {url}: {e}\n")
        return None


def extract_text(html):
    """Extract structured text from HTML content."""
    parser = TextExtractor()
    parser.feed(html)
    return {
        "headings": parser.headings,
        "paragraphs": parser.paragraphs,
        "full_text": " ".join(parser.text_parts),
    }


def extract_facts_from_text(text, source_name="web"):
    """
    Extract facts from text and format as .mk pipe-delimited lines.
    Format: source|relation|target|weight
    """
    facts = []
    sentences = re.split(r"[.!?]+", text)

    for sentence in sentences:
        sentence = sentence.strip()
        if len(sentence) < 10 or len(sentence) > 200:
            continue

        # Pattern: "X is a Y" or "X is Y"
        match = re.match(
            r"^(.+?)\s+(?:is|are)\s+(?:a|an|the)?\s*(.+)$", sentence, re.IGNORECASE
        )
        if match:
            subject = match.group(1).strip()[:50]
            obj = match.group(2).strip()[:80]
            if len(subject) > 2 and len(obj) > 2:
                facts.append(f"{subject}|is_a|{obj}|0.7")
                continue

        # Pattern: "X was created/developed/invented by Y"
        match = re.match(
            r"^(.+?)\s+(?:was|were)\s+(?:created|developed|invented|founded|built)\s+by\s+(.+)$",
            sentence,
            re.IGNORECASE,
        )
        if match:
            subject = match.group(1).strip()[:50]
            creator = match.group(2).strip()[:50]
            if len(subject) > 2 and len(creator) > 2:
                facts.append(f"{subject}|created_by|{creator}|0.8")
                continue

        # Pattern: "X has/have Y"
        match = re.match(
            r"^(.+?)\s+(?:has|have)\s+(.+)$", sentence, re.IGNORECASE
        )
        if match:
            subject = match.group(1).strip()[:50]
            obj = match.group(2).strip()[:80]
            if len(subject) > 2 and len(obj) > 2:
                facts.append(f"{subject}|has|{obj}|0.6")
                continue

        # Pattern: "X uses/utilizes Y"
        match = re.match(
            r"^(.+?)\s+(?:uses?|utilizes?)\s+(.+)$", sentence, re.IGNORECASE
        )
        if match:
            subject = match.group(1).strip()[:50]
            obj = match.group(2).strip()[:80]
            if len(subject) > 2 and len(obj) > 2:
                facts.append(f"{subject}|uses|{obj}|0.6")
                continue

        # Pattern: "X contains/includes Y"
        match = re.match(
            r"^(.+?)\s+(?:contains?|includes?)\s+(.+)$", sentence, re.IGNORECASE
        )
        if match:
            subject = match.group(1).strip()[:50]
            obj = match.group(2).strip()[:80]
            if len(subject) > 2 and len(obj) > 2:
                facts.append(f"{subject}|contains|{obj}|0.6")
                continue

    return facts


def scrape_url(url):
    """Scrape a URL and extract facts."""
    # Check robots.txt
    if not check_robots_txt(url):
        sys.stderr.write(f"[BLOCKED] robots.txt disallows access to {url}\n")
        return {"url": url, "allowed": False, "facts": []}

    html = fetch_page(url)
    if html is None:
        return {"url": url, "error": "Failed to fetch page", "facts": []}

    content = extract_text(html)
    facts = extract_facts_from_text(content["full_text"], url)

    return {
        "url": url,
        "headings": content["headings"][:10],
        "paragraph_count": len(content["paragraphs"]),
        "facts": facts,
        "fact_count": len(facts),
    }


def search_topic(topic):
    """
    Search for a topic using a simple Wikipedia lookup.
    Returns facts extracted from the page.
    """
    # Use Wikipedia API for topic search
    encoded_topic = urllib.parse.quote(topic.replace(" ", "_"))
    url = f"https://en.wikipedia.org/wiki/{encoded_topic}"
    return scrape_url(url)


def main():
    parser = argparse.ArgumentParser(
        description="MK OS Web Scraper - Extracts facts from web pages in .mk format"
    )
    parser.add_argument("--url", type=str, help="URL to scrape for facts")
    parser.add_argument(
        "--topic", type=str, help="Topic to search (uses Wikipedia)"
    )
    parser.add_argument(
        "--output-format",
        choices=["json", "mk"],
        default="json",
        help="Output format: json or mk (pipe-delimited facts)",
    )
    parser.add_argument(
        "--max-facts",
        type=int,
        default=50,
        help="Maximum number of facts to extract (default: 50)",
    )
    parser.add_argument(
        "--pretty", action="store_true", help="Pretty-print JSON output"
    )

    args = parser.parse_args()

    if not args.url and not args.topic:
        parser.error("Either --url or --topic is required")

    if args.url:
        result = scrape_url(args.url)
    else:
        result = search_topic(args.topic)

    # Limit facts
    if result.get("facts"):
        result["facts"] = result["facts"][: args.max_facts]
        result["fact_count"] = len(result["facts"])

    # Output
    if args.output_format == "mk":
        for fact in result.get("facts", []):
            print(fact)
    else:
        indent = 2 if args.pretty else None
        print(json.dumps(result, indent=indent))

    return 0


if __name__ == "__main__":
    sys.exit(main())
