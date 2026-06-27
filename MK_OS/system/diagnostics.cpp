#ifndef MK_DIAGNOSTICS_CPP
#define MK_DIAGNOSTICS_CPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <array>
#include <algorithm>

// ===================================================================================
// MK SYSTEM DIAGNOSTICS
// ===================================================================================
// On-demand system health checks that MK can run to report system status.
// Returns structured health reports with OK / WARNING / CRITICAL for each metric.
//
// Supported checks:
//   - CPU temperature (sysctl on macOS, /sys/class/thermal on Linux)
//   - Disk space (total/used/free per mount)
//   - Memory usage (total RAM, used, free, swap)
//   - Network connectivity (ping a reliable host)
//   - Process count and top CPU consumers
//   - Uptime and load average
//   - Battery status (if laptop)
// ===================================================================================

enum class HealthStatus {
    OK,
    WARNING,
    CRITICAL,
    UNKNOWN
};

struct DiagnosticResult {
    std::string metric_name;
    HealthStatus status;
    std::string value;
    std::string detail;
};

struct DiskInfo {
    std::string mount_point;
    long total_mb;
    long used_mb;
    long free_mb;
    int usage_percent;
};

struct MemoryInfo {
    long total_mb;
    long used_mb;
    long free_mb;
    long swap_total_mb;
    long swap_used_mb;
};

struct ProcessInfo {
    int total_count;
    std::vector<std::string> top_consumers;
};

class MKDiagnostics {
private:
    std::vector<DiagnosticResult> results_;
    std::string last_report_time_;

    std::string exec_command(const std::string& cmd) {
        std::string output;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        char buffer[512];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            output += buffer;
        }
        pclose(pipe);
        // Trim trailing newline
        while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
            output.pop_back();
        }
        return output;
    }

    std::string status_to_string(HealthStatus s) {
        switch (s) {
            case HealthStatus::OK:       return "OK";
            case HealthStatus::WARNING:  return "WARNING";
            case HealthStatus::CRITICAL: return "CRITICAL";
            default:                     return "UNKNOWN";
        }
    }

    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t_now));
        return std::string(buf);
    }

