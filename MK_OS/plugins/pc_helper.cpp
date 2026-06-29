#ifndef MK_PC_HELPER_CPP
#define MK_PC_HELPER_CPP

#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <algorithm>
#include <cstring>
#include <unordered_set>

class MKPCHelper {
private:
    // Command history
    std::vector<std::string> commandHistory;
    static constexpr size_t MAX_HISTORY = 100;

    // Safe command allowlist - only these prefixes are permitted for runCommand
    static const std::unordered_set<std::string>& getAllowedCommands() {
        static const std::unordered_set<std::string> allowed = {
            "ls", "pwd", "whoami", "date", "uptime", "df", "du",
            "cat", "head", "tail", "wc", "echo", "uname", "hostname",
            "free", "top", "ps", "which", "file", "stat", "id",
            "lscpu", "lsblk", "ip", "ifconfig", "ping", "nslookup",
            "env", "printenv", "cal", "bc", "expr"
        };
        return allowed;
    }

    // Execute a command and capture output
    std::string executeCapture(const std::string& cmd) {
        std::string result;
        std::array<char, 256> buffer;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "[error: could not execute command]";
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
            result += buffer.data();
        }
        pclose(pipe);
        return result;
    }

    // Check if a command is safe
    bool isCommandSafe(const std::string& command) const {
        if (command.empty()) return false;

        // Block dangerous patterns
        static const std::vector<std::string> blocked = {
            "rm ", "rm\t", "rmdir", "mkfs", "dd ", "dd\t",
            "chmod 777", ":(){ ", "fork", "> /dev/",
            "sudo", "su ", "passwd", "shutdown", "reboot",
            "kill -9", "killall", "pkill -9", "wget ", "curl ",
            "nc ", "ncat ", "bash -c", "sh -c", "eval ",
            "/etc/shadow", "/etc/passwd"
        };
        std::string lower = command;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (const auto& pattern : blocked) {
            if (lower.find(pattern) != std::string::npos) return false;
        }

        // Check allowlist: first word of command must be in allowlist
        std::string firstWord;
        std::istringstream iss(command);
        iss >> firstWord;
        // Strip path prefix
        size_t slashPos = firstWord.rfind('/');
        if (slashPos != std::string::npos) {
            firstWord = firstWord.substr(slashPos + 1);
        }

        return getAllowedCommands().count(firstWord) > 0;
    }

public:
    MKPCHelper() {
        // Silent initialization - no verbose output to keep boot clean
    }

    // ============================================================
    // System Information
    // ============================================================

    struct SystemInfo {
        std::string cpuModel;
        int cpuCores = 0;
        long totalMemMB = 0;
        long freeMemMB = 0;
        long totalDiskMB = 0;
        long freeDiskMB = 0;
        int batteryPercent = -1; // -1 means not available
        bool onAC = true;
        std::string osName;
        std::string hostname;
        std::string uptime;
    };

    SystemInfo getSystemInfo() {
        SystemInfo info;

        // OS Name
#ifdef __APPLE__
        info.osName = "macOS";
#elif __linux__
        info.osName = "Linux";
        std::ifstream osRelease("/etc/os-release");
        if (osRelease.is_open()) {
            std::string line;
            while (std::getline(osRelease, line)) {
                if (line.find("PRETTY_NAME=") == 0) {
                    info.osName = line.substr(13);
                    // Remove trailing quote
                    if (!info.osName.empty() && info.osName.back() == '"')
                        info.osName.pop_back();
                    break;
                }
            }
        }
#else
        info.osName = "Unknown";
#endif

        // Hostname
        std::string hostnameOut = executeCapture("hostname 2>/dev/null");
        if (!hostnameOut.empty()) {
            hostnameOut.erase(hostnameOut.find_last_not_of(" \n\r\t") + 1);
            info.hostname = hostnameOut;
        }

        // CPU Info
#ifdef __linux__
        std::ifstream cpuinfo("/proc/cpuinfo");
        if (cpuinfo.is_open()) {
            std::string line;
            int coreCount = 0;
            while (std::getline(cpuinfo, line)) {
                if (line.find("model name") == 0 && info.cpuModel.empty()) {
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        info.cpuModel = line.substr(pos + 2);
                    }
                }
                if (line.find("processor") == 0) coreCount++;
            }
            info.cpuCores = coreCount;
        }
#elif defined(__APPLE__)
        info.cpuModel = executeCapture("sysctl -n machdep.cpu.brand_string 2>/dev/null");
        if (!info.cpuModel.empty()) info.cpuModel.erase(info.cpuModel.find_last_not_of(" \n\r\t") + 1);
        std::string coreStr = executeCapture("sysctl -n hw.ncpu 2>/dev/null");
        if (!coreStr.empty()) info.cpuCores = std::atoi(coreStr.c_str());
#endif

        // Memory Info
