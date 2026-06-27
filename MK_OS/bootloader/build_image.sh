#!/bin/bash
# =============================================================================
# MK OS - Bootable Disk Image Builder for UTM SE (iPhone) / QEMU
# =============================================================================
#
# FIXED VERSION: Creates a PROPER bootable disk image with:
#   - MBR partition table (required by QEMU/UTM SE)
#   - Syslinux/extlinux bootloader
#   - Alpine Linux kernel + initramfs (so the VM can actually boot)
#   - MK OS binary installed as a service
#
# USAGE:
#   ./build_image.sh [OPTIONS]
#
# OPTIONS:
#   --arch aarch64    Target architecture (default: aarch64 for iPhone)
#   --arch x86_64     Build x86_64 image for testing in QEMU on Mac
#   --size 512        Image size in MB (default: 512)
#   --output NAME     Output filename base (default: mk_os)
#   --skip-compile    Skip compilation, use pre-built binary from staging/
#   --help            Show this help message
#
# PREREQUISITES (Linux host or Docker required for loop mount):
#   - Must run as root (or with sudo) for loop device + chroot
#   - qemu-img: apt install qemu-utils / brew install qemu
#   - syslinux: apt install syslinux syslinux-common
#   - For cross-compile: aarch64-linux-gnu-g++
#
# OUTPUT:
#   - build_work/mk_os.qcow2   QCOW2 image (use this in UTM SE)
#   - build_work/mk_os.img     Raw disk image (alternative)
#
# =============================================================================

set -euo pipefail

# --- Configuration ---
ARCH="aarch64"
IMAGE_SIZE_MB=512
OUTPUT_BASE="mk_os"
SKIP_COMPILE=0
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
WORK_DIR="${SCRIPT_DIR}/build_work"
STAGING_DIR="${SCRIPT_DIR}/staging"
ROOTFS_DIR="${WORK_DIR}/rootfs"
MOUNT_DIR="${WORK_DIR}/mnt"

# Alpine Linux version (use virtual ISO which is small and includes kernel)
ALPINE_VERSION="3.20"
ALPINE_BRANCH="v3.20"
ALPINE_MIRROR="https://dl-cdn.alpinelinux.org/alpine"

# --- Parse Arguments ---
while [[ $# -gt 0 ]]; do
    case "$1" in
        --arch)    ARCH="$2"; shift 2 ;;
        --size)    IMAGE_SIZE_MB="$2"; shift 2 ;;
        --output)  OUTPUT_BASE="$2"; shift 2 ;;
        --skip-compile) SKIP_COMPILE=1; shift ;;
        --help)
            head -30 "$0" | tail -25
            exit 0
            ;;
        *) echo "ERROR: Unknown option: $1"; exit 1 ;;
    esac
done

# --- Helper Functions ---
log() { echo "[MK BUILD] $(date '+%H:%M:%S') $*"; }
error() { echo "[MK ERROR] $*" >&2; exit 1; }

cleanup() {
    log "Cleaning up..."
    sync 2>/dev/null || true
    umount "${MOUNT_DIR}/proc" 2>/dev/null || true
    umount "${MOUNT_DIR}/sys" 2>/dev/null || true
    umount "${MOUNT_DIR}/dev" 2>/dev/null || true
    umount "${MOUNT_DIR}" 2>/dev/null || true
    [ -n "${LOOP_DEV:-}" ] && losetup -d "${LOOP_DEV}" 2>/dev/null || true
}
trap cleanup EXIT

# --- Validate ---
if [ "$(id -u)" -ne 0 ]; then
    error "This script must be run as root (needed for loop mount + chroot).
       Run: sudo ./build_image.sh $*"
fi

if [ "$(uname)" != "Linux" ]; then
    error "This script requires Linux (for loop devices and chroot).
       Use a Linux VM, Docker, or GitHub Actions to build the image.
       On Mac: docker run --privileged -v \$(pwd):/work alpine sh /work/build_image.sh"
fi

