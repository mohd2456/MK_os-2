#!/usr/bin/env python3
"""
MK OS - Remote PC Agent
========================
Lightweight TCP agent that runs on the user's PC.
Receives JSON commands from MK (Mac side) and executes them locally.

Usage:
    python agent.py --token YOUR_SECRET_TOKEN
    python agent.py --token YOUR_SECRET_TOKEN --port 9876 --host 0.0.0.0

Environment variables (alternative to CLI args):
    MK_PC_TOKEN  - Auth token
    MK_PC_PORT   - Listen port (default: 9876)
"""

import socket
import json
import subprocess
import os
import sys
import platform
import time
import argparse
import threading
import tempfile
from datetime import datetime

# ─── Configuration ────────────────────────────────────────────────────────────

DEFAULT_PORT = 9876
DEFAULT_HOST = "0.0.0.0"
COMMAND_TIMEOUT = 30
MAX_MESSAGE_SIZE = 1048576  # 1MB
ALLOWED_IPS = []  # Empty = allow all; set to ["192.168.1.x"] to whitelist

# ─── Logging ──────────────────────────────────────────────────────────────────

def log(level, msg):
    ts = datetime.now().strftime("%H:%M:%S")
    prefix = {"INFO": "\033[32m[INFO]\033[0m", "WARN": "\033[33m[WARN]\033[0m",
              "ERR": "\033[31m[ERR]\033[0m", "CMD": "\033[36m[CMD]\033[0m"}
    print(f"  {ts} {prefix.get(level, '[???]')} {msg}")

# ─── Command Handlers ─────────────────────────────────────────────────────────

def handle_ping(payload, fields):
    return {"success": True, "message": "pong"}


