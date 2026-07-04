#!/bin/bash
# ============================================================
# MK OS — Universal Quick Deploy Script
# ============================================================
# One-command deployment for Ubuntu/Debian VPS
#
# Supported: Ubuntu 20.04, 22.04, 24.04 | Debian 11, 12
#
# Usage:
#   # Interactive (will prompt for Telegram token):
#   bash quick_deploy.sh
#
#   # With token as argument:
#   bash quick_deploy.sh YOUR_TELEGRAM_BOT_TOKEN
#
#   # With environment variable:
#   MK_TELEGRAM_TOKEN="YOUR_TOKEN" bash quick_deploy.sh
#
# What this does:
#   1. Installs system dependencies (git, g++, make, libcurl, sqlite3)
#   2. Clones MK OS from GitHub
#   3. Builds the MK binary
#   4. Creates a dedicated 'mk' system user (security)
#   5. Sets up systemd service (auto-start, auto-restart on crash)
#   6. Starts MK immediately
#   7. Configures logging via journalctl
#
# After deployment, talk to MK on Telegram and send /setkey
# to configure your LLM API key for full AI capabilities.
# ============================================================

set -euo pipefail

# ─────────────────────────────────────────────
# Configuration
# ─────────────────────────────────────────────
REPO_URL="https://github.com/mohd2456/MK_os-2.git"
REPO_BRANCH="main"
INSTALL_DIR="/opt/mk-os"
SERVICE_USER="mk"
SERVICE_NAME="mk-os"
CONFIG_DIR="/etc/mk-os"
LOG_TAG="mk-os"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# ─────────────────────────────────────────────
# Helper Functions
# ─────────────────────────────────────────────
info()  { echo -e "${BLUE}[INFO]${NC} $1"; }
ok()    { echo -e "${GREEN}[✓]${NC} $1"; }
warn()  { echo -e "${YELLOW}[!]${NC} $1"; }
error() { echo -e "${RED}[✗]${NC} $1"; exit 1; }

banner() {
    echo ""
    echo -e "${BOLD}╔══════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}║         MK OS — Quick Deploy Script              ║${NC}"
    echo -e "${BOLD}║         Living AI • Telegram • 24/7              ║${NC}"
    echo -e "${BOLD}╚══════════════════════════════════════════════════╝${NC}"
    echo ""
}

# ─────────────────────────────────────────────
# Pre-flight Checks
# ─────────────────────────────────────────────
preflight() {
    # Must be root or have sudo
    if [ "$(id -u)" -ne 0 ]; then
        if ! command -v sudo &> /dev/null; then
            error "This script requires root privileges. Run with: sudo bash quick_deploy.sh"
        fi
        SUDO="sudo"
    else
        SUDO=""
    fi

    # Check OS compatibility
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        case "$ID" in
            ubuntu|debian)
                info "Detected: $PRETTY_NAME"
                ;;
            *)
                warn "Detected: $PRETTY_NAME (not officially supported, trying anyway...)"
                ;;
        esac
    else
        warn "Cannot detect OS. Proceeding anyway..."
    fi
}

# ─────────────────────────────────────────────
# Step 1: Get Telegram Token
# ─────────────────────────────────────────────
get_telegram_token() {
    # Priority: argument > environment variable > interactive prompt
    if [ -n "${1:-}" ]; then
        TELEGRAM_TOKEN="$1"
        info "Using Telegram token from command argument"
    elif [ -n "${MK_TELEGRAM_TOKEN:-}" ]; then
        TELEGRAM_TOKEN="$MK_TELEGRAM_TOKEN"
        info "Using Telegram token from MK_TELEGRAM_TOKEN environment variable"
    else
        echo ""
        echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${BOLD}  Telegram Bot Token Required${NC}"
        echo ""
        echo "  To get a token:"
        echo "  1. Open Telegram and search for @BotFather"
        echo "  2. Send /newbot and follow the prompts"
        echo "  3. Copy the token BotFather gives you"
        echo ""
        echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo ""
        read -rp "  Enter your Telegram Bot Token: " TELEGRAM_TOKEN
        echo ""
    fi

    # Validate token format (basic check: contains a colon)
    if [[ ! "$TELEGRAM_TOKEN" == *":"* ]]; then
        error "Invalid token format. Telegram tokens look like: 123456789:ABCdefGHIjklMNOpqrsTUVwxyz"
    fi

    ok "Telegram token validated"
}