log "Architecture: ${ARCH}"
log "Image size: ${IMAGE_SIZE_MB}MB"
log "Output: ${WORK_DIR}/${OUTPUT_BASE}.qcow2"

# --- Step 1: Download Alpine mini rootfs ---
log "=== Step 1: Downloading Alpine Linux rootfs ==="
mkdir -p "${WORK_DIR}"

ROOTFS_URL="${ALPINE_MIRROR}/${ALPINE_BRANCH}/releases/${ARCH}/alpine-minirootfs-${ALPINE_VERSION}.0-${ARCH}.tar.gz"
ROOTFS_TARBALL="${WORK_DIR}/alpine-minirootfs-${ARCH}.tar.gz"

if [ ! -f "${ROOTFS_TARBALL}" ]; then
    log "Downloading Alpine ${ALPINE_VERSION} rootfs for ${ARCH}..."
    wget -q --show-progress -O "${ROOTFS_TARBALL}" "${ROOTFS_URL}" || \
        error "Failed to download Alpine rootfs from ${ROOTFS_URL}"
else
    log "Using cached rootfs tarball."
fi

# --- Step 2: Create disk image with MBR partition table ---
log "=== Step 2: Creating ${IMAGE_SIZE_MB}MB disk image with MBR partition ==="

IMAGE_FILE="${WORK_DIR}/${OUTPUT_BASE}.img"
rm -f "${IMAGE_FILE}"

# Create the raw image
dd if=/dev/zero of="${IMAGE_FILE}" bs=1M count="${IMAGE_SIZE_MB}" status=progress

# Create MBR partition table with one bootable Linux partition
# Partition starts at sector 2048 (1MB offset for alignment)
parted -s "${IMAGE_FILE}" -- \
    mklabel msdos \
    mkpart primary ext4 1MiB 100% \
    set 1 boot on

log "Partition table created (MBR, 1 bootable ext4 partition)."

# --- Step 3: Set up loop device and format partition ---
log "=== Step 3: Formatting partition ==="

LOOP_DEV=$(losetup --find --show --partscan "${IMAGE_FILE}")
log "Loop device: ${LOOP_DEV}"

# Wait for partition device to appear
sleep 1
PART_DEV="${LOOP_DEV}p1"
if [ ! -b "${PART_DEV}" ]; then
    # Try alternative naming
    PART_DEV="${LOOP_DEV}p1"
    partprobe "${LOOP_DEV}" 2>/dev/null || true
    sleep 1
fi

if [ ! -b "${PART_DEV}" ]; then
    # Fallback: use offset-based access
    log "Partition device not found, using offset mount..."
    PART_OFFSET=$(parted -s "${IMAGE_FILE}" unit B print | grep "^ 1" | awk '{print $2}' | tr -d 'B')
    losetup -d "${LOOP_DEV}"
    LOOP_DEV=$(losetup --find --show --offset "${PART_OFFSET}" "${IMAGE_FILE}")
    PART_DEV="${LOOP_DEV}"
fi

mkfs.ext4 -F -L "MK_OS" "${PART_DEV}"
log "Formatted partition as ext4."

# --- Step 4: Mount and extract rootfs ---
log "=== Step 4: Mounting and extracting Alpine rootfs ==="

mkdir -p "${MOUNT_DIR}"
mount "${PART_DEV}" "${MOUNT_DIR}"

tar -xzf "${ROOTFS_TARBALL}" -C "${MOUNT_DIR}"
log "Alpine rootfs extracted."

# --- Step 5: Chroot and install kernel + bootloader + packages ---
log "=== Step 5: Installing kernel, bootloader, and packages via chroot ==="

# Set up for chroot
cp /etc/resolv.conf "${MOUNT_DIR}/etc/resolv.conf"
mount -t proc proc "${MOUNT_DIR}/proc"
mount -t sysfs sysfs "${MOUNT_DIR}/sys"
mount --bind /dev "${MOUNT_DIR}/dev"

# Configure APK repositories
cat > "${MOUNT_DIR}/etc/apk/repositories" << EOF
${ALPINE_MIRROR}/${ALPINE_BRANCH}/main
${ALPINE_MIRROR}/${ALPINE_BRANCH}/community
EOF

