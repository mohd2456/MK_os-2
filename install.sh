#!/bin/bash
# ============================================================
# MK OS -- One-Shot Installer (v2.0)
# ============================================================
# Works from a COMPLETELY fresh system. Just run:
#
#   curl -sSL https://raw.githubusercontent.com/mohd2456/MK_os-2/main/install.sh | bash
#
# OR if you already cloned:
#   ./install.sh
#
# Options:
#   ./install.sh              Normal install
#   ./install.sh --update     Update existing installation
#   ./install.sh --uninstall  Remove MK OS from your system
#
# This script handles EVERYTHING:
#   1. Installs git (if missing)
#   2. Clones the repo (if not already in it)
#   3. Installs all dependencies (Linux + macOS)
#   4. Builds MK OS
#   5. Installs 'mk' command
#   6. Sets up Telegram (optional)
#   7. Optionally creates systemd service
# ============================================================

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m' # No Color

MK_HOME="${HOME}/.mk_os"
MK_BINARY="/usr/local/bin/mk"
MK_VERSION="2.0.0"

# -----------------------------------------------
# Helper functions
# -----------------------------------------------

print_banner() {
    echo -e "${CYAN}${BOLD}"
    echo "  +--------------------------------------------------+"
    echo "  |        MK OS -- Installer v${MK_VERSION}              |"
    echo "  |   Hybrid AI that runs locally on your machine    |"
    echo "  +--------------------------------------------------+"
    echo -e "${NC}"
}

info()    { echo -e "  ${BLUE}[INFO]${NC} $1"; }
success() { echo -e "  ${GREEN}[OK]${NC} $1"; }
warn()    { echo -e "  ${YELLOW}[!]${NC} $1"; }
error()   { echo -e "  ${RED}[X]${NC} $1"; }

# Progress bar helper
progress_bar() {
    local current=$1
    local total=$2
    local label=$3
    local width=30
    local percent=$((current * 100 / total))
    local filled=$((current * width / total))
    local empty=$((width - filled))

    printf "\r  ${DIM}[${NC}${GREEN}"
    for ((i=0; i<filled; i++)); do printf "#"; done
    printf "${DIM}"
    for ((i=0; i<empty; i++)); do printf "-"; done
    printf "${NC}${DIM}]${NC} %3d%% %s" "$percent" "$label"
}

check_root() {
    if [ "$EUID" -eq 0 ]; then
        warn "Running as root. Will install without sudo."
        sudo() { "$@"; }
        export -f sudo 2>/dev/null || true
    fi
}

# -----------------------------------------------
# Detect OS and install dependencies
# -----------------------------------------------

