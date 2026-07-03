#!/usr/bin/env python3
"""
MK OS - Data Analyzer
Analyzes portfolio performance, system metrics, and learning progress.
Uses standard library only (no numpy). Outputs JSON reports.

Usage:
    python3 data_analyzer.py --help
    python3 data_analyzer.py --input data.json --type portfolio
    python3 data_analyzer.py --input metrics.json --type system
    echo '[1,2,3,4,5]' | python3 data_analyzer.py --type series
"""

import argparse
import json
import math
import sys
from datetime import datetime


def mean(values):
    """Calculate arithmetic mean."""
    if not values:
        return 0.0
    return sum(values) / len(values)


def standard_deviation(values):
    """Calculate population standard deviation."""
    if len(values) < 2:
        return 0.0
    avg = mean(values)
    variance = sum((x - avg) ** 2 for x in values) / len(values)
    return math.sqrt(variance)


def moving_average(values, window=5):
    """Calculate simple moving average with given window size."""
    if not values or window <= 0:
        return []
    result = []
    for i in range(len(values)):
        start = max(0, i - window + 1)
        window_vals = values[start : i + 1]
        result.append(mean(window_vals))
    return result


def exponential_moving_average(values, alpha=0.3):
    """Calculate exponential moving average."""
    if not values:
        return []
    result = [values[0]]
    for i in range(1, len(values)):
        ema = alpha * values[i] + (1 - alpha) * result[-1]
        result.append(ema)
    return result


def detect_trend(values, window=5):
    """
    Detect trend direction using linear regression slope on recent values.
    Returns: 'uptrend', 'downtrend', or 'sideways'
    """
    if len(values) < window:
        return "insufficient_data"

    recent = values[-window:]
    n = len(recent)
    x_mean = (n - 1) / 2.0
    y_mean = mean(recent)

    numerator = sum((i - x_mean) * (recent[i] - y_mean) for i in range(n))
    denominator = sum((i - x_mean) ** 2 for i in range(n))

    if denominator == 0:
        return "sideways"

    slope = numerator / denominator
    # Normalize slope relative to mean
    if y_mean != 0:
        normalized_slope = slope / abs(y_mean)
    else:
        normalized_slope = slope

    if normalized_slope > 0.02:
        return "uptrend"
    elif normalized_slope < -0.02:
        return "downtrend"
    else:
        return "sideways"


def detect_anomalies(values, threshold=2.0):
    """
    Detect anomalies using z-score method.
    Returns list of (index, value, z_score) tuples for anomalous points.
    """
    if len(values) < 3:
        return []

    avg = mean(values)
    std = standard_deviation(values)
    if std == 0:
        return []

    anomalies = []
    for i, val in enumerate(values):
        z_score = (val - avg) / std
        if abs(z_score) > threshold:
            anomalies.append({"index": i, "value": val, "z_score": round(z_score, 3)})

    return anomalies


def analyze_portfolio(data):
    """Analyze portfolio data and generate insights."""
    prices = data.get("prices", [])
    if not prices:
        return {"error": "No price data provided"}

    values = [float(p) for p in prices]

    current = values[-1] if values else 0
    initial = values[0] if values else 0
    pnl = current - initial
    pnl_percent = (pnl / initial * 100) if initial != 0 else 0

    ma_5 = moving_average(values, 5)
    ma_20 = moving_average(values, 20)
    ema = exponential_moving_average(values)

    high = max(values)
    low = min(values)
    drawdown = ((high - current) / high * 100) if high != 0 else 0

    return {
        "type": "portfolio_analysis",
        "timestamp": datetime.utcnow().isoformat() + "Z",
        "summary": {
            "current_value": round(current, 2),
            "initial_value": round(initial, 2),
            "pnl": round(pnl, 2),
            "pnl_percent": round(pnl_percent, 2),
            "high": round(high, 2),
            "low": round(low, 2),
            "max_drawdown_percent": round(drawdown, 2),
        },
        "statistics": {
            "mean": round(mean(values), 4),
            "std_deviation": round(standard_deviation(values), 4),
            "data_points": len(values),
        },
        "moving_averages": {
            "sma_5": [round(v, 4) for v in ma_5[-5:]],
            "sma_20": [round(v, 4) for v in ma_20[-5:]],
            "ema": [round(v, 4) for v in ema[-5:]],
        },
        "trend": detect_trend(values),
        "anomalies": detect_anomalies(values),
        "insights": generate_portfolio_insights(values, pnl_percent),
    }