# Install kernel, bootloader, and required packages inside chroot
# THIS IS THE KEY FIX: We install linux-virt (kernel) + syslinux (bootloader)
chroot "${MOUNT_DIR}" /bin/sh -e << 'CHROOTEOF'
# Update package index
apk update

# Install the Linux kernel (linux-virt is optimized for VMs, much smaller)
apk add linux-virt

# Install syslinux/extlinux bootloader (works with MBR/BIOS boot in QEMU)
apk add syslinux

# Install essential system packages
apk add openrc busybox-initscripts alpine-base

# Install MK OS dependencies
apk add curl libcurl sqlite-libs zlib openssl openssh git busybox-extras \
    dhcpcd iptables procps util-linux grep sed tzdata

# Install extlinux bootloader to /boot
extlinux --install /boot

# Enable essential OpenRC services
rc-update add devfs sysinit
rc-update add dmesg sysinit
rc-update add mdev sysinit
rc-update add hwdrivers sysinit

rc-update add hwclock boot
rc-update add modules boot
rc-update add sysctl boot
rc-update add hostname boot
rc-update add bootmisc boot
rc-update add syslog boot
rc-update add networking boot

rc-update add mount-ro shutdown
rc-update add killprocs shutdown
rc-update add savecache shutdown

# Enable SSH for remote access
rc-update add sshd default
echo "PermitRootLogin yes" >> /etc/ssh/sshd_config

# Set root password to "mk" (user can change later)
echo "root:mk" | chpasswd

# Enable serial console for UTM SE (CRITICAL for headless VMs)
sed -i 's/^#ttyS0/ttyS0/' /etc/inittab 2>/dev/null || true
# Add serial console if not present
grep -q "ttyS0" /etc/inittab || \
    echo "ttyS0::respawn:/sbin/getty -L ttyS0 115200 vt100" >> /etc/inittab

# Enable login on the main console
grep -q "tty1" /etc/inittab || \
    echo "tty1::respawn:/sbin/getty 38400 tty1" >> /etc/inittab

# Configure fstab
cat > /etc/fstab << 'FSTAB'
/dev/vda1   /       ext4    defaults,noatime    0 1
proc        /proc   proc    defaults            0 0
sysfs       /sys    sysfs   defaults            0 0
devpts      /dev/pts devpts defaults            0 0
tmpfs       /tmp    tmpfs   defaults,nosuid     0 0
FSTAB

# Set hostname
echo "mk-os" > /etc/hostname
echo "127.0.0.1 mk-os localhost" > /etc/hosts

# Configure networking (DHCP on eth0)
cat > /etc/network/interfaces << 'NETCFG'
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp
NETCFG

# DNS
cat > /etc/resolv.conf << 'DNS'
nameserver 8.8.8.8
nameserver 1.1.1.1
DNS

CHROOTEOF

log "Packages and kernel installed successfully."

# --- Step 6: Configure extlinux/syslinux bootloader ---
log "=== Step 6: Configuring bootloader (extlinux) ==="

# Find the installed kernel and initramfs
KERNEL_FILE=$(ls "${MOUNT_DIR}/boot/vmlinuz-"* 2>/dev/null | head -1)
INITRD_FILE=$(ls "${MOUNT_DIR}/boot/initramfs-"* 2>/dev/null | head -1)

if [ -z "${KERNEL_FILE}" ] || [ -z "${INITRD_FILE}" ]; then
    error "Kernel or initramfs not found in /boot after package install!"
fi

KERNEL_BASENAME=$(basename "${KERNEL_FILE}")
INITRD_BASENAME=$(basename "${INITRD_FILE}")

log "Kernel: ${KERNEL_BASENAME}"
log "Initrd: ${INITRD_BASENAME}"

# Create extlinux configuration
# The serial console line is CRITICAL for UTM SE (Console Only mode)
mkdir -p "${MOUNT_DIR}/boot"
cat > "${MOUNT_DIR}/boot/extlinux.conf" << EOF
DEFAULT mk_os
PROMPT 0
TIMEOUT 30

