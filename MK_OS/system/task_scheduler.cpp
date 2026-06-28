#ifndef MK_TASK_SCHEDULER_CPP
#define MK_TASK_SCHEDULER_CPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iostream>
#include <algorithm>

// ============================================================
// MK Task Scheduler
// Stores scheduled tasks/reminders with optional recurrence.
// Supports: one-time, daily, hourly reminders
// Persists to mk_schedules.dat
// ============================================================

enum class MKRecurrence { ONCE, DAILY, HOURLY };

struct MKScheduledTask {
    std::time_t triggerTime;
    std::string message;
    MKRecurrence recurrence;
    std::string chatId;      // For Telegram delivery
    bool fired;              // Has this been triggered
    int id;
};

class MKTaskScheduler {
private:
    std::vector<MKScheduledTask> tasks;
    std::string persistPath;
    int nextId;

    std::string recurrenceToStr(MKRecurrence r) const {
        switch (r) {
            case MKRecurrence::ONCE: return "once";
            case MKRecurrence::DAILY: return "daily";
            case MKRecurrence::HOURLY: return "hourly";
        }
        return "once";
    }

    MKRecurrence strToRecurrence(const std::string& s) const {
        if (s == "daily") return MKRecurrence::DAILY;
        if (s == "hourly") return MKRecurrence::HOURLY;
        return MKRecurrence::ONCE;
    }


public:
    MKTaskScheduler(const std::string& path = "mk_schedules.dat")
        : persistPath(path), nextId(1) {
        loadFromDisk();
    }

    // Add a reminder at a specific time offset (seconds from now)
    int addReminder(int secondsFromNow, const std::string& message,
                    MKRecurrence recurrence = MKRecurrence::ONCE,
                    const std::string& chatId = "") {
        MKScheduledTask task;
        task.triggerTime = std::time(nullptr) + secondsFromNow;
        task.message = message;
        task.recurrence = recurrence;
        task.chatId = chatId;
        task.fired = false;
        task.id = nextId++;
        tasks.push_back(task);
        saveToDisk();
        return task.id;
    }

    // Add a reminder at a specific absolute time
    int addReminderAt(std::time_t when, const std::string& message,
                      MKRecurrence recurrence = MKRecurrence::ONCE,
                      const std::string& chatId = "") {
        MKScheduledTask task;
        task.triggerTime = when;
        task.message = message;
        task.recurrence = recurrence;
        task.chatId = chatId;
        task.fired = false;
        task.id = nextId++;
        tasks.push_back(task);
        saveToDisk();
        return task.id;
    }

    // Check and return all due tasks
    std::vector<MKScheduledTask> checkDueTasks() {
        std::vector<MKScheduledTask> due;
        std::time_t now = std::time(nullptr);

        for (auto& task : tasks) {
            if (!task.fired && task.triggerTime <= now) {
                due.push_back(task);
                task.fired = true;

                // Reschedule recurring tasks
                if (task.recurrence == MKRecurrence::DAILY) {
                    task.triggerTime += 86400; // 24 hours
                    task.fired = false;
                } else if (task.recurrence == MKRecurrence::HOURLY) {
                    task.triggerTime += 3600; // 1 hour
                    task.fired = false;
                }
            }
        }

        // Clean up fired one-time tasks
        tasks.erase(
            std::remove_if(tasks.begin(), tasks.end(),
                [](const MKScheduledTask& t) {
                    return t.fired && t.recurrence == MKRecurrence::ONCE;
                }),
            tasks.end());

        if (!due.empty()) saveToDisk();
        return due;
    }

    // Get all pending tasks
    std::vector<MKScheduledTask> getDueTasks() const {
        std::vector<MKScheduledTask> pending;
        for (const auto& task : tasks) {
            if (!task.fired) pending.push_back(task);
        }
        return pending;
    }

    // Remove a task by ID
    bool removeTask(int taskId) {
        auto it = std::remove_if(tasks.begin(), tasks.end(),
            [taskId](const MKScheduledTask& t) { return t.id == taskId; });
        if (it != tasks.end()) {
            tasks.erase(it, tasks.end());
            saveToDisk();
            return true;
        }
        return false;
    }

    // Parse time string like "5m", "2h", "30s", "1d"
    static int parseTimeOffset(const std::string& timeStr) {
        if (timeStr.empty()) return 0;
        int value = 0;
        try { value = std::stoi(timeStr); } catch (...) { return 0; }
        char unit = timeStr.back();
        switch (unit) {
            case 's': return value;
            case 'm': return value * 60;
            case 'h': return value * 3600;
            case 'd': return value * 86400;
            default: return value * 60; // Default to minutes
        }
    }

    int taskCount() const { return (int)tasks.size(); }

    void saveToDisk() {
        std::ofstream file(persistPath);
        if (!file.is_open()) return;
        file << "# MK Schedules\n";
        for (const auto& task : tasks) {
            file << task.id << "|" << task.triggerTime << "|"
                 << task.message << "|" << recurrenceToStr(task.recurrence) << "|"
                 << task.chatId << "|" << (task.fired ? "1" : "0") << "\n";
        }
        file.close();
    }

    void loadFromDisk() {
        std::ifstream file(persistPath);
        if (!file.is_open()) return;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::stringstream ss(line);
            std::string idStr, timeStr, msg, recStr, chatId, firedStr;
            if (std::getline(ss, idStr, '|') && std::getline(ss, timeStr, '|') &&
                std::getline(ss, msg, '|') && std::getline(ss, recStr, '|') &&
                std::getline(ss, chatId, '|') && std::getline(ss, firedStr, '|')) {
                MKScheduledTask task;
                try {
                    task.id = std::stoi(idStr);
                    task.triggerTime = (std::time_t)std::stol(timeStr);
                } catch (...) { continue; }
                task.message = msg;
                task.recurrence = strToRecurrence(recStr);
                task.chatId = chatId;
                task.fired = (firedStr == "1");
                tasks.push_back(task);
                if (task.id >= nextId) nextId = task.id + 1;
            }
        }
        file.close();
    }
};

#endif // MK_TASK_SCHEDULER_CPP
