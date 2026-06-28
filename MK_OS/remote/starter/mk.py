#!/usr/bin/env python3
"""
MK OS — Beautiful CLI Client
==============================
Connects directly to MK on Mac (port 9878) for interactive queries.
No external dependencies — uses only Python standard library.

Usage:
    python mk.py
    python mk.py --host 192.168.1.50 --port 9878
"""

import socket
import json
import sys
import os
import time
import threading
import readline  # enables up/down history

# ─── ANSI Colors ──────────────────────────────────────────────────────────────

CYAN    = "\033[96m"
GREEN   = "\033[92m"
YELLOW  = "\033[93m"
RED     = "\033[91m"
BLUE    = "\033[94m"
MAGENTA = "\033[95m"
BOLD    = "\033[1m"
DIM     = "\033[2m"
RESET   = "\033[0m"
CLEAR   = "\033[2J\033[H"

# ─── Configuration ────────────────────────────────────────────────────────────

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 9878
RECV_TIMEOUT = 30  # seconds


def load_config():
    """Load MK_SERVER_IP, MK_SERVER_PORT, TOKEN from config.txt or env."""
    config = {
        "host": os.environ.get("MK_SERVER_IP", DEFAULT_HOST),
        "port": int(os.environ.get("MK_SERVER_PORT", DEFAULT_PORT)),
        "token": os.environ.get("MK_PC_TOKEN", ""),
    }

    # Try config.txt in same directory
    config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "config.txt")
    if os.path.exists(config_path):
        with open(config_path, "r") as f:
            for line in f:
                line = line.strip()
                if line.startswith("#") or "=" not in line:
                    continue
                key, val = line.split("=", 1)
                key, val = key.strip(), val.strip()
                if key == "MK_SERVER_IP":
                    config["host"] = val
                elif key == "MK_SERVER_PORT":
                    config["port"] = int(val)
                elif key == "TOKEN":
                    config["token"] = val

    return config


# ─── Network ──────────────────────────────────────────────────────────────────

class MKConnection:
    """Manages TCP connection to MK server."""

    def __init__(self, host, port, token):
        self.host = host
        self.port = port
        self.token = token
        self.sock = None
        self.connected = False

    def connect(self):
        """Establish connection to MK server."""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(5)
            self.sock.connect((self.host, self.port))
            self.sock.settimeout(RECV_TIMEOUT)
            self.connected = True
            return True
        except (socket.error, OSError) as e:
            self.connected = False
            return False

    def disconnect(self):
        """Close connection."""
        if self.sock:
            try:
                self.sock.close()
            except Exception:
                pass
        self.sock = None
        self.connected = False

    def send_query(self, text):
        """Send a query and receive response. Returns (success, response_text)."""
        if not self.connected:
            return False, "Not connected"

        request = json.dumps({
            "type": "query",
            "text": text,
            "token": self.token,
        }) + "\n"

        try:
            self.sock.sendall(request.encode("utf-8"))

            # Receive response (newline-delimited)
            buffer = ""
            while "\n" not in buffer:
                data = self.sock.recv(4096)
                if not data:
                    self.connected = False
                    return False, "Connection lost"
                buffer += data.decode("utf-8", errors="replace")

            line = buffer.split("\n", 1)[0].strip()
            response = json.loads(line)

            if response.get("success"):
                return True, response.get("response", "")
            else:
                return False, response.get("error", "Unknown error")

        except socket.timeout:
            return False, "Request timed out"
        except (socket.error, OSError) as e:
            self.connected = False
            return False, f"Connection error: {e}"
        except json.JSONDecodeError:
            return False, "Invalid response from server"

    def ping(self):
        """Check if server is responsive."""
        if not self.connected:
            return False
        request = json.dumps({"type": "ping", "token": self.token}) + "\n"
        try:
            self.sock.sendall(request.encode("utf-8"))
            buffer = ""
            self.sock.settimeout(5)
            while "\n" not in buffer:
                data = self.sock.recv(1024)
                if not data:
                    return False
                buffer += data.decode("utf-8", errors="replace")
            self.sock.settimeout(RECV_TIMEOUT)
            resp = json.loads(buffer.split("\n")[0])
            return resp.get("success", False)
        except Exception:
            self.connected = False
            return False


