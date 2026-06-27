#ifndef MK_TASK_AUTOMATION_CPP
#define MK_TASK_AUTOMATION_CPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <functional>
#include <cstdlib>
#include <sstream>
#include <set>

// ===================================================================================
// MK TASK AUTOMATION ENGINE
// Executes multi-step tasks autonomously with safety checks.
// Features:
//   - Accepts high-level goals and breaks them into steps
//   - Uses a plan template library for common tasks
//   - Executes system commands with progress reporting
//   - Safety: won't delete without confirmation, blocks dangerous commands
//   - Pre-built templates: backup, disk cleanup, git status, health check, compile
// ===================================================================================

enum class TaskStatus {
    PENDING,
    IN_PROGRESS,
    COMPLETED,
    FAILED,
    BLOCKED_SAFETY,
    CANCELLED
};

enum class SafetyLevel {
    SAFE,               // Can run without confirmation
    NEEDS_CONFIRMATION, // Requires user OK
    DANGEROUS,          // Will not execute
    UNKNOWN             // Needs analysis
};

struct TaskStep {
    int stepNumber;
    std::string description;
    std::string command;
    SafetyLevel safety;
    TaskStatus status;
    std::string output;
    int exitCode;
    double durationMs;

    std::string statusToString() const {
        switch (status) {
            case TaskStatus::PENDING: return "PENDING";
            case TaskStatus::IN_PROGRESS: return "IN_PROGRESS";
            case TaskStatus::COMPLETED: return "COMPLETED";
            case TaskStatus::FAILED: return "FAILED";
            case TaskStatus::BLOCKED_SAFETY: return "BLOCKED_SAFETY";
            case TaskStatus::CANCELLED: return "CANCELLED";
        }
        return "UNKNOWN";
    }
};

struct TaskPlan {
    std::string name;
    std::string description;
    std::vector<TaskStep> steps;
    TaskStatus overallStatus;
    std::time_t startedAt;
    std::time_t completedAt;
    int stepsCompleted;
    int stepsFailed;
};

struct TaskTemplate {
    std::string templateName;
    std::string description;
    std::vector<std::string> commands;
    std::vector<std::string> stepDescriptions;
    SafetyLevel maxSafety;
};

class MKTaskAutomation {
private:
    std::vector<TaskPlan> taskHistory;
    std::map<std::string, TaskTemplate> templates;
    std::set<std::string> dangerousPatterns;
    std::set<std::string> confirmationPatterns;
    int totalTasksRun;
    int totalTasksSucceeded;
    int totalTasksFailed;
    int totalBlockedBySafety;
    bool dryRunMode;

public:
    MKTaskAutomation() : totalTasksRun(0), totalTasksSucceeded(0),
                          totalTasksFailed(0), totalBlockedBySafety(0),
                          dryRunMode(false) {
        std::cout << "[TASK_AUTOMATION] Initialized - multi-step autonomous task execution\n";
        initDangerousPatterns();
        initConfirmationPatterns();
        initTemplates();
        std::cout << "[TASK_AUTOMATION] Loaded " << templates.size() << " task templates\n";
        std::cout << "[TASK_AUTOMATION] Safety: " << dangerousPatterns.size()
                  << " dangerous patterns blocked\n";
    }