detect_os() {
    if [[ "$(uname)" == "Darwin" ]]; then
        echo "macos"
    elif [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "$ID"
    elif [ -f /etc/arch-release ]; then
        echo "arch"
    elif command -v apt &>/dev/null; then
        echo "ubuntu"
    elif command -v dnf &>/dev/null; then
        echo "fedora"
    else
        echo "unknown"
    fi
}

install_deps() {
    local os
    os=$(detect_os)

    info "Detected OS: ${BOLD}${os}${NC}"

    case "$os" in
        macos)
            info "Installing dependencies via Homebrew..."
            # Check if brew is installed
            if ! command -v brew &>/dev/null; then
                warn "Homebrew not found. Installing Homebrew first..."
                /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
                # Add brew to PATH for this session
                if [ -f /opt/homebrew/bin/brew ]; then
                    eval "$(/opt/homebrew/bin/brew shellenv)"
                elif [ -f /usr/local/bin/brew ]; then
                    eval "$(/usr/local/bin/brew shellenv)"
                fi
                success "Homebrew installed"
            fi
            progress_bar 1 4 "Updating brew..."
            brew update >/dev/null 2>&1 || true
            progress_bar 2 4 "Installing compiler..."
            # clang comes with Xcode CLT, but ensure it is there
            if ! command -v clang++ &>/dev/null; then
                xcode-select --install 2>/dev/null || true
            fi
            progress_bar 3 4 "Installing libraries..."
            brew install curl sqlite git python3 2>/dev/null || true
            progress_bar 4 4 "Done!"
            echo ""
            success "macOS dependencies installed"
            ;;
        arch|endeavouros|manjaro|garuda|artix)
            info "Installing dependencies via pacman..."
            progress_bar 1 3 "Syncing packages..."
            sudo pacman -Sy --noconfirm >/dev/null 2>&1
            progress_bar 2 3 "Installing packages..."
            sudo pacman -S --needed --noconfirm \
                base-devel clang curl sqlite git python python-pip >/dev/null 2>&1
            progress_bar 3 3 "Done!"
            echo ""
            success "Arch Linux dependencies installed"
            ;;
        ubuntu|debian|pop|linuxmint|zorin)
            info "Installing dependencies via apt..."
            progress_bar 1 3 "Updating apt..."
            sudo apt update >/dev/null 2>&1
            progress_bar 2 3 "Installing packages..."
            sudo apt install -y \
                build-essential clang libcurl4-openssl-dev libsqlite3-dev \
                git python3 python3-pip curl >/dev/null 2>&1
            progress_bar 3 3 "Done!"
            echo ""
            success "Debian/Ubuntu dependencies installed"
            ;;
        fedora|rhel|centos|rocky|alma)
            info "Installing dependencies via dnf..."
            progress_bar 1 2 "Installing packages..."
            sudo dnf install -y \
                gcc-c++ clang libcurl-devel sqlite-devel \
                git python3 python3-pip make curl >/dev/null 2>&1
            progress_bar 2 2 "Done!"
            echo ""
            success "Fedora/RHEL dependencies installed"
            ;;
        opensuse*|sles)
            info "Installing dependencies via zypper..."
            sudo zypper install -y gcc-c++ clang libcurl-devel sqlite3-devel git python3 make
            success "openSUSE dependencies installed"
            ;;
        *)
            warn "Unknown OS '${os}'. Trying to continue anyway..."
            warn "Make sure you have: clang++ (or g++), libcurl, libsqlite3, git, python3"
            ;;
    esac
}

# -----------------------------------------------
# Build MK OS
# -----------------------------------------------

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

    # Detect compiler
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

    # Build with progress indication
    progress_bar 1 3 "Compiling..."
    ${compiler} -std=c++17 -O2 -Wall -Wextra ${os_flag} \
        -o mk_os system/mk_entry.cpp \
        -lcurl -lsqlite3 2>&1 || {
        echo ""
        error "Build failed! Check the errors above."
        error "Common fixes:"
        error "  - macOS: brew install curl sqlite"
        error "  - Arch: sudo pacman -S curl sqlite"
        error "  - Ubuntu: sudo apt install libcurl4-openssl-dev libsqlite3-dev"
        exit 1
    }
    progress_bar 2 3 "Verifying binary..."

    # Quick verification test: binary should print something and exit
    if [ -f "mk_os" ] && [ -x "mk_os" ]; then
        progress_bar 3 3 "Done!"
        echo ""
        success "MK OS built successfully! ($(du -h mk_os | cut -f1) binary)"
    else
        echo ""
        error "Binary not created. Build may have failed silently."
        exit 1
    fi

    cd ..
}

# -----------------------------------------------
# Install MK OS
# -----------------------------------------------