LABEL mk_os
    LINUX /boot/${KERNEL_BASENAME}
    INITRD /boot/${INITRD_BASENAME}
    APPEND root=/dev/vda1 rootfstype=ext4 console=ttyS0,115200 console=tty0 modules=virtio_blk,virtio_net,ext4 quiet
EOF

log "Bootloader configuration written."

# Install MBR boot code (syslinux MBR -> extlinux on partition)
if [ -f /usr/lib/syslinux/mbr/mbr.bin ]; then
    dd if=/usr/lib/syslinux/mbr/mbr.bin of="${IMAGE_FILE}" bs=440 count=1 conv=notrunc
elif [ -f /usr/share/syslinux/mbr.bin ]; then
    dd if=/usr/share/syslinux/mbr.bin of="${IMAGE_FILE}" bs=440 count=1 conv=notrunc
elif [ -f "${MOUNT_DIR}/usr/share/syslinux/mbr.bin" ]; then
    dd if="${MOUNT_DIR}/usr/share/syslinux/mbr.bin" of="${IMAGE_FILE}" bs=440 count=1 conv=notrunc
else
    log "WARNING: mbr.bin not found on host. Copying from chroot..."
    chroot "${MOUNT_DIR}" dd if=/usr/share/syslinux/mbr.bin of=/dev/null bs=440 count=1 2>/dev/null
    # Copy mbr.bin out of chroot
    cp "${MOUNT_DIR}/usr/share/syslinux/mbr.bin" "${WORK_DIR}/mbr.bin" 2>/dev/null || true
    if [ -f "${WORK_DIR}/mbr.bin" ]; then
        dd if="${WORK_DIR}/mbr.bin" of="${IMAGE_FILE}" bs=440 count=1 conv=notrunc
    else
        log "WARNING: Could not install MBR boot code. Image may not boot with BIOS."
        log "For UTM SE aarch64, this may still work with direct kernel boot."
    fi
fi

log "MBR boot code installed."

# --- Step 7: Install MK OS binary and service ---
log "=== Step 7: Installing MK OS binary and service ==="

mkdir -p "${MOUNT_DIR}/opt/mk_os"
mkdir -p "${MOUNT_DIR}/opt/mk_os/knowledge"
mkdir -p "${MOUNT_DIR}/var/log"

# Copy init and service scripts
cp "${SCRIPT_DIR}/mk_init.sh" "${MOUNT_DIR}/opt/mk_os/mk_init.sh"
chmod +x "${MOUNT_DIR}/opt/mk_os/mk_init.sh"

cp "${SCRIPT_DIR}/network_setup.sh" "${MOUNT_DIR}/opt/mk_os/network_setup.sh"
chmod +x "${MOUNT_DIR}/opt/mk_os/network_setup.sh"

# Install OpenRC service
cp "${SCRIPT_DIR}/mk_service" "${MOUNT_DIR}/etc/init.d/mk_os"
chmod +x "${MOUNT_DIR}/etc/init.d/mk_os"

# Enable MK OS service on boot
chroot "${MOUNT_DIR}" rc-update add mk_os default 2>/dev/null || \
    ln -sf /etc/init.d/mk_os "${MOUNT_DIR}/etc/runlevels/default/mk_os"

# Install MK OS binary (if available)
if [ "${SKIP_COMPILE}" = "1" ] && [ -f "${STAGING_DIR}/mk_os" ]; then
    cp "${STAGING_DIR}/mk_os" "${MOUNT_DIR}/opt/mk_os/mk_os"
    chmod +x "${MOUNT_DIR}/opt/mk_os/mk_os"
    log "Installed pre-built MK OS binary."
elif [ -f "${STAGING_DIR}/mk_os" ]; then
    cp "${STAGING_DIR}/mk_os" "${MOUNT_DIR}/opt/mk_os/mk_os"
    chmod +x "${MOUNT_DIR}/opt/mk_os/mk_os"
    log "Installed MK OS binary from staging."
