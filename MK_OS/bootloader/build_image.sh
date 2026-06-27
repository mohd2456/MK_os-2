#!/bin/bash
# =============================================================================
# MK OS - Bootable Disk Image Builder for iPhone UTM SE
# =============================================================================
#
# USAGE:
#   ./build_image.sh [OPTIONS]
#
# OPTIONS:
#   --arch aarch64    Target architecture (default: aarch64 for iPhone)
#   --arch x86_64     Build x86_64 image for testing in QEMU on Mac
#   --size 256        Image size in MB (default: 256, small for iPhone storage)
#   --output NAME     Output filename base (default: mk_os_boot)
#   --skip-compile    Skip compilation, use pre-built binary from staging/
#   --help            Show this help message
#
# PREREQUISITES:
#   - Alpine Linux mini rootfs will be downloaded automatically
#   - qemu-img (for qcow2 conversion): brew install qemu
#   - For cross-compilation: aarch64-linux-gnu-g++ or clang with ARM target
#
# OUTPUT:
#   - mk_os_boot.img     Raw disk image (works with UTM SE)
#   - mk_os_boot.qcow2   QCOW2 image (preferred by UTM SE, smaller file)
#
# EXAMPLES:
#   ./build_image.sh                         # Build aarch64 image for iPhone
#   ./build_image.sh --arch x86_64           # Build x86_64 for testing
#   ./build_image.sh --size 512              # Larger image (512MB)
#   ./build_image.sh --skip-compile          # Use pre-built binary
#
# =============================================================================

set -euo pipefail

# --- Configuration ---
ARCH="aarch64"
IMAGE_SIZE_MB=256
OUTPUT_BASE="mk_os_boot"
SKIP_COMPILE=0
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
WORK_DIR="${SCRIPT_DIR}/build_work"
STAGING_DIR="${SCRIPT_DIR}/staging"
ROOTFS_DIR="${WORK_DIR}/rootfs"

# Alpine Linux mini rootfs URLs
ALPINE_VERSION="3.19"
ALPINE_MIRROR="https://dl-cdn.alpinelinux.org/alpine/v${ALPINE_VERSION}/releases"
ALPINE_ROOTFS_AARCH64="${ALPINE_MIRROR}/aarch64/alpine-minirootfs-${ALPINE_VERSION}.0-aarch64.tar.gz"
ALPINE_ROOTFS_X86_64="${ALPINE_MIRROR}/x86_64/alpine-minirootfs-${ALPINE_VERSION}.0-x86_64.tar.gz"

# --- Parse Arguments ---
while [[ $# -gt 0 ]]; do
    case "$1" in
        --arch)
            ARCH="$2"
            shift 2
            ;;
        --size)
            IMAGE_SIZE_MB="$2"
            shift 2
            ;;
        --output)
            OUTPUT_BASE="$2"
            shift 2
            ;;
        --skip-compile)
            SKIP_COMPILE=1
            shift
            ;;
        --help)
            head -35 "$0" | tail -30
            exit 0
            ;;
        *)
            echo "ERROR: Unknown option: $1"
            exit 1
            ;;
    esac
done

# --- Helper Functions ---
log() {
    echo "[MK BUILD] $(date '+%H:%M:%S') $*"
}

error() {
    echo "[MK ERROR] $*" >&2
    exit 1
}

cleanup() {
    log "Cleaning up mounts..."
    if mountpoint -q "${ROOTFS_DIR}/proc" 2>/dev/null; then
        umount "${ROOTFS_DIR}/proc" || true
    fi
    if mountpoint -q "${ROOTFS_DIR}/sys" 2>/dev/null; then
        umount "${ROOTFS_DIR}/sys" || true
    fi
    if mountpoint -q "${ROOTFS_DIR}/dev" 2>/dev/null; then
        umount "${ROOTFS_DIR}/dev" || true
    fi
    if [ -n "${LOOP_DEV:-}" ]; then
        losetup -d "${LOOP_DEV}" 2>/dev/null || true
    fi
}

trap cleanup EXIT

# --- Validate Environment ---
log "Validating build environment..."

check_tool() {
    if ! command -v "$1" &>/dev/null; then
        error "Required tool not found: $1. Install it first."
    fi
}

check_tool dd
check_tool mkfs.ext4 || check_tool mke2fs

# qemu-img is optional but needed for qcow2
HAVE_QEMU_IMG=0
if command -v qemu-img &>/dev/null; then
    HAVE_QEMU_IMG=1
fi

log "Target architecture: ${ARCH}"
log "Image size: ${IMAGE_SIZE_MB}MB"
log "Output: ${OUTPUT_BASE}.img / ${OUTPUT_BASE}.qcow2"