install_mk() {
    info "Installing MK OS to ${MK_HOME}..."

    # Create directories
    mkdir -p "${MK_HOME}"
    mkdir -p "${MK_HOME}/knowledge"
    mkdir -p "${MK_HOME}/logs"
    mkdir -p "${MK_HOME}/data"

    # Copy the binary
    progress_bar 1 4 "Copying binary..."
    cp MK_OS/mk_os "${MK_HOME}/mk_os"
    chmod +x "${MK_HOME}/mk_os"

    # Copy knowledge files
    progress_bar 2 4 "Copying knowledge..."
    if [ -f "MK_OS/knowledge/knowledge_data.txt" ]; then
        cp MK_OS/knowledge/knowledge_data.txt "${MK_HOME}/knowledge/"
    fi
    # Copy all .mk knowledge files
    find MK_OS/ -name "*.mk" -exec cp {} "${MK_HOME}/knowledge/" \; 2>/dev/null || true
    find MK_OS/ -name "*.dat" -exec cp {} "${MK_HOME}/data/" \; 2>/dev/null || true

    # Create default config if it doesn't exist
    progress_bar 3 4 "Setting up config..."
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
    progress_bar 4 4 "Installing 'mk' command..."
    sudo tee /usr/local/bin/mk > /dev/null << 'LAUNCHER'
#!/bin/bash
# MK OS Launcher

MK_HOME="${HOME}/.mk_os"
MK_BINARY="${MK_HOME}/mk_os"

mkdir -p "${MK_HOME}/knowledge"
mkdir -p "${MK_HOME}/logs"

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

export MK_HOME
export MK_KNOWLEDGE_DIR="${MK_HOME}/knowledge"

cd "${MK_HOME}"
exec "${MK_BINARY}" "$@"
LAUNCHER
    sudo chmod +x /usr/local/bin/mk
    echo ""
    success "Command 'mk' installed to /usr/local/bin/mk"
}

# -----------------------------------------------
# Post-install verification
# -----------------------------------------------

post_install_test() {
    info "Running post-install verification..."

    # Check binary exists and is executable
    if [ ! -x "${MK_HOME}/mk_os" ]; then
        error "Binary not found or not executable"
        return 1
    fi

    # Check knowledge files were copied
    local mk_count
    mk_count=$(find "${MK_HOME}/knowledge" -name "*.mk" 2>/dev/null | wc -l)
    if [ "$mk_count" -gt 0 ]; then
        success "Knowledge files: ${mk_count} files installed"
    else
        warn "No knowledge files found in ${MK_HOME}/knowledge"
    fi

    # Check mk command is available
    if command -v mk &>/dev/null; then
        success "'mk' command is accessible from PATH"
    else
        warn "'mk' not in PATH. Try: export PATH=/usr/local/bin:\$PATH"
    fi

    success "Post-install verification passed"
}

# -----------------------------------------------
# Telegram setup (optional)
# -----------------------------------------------

setup_telegram() {
    echo ""
    echo -e "  ${CYAN}${BOLD}Telegram Bot Setup (optional)${NC}"
    echo -e "  ${DIM}──────────────────────────────────────────${NC}"
    echo ""
    echo "  To use MK from your phone via Telegram:"
    echo "  1. Open Telegram, search for @BotFather"
    echo "  2. Send /newbot and follow the steps"
    echo "  3. Copy the bot token"
    echo ""
    echo -n "  Paste your Telegram bot token (or press Enter to skip): "
    read -r token

    if [ -n "$token" ]; then
        sed -i.bak "s|^MK_TELEGRAM_TOKEN=.*|MK_TELEGRAM_TOKEN=\"${token}\"|" "${MK_HOME}/config" 2>/dev/null || \
            sed -i '' "s|^MK_TELEGRAM_TOKEN=.*|MK_TELEGRAM_TOKEN=\"${token}\"|" "${MK_HOME}/config"
        rm -f "${MK_HOME}/config.bak"
        success "Telegram token saved!"
        info "MK will connect to Telegram when you run 'mk'"
    else
        info "Skipped. You can add it later in ~/.mk_os/config"
    fi
}

# -----------------------------------------------
# Systemd service setup (optional, Linux only)
# -----------------------------------------------

setup_systemd_service() {
    # Only offer on Linux with systemd
    if [[ "$(uname)" != "Linux" ]] || ! command -v systemctl &>/dev/null; then
        return
    fi

    echo ""
    echo -n "  Create a systemd service for background Telegram bot? [y/N] "
    read -r create_service

    if [[ "$create_service" =~ ^[Yy] ]]; then
        local service_file="/etc/systemd/system/mk-os-telegram.service"
        sudo tee "$service_file" > /dev/null << EOF
[Unit]
Description=MK OS Telegram Bot Service
After=network.target

[Service]
Type=simple
User=${USER}
WorkingDirectory=${MK_HOME}
ExecStart=${MK_HOME}/mk_os --telegram-only
Restart=on-failure
RestartSec=10
Environment=MK_HOME=${MK_HOME}

[Install]
WantedBy=multi-user.target
EOF
        sudo systemctl daemon-reload
        success "Systemd service created: mk-os-telegram"
        info "Start with: sudo systemctl start mk-os-telegram"
        info "Enable on boot: sudo systemctl enable mk-os-telegram"
    fi
}

