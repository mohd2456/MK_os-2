#ifndef MK_TASK_PLANNER_CPP
#define MK_TASK_PLANNER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <ctime>

// ===========================================================================
// MK TASK PLANNER - Goal Decomposition and Planning
// ===========================================================================
// Breaks high-level goals into actionable task lists. Given a goal like
// 'build a web app', produces a structured plan with milestones,
// dependencies, and time estimates.
// ===========================================================================

// Task priority level
enum class MKTaskPriority { LOW, MEDIUM, HIGH, CRITICAL };

// Task status
enum class MKTaskStatus { TODO, IN_PROGRESS, BLOCKED, DONE, SKIPPED };

// A single task within a plan
struct MKTask {
    int id;
    std::string title;
    std::string description;
    MKTaskPriority priority;
    MKTaskStatus status;
    int estimated_minutes;
    std::vector<int> dependencies;  // task IDs that must complete first
    std::string category;           // setup, development, testing, deployment
    int milestone_id;               // which milestone this belongs to
};

// A milestone (group of related tasks)
struct MKMilestone {
    int id;
    std::string name;
    std::string description;
    std::vector<int> task_ids;
    int estimated_days;
    bool completed;
};

// A complete project plan
struct MKTaskPlan {
    std::string goal;
    std::string project_type;
    std::vector<MKMilestone> milestones;
    std::vector<MKTask> tasks;
    int total_estimated_hours;
    int total_tasks;
    std::string created_at;
};

// Plan template for common project types
struct MKPlanTemplate {
    std::string project_type;
    std::vector<std::string> keywords;
    std::vector<std::pair<std::string, std::vector<std::string>>> milestones;  // name -> tasks
    int base_hours;
};

class MKTaskPlanner {
private:
    std::vector<MKPlanTemplate> templates;
    int total_plans_created;

    void initTemplates() {
        // Web application template
        templates.push_back({"web_app",
            {"web", "website", "webapp", "frontend", "fullstack", "react", "vue"},
            {
                {"Setup & Architecture", {
                    "Initialize project with build tools",
                    "Set up version control (git)",
                    "Define project structure and directories",
                    "Choose and configure framework",
                    "Set up development environment"}},
                {"Backend Development", {
                    "Design database schema",
                    "Set up database and ORM",
                    "Implement authentication system",
                    "Build REST API endpoints",
                    "Add input validation and error handling",
                    "Implement business logic layer"}},
                {"Frontend Development", {
                    "Create component hierarchy",
                    "Build layout and navigation",
                    "Implement forms and user input",
                    "Add state management",
                    "Connect to backend API",
                    "Responsive design and styling"}},
                {"Testing & QA", {
                    "Write unit tests for backend",
                    "Write component tests for frontend",
                    "Integration testing",
                    "Fix bugs and edge cases",
                    "Performance optimization"}},
                {"Deployment", {
                    "Set up CI/CD pipeline",
                    "Configure production environment",
                    "Deploy application",
                    "Set up monitoring and logging",
                    "Documentation"}},
            },
            80  // base hours
        });

        // CLI tool template
        templates.push_back({"cli_tool",
            {"cli", "command line", "terminal", "tool", "script", "utility"},
            {
                {"Design", {
                    "Define CLI interface (commands, flags, arguments)",
                    "Design input/output formats",
                    "Plan error messages and help text"}},
                {"Core Implementation", {
                    "Set up argument parsing",
                    "Implement main command logic",
                    "Add file I/O handling",
                    "Implement sub-commands",
                    "Error handling and validation"}},
                {"Polish & Ship", {
                    "Write unit tests",
                    "Add help text and man page",
                    "Package for distribution",
                    "Create installation instructions"}},
            },
            24  // base hours
        });

        // Data pipeline template
        templates.push_back({"data_pipeline",
            {"data", "pipeline", "etl", "analytics", "processing", "transform"},
            {
                {"Data Exploration", {
                    "Identify data sources",
                    "Profile source data quality",
                    "Define schema and transformations",
                    "Design pipeline architecture"}},
                {"Implementation", {
                    "Build data ingestion layer",
                    "Implement transformation logic",
                    "Add data validation checks",
                    "Build output/load stage",
                    "Add error handling and retries"}},
                {"Operations", {
                    "Set up scheduling (cron/airflow)",
                    "Add monitoring and alerting",
                    "Performance tuning",
                    "Documentation and runbooks"}},
            },
            40  // base hours
        });

        // Mobile app template
        templates.push_back({"mobile_app",
            {"mobile", "app", "ios", "android", "flutter", "react native"},
            {
                {"Planning & Design", {
                    "Define user stories",
                    "Create wireframes/mockups",
                    "Design navigation flow",
                    "Set up development environment"}},
                {"Core Features", {
                    "Build navigation structure",
                    "Implement main screens",
                    "Add authentication flow",
                    "Integrate with backend API",
                    "Implement offline support"}},
                {"Polish & Launch", {
                    "UI polish and animations",
                    "Testing on multiple devices",
                    "Performance optimization",
                    "App store preparation",
                    "Submit for review"}},
            },
            100  // base hours
        });
    }

