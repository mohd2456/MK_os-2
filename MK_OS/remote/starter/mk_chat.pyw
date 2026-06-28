#!/usr/bin/env pythonw
"""
MK OS — Tray Icon + Chat Window
=================================
A system tray application with a dark-themed chat window.
Connects to MK on Mac (port 9878) for interactive queries.
Uses tkinter (built-in) + pystray (pip).

Usage:
    pythonw mk_chat.pyw
    (or double-click START_MK_CHAT.bat)
"""

import tkinter as tk
from tkinter import scrolledtext, font as tkfont
import socket
import json
import os
import sys
import threading
import time
from datetime import datetime

# ─── Configuration ────────────────────────────────────────────────────────────

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 9878
RECV_TIMEOUT = 30

# Dark theme colors
BG_DARK      = "#1e1e1e"
BG_CHAT      = "#252526"
BG_INPUT     = "#2d2d2d"
BG_USER_MSG  = "#264f78"
BG_MK_MSG    = "#1e3a1e"
FG_TEXT      = "#d4d4d4"
FG_DIM       = "#808080"
FG_CYAN      = "#4ec9b0"
FG_GREEN     = "#6a9955"
FG_BLUE      = "#569cd6"
FG_USER      = "#9cdcfe"
FG_MK        = "#4ec9b0"
ACCENT       = "#007acc"


def load_config():
    """Load connection settings from config.txt or env."""
    config = {
        "host": os.environ.get("MK_SERVER_IP", DEFAULT_HOST),
        "port": int(os.environ.get("MK_SERVER_PORT", DEFAULT_PORT)),
        "token": os.environ.get("MK_PC_TOKEN", ""),
    }
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


# ─── Network Connection ───────────────────────────────────────────────────────

class MKConnection:
    """TCP connection to MK query server."""

    def __init__(self, host, port, token):
        self.host = host
        self.port = port
        self.token = token
        self.sock = None
        self.connected = False
        self.lock = threading.Lock()

    def connect(self):
        with self.lock:
            try:
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.sock.settimeout(5)
                self.sock.connect((self.host, self.port))
                self.sock.settimeout(RECV_TIMEOUT)
                self.connected = True
                return True
            except (socket.error, OSError):
                self.connected = False
                return False

    def disconnect(self):
        with self.lock:
            if self.sock:
                try:
                    self.sock.close()
                except Exception:
                    pass
            self.sock = None
            self.connected = False

    def send_query(self, text):
        with self.lock:
            if not self.connected:
                return False, "Not connected"
            request = json.dumps({
                "type": "query", "text": text, "token": self.token
            }) + "\n"
            try:
                self.sock.sendall(request.encode("utf-8"))
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


# ─── Chat Window ──────────────────────────────────────────────────────────────