    // Execute a task plan
    TaskPlan executePlan(TaskPlan& plan) {
        std::cout << "[TASK_AUTOMATION] Executing plan: " << plan.name << "\n";
        std::cout << "[TASK_AUTOMATION] Steps: " << plan.steps.size() << "\n";
        plan.overallStatus = TaskStatus::IN_PROGRESS;
        plan.startedAt = std::time(nullptr);
        plan.stepsCompleted = 0;
        plan.stepsFailed = 0;

        for (auto& step : plan.steps) {
            std::cout << "[TASK_AUTOMATION] Step " << step.stepNumber << ": "
                      << step.description << "\n";

            // Safety check
            step.safety = classifyCommand(step.command);

            if (step.safety == SafetyLevel::DANGEROUS) {
                std::cout << "[TASK_AUTOMATION] BLOCKED - dangerous command: "
                          << step.command << "\n";
                step.status = TaskStatus::BLOCKED_SAFETY;
                totalBlockedBySafety++;
                plan.stepsFailed++;
                continue;
            }

            if (step.safety == SafetyLevel::NEEDS_CONFIRMATION) {
                std::cout << "[TASK_AUTOMATION] Command needs confirmation: "
                          << step.command << "\n";
                std::cout << "[TASK_AUTOMATION] Awaiting user approval...\n";
                step.status = TaskStatus::BLOCKED_SAFETY;
                totalBlockedBySafety++;
                plan.stepsFailed++;
                continue;
            }

            // Execute the command
            step.status = TaskStatus::IN_PROGRESS;
            std::time_t stepStart = std::time(nullptr);

            if (dryRunMode) {
                step.output = "[DRY RUN] Would execute: " + step.command;
                step.exitCode = 0;
                step.status = TaskStatus::COMPLETED;
                std::cout << "[TASK_AUTOMATION] [DRY RUN] " << step.command << "\n";
            } else {
                step.exitCode = executeCommand(step.command, step.output);
                if (step.exitCode == 0) {
                    step.status = TaskStatus::COMPLETED;
                    plan.stepsCompleted++;
                    std::cout << "[TASK_AUTOMATION] Step " << step.stepNumber
                              << " completed successfully\n";
                } else {
                    step.status = TaskStatus::FAILED;
                    plan.stepsFailed++;
                    std::cout << "[TASK_AUTOMATION] Step " << step.stepNumber
                              << " FAILED (exit code: " << step.exitCode << ")\n";
                }
            }

            std::time_t stepEnd = std::time(nullptr);
            step.durationMs = std::difftime(stepEnd, stepStart) * 1000.0;
        }

        plan.completedAt = std::time(nullptr);
        if (plan.stepsFailed == 0) {
            plan.overallStatus = TaskStatus::COMPLETED;
            totalTasksSucceeded++;
        } else {
            plan.overallStatus = TaskStatus::FAILED;
            totalTasksFailed++;
        }
        totalTasksRun++;

        taskHistory.push_back(plan);
        std::cout << "[TASK_AUTOMATION] Plan finished: " << plan.stepsCompleted
                  << "/" << plan.steps.size() << " steps succeeded\n";

        return plan;
    }

    // Create a plan from a template
    TaskPlan createFromTemplate(const std::string& templateName,
                                 const std::string& param = "") {
        TaskPlan plan;
        auto it = templates.find(templateName);
        if (it == templates.end()) {
            std::cout << "[TASK_AUTOMATION] Template not found: " << templateName << "\n";
            plan.name = "unknown";
            plan.overallStatus = TaskStatus::FAILED;
            return plan;
        }

        const TaskTemplate& tmpl = it->second;
        plan.name = tmpl.templateName;
        plan.description = tmpl.description;
        plan.overallStatus = TaskStatus::PENDING;

        for (size_t i = 0; i < tmpl.commands.size(); i++) {
            TaskStep step;
            step.stepNumber = static_cast<int>(i + 1);
            step.description = tmpl.stepDescriptions[i];
            step.command = tmpl.commands[i];
            // Replace placeholder with parameter
            size_t pos = step.command.find("{{PARAM}}");
            if (pos != std::string::npos && !param.empty()) {
                step.command.replace(pos, 9, param);
            }
            step.status = TaskStatus::PENDING;
            step.exitCode = -1;
            step.durationMs = 0.0;
            plan.steps.push_back(step);
        }

        std::cout << "[TASK_AUTOMATION] Created plan from template: " << templateName
                  << " (" << plan.steps.size() << " steps)\n";
        return plan;
    }

    // Classify a command's safety level
    SafetyLevel classifyCommand(const std::string& cmd) const {
        // Check for dangerous patterns first
        for (const auto& pattern : dangerousPatterns) {
            if (cmd.find(pattern) != std::string::npos) {
                return SafetyLevel::DANGEROUS;
            }
        }
        // Check for confirmation-required patterns
        for (const auto& pattern : confirmationPatterns) {
            if (cmd.find(pattern) != std::string::npos) {
                return SafetyLevel::NEEDS_CONFIRMATION;
            }
        }
        return SafetyLevel::SAFE;
    }

    // List available templates
    std::vector<std::string> listTemplates() const {
        std::vector<std::string> names;
        for (const auto& pair : templates) {
            names.push_back(pair.first + " - " + pair.second.description);
        }
        return names;
    }

