# MK OS - UTM SE Deployment Guide

## Complete guide to running MK OS on iPhone via UTM SE

---

## Table of Contents

1. [What is UTM SE?](#what-is-utm-se)
2. [Building the MK OS Image](#building-the-mk-os-image)
3. [Transferring to iPhone](#transferring-to-iphone)
4. [Configuring the VM](#configuring-the-vm)
5. [First Boot](#first-boot)
6. [SSH Remote Access](#ssh-remote-access)
7. [Knowledge Sync with GitHub](#knowledge-sync-with-github)
8. [Troubleshooting](#troubleshooting)
9. [Recommended VM Settings](#recommended-vm-settings)

---

## What is UTM SE?

UTM SE (Slow Edition) is a free, open-source virtual machine app for iOS/iPadOS. It allows you to run full Linux distributions on your iPhone or iPad without jailbreaking.

**Key features:**
- Runs ARM64 and x86_64 Linux VMs
- No jailbreak required (available on App Store)
- Supports QCOW2 and raw disk images
- Emulates a full system with networking
- JIT-free (uses threaded interpreter for App Store compliance)

**How to get UTM SE:**
- App Store: Search for "UTM SE" (free)
- AltStore: [https://getutm.app](https://getutm.app)
- GitHub: [https://github.com/utmapp/UTM](https://github.com/utmapp/UTM)

> Note: UTM SE is the App Store version. The full "UTM" app requires sideloading
> but offers better performance via JIT compilation.

---

## Building the MK OS Image

### Prerequisites (on macOS)

```bash
# Install required tools
brew install qemu         # For qemu-img (QCOW2 conversion)
brew install fuse-ext2    # For ext4 filesystem support on macOS (optional)

# For cross-compilation to ARM64
brew install aarch64-elf-gcc
# OR use Xcode's clang with cross-target:
# clang++ --target=aarch64-linux-gnu
```

### Option A: Full Build (compiles MK OS for ARM64)

```bash
cd MK_OS/

# Step 1: Cross-compile MK OS for ARM64
make -f bootloader/Makefile.arm64 static
make -f bootloader/Makefile.arm64 install

# Step 2: Build the bootable disk image
cd bootloader/
./build_image.sh --arch aarch64 --skip-compile
```

### Option B: Build x86_64 Image (for testing)

```bash
cd MK_OS/bootloader/
./build_image.sh --arch x86_64
```

### Build Output

After running `build_image.sh`, you will have:
- `bootloader/mk_os_boot.img` - Raw disk image (256MB)
- `bootloader/mk_os_boot.qcow2` - QCOW2 image (compressed, preferred by UTM SE)

---

## Transferring to iPhone

### Method 1: AirDrop (Easiest)
1. AirDrop the `.qcow2` file from Mac to iPhone
2. Save it to the Files app
3. Move it to the UTM SE documents folder

### Method 2: iCloud Drive
1. Copy `mk_os_boot.qcow2` to iCloud Drive
2. On iPhone, open Files app
3. Navigate to iCloud Drive and find the file
4. Move to UTM SE's folder: `On My iPhone > UTM SE`

### Method 3: USB + Files App
1. Connect iPhone to Mac via USB/Lightning/USB-C
2. Open Finder (macOS Ventura+) or iTunes
3. Select your iPhone > Files > UTM SE
4. Drag the `.qcow2` file into UTM SE's documents

### Method 4: Direct Download (if hosted)
1. Host the `.qcow2` file on a web server or GitHub Release
2. Download directly on iPhone via Safari
3. Open with UTM SE

---

## Configuring the VM

### Creating a New VM in UTM SE

1. Open UTM SE on your iPhone
2. Tap the **+** button to create a new VM
3. Select **Emulate** (not Virtualize)
4. Choose **Linux** as the operating system

### VM Configuration Settings

| Setting | Recommended Value | Notes |
|---------|------------------|-------|
| Architecture | ARM64 (aarch64) | Matches iPhone CPU |
| System | QEMU ARM Virtual Machine | Default is fine |
| RAM | 512MB - 1024MB | 512MB minimum for MK OS |
| CPU Cores | 2 | UTM SE limits this |
| Storage | Import QCOW2 | Select mk_os_boot.qcow2 |
| Network | Emulated VLAN | Shared networking (NAT) |
| Display | Console Only | MK OS is terminal-based |

### Detailed Setup Steps

1. **System tab:**
   - Architecture: ARM64 (aarch64)
   - Machine: virt
   - CPU: Default (or cortex-a72 if available)

2. **Memory tab:**
   - RAM: 512MB (or 1024MB if your iPhone has 4GB+ RAM)

3. **Drives tab:**
   - Tap "Import Drive"
   - Navigate to your `mk_os_boot.qcow2`
   - Interface: VirtIO (fastest) or IDE (most compatible)

4. **Network tab:**
   - Mode: Emulated VLAN (shared networking)
   - Emulated NIC: virtio-net-pci

5. **Display tab:**
   - Type: Console Only (serial)
   - This gives you a terminal interface to MK OS

6. Save the VM configuration

---

## First Boot

1. Tap your MK OS VM in UTM SE
2. Tap the **Play** button to start
3. You should see the Alpine Linux boot sequence
4. MK OS starts automatically via the OpenRC service
5. You will see the MK OS prompt:

```
==========================================
  MK OS Boot Sequence Starting
  Date: 2025-01-15 10:30:00
  Kernel: 6.1.0-alpine
  Arch: aarch64
==========================================
[MK INIT] Mounting required filesystems...
[MK INIT] Setting up networking...
[MK INIT] Starting MK OS binary...

Welcome to MK OS v2.0
Type /help for available commands.
>
```

### First Boot Checklist

- [ ] Alpine boots without kernel panic
- [ ] Network is configured (check with `/search test`)
- [ ] MK OS prompt appears
- [ ] Knowledge files are loaded (check with `/status`)

---

## SSH Remote Access

SSH lets you access MK OS from your Mac or another device on the same network.

### Enable SSH (one-time setup)

Connect to the VM console and run:

```bash
# Set root password
passwd

# Start SSH daemon
rc-service sshd start
rc-update add sshd default
```

### Find VM IP Address

In the VM console:
```bash
ip addr show eth0
```

Or from UTM SE, check the VM's network settings for the port forwarding IP.

### Port Forwarding (UTM SE NAT mode)

Since UTM SE uses NAT, you need port forwarding:

1. In UTM SE VM settings > Network
2. Add port forward: Host 2222 -> Guest 22
3. Connect from Mac: `ssh -p 2222 root@localhost`

### Connect via SSH

```bash
# From your Mac (with port forwarding)
ssh -p 2222 root@localhost

# Or if using bridged networking with known IP
ssh root@<vm-ip-address>
```

---

## Knowledge Sync with GitHub

MK OS can sync its knowledge base with a GitHub repository for updates.

### Setup Inside the VM

```bash
# Configure git
git config --global user.name "MK OS"
git config --global user.email "mk@local"

# Clone the knowledge repository
cd /opt/mk_os/knowledge
git init
git remote add origin https://github.com/mohd2456/MK_os-2.git

# Pull latest knowledge files
git fetch origin
git checkout origin/mk-accuracy-upgrade-2026 -- MK_OS/ai_core/hre/knowledge_files/

# Copy to knowledge directory
cp MK_OS/ai_core/hre/knowledge_files/*.mk /opt/mk_os/knowledge/
rm -rf MK_OS/
```

### Automated Sync (cron job)

```bash
# Add to crontab (sync every 6 hours)
echo "0 */6 * * * cd /opt/mk_os/knowledge && git pull origin mk-accuracy-upgrade-2026 2>/dev/null" \
    | crontab -
```

### Using the Built-in Sync Tool

If `mk_sync` is compiled and available:
```bash
/opt/mk_os/mk_sync --repo https://github.com/mohd2456/MK_os-2.git --branch mk-accuracy-upgrade-2026
```

---

## Troubleshooting

### VM won't boot / Kernel panic

- **Cause:** Wrong architecture selected in UTM SE
- **Fix:** Ensure architecture matches your image (aarch64 for ARM build)
- **Try:** Recreate VM with "virt" machine type

### No network connectivity

- **Cause:** Network interface not detected
- **Fix:** In UTM SE settings, try changing NIC to e1000 or rtl8139
- **Try:** Inside VM, run: `ip link` to see available interfaces
- If eth0 is not listed, try: `ip link set enp0s1 up && udhcpc -i enp0s1`

### MK OS binary crashes on start

- **Cause:** Binary compiled for wrong architecture
- **Fix:** Rebuild with correct target:
  ```bash
  make -f bootloader/Makefile.arm64 clean
  make -f bootloader/Makefile.arm64 static
  ```
- **Check:** Run `file staging/mk_os` to verify it says "ARM aarch64"

### "Permission denied" errors

- **Fix:** `chmod +x /opt/mk_os/mk_os`
- **Fix:** Ensure running as root or the binary has correct permissions

### Very slow performance

- UTM SE uses a threaded interpreter (no JIT), so expect 5-10x slower than native
- **Tips:**
  - Close other apps on iPhone to free RAM
  - Reduce RAM allocation if iPhone is low on memory
  - Use Console display mode (no GPU emulation)
  - Consider the full UTM app (sideloaded) for JIT support

### /search and /weather commands fail

- **Cause:** No internet connectivity in VM
- **Fix:** Check UTM SE network settings are "Shared Network" (NAT)
- **Test:** `ping 8.8.8.8` inside the VM
- **DNS issue:** Check `/etc/resolv.conf` has nameserver entries

### Image too large for iPhone storage

- Default image is 256MB (compressed QCOW2 is much smaller)
- Reduce with: `./build_image.sh --size 128`
- Remove unnecessary packages from `alpine_packages.txt`

### SSH connection refused

- Ensure sshd is running: `rc-service sshd status`
- Check port forwarding is configured in UTM SE
- Verify firewall allows port 22: `iptables -L INPUT`

---

## Recommended VM Settings

### iPhone SE (2016) / iPhone 6s / iPhone 7

These older devices have 2GB RAM. Use conservative settings:

| Setting | Value |
|---------|-------|
| RAM | 256MB - 384MB |
| CPU Cores | 1 |
| Display | Console Only |
| Storage | 128MB image |

### iPhone SE (2020/2022) / iPhone 8-11

Devices with 3-4GB RAM:

| Setting | Value |
|---------|-------|
| RAM | 512MB |
| CPU Cores | 2 |
| Display | Console Only |
| Storage | 256MB image |

### iPhone 12-16 / iPad Pro

Modern devices with 4-8GB RAM:

| Setting | Value |
|---------|-------|
| RAM | 1024MB |
| CPU Cores | 2-4 |
| Display | Console Only |
| Storage | 256MB-512MB image |

### Performance Expectations

- **Boot time:** 15-45 seconds (UTM SE interpreter mode)
- **Command response:** 0.5-2 seconds for local commands
- **Search/Weather:** 3-10 seconds (network dependent)
- **Knowledge loading:** 5-15 seconds on first start

> Tip: UTM (full version, sideloaded via AltStore) with JIT enabled is
> approximately 5-10x faster than UTM SE. If performance is critical,
> consider sideloading the full UTM app.

---

## Quick Reference

```bash
# Build image for iPhone
make -f bootloader/Makefile.arm64 static && make -f bootloader/Makefile.arm64 install
cd bootloader && ./build_image.sh --skip-compile

# Test locally with QEMU (x86_64)
qemu-system-x86_64 -m 512 -drive file=mk_os_boot.qcow2,format=qcow2 -nographic

# Test locally with QEMU (aarch64)
qemu-system-aarch64 -M virt -cpu cortex-a72 -m 512 \
    -drive file=mk_os_boot.qcow2,format=qcow2 \
    -nographic -kernel /path/to/alpine-kernel

# Inside VM: check MK OS status
rc-service mk_os status
cat /var/log/mk_os.log
cat /var/log/mk_boot.log
```
