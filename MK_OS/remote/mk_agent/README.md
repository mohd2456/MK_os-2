# MK OS - PC Agent

A lightweight Python agent that runs on your PC, allowing MK (running on Mac) to send commands remotely.

## Quick Start

### 1. Install

Copy the `mk_agent/` folder to your PC, or clone the repo and navigate to it.

```bash
cd MK_OS/remote/mk_agent
chmod +x setup.sh
./setup.sh
```

The setup script installs optional Python packages and generates an auth token.

### 2. Start the Agent

```bash
python3 agent.py --token YOUR_SECRET_TOKEN
```

Or using environment variable:
```bash
export MK_PC_TOKEN=your_secret_token_here
python3 agent.py
```

Options:
- `--token TOKEN` — Auth token (required)
- `--port 9876` — Listen port (default: 9876)
- `--host 0.0.0.0` — Listen address (default: all interfaces)
- `--allow-ips 192.168.1.5 192.168.1.10` — IP whitelist (optional)

### 3. Configure MK to Connect

On the Mac where MK OS runs, set environment variables:

```bash
export MK_PC_IP=192.168.1.100     # Your PC's local IP
export MK_PC_PORT=9876            # Must match agent port
export MK_PC_TOKEN=your_secret    # Must match agent token
```

Then start MK OS. It will auto-connect during boot.

## Supported Commands

| Command | Description |
|---------|-------------|
| `shell` | Execute any shell command |
| `read_file` | Read a file from PC |
| `write_file` | Write a file on PC |
| `open_app` | Launch an application |
| `system_info` | Get CPU, RAM, disk, OS info |
| `list_processes` | Show running processes |
| `kill_process` | Kill a process by name/PID |
| `clipboard_get` | Read clipboard contents |
| `clipboard_set` | Write to clipboard |
| `screenshot` | Take a screenshot |
| `list_files` | List directory contents |

## MK OS Commands (from the REPL)

Once connected, use these in MK:
```
/pc run <command>        Execute shell command
/pc open <app>           Open an application
/pc read <path>          Read a file
/pc write <path> <text>  Write to a file
/pc info                 System info
/pc processes            List processes
/pc kill <name>          Kill a process
/pc files <dir>          List directory
/pc clipboard            Get clipboard
/pc status               Connection status
```

Or use natural language:
```
"open chrome on my pc"
"what's running on my pc"
"read C:\Users\me\notes.txt on my computer"
```

## Dependencies

All dependencies are **optional**. The agent gracefully degrades:

| Package | Used For | Fallback |
|---------|----------|----------|
| `psutil` | Detailed system info | `os.cpu_count()` + limited info |
| `pyperclip` | Cross-platform clipboard | OS commands (pbcopy, xclip, powershell) |
| `Pillow` | Screenshot capture | OS commands (screencapture, scrot) |

Install all:
```bash
pip install psutil pyperclip Pillow
```

## Security

- **Token Authentication**: Every request must include the correct auth token
- **IP Whitelisting**: Optionally restrict which IPs can connect
- **Audit Log**: All commands are logged to stdout with timestamps
- **Local Network Only**: Do NOT expose to the public internet
- **No Persistent State**: Agent stores nothing to disk

### Recommendations

1. Use a strong random token (setup.sh generates one automatically)
2. Run on a trusted LAN behind a firewall
3. Use `--allow-ips` to restrict to your Mac's IP only
4. Monitor the agent's log output for unauthorized attempts

## Platform Support

| Platform | Shell | Clipboard | Screenshot | Apps |
|----------|-------|-----------|------------|------|
| Windows | cmd/powershell | powershell/pyperclip | Pillow/powershell | os.startfile |
| macOS | bash/zsh | pbcopy/pbpaste | screencapture | open -a |
| Linux | bash | xclip/pyperclip | scrot/Pillow | direct exec |

## Troubleshooting

**Agent won't start:**
- Check if port 9876 is already in use: `lsof -i :9876` or `netstat -tlnp | grep 9876`
- Try a different port: `--port 9877`

**MK can't connect:**
- Verify PC's IP: `ipconfig` (Windows) or `ifconfig`/`ip addr` (Unix)
- Check firewall allows port 9876
- Ensure both devices are on the same network
- Test: `telnet <pc-ip> 9876`

**Commands timing out:**
- Default timeout is 30 seconds
- Long-running commands may need to be run with `&` (background)
