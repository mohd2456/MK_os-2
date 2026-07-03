#!/usr/bin/env python3
"""
MK OS - Airdrop Tracker
Maintains a JSON database of DeFi airdrop opportunities.
Tracks protocols, requirements, deadlines, estimated values, and completion.

Usage:
    python3 airdrop_tracker.py --help
    python3 airdrop_tracker.py list
    python3 airdrop_tracker.py add --name "Protocol X" --requirements "Bridge 0.1 ETH" --deadline "2025-03-01" --value 500
    python3 airdrop_tracker.py update --name "Protocol X" --status completed
    python3 airdrop_tracker.py summary
"""

import argparse
import json
import os
import sys
from datetime import datetime


DEFAULT_DB_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "airdrops.json")


def load_database(db_path):
    """Load the airdrop database from JSON file."""
    if not os.path.exists(db_path):
        return {"airdrops": [], "last_updated": None}
    try:
        with open(db_path, "r") as f:
            return json.load(f)
    except (json.JSONDecodeError, IOError):
        return {"airdrops": [], "last_updated": None}


def save_database(db_path, data):
    """Save the airdrop database to JSON file."""
    data["last_updated"] = datetime.utcnow().isoformat() + "Z"
    with open(db_path, "w") as f:
        json.dump(data, f, indent=2)


def add_airdrop(db, name, protocol="", requirements="", deadline="", estimated_value=0, category="defi", notes=""):
    """Add a new airdrop to the database."""
    # Check for duplicates
    for airdrop in db["airdrops"]:
        if airdrop["name"].lower() == name.lower():
            return False, f"Airdrop '{name}' already exists. Use 'update' to modify."

    airdrop = {
        "name": name,
        "protocol": protocol or name,
        "requirements": requirements,
        "deadline": deadline,
        "estimated_value_usd": estimated_value,
        "category": category,
        "status": "pending",
        "completion_percent": 0,
        "notes": notes,
        "added_date": datetime.utcnow().isoformat() + "Z",
        "last_updated": datetime.utcnow().isoformat() + "Z",
    }
    db["airdrops"].append(airdrop)
    return True, f"Added airdrop: {name}"


def update_airdrop(db, name, status=None, completion=None, notes=None, value=None):
    """Update an existing airdrop entry."""
    for airdrop in db["airdrops"]:
        if airdrop["name"].lower() == name.lower():
            if status:
                airdrop["status"] = status
            if completion is not None:
                airdrop["completion_percent"] = completion
            if notes:
                airdrop["notes"] = notes
            if value is not None:
                airdrop["estimated_value_usd"] = value
            airdrop["last_updated"] = datetime.utcnow().isoformat() + "Z"
            return True, f"Updated airdrop: {name}"
    return False, f"Airdrop '{name}' not found."


def remove_airdrop(db, name):
    """Remove an airdrop from the database."""
    for i, airdrop in enumerate(db["airdrops"]):
        if airdrop["name"].lower() == name.lower():
            db["airdrops"].pop(i)
            return True, f"Removed airdrop: {name}"
    return False, f"Airdrop '{name}' not found."


def list_airdrops(db, status_filter=None):
    """List all airdrops, optionally filtered by status."""
    airdrops = db["airdrops"]
    if status_filter:
        airdrops = [a for a in airdrops if a["status"] == status_filter]
    return airdrops


def get_summary(db):
    """Get a summary of all tracked airdrops."""
    airdrops = db["airdrops"]
    if not airdrops:
        return {
            "total": 0,
            "pending": 0,
            "in_progress": 0,
            "completed": 0,
            "expired": 0,
            "total_estimated_value": 0,
            "completed_value": 0,
        }

    total_value = sum(a.get("estimated_value_usd", 0) for a in airdrops)
    completed_value = sum(
        a.get("estimated_value_usd", 0) for a in airdrops if a["status"] == "completed"
    )

    status_counts = {}
    for a in airdrops:
        s = a.get("status", "pending")
        status_counts[s] = status_counts.get(s, 0) + 1

    return {
        "total": len(airdrops),
        "pending": status_counts.get("pending", 0),
        "in_progress": status_counts.get("in_progress", 0),
        "completed": status_counts.get("completed", 0),
        "expired": status_counts.get("expired", 0),
        "total_estimated_value": total_value,
        "completed_value": completed_value,
        "upcoming_deadlines": get_upcoming_deadlines(airdrops),
    }