# -----------------------------------------------
# Uninstall
# -----------------------------------------------

uninstall_mk() {
    print_banner
    echo -e "  ${RED}${BOLD}Uninstalling MK OS...${NC}"
    echo ""

    echo -n "  This will remove MK OS from your system. Continue? [y/N] "
    read -r confirm
    if [[ ! "$confirm" =~ ^[Yy] ]]; then
        echo "  Cancelled."
        exit 0
    fi

    # Remove binary
    if [ -f "/usr/local/bin/mk" ]; then
        sudo rm -f /usr/local/bin/mk
        success "Removed /usr/local/bin/mk"
    fi

    # Remove systemd service if it exists
    if [ -f "/etc/systemd/system/mk-os-telegram.service" ]; then
        sudo systemctl stop mk-os-telegram 2>/dev/null || true
        sudo systemctl disable mk-os-telegram 2>/dev/null || true
        sudo rm -f /etc/systemd/system/mk-os-telegram.service
        sudo systemctl daemon-reload 2>/dev/null || true
        success "Removed systemd service"
    fi

    # Ask about data
    echo -n "  Remove all data and knowledge? (~/.mk_os) [y/N] "
    read -r remove_data
    if [[ "$remove_data" =~ ^[Yy] ]]; then
        rm -rf "${MK_HOME}"
        success "Removed ${MK_HOME}"
    else
        info "Kept ${MK_HOME} (your knowledge and config are safe)"
    fi

    echo ""
    success "MK OS has been uninstalled."
    echo ""
    exit 0
}

# -----------------------------------------------
# Update
# -----------------------------------------------

update_mk() {
    print_banner
    echo -e "  ${CYAN}${BOLD}Updating MK OS...${NC}"
    echo ""

    # Try to find repo
    local repo_dir=""
    if [ -d "MK_OS" ]; then
        repo_dir="$(pwd)"
    elif [ -d "../MK_OS" ]; then
        repo_dir="$(cd .. && pwd)"
    elif [ -f "${MK_HOME}/config" ]; then
        # Check config for saved repo dir
        local saved_dir
        saved_dir=$(grep "^MK_REPO_DIR=" "${MK_HOME}/config" 2>/dev/null | cut -d'"' -f2)
        if [ -d "$saved_dir/MK_OS" ]; then
            repo_dir="$saved_dir"
        fi
    fi

    if [ -z "$repo_dir" ]; then
        error "Cannot find MK OS repository."
        error "Please run from the repo directory or re-clone:"
        error "  git clone https://github.com/mohd2456/MK_os-2.git"
        exit 1
    fi

    cd "$repo_dir"

    # Pull latest
    progress_bar 1 4 "Pulling latest..."
    git pull --rebase 2>/dev/null || {
        warn "Git pull failed. Trying fetch + reset..."
        git fetch origin
        git reset --hard origin/main 2>/dev/null || git reset --hard origin/master 2>/dev/null
    }
    echo ""

    # Rebuild
    progress_bar 2 4 "Building..."
    build_mk
    progress_bar 3 4 "Installing..."

    # Copy new binary
    cp MK_OS/mk_os "${MK_HOME}/mk_os"
    chmod +x "${MK_HOME}/mk_os"

    # Update knowledge
    find MK_OS/ -name "*.mk" -exec cp {} "${MK_HOME}/knowledge/" \; 2>/dev/null || true

    progress_bar 4 4 "Done!"
    echo ""
    echo ""
    success "MK OS updated to latest version!"
    echo ""
    exit 0
}

# -----------------------------------------------
# Auto-update setup
# -----------------------------------------------

