#ifndef MK_SSH_CONTROLLER_CPP
#define MK_SSH_CONTROLLER_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <sstream>
#include <cstdio>
#include <array>
#include <chrono>

// ===================================================================================
// MK SSH CONTROLLER
// Remote command execution via SSH. Uses popen with ssh command.
// Methods: executeRemote(device, command) -> output, copyFile(src, dst, path),
// checkAlive(device) -> bool. Manages SSH key paths. Timeout protection.
// ===================================================================================

struct MKSSHConfig {
    std::string hostname;
    std::string ip;
    std::string user;
    int port;
    std::string key_path;
    int timeout_seconds;
    bool strict_host_checking;
};

struct MKSSHResult {
    std::string output;
    int exit_code;
    float duration_ms;
    bool timed_out;
    bool success;
    std::string error;
};

class MKSSHController {
private:
    std::map<std::string, MKSSHConfig> configs;
    int default_timeout;
    int max_retries;
    std::vector<std::string> execution_log;

    // Escape a string for safe inclusion inside single quotes.
    // Every single quote in the input becomes: '\'' (end quote, escaped quote, start quote).
    static std::string shellEscapeSingleQuotes(const std::string& input) {
        std::string escaped;
        escaped.reserve(input.size() + 16);
        for (char c : input) {
            if (c == '\'') {
                escaped += "'\\''";
            } else {
                escaped += c;
            }
        }
        return escaped;
    }

    // Build the SSH command string
    std::string buildSSHCommand(const MKSSHConfig& config, const std::string& remote_cmd) const {
        std::ostringstream cmd;
        cmd << "ssh";
        cmd << " -o ConnectTimeout=" << config.timeout_seconds;
        if (!config.strict_host_checking) {
            cmd << " -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null";
        }
        cmd << " -o BatchMode=yes";
        if (!config.key_path.empty()) {
            cmd << " -i " << config.key_path;
        }
        if (config.port != 22) {
            cmd << " -p " << config.port;
        }
        cmd << " " << config.user << "@" << config.ip;
        // Sanitize remote command to prevent shell injection via single-quote breakout
        cmd << " '" << shellEscapeSingleQuotes(remote_cmd) << "'";
        cmd << " 2>&1";
        return cmd.str();
    }

    // Build the SCP command string
    std::string buildSCPCommand(const MKSSHConfig& src_config,
                                const MKSSHConfig& dst_config,
                                const std::string& src_path,
                                const std::string& dst_path) const {
        std::ostringstream cmd;
        cmd << "scp -o ConnectTimeout=" << default_timeout;
        cmd << " -o StrictHostKeyChecking=no -o BatchMode=yes";
        if (!src_config.key_path.empty()) {
            cmd << " -i " << src_config.key_path;
        }
        if (src_config.port != 22) {
            cmd << " -P " << src_config.port;
        }
        cmd << " " << src_config.user << "@" << src_config.ip << ":" << src_path;
        cmd << " " << dst_config.user << "@" << dst_config.ip << ":" << dst_path;
        cmd << " 2>&1";
        return cmd.str();
    }

    // Execute a shell command with timeout
    MKSSHResult executeWithTimeout(const std::string& cmd, int timeout_sec) const {
        MKSSHResult result;
        result.exit_code = -1;
        result.timed_out = false;
        result.success = false;
        result.duration_ms = 0.0f;

        auto start = std::chrono::steady_clock::now();

        // Wrap with timeout command
        std::string timeout_cmd;
        #ifdef __APPLE__
        // macOS uses gtimeout from coreutils, or we rely on SSH's own timeout
        timeout_cmd = cmd;
        #else
        timeout_cmd = "timeout " + std::to_string(timeout_sec) + " " + cmd;
        #endif

        std::array<char, 4096> buffer;
        FILE* pipe = popen(timeout_cmd.c_str(), "r");
        if (!pipe) {
            result.error = "Failed to open pipe";
            return result;
        }

        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
            result.output += buffer.data();
        }

        int status = pclose(pipe);
        auto end = std::chrono::steady_clock::now();
        result.duration_ms = std::chrono::duration<float, std::milli>(end - start).count();