def generate_portfolio_insights(values, pnl_percent):
    """Generate human-readable insights from portfolio data."""
    insights = []
    trend = detect_trend(values)
    std = standard_deviation(values)
    avg = mean(values)
    volatility = (std / avg * 100) if avg != 0 else 0

    if trend == "uptrend":
        insights.append("Portfolio is in an uptrend. Consider holding positions.")
    elif trend == "downtrend":
        insights.append(
            "Portfolio is trending down. Review stop-loss levels."
        )
    else:
        insights.append("Portfolio is moving sideways. Watch for breakout signals.")

    if volatility > 20:
        insights.append(
            f"High volatility detected ({volatility:.1f}%). Consider reducing position sizes."
        )
    elif volatility < 5:
        insights.append(
            f"Low volatility ({volatility:.1f}%). Market may be consolidating."
        )

    anomalies = detect_anomalies(values)
    if anomalies:
        insights.append(
            f"{len(anomalies)} anomalous price point(s) detected. Investigate unusual moves."
        )

    if pnl_percent > 10:
        insights.append("Strong gains. Consider taking partial profits.")
    elif pnl_percent < -10:
        insights.append("Significant losses. Review risk management strategy.")

    return insights


def analyze_system_metrics(data):
    """Analyze system metrics (CPU, RAM, etc.)."""
    metrics = {}

    for key in ["cpu_percent", "ram_percent", "disk_percent", "temperature"]:
        values = data.get(key, [])
        if not values:
            continue
        values = [float(v) for v in values]
        metrics[key] = {
            "current": round(values[-1], 2),
            "mean": round(mean(values), 2),
            "std_deviation": round(standard_deviation(values), 2),
            "max": round(max(values), 2),
            "min": round(min(values), 2),
            "trend": detect_trend(values),
            "anomalies": detect_anomalies(values),
        }

    insights = []
    cpu = data.get("cpu_percent", [])
    if cpu and mean(cpu) > 70:
        insights.append("High average CPU usage. Consider load balancing.")
    ram = data.get("ram_percent", [])
    if ram and max(ram) > 90:
        insights.append("RAM usage peaked above 90%. Monitor for memory leaks.")
    temp = data.get("temperature", [])
    if temp and max(temp) > 80:
        insights.append("High temperature detected. Check cooling system.")

    return {
        "type": "system_analysis",
        "timestamp": datetime.utcnow().isoformat() + "Z",
        "metrics": metrics,
        "insights": insights,
    }


def analyze_time_series(data):
    """Generic time series analysis."""
    if isinstance(data, list):
        values = [float(v) for v in data]
    elif isinstance(data, dict) and "values" in data:
        values = [float(v) for v in data["values"]]
    else:
        return {"error": "Expected a list of numbers or {values: [...]}"}

    if not values:
        return {"error": "Empty data set"}

    ma_5 = moving_average(values, 5)
    ma_10 = moving_average(values, 10)
    ema = exponential_moving_average(values)

    return {
        "type": "time_series_analysis",
        "timestamp": datetime.utcnow().isoformat() + "Z",
        "statistics": {
            "count": len(values),
            "mean": round(mean(values), 4),
            "std_deviation": round(standard_deviation(values), 4),
            "min": round(min(values), 4),
            "max": round(max(values), 4),
            "range": round(max(values) - min(values), 4),
        },
        "moving_averages": {
            "sma_5_last": round(ma_5[-1], 4) if ma_5 else None,
            "sma_10_last": round(ma_10[-1], 4) if ma_10 else None,
            "ema_last": round(ema[-1], 4) if ema else None,
        },
        "trend": detect_trend(values),
        "anomalies": detect_anomalies(values),
    }


def main():
    parser = argparse.ArgumentParser(
        description="MK OS Data Analyzer - Portfolio, system metrics, and time series analysis"
    )
    parser.add_argument(
        "--input",
        type=str,
        help="Input JSON file (reads from stdin if not provided)",
    )
    parser.add_argument(
        "--type",
        choices=["portfolio", "system", "series"],
        default="series",
        help="Analysis type: portfolio, system, or series (default: series)",
    )
    parser.add_argument(
        "--pretty", action="store_true", help="Pretty-print JSON output"
    )

    args = parser.parse_args()

    # Read input data
    try:
        if args.input:
            with open(args.input, "r") as f:
                data = json.load(f)
        else:
            data = json.load(sys.stdin)
    except json.JSONDecodeError as e:
        sys.stderr.write(f"[ERROR] Invalid JSON input: {e}\n")
        return 1
    except FileNotFoundError:
        sys.stderr.write(f"[ERROR] File not found: {args.input}\n")
        return 1

    # Perform analysis
    if args.type == "portfolio":
        result = analyze_portfolio(data)
    elif args.type == "system":
        result = analyze_system_metrics(data)
    else:
        result = analyze_time_series(data)

    # Output
    indent = 2 if args.pretty else None
    print(json.dumps(result, indent=indent))

    return 0


if __name__ == "__main__":
    sys.exit(main())