setup_autoupdate() {
    local repo_dir
    repo_dir="$(pwd)"
    # Save repo location for updates (use grep to avoid duplicates)
    if ! grep -q "^MK_REPO_DIR=" "${MK_HOME}/config" 2>/dev/null; then
        echo "MK_REPO_DIR=\"${repo_dir}\"" >> "${MK_HOME}/config"
    fi
}

# -----------------------------------------------
# Main
# -----------------------------------------------

main() {
    print_banner
    check_root

    echo -e "  ${BOLD}This will install MK OS on your system.${NC}"
    echo "  - Dependencies via your package manager"
    echo "  - MK binary at ~/.mk_os/mk_os"
    echo "  - 'mk' command at /usr/local/bin/mk"
    echo "  - Knowledge files at ~/.mk_os/knowledge/"
    echo ""
    echo -n "  Continue? [Y/n] "
    read -r confirm
    if [[ "$confirm" =~ ^[Nn] ]]; then
        echo "  Cancelled."
        exit 0
    fi
    echo ""

    # Step 1: Install dependencies
    info "Step 1/5: Installing dependencies..."
    install_deps
    echo ""

    # Step 2: Build MK OS
    info "Step 2/5: Building MK OS..."
    build_mk
    echo ""

    # Step 3: Install MK
    info "Step 3/5: Installing MK OS..."
    install_mk
    echo ""

    # Step 4: Verify installation
    info "Step 4/5: Verifying installation..."
    post_install_test
    echo ""

    # Step 5: Optional setup
    info "Step 5/5: Optional setup..."
    setup_telegram
    setup_systemd_service
    setup_autoupdate
    echo ""

    # Done!
    echo ""
    echo -e "  ${GREEN}${BOLD}+--------------------------------------------------+${NC}"
    echo -e "  ${GREEN}${BOLD}|  MK OS installed successfully!                   |${NC}"
    echo -e "  ${GREEN}${BOLD}+--------------------------------------------------+${NC}"
    echo ""
    echo -e "  ${BOLD}To start chatting with MK, just type:${NC}"
    echo ""
    echo -e "    ${CYAN}${BOLD}mk${NC}"
    echo ""
    echo -e "  ${DIM}MK lives at:${NC} ~/.mk_os/"
    echo -e "  ${DIM}Config:${NC} ~/.mk_os/config"
    echo -e "  ${DIM}Knowledge:${NC} ~/.mk_os/knowledge/"
    echo ""
    echo -e "  ${DIM}Update:${NC} ./install.sh --update"
    echo -e "  ${DIM}Uninstall:${NC} ./install.sh --uninstall"
    echo ""
}

# -----------------------------------------------
# Parse arguments and entry point
# -----------------------------------------------

# Handle flags
case "${1:-}" in
    --uninstall|-u)
        uninstall_mk
        ;;
    --update|--upgrade)
        update_mk
        ;;
    --help|-h)
        echo "MK OS Installer v${MK_VERSION}"
        echo ""
        echo "Usage: ./install.sh [option]"
        echo ""
        echo "Options:"
        echo "  (no option)     Install MK OS"
        echo "  --update        Update existing installation"
        echo "  --uninstall     Remove MK OS from system"
        echo "  --help          Show this help"
        echo ""
        exit 0
        ;;
esac

# Handle curl pipe install (no repo cloned yet)
if [ ! -d "MK_OS" ] && [ ! -d "../MK_OS" ]; then
    info "No local repo detected. Need to clone from GitHub..."

    # Install git first if not available
    if ! command -v git &>/dev/null; then
        info "Git not found. Installing git first..."
        if [[ "$(uname)" == "Darwin" ]]; then
            xcode-select --install 2>/dev/null || true
        elif [ -f /etc/arch-release ] || command -v pacman &>/dev/null; then
            pacman -Sy --noconfirm git
        elif command -v apt &>/dev/null; then
            apt update && apt install -y git
        elif command -v dnf &>/dev/null; then
            dnf install -y git
        else
            error "Cannot install git automatically. Please install git manually."
            exit 1
        fi
        success "Git installed!"
    fi

    git clone https://github.com/mohd2456/MK_os-2.git /tmp/mk_os_install
    cd /tmp/mk_os_install
fi

main "$@"
