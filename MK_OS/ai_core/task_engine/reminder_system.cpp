#ifndef MK_REMINDER_SYSTEM_CPP
#define MK_REMINDER_SYSTEM_CPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <unordered_map>

// ===========================================================================
// MK REMINDER SYSTEM - Scheduling and Reminder Management
// ===========================================================================
// Set reminders by time, recurring reminders, task deadlines.
// Persists to disk. Includes natural language time parsing.
// Parses: '5 minutes from now', 'next tuesday', 'every weekday at 9am'
// ===========================================================================

// Reminder frequency
enum class MKReminderFrequency {
    ONCE,
    DAILY,
    WEEKLY,
    WEEKDAYS,
    MONTHLY,
    CUSTOM
};

// Reminder state
enum class MKReminderState {
    ACTIVE,
    TRIGGERED,
    SNOOZED,
    COMPLETED,
    CANCELLED
};

// A single reminder
struct MKReminder {
    int id;
    std::string message;
    time_t trigger_time;
    MKReminderFrequency frequency;
    MKReminderState state;
    std::string created_at;
    std::string category;     // work, personal, health, etc.
    int snooze_count;
    time_t last_triggered;
};

// Parsed time expression
struct MKParsedTime {
    bool valid;
    time_t absolute_time;
    int relative_seconds;    // seconds from now (if relative)
    bool is_relative;
    MKReminderFrequency frequency;
    std::string original_text;
    std::string error;
};

class MKReminderSystem {
private:
    std::vector<MKReminder> reminders;
    std::string storage_path;
    int next_id;

    // Get current time as time_t
    time_t now() { return std::time(nullptr); }

    // Format time_t to string
    std::string formatTime(time_t t) {
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
        return std::string(buf);
    }

    // Parse natural language time expression
    MKParsedTime parseTimeExpression(const std::string& expr) {
        MKParsedTime parsed;
        parsed.valid = false;
        parsed.is_relative = false;
        parsed.frequency = MKReminderFrequency::ONCE;
        parsed.original_text = expr;

        std::string lower = expr;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Check for recurring patterns first
        if (lower.find("every day") != std::string::npos || lower.find("daily") != std::string::npos) {
            parsed.frequency = MKReminderFrequency::DAILY;
        } else if (lower.find("every week") != std::string::npos || lower.find("weekly") != std::string::npos) {
            parsed.frequency = MKReminderFrequency::WEEKLY;
        } else if (lower.find("weekday") != std::string::npos) {
            parsed.frequency = MKReminderFrequency::WEEKDAYS;
        } else if (lower.find("every month") != std::string::npos || lower.find("monthly") != std::string::npos) {
            parsed.frequency = MKReminderFrequency::MONTHLY;
        }

        // Relative time: 'in X minutes/hours/days'
        if (lower.find("in ") != std::string::npos || lower.find("from now") != std::string::npos) {
            parsed.is_relative = true;
            int value = 0;
            // Extract number
            std::stringstream ss(lower);
            std::string word;
            while (ss >> word) {
                try {
                    value = std::stoi(word);
                    break;
                } catch (...) { continue; }
            }
            if (value == 0) value = 1;  // default to 1

            int multiplier = 60;  // default: minutes
            if (lower.find("second") != std::string::npos) multiplier = 1;
            else if (lower.find("minute") != std::string::npos) multiplier = 60;
            else if (lower.find("hour") != std::string::npos) multiplier = 3600;
            else if (lower.find("day") != std::string::npos) multiplier = 86400;
            else if (lower.find("week") != std::string::npos) multiplier = 604800;

            parsed.relative_seconds = value * multiplier;
            parsed.absolute_time = now() + parsed.relative_seconds;
            parsed.valid = true;
        }
        // Absolute time: 'at 3pm', 'at 15:00'
        else if (lower.find("at ") != std::string::npos) {
            size_t at_pos = lower.find("at ") + 3;
            std::string time_str = lower.substr(at_pos);
            int hour = 0, minute = 0;

            // Parse 'Xpm' or 'Xam'
            if (time_str.find("pm") != std::string::npos || time_str.find("am") != std::string::npos) {
                try { hour = std::stoi(time_str); } catch (...) {}
                if (time_str.find("pm") != std::string::npos && hour != 12) hour += 12;
                if (time_str.find("am") != std::string::npos && hour == 12) hour = 0;
            }
            // Parse 'HH:MM'
            else if (time_str.find(':') != std::string::npos) {
                sscanf(time_str.c_str(), "%d:%d", &hour, &minute);
            }
            else {
                try { hour = std::stoi(time_str); } catch (...) {}
            }

            time_t t = now();
            struct tm* tm_info = localtime(&t);
            tm_info->tm_hour = hour;
            tm_info->tm_min = minute;
            tm_info->tm_sec = 0;
            parsed.absolute_time = mktime(tm_info);
            // If time already passed today, schedule for tomorrow
            if (parsed.absolute_time <= now()) {
                parsed.absolute_time += 86400;
            }
            parsed.valid = true;
        }
        // Day-based: 'tomorrow', 'next tuesday'
        else if (lower.find("tomorrow") != std::string::npos) {
            parsed.absolute_time = now() + 86400;
            parsed.valid = true;
        }
        else if (lower.find("next ") != std::string::npos) {
            // Find day of week
            std::vector<std::string> days = {"sunday", "monday", "tuesday", "wednesday",
                                              "thursday", "friday", "saturday"};
            time_t t = now();
            struct tm* tm_info = localtime(&t);
            int today = tm_info->tm_wday;

            for (int i = 0; i < 7; i++) {
                if (lower.find(days[i]) != std::string::npos) {
                    int days_ahead = (i - today + 7) % 7;
                    if (days_ahead == 0) days_ahead = 7;  // 'next' means at least 1 week
                    parsed.absolute_time = now() + days_ahead * 86400;
                    parsed.valid = true;
                    break;
                }
            }
        }

        if (!parsed.valid) {
            parsed.error = "Could not parse time expression: " + expr;
        }
        return parsed;
    }

