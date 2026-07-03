#!/usr/bin/env python3
"""
MK OS - Crypto Price Monitor
Fetches real-time cryptocurrency prices from CoinGecko free API.
Outputs JSON with prices, 24h changes, volume, and market cap.
Rate limited to 10 requests per minute.

Usage:
    python3 crypto_monitor.py --help
    python3 crypto_monitor.py --coins bitcoin,ethereum,solana
    python3 crypto_monitor.py --top 10
"""

import argparse
import json
import sys
import time
import urllib.request
import urllib.error
import urllib.parse
from datetime import datetime


# Rate limiting state
_request_times = []
MAX_REQUESTS_PER_MINUTE = 10
COINGECKO_BASE_URL = "https://api.coingecko.com/api/v3"


def check_rate_limit():
    """Check and enforce rate limit (10 requests per minute)."""
    global _request_times
    now = time.time()
    # Remove timestamps older than 60 seconds
    _request_times = [t for t in _request_times if now - t < 60]
    if len(_request_times) >= MAX_REQUESTS_PER_MINUTE:
        wait_time = 60 - (now - _request_times[0])
        if wait_time > 0:
            sys.stderr.write(
                f"[RATE LIMIT] Waiting {wait_time:.1f}s before next request\n"
            )
            time.sleep(wait_time)
            _request_times = []
    _request_times.append(time.time())


def fetch_json(url, timeout=30):
    """Fetch JSON from a URL with error handling."""
    check_rate_limit()
    try:
        req = urllib.request.Request(
            url,
            headers={"User-Agent": "MK-OS-CryptoMonitor/1.0", "Accept": "application/json"},
        )
        with urllib.request.urlopen(req, timeout=timeout) as response:
            data = response.read().decode("utf-8")
            return json.loads(data)
    except urllib.error.HTTPError as e:
        sys.stderr.write(f"[ERROR] HTTP {e.code}: {e.reason}\n")
        if e.code == 429:
            sys.stderr.write("[ERROR] Rate limited by CoinGecko. Wait and retry.\n")
        return None
    except urllib.error.URLError as e:
        sys.stderr.write(f"[ERROR] Connection failed: {e.reason}\n")
        return None
    except json.JSONDecodeError as e:
        sys.stderr.write(f"[ERROR] Invalid JSON response: {e}\n")
        return None
    except Exception as e:
        sys.stderr.write(f"[ERROR] Unexpected error: {e}\n")
        return None


def get_coin_prices(coin_ids, currency="usd"):
    """
    Fetch prices for a list of coin IDs from CoinGecko.
    Returns list of coin data dicts.
    """
    if not coin_ids:
        return []

    ids_param = ",".join(coin_ids)
    url = (
        f"{COINGECKO_BASE_URL}/coins/markets?"
        f"vs_currency={currency}&ids={ids_param}"
        f"&order=market_cap_desc&per_page=100&page=1"
        f"&sparkline=false&price_change_percentage=24h"
    )

    data = fetch_json(url)
    if data is None:
        return []

    results = []
    for coin in data:
        results.append(
            {
                "id": coin.get("id", ""),
                "symbol": coin.get("symbol", "").upper(),
                "name": coin.get("name", ""),
                "price": coin.get("current_price", 0),
                "change_24h": coin.get("price_change_percentage_24h", 0),
                "volume": coin.get("total_volume", 0),
                "market_cap": coin.get("market_cap", 0),
                "high_24h": coin.get("high_24h", 0),
                "low_24h": coin.get("low_24h", 0),
                "last_updated": coin.get("last_updated", ""),
            }
        )

    return results


def get_top_coins(count=10, currency="usd"):
    """Fetch top N coins by market cap."""
    url = (
        f"{COINGECKO_BASE_URL}/coins/markets?"
        f"vs_currency={currency}&order=market_cap_desc"
        f"&per_page={count}&page=1&sparkline=false"
        f"&price_change_percentage=24h"
    )

    data = fetch_json(url)
    if data is None:
        return []

    results = []
    for coin in data:
        results.append(
            {
                "id": coin.get("id", ""),
                "symbol": coin.get("symbol", "").upper(),
                "name": coin.get("name", ""),
                "price": coin.get("current_price", 0),
                "change_24h": coin.get("price_change_percentage_24h", 0),
                "volume": coin.get("total_volume", 0),
                "market_cap": coin.get("market_cap", 0),
                "high_24h": coin.get("high_24h", 0),
                "low_24h": coin.get("low_24h", 0),
                "last_updated": coin.get("last_updated", ""),
            }
        )

    return results


def get_global_market():
    """Fetch global crypto market overview."""
    url = f"{COINGECKO_BASE_URL}/global"
    data = fetch_json(url)
    if data is None:
        return None

    global_data = data.get("data", {})
    return {
        "total_market_cap_usd": global_data.get("total_market_cap", {}).get("usd", 0),
        "total_volume_24h_usd": global_data.get("total_volume", {}).get("usd", 0),
        "bitcoin_dominance": global_data.get("market_cap_percentage", {}).get("btc", 0),
        "active_cryptocurrencies": global_data.get("active_cryptocurrencies", 0),
        "market_cap_change_24h": global_data.get(
            "market_cap_change_percentage_24h_usd", 0
        ),
    }


def main():
    parser = argparse.ArgumentParser(
        description="MK OS Crypto Price Monitor - Fetches live crypto data from CoinGecko"
    )
    parser.add_argument(
        "--coins",
        type=str,
        default="bitcoin,ethereum,solana",
        help="Comma-separated list of coin IDs (default: bitcoin,ethereum,solana)",
    )
    parser.add_argument(
        "--top",
        type=int,
        default=0,
        help="Fetch top N coins by market cap instead of specific coins",
    )
    parser.add_argument(
        "--currency",
        type=str,
        default="usd",
        help="Currency for prices (default: usd)",
    )
    parser.add_argument(
        "--global-market",
        action="store_true",
        help="Include global market overview",
    )
    parser.add_argument(
        "--pretty", action="store_true", help="Pretty-print JSON output"
    )

    args = parser.parse_args()

    output = {"timestamp": datetime.utcnow().isoformat() + "Z", "currency": args.currency}

    # Fetch coin data
    if args.top > 0:
        coins = get_top_coins(args.top, args.currency)
    else:
        coin_ids = [c.strip() for c in args.coins.split(",") if c.strip()]
        coins = get_coin_prices(coin_ids, args.currency)

    output["coins"] = coins

    # Optionally include global market
    if args.global_market:
        global_data = get_global_market()
        if global_data:
            output["global_market"] = global_data

    # Output JSON
    indent = 2 if args.pretty else None
    print(json.dumps(output, indent=indent))

    return 0


if __name__ == "__main__":
    sys.exit(main())
