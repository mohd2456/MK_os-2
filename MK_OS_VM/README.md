# MK OS - UTM Virtual Machine (ARM64)

## MK OS 2.0 - Custom Desktop Environment

This is a complete MK OS experience packaged as a UTM virtual machine for macOS (Apple Silicon).

### What's Inside
- **MK OS Splash Screen** - Animated ASCII art logo on boot
- **MK OS Loading Animation** - Progress bar with loading messages
- **MK OS Desktop** - Full terminal-based desktop environment with:
  - Start menu (press M or ESC)
  - Terminal app (press T)
  - File manager
  - System info
  - Settings
  - Lock screen (press L)
  - Power menu (press Q)
  - Taskbar with clock

### How to Reassemble & Use

1. **Reassemble the zip file:**
```bash
cat MK_OS_ARM64.utm.zip.part_* > MK_OS_ARM64.utm.zip
```

2. **Extract:**
```bash
unzip MK_OS_ARM64.utm.zip
```

3. **Open in UTM:**
   - Double-click `MK_OS.utm` or import it into UTM app

4. **Boot:**
   - The VM will auto-boot into MK OS
   - You'll see: Splash → Loading → Desktop

### Keyboard Shortcuts (in MK Desktop)
| Key | Action |
|-----|--------|
| M / ESC | Open start menu |
| T | Open terminal |
| L | Lock screen |
| Q | Power menu |
| Arrow keys | Navigate menu |
| Enter | Select |

### System Info
- **Base:** Arch Linux ARM (aarch64)
- **Architecture:** ARM64
- **RAM:** 2GB
- **Disk:** 10GB
- **Boot:** UEFI

### Credits
Created by Mohammed | MK OS Project