# ─── UI Helpers ───────────────────────────────────────────────────────────────

def print_banner():
    """Print the startup banner with animation."""
    frames = ["◐", "◓", "◑", "◒"]
    sys.stdout.write(f"\n  {DIM}Initializing MK CLI...{RESET}")
    sys.stdout.flush()
    for i in range(6):
        sys.stdout.write(f"\r  {CYAN}{frames[i % 4]}{RESET} {DIM}Initializing MK CLI...{RESET}")
        sys.stdout.flush()
        time.sleep(0.1)
    sys.stdout.write("\r" + " " * 40 + "\r")

    print(f"""
  {BOLD}{CYAN}╭─────────────────────────────────────────────╮{RESET}
  {BOLD}{CYAN}│{RESET}{BOLD}          MK OS — Remote CLI v1.0           {BOLD}{CYAN}│{RESET}
  {BOLD}{CYAN}╰─────────────────────────────────────────────╯{RESET}
""")


def print_response(text):
    """Format and display MK's response in a nice box."""
    lines = text.split("\n")
    max_width = max(len(l) for l in lines) if lines else 40
    box_width = min(max(max_width + 4, 40), 70)

    print(f"\n  {CYAN}╭─ {BOLD}MK{RESET}{CYAN} {'─' * (box_width - 5)}{RESET}")
    print(f"  {CYAN}│{RESET}")
    for line in lines:
        print(f"  {CYAN}│{RESET}  {GREEN}●{RESET} {line}")
    print(f"  {CYAN}│{RESET}")
    print(f"  {CYAN}╰{'─' * box_width}{RESET}")


def print_error(text):
    """Display error message."""
    print(f"\n  {RED}✗{RESET} {text}")


def print_warning(text):
    """Display warning message."""
    print(f"\n  {YELLOW}○{RESET} {text}")


def print_info(text):
    """Display info message."""
    print(f"  {DIM}{text}{RESET}")


def spinner_wait(stop_event, message="Thinking"):
    """Animated spinner while waiting for response."""
    frames = ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]
    i = 0
    while not stop_event.is_set():
        sys.stdout.write(f"\r  {CYAN}{frames[i % len(frames)]}{RESET} {DIM}{message}...{RESET}  ")
        sys.stdout.flush()
        i += 1
        time.sleep(0.08)
    # Clear spinner line
    sys.stdout.write("\r" + " " * 40 + "\r")
    sys.stdout.flush()


# ─── Local Commands ───────────────────────────────────────────────────────────

def handle_local_command(cmd, conn):
    """Handle /commands locally. Returns True if handled."""
    cmd_lower = cmd.lower().strip()

    if cmd_lower in ("/quit", "/exit", "/q"):
        print(f"\n  {DIM}Disconnecting...{RESET}")
        conn.disconnect()
        print(f"  {GREEN}✓{RESET} Goodbye.\n")
        sys.exit(0)

    elif cmd_lower == "/clear":
        sys.stdout.write(CLEAR)
        sys.stdout.flush()
        return True

    elif cmd_lower == "/status":
        if conn.connected:
            alive = conn.ping()
            if alive:
                print(f"\n  {GREEN}●{RESET} Connected to {BOLD}{conn.host}:{conn.port}{RESET}")
                print(f"  {DIM}  Server is responsive{RESET}")
            else:
                print(f"\n  {YELLOW}○{RESET} Socket open to {conn.host}:{conn.port}")
                print(f"  {DIM}  Server not responding to ping{RESET}")
        else:
            print(f"\n  {RED}○{RESET} Disconnected")
        return True

    elif cmd_lower == "/reconnect":
        print(f"  {DIM}Reconnecting to {conn.host}:{conn.port}...{RESET}")
        conn.disconnect()
        if conn.connect():
            print(f"  {GREEN}✓{RESET} Reconnected successfully")
        else:
            print_error(f"Could not connect to {conn.host}:{conn.port}")
        return True

    elif cmd_lower == "/help":
        print(f"""
  {BOLD}{CYAN}╭─────────────────────────────────────────────╮{RESET}
  {BOLD}{CYAN}│{RESET}{BOLD}           MK CLI — Commands                 {BOLD}{CYAN}│{RESET}
  {BOLD}{CYAN}╰─────────────────────────────────────────────╯{RESET}

  {GREEN}/help{RESET}        Show this help
  {GREEN}/status{RESET}      Show connection status
  {GREEN}/reconnect{RESET}   Reconnect to MK server
  {GREEN}/clear{RESET}       Clear screen
  {GREEN}/quit{RESET}        Exit the CLI

  {DIM}Or just type naturally — MK will answer.{RESET}
""")
        return True

    return False


