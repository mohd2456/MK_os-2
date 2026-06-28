#!/bin/bash
# ============================================================
# MK OS - PC Agent Setup Script
# Installs optional dependencies and shows configuration info.
# ============================================================

set -e

echo ""
echo "  ╭─────────────────────────────────────╮"
echo "  │     MK OS PC Agent - Setup          │"
echo "  ╰─────────────────────────────────────╯"
echo ""

# Check Python version
PYTHON_CMD=""
if command -v python3 &> /dev/null; then
    PYTHON_CMD="python3"
elif command -v python &> /dev/null; then
    PYTHON_CMD="python"
else
    echo "  [ERROR] Python 3 not found. Please install Python 3.8+."
    exit 1
fi

PY_VERSION=$($PYTHON_CMD --version 2>&1)
echo "  [OK] Found: $PY_VERSION"

# Install optional dependencies
echo ""
echo "  Installing optional dependencies..."
echo "  (The agent works without these but with reduced functionality)"
echo ""

$PYTHON_CMD -m pip install --quiet psutil 2>/dev/null && \
    echo "  [OK] psutil installed (system info, process management)" || \
    echo "  [SKIP] psutil (optional - system info will be limited)"

$PYTHON_CMD -m pip install --quiet pyperclip 2>/dev/null && \
    echo "  [OK] pyperclip installed (clipboard access)" || \
    echo "  [SKIP] pyperclip (optional - will use OS clipboard commands)"

$PYTHON_CMD -m pip install --quiet Pillow 2>/dev/null && \
    echo "  [OK] Pillow installed (screenshot capture)" || \
    echo "  [SKIP] Pillow (optional - will use OS screenshot commands)"

# Generate a random token if none exists
echo ""
TOKEN_FILE="$HOME/.mk_agent_token"
if [ ! -f "$TOKEN_FILE" ]; then
    TOKEN=$($PYTHON_CMD -c "import secrets; print(secrets.token_hex(16))")
    echo "$TOKEN" > "$TOKEN_FILE"
    chmod 600 "$TOKEN_FILE"
    echo "  [OK] Generated auth token: $TOKEN"
    echo "  [OK] Saved to: $TOKEN_FILE"
else
    TOKEN=$(cat "$TOKEN_FILE")
    echo "  [OK] Existing token found: $TOKEN"
fi

# Show instructions
echo ""
echo "  ─────────────────────────────────────────────"
echo "  HOW TO USE:"
echo "  ─────────────────────────────────────────────"
echo ""
echo "  1. Start the agent on this PC:"
echo "     $PYTHON_CMD agent.py --token $TOKEN"
echo ""
echo "  2. On your Mac (where MK runs), set these env vars:"
echo "     export MK_PC_IP=$(hostname -I 2>/dev/null | awk '{print $1}' || echo '<this-pc-ip>')"
echo "     export MK_PC_PORT=9876"
echo "     export MK_PC_TOKEN=$TOKEN"
echo ""
echo "  3. Start MK OS. It will auto-connect to this PC."
echo ""
echo "  ─────────────────────────────────────────────"
echo "  SECURITY NOTES:"
echo "  ─────────────────────────────────────────────"
echo ""
echo "  - Keep your token SECRET. Anyone with it can control this PC."
echo "  - Use --allow-ips to restrict which IPs can connect."
echo "  - The agent logs all commands for audit."
echo "  - Run on a trusted local network only."
echo ""
