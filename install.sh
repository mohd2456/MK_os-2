#!/bin/bash
# ============================================================
# MK OS — One-Shot Installer
# ============================================================
# Usage:
#   git clone https://github.com/mohd2456/MK_os-2.git
#   cd MK_os-2
#   ./install.sh
#
# OR (curl one-liner):
#   curl -sSL https://raw.githubusercontent.com/mohd2456/MK_os-2/main/install.sh | bash
# ============================================================

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

MK_HOME="${HOME}/.mk_os"
MK_BINARY="/usr/local/bin/mk"

# ─────────────────────────────────────────────
# Helper functions
# ─────────────────────────────────────────────

print_banner() {
    echo -e "${CYAN}"
    echo "  ╔══════════════════════════════════════════════╗"
    echo "  ║        MK OS — Installer for Linux          ║"
    echo "  ║   Hybrid AI that runs locally on your PC    ║"
    echo "  ╚══════════════════════════════════════════════╝"
    echo -e "${NC}"
}

info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
success() { echo -e "${GREEN}[✓]${NC} $1"; }
warn()    { echo -e "${YELLOW}[!]${NC} $1"; }
error()   { echo -e "${RED}[✗]${NC} $1"; }

check_root() {
    if [ "$EUID" -eq 0 ]; then
        error "Do NOT run this script as root. It will use sudo when needed."
        exit 1
    fi
}

# ─────────────────────────────────────────────
# Detect distro and install dependencies
# ─────────────────────────────────────────────

detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO="$ID"
    elif [ -f /etc/arch-release ]; then
        DISTRO="arch"
    elif command -v apt &>/dev/null; then
        DISTRO="ubuntu"
    elif command -v dnf &>/dev/null; then
        DISTRO="fedora"
    else
        DISTRO="unknown"
    fi
    echo "$DISTRO"
}

install_deps() {
    local distro
    distro=$(detect_distro)
    
    info "Detected distro: ${BOLD}${distro}${NC}"
    
    case "$distro" in
        arch|endeavouros|manjaro|garuda|artix)
            info "Installing dependencies via pacman..."
            sudo pacman -Syu --needed --noconfirm \
                base-devel clang curl sqlite git python python-pip
            success "Arch Linux dependencies installed"
            ;;
        ubuntu|debian|pop|linuxmint|zorin)
            info "Installing dependencies via apt..."
            sudo apt update
            sudo apt install -y \
                build-essential clang libcurl4-openssl-dev libsqlite3-dev \
                git python3 python3-pip curl
            success "Debian/Ubuntu dependencies installed"
            ;;
        fedora|rhel|centos|rocky|alma)
            info "Installing dependencies via dnf..."
            sudo dnf install -y \
                gcc-c++ clang libcurl-devel sqlite-devel \
                git python3 python3-pip make curl
            success "Fedora/RHEL dependencies installed"
            ;;
        *)
            warn "Unknown distro '${distro}'. Trying to continue anyway..."
            warn "Make sure you have: clang++, libcurl, libsqlite3, git, python3"
            ;;
    esac
}

# ─────────────────────────────────────────────
# Build MK OS
# ─────────────────────────────────────────────

build_mk() {
    info "Building MK OS..."
    
    # Find the MK_OS directory
    local mk_src=""
    if [ -d "MK_OS" ]; then
        mk_src="MK_OS"
    elif [ -d "../MK_OS" ]; then
        mk_src="../MK_OS"
    else
        error "Cannot find MK_OS source directory!"
        error "Make sure you're running this from the MK_os-2 repo root."
        exit 1
    fi
    
    cd "$mk_src"
    
    # Detect compiler — prefer clang++, fall back to g++
    local compiler="clang++"
    if ! command -v clang++ &>/dev/null; then
        if command -v g++ &>/dev/null; then
            compiler="g++"
            warn "clang++ not found, using g++ instead"
        else
            error "No C++ compiler found! Install clang or gcc."
            exit 1
        fi
    fi
    
    # Detect OS for build flags
    local os_flag="-DLINUX"
    if [[ "$(uname)" == "Darwin" ]]; then
        os_flag="-DDARWIN"
    fi
    
    # Build with appropriate flags
    info "Compiling with ${compiler} (C++17, optimized)..."
    ${compiler} -std=c++17 -O2 -Wall -Wextra ${os_flag} \
        -o mk_os system/mk_entry.cpp \
        -lcurl -lsqlite3 2>&1 || {
        error "Build failed! Check the errors above."
        error "Common fixes:"
        error "  - Install libcurl: sudo pacman -S curl"
        error "  - Install sqlite: sudo pacman -S sqlite"
        exit 1
    }
    
    success "MK OS built successfully!"
    cd ..
}

