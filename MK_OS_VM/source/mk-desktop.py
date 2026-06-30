#!/usr/bin/env python3
"""MK OS Desktop Environment - Terminal Based"""
import os, sys, time, curses
from datetime import datetime

class MKDesktop:
    def __init__(self, stdscr):
        self.stdscr = stdscr
        self.running = True
        self.menu_open = False
        self.selected_menu = 0
        self.apps = [
            {"name": "Terminal", "icon": "[>_]", "action": "terminal"},
            {"name": "Files", "icon": "[D]", "action": "files"},
            {"name": "System Info", "icon": "[i]", "action": "sysinfo"},
            {"name": "Settings", "icon": "[S]", "action": "settings"},
            {"name": "About MK OS", "icon": "[?]", "action": "about"},
            {"name": "Power", "icon": "[P]", "action": "power"},
        ]
        self.setup_colors()
        self.setup_screen()

    def setup_colors(self):
        curses.start_color()
        curses.use_default_colors()
        curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLUE)
        curses.init_pair(2, curses.COLOR_BLACK, curses.COLOR_WHITE)
        curses.init_pair(3, curses.COLOR_WHITE, curses.COLOR_BLACK)
        curses.init_pair(4, curses.COLOR_CYAN, curses.COLOR_BLACK)
        curses.init_pair(5, curses.COLOR_GREEN, curses.COLOR_BLACK)
        curses.init_pair(6, curses.COLOR_BLACK, curses.COLOR_CYAN)

    def setup_screen(self):
        curses.curs_set(0)
        self.stdscr.nodelay(True)
        self.stdscr.keypad(True)
        self.height, self.width = self.stdscr.getmaxyx()

    def draw_taskbar(self):
        y = self.height - 1
        try:
            self.stdscr.addstr(y, 0, " " * (self.width - 1), curses.color_pair(1))
            self.stdscr.addstr(y, 1, " MK ", curses.color_pair(6) | curses.A_BOLD)
            now = datetime.now().strftime("%H:%M")
            date_str = datetime.now().strftime("%b %d")
            clock = f" {date_str}  {now} "
            self.stdscr.addstr(y, self.width - len(clock) - 1, clock, curses.color_pair(1))
        except curses.error: pass

    def draw_desktop(self):
        for y in range(self.height - 1):
            try: self.stdscr.addstr(y, 0, " " * (self.width - 1), curses.color_pair(3))
            except: pass
        title = "MK OS 2.0"
        try: self.stdscr.addstr(0, (self.width - len(title)) // 2, title, curses.color_pair(4) | curses.A_BOLD)
        except: pass
        for i, app in enumerate(self.apps):
            y = 3 + (i * 3)
            if y < self.height - 4:
                try:
                    self.stdscr.addstr(y, 4, app["icon"], curses.color_pair(4) | curses.A_BOLD)
                    self.stdscr.addstr(y + 1, 4, app["name"], curses.color_pair(3))
                except: pass

    def draw_menu(self):
        if not self.menu_open: return
        mw, mh = 30, len(self.apps) + 4
        my = self.height - 1 - mh
        mx = 1
        for y in range(mh):
            try: self.stdscr.addstr(my + y, mx, " " * mw, curses.color_pair(2))
            except: pass
        try:
            self.stdscr.addstr(my, mx + 1, " MK OS Menu ", curses.color_pair(6) | curses.A_BOLD)
            self.stdscr.addstr(my + 1, mx + 1, "-" * (mw - 2), curses.color_pair(2))
        except: pass
        for i, app in enumerate(self.apps):
            attr = curses.color_pair(6) if i == self.selected_menu else curses.color_pair(2)
            prefix = " > " if i == self.selected_menu else "   "
            try: self.stdscr.addstr(my + 2 + i, mx + 1, f"{prefix}{app['icon']} {app['name']}", attr)
            except: pass

    def show_window(self, title, lines, w=50, h=14):
        y = (self.height - h) // 2
        x = (self.width - w) // 2
        for row in range(h):
            try: self.stdscr.addstr(y + row, x, " " * w, curses.color_pair(2))
            except: pass
        try:
            self.stdscr.addstr(y, x, " " * w, curses.color_pair(6))
            self.stdscr.addstr(y, x + 2, title, curses.color_pair(6) | curses.A_BOLD)
            self.stdscr.addstr(y, x + w - 4, "[X]", curses.color_pair(6))
        except: pass
        for i, line in enumerate(lines):
            if y + 1 + i < y + h:
                try: self.stdscr.addstr(y + 1 + i, x + 1, line, curses.color_pair(2))
                except: pass
        self.stdscr.refresh()
        self.stdscr.nodelay(False)
        self.stdscr.getch()
        self.stdscr.nodelay(True)

    def show_about(self):
        self.show_window("About MK OS", ["", "    MK OS Version 2.0", "", "    Created by Mohammed", "    Built on Arch Linux ARM", "", "    A custom operating system", "    experience designed for", "    simplicity and power.", "", "    Press any key to close..."])

    def show_sysinfo(self):
        try:
            u = os.uname()
            kernel, arch, host = u.release, u.machine, u.nodename
        except: kernel, arch, host = "unknown", "aarch64", "MK-OS"
        try:
            with open('/proc/meminfo') as f: mem = f"{int(f.readline().split()[1])//1024} MB"
        except: mem = "Unknown"
        try:
            up = float(open('/proc/uptime').read().split()[0])
            uptime = f"{int(up//60)}m {int(up%60)}s"
        except: uptime = "Unknown"
        self.show_window("System Information", ["", f"  OS:        MK OS 2.0", f"  Kernel:    {kernel}", f"  Arch:      {arch}", f"  Host:      {host}", f"  Memory:    {mem}", f"  Uptime:    {uptime}", f"  Shell:     /bin/bash", f"  Desktop:   MK Desktop", "", "  Press any key to close..."], w=55, h=14)

    def show_settings(self):
        self.show_window("Settings", ["", "  [1] Change Hostname", "  [2] Network Settings", "  [3] Display Settings", "  [4] User Account", "", "  Settings coming in future updates.", "  Press any key to close..."])

    def show_files(self):
        try: entries = os.listdir(os.path.expanduser("~"))[:10]
        except: entries = ["Documents", "Downloads"]
        lines = ["", "  Name", "  " + "-"*40]
        for e in entries:
            icon = "[D]" if os.path.isdir(os.path.join(os.path.expanduser("~"), e)) else "[F]"
            lines.append(f"  {icon} {e}")
        lines += ["", "  Press any key to close..."]
        self.show_window("MK Files", lines, w=50, h=len(lines)+2)

    def show_power_menu(self):
        w, h = 35, 9
        y = (self.height - h) // 2
        x = (self.width - w) // 2
        for row in range(h):
            try: self.stdscr.addstr(y + row, x, " " * w, curses.color_pair(2))
            except: pass
        try:
            self.stdscr.addstr(y, x, " " * w, curses.color_pair(6))
            self.stdscr.addstr(y, x + 2, "Power", curses.color_pair(6) | curses.A_BOLD)
        except: pass
        for i, line in enumerate(["", "  [R] Reboot", "  [S] Shutdown", "  [L] Lock Screen", "  [C] Cancel", ""]):
            try: self.stdscr.addstr(y + 1 + i, x + 1, line, curses.color_pair(2))
            except: pass
        self.stdscr.refresh()
        self.stdscr.nodelay(False)
        key = self.stdscr.getch()
        self.stdscr.nodelay(True)
        if key in (ord('r'), ord('R')):
            self.running = False; curses.endwin(); os.system("reboot")
        elif key in (ord('s'), ord('S')):
            self.running = False; curses.endwin(); os.system("poweroff")
        elif key in (ord('l'), ord('L')):
            self.show_lockscreen()

    def show_lockscreen(self):
        self.stdscr.nodelay(False)
        while True:
            self.stdscr.clear()
            h, w = self.height, self.width
            for row in range(h):
                try: self.stdscr.addstr(row, 0, " " * (w - 1), curses.color_pair(3))
                except: pass
            now = datetime.now()
            time_str = now.strftime("%H:%M")
            date_str = now.strftime("%A, %B %d")
            cy = h // 2 - 3
            try:
                self.stdscr.addstr(cy, (w - len(time_str)) // 2, time_str, curses.color_pair(4) | curses.A_BOLD)
                self.stdscr.addstr(cy + 2, (w - len(date_str)) // 2, date_str, curses.color_pair(3))
                self.stdscr.addstr(cy + 5, (w - 21) // 2, "Press ENTER to unlock", curses.color_pair(5))
            except: pass
            self.stdscr.refresh()
            key = self.stdscr.getch()
            if key in (10, 13): break
        self.stdscr.nodelay(True)

    def open_terminal(self):
        curses.endwin()
        os.system("clear")
        print("\033[1;36m" + "=" * 36)
        print("     MK OS Terminal v2.0")
        print("  Type 'exit' to return to MK OS")
        print("=" * 36 + "\033[0m\n")
        os.system("/bin/bash")
        self.stdscr = curses.initscr()
        curses.start_color()
        curses.use_default_colors()
        self.setup_colors()
        self.setup_screen()

    def handle_action(self, action):
        actions = {"terminal": self.open_terminal, "files": self.show_files, "sysinfo": self.show_sysinfo, "settings": self.show_settings, "about": self.show_about, "power": self.show_power_menu}
        if action in actions: actions[action]()

    def handle_input(self):
        try: key = self.stdscr.getch()
        except: return
        if key == -1: return
        if self.menu_open:
            if key == curses.KEY_UP: self.selected_menu = (self.selected_menu - 1) % len(self.apps)
            elif key == curses.KEY_DOWN: self.selected_menu = (self.selected_menu + 1) % len(self.apps)
            elif key in (10, 13):
                self.menu_open = False
                self.handle_action(self.apps[self.selected_menu]["action"])
            elif key == 27: self.menu_open = False
        else:
            if key in (27, ord('m'), ord('M')): self.menu_open = True; self.selected_menu = 0
            elif key in (ord('t'), ord('T')): self.open_terminal()
            elif key in (ord('l'), ord('L')): self.show_lockscreen()
            elif key in (ord('q'), ord('Q')): self.show_power_menu()

    def run(self):
        while self.running:
            try:
                self.draw_desktop()
                self.draw_taskbar()
                self.draw_menu()
                self.stdscr.refresh()
                self.handle_input()
                time.sleep(0.05)
            except curses.error: pass
            except KeyboardInterrupt: break

def main(stdscr):
    MKDesktop(stdscr).run()

if __name__ == "__main__":
    sys.stdout.write("\033]0;MK OS 2.0\007")
    sys.stdout.flush()
    curses.wrapper(main)
