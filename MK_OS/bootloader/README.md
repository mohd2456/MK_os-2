# Bootloader

Boot infrastructure for deploying MK OS on real hardware or VMs (ARM64/x86).

## Key Files

- **mk_boot.cfg** - Boot configuration parameters
- **mk_init.sh** - Init script executed at boot
- **init.mk** - MK-specific initialization data
- **build_image.sh** - Script to build bootable disk image
- **Makefile.arm64** - Cross-compilation for ARM64 targets
- **alpine_packages.txt** - Package list for Alpine Linux base
- **network_setup.sh** - Network interface configuration
- **splash.jpeg** - Boot splash screen
- **utm_guide.md** - Guide for running MK OS in UTM virtual machines
- **mk_service/** - Systemd service definitions
- **build_work/** - Build workspace and artifacts