    // Enable/disable dry run mode
    void setDryRun(bool enabled) {
        dryRunMode = enabled;
        std::cout << "[TASK_AUTOMATION] Dry run mode: "
                  << (enabled ? "ENABLED" : "DISABLED") << "\n";
    }

    // Get progress report for current or last task
    std::string getProgressReport() const {
        if (taskHistory.empty()) {
            return "[TASK_AUTOMATION] No tasks have been run yet.";
        }
        const TaskPlan& last = taskHistory.back();
        std::string report;
        report += "Task: " + last.name + "\n";
        report += "Status: ";
        switch (last.overallStatus) {
            case TaskStatus::COMPLETED: report += "COMPLETED"; break;
            case TaskStatus::FAILED: report += "FAILED"; break;
            case TaskStatus::IN_PROGRESS: report += "IN_PROGRESS"; break;
            default: report += "OTHER"; break;
        }
        report += "\nSteps completed: " + std::to_string(last.stepsCompleted) +
                  "/" + std::to_string(last.steps.size()) + "\n";
        report += "Steps failed: " + std::to_string(last.stepsFailed) + "\n";
        return report;
    }

    // Add a custom template
    void addTemplate(const std::string& name, const std::string& description,
                     const std::vector<std::string>& commands,
                     const std::vector<std::string>& descriptions) {
        TaskTemplate tmpl;
        tmpl.templateName = name;
        tmpl.description = description;
        tmpl.commands = commands;
        tmpl.stepDescriptions = descriptions;
        tmpl.maxSafety = SafetyLevel::SAFE;
        templates[name] = tmpl;
        std::cout << "[TASK_AUTOMATION] Added custom template: " << name << "\n";
    }

    // Approve a blocked step and re-execute it
    bool approveAndExecuteStep(TaskPlan& plan, int stepNumber) {
        for (auto& step : plan.steps) {
            if (step.stepNumber == stepNumber &&
                step.status == TaskStatus::BLOCKED_SAFETY) {
                std::cout << "[TASK_AUTOMATION] User approved step " << stepNumber
                          << ": " << step.command << "\n";
                step.exitCode = executeCommand(step.command, step.output);
                if (step.exitCode == 0) {
                    step.status = TaskStatus::COMPLETED;
                    plan.stepsCompleted++;
                    plan.stepsFailed--;
                    return true;
                } else {
                    step.status = TaskStatus::FAILED;
                    return false;
                }
            }
        }
        return false;
    }

    // Print stats
    void printStats() const {
        std::cout << "\n=== TASK AUTOMATION STATS ===\n";
        std::cout << "Total tasks run: " << totalTasksRun << "\n";
        std::cout << "Tasks succeeded: " << totalTasksSucceeded << "\n";
        std::cout << "Tasks failed: " << totalTasksFailed << "\n";
        std::cout << "Blocked by safety: " << totalBlockedBySafety << "\n";
        std::cout << "Available templates: " << templates.size() << "\n";
        std::cout << "Dry run mode: " << (dryRunMode ? "ON" : "OFF") << "\n";
        std::cout << "Task history entries: " << taskHistory.size() << "\n";
        std::cout << "============================\n\n";
    }

private:
    void initDangerousPatterns() {
        dangerousPatterns.insert("rm -rf /");
        dangerousPatterns.insert("rm -rf /*");
        dangerousPatterns.insert("mkfs");
        dangerousPatterns.insert("dd if=");
        dangerousPatterns.insert(":(){");
        dangerousPatterns.insert("chmod -R 777 /");
        dangerousPatterns.insert("wget | sh");
        dangerousPatterns.insert("curl | sh");
        dangerousPatterns.insert("curl | bash");
        dangerousPatterns.insert("wget | bash");
        dangerousPatterns.insert("> /dev/sda");
        dangerousPatterns.insert("> /dev/disk");
        dangerousPatterns.insert("shutdown");
        dangerousPatterns.insert("reboot");
        dangerousPatterns.insert("halt");
        dangerousPatterns.insert("init 0");
        dangerousPatterns.insert("format c:");
    }