# ─────────────────────────────────────────────
# Install MK OS
# ─────────────────────────────────────────────

install_mk() {
    info "Installing MK OS to ${MK_HOME}..."
    
    # Create MK home directory
    mkdir -p "${MK_HOME}"
    mkdir -p "${MK_HOME}/knowledge"
    mkdir -p "${MK_HOME}/logs"
    
    # Copy the binary
    cp MK_OS/mk_os "${MK_HOME}/mk_os"
    chmod +x "${MK_HOME}/mk_os"
    success "Binary installed to ${MK_HOME}/mk_os"
    
    # Copy knowledge files
    if [ -f "MK_OS/knowledge/knowledge_data.txt" ]; then
        cp MK_OS/knowledge/knowledge_data.txt "${MK_HOME}/knowledge/"
        success "Knowledge data copied"
    fi
    
    # Copy any .mk knowledge files that exist
    find MK_OS/ -name "*.mk" -exec cp {} "${MK_HOME}/knowledge/" \; 2>/dev/null || true
    find MK_OS/ -name "*.dat" -exec cp {} "${MK_HOME}/" \; 2>/dev/null || true
    
    # Create default config if it doesn't exist
    if [ ! -f "${MK_HOME}/config" ]; then
        cat > "${MK_HOME}/config" << 'EOF'
# MK OS Configuration
# -------------------
# Telegram Bot Token (get from @BotFather on Telegram)
MK_TELEGRAM_TOKEN=""

# Encryption key for memory (auto-generated if empty)
MK_ENCRYPT_KEY=""

# Auto-update from GitHub (yes/no)
MK_AUTO_UPDATE="yes"

# Knowledge directory
MK_KNOWLEDGE_DIR="${HOME}/.mk_os/knowledge"

# Log level (debug, info, warn, error)
MK_LOG_LEVEL="info"
EOF
        success "Default config created at ${MK_HOME}/config"
    else
        warn "Config already exists, not overwriting"
    fi
    
    # Install the launcher script
    info "Installing 'mk' command..."
    sudo tee /usr/local/bin/mk > /dev/null << 'LAUNCHER'
#!/bin/bash
# ═══════════════════════════════════════════
# MK OS Launcher — Type 'mk' to start chat
# ═══════════════════════════════════════════

MK_HOME="${HOME}/.mk_os"
MK_BINARY="${MK_HOME}/mk_os"

# Ensure directories exist
mkdir -p "${MK_HOME}/knowledge"
mkdir -p "${MK_HOME}/logs"

# Check binary exists
if [ ! -f "${MK_BINARY}" ]; then
    echo "ERROR: MK OS binary not found at ${MK_BINARY}"
    echo "Run the installer again: cd MK_os-2 && ./install.sh"
    exit 1
fi

# Load config
if [ -f "${MK_HOME}/config" ]; then
    set -a
    source "${MK_HOME}/config"
    set +a
fi

# Export important paths
export MK_HOME
export MK_KNOWLEDGE_DIR="${MK_HOME}/knowledge"

# Run MK from its home directory (so knowledge files are found)
cd "${MK_HOME}"
exec "${MK_BINARY}" "$@"
LAUNCHER
    sudo chmod +x /usr/local/bin/mk
    success "Command 'mk' installed to /usr/local/bin/mk"
}