# --- Step 1: Download Alpine Linux mini rootfs ---
log "=== Step 1: Downloading Alpine Linux rootfs ==="

mkdir -p "${WORK_DIR}"

if [ "${ARCH}" = "aarch64" ]; then
    ROOTFS_URL="${ALPINE_ROOTFS_AARCH64}"
elif [ "${ARCH}" = "x86_64" ]; then
    ROOTFS_URL="${ALPINE_ROOTFS_X86_64}"
else
    error "Unsupported architecture: ${ARCH}. Use aarch64 or x86_64."
fi

ROOTFS_TARBALL="${WORK_DIR}/alpine-minirootfs-${ARCH}.tar.gz"

if [ ! -f "${ROOTFS_TARBALL}" ]; then
    log "Downloading Alpine ${ALPINE_VERSION} mini rootfs for ${ARCH}..."
    curl -L -o "${ROOTFS_TARBALL}" "${ROOTFS_URL}" || \
        error "Failed to download Alpine rootfs. Check network connection."
    log "Download complete."
else
    log "Using cached rootfs tarball."
fi

# --- Step 2: Create raw disk image ---
log "=== Step 2: Creating ${IMAGE_SIZE_MB}MB raw disk image ==="

IMAGE_FILE="${SCRIPT_DIR}/${OUTPUT_BASE}.img"

dd if=/dev/zero of="${IMAGE_FILE}" bs=1M count="${IMAGE_SIZE_MB}" status=progress 2>&1
log "Raw image created: ${IMAGE_FILE}"

# --- Step 3: Format and mount the image ---
log "=== Step 3: Formatting image as ext4 ==="

# Create ext4 filesystem on the image
mkfs.ext4 -F -L "MK_OS" "${IMAGE_FILE}"

# Mount the image
mkdir -p "${ROOTFS_DIR}"

if [ "$(uname)" = "Linux" ]; then
    LOOP_DEV=$(losetup --find --show "${IMAGE_FILE}")
    mount "${LOOP_DEV}" "${ROOTFS_DIR}"
elif [ "$(uname)" = "Darwin" ]; then
    # macOS: use hdiutil or fuse-ext2
    if command -v fuse-ext2 &>/dev/null; then
        fuse-ext2 "${IMAGE_FILE}" "${ROOTFS_DIR}" -o rw+
    else
        log "WARNING: On macOS, install fuse-ext2 for direct mount."
        log "Falling back to tar-based assembly (no chroot)."
        # We will assemble the rootfs in a directory and use genext2fs later
        FALLBACK_MODE=1
    fi
fi

# --- Step 4: Extract Alpine rootfs ---
log "=== Step 4: Extracting Alpine rootfs ==="

if [ "${FALLBACK_MODE:-0}" = "1" ]; then
    mkdir -p "${ROOTFS_DIR}"
fi

tar -xzf "${ROOTFS_TARBALL}" -C "${ROOTFS_DIR}"
log "Alpine rootfs extracted."

# --- Step 5: Install packages (Alpine base + deps) ---
log "=== Step 5: Installing Alpine packages ==="

PACKAGES_FILE="${SCRIPT_DIR}/alpine_packages.txt"

if [ "${FALLBACK_MODE:-0}" != "1" ] && [ "$(uname)" = "Linux" ]; then
    # Set up DNS in chroot
    cp /etc/resolv.conf "${ROOTFS_DIR}/etc/resolv.conf" 2>/dev/null || true

    # Mount necessary filesystems for chroot
    mount -t proc proc "${ROOTFS_DIR}/proc"
    mount -t sysfs sys "${ROOTFS_DIR}/sys"
    mount --bind /dev "${ROOTFS_DIR}/dev"

    # Install packages via chroot
    if [ -f "${PACKAGES_FILE}" ]; then
        PACKAGES=$(grep -v '^#' "${PACKAGES_FILE}" | grep -v '^$' | tr '\n' ' ')
        chroot "${ROOTFS_DIR}" /sbin/apk add --no-cache ${PACKAGES}
    fi
else
    log "Skipping package installation (no chroot available)."
    log "Packages will need to be installed on first boot."
    # Copy package list for first-boot installation
    cp "${PACKAGES_FILE}" "${ROOTFS_DIR}/root/alpine_packages.txt" 2>/dev/null || true
fi

# --- Step 6: Compile MK OS or copy pre-built binary ---
log "=== Step 6: Installing MK OS binary ==="

mkdir -p "${ROOTFS_DIR}/opt/mk_os"
mkdir -p "${ROOTFS_DIR}/opt/mk_os/knowledge"
mkdir -p "${ROOTFS_DIR}/var/log"
mkdir -p "${ROOTFS_DIR}/var/run"

