#!/usr/bin/env python3
"""
MK OS - Knowledge Expander
Autonomously expands MK's knowledge base by gathering information about topics,
validating facts, deduplicating against existing .mk files, and outputting
new facts ready for integration.

Usage:
    python3 knowledge_expander.py --help
    python3 knowledge_expander.py --topic "quantum computing"
    python3 knowledge_expander.py --topic "blockchain" --existing-dir ../ai_core/hre/knowledge_files/
    python3 knowledge_expander.py --topic "python programming" --output new_facts.mk
"""

import argparse
import json
import os
import re
import sys
from datetime import datetime

# Import sibling module for web scraping
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
try:
    from web_scraper import search_topic, scrape_url
except ImportError:
    # Fallback if import fails
    def search_topic(topic):
        return {"facts": [], "error": "web_scraper module not available"}

    def scrape_url(url):
        return {"facts": [], "error": "web_scraper module not available"}


def load_existing_facts(directory):
    """Load all existing facts from .mk files in the given directory."""
    existing = set()
    if not os.path.isdir(directory):
        return existing

    for filename in os.listdir(directory):
        if not filename.endswith(".mk"):
            continue
        filepath = os.path.join(directory, filename)
        try:
            with open(filepath, "r", encoding="utf-8", errors="replace") as f:
                for line in f:
                    line = line.strip()
                    if line and not line.startswith("#"):
                        # Normalize for comparison
                        existing.add(line.lower())
        except IOError:
            continue

    return existing


def validate_fact(fact_line):
    """
    Validate that a fact line is in correct .mk format.
    Format: source|relation|target|weight
    Returns (is_valid, reason)
    """
    parts = fact_line.split("|")
    if len(parts) < 3:
        return False, "Too few pipe-separated fields (need at least 3)"
    if len(parts) > 4:
        return False, "Too many pipe-separated fields (max 4)"

    source = parts[0].strip()
    relation = parts[1].strip()
    target = parts[2].strip()

    if not source:
        return False, "Empty source field"
    if not relation:
        return False, "Empty relation field"
    if not target:
        return False, "Empty target field"

    if len(source) > 100:
        return False, "Source too long (max 100 chars)"
    if len(relation) > 50:
        return False, "Relation too long (max 50 chars)"
    if len(target) > 200:
        return False, "Target too long (max 200 chars)"

    # Check weight if present
    if len(parts) == 4:
        try:
            weight = float(parts[3].strip())
            if weight < 0 or weight > 1:
                return False, "Weight must be between 0 and 1"
        except ValueError:
            return False, "Weight must be a number"

    # Check for invalid characters
    for part in parts[:3]:
        if "\n" in part or "\r" in part:
            return False, "Fields cannot contain newlines"

    return True, "Valid"


def deduplicate_facts(new_facts, existing_facts):
    """
    Remove facts that already exist in the knowledge base.
    Returns deduplicated list.
    """
    unique = []
    for fact in new_facts:
        normalized = fact.lower().strip()
        if normalized not in existing_facts:
            unique.append(fact)
            existing_facts.add(normalized)  # prevent duplicates within batch too
    return unique


def expand_topic(topic, existing_dir=None, max_facts=100):
    """
    Expand knowledge about a topic by scraping and extracting facts.
    Returns dict with new facts and metadata.
    """
    # Load existing facts for deduplication
    existing = set()
    if existing_dir:
        existing = load_existing_facts(existing_dir)
        sys.stderr.write(f"[INFO] Loaded {len(existing)} existing facts for dedup\n")

    # Gather facts from web
    sys.stderr.write(f"[INFO] Searching for information about: {topic}\n")
    result = search_topic(topic)

    raw_facts = result.get("facts", [])
    sys.stderr.write(f"[INFO] Extracted {len(raw_facts)} raw facts\n")

    # Validate facts
    valid_facts = []
    invalid_count = 0
    for fact in raw_facts:
        is_valid, _ = validate_fact(fact)
        if is_valid:
            valid_facts.append(fact)
        else:
            invalid_count += 1

    sys.stderr.write(f"[INFO] {len(valid_facts)} valid, {invalid_count} invalid\n")

    # Deduplicate
    new_facts = deduplicate_facts(valid_facts, existing)
    sys.stderr.write(f"[INFO] {len(new_facts)} new unique facts after dedup\n")

    # Limit output
    new_facts = new_facts[:max_facts]

    return {
        "topic": topic,
        "timestamp": datetime.utcnow().isoformat() + "Z",
        "source_url": result.get("url", ""),
        "raw_extracted": len(raw_facts),
        "valid": len(valid_facts),
        "duplicates_removed": len(valid_facts) - len(new_facts),
        "new_facts_count": len(new_facts),
        "facts": new_facts,
    }


def main():
    parser = argparse.ArgumentParser(
        description="MK OS Knowledge Expander - Autonomous fact gathering and validation"
    )
    parser.add_argument(
        "--topic", type=str, required=True, help="Topic to expand knowledge about"
    )
    parser.add_argument(
        "--existing-dir",
        type=str,
        default=None,
        help="Directory with existing .mk files for deduplication",
    )
    parser.add_argument(
        "--output",
        type=str,
        default=None,
        help="Output file path for .mk format (default: stdout as JSON)",
    )
    parser.add_argument(
        "--max-facts",
        type=int,
        default=100,
        help="Maximum number of new facts to output (default: 100)",
    )
    parser.add_argument(
        "--format",
        choices=["json", "mk"],
        default="json",
        help="Output format: json or mk (default: json)",
    )
    parser.add_argument(
        "--pretty", action="store_true", help="Pretty-print JSON output"
    )

    args = parser.parse_args()

    result = expand_topic(args.topic, args.existing_dir, args.max_facts)

    if args.output:
        # Write to file in .mk format
        with open(args.output, "w") as f:
            f.write(f"# MK OS Knowledge Expansion: {args.topic}\n")
            f.write(f"# Generated: {result['timestamp']}\n")
            f.write(f"# Source: {result.get('source_url', 'web search')}\n\n")
            for fact in result["facts"]:
                f.write(fact + "\n")
        sys.stderr.write(f"[INFO] Wrote {len(result['facts'])} facts to {args.output}\n")
    elif args.format == "mk":
        for fact in result["facts"]:
            print(fact)
    else:
        indent = 2 if args.pretty else None
        print(json.dumps(result, indent=indent))

    return 0


if __name__ == "__main__":
    sys.exit(main())
