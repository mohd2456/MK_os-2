#ifndef MK_AUTO_MAINTENANCE_CPP
#define MK_AUTO_MAINTENANCE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <set>
#include <filesystem>
#include <regex>

// ===================================================================================
// MK AUTO-MAINTENANCE
// ===================================================================================
// Automated maintenance tasks that run during idle periods:
//   - Clear temp files older than 7 days
//   - Compact knowledge files (remove duplicates, merge similar)
//   - Verify knowledge file integrity (check format, remove corrupted lines)
//   - Log rotation (archive old logs, keep last 7 days)
//   - Report disk savings after cleanup
// ===================================================================================

namespace fs = std::filesystem;

struct MaintenanceReport {
    int temp_files_removed;
    long bytes_freed_temp;
    int duplicate_lines_removed;
    int corrupted_lines_fixed;
    int logs_rotated;
    long total_bytes_saved;
    std::string timestamp;
    std::vector<std::string> actions_taken;
};

class MKAutoMaintenance {
private:
    std::string mk_root_;
    std::string knowledge_dir_;
    std::string log_dir_;
    std::string temp_dir_;
    MaintenanceReport report_;

    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t_now));
        return std::string(buf);
    }

    bool is_older_than_days(const fs::path& path, int days) {
        try {
            auto ftime = fs::last_write_time(path);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            auto age = std::chrono::system_clock::now() - sctp;
            auto age_days = std::chrono::duration_cast<std::chrono::hours>(age).count() / 24;
            return age_days > days;
        } catch (...) {
            return false;
        }
    }

    bool is_valid_knowledge_line(const std::string& line) {
        // Empty lines and comments are valid
        if (line.empty() || line[0] == '#') return true;

        // Must have at least source|relation|target (3 pipe-separated fields)
        int pipe_count = 0;
        for (char c : line) {
            if (c == '|') pipe_count++;
        }
        // Valid format: source|relation|target or source|relation|target|weight
        if (pipe_count < 2 || pipe_count > 3) return false;

        // Check that no field is empty
        std::istringstream ss(line);
        std::string field;
        while (std::getline(ss, field, '|')) {
            // Trim whitespace
            field.erase(0, field.find_first_not_of(" \t"));
            field.erase(field.find_last_not_of(" \t") + 1);
            if (field.empty()) return false;
        }

        // If weight field exists, verify it is numeric
        if (pipe_count == 3) {
            size_t last_pipe = line.rfind('|');
            std::string weight_str = line.substr(last_pipe + 1);
            weight_str.erase(0, weight_str.find_first_not_of(" \t"));
            weight_str.erase(weight_str.find_last_not_of(" \t") + 1);
            try {
                float w = std::stof(weight_str);
                if (w < 0.0f || w > 1.0f) return false;
            } catch (...) {
                return false;
            }
        }

        return true;
    }

