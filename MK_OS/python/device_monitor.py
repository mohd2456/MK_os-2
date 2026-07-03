#!/usr/bin/env python3
"""
MK OS - Device Monitor
Reports system metrics as JSON. Works on Linux with graceful fallback on other OS.
Monitors: CPU, RAM, temperature, disk, load average, uptime.

Usage:
    python3 device_monitor.py --help
    python3 device_monitor.py
    python3 device_monitor.py --pretty
    python3 device_monitor.py --interval 5
"""

import argparse
import json
import os
import platform
import sys
import time
from datetime import datetime


def get_cpu_percent():
    """Get CPU usage percentage from /proc/stat (Linux) or fallback."""
    try:
        if platform.system() == "Linux":
            with open("/proc/stat", "r") as f:
                line = f.readline()
            parts = line.split()
            # First reading
            idle1 = int(parts[4])
            total1 = sum(int(p) for p in parts[1:])
            time.sleep(0.1)
            with open("/proc/stat", "r") as f:
                line = f.readline()
            parts = line.split()
            idle2 = int(parts[4])
            total2 = sum(int(p) for p in parts[1:])
            idle_delta = idle2 - idle1
            total_delta = total2 - total1
            if total_delta == 0:
                return 0.0
            return round((1.0 - idle_delta / total_delta) * 100, 1)
        else:
            # macOS/other fallback using os.getloadavg
            load = os.getloadavg()
            cpu_count = os.cpu_count() or 1
            return round(min(load[0] / cpu_count * 100, 100), 1)
    except Exception:
        return -1.0


def get_ram_info():
    """Get RAM usage in MB."""
    try:
        if platform.system() == "Linux":
            with open("/proc/meminfo", "r") as f:
                lines = f.readlines()
            mem_info = {}
            for line in lines:
                parts = line.split()
                key = parts[0].rstrip(":")
                value = int(parts[1])  # in kB
                mem_info[key] = value

            total_mb = mem_info.get("MemTotal", 0) / 1024
            available_mb = mem_info.get("MemAvailable", 0) / 1024
            used_mb = total_mb - available_mb
            return {
                "used_mb": round(used_mb, 1),
                "total_mb": round(total_mb, 1),
                "available_mb": round(available_mb, 1),
                "percent": round((used_mb / total_mb * 100) if total_mb > 0 else 0, 1),
            }
        else:
            # macOS fallback using sysctl
            try:
                import subprocess

                result = subprocess.run(
                    ["sysctl", "-n", "hw.memsize"],
                    capture_output=True,
                    text=True,
                    timeout=5,
                )
                total_bytes = int(result.stdout.strip())
                total_mb = total_bytes / (1024 * 1024)
                # Cannot easily get used RAM without psutil, estimate from load
                load = os.getloadavg()[0]
                est_percent = min(load * 10 + 30, 95)
                used_mb = total_mb * est_percent / 100
                return {
                    "used_mb": round(used_mb, 1),
                    "total_mb": round(total_mb, 1),
                    "available_mb": round(total_mb - used_mb, 1),
                    "percent": round(est_percent, 1),
                }
            except Exception:
                return {"used_mb": -1, "total_mb": -1, "available_mb": -1, "percent": -1}
    except Exception:
        return {"used_mb": -1, "total_mb": -1, "available_mb": -1, "percent": -1}


def get_temperature():
    """Read CPU temperature from thermal zones (Linux)."""
    temps = []
    try:
        if platform.system() == "Linux":
            thermal_base = "/sys/class/thermal/"
            if os.path.isdir(thermal_base):
                for entry in os.listdir(thermal_base):
                    if entry.startswith("thermal_zone"):
                        temp_file = os.path.join(thermal_base, entry, "temp")
                        if os.path.exists(temp_file):
                            with open(temp_file, "r") as f:
                                temp_raw = int(f.read().strip())
                                temps.append(temp_raw / 1000.0)  # millidegrees to C
        if temps:
            return round(max(temps), 1)
        return -1.0
    except Exception:
        return -1.0