    // Detect project type from goal description
    std::string detectProjectType(const std::string& goal) {
        std::string lower = goal;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        for (const auto& tmpl : templates) {
            for (const auto& keyword : tmpl.keywords) {
                if (lower.find(keyword) != std::string::npos) {
                    return tmpl.project_type;
                }
            }
        }
        return "web_app";  // default
    }

    // Get template by project type
    const MKPlanTemplate* getTemplate(const std::string& type) {
        for (const auto& t : templates) {
            if (t.project_type == type) return &t;
        }
        return nullptr;
    }

public:
    MKTaskPlanner() : total_plans_created(0) {
        initTemplates();
    }

    // Create a plan from a goal description
    MKTaskPlan createPlan(const std::string& goal) {
        MKTaskPlan plan;
        plan.goal = goal;
        plan.project_type = detectProjectType(goal);
        plan.total_tasks = 0;
        plan.total_estimated_hours = 0;

        time_t now = std::time(nullptr);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        plan.created_at = buf;

        const auto* tmpl = getTemplate(plan.project_type);
        if (!tmpl) return plan;

        int task_id = 1;
        int milestone_id = 1;
        int prev_milestone_last_task = 0;

        for (const auto& [ms_name, ms_tasks] : tmpl->milestones) {
            MKMilestone ms;
            ms.id = milestone_id++;
            ms.name = ms_name;
            ms.description = "Milestone for: " + ms_name;
            ms.completed = false;

            int first_task_in_ms = task_id;
            for (const auto& task_desc : ms_tasks) {
                MKTask task;
                task.id = task_id++;
                task.title = task_desc;
                task.description = task_desc;
                task.priority = MKTaskPriority::MEDIUM;
                task.status = MKTaskStatus::TODO;
                task.estimated_minutes = tmpl->base_hours * 60 / static_cast<int>(ms_tasks.size() * tmpl->milestones.size());
                task.category = ms_name;
                task.milestone_id = ms.id;

                // First task in milestone depends on last task of previous milestone
                if (task.id == first_task_in_ms && prev_milestone_last_task > 0) {
                    task.dependencies.push_back(prev_milestone_last_task);
                }
                // Sequential dependency within milestone
                else if (task.id > first_task_in_ms) {
                    task.dependencies.push_back(task.id - 1);
                }

                ms.task_ids.push_back(task.id);
                plan.tasks.push_back(task);
                plan.total_tasks++;
            }

            ms.estimated_days = static_cast<int>(ms_tasks.size()) * 2;
            plan.milestones.push_back(ms);
            prev_milestone_last_task = task_id - 1;
        }

        plan.total_estimated_hours = tmpl->base_hours;
        total_plans_created++;
        return plan;
    }

    // Format a plan for display
    std::string formatPlan(const MKTaskPlan& plan) {
        std::stringstream ss;
        ss << "\n=== Project Plan: " << plan.goal << " ===\n";
        ss << "Type: " << plan.project_type << "\n";
        ss << "Estimated: " << plan.total_estimated_hours << " hours\n";
        ss << "Tasks: " << plan.total_tasks << "\n\n";

        for (const auto& ms : plan.milestones) {
            ss << "--- " << ms.name << " (" << ms.estimated_days << " days) ---\n";
            for (int tid : ms.task_ids) {
                for (const auto& task : plan.tasks) {
                    if (task.id == tid) {
                        std::string status_icon = "[ ]";
                        if (task.status == MKTaskStatus::DONE) status_icon = "[x]";
                        else if (task.status == MKTaskStatus::IN_PROGRESS) status_icon = "[~]";
                        ss << "  " << status_icon << " " << task.title;
                        if (!task.dependencies.empty())
                            ss << " (after #" << task.dependencies[0] << ")";
                        ss << "\n";
                        break;
                    }
                }
            }
            ss << "\n";
        }
        return ss.str();
    }

    int getTotalPlans() const { return total_plans_created; }

    void printStats() const {
        std::cout << "[MKTaskPlanner] Plans created: " << total_plans_created
                  << ", Templates: " << templates.size() << std::endl;
    }
};

#endif // MK_TASK_PLANNER_CPP
