#!/usr/bin/env python3
"""
MK OS - Content Generator
Generates content using local Ollama API or template-based fallback.
Supports blog posts, social media posts, and documentation generation.

Usage:
    python3 content_generator.py --help
    python3 content_generator.py --topic "AI in healthcare" --style blog
    python3 content_generator.py --topic "Bitcoin update" --style social
    python3 content_generator.py --topic "API reference" --style docs
"""

import argparse
import json
import sys
import urllib.request
import urllib.error
from datetime import datetime

OLLAMA_URL = "http://localhost:11434"
DEFAULT_MODEL = "llama3.2"


def query_ollama(prompt, model=DEFAULT_MODEL, timeout=60):
    """
    Query local Ollama API for text generation.
    Returns generated text or None if unavailable.
    """
    url = f"{OLLAMA_URL}/api/generate"
    payload = json.dumps({"model": model, "prompt": prompt, "stream": False}).encode(
        "utf-8"
    )

    try:
        req = urllib.request.Request(
            url,
            data=payload,
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        with urllib.request.urlopen(req, timeout=timeout) as response:
            data = json.loads(response.read().decode("utf-8"))
            return data.get("response", "")
    except urllib.error.URLError:
        return None
    except urllib.error.HTTPError:
        return None
    except Exception:
        return None


def check_ollama_available():
    """Check if Ollama is running and accessible."""
    try:
        req = urllib.request.Request(f"{OLLAMA_URL}/api/tags")
        with urllib.request.urlopen(req, timeout=5) as response:
            return response.status == 200
    except Exception:
        return False


def generate_blog_template(topic):
    """Template-based blog post generation (fallback)."""
    return {
        "title": f"Understanding {topic.title()}",
        "type": "blog_post",
        "sections": [
            {
                "heading": "Introduction",
                "content": (
                    f"{topic.title()} is an important and evolving subject that impacts "
                    f"many areas of technology and daily life. In this post, we explore "
                    f"the key aspects and recent developments."
                ),
            },
            {
                "heading": "Background",
                "content": (
                    f"The history of {topic} spans several years of research and "
                    f"development. Understanding the foundations helps us appreciate "
                    f"current advancements."
                ),
            },
            {
                "heading": "Current State",
                "content": (
                    f"Today, {topic} has reached a level of maturity that enables "
                    f"practical applications. Key players in the space are pushing "
                    f"boundaries and creating new possibilities."
                ),
            },
            {
                "heading": "Future Outlook",
                "content": (
                    f"The future of {topic} looks promising with continued investment "
                    f"and research. We can expect significant breakthroughs in the "
                    f"coming years."
                ),
            },
            {
                "heading": "Conclusion",
                "content": (
                    f"In summary, {topic} represents a critical area of focus. "
                    f"Staying informed and adapting to changes will be key to "
                    f"leveraging its potential."
                ),
            },
        ],
        "word_count_estimate": 250,
        "reading_time_minutes": 2,
    }


def generate_social_template(topic):
    """Template-based social media content (fallback)."""
    return {
        "type": "social_media",
        "posts": [
            {
                "platform": "twitter",
                "content": (
                    f"Exploring {topic} today. The potential here is massive. "
                    f"Key takeaway: innovation never stops. #tech #future"
                ),
                "char_count": 140,
            },
            {
                "platform": "linkedin",
                "content": (
                    f"Diving deep into {topic}.\n\n"
                    f"What I have learned:\n"
                    f"- The fundamentals are shifting\n"
                    f"- Early adopters have an advantage\n"
                    f"- Continuous learning is essential\n\n"
                    f"What are your thoughts on {topic}? #innovation #technology"
                ),
                "char_count": 300,
            },
            {
                "platform": "short_form",
                "content": (
                    f"Quick update on {topic}: Things are moving fast. "
                    f"Stay tuned for a deeper analysis coming soon."
                ),
                "char_count": 100,
            },
        ],
    }


def generate_docs_template(topic):
    """Template-based documentation generation (fallback)."""
    return {
        "type": "documentation",
        "title": f"{topic.title()} Reference",
        "sections": [
            {
                "heading": "Overview",
                "content": (
                    f"This document provides a reference for {topic}. "
                    f"It covers setup, configuration, and usage."
                ),
            },
            {
                "heading": "Prerequisites",
                "content": (
                    f"Before working with {topic}, ensure you have:\n"
                    f"- Required dependencies installed\n"
                    f"- Proper access credentials configured\n"
                    f"- A basic understanding of the underlying concepts"
                ),
            },
            {
                "heading": "Getting Started",
                "content": (
                    f"To begin using {topic}:\n"
                    f"1. Initialize the environment\n"
                    f"2. Configure your settings\n"
                    f"3. Run the initial setup command\n"
                    f"4. Verify the installation"
                ),
            },
            {
                "heading": "Configuration",
                "content": (
                    f"Configuration for {topic} can be done via:\n"
                    f"- Environment variables\n"
                    f"- Configuration files\n"
                    f"- Command line arguments"
                ),
            },
            {
                "heading": "Troubleshooting",
                "content": (
                    f"Common issues with {topic}:\n"
                    f"- Check logs for error messages\n"
                    f"- Verify network connectivity\n"
                    f"- Ensure correct permissions are set\n"
                    f"- Restart services if needed"
                ),
            },
        ],
    }


def generate_with_ollama(topic, style, model):
    """Generate content using Ollama LLM."""
    prompts = {
        "blog": (
            f"Write a detailed blog post about '{topic}'. "
            f"Include an introduction, main points, and conclusion. "
            f"Keep it informative and engaging. Output as plain text."
        ),
        "social": (
            f"Create 3 social media posts about '{topic}'. "
            f"One short tweet (under 280 chars), one LinkedIn post, "
            f"and one brief update. Format as plain text with labels."
        ),
        "docs": (
            f"Write technical documentation about '{topic}'. "
            f"Include overview, prerequisites, getting started, "
            f"configuration, and troubleshooting sections."
        ),
    }

    prompt = prompts.get(style, prompts["blog"])
    response = query_ollama(prompt, model=model)
    if response:
        return {
            "type": f"{style}_post",
            "source": "ollama",
            "model": model,
            "topic": topic,
            "content": response,
            "generated_at": datetime.utcnow().isoformat() + "Z",
        }
    return None


def main():
    parser = argparse.ArgumentParser(
        description="MK OS Content Generator - Creates content using AI or templates"
    )
    parser.add_argument(
        "--topic", type=str, required=True, help="Topic to generate content about"
    )
    parser.add_argument(
        "--style",
        type=str,
        choices=["blog", "social", "docs"],
        default="blog",
        help="Content style: blog, social, or docs (default: blog)",
    )
    parser.add_argument(
        "--model",
        type=str,
        default=DEFAULT_MODEL,
        help=f"Ollama model to use (default: {DEFAULT_MODEL})",
    )
    parser.add_argument(
        "--force-template",
        action="store_true",
        help="Force template-based generation (skip Ollama)",
    )
    parser.add_argument(
        "--pretty", action="store_true", help="Pretty-print JSON output"
    )

    args = parser.parse_args()

    output = None

    # Try Ollama first unless forced to use templates
    if not args.force_template:
        if check_ollama_available():
            sys.stderr.write("[INFO] Ollama detected. Generating with LLM...\n")
            output = generate_with_ollama(args.topic, args.style, args.model)
            if output:
                sys.stderr.write("[INFO] Content generated successfully via Ollama.\n")
        else:
            sys.stderr.write(
                "[INFO] Ollama not available. Falling back to template generation.\n"
            )

    # Fallback to templates
    if output is None:
        generators = {
            "blog": generate_blog_template,
            "social": generate_social_template,
            "docs": generate_docs_template,
        }
        generator = generators.get(args.style, generate_blog_template)
        output = generator(args.topic)
        output["source"] = "template"
        output["topic"] = args.topic
        output["generated_at"] = datetime.utcnow().isoformat() + "Z"

    # Output JSON
    indent = 2 if args.pretty else None
    print(json.dumps(output, indent=indent))

    return 0


if __name__ == "__main__":
    sys.exit(main())
