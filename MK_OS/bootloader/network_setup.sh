#!/bin/sh
# =============================================================================
# MK OS - Network Configuration Script
# =============================================================================
# Sets up networking inside the Alpine Linux VM.
# Called by mk_init.sh during boot sequence.
# =============================================================================

LOG_TAG="[MK NET]"

log() {
    echo "${LOG_TAG} $(date '+%H:%M:%S') $*"
}

# --- Step 1: Bring up loopback ---
log "Bringing up loopback interface..."
ip link set lo up

# --- Step 2: Bring up eth0 with DHCP ---
log "Configuring eth0 with DHCP..."
ip link set eth0 up 2>/dev/null

# Wait briefly for link to come up
sleep 2

# Try udhcpc first (busybox DHCP client, available on Alpine)
if command -v udhcpc >/dev/null 2>&1; then
    log "Using udhcpc for DHCP..."
    udhcpc -i eth0 -q -t 10 -n 2>/dev/null
    DHCP_RESULT=$?
elif command -v dhcpcd >/dev/null 2>&1; then
    log "Using dhcpcd for DHCP..."
    dhcpcd eth0 2>/dev/null
    DHCP_RESULT=$?
elif command -v dhclient >/dev/null 2>&1; then
    log "Using dhclient for DHCP..."
    dhclient eth0 2>/dev/null
    DHCP_RESULT=$?
else
    log "WARNING: No DHCP client found!"
    DHCP_RESULT=1
fi

if [ ${DHCP_RESULT} -ne 0 ]; then
    log "DHCP failed. Attempting static fallback..."
    # Static fallback (common UTM SE NAT range)
    ip addr add 10.0.2.15/24 dev eth0 2>/dev/null || true
    ip route add default via 10.0.2.2 2>/dev/null || true
    log "Static IP configured: 10.0.2.15/24, gateway 10.0.2.2"
fi

# --- Step 3: Configure DNS ---
log "Configuring DNS resolvers..."
cat > /etc/resolv.conf << 'EOF'
# MK OS DNS Configuration
nameserver 8.8.8.8
nameserver 1.1.1.1
nameserver 8.8.4.4
EOF

log "DNS configured (Google 8.8.8.8, Cloudflare 1.1.1.1)"

# --- Step 4: Set up iptables firewall ---
log "Setting up iptables firewall rules..."

# Check if iptables is available
if command -v iptables >/dev/null 2>&1; then
    # Flush existing rules
    iptables -F
    iptables -X
    iptables -t nat -F

    # Default policies
    iptables -P INPUT DROP
    iptables -P FORWARD DROP
    iptables -P OUTPUT ACCEPT

    # Allow loopback
    iptables -A INPUT -i lo -j ACCEPT
    iptables -A OUTPUT -o lo -j ACCEPT

    # Allow established and related connections (responses to our requests)
    iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT

    # Allow SSH inbound (port 22) for remote management
    iptables -A INPUT -p tcp --dport 22 -j ACCEPT

    # Allow ICMP (ping) for diagnostics
    iptables -A INPUT -p icmp -j ACCEPT

    # Allow DHCP responses
    iptables -A INPUT -p udp --sport 67 --dport 68 -j ACCEPT

    # Drop everything else inbound
    iptables -A INPUT -j DROP

    log "Firewall configured: outbound allowed, inbound blocked except SSH"
else
    log "WARNING: iptables not available. No firewall configured."
fi

# --- Step 5: Test connectivity ---
log "Testing network connectivity..."

# Test DNS resolution
CONNECTIVITY_OK=0

if command -v ping >/dev/null 2>&1; then
    if ping -c 1 -W 5 8.8.8.8 >/dev/null 2>&1; then
        log "  Connectivity to 8.8.8.8: OK"
        CONNECTIVITY_OK=1
    else
        log "  Connectivity to 8.8.8.8: FAILED"
    fi
fi

# Test DNS
if command -v nslookup >/dev/null 2>&1; then
    if nslookup google.com >/dev/null 2>&1; then
        log "  DNS resolution: OK"
    else
        log "  DNS resolution: FAILED"
    fi
elif command -v wget >/dev/null 2>&1; then
    if wget -q --spider http://google.com 2>/dev/null; then
        log "  HTTP connectivity: OK"
        CONNECTIVITY_OK=1
    else
        log "  HTTP connectivity: FAILED"
    fi
fi

if [ ${CONNECTIVITY_OK} -eq 1 ]; then
    log "Network is operational."
else
    log "WARNING: Network connectivity could not be verified."
    log "MK OS features requiring internet (/search, /weather) may not work."
fi

# --- Display network info ---
log "Network configuration:"
log "  IP: $(ip -4 addr show eth0 2>/dev/null | grep inet | awk '{print $2}')"
log "  Gateway: $(ip route show default 2>/dev/null | awk '{print $3}')"
log "  DNS: $(cat /etc/resolv.conf | grep nameserver | head -1 | awk '{print $2}')"

log "Network setup complete."
exit 0