class MKChatWindow:
    """Dark-themed chat window with message bubbles."""

    def __init__(self, conn):
        self.conn = conn
        self.root = None
        self.chat_area = None
        self.input_field = None
        self.status_label = None
        self.visible = False
        self.tray_icon = None

    def create_window(self):
        """Create the main chat window."""
        self.root = tk.Tk()
        self.root.title("MK OS")
        self.root.geometry("500x650")
        self.root.configure(bg=BG_DARK)
        self.root.minsize(400, 400)

        # Set window icon (if available)
        try:
            self.root.iconbitmap(default="")
        except Exception:
            pass

        # Override close to minimize to tray
        self.root.protocol("WM_DELETE_WINDOW", self.hide_window)

        # ─── Title Bar ────────────────────────────────────────
        title_frame = tk.Frame(self.root, bg=BG_DARK, height=45)
        title_frame.pack(fill=tk.X, padx=0, pady=0)
        title_frame.pack_propagate(False)

        title_label = tk.Label(
            title_frame, text="  MK OS", font=("Segoe UI", 14, "bold"),
            fg=FG_CYAN, bg=BG_DARK, anchor="w"
        )
        title_label.pack(side=tk.LEFT, padx=10, pady=8)

        self.status_label = tk.Label(
            title_frame, text="● Connected", font=("Segoe UI", 9),
            fg=FG_GREEN, bg=BG_DARK, anchor="e"
        )
        self.status_label.pack(side=tk.RIGHT, padx=10, pady=8)

        # Separator
        sep = tk.Frame(self.root, bg=ACCENT, height=2)
        sep.pack(fill=tk.X)

        # ─── Chat Area ────────────────────────────────────────
        chat_frame = tk.Frame(self.root, bg=BG_CHAT)
        chat_frame.pack(fill=tk.BOTH, expand=True, padx=0, pady=0)

        self.chat_area = scrolledtext.ScrolledText(
            chat_frame, wrap=tk.WORD, state=tk.DISABLED,
            bg=BG_CHAT, fg=FG_TEXT, font=("Consolas", 10),
            insertbackground=FG_TEXT, selectbackground=ACCENT,
            relief=tk.FLAT, padx=12, pady=10, spacing3=4,
            cursor="arrow"
        )
        self.chat_area.pack(fill=tk.BOTH, expand=True)

        # Configure text tags for styling
        self.chat_area.tag_configure("user_name", foreground=FG_USER,
                                     font=("Segoe UI", 9, "bold"))
        self.chat_area.tag_configure("mk_name", foreground=FG_MK,
                                     font=("Segoe UI", 9, "bold"))
        self.chat_area.tag_configure("user_msg", foreground="#ffffff",
                                     lmargin1=20, lmargin2=20)
        self.chat_area.tag_configure("mk_msg", foreground=FG_TEXT,
                                     lmargin1=20, lmargin2=20)
        self.chat_area.tag_configure("timestamp", foreground=FG_DIM,
                                     font=("Segoe UI", 8))
        self.chat_area.tag_configure("error", foreground="#f44747")
        self.chat_area.tag_configure("system", foreground=FG_DIM,
                                     font=("Segoe UI", 9, "italic"),
                                     justify="center")

        # ─── Input Area ───────────────────────────────────────
        input_frame = tk.Frame(self.root, bg=BG_INPUT, height=50)
        input_frame.pack(fill=tk.X, side=tk.BOTTOM)
        input_frame.pack_propagate(False)

        self.input_field = tk.Entry(
            input_frame, bg=BG_INPUT, fg=FG_TEXT,
            font=("Consolas", 11), insertbackground=FG_TEXT,
            relief=tk.FLAT, highlightthickness=1,
            highlightcolor=ACCENT, highlightbackground="#3e3e3e"
        )
        self.input_field.pack(side=tk.LEFT, fill=tk.BOTH, expand=True,
                              padx=(12, 5), pady=10)
        self.input_field.bind("<Return>", self.on_send)

        send_btn = tk.Button(
            input_frame, text="Send", command=self.on_send,
            bg=ACCENT, fg="white", font=("Segoe UI", 9, "bold"),
            relief=tk.FLAT, activebackground="#005a9e",
            activeforeground="white", cursor="hand2", padx=12
        )
        send_btn.pack(side=tk.RIGHT, padx=(0, 12), pady=10)

        # Focus input
        self.input_field.focus_set()

        # Show welcome message
        self.add_system_message("Welcome to MK OS Chat")
        if self.conn.connected:
            self.update_status(True)
        else:
            self.update_status(False)
            self.add_system_message("Attempting to connect...")
            threading.Thread(target=self._try_connect, daemon=True).start()

    def _try_connect(self):
        """Try to connect in background."""
        if self.conn.connect():
            self.root.after(0, lambda: self.update_status(True))
            self.root.after(0, lambda: self.add_system_message("Connected to MK"))
        else:
            self.root.after(0, lambda: self.update_status(False))
            self.root.after(0, lambda: self.add_system_message(
                f"Cannot reach MK at {self.conn.host}:{self.conn.port}"))

    def update_status(self, connected):
        """Update the status indicator."""
        if connected:
            self.status_label.config(text="● Connected", fg=FG_GREEN)
        else:
            self.status_label.config(text="○ Disconnected", fg="#f44747")

    def add_system_message(self, text):
        """Add a system/info message."""
        self.chat_area.config(state=tk.NORMAL)
        self.chat_area.insert(tk.END, f"\n  {text}\n", "system")
        self.chat_area.config(state=tk.DISABLED)
        self.chat_area.see(tk.END)

    def add_user_message(self, text):
        """Add a user message to the chat."""
        ts = datetime.now().strftime("%H:%M")
        self.chat_area.config(state=tk.NORMAL)
        self.chat_area.insert(tk.END, f"\n You  ", "user_name")
        self.chat_area.insert(tk.END, f"{ts}\n", "timestamp")
        self.chat_area.insert(tk.END, f" {text}\n", "user_msg")
        self.chat_area.config(state=tk.DISABLED)
        self.chat_area.see(tk.END)

    def add_mk_message(self, text):
        """Add an MK response to the chat."""
        ts = datetime.now().strftime("%H:%M")
        self.chat_area.config(state=tk.NORMAL)
        self.chat_area.insert(tk.END, f"\n MK  ", "mk_name")
        self.chat_area.insert(tk.END, f"{ts}\n", "timestamp")
        self.chat_area.insert(tk.END, f" {text}\n", "mk_msg")
        self.chat_area.config(state=tk.DISABLED)
        self.chat_area.see(tk.END)

    def add_error_message(self, text):
        """Add an error message."""
        self.chat_area.config(state=tk.NORMAL)
        self.chat_area.insert(tk.END, f"\n ✗ {text}\n", "error")
        self.chat_area.config(state=tk.DISABLED)
        self.chat_area.see(tk.END)

    def on_send(self, event=None):
        """Handle send button / Enter key."""
        text = self.input_field.get().strip()
        if not text:
            return

        self.input_field.delete(0, tk.END)
        self.add_user_message(text)

        # Handle local commands
        if text.lower() in ("/clear",):
            self.chat_area.config(state=tk.NORMAL)
            self.chat_area.delete("1.0", tk.END)
            self.chat_area.config(state=tk.DISABLED)
            return
        if text.lower() in ("/reconnect",):
            self.conn.disconnect()
            threading.Thread(target=self._try_connect, daemon=True).start()
            return

        # Send to MK in background thread
        threading.Thread(target=self._send_query, args=(text,), daemon=True).start()

    def _send_query(self, text):
        """Send query in background and update UI."""
        if not self.conn.connected:
            if not self.conn.connect():
                self.root.after(0, lambda: self.add_error_message(
                    f"Cannot connect to MK at {self.conn.host}:{self.conn.port}"))
                self.root.after(0, lambda: self.update_status(False))
                return
            self.root.after(0, lambda: self.update_status(True))

        success, response = self.conn.send_query(text)

        if success:
            self.root.after(0, lambda: self.add_mk_message(response))
        else:
            self.root.after(0, lambda: self.add_error_message(response))
            if "Connection" in response or "lost" in response:
                self.root.after(0, lambda: self.update_status(False))

    def show_window(self):
        """Show the chat window."""
        if self.root:
            self.root.deiconify()
            self.root.lift()
            self.root.focus_force()
            self.input_field.focus_set()
            self.visible = True

    def hide_window(self):
        """Hide window to tray."""
        if self.root:
            self.root.withdraw()
            self.visible = False

    def quit_app(self):
        """Fully quit the application."""
        self.conn.disconnect()
        if self.tray_icon:
            self.tray_icon.stop()
        if self.root:
            self.root.quit()
            self.root.destroy()