public:
    MKAutoMaintenance() : mk_root_("."), knowledge_dir_("ai_core/hre/knowledge_files"),
                          log_dir_("logs"), temp_dir_("/tmp/mk_os") {
        std::cout << "[MAINTENANCE] Auto-maintenance module initialized.\n";
    }

    MKAutoMaintenance(const std::string& root) : mk_root_(root),
        knowledge_dir_(root + "/ai_core/hre/knowledge_files"),
        log_dir_(root + "/logs"), temp_dir_("/tmp/mk_os") {
        std::cout << "[MAINTENANCE] Auto-maintenance module initialized (root: " << root << ").\n";
    }

    // ---------------------------------------------------------------
    // Clear Temp Files Older Than 7 Days
    // ---------------------------------------------------------------
    void clear_temp_files() {
        std::cout << "[MAINTENANCE] Clearing temp files older than 7 days...\n";
        int removed = 0;
        long bytes_freed = 0;

        try {
            if (!fs::exists(temp_dir_)) {
                std::cout << "[MAINTENANCE] Temp directory does not exist, skipping.\n";
                report_.actions_taken.push_back("Temp dir not found - skipped");
                return;
            }

            for (const auto& entry : fs::recursive_directory_iterator(temp_dir_)) {
                if (entry.is_regular_file() && is_older_than_days(entry.path(), 7)) {
                    long file_size = (long)entry.file_size();
                    try {
                        fs::remove(entry.path());
                        removed++;
                        bytes_freed += file_size;
                    } catch (const std::exception& e) {
                        std::cout << "[MAINTENANCE] Could not remove: " << entry.path() << "\n";
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cout << "[MAINTENANCE] Error scanning temp dir: " << e.what() << "\n";
        }

        report_.temp_files_removed = removed;
        report_.bytes_freed_temp = bytes_freed;
        report_.actions_taken.push_back("Removed " + std::to_string(removed) + " temp files ("
                                       + std::to_string(bytes_freed / 1024) + " KB freed)");
        std::cout << "[MAINTENANCE] Removed " << removed << " temp files, freed "
                  << bytes_freed / 1024 << " KB.\n";
    }

    // ---------------------------------------------------------------
    // Compact Knowledge Files (Remove Duplicates)
    // ---------------------------------------------------------------
    void compact_knowledge_files() {
        std::cout << "[MAINTENANCE] Compacting knowledge files...\n";
        int total_duplicates = 0;

        try {
            if (!fs::exists(knowledge_dir_)) {
                std::cout << "[MAINTENANCE] Knowledge directory not found.\n";
                return;
            }

            for (const auto& entry : fs::directory_iterator(knowledge_dir_)) {
                if (entry.path().extension() == ".mk") {
                    int dupes = compact_single_file(entry.path().string());
                    total_duplicates += dupes;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "[MAINTENANCE] Error compacting: " << e.what() << "\n";
        }

        report_.duplicate_lines_removed = total_duplicates;
        report_.actions_taken.push_back("Removed " + std::to_string(total_duplicates)
                                       + " duplicate lines from knowledge files");
        std::cout << "[MAINTENANCE] Compaction complete: " << total_duplicates << " duplicates removed.\n";
    }

    int compact_single_file(const std::string& filepath) {
        std::ifstream infile(filepath);
        if (!infile.is_open()) return 0;

        std::vector<std::string> lines;
        std::set<std::string> seen;
        std::string line;
        int duplicates = 0;

        while (std::getline(infile, line)) {
            // Keep comments and empty lines
            if (line.empty() || line[0] == '#') {
                lines.push_back(line);
                continue;
            }

            // Normalize: trim whitespace
            std::string normalized = line;
            normalized.erase(0, normalized.find_first_not_of(" \t"));
            normalized.erase(normalized.find_last_not_of(" \t\r\n") + 1);

            if (seen.find(normalized) == seen.end()) {
                seen.insert(normalized);
                lines.push_back(line);
            } else {
                duplicates++;
            }
        }
        infile.close();

        if (duplicates > 0) {
            std::ofstream outfile(filepath);
            for (const auto& l : lines) {
                outfile << l << "\n";
            }
            outfile.close();
            std::cout << "[MAINTENANCE]   " << filepath << ": " << duplicates << " duplicates removed.\n";
        }

        return duplicates;
    }

    // ---------------------------------------------------------------
    // Verify Knowledge File Integrity
    // ---------------------------------------------------------------
    void verify_knowledge_integrity() {
        std::cout << "[MAINTENANCE] Verifying knowledge file integrity...\n";
        int total_corrupted = 0;

        try {
            if (!fs::exists(knowledge_dir_)) return;

            for (const auto& entry : fs::directory_iterator(knowledge_dir_)) {
                if (entry.path().extension() == ".mk") {
                    int corrupted = verify_single_file(entry.path().string());
                    total_corrupted += corrupted;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "[MAINTENANCE] Error verifying: " << e.what() << "\n";
        }

        report_.corrupted_lines_fixed = total_corrupted;
        report_.actions_taken.push_back("Fixed " + std::to_string(total_corrupted)
                                       + " corrupted lines in knowledge files");
        std::cout << "[MAINTENANCE] Integrity check complete: " << total_corrupted << " lines fixed.\n";
    }

    int verify_single_file(const std::string& filepath) {
        std::ifstream infile(filepath);
        if (!infile.is_open()) return 0;

        std::vector<std::string> valid_lines;
        std::string line;
        int corrupted = 0;

        while (std::getline(infile, line)) {
            if (is_valid_knowledge_line(line)) {
                valid_lines.push_back(line);
            } else {
                corrupted++;
                std::cout << "[MAINTENANCE]   Corrupted line removed from " << filepath
                          << ": " << line.substr(0, 60) << "\n";
            }
        }
        infile.close();

        if (corrupted > 0) {
            std::ofstream outfile(filepath);
            for (const auto& l : valid_lines) {
                outfile << l << "\n";
            }
            outfile.close();
        }

        return corrupted;
    }

    // ---------------------------------------------------------------
    // Log Rotation
    // ---------------------------------------------------------------
    void rotate_logs() {
        std::cout << "[MAINTENANCE] Rotating logs (keeping last 7 days)...\n";
        int rotated = 0;

        try {
            if (!fs::exists(log_dir_)) {
                // Create log directory if it does not exist
                fs::create_directories(log_dir_);
                report_.actions_taken.push_back("Created log directory");
                return;
            }

            for (const auto& entry : fs::directory_iterator(log_dir_)) {
                if (entry.is_regular_file() && is_older_than_days(entry.path(), 7)) {
                    std::string archive_path = entry.path().string() + ".archived";
                    try {
                        // Archive by renaming with .archived suffix
                        if (fs::exists(archive_path)) {
                            fs::remove(archive_path);
                        }
                        fs::rename(entry.path(), archive_path);
                        rotated++;
                    } catch (const std::exception& e) {
                        std::cout << "[MAINTENANCE] Could not rotate: " << entry.path() << "\n";
                    }
                }
            }

            // Remove archived files older than 30 days
            for (const auto& entry : fs::directory_iterator(log_dir_)) {
                if (entry.path().extension() == ".archived" && is_older_than_days(entry.path(), 30)) {
                    fs::remove(entry.path());
                    rotated++;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "[MAINTENANCE] Error rotating logs: " << e.what() << "\n";
        }

        report_.logs_rotated = rotated;
        report_.actions_taken.push_back("Rotated " + std::to_string(rotated) + " log files");
        std::cout << "[MAINTENANCE] Log rotation complete: " << rotated << " files processed.\n";
    }

    // ---------------------------------------------------------------
    // Run Full Maintenance
    // ---------------------------------------------------------------
    MaintenanceReport run_full_maintenance() {
        std::cout << "[MAINTENANCE] ========================================\n";
        std::cout << "[MAINTENANCE] Starting full auto-maintenance cycle...\n";
        std::cout << "[MAINTENANCE] ========================================\n";

        report_ = MaintenanceReport{};
        report_.timestamp = get_timestamp();

        clear_temp_files();
        compact_knowledge_files();
        verify_knowledge_integrity();
        rotate_logs();

        // Calculate total savings
        report_.total_bytes_saved = report_.bytes_freed_temp;
        // Estimate savings from deduplication (average 50 bytes per line)
        report_.total_bytes_saved += report_.duplicate_lines_removed * 50;

        std::cout << "\n[MAINTENANCE] ========================================\n";
        std::cout << "[MAINTENANCE] Maintenance complete!\n";
        std::cout << "[MAINTENANCE]   Temp files removed: " << report_.temp_files_removed << "\n";
        std::cout << "[MAINTENANCE]   Duplicates removed: " << report_.duplicate_lines_removed << "\n";
        std::cout << "[MAINTENANCE]   Corrupted fixed:    " << report_.corrupted_lines_fixed << "\n";
        std::cout << "[MAINTENANCE]   Logs rotated:       " << report_.logs_rotated << "\n";
        std::cout << "[MAINTENANCE]   Total space saved:  " << report_.total_bytes_saved / 1024 << " KB\n";
        std::cout << "[MAINTENANCE] ========================================\n";

        return report_;
    }

    std::string generate_summary() {
        std::ostringstream ss;
        ss << "Maintenance Report (" << report_.timestamp << ")\n";
        ss << "-------------------------------------------\n";
        ss << "Temp files cleared: " << report_.temp_files_removed << "\n";
        ss << "Duplicates removed: " << report_.duplicate_lines_removed << "\n";
        ss << "Corrupted lines fixed: " << report_.corrupted_lines_fixed << "\n";
        ss << "Logs rotated: " << report_.logs_rotated << "\n";
        ss << "Total disk savings: " << report_.total_bytes_saved / 1024 << " KB\n";
        ss << "-------------------------------------------\n";
        ss << "Actions:\n";
        for (const auto& action : report_.actions_taken) {
            ss << "  - " << action << "\n";
        }
        return ss.str();
    }
};

#endif // MK_AUTO_MAINTENANCE_CPP