# ─────────────────────────────────────────────
# Telegram setup (optional)
# ─────────────────────────────────────────────

setup_telegram() {
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════${NC}"
    echo -e "${BOLD}  Telegram Bot Setup (optional)${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════${NC}"
    echo ""
    echo "  To use MK from your phone via Telegram:"
    echo "  1. Open Telegram, search for @BotFather"
    echo "  2. Send /newbot and follow the steps"
    echo "  3. Copy the bot token"
    echo ""
    echo -n "  Paste your Telegram bot token (or press Enter to skip): "
    read -r token
    
    if [ -n "$token" ]; then
        # Update config with token
        sed -i "s|^MK_TELEGRAM_TOKEN=.*|MK_TELEGRAM_TOKEN=\"${token}\"|" "${MK_HOME}/config"
        success "Telegram token saved!"
        echo ""
        info "MK will connect to Telegram when you run 'mk'"
        info "You can message your bot from your phone anytime."
    else
        info "Skipped. You can add it later:"
        echo "  echo 'MK_TELEGRAM_TOKEN=\"your_token\"' >> ~/.mk_os/config"
    fi
}

# ─────────────────────────────────────────────
# Auto-update setup
# ─────────────────────────────────────────────

setup_autoupdate() {
    # Save repo location for auto-updates
    local repo_dir
    repo_dir="$(pwd)"
    echo "MK_REPO_DIR=\"${repo_dir}\"" >> "${MK_HOME}/config"
}

# ─────────────────────────────────────────────
# Main
# ─────────────────────────────────────────────

main() {
    print_banner
    check_root
    
    echo -e "${BOLD}This will install MK OS on your system.${NC}"
    echo "  • Dependencies via your package manager"
    echo "  • MK binary at ~/.mk_os/mk_os"
    echo "  • 'mk' command at /usr/local/bin/mk"
    echo "  • Knowledge files at ~/.mk_os/knowledge/"
    echo ""
    echo -n "Continue? [Y/n] "
    read -r confirm
    if [[ "$confirm" =~ ^[Nn] ]]; then
        echo "Cancelled."
        exit 0
    fi
    echo ""
    
    # Step 1: Install dependencies
    info "Step 1/4: Installing dependencies..."
    install_deps
    echo ""
    
    # Step 2: Build MK OS
    info "Step 2/4: Building MK OS..."
    build_mk
    echo ""
    
    # Step 3: Install MK
    info "Step 3/4: Installing MK OS..."
    install_mk
    echo ""
    
    # Step 4: Telegram (optional)
    info "Step 4/4: Optional setup..."
    setup_telegram
    setup_autoupdate
    echo ""
    
    # Done!
    echo ""
    echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}${BOLD}  ✓ MK OS installed successfully!${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"
    echo ""
    echo -e "  ${BOLD}To start chatting with MK, just type:${NC}"
    echo ""
    echo -e "    ${CYAN}mk${NC}"
    echo ""
    echo -e "  ${BOLD}MK lives at:${NC} ~/.mk_os/"
    echo -e "  ${BOLD}Config:${NC} ~/.mk_os/config"
    echo -e "  ${BOLD}Knowledge:${NC} ~/.mk_os/knowledge/"
    echo ""
    echo -e "  ${BOLD}Telegram:${NC} Message your bot from your phone!"
    echo -e "  ${BOLD}Update:${NC} cd MK_os-2 && git pull && ./install.sh"
    echo ""
    echo -e "  Enjoy chatting with MK! 🧠"
    echo ""
}

# Handle curl pipe install (no repo cloned yet)
if [ ! -d "MK_OS" ] && [ ! -d "../MK_OS" ]; then
    info "No local repo detected. Cloning from GitHub..."
    git clone https://github.com/mohd2456/MK_os-2.git /tmp/mk_os_install
    cd /tmp/mk_os_install
fi

main "$@"