        #ifndef __APPLE__
        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
            if (result.exit_code == 124) {
                result.timed_out = true;
                result.error = "Command timed out after " + std::to_string(timeout_sec) + "s";
            }
        }
        #else
        result.exit_code = status;
        #endif

        result.success = (result.exit_code == 0);
        return result;
    }

public:
    MKSSHController() : default_timeout(30), max_retries(2) {}

    // Register a device for SSH access
    void registerDevice(const std::string& hostname, const std::string& ip,
                       const std::string& user, int port = 22,
                       const std::string& key_path = "",
                       int timeout = 30) {
        MKSSHConfig config;
        config.hostname = hostname;
        config.ip = ip;
        config.user = user;
        config.port = port;
        config.key_path = key_path;
        config.timeout_seconds = timeout;
        config.strict_host_checking = false;
        configs[hostname] = config;
    }

    // Execute a command on a remote device
    MKSSHResult executeRemote(const std::string& hostname, const std::string& command) {
        auto it = configs.find(hostname);
        if (it == configs.end()) {
            MKSSHResult result;
            result.success = false;
            result.error = "Device not registered: " + hostname;
            return result;
        }

        std::string ssh_cmd = buildSSHCommand(it->second, command);
        MKSSHResult result = executeWithTimeout(ssh_cmd, it->second.timeout_seconds);

        // Log execution
        std::ostringstream log_entry;
        log_entry << "[" << std::time(nullptr) << "] " << hostname << ": " << command
                  << " -> " << (result.success ? "OK" : "FAIL")
                  << " (" << (int)result.duration_ms << "ms)";
        execution_log.push_back(log_entry.str());

        return result;
    }

    // Copy a file between devices
    MKSSHResult copyFile(const std::string& src_host, const std::string& dst_host,
                        const std::string& src_path, const std::string& dst_path) {
        auto src_it = configs.find(src_host);
        auto dst_it = configs.find(dst_host);

        if (src_it == configs.end() || dst_it == configs.end()) {
            MKSSHResult result;
            result.success = false;
            result.error = "Source or destination device not registered";
            return result;
        }

        std::string scp_cmd = buildSCPCommand(src_it->second, dst_it->second, src_path, dst_path);
        return executeWithTimeout(scp_cmd, default_timeout * 2);
    }

    // Check if a device is alive (ping + SSH check)
    bool checkAlive(const std::string& hostname) {
        auto it = configs.find(hostname);
        if (it == configs.end()) return false;

        // Quick SSH echo test
        std::string cmd = buildSSHCommand(it->second, "echo alive");
        MKSSHResult result = executeWithTimeout(cmd, 10);
        return result.success && result.output.find("alive") != std::string::npos;
    }

    // Get registered device count
    int deviceCount() const { return static_cast<int>(configs.size()); }

    // Get all registered hostnames
    std::vector<std::string> getRegisteredDevices() const {
        std::vector<std::string> names;
        for (const auto& kv : configs) {
            names.push_back(kv.first);
        }
        return names;
    }

    // Check if a device is registered
    bool isRegistered(const std::string& hostname) const {
        return configs.find(hostname) != configs.end();
    }

    // Set default timeout
    void setDefaultTimeout(int seconds) { default_timeout = seconds; }

    // Get execution log
    std::vector<std::string> getLog() const { return execution_log; }

    // Clear log
    void clearLog() { execution_log.clear(); }

    // Get summary
    std::string getSummary() const {
        std::ostringstream ss;
        ss << "=== SSH Controller ===\n";
        ss << "  Registered devices: " << configs.size() << "\n";
        ss << "  Default timeout: " << default_timeout << "s\n";
        ss << "  Execution log entries: " << execution_log.size() << "\n";
        for (const auto& kv : configs) {
            ss << "  " << kv.first << " -> " << kv.second.user << "@"
               << kv.second.ip << ":" << kv.second.port << "\n";
        }
        return ss.str();
    }
};

#endif // MK_SSH_CONTROLLER_CPP
