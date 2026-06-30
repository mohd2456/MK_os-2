# How to Create the GitHub Release

Since the file is already uploaded to gofile.io, you can download it from there.
But if you want it as a proper GitHub Release on your repo, follow these steps:

## Option 1: Quick Download (Already Done)
**Download:** https://gofile.io/d/m7zMYF

## Option 2: Create GitHub Release (Manual - 1 minute)

1. Go to: https://github.com/mohd2456/MK_os-2/releases/new
2. Tag: `v2.0-arm64` (create new tag)
3. Target: `mk-os-utm-vm` branch
4. Title: `MK OS 2.0 - ARM64 UTM Virtual Machine`
5. Description:
```
## MK OS 2.0 ARM64

Download the zip below, unzip, and open MK_OS.utm in UTM on your Mac.

### Boot Sequence
1. MK OS Splash Screen (animated logo)
2. Loading Animation (progress bar)
3. MK OS Desktop (terminal-based)

### Keyboard Shortcuts
- M/ESC = Start Menu
- T = Terminal
- L = Lock Screen
- Q = Power Menu
```
6. Reassemble the zip from parts:
```bash
cat MK_OS_ARM64.utm.zip.part_* > MK_OS_ARM64.utm.zip
```
7. Drag & drop the reassembled `MK_OS_ARM64.utm.zip` (538MB) into the release assets
8. Click "Publish Release"

Done! You'll get a permanent download link like:
https://github.com/mohd2456/MK_os-2/releases/download/v2.0-arm64/MK_OS_ARM64.utm.zip