# ─────────────────────────────────────────────
# Step 2: Install System Dependencies
# ─────────────────────────────────────────────
install_dependencies() {
    info "Installing system dependencies..."

    # Update package lists
    $SUDO apt-get update -qq

    # Install required packages
    $SUDO apt-get install -y -qq \
        git \
        g++ \
        make \
        libcurl4-openssl-dev \
        libsqlite3-dev \
        curl \
        python3 \
        python3-pip \
        2>&1 | tail -5

    ok "System dependencies installed"
}

# ─────────────────────────────────────────────
# Step 3: Clone or Update Repository
# ─────────────────────────────────────────────
clone_or_update_repo() {
    if [ -d "$INSTALL_DIR/.git" ]; then
        info "MK OS already installed — updating..."
        cd "$INSTALL_DIR"
        $SUDO git fetch origin
        $SUDO git reset --hard "origin/$REPO_BRANCH"
        ok "Repository updated to latest"
    else
        info "Cloning MK OS repository..."
        $SUDO rm -rf "$INSTALL_DIR"
        $SUDO git clone --depth 1 --branch "$REPO_BRANCH" "$REPO_URL" "$INSTALL_DIR"
        ok "Repository cloned to $INSTALL_DIR"
    fi
}

# ─────────────────────────────────────────────
# Step 4: Build MK
# ─────────────────────────────────────────────
build_mk() {
    info "Building MK OS..."

    cd "$INSTALL_DIR/MK_OS"

    # Build with g++ for Linux (the Makefile defaults to clang++ for macOS)
    $SUDO g++ -std=c++17 -O2 -Wall -DLINUX \
        -o mk_os system/mk_entry.cpp \
        -lcurl -lsqlite3

    if [ ! -f mk_os ]; then
        error "Build failed — mk_os binary not found"
    fi

    $SUDO chmod +x mk_os
    ok "MK OS built successfully"
}

# ─────────────────────────────────────────────
# Step 5: Create Service User
# ─────────────────────────────────────────────
create_service_user() {
    if id "$SERVICE_USER" &>/dev/null; then
        info "Service user '$SERVICE_USER' already exists"
    else
        info "Creating dedicated service user '$SERVICE_USER'..."
        $SUDO useradd --system --no-create-home --shell /usr/sbin/nologin "$SERVICE_USER"
        ok "Service user created (no login shell, no home = secure)"
    fi

    # Set ownership of install directory
    $SUDO chown -R "$SERVICE_USER":"$SERVICE_USER" "$INSTALL_DIR"
}

# ─────────────────────────────────────────────
# Step 6: Configure MK
# ─────────────────────────────────────────────
configure_mk() {
    info "Configuring MK OS..."

    # Create config directory
    $SUDO mkdir -p "$CONFIG_DIR"

    # Write environment file (secrets stored here, not in repo)
    $SUDO tee "$CONFIG_DIR/mk-os.env" > /dev/null << ENVEOF
# MK OS Configuration
# Edit this file to change settings: sudo nano $CONFIG_DIR/mk-os.env
# After editing, restart MK: sudo systemctl restart $SERVICE_NAME

# Telegram Bot Token (required)
MK_TELEGRAM_TOKEN=$TELEGRAM_TOKEN

# LLM API Key (optional but recommended for full AI capabilities)
# Get a free key from https://console.groq.com (email signup, no card needed)
# GROQ_API_KEY=gsk_your_key_here

# Alternative LLM providers (uncomment one):
# OPENROUTER_API_KEY=your_key_here
# HF_API_KEY=your_key_here
ENVEOF

    # Secure the env file (only root and service user can read)
    $SUDO chmod 600 "$CONFIG_DIR/mk-os.env"
    $SUDO chown root:$SERVICE_USER "$CONFIG_DIR/mk-os.env"
    $SUDO chmod 640 "$CONFIG_DIR/mk-os.env"

    ok "Configuration written to $CONFIG_DIR/mk-os.env"
}