# ─── Main Loop ────────────────────────────────────────────────────────────────

def main():
    config = load_config()

    # Parse command line overrides
    args = sys.argv[1:]
    i = 0
    while i < len(args):
        if args[i] in ("--host", "-h") and i + 1 < len(args):
            config["host"] = args[i + 1]
            i += 2
        elif args[i] in ("--port", "-p") and i + 1 < len(args):
            config["port"] = int(args[i + 1])
            i += 2
        elif args[i] in ("--token", "-t") and i + 1 < len(args):
            config["token"] = args[i + 1]
            i += 2
        elif args[i] in ("--help",):
            print(f"Usage: python mk.py [--host IP] [--port PORT] [--token TOKEN]")
            sys.exit(0)
        else:
            i += 1

    # Show banner
    print_banner()

    # Connect to MK
    conn = MKConnection(config["host"], config["port"], config["token"])
    print_info(f"Connecting to MK at {config['host']}:{config['port']}...")

    if conn.connect():
        print(f"  {GREEN}✓{RESET} Connected to MK server\n")
    else:
        print_error(f"Cannot reach MK at {config['host']}:{config['port']}")
        print_info("Make sure MK is running on your Mac with the query server enabled.")
        print_info("Use /reconnect to try again, or check config.txt\n")

    # Setup readline history
    histfile = os.path.join(os.path.expanduser("~"), ".mk_cli_history")
    try:
        readline.read_history_file(histfile)
    except FileNotFoundError:
        pass
    readline.set_history_length(500)

    # Main input loop
    try:
        while True:
            try:
                user_input = input(f"  {BOLD}{CYAN}MK{RESET}{CYAN} › {RESET}")
            except EOFError:
                print()
                break

            user_input = user_input.strip()
            if not user_input:
                continue

            # Check for local commands
            if user_input.startswith("/"):
                if handle_local_command(user_input, conn):
                    continue
                # If not a local command, send it to MK as-is

            # Check connection
            if not conn.connected:
                print_warning("Not connected. Trying to reconnect...")
                if not conn.connect():
                    print_error(f"Cannot reach MK at {conn.host}:{conn.port}")
                    print_info("Use /reconnect to try again.")
                    continue
                print(f"  {GREEN}✓{RESET} Reconnected!")

            # Send query with spinner
            stop_event = threading.Event()
            spinner_thread = threading.Thread(
                target=spinner_wait, args=(stop_event, "Thinking"), daemon=True
            )
            spinner_thread.start()

            success, response = conn.send_query(user_input)

            stop_event.set()
            spinner_thread.join(timeout=1)

            # Display result
            if success:
                print_response(response)
            else:
                if "Connection" in response or "lost" in response:
                    print_error(f"Lost connection: {response}")
                    conn.connected = False
                    print_info("Use /reconnect to try again.")
                else:
                    print_error(response)

    except KeyboardInterrupt:
        print(f"\n\n  {DIM}Interrupted. Goodbye.{RESET}\n")
    finally:
        # Save history
        try:
            readline.write_history_file(histfile)
        except Exception:
            pass
        conn.disconnect()


if __name__ == "__main__":
    main()