else
    log "WARNING: No MK OS binary found in ${STAGING_DIR}/"
    log "Build it first: make -f bootloader/Makefile.arm64 static && make -f bootloader/Makefile.arm64 install"
    log "The VM will boot Alpine Linux but MK OS service won't start until binary is installed."
    # Create placeholder script so the service doesn't crash
    cat > "${MOUNT_DIR}/opt/mk_os/mk_os" << 'PLACEHOLDER'
#!/bin/sh
echo "MK OS binary not yet installed. Build and copy it to /opt/mk_os/mk_os"
echo "Sleeping to prevent service restart loop..."
sleep 86400
PLACEHOLDER
    chmod +x "${MOUNT_DIR}/opt/mk_os/mk_os"
fi

# Copy knowledge files if available
KNOWLEDGE_SRC="${PROJECT_ROOT}/ai_core/hre/knowledge_files"
if [ -d "${KNOWLEDGE_SRC}" ]; then
    cp "${KNOWLEDGE_SRC}"/*.mk "${MOUNT_DIR}/opt/mk_os/knowledge/" 2>/dev/null || true
    log "Knowledge files copied."
fi

# --- Step 8: Create MOTD (login banner) ---
cat > "${MOUNT_DIR}/etc/motd" << 'MOTD'

  ==================================
     MK OS - Hybrid AI System
     Running on Alpine Linux
  ==================================
  
  MK OS Status: rc-service mk_os status
  MK OS Logs:   cat /var/log/mk_os.log
  
MOTD

# --- Step 9: Unmount and create QCOW2 ---
log "=== Step 9: Finalizing image ==="

sync
umount "${MOUNT_DIR}/proc" 2>/dev/null || true
umount "${MOUNT_DIR}/sys" 2>/dev/null || true
umount "${MOUNT_DIR}/dev" 2>/dev/null || true
umount "${MOUNT_DIR}"
losetup -d "${LOOP_DEV}" 2>/dev/null || true
LOOP_DEV=""

log "Filesystem unmounted."

# Create QCOW2 (compressed, preferred by UTM SE)
QCOW2_FILE="${WORK_DIR}/${OUTPUT_BASE}.qcow2"
if command -v qemu-img &>/dev/null; then
    qemu-img convert -f raw -O qcow2 -c "${IMAGE_FILE}" "${QCOW2_FILE}"
    log "QCOW2 image created: ${QCOW2_FILE}"
    log "QCOW2 size: $(du -h "${QCOW2_FILE}" | cut -f1)"
else
    log "WARNING: qemu-img not found. Only raw image available."
    log "Install: apt install qemu-utils"
fi

# --- Done ---
echo ""
echo "============================================================"
echo "  MK OS Bootable Image - Build Complete"
echo "============================================================"
echo ""
echo "  Architecture:   ${ARCH}"
echo "  Raw image:      ${IMAGE_FILE} ($(du -h "${IMAGE_FILE}" | cut -f1))"
[ -f "${QCOW2_FILE}" ] && \
echo "  QCOW2 image:    ${QCOW2_FILE} ($(du -h "${QCOW2_FILE}" | cut -f1))"
echo ""
echo "  Root password:  mk"
echo ""
echo "  UTM SE Setup:"
echo "    1. Transfer ${OUTPUT_BASE}.qcow2 to iPhone"
echo "    2. Create new VM: Emulate > Linux > ARM64 (aarch64)"
echo "    3. RAM: 512MB, CPU: 2 cores"
echo "    4. Import the QCOW2 as VirtIO drive"
echo "    5. Display: Console Only (serial)"
echo "    6. Network: Shared Network (NAT)"
echo "    7. Boot!"
echo ""
echo "  Test locally with QEMU:"
echo "    qemu-system-${ARCH} -M virt -cpu cortex-a72 -m 512 \\"
echo "      -drive file=${QCOW2_FILE},format=qcow2,if=virtio \\"
echo "      -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \\"
echo "      -nographic"
echo ""
echo "============================================================"