def get_disk_free():
    """Get free disk space in GB for root partition."""
    try:
        stat = os.statvfs("/")
        free_gb = (stat.f_bavail * stat.f_frsize) / (1024 * 1024 * 1024)
        total_gb = (stat.f_blocks * stat.f_frsize) / (1024 * 1024 * 1024)
        return {
            "free_gb": round(free_gb, 2),
            "total_gb": round(total_gb, 2),
            "used_percent": round((1 - free_gb / total_gb) * 100, 1) if total_gb > 0 else 0,
        }
    except Exception:
        return {"free_gb": -1, "total_gb": -1, "used_percent": -1}


def get_load_average():
    """Get system load average."""
    try:
        load = os.getloadavg()
        return {"1min": round(load[0], 2), "5min": round(load[1], 2), "15min": round(load[2], 2)}
    except Exception:
        return {"1min": -1, "5min": -1, "15min": -1}


def get_uptime_seconds():
    """Get system uptime in seconds."""
    try:
        if platform.system() == "Linux":
            with open("/proc/uptime", "r") as f:
                uptime = float(f.read().split()[0])
                return round(uptime, 0)
        else:
            # macOS fallback
            import subprocess

            result = subprocess.run(
                ["sysctl", "-n", "kern.boottime"],
                capture_output=True,
                text=True,
                timeout=5,
            )
            # Parse: { sec = 1234567890, usec = 0 }
            import re

            match = re.search(r"sec\s*=\s*(\d+)", result.stdout)
            if match:
                boot_time = int(match.group(1))
                return round(time.time() - boot_time, 0)
        return -1
    except Exception:
        return -1


def get_hostname():
    """Get the system hostname."""
    try:
        return platform.node()
    except Exception:
        return "unknown"


def collect_metrics():
    """Collect all system metrics."""
    return {
        "hostname": get_hostname(),
        "platform": platform.system(),
        "architecture": platform.machine(),
        "timestamp": datetime.utcnow().isoformat() + "Z",
        "cpu_percent": get_cpu_percent(),
        "ram": get_ram_info(),
        "temperature_c": get_temperature(),
        "disk": get_disk_free(),
        "load_average": get_load_average(),
        "uptime_seconds": get_uptime_seconds(),
        "cpu_count": os.cpu_count() or -1,
    }


def main():
    parser = argparse.ArgumentParser(
        description="MK OS Device Monitor - Reports system metrics as JSON"
    )
    parser.add_argument(
        "--pretty", action="store_true", help="Pretty-print JSON output"
    )
    parser.add_argument(
        "--interval",
        type=int,
        default=0,
        help="Continuous monitoring interval in seconds (0 = single shot)",
    )
    parser.add_argument(
        "--compact",
        action="store_true",
        help="Output only essential metrics (cpu, ram, temp)",
    )

    args = parser.parse_args()
    indent = 2 if args.pretty else None

    if args.interval > 0:
        # Continuous monitoring mode
        try:
            while True:
                metrics = collect_metrics()
                if args.compact:
                    metrics = {
                        "cpu_percent": metrics["cpu_percent"],
                        "ram_used_mb": metrics["ram"]["used_mb"],
                        "ram_total_mb": metrics["ram"]["total_mb"],
                        "temperature_c": metrics["temperature_c"],
                        "disk_free_gb": metrics["disk"]["free_gb"],
                        "timestamp": metrics["timestamp"],
                    }
                print(json.dumps(metrics, indent=indent))
                sys.stdout.flush()
                time.sleep(args.interval)
        except KeyboardInterrupt:
            pass
    else:
        # Single shot
        metrics = collect_metrics()
        if args.compact:
            metrics = {
                "cpu_percent": metrics["cpu_percent"],
                "ram_used_mb": metrics["ram"]["used_mb"],
                "ram_total_mb": metrics["ram"]["total_mb"],
                "temperature_c": metrics["temperature_c"],
                "disk_free_gb": metrics["disk"]["free_gb"],
                "load_average": metrics["load_average"],
                "uptime_seconds": metrics["uptime_seconds"],
                "timestamp": metrics["timestamp"],
            }
        print(json.dumps(metrics, indent=indent))

    return 0


if __name__ == "__main__":
    sys.exit(main())