# ─── System Tray ──────────────────────────────────────────────────────────────

def create_tray_icon(chat_window):
    """Create system tray icon using pystray."""
    try:
        import pystray
        from PIL import Image, ImageDraw
    except ImportError:
        # pystray not available — just show window directly
        print("Note: pystray/Pillow not installed. Running without tray icon.")
        print("Install with: pip install pystray Pillow")
        return None

    # Create a simple MK icon (cyan circle with "M" letter)
    img = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Draw filled circle
    draw.ellipse([4, 4, 60, 60], fill=(78, 201, 176, 255))
    # Draw "M" letter
    draw.text((20, 16), "M", fill=(30, 30, 30, 255))

    def on_show(icon, item):
        chat_window.root.after(0, chat_window.show_window)

    def on_quit(icon, item):
        chat_window.root.after(0, chat_window.quit_app)

    menu = pystray.Menu(
        pystray.MenuItem("Show MK Chat", on_show, default=True),
        pystray.Menu.SEPARATOR,
        pystray.MenuItem("Quit", on_quit),
    )

    icon = pystray.Icon("MK OS", img, "MK OS Chat", menu)
    return icon


# ─── Main ─────────────────────────────────────────────────────────────────────

def main():
    config = load_config()

    # Create connection
    conn = MKConnection(config["host"], config["port"], config["token"])
    conn.connect()  # Try initial connection (non-blocking on failure)

    # Create chat window
    chat = MKChatWindow(conn)
    chat.create_window()

    # Try to create tray icon
    tray_icon = create_tray_icon(chat)
    if tray_icon:
        chat.tray_icon = tray_icon
        # Run tray icon in a background thread
        tray_thread = threading.Thread(target=tray_icon.run, daemon=True)
        tray_thread.start()

    # Show window
    chat.visible = True

    # Run tkinter mainloop
    try:
        chat.root.mainloop()
    except KeyboardInterrupt:
        pass
    finally:
        conn.disconnect()
        if tray_icon:
            try:
                tray_icon.stop()
            except Exception:
                pass


if __name__ == "__main__":
    main()
