#!/bin/bash
# ============================================================
# MK OS - Hardware Monitor
# Reads thermal zones, meminfo, cpuinfo, disk space.
# Outputs unified JSON for the C++ resource monitor.
# ============================================================

set -euo pipefail

# Get CPU temperature from thermal zones
get_temperature() {
    local temp=-1
    if [ -d /sys/class/thermal ]; then
        for zone in /sys/class/thermal/thermal_zone*/temp; do
            if [ -f "$zone" ]; then
                local raw
                raw=$(cat "$zone" 2>/dev/null || echo "0")
                local celsius=$(( raw / 1000 ))
                if [ "$celsius" -gt "$temp" ]; then
                    temp=$celsius
                fi
            fi
        done
    fi
    echo "$temp"
}

# Get memory info from /proc/meminfo
get_memory() {
    if [ -f /proc/meminfo ]; then
        local total_kb available_kb used_kb
        total_kb=$(grep '^MemTotal:' /proc/meminfo | awk '{print $2}')
        available_kb=$(grep '^MemAvailable:' /proc/meminfo | awk '{print $2}')
        used_kb=$(( total_kb - available_kb ))
        local total_mb=$(( total_kb / 1024 ))
        local used_mb=$(( used_kb / 1024 ))
        local available_mb=$(( available_kb / 1024 ))
        local percent=0
        if [ "$total_kb" -gt 0 ]; then
            percent=$(( used_kb * 100 / total_kb ))
        fi
        echo "{\"total_mb\":${total_mb},\"used_mb\":${used_mb},\"available_mb\":${available_mb},\"percent\":${percent}}"
    else
        echo "{\"total_mb\":-1,\"used_mb\":-1,\"available_mb\":-1,\"percent\":-1}"
    fi
}

# Get CPU info
get_cpu_info() {
    local cores=-1
    local model="unknown"
    if [ -f /proc/cpuinfo ]; then
        cores=$(grep -c '^processor' /proc/cpuinfo 2>/dev/null || echo "-1")
        model=$(grep '^model name' /proc/cpuinfo 2>/dev/null | head -1 | cut -d: -f2 | xargs || echo "unknown")
    fi
    # Get load average
    local load1 load5 load15
    if [ -f /proc/loadavg ]; then
        read -r load1 load5 load15 _ _ < /proc/loadavg
    else
        load1="0" ; load5="0" ; load15="0"
    fi
    echo "{\"cores\":${cores},\"model\":\"${model}\",\"load_1m\":${load1},\"load_5m\":${load5},\"load_15m\":${load15}}"
}

# Get disk space
get_disk_info() {
    local disk_json="["
    local first=true
    while IFS= read -r line; do
        local device mount total used available percent
        device=$(echo "$line" | awk '{print $1}')
        mount=$(echo "$line" | awk '{print $6}')
        total=$(echo "$line" | awk '{print $2}')
        used=$(echo "$line" | awk '{print $3}')
        available=$(echo "$line" | awk '{print $4}')
        percent=$(echo "$line" | awk '{print $5}' | tr -d '%')

        if [ "$first" = "true" ]; then
            first=false
        else
            disk_json+=","
        fi
        disk_json+="{\"device\":\"${device}\",\"mount\":\"${mount}\",\"total_mb\":${total},\"used_mb\":${used},\"available_mb\":${available},\"percent\":${percent}}"
    done < <(df -BM --output=source,size,used,avail,pcent,target 2>/dev/null | tail -n +2 | grep '^/' || echo "/dev/sda1 0M 0M 0M 0% /")
    disk_json+="]"
    echo "$disk_json"
}

# Get uptime
get_uptime() {
    if [ -f /proc/uptime ]; then
        local uptime_sec
        uptime_sec=$(cut -d' ' -f1 /proc/uptime | cut -d'.' -f1)
        echo "$uptime_sec"
    else
        echo "-1"
    fi
}

# Build full JSON output
main() {
    local temp
    temp=$(get_temperature)

    local memory
    memory=$(get_memory)

    local cpu
    cpu=$(get_cpu_info)

    local disks
    disks=$(get_disk_info)

    local uptime_sec
    uptime_sec=$(get_uptime)

    local timestamp
    timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

    cat <<EOF
{
  "timestamp": "${timestamp}",
  "hostname": "$(hostname)",
  "temperature_c": ${temp},
  "memory": ${memory},
  "cpu": ${cpu},
  "disks": ${disks},
  "uptime_seconds": ${uptime_sec}
}
EOF
}

main "$@"