#ifdef __linux__
        std::ifstream meminfo("/proc/meminfo");
        if (meminfo.is_open()) {
            std::string line;
            while (std::getline(meminfo, line)) {
                if (line.find("MemTotal:") == 0) {
                    long kb = 0;
                    std::sscanf(line.c_str(), "MemTotal: %ld kB", &kb);
                    info.totalMemMB = kb / 1024;
                }
                if (line.find("MemAvailable:") == 0) {
                    long kb = 0;
                    std::sscanf(line.c_str(), "MemAvailable: %ld kB", &kb);
                    info.freeMemMB = kb / 1024;
                }
            }
        }
#elif defined(__APPLE__)
        std::string memStr = executeCapture("sysctl -n hw.memsize 2>/dev/null");
        if (!memStr.empty()) info.totalMemMB = std::atol(memStr.c_str()) / (1024 * 1024);
#endif

        // Disk Info
        std::string dfOut = executeCapture("df -m / 2>/dev/null | tail -1");
        if (!dfOut.empty()) {
            std::istringstream dfs(dfOut);
            std::string dev;
            long total = 0, used = 0, avail = 0;
            dfs >> dev >> total >> used >> avail;
            info.totalDiskMB = total;
            info.freeDiskMB = avail;
        }

        // Battery (Linux)
#ifdef __linux__
        std::ifstream batCap("/sys/class/power_supply/BAT0/capacity");
        if (batCap.is_open()) {
            batCap >> info.batteryPercent;
        }
        std::ifstream batStatus("/sys/class/power_supply/BAT0/status");
        if (batStatus.is_open()) {
            std::string status;
            std::getline(batStatus, status);
            info.onAC = (status == "Charging" || status == "Full");
        }
#elif defined(__APPLE__)
        std::string pmOut = executeCapture("pmset -g batt 2>/dev/null | grep -o '[0-9]*%'");
        if (!pmOut.empty()) {
            info.batteryPercent = std::atoi(pmOut.c_str());
        }