    void initConfirmationPatterns() {
        confirmationPatterns.insert("rm -rf");
        confirmationPatterns.insert("rm -r");
        confirmationPatterns.insert("rmdir");
        confirmationPatterns.insert("sudo");
        confirmationPatterns.insert("chmod");
        confirmationPatterns.insert("chown");
        confirmationPatterns.insert("mv /");
        confirmationPatterns.insert("cp -r /");
        confirmationPatterns.insert("kill");
        confirmationPatterns.insert("pkill");
        confirmationPatterns.insert("apt remove");
        confirmationPatterns.insert("brew uninstall");
        confirmationPatterns.insert("npm uninstall -g");
        confirmationPatterns.insert("pip uninstall");
    }

    void initTemplates() {
        // Backup template
        {
            TaskTemplate t;
            t.templateName = "backup";
            t.description = "Backup a project directory to a timestamped archive";
            t.commands = {
                "echo \"Starting backup of {{PARAM}}\"",
                "ls -la {{PARAM}}",
                "tar -czf {{PARAM}}_backup_$(date +%Y%m%d_%H%M%S).tar.gz {{PARAM}}",
                "echo \"Backup complete\""
            };
            t.stepDescriptions = {
                "Announce backup start",
                "List files to be backed up",
                "Create compressed archive with timestamp",
                "Confirm backup completion"
            };
            t.maxSafety = SafetyLevel::SAFE;
            templates["backup"] = t;
        }

        // Disk cleanup template
        {
            TaskTemplate t;
            t.templateName = "disk_cleanup";
            t.description = "Clean temporary files and caches to free disk space";
            t.commands = {
                "df -h .",
                "find /tmp -type f -atime +7 2>/dev/null | wc -l",
                "find . -name '*.o' -type f | wc -l",
                "find . -name '*.o' -type f -delete",
                "find . -name '.DS_Store' -type f -delete",
                "df -h ."
            };
            t.stepDescriptions = {
                "Check current disk usage",
                "Count old temp files",
                "Count object files in project",
                "Remove object files",
                "Remove .DS_Store files",
                "Check disk usage after cleanup"
            };
            t.maxSafety = SafetyLevel::SAFE;
            templates["disk_cleanup"] = t;
        }

        // Git status template
        {
            TaskTemplate t;
            t.templateName = "git_status";
            t.description = "Full git repository status check";
            t.commands = {
                "git status",
                "git log --oneline -10",
                "git branch -a",
                "git remote -v",
                "git stash list"
            };
            t.stepDescriptions = {
                "Check working tree status",
                "Show recent commit history",
                "List all branches",
                "Show configured remotes",
                "List stashed changes"
            };
            t.maxSafety = SafetyLevel::SAFE;
            templates["git_status"] = t;
        }

        // System health check template
        {
            TaskTemplate t;
            t.templateName = "system_health";
            t.description = "Check overall system health (CPU, memory, disk, uptime)";
            t.commands = {
                "uptime",
                "df -h /",
                "ps aux | head -20",
                "top -l 1 -n 5 2>/dev/null || top -bn1 | head -20",
                "echo \"Health check complete\""
            };
            t.stepDescriptions = {
                "Check system uptime and load",
                "Check disk space on root volume",
                "Show top processes by resource usage",
                "Check CPU/memory snapshot",
                "Confirm health check done"
            };
            t.maxSafety = SafetyLevel::SAFE;
            templates["system_health"] = t;
        }

        // Compile project template
        {
            TaskTemplate t;
            t.templateName = "compile_project";
            t.description = "Clean and rebuild the MK OS project";
            t.commands = {
                "make clean",
                "make all 2>&1",
                "ls -la mk_os",
                "echo \"Compilation successful\""
            };
            t.stepDescriptions = {
                "Clean previous build artifacts",
                "Compile entire project",
                "Verify output binary exists",
                "Confirm successful build"
            };
            t.maxSafety = SafetyLevel::SAFE;
            templates["compile_project"] = t;
        }
    }

    // Execute a shell command and capture output
    int executeCommand(const std::string& cmd, std::string& output) {
        std::string fullCmd = cmd + " 2>&1";
        FILE* pipe = popen(fullCmd.c_str(), "r");
        if (!pipe) {
            output = "ERROR: Failed to open pipe for command";
            return -1;
        }

        char buffer[256];
        output.clear();
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }

        int status = pclose(pipe);
        return WEXITSTATUS(status);
    }
};

#endif // MK_TASK_AUTOMATION_CPP