def handle_shell(payload, fields):
    if not payload:
        return {"success": False, "error": "No command provided"}
    try:
        result = subprocess.run(
            payload, shell=True, capture_output=True, text=True,
            timeout=COMMAND_TIMEOUT
        )
        return {
            "success": True,
            "stdout": result.stdout,
            "stderr": result.stderr,
            "code": result.returncode
        }
    except subprocess.TimeoutExpired:
        return {"success": False, "error": "Command timed out (30s)"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_read_file(payload, fields):
    path = payload or fields.get("path", "")
    if not path:
        return {"success": False, "error": "No path provided"}
    try:
        path = os.path.expanduser(path)
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            content = f.read(MAX_MESSAGE_SIZE)
        return {"success": True, "content": content}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_write_file(payload, fields):
    path = fields.get("path", payload)
    content = fields.get("content", "")
    if not path:
        return {"success": False, "error": "No path provided"}
    try:
        path = os.path.expanduser(path)
        os.makedirs(os.path.dirname(path), exist_ok=True) if os.path.dirname(path) else None
        with open(path, "w", encoding="utf-8") as f:
            f.write(content)
        return {"success": True, "message": f"Written {len(content)} bytes to {path}"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_open_app(payload, fields):
    app = payload or fields.get("app", "")
    if not app:
        return {"success": False, "error": "No app name provided"}
    try:
        system = platform.system()
        if system == "Windows":
            os.startfile(app)
        elif system == "Darwin":
            subprocess.Popen(["open", "-a", app])
        else:  # Linux
            subprocess.Popen([app], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return {"success": True, "message": f"Launched: {app}"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_system_info(payload, fields):
    info = {
        "os": f"{platform.system()} {platform.release()} ({platform.machine()})",
        "hostname": platform.node(),
        "python": platform.python_version(),
    }
    # Try psutil for detailed info
    try:
        import psutil
        mem = psutil.virtual_memory()
        disk = psutil.disk_usage("/")
        info["cpu"] = f"{psutil.cpu_count()} cores, {psutil.cpu_percent()}% usage"
        info["ram"] = f"{mem.used // (1024**3)}GB / {mem.total // (1024**3)}GB ({mem.percent}%)"
        info["disk"] = f"{disk.used // (1024**3)}GB / {disk.total // (1024**3)}GB ({disk.percent}%)"
    except ImportError:
        # Fallback without psutil
        info["cpu"] = f"{os.cpu_count()} cores"
        info["ram"] = "Install psutil for RAM info"
        info["disk"] = "Install psutil for disk info"
    info["success"] = True
    return info


def handle_list_processes(payload, fields):
    try:
        system = platform.system()
        if system == "Windows":
            result = subprocess.run(
                ["tasklist"], capture_output=True, text=True, timeout=10
            )
        else:
            result = subprocess.run(
                ["ps", "aux"], capture_output=True, text=True, timeout=10
            )
        return {"success": True, "output": result.stdout}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_kill_process(payload, fields):
    target = payload or fields.get("target", "")
    if not target:
        return {"success": False, "error": "No process name/PID provided"}
    try:
        system = platform.system()
        if system == "Windows":
            # Try as PID first, then as name
            if target.isdigit():
                cmd = ["taskkill", "/F", "/PID", target]
            else:
                cmd = ["taskkill", "/F", "/IM", target]
        else:
            if target.isdigit():
                cmd = ["kill", "-9", target]
            else:
                cmd = ["pkill", "-9", "-f", target]
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        if result.returncode == 0:
            return {"success": True, "message": f"Killed: {target}"}
        else:
            return {"success": False, "error": result.stderr or result.stdout}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_clipboard_get(payload, fields):
    # Try pyperclip first
    try:
        import pyperclip
        content = pyperclip.paste()
        return {"success": True, "content": content}
    except ImportError:
        pass
    # Fallback to OS commands
    try:
        system = platform.system()
        if system == "Darwin":
            result = subprocess.run(["pbpaste"], capture_output=True, text=True, timeout=5)
            return {"success": True, "content": result.stdout}
        elif system == "Windows":
            # PowerShell fallback
            result = subprocess.run(
                ["powershell", "-command", "Get-Clipboard"],
                capture_output=True, text=True, timeout=5
            )
            return {"success": True, "content": result.stdout}
        else:
            result = subprocess.run(
                ["xclip", "-selection", "clipboard", "-o"],
                capture_output=True, text=True, timeout=5
            )
            return {"success": True, "content": result.stdout}
    except Exception as e:
        return {"success": False, "error": f"Clipboard unavailable: {e}"}


def handle_clipboard_set(payload, fields):
    text = payload or fields.get("text", "")
    # Try pyperclip first
    try:
        import pyperclip
        pyperclip.copy(text)
        return {"success": True, "message": "Clipboard set"}
    except ImportError:
        pass
    # Fallback to OS commands
    try:
        system = platform.system()
        if system == "Darwin":
            proc = subprocess.Popen(["pbcopy"], stdin=subprocess.PIPE)
            proc.communicate(text.encode("utf-8"))
            return {"success": True, "message": "Clipboard set"}
        elif system == "Windows":
            proc = subprocess.Popen(
                ["powershell", "-command", "$input | Set-Clipboard"],
                stdin=subprocess.PIPE
            )
            proc.communicate(text.encode("utf-8"))
            return {"success": True, "message": "Clipboard set"}
        else:
            proc = subprocess.Popen(
                ["xclip", "-selection", "clipboard"],
                stdin=subprocess.PIPE
            )
            proc.communicate(text.encode("utf-8"))
            return {"success": True, "message": "Clipboard set"}
    except Exception as e:
        return {"success": False, "error": f"Clipboard unavailable: {e}"}


def handle_screenshot(payload, fields):
    try:
        from PIL import ImageGrab
        path = os.path.join(tempfile.gettempdir(), f"mk_screenshot_{int(time.time())}.png")
        img = ImageGrab.grab()
        img.save(path)
        return {"success": True, "path": path}
    except ImportError:
        pass
    # Fallback to OS commands
    try:
        system = platform.system()
        path = os.path.join(tempfile.gettempdir(), f"mk_screenshot_{int(time.time())}.png")
        if system == "Darwin":
            subprocess.run(["screencapture", "-x", path], timeout=10)
            return {"success": True, "path": path}
        elif system == "Windows":
            # PowerShell screenshot
            ps_cmd = (
                f"Add-Type -AssemblyName System.Windows.Forms;"
                f"[System.Windows.Forms.Screen]::PrimaryScreen | ForEach-Object {{"
                f"$bmp = New-Object System.Drawing.Bitmap($_.Bounds.Width, $_.Bounds.Height);"
                f"$g = [System.Drawing.Graphics]::FromImage($bmp);"
                f"$g.CopyFromScreen($_.Bounds.Location, [System.Drawing.Point]::Empty, $_.Bounds.Size);"
                f"$bmp.Save('{path}')}}"
            )
            subprocess.run(["powershell", "-command", ps_cmd], timeout=10)
            return {"success": True, "path": path}
        else:
            # Try scrot on Linux
            subprocess.run(["scrot", path], timeout=10)
            return {"success": True, "path": path}
    except Exception as e:
        return {"success": False, "error": f"Screenshot failed: {e}"}


def handle_list_files(payload, fields):
    directory = payload or fields.get("directory", ".")
    try:
        directory = os.path.expanduser(directory)
        entries = os.listdir(directory)
        # Format nicely with type indicators
        output_lines = []
        for entry in sorted(entries):
            full = os.path.join(directory, entry)
            if os.path.isdir(full):
                output_lines.append(f"[DIR]  {entry}/")
            else:
                try:
                    size = os.path.getsize(full)
                    if size < 1024:
                        sz = f"{size}B"
                    elif size < 1048576:
                        sz = f"{size // 1024}KB"
                    else:
                        sz = f"{size // 1048576}MB"
                    output_lines.append(f"[FILE] {entry} ({sz})")
                except OSError:
                    output_lines.append(f"[FILE] {entry}")
        return {"success": True, "output": "\n".join(output_lines)}
    except Exception as e:
        return {"success": False, "error": str(e)}


# ─── Command Registry ─────────────────────────────────────────────────────────

COMMANDS = {
    "ping": handle_ping,
    "shell": handle_shell,
    "read_file": handle_read_file,
    "write_file": handle_write_file,
    "open_app": handle_open_app,
    "system_info": handle_system_info,
    "list_processes": handle_list_processes,
    "kill_process": handle_kill_process,
    "clipboard_get": handle_clipboard_get,
    "clipboard_set": handle_clipboard_set,
    "screenshot": handle_screenshot,
    "list_files": handle_list_files,
}

# ─── Client Handler ───────────────────────────────────────────────────────────

def handle_client(conn, addr, auth_token):
    """Handle a single client connection (one command per line, persistent)."""
    log("INFO", f"Connection from {addr[0]}:{addr[1]}")

    # IP whitelist check
    if ALLOWED_IPS and addr[0] not in ALLOWED_IPS:
        log("WARN", f"Rejected connection from non-whitelisted IP: {addr[0]}")
        conn.close()
        return

    conn.settimeout(60.0)  # 60s idle timeout
    buffer = ""

    try:
        while True:
            try:
                data = conn.recv(4096)
                if not data:
                    break
                buffer += data.decode("utf-8", errors="replace")
            except socket.timeout:
                break

            # Process complete lines
            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip()
                if not line:
                    continue

                # Parse JSON request
                try:
                    request = json.loads(line)
                except json.JSONDecodeError:
                    response = {"success": False, "error": "Invalid JSON"}
                    conn.sendall((json.dumps(response) + "\n").encode("utf-8"))
                    continue

                req_id = request.get("id", 0)
                req_type = request.get("type", "")
                req_token = request.get("token", "")
                req_payload = request.get("payload", "")

                # Auth check
                if req_token != auth_token:
                    log("WARN", f"Auth failed from {addr[0]} (bad token)")
                    response = {"id": req_id, "success": False, "error": "Authentication failed"}
                    conn.sendall((json.dumps(response) + "\n").encode("utf-8"))
                    continue

                # Execute command
                log("CMD", f"[{addr[0]}] {req_type}: {req_payload[:80]}")
                handler = COMMANDS.get(req_type)
                if handler:
                    try:
                        response = handler(req_payload, request)
                    except Exception as e:
                        response = {"success": False, "error": f"Handler error: {e}"}
                else:
                    response = {"success": False, "error": f"Unknown command: {req_type}"}

                response["id"] = req_id
                conn.sendall((json.dumps(response) + "\n").encode("utf-8"))

    except (ConnectionResetError, BrokenPipeError):
        pass
    except Exception as e:
        log("ERR", f"Client error: {e}")
    finally:
        conn.close()
        log("INFO", f"Disconnected: {addr[0]}:{addr[1]}")


# ─── Server ───────────────────────────────────────────────────────────────────

def start_server(host, port, auth_token):
    """Start the TCP agent server."""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server.bind((host, port))
    except OSError as e:
        log("ERR", f"Cannot bind to {host}:{port} - {e}")
        sys.exit(1)

    server.listen(5)
    log("INFO", f"MK PC Agent listening on {host}:{port}")
    log("INFO", f"Platform: {platform.system()} {platform.release()}")
    log("INFO", "Waiting for MK connection...")
    print()

    try:
        while True:
            conn, addr = server.accept()
            # Handle each connection in a thread
            t = threading.Thread(target=handle_client, args=(conn, addr, auth_token), daemon=True)
            t.start()
    except KeyboardInterrupt:
        log("INFO", "Shutting down...")
    finally:
        server.close()


# ─── Main ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="MK OS - Remote PC Agent")
    parser.add_argument("--token", default=os.environ.get("MK_PC_TOKEN", ""),
                        help="Authentication token (required)")
    parser.add_argument("--port", type=int,
                        default=int(os.environ.get("MK_PC_PORT", DEFAULT_PORT)),
                        help=f"Listen port (default: {DEFAULT_PORT})")
    parser.add_argument("--host", default=DEFAULT_HOST,
                        help=f"Listen address (default: {DEFAULT_HOST})")
    parser.add_argument("--allow-ips", nargs="*", default=[],
                        help="Whitelist of allowed client IPs (default: allow all)")
    args = parser.parse_args()

    if not args.token:
        print("\n  \033[31mError:\033[0m Auth token required!")
        print("  Usage: python agent.py --token YOUR_SECRET_TOKEN")
        print("  Or set MK_PC_TOKEN environment variable.\n")
        sys.exit(1)

    if args.allow_ips:
        global ALLOWED_IPS
        ALLOWED_IPS = args.allow_ips
        log("INFO", f"IP whitelist: {ALLOWED_IPS}")

    print()
    print("  \033[1m\033[96m╭─────────────────────────────────────╮\033[0m")
    print("  \033[1m\033[96m│       MK OS - PC Agent v1.0         │\033[0m")
    print("  \033[1m\033[96m╰─────────────────────────────────────╯\033[0m")
    print()

    start_server(args.host, args.port, args.token)


if __name__ == "__main__":
    main()