if [ "${SKIP_COMPILE}" = "1" ]; then
    # Use pre-built binary from staging directory
    if [ -f "${STAGING_DIR}/mk_os" ]; then
        cp "${STAGING_DIR}/mk_os" "${ROOTFS_DIR}/opt/mk_os/mk_os"
        log "Copied pre-built binary from staging."
    else
        error "No pre-built binary found at ${STAGING_DIR}/mk_os. Build it first with Makefile.arm64."
    fi
else
    # Attempt native compilation inside chroot (only works on matching arch)
    if [ "${FALLBACK_MODE:-0}" != "1" ] && [ "$(uname)" = "Linux" ]; then
        log "Compiling MK OS inside chroot..."
        # Copy source into chroot
        mkdir -p "${ROOTFS_DIR}/tmp/mk_build"
        cp -r "${PROJECT_ROOT}"/*.cpp "${ROOTFS_DIR}/tmp/mk_build/" 2>/dev/null || true
        cp -r "${PROJECT_ROOT}/system" "${ROOTFS_DIR}/tmp/mk_build/" 2>/dev/null || true
        cp -r "${PROJECT_ROOT}/ai_core" "${ROOTFS_DIR}/tmp/mk_build/" 2>/dev/null || true
        cp -r "${PROJECT_ROOT}/network" "${ROOTFS_DIR}/tmp/mk_build/" 2>/dev/null || true
        cp -r "${PROJECT_ROOT}/mk_brain" "${ROOTFS_DIR}/tmp/mk_build/" 2>/dev/null || true
        cp -r "${PROJECT_ROOT}/Makefile" "${ROOTFS_DIR}/tmp/mk_build/" 2>/dev/null || true

        chroot "${ROOTFS_DIR}" sh -c "cd /tmp/mk_build && make all" || \
            log "WARNING: Chroot compilation failed. Use --skip-compile with a pre-built binary."

        if [ -f "${ROOTFS_DIR}/tmp/mk_build/mk_os" ]; then
            cp "${ROOTFS_DIR}/tmp/mk_build/mk_os" "${ROOTFS_DIR}/opt/mk_os/mk_os"
            log "MK OS compiled and installed."
        fi
        rm -rf "${ROOTFS_DIR}/tmp/mk_build"
    else
        log "Cannot compile in this mode. Use --skip-compile with pre-built binary."
        log "Build with: make -f bootloader/Makefile.arm64 static"
    fi
fi

chmod +x "${ROOTFS_DIR}/opt/mk_os/mk_os" 2>/dev/null || true

# --- Step 7: Set up MK OS as auto-start service ---
log "=== Step 7: Configuring auto-start service ==="

# Copy init script
cp "${SCRIPT_DIR}/mk_init.sh" "${ROOTFS_DIR}/opt/mk_os/mk_init.sh"
chmod +x "${ROOTFS_DIR}/opt/mk_os/mk_init.sh"

# Copy network setup script
cp "${SCRIPT_DIR}/network_setup.sh" "${ROOTFS_DIR}/opt/mk_os/network_setup.sh"
chmod +x "${ROOTFS_DIR}/opt/mk_os/network_setup.sh"

# Install OpenRC service
mkdir -p "${ROOTFS_DIR}/etc/init.d"
cp "${SCRIPT_DIR}/mk_service" "${ROOTFS_DIR}/etc/init.d/mk_os"
chmod +x "${ROOTFS_DIR}/etc/init.d/mk_os"

# Enable service on boot (OpenRC)
mkdir -p "${ROOTFS_DIR}/etc/runlevels/default"
ln -sf /etc/init.d/mk_os "${ROOTFS_DIR}/etc/runlevels/default/mk_os" 2>/dev/null || true

# Set up inittab for auto-login and service start
cat > "${ROOTFS_DIR}/etc/inittab" << 'EOF'
::sysinit:/sbin/openrc sysinit
::sysinit:/sbin/openrc boot
::wait:/sbin/openrc default
tty1::respawn:/sbin/getty 38400 tty1
::shutdown:/sbin/openrc shutdown
::ctrlaltdel:/sbin/reboot
EOF

# --- Step 8: Set up networking ---
log "=== Step 8: Configuring networking ==="

# Network interfaces
mkdir -p "${ROOTFS_DIR}/etc/network"
cat > "${ROOTFS_DIR}/etc/network/interfaces" << 'EOF'
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp
EOF

# DNS configuration
cat > "${ROOTFS_DIR}/etc/resolv.conf" << 'EOF'
nameserver 8.8.8.8
nameserver 1.1.1.1
EOF

# Hostname
echo "mk-os" > "${ROOTFS_DIR}/etc/hostname"
echo "127.0.0.1 mk-os localhost" > "${ROOTFS_DIR}/etc/hosts"

# --- Step 9: Copy knowledge files ---
log "=== Step 9: Copying knowledge files ==="

KNOWLEDGE_SRC="${PROJECT_ROOT}/ai_core/hre/knowledge_files"
if [ -d "${KNOWLEDGE_SRC}" ]; then
    cp "${KNOWLEDGE_SRC}"/*.mk "${ROOTFS_DIR}/opt/mk_os/knowledge/" 2>/dev/null || true
    KNOWLEDGE_COUNT=$(ls "${ROOTFS_DIR}/opt/mk_os/knowledge/"*.mk 2>/dev/null | wc -l)
    log "Copied ${KNOWLEDGE_COUNT} knowledge files."
else
    log "WARNING: Knowledge files directory not found at ${KNOWLEDGE_SRC}"
fi

# Also copy any other .mk files from the project
find "${PROJECT_ROOT}" -name "*.mk" -not -path "*/build_work/*" -not -path "*/bootloader/*" \
    -exec cp {} "${ROOTFS_DIR}/opt/mk_os/knowledge/" \; 2>/dev/null || true

# --- Step 10: Finalize image ---
log "=== Step 10: Finalizing disk image ==="

# Unmount if mounted
if [ "${FALLBACK_MODE:-0}" != "1" ]; then
    sync
    if [ "$(uname)" = "Linux" ]; then
        umount "${ROOTFS_DIR}/proc" 2>/dev/null || true
        umount "${ROOTFS_DIR}/sys" 2>/dev/null || true
        umount "${ROOTFS_DIR}/dev" 2>/dev/null || true
        umount "${ROOTFS_DIR}" 2>/dev/null || true
        if [ -n "${LOOP_DEV:-}" ]; then
            losetup -d "${LOOP_DEV}" 2>/dev/null || true
            LOOP_DEV=""
        fi
    elif [ "$(uname)" = "Darwin" ]; then
        umount "${ROOTFS_DIR}" 2>/dev/null || true
    fi
else
    # Fallback: repack the directory into the ext4 image
    if command -v genext2fs &>/dev/null; then
        genext2fs -b $((IMAGE_SIZE_MB * 1024)) -d "${ROOTFS_DIR}" "${IMAGE_FILE}"
        log "Image repacked with genext2fs."
    elif command -v mke2fs &>/dev/null; then
        # Use mke2fs with directory (-d) option
        rm -f "${IMAGE_FILE}"
        mke2fs -t ext4 -d "${ROOTFS_DIR}" -L "MK_OS" "${IMAGE_FILE}" "${IMAGE_SIZE_MB}M" 2>/dev/null || \
            log "WARNING: mke2fs -d not supported on this version. Image may be incomplete."
    fi
fi

log "Raw image ready: ${IMAGE_FILE}"
log "Image size: $(du -h "${IMAGE_FILE}" | cut -f1)"

# --- Step 11: Create QCOW2 variant ---
log "=== Step 11: Creating QCOW2 image (UTM SE preferred) ==="

QCOW2_FILE="${SCRIPT_DIR}/${OUTPUT_BASE}.qcow2"

if [ "${HAVE_QEMU_IMG}" = "1" ]; then
    qemu-img convert -f raw -O qcow2 "${IMAGE_FILE}" "${QCOW2_FILE}"
    log "QCOW2 image created: ${QCOW2_FILE}"
    log "QCOW2 size: $(du -h "${QCOW2_FILE}" | cut -f1)"
else
    log "WARNING: qemu-img not found. Skipping QCOW2 conversion."
    log "Install with: brew install qemu (macOS) or apt install qemu-utils (Linux)"
    log "Then convert manually: qemu-img convert -f raw -O qcow2 ${IMAGE_FILE} ${QCOW2_FILE}"
fi

# --- Done ---
echo ""
echo "============================================================"
echo "  MK OS Boot Image Build Complete"
echo "============================================================"
echo ""
echo "  Architecture:  ${ARCH}"
echo "  Raw image:     ${IMAGE_FILE}"
if [ "${HAVE_QEMU_IMG}" = "1" ]; then
echo "  QCOW2 image:   ${QCOW2_FILE}"
fi
echo ""
echo "  Next steps:"
echo "    1. Transfer ${OUTPUT_BASE}.qcow2 to your iPhone"
echo "    2. Open UTM SE and create a new VM"
echo "    3. Select the QCOW2 file as the drive"
echo "    4. Set RAM to 512MB-1GB, CPU to 2 cores"
echo "    5. Boot and enjoy MK OS!"
echo ""
echo "  For detailed instructions, see: bootloader/utm_guide.md"
echo "============================================================"