    // Save reminders to disk
    void saveReminders() {
        std::ofstream out(storage_path);
        if (!out.is_open()) return;
        for (const auto& r : reminders) {
            out << r.id << "|" << r.message << "|" << r.trigger_time << "|"
                << static_cast<int>(r.frequency) << "|" << static_cast<int>(r.state)
                << "|" << r.created_at << "|" << r.category << "\n";
        }
        out.close();
    }

    // Load reminders from disk
    void loadReminders() {
        std::ifstream in(storage_path);
        if (!in.is_open()) return;
        std::string line;
        while (std::getline(in, line)) {
            std::stringstream ss(line);
            MKReminder r;
            std::string id_str, time_str, freq_str, state_str;
            std::getline(ss, id_str, '|');
            std::getline(ss, r.message, '|');
            std::getline(ss, time_str, '|');
            std::getline(ss, freq_str, '|');
            std::getline(ss, state_str, '|');
            std::getline(ss, r.created_at, '|');
            std::getline(ss, r.category, '|');
            try {
                r.id = std::stoi(id_str);
                r.trigger_time = std::stol(time_str);
                r.frequency = static_cast<MKReminderFrequency>(std::stoi(freq_str));
                r.state = static_cast<MKReminderState>(std::stoi(state_str));
            } catch (...) { continue; }
            r.snooze_count = 0;
            r.last_triggered = 0;
            reminders.push_back(r);
            if (r.id >= next_id) next_id = r.id + 1;
        }
    }

public:
    MKReminderSystem() : next_id(1) {
        storage_path = "mk_reminders.dat";
    }

    void init(const std::string& path = "mk_reminders.dat") {
        storage_path = path;
        loadReminders();
    }

    // Set a new reminder with natural language time
    int setReminder(const std::string& message, const std::string& time_expr,
                    const std::string& category = "general") {
        MKParsedTime parsed = parseTimeExpression(time_expr);
        if (!parsed.valid) {
            std::cerr << "[Reminder] " << parsed.error << std::endl;
            return -1;
        }

        MKReminder r;
        r.id = next_id++;
        r.message = message;
        r.trigger_time = parsed.absolute_time;
        r.frequency = parsed.frequency;
        r.state = MKReminderState::ACTIVE;
        r.created_at = formatTime(now());
        r.category = category;
        r.snooze_count = 0;
        r.last_triggered = 0;

        reminders.push_back(r);
        saveReminders();
        return r.id;
    }

    // Check for triggered reminders
    std::vector<MKReminder> checkReminders() {
        std::vector<MKReminder> triggered;
        time_t current = now();

        for (auto& r : reminders) {
            if (r.state != MKReminderState::ACTIVE) continue;
            if (current >= r.trigger_time) {
                r.state = MKReminderState::TRIGGERED;
                r.last_triggered = current;
                triggered.push_back(r);

                // Handle recurring reminders
                if (r.frequency != MKReminderFrequency::ONCE) {
                    int interval = 86400;  // default daily
                    if (r.frequency == MKReminderFrequency::WEEKLY) interval = 604800;
                    else if (r.frequency == MKReminderFrequency::MONTHLY) interval = 2592000;
                    r.trigger_time += interval;
                    r.state = MKReminderState::ACTIVE;
                }
            }
        }
        if (!triggered.empty()) saveReminders();
        return triggered;
    }

    // Cancel a reminder
    bool cancelReminder(int id) {
        for (auto& r : reminders) {
            if (r.id == id) {
                r.state = MKReminderState::CANCELLED;
                saveReminders();
                return true;
            }
        }
        return false;
    }

    // Snooze a reminder (add 10 minutes)
    bool snoozeReminder(int id, int minutes = 10) {
        for (auto& r : reminders) {
            if (r.id == id) {
                r.trigger_time = now() + minutes * 60;
                r.state = MKReminderState::ACTIVE;
                r.snooze_count++;
                saveReminders();
                return true;
            }
        }
        return false;
    }

    // List active reminders
    std::vector<MKReminder> listActive() {
        std::vector<MKReminder> active;
        for (const auto& r : reminders) {
            if (r.state == MKReminderState::ACTIVE) active.push_back(r);
        }
        return active;
    }

    int getTotalReminders() const { return static_cast<int>(reminders.size()); }

    void printStats() const {
        int active = 0;
        for (const auto& r : reminders)
            if (r.state == MKReminderState::ACTIVE) active++;
        std::cout << "[MKReminderSystem] Total: " << reminders.size()
                  << ", Active: " << active << std::endl;
    }
};

#endif // MK_REMINDER_SYSTEM_CPP