def get_upcoming_deadlines(airdrops, limit=5):
    """Get airdrops with nearest deadlines."""
    with_deadlines = []
    now = datetime.utcnow().isoformat()
    for a in airdrops:
        if a.get("deadline") and a["status"] not in ("completed", "expired"):
            if a["deadline"] > now[:10]:
                with_deadlines.append(
                    {"name": a["name"], "deadline": a["deadline"], "value": a.get("estimated_value_usd", 0)}
                )
    with_deadlines.sort(key=lambda x: x["deadline"])
    return with_deadlines[:limit]


def main():
    parser = argparse.ArgumentParser(
        description="MK OS Airdrop Tracker - Track DeFi airdrop opportunities"
    )
    parser.add_argument(
        "--db",
        type=str,
        default=DEFAULT_DB_PATH,
        help=f"Database file path (default: {DEFAULT_DB_PATH})",
    )
    parser.add_argument(
        "--pretty", action="store_true", help="Pretty-print JSON output"
    )

    subparsers = parser.add_subparsers(dest="command", help="Command to execute")

    # List command
    list_parser = subparsers.add_parser("list", help="List all tracked airdrops")
    list_parser.add_argument(
        "--status",
        choices=["pending", "in_progress", "completed", "expired"],
        help="Filter by status",
    )

    # Add command
    add_parser = subparsers.add_parser("add", help="Add a new airdrop")
    add_parser.add_argument("--name", required=True, help="Airdrop/protocol name")
    add_parser.add_argument("--protocol", default="", help="Protocol name")
    add_parser.add_argument("--requirements", default="", help="Requirements to qualify")
    add_parser.add_argument("--deadline", default="", help="Deadline (YYYY-MM-DD)")
    add_parser.add_argument("--value", type=float, default=0, help="Estimated value in USD")
    add_parser.add_argument("--category", default="defi", help="Category (defi, nft, gaming, etc)")
    add_parser.add_argument("--notes", default="", help="Additional notes")

    # Update command
    update_parser = subparsers.add_parser("update", help="Update an existing airdrop")
    update_parser.add_argument("--name", required=True, help="Airdrop name to update")
    update_parser.add_argument(
        "--status",
        choices=["pending", "in_progress", "completed", "expired"],
        help="New status",
    )
    update_parser.add_argument("--completion", type=int, help="Completion percentage (0-100)")
    update_parser.add_argument("--notes", help="Updated notes")
    update_parser.add_argument("--value", type=float, help="Updated estimated value")

    # Remove command
    remove_parser = subparsers.add_parser("remove", help="Remove an airdrop")
    remove_parser.add_argument("--name", required=True, help="Airdrop name to remove")

    # Summary command
    subparsers.add_parser("summary", help="Show summary of all tracked airdrops")

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return 0

    db = load_database(args.db)
    indent = 2 if args.pretty else None

    if args.command == "list":
        airdrops = list_airdrops(db, args.status)
        print(json.dumps({"airdrops": airdrops, "count": len(airdrops)}, indent=indent))

    elif args.command == "add":
        success, message = add_airdrop(
            db, args.name, args.protocol, args.requirements,
            args.deadline, args.value, args.category, args.notes
        )
        if success:
            save_database(args.db, db)
        print(json.dumps({"success": success, "message": message}, indent=indent))

    elif args.command == "update":
        success, message = update_airdrop(
            db, args.name, args.status, args.completion, args.notes, args.value
        )
        if success:
            save_database(args.db, db)
        print(json.dumps({"success": success, "message": message}, indent=indent))

    elif args.command == "remove":
        success, message = remove_airdrop(db, args.name)
        if success:
            save_database(args.db, db)
        print(json.dumps({"success": success, "message": message}, indent=indent))

    elif args.command == "summary":
        summary = get_summary(db)
        print(json.dumps(summary, indent=indent))

    return 0


if __name__ == "__main__":
    sys.exit(main())