public:
    MKDiagnostics() {
        std::cout << "[DIAGNOSTICS] System health diagnostics module initialized.\n";
    }

    // ---------------------------------------------------------------
    // CPU Temperature
    // ---------------------------------------------------------------
    DiagnosticResult check_cpu_temperature() {
        DiagnosticResult result;
        result.metric_name = "CPU Temperature";
        int temp_celsius = -1;

#ifdef __APPLE__
        // macOS: use sysctl for thermal level
        std::string output = exec_command("sysctl -n machdep.xcpm.cpu_thermal_level 2>/dev/null");
        if (!output.empty()) {
            int level = std::stoi(output);
            temp_celsius = 40 + (level * 12); // Estimate from thermal level
        } else {
            // Fallback: try powermetrics (requires sudo) or estimate
            temp_celsius = 55; // Default estimate
        }
#else
        // Linux: read from /sys/class/thermal
        std::ifstream thermal("/sys/class/thermal/thermal_zone0/temp");
        if (thermal.is_open()) {
            int raw_temp;
            thermal >> raw_temp;
            temp_celsius = raw_temp / 1000; // Convert millidegrees to degrees
            thermal.close();
        } else {
            temp_celsius = -1;
        }
#endif

        if (temp_celsius < 0) {
            result.status = HealthStatus::UNKNOWN;
            result.value = "N/A";
            result.detail = "Unable to read CPU temperature sensor";
        } else if (temp_celsius < 65) {
            result.status = HealthStatus::OK;
            result.value = std::to_string(temp_celsius) + " C";
            result.detail = "Temperature within normal range";
        } else if (temp_celsius < 85) {
            result.status = HealthStatus::WARNING;
            result.value = std::to_string(temp_celsius) + " C";
            result.detail = "CPU running warm, consider reducing load";
        } else {
            result.status = HealthStatus::CRITICAL;
            result.value = std::to_string(temp_celsius) + " C";
            result.detail = "CPU overheating! Thermal throttling likely active";
        }

        results_.push_back(result);
        return result;
    }

    // ---------------------------------------------------------------
    // Disk Space
    // ---------------------------------------------------------------
    DiagnosticResult check_disk_space() {
        DiagnosticResult result;
        result.metric_name = "Disk Space";
        std::vector<DiskInfo> disks;

        std::string output = exec_command("df -m 2>/dev/null | grep -E '^/' | head -10");
        std::istringstream stream(output);
        std::string line;

        while (std::getline(stream, line)) {
            std::istringstream ls(line);
            std::string device;
            long total, used, free;
            int percent;
            std::string mount;

            ls >> device >> total >> used >> free;
            std::string pct_str;
            ls >> pct_str >> mount;

            if (!pct_str.empty()) {
                percent = std::stoi(pct_str);
            } else {
                percent = (total > 0) ? (int)((used * 100) / total) : 0;
            }

            DiskInfo di;
            di.mount_point = mount.empty() ? "/" : mount;
            di.total_mb = total;
            di.used_mb = used;
            di.free_mb = free;
            di.usage_percent = percent;
            disks.push_back(di);
        }

        if (disks.empty()) {
            result.status = HealthStatus::UNKNOWN;
            result.value = "N/A";
            result.detail = "Unable to read disk information";
        } else {
            int max_usage = 0;
            std::string detail_str;
            for (const auto& d : disks) {
                if (d.usage_percent > max_usage) max_usage = d.usage_percent;
                detail_str += d.mount_point + ": " + std::to_string(d.used_mb) + "MB/"
                           + std::to_string(d.total_mb) + "MB (" + std::to_string(d.usage_percent) + "%); ";
            }

            if (max_usage < 80) {
                result.status = HealthStatus::OK;
            } else if (max_usage < 95) {
                result.status = HealthStatus::WARNING;
            } else {
                result.status = HealthStatus::CRITICAL;
            }
            result.value = std::to_string(max_usage) + "% max usage";
            result.detail = detail_str;
        }

        results_.push_back(result);
        return result;
    }

    // ---------------------------------------------------------------
    // Memory Usage
    // ---------------------------------------------------------------
    DiagnosticResult check_memory() {
        DiagnosticResult result;
        result.metric_name = "Memory Usage";
        MemoryInfo mem{0, 0, 0, 0, 0};

#ifdef __APPLE__
        // macOS: use sysctl and vm_stat
        std::string total_str = exec_command("sysctl -n hw.memsize 2>/dev/null");
        if (!total_str.empty()) {
            mem.total_mb = std::stol(total_str) / (1024 * 1024);
        }
        std::string vm_output = exec_command("vm_stat 2>/dev/null | head -5");
        // Parse page size and free pages
        long page_size = 4096;
        long free_pages = 0;
        std::istringstream vm_stream(vm_output);
        std::string vm_line;
        while (std::getline(vm_stream, vm_line)) {
            if (vm_line.find("page size") != std::string::npos) {
                size_t pos = vm_line.find_last_of(' ');
                if (pos != std::string::npos) {
                    page_size = std::stol(vm_line.substr(pos + 1));
                }
            }
            if (vm_line.find("Pages free") != std::string::npos) {
                size_t colon = vm_line.find(':');
                if (colon != std::string::npos) {
                    std::string val = vm_line.substr(colon + 1);
                    val.erase(std::remove(val.begin(), val.end(), '.'), val.end());
                    val.erase(std::remove(val.begin(), val.end(), ' '), val.end());
                    if (!val.empty()) free_pages = std::stol(val);
                }
            }
        }
        mem.free_mb = (free_pages * page_size) / (1024 * 1024);
        mem.used_mb = mem.total_mb - mem.free_mb;

        // Swap info
        std::string swap_str = exec_command("sysctl -n vm.swapusage 2>/dev/null");
        if (swap_str.find("total") != std::string::npos) {
            // Parse "total = 2048.00M  used = 512.00M  free = 1536.00M"
            size_t tpos = swap_str.find("total = ");
            size_t upos = swap_str.find("used = ");
            if (tpos != std::string::npos) {
                mem.swap_total_mb = (long)std::stof(swap_str.substr(tpos + 8));
            }
            if (upos != std::string::npos) {
                mem.swap_used_mb = (long)std::stof(swap_str.substr(upos + 7));
            }
        }
#else
        // Linux: read /proc/meminfo
        std::ifstream meminfo("/proc/meminfo");
        if (meminfo.is_open()) {
            std::string line;
            while (std::getline(meminfo, line)) {
                if (line.find("MemTotal:") == 0) {
                    mem.total_mb = std::stol(line.substr(10)) / 1024;
                } else if (line.find("MemFree:") == 0) {
                    mem.free_mb = std::stol(line.substr(9)) / 1024;
                } else if (line.find("SwapTotal:") == 0) {
                    mem.swap_total_mb = std::stol(line.substr(11)) / 1024;
                } else if (line.find("SwapFree:") == 0) {
                    long swap_free = std::stol(line.substr(10)) / 1024;
                    mem.swap_used_mb = mem.swap_total_mb - swap_free;
                }
            }
            mem.used_mb = mem.total_mb - mem.free_mb;
        }
#endif

        int usage_percent = (mem.total_mb > 0) ? (int)((mem.used_mb * 100) / mem.total_mb) : 0;

        if (usage_percent < 75) {
            result.status = HealthStatus::OK;
        } else if (usage_percent < 90) {
            result.status = HealthStatus::WARNING;
        } else {
            result.status = HealthStatus::CRITICAL;
        }

        result.value = std::to_string(usage_percent) + "% used";
        result.detail = "Total: " + std::to_string(mem.total_mb) + "MB, Used: "
                      + std::to_string(mem.used_mb) + "MB, Free: " + std::to_string(mem.free_mb)
                      + "MB, Swap: " + std::to_string(mem.swap_used_mb) + "/"
                      + std::to_string(mem.swap_total_mb) + "MB";

        results_.push_back(result);
        return result;
    }

    // ---------------------------------------------------------------
    // Network Connectivity
    // ---------------------------------------------------------------
    DiagnosticResult check_network() {
        DiagnosticResult result;
        result.metric_name = "Network Connectivity";

        // Ping a reliable host (Google DNS)
        std::string output = exec_command("ping -c 1 -W 3 8.8.8.8 2>/dev/null");

        if (output.find("1 packets received") != std::string::npos ||
            output.find("1 received") != std::string::npos) {
            // Extract latency
            size_t time_pos = output.find("time=");
            std::string latency = "unknown";
            if (time_pos != std::string::npos) {
                size_t end = output.find_first_of(" \n", time_pos);
                latency = output.substr(time_pos + 5, end - time_pos - 5);
            }
            result.status = HealthStatus::OK;
            result.value = "Connected";
            result.detail = "Ping to 8.8.8.8 successful, latency: " + latency;
        } else {
            result.status = HealthStatus::CRITICAL;
            result.value = "No connection";
            result.detail = "Unable to reach 8.8.8.8 - network may be down";
        }

        results_.push_back(result);
        return result;
    }

    // ---------------------------------------------------------------
    // Process Count and Top CPU Consumers
    // ---------------------------------------------------------------
    DiagnosticResult check_processes() {
        DiagnosticResult result;
        result.metric_name = "Processes";

        // Get process count
        std::string count_str = exec_command("ps aux 2>/dev/null | wc -l");
        int proc_count = 0;
        if (!count_str.empty()) {
            proc_count = std::stoi(count_str) - 1; // Subtract header line
        }

        // Get top CPU consumers
        std::string top_str = exec_command("ps aux --sort=-%cpu 2>/dev/null | head -6 | tail -5");
        if (top_str.empty()) {
            // macOS variant
            top_str = exec_command("ps aux -r 2>/dev/null | head -6 | tail -5");
        }

        std::vector<std::string> top_procs;
        std::istringstream stream(top_str);
        std::string line;
        while (std::getline(stream, line) && top_procs.size() < 5) {
            if (!line.empty()) {
                // Extract process name and CPU usage
                std::istringstream ls(line);
                std::string user, pid, cpu;
                ls >> user >> pid >> cpu;
                // Get command name (last field)
                std::string rest;
                std::getline(ls, rest);
                size_t last_slash = rest.rfind('/');
                std::string proc_name = (last_slash != std::string::npos) ? rest.substr(last_slash + 1) : rest;
                if (proc_name.size() > 30) proc_name = proc_name.substr(0, 30);
                top_procs.push_back(cpu + "% " + proc_name);
            }
        }

        if (proc_count < 300) {
            result.status = HealthStatus::OK;
        } else if (proc_count < 500) {
            result.status = HealthStatus::WARNING;
        } else {
            result.status = HealthStatus::CRITICAL;
        }

        result.value = std::to_string(proc_count) + " processes";
        std::string detail = "Top consumers: ";
        for (const auto& p : top_procs) {
            detail += p + "; ";
        }
        result.detail = detail;

        results_.push_back(result);
        return result;
    }

    // ---------------------------------------------------------------
    // Uptime and Load Average
    // ---------------------------------------------------------------
    DiagnosticResult check_uptime() {
        DiagnosticResult result;
        result.metric_name = "Uptime & Load";

        std::string uptime_str = exec_command("uptime 2>/dev/null");
        std::string load_str;

        // Extract load average
        size_t load_pos = uptime_str.find("load average");
        if (load_pos == std::string::npos) {
            load_pos = uptime_str.find("load averages");
        }
        if (load_pos != std::string::npos) {
            size_t colon = uptime_str.find(':', load_pos);
            if (colon != std::string::npos) {
                load_str = uptime_str.substr(colon + 2);
            }
        }

        // Parse first load value for status determination
        float load_1min = 0.0f;
        if (!load_str.empty()) {
            try {
                load_1min = std::stof(load_str);
            } catch (...) {
                load_1min = 0.0f;
            }
        }

        // Get number of CPU cores for context
        std::string cores_str = exec_command("nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null");
        int cores = cores_str.empty() ? 4 : std::stoi(cores_str);

        float load_per_core = load_1min / (float)cores;

        if (load_per_core < 0.7f) {
            result.status = HealthStatus::OK;
        } else if (load_per_core < 1.5f) {
            result.status = HealthStatus::WARNING;
        } else {
            result.status = HealthStatus::CRITICAL;
        }

        result.value = "Load: " + load_str;
        result.detail = uptime_str + " (" + std::to_string(cores) + " cores)";

        results_.push_back(result);
        return result;
    }

    // ---------------------------------------------------------------
    // Battery Status
    // ---------------------------------------------------------------
    DiagnosticResult check_battery() {
        DiagnosticResult result;
        result.metric_name = "Battery";

#ifdef __APPLE__
        std::string output = exec_command("pmset -g batt 2>/dev/null");
        if (output.find("InternalBattery") != std::string::npos) {
            // Extract percentage
            size_t pct_pos = output.find('%');
            if (pct_pos != std::string::npos) {
                size_t start = output.rfind('\t', pct_pos);
                if (start == std::string::npos) start = output.rfind(' ', pct_pos);
                std::string pct_str = output.substr(start + 1, pct_pos - start - 1);
                int battery_pct = std::stoi(pct_str);

                bool charging = (output.find("charging") != std::string::npos ||
                               output.find("AC Power") != std::string::npos);

                if (battery_pct > 20 || charging) {
                    result.status = HealthStatus::OK;
                } else if (battery_pct > 10) {
                    result.status = HealthStatus::WARNING;
                } else {
                    result.status = HealthStatus::CRITICAL;
                }

                result.value = std::to_string(battery_pct) + "%";
                result.detail = charging ? "Charging" : "On battery power";
            } else {
                result.status = HealthStatus::UNKNOWN;
                result.value = "N/A";
                result.detail = "Could not parse battery level";
            }
        } else {
            result.status = HealthStatus::OK;
            result.value = "No battery";
            result.detail = "Desktop system (AC power)";
        }
#else
        // Linux: check /sys/class/power_supply
        std::ifstream capacity("/sys/class/power_supply/BAT0/capacity");
        std::ifstream status_file("/sys/class/power_supply/BAT0/status");
        if (capacity.is_open()) {
            int battery_pct;
            capacity >> battery_pct;
            std::string charge_status;
            if (status_file.is_open()) {
                status_file >> charge_status;
            }

            bool charging = (charge_status == "Charging" || charge_status == "Full");

            if (battery_pct > 20 || charging) {
                result.status = HealthStatus::OK;
            } else if (battery_pct > 10) {
                result.status = HealthStatus::WARNING;
            } else {
                result.status = HealthStatus::CRITICAL;
            }

            result.value = std::to_string(battery_pct) + "%";
            result.detail = charge_status;
        } else {
            result.status = HealthStatus::OK;
            result.value = "No battery";
            result.detail = "Desktop system or battery not detected";
        }
#endif

        results_.push_back(result);
        return result;
    }

    // ---------------------------------------------------------------
    // Run All Checks and Generate Report
    // ---------------------------------------------------------------
    std::string run_full_diagnostics() {
        std::cout << "[DIAGNOSTICS] Running full system health check...\n";
        results_.clear();
        last_report_time_ = get_timestamp();

        check_cpu_temperature();
        check_disk_space();
        check_memory();
        check_network();
        check_processes();
        check_uptime();
        check_battery();

        return generate_report();
    }

    std::string generate_report() {
        std::ostringstream report;
        report << "========================================\n";
        report << " MK SYSTEM HEALTH REPORT\n";
        report << " Time: " << last_report_time_ << "\n";
        report << "========================================\n\n";

        int ok_count = 0, warn_count = 0, crit_count = 0;

        for (const auto& r : results_) {
            std::string icon;
            switch (r.status) {
                case HealthStatus::OK:       icon = "[OK]      "; ok_count++; break;
                case HealthStatus::WARNING:  icon = "[WARNING] "; warn_count++; break;
                case HealthStatus::CRITICAL: icon = "[CRITICAL]"; crit_count++; break;
                default:                     icon = "[UNKNOWN] "; break;
            }

            report << icon << " " << r.metric_name << "\n";
            report << "           Value: " << r.value << "\n";
            report << "           Detail: " << r.detail << "\n\n";
        }

        report << "----------------------------------------\n";
        report << " Summary: " << ok_count << " OK, " << warn_count << " WARNING, "
               << crit_count << " CRITICAL\n";

        HealthStatus overall;
        if (crit_count > 0) overall = HealthStatus::CRITICAL;
        else if (warn_count > 0) overall = HealthStatus::WARNING;
        else overall = HealthStatus::OK;

        report << " Overall: " << status_to_string(overall) << "\n";
        report << "========================================\n";

        std::cout << "[DIAGNOSTICS] Health check complete: " << status_to_string(overall) << "\n";
        return report.str();
    }

    // Get the last results for programmatic access
    const std::vector<DiagnosticResult>& get_results() const {
        return results_;
    }

    // Quick status check (returns overall health without full report)
    HealthStatus quick_check() {
        results_.clear();
        check_cpu_temperature();
        check_memory();
        check_disk_space();

        for (const auto& r : results_) {
            if (r.status == HealthStatus::CRITICAL) return HealthStatus::CRITICAL;
        }
        for (const auto& r : results_) {
            if (r.status == HealthStatus::WARNING) return HealthStatus::WARNING;
        }
        return HealthStatus::OK;
    }
};

#endif // MK_DIAGNOSTICS_CPP