# ─────────────────────────────────────────────
# Step 7: Set Up systemd Service
# ─────────────────────────────────────────────
setup_systemd() {
    info "Setting up systemd service..."

    # Stop existing service if running (update flow)
    if systemctl is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
        warn "Stopping existing MK OS service for update..."
        $SUDO systemctl stop "$SERVICE_NAME"
    fi

    # Write systemd unit file
    $SUDO tee /etc/systemd/system/${SERVICE_NAME}.service > /dev/null << SVCEOF
[Unit]
Description=MK OS — Living AI (Telegram Bot)
Documentation=https://github.com/mohd2456/MK_os-2
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=$SERVICE_USER
Group=$SERVICE_USER
WorkingDirectory=$INSTALL_DIR/MK_OS
EnvironmentFile=$CONFIG_DIR/mk-os.env
ExecStart=$INSTALL_DIR/MK_OS/mk_os --daemon

# Auto-restart on crash (wait 10 seconds between restarts)
Restart=always
RestartSec=10

# Security hardening
NoNewPrivileges=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=$INSTALL_DIR/MK_OS
PrivateTmp=true

# Resource limits (safe defaults)
MemoryMax=1G
MemoryHigh=768M
CPUQuota=80%

# Logging goes to journalctl
StandardOutput=journal
StandardError=journal
SyslogIdentifier=$LOG_TAG

[Install]
WantedBy=multi-user.target
SVCEOF

    # Reload systemd, enable and start
    $SUDO systemctl daemon-reload
    $SUDO systemctl enable "$SERVICE_NAME"

    ok "systemd service configured (auto-start on boot, auto-restart on crash)"
}

# ─────────────────────────────────────────────
# Step 8: Start MK
# ─────────────────────────────────────────────
start_mk() {
    info "Starting MK OS..."
    $SUDO systemctl start "$SERVICE_NAME"

    # Wait a moment and check if it's running
    sleep 3
    if systemctl is-active --quiet "$SERVICE_NAME"; then
        ok "MK OS is running!"
    else
        warn "MK OS may have failed to start. Checking logs..."
        $SUDO journalctl -u "$SERVICE_NAME" -n 20 --no-pager
        echo ""
        warn "Check full logs with: sudo journalctl -u $SERVICE_NAME -f"
    fi
}

# ─────────────────────────────────────────────
# Final Summary
# ─────────────────────────────────────────────
print_summary() {
    echo ""
    echo -e "${GREEN}══════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}${BOLD}  ✓ MK OS DEPLOYED SUCCESSFULLY!${NC}"
    echo -e "${GREEN}══════════════════════════════════════════════════════${NC}"
    echo ""
    echo -e "  ${BOLD}Telegram:${NC} Open Telegram, find your bot, and send a message!"
    echo -e "  ${BOLD}Set LLM key:${NC} Send ${YELLOW}/setkey${NC} to your bot in Telegram"
    echo ""
    echo -e "  ${BOLD}─── Useful Commands ───${NC}"
    echo ""
    echo -e "  ${BLUE}Check status:${NC}    sudo systemctl status $SERVICE_NAME"
    echo -e "  ${BLUE}View logs:${NC}       sudo journalctl -u $SERVICE_NAME -f"
    echo -e "  ${BLUE}Restart MK:${NC}      sudo systemctl restart $SERVICE_NAME"
    echo -e "  ${BLUE}Stop MK:${NC}         sudo systemctl stop $SERVICE_NAME"
    echo -e "  ${BLUE}Edit config:${NC}     sudo nano $CONFIG_DIR/mk-os.env"
    echo ""
    echo -e "  ${BOLD}─── Add LLM API Key (for full AI brain) ───${NC}"
    echo ""
    echo "  1. Get a free key from https://console.groq.com"
    echo "  2. Edit config: sudo nano $CONFIG_DIR/mk-os.env"
    echo "  3. Uncomment GROQ_API_KEY and paste your key"
    echo "  4. Restart: sudo systemctl restart $SERVICE_NAME"
    echo ""
    echo -e "  ${BOLD}─── Update MK (when new code is pushed) ───${NC}"
    echo ""
    echo "  curl -sSL https://raw.githubusercontent.com/mohd2456/MK_os-2/main/MK_OS/deploy/quick_deploy.sh | bash"
    echo "  (or re-run this script — it handles updates automatically)"
    echo ""
    echo -e "${GREEN}══════════════════════════════════════════════════════${NC}"
    echo ""
}

# ─────────────────────────────────────────────
# Main Execution
# ─────────────────────────────────────────────
main() {
    banner
    preflight
    get_telegram_token "${1:-}"
    install_dependencies
    clone_or_update_repo
    build_mk
    create_service_user
    configure_mk
    setup_systemd
    start_mk
    print_summary
}

# Run it
main "$@"