#endif

        // Uptime
        std::string uptimeOut = executeCapture("uptime -p 2>/dev/null || uptime 2>/dev/null");
        if (!uptimeOut.empty()) {
            uptimeOut.erase(uptimeOut.find_last_not_of(" \n\r\t") + 1);
            info.uptime = uptimeOut;
        }

        return info;
    }

    std::string formatSystemInfo() {
        auto info = getSystemInfo();
        std::ostringstream ss;
        ss << "System Information:\n";
        ss << "  OS:       " << info.osName << "\n";
        ss << "  Host:     " << info.hostname << "\n";
        ss << "  CPU:      " << info.cpuModel << " (" << info.cpuCores << " cores)\n";
        ss << "  Memory:   " << info.freeMemMB << " MB free / " << info.totalMemMB << " MB total\n";
        ss << "  Disk:     " << info.freeDiskMB << " MB free / " << info.totalDiskMB << " MB total\n";
        if (info.batteryPercent >= 0) {
            ss << "  Battery:  " << info.batteryPercent << "%" << (info.onAC ? " (charging)" : " (on battery)") << "\n";
        }
        if (!info.uptime.empty()) {
            ss << "  Uptime:   " << info.uptime << "\n";
        }
        return ss.str();
    }

    // ============================================================
    // Process Management
    // ============================================================

    struct ProcessInfo {
        int pid;
        std::string user;
        std::string command;
        float cpuPercent;
        float memPercent;
    };

    std::vector<ProcessInfo> listProcesses(int limit = 15) {
        std::vector<ProcessInfo> procs;
        std::string cmd = "ps aux --sort=-%cpu 2>/dev/null | head -" + std::to_string(limit + 1);
        std::string output = executeCapture(cmd);
        if (output.empty()) {
            // macOS fallback
            cmd = "ps aux -r 2>/dev/null | head -" + std::to_string(limit + 1);
            output = executeCapture(cmd);
        }

        std::istringstream iss(output);
        std::string line;
        std::getline(iss, line); // skip header
        while (std::getline(iss, line)) {
            if (line.empty()) continue;
            ProcessInfo p;
            std::istringstream ls(line);
            std::string pidStr, cpuStr, memStr;
            std::string vsz, rss, tty, stat, start, timeStr;
            ls >> p.user >> pidStr >> cpuStr >> memStr >> vsz >> rss >> tty >> stat >> start >> timeStr;
            // Rest is command
            std::getline(ls, p.command);
            if (!p.command.empty() && p.command[0] == ' ') p.command = p.command.substr(1);
            p.pid = std::atoi(pidStr.c_str());
            p.cpuPercent = static_cast<float>(std::atof(cpuStr.c_str()));
            p.memPercent = static_cast<float>(std::atof(memStr.c_str()));
            procs.push_back(p);
        }
        return procs;
    }

    std::string formatProcessList(int limit = 10) {
        auto procs = listProcesses(limit);
        std::ostringstream ss;
        ss << "Top " << procs.size() << " processes by CPU:\n";
        ss << "  PID      CPU%   MEM%   COMMAND\n";
        for (const auto& p : procs) {
            char buf[128];
            std::snprintf(buf, sizeof(buf), "  %-8d %-6.1f %-6.1f %s\n",
                         p.pid, p.cpuPercent, p.memPercent, p.command.substr(0, 40).c_str());
            ss << buf;
        }
        return ss.str();
    }

    bool killProcess(int pid) {
        // Only allow killing user processes, not system ones (PID > 1000)
        if (pid <= 1000) {
            std::cerr << "[PC HELPER] Refusing to kill system process (PID <= 1000)\n";
            return false;
        }
        std::string cmd = "kill " + std::to_string(pid) + " 2>/dev/null";
        return std::system(cmd.c_str()) == 0;
    }

    // ============================================================
    // Clipboard Integration
    // ============================================================

    bool copyToClipboard(const std::string& text) {
        std::string cmd;
#ifdef __APPLE__
        cmd = "pbcopy";
#elif __linux__
        // Try xclip first, then xsel
        if (std::system("which xclip >/dev/null 2>&1") == 0) {
            cmd = "xclip -selection clipboard";
        } else if (std::system("which xsel >/dev/null 2>&1") == 0) {
            cmd = "xsel --clipboard --input";
        } else {
            std::cerr << "[PC HELPER] No clipboard tool found (install xclip or xsel)\n";
            return false;
        }
#else
        return false;
#endif
        FILE* pipe = popen(cmd.c_str(), "w");
        if (!pipe) return false;
        fwrite(text.c_str(), 1, text.size(), pipe);
        pclose(pipe);
        return true;
    }

    std::string getFromClipboard() {
#ifdef __APPLE__
        return executeCapture("pbpaste 2>/dev/null");
#elif __linux__
        if (std::system("which xclip >/dev/null 2>&1") == 0) {
            return executeCapture("xclip -selection clipboard -o 2>/dev/null");
        } else if (std::system("which xsel >/dev/null 2>&1") == 0) {
            return executeCapture("xsel --clipboard --output 2>/dev/null");
        }
#endif
        return "";
    }

    // ============================================================
    // Safe Command Execution
    // ============================================================

    // Execute a safe command with output capture
    std::string runCommand(const std::string& command) {
        if (command.empty()) {
            return "[error: empty command]";
        }

        if (!isCommandSafe(command)) {
            return "[error: command not in allowlist or contains blocked patterns]\n"
                   "Allowed commands: ls, pwd, whoami, date, uptime, df, du, cat, head, "
                   "tail, wc, echo, uname, hostname, free, ps, which, file, stat, id, "
                   "lscpu, lsblk, ip, ifconfig, ping, cal, bc, expr";
        }

        // Record in history
        commandHistory.push_back(command);
        if (commandHistory.size() > MAX_HISTORY) {
            commandHistory.erase(commandHistory.begin());
        }

        return executeCapture(command + " 2>&1");
    }

    // ============================================================
    // Command History
    // ============================================================

    const std::vector<std::string>& getHistory() const {
        return commandHistory;
    }

    std::string formatHistory(size_t limit = 20) const {
        std::ostringstream ss;
        ss << "Command History (last " << std::min(limit, commandHistory.size()) << "):\n";
        size_t start = commandHistory.size() > limit ? commandHistory.size() - limit : 0;
        for (size_t i = start; i < commandHistory.size(); i++) {
            ss << "  " << (i + 1) << ". " << commandHistory[i] << "\n";
        }
        return ss.str();
    }

    void clearHistory() {
        commandHistory.clear();
    }

    // ============================================================
    // File Operations (Safe)
    // ============================================================

    void openFile(const std::string& filename) {
        if (filename.empty()) return;
        std::string command;
#if defined(__APPLE__)
        command = "open \"" + filename + "\" 2>/dev/null &";
#elif defined(__linux__)
        command = "xdg-open \"" + filename + "\" 2>/dev/null &";
#else
        command = "start \"\" \"" + filename + "\" 2>/dev/null";
#endif
        std::system(command.c_str());
    }

    // ============================================================
    // Screenshot Capability Detection
    // ============================================================

    bool hasScreenshotCapability() {
#ifdef __APPLE__
        return true; // macOS always has screencapture
#elif __linux__
        return (std::system("which gnome-screenshot >/dev/null 2>&1") == 0 ||
                std::system("which scrot >/dev/null 2>&1") == 0 ||
                std::system("which maim >/dev/null 2>&1") == 0);
#else
        return false;
#endif
    }

    std::string takeScreenshot(const std::string& path = "/tmp/mk_screenshot.png") {
        std::string cmd;
#ifdef __APPLE__
        cmd = "screencapture -x \"" + path + "\" 2>/dev/null";
#elif __linux__
        if (std::system("which maim >/dev/null 2>&1") == 0) {
            cmd = "maim \"" + path + "\" 2>/dev/null";
        } else if (std::system("which scrot >/dev/null 2>&1") == 0) {
            cmd = "scrot \"" + path + "\" 2>/dev/null";
        } else if (std::system("which gnome-screenshot >/dev/null 2>&1") == 0) {
            cmd = "gnome-screenshot -f \"" + path + "\" 2>/dev/null";
        } else {
            return "[error: no screenshot tool available]";
        }
#else
        return "[error: unsupported platform]";
#endif
        if (std::system(cmd.c_str()) == 0) {
            return path;
        }
        return "[error: screenshot failed]";
    }
};

#endif // MK_PC_HELPER_CPP
