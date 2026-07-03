#!/bin/bash
# ============================================================
# MK OS - Network Discovery
# Discovers devices on the local network using ping sweep or arp.
# Outputs JSON array of discovered hosts.
# ============================================================

set -euo pipefail

# Configuration
TIMEOUT="${SCAN_TIMEOUT:-1}"
SUBNET="${SCAN_SUBNET:-}"

# Detect local subnet if not specified
detect_subnet() {
    if [ -n "$SUBNET" ]; then
        echo "$SUBNET"
        return
    fi

    # Try to detect from default route interface
    local iface
    iface=$(ip route show default 2>/dev/null | awk '{print $5}' | head -1 || echo "")

    if [ -z "$iface" ]; then
        # Fallback: try common interfaces
        for try_iface in eth0 wlan0 enp0s3 ens33; do
            if ip link show "$try_iface" &>/dev/null; then
                iface="$try_iface"
                break
            fi
        done
    fi

    if [ -z "$iface" ]; then
        echo "192.168.1.0/24"
        return
    fi

    # Get subnet from interface
    local addr
    addr=$(ip -4 addr show "$iface" 2>/dev/null | grep 'inet ' | awk '{print $2}' | head -1 || echo "")
    if [ -n "$addr" ]; then
        echo "$addr"
    else
        echo "192.168.1.0/24"
    fi
}

# Ping sweep discovery
ping_sweep() {
    local subnet="$1"
    local base_ip

    # Extract base IP (e.g., 192.168.1 from 192.168.1.0/24 or 192.168.1.100/24)
    base_ip=$(echo "$subnet" | cut -d'/' -f1 | rev | cut -d'.' -f2- | rev)

    local results="["
    local first=true

    for i in $(seq 1 254); do
        local ip="${base_ip}.${i}"
        if ping -c 1 -W "$TIMEOUT" "$ip" &>/dev/null; then
            local hostname
            hostname=$(getent hosts "$ip" 2>/dev/null | awk '{print $2}' || echo "")
            local mac
            mac=$(arp -n "$ip" 2>/dev/null | grep -v incomplete | awk '{print $3}' | grep -v Address || echo "")

            if [ "$first" = "true" ]; then
                first=false
            else
                results+=","
            fi

            results+="{\"ip\":\"${ip}\",\"mac\":\"${mac:-unknown}\",\"hostname\":\"${hostname:-unknown}\"}"
        fi
    done &

    # Wait with timeout (max 60 seconds for full scan)
    wait

    results+="]"
    echo "$results"
}

# ARP-based discovery (faster, requires arp-scan)
arp_discovery() {
    local subnet="$1"
    local results="["
    local first=true

    if command -v arp-scan &>/dev/null; then
        while IFS=$'\t' read -r ip mac vendor; do
            if [[ "$ip" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
                local hostname
                hostname=$(getent hosts "$ip" 2>/dev/null | awk '{print $2}' || echo "")

                if [ "$first" = "true" ]; then
                    first=false
                else
                    results+=","
                fi
                results+="{\"ip\":\"${ip}\",\"mac\":\"${mac:-unknown}\",\"hostname\":\"${hostname:-unknown}\",\"vendor\":\"${vendor:-unknown}\"}"
            fi
        done < <(arp-scan --localnet --quiet 2>/dev/null || echo "")
    else
        # Fallback: read ARP table
        while read -r ip _ _ mac _ iface; do
            if [[ "$ip" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]] && [ "$mac" != "00:00:00:00:00:00" ]; then
                local hostname
                hostname=$(getent hosts "$ip" 2>/dev/null | awk '{print $2}' || echo "")

                if [ "$first" = "true" ]; then
                    first=false
                else
                    results+=","
                fi
                results+="{\"ip\":\"${ip}\",\"mac\":\"${mac:-unknown}\",\"hostname\":\"${hostname:-unknown}\"}"
            fi
        done < <(arp -n 2>/dev/null | tail -n +2 || echo "")
    fi

    results+="]"
    echo "$results"
}

# Main
main() {
    local method="${1:-arp}"
    local subnet
    subnet=$(detect_subnet)

    echo "{" >&2
    echo "  \"scan_method\": \"${method}\"," >&2
    echo "  \"subnet\": \"${subnet}\"," >&2
    echo "  \"timestamp\": \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\"" >&2
    echo "}" >&2

    case "$method" in
        ping)
            ping_sweep "$subnet"
            ;;
        arp|*)
            arp_discovery "$subnet"
            ;;
    esac
}

main "$@"
