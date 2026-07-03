#ifndef MK_GOAL_ENGINE_CPP
#define MK_GOAL_ENGINE_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

// ===================================================================================
// MK GOAL ENGINE
// Implements the Think-Act-Evaluate-Learn autonomous loop.
// Goals have: description, priority, deadline, sub_goals[], progress (0-100),
// success_criteria, assigned_device.
// The engine picks the highest priority goal, breaks it into actions, executes,
// evaluates results, and learns from outcomes.
// ===================================================================================

enum class MKGoalStatus {
    PENDING,
    IN_PROGRESS,
    COMPLETED,
    FAILED,
    BLOCKED,
    CANCELLED
};

enum class MKGoalPriority {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4,
    URGENT = 5
};

struct MKGoalAction {
    std::string description;
    std::string command;
    MKGoalStatus status;
    std::string result;
    std::time_t started_at;
    std::time_t completed_at;
    float success_score;  // 0-1
};

struct MKGoal {
    int id;
    std::string description;
    MKGoalPriority priority;
    std::time_t deadline;
    std::vector<int> sub_goal_ids;
    float progress;       // 0-100
    std::string success_criteria;
    std::string assigned_device;
    MKGoalStatus status;
    std::vector<MKGoalAction> actions;
    std::time_t created_at;
    std::time_t updated_at;
    std::string category;  // earning, learning, maintenance, growth
    int attempts;
    std::string last_evaluation;
};

struct MKLessonLearned {
    std::string context;
    std::string lesson;
    std::time_t learned_at;
    int goal_id;
    float confidence;
};

class MKGoalEngine {
private:
    std::vector<MKGoal> goals;
    std::vector<MKLessonLearned> lessons;
    int next_goal_id;
    int total_completed;
    int total_failed;
    float overall_success_rate;

    // Think: analyze the current situation and pick the best goal
    int selectNextGoal() const {
        int best_id = -1;
        int best_score = -1;

        for (const auto& goal : goals) {
            if (goal.status != MKGoalStatus::PENDING &&
                goal.status != MKGoalStatus::IN_PROGRESS) continue;

            int score = static_cast<int>(goal.priority) * 100;
            // Urgency bonus for approaching deadlines
            if (goal.deadline > 0) {
                double hours_left = std::difftime(goal.deadline, std::time(nullptr)) / 3600.0;
                if (hours_left < 24) score += 200;
                else if (hours_left < 72) score += 100;
            }
            // In-progress goals get a continuation bonus
            if (goal.status == MKGoalStatus::IN_PROGRESS) score += 50;

            if (score > best_score) {
                best_score = score;
                best_id = goal.id;
            }
        }
        return best_id;
    }

    // Evaluate: assess how well an action went
    float evaluateAction(const MKGoalAction& action) const {
        if (action.status == MKGoalStatus::COMPLETED) return 1.0f;
        if (action.status == MKGoalStatus::FAILED) return 0.0f;
        return 0.5f;
    }

    // Learn: extract lessons from completed/failed goals
    void extractLesson(const MKGoal& goal) {
        MKLessonLearned lesson;
        lesson.goal_id = goal.id;
        lesson.learned_at = std::time(nullptr);
        lesson.context = goal.category + ": " + goal.description;

        if (goal.status == MKGoalStatus::COMPLETED) {
            lesson.lesson = "Successfully completed: " + goal.description +
                          " in " + std::to_string(goal.attempts) + " attempts.";
            lesson.confidence = 0.9f;
        } else if (goal.status == MKGoalStatus::FAILED) {
            lesson.lesson = "Failed: " + goal.description +
                          ". Last evaluation: " + goal.last_evaluation;
            lesson.confidence = 0.7f;
        } else {
            return;  // No lesson from in-progress goals
        }

        lessons.push_back(lesson);
    }

public:
    MKGoalEngine() : next_goal_id(1), total_completed(0), total_failed(0),
                     overall_success_rate(0.5f) {}

    // Create a new goal
    int createGoal(const std::string& description, MKGoalPriority priority,
                   const std::string& success_criteria, const std::string& category = "general",
                   const std::string& assigned_device = "local",
                   std::time_t deadline = 0) {
        MKGoal goal;
        goal.id = next_goal_id++;
        goal.description = description;
        goal.priority = priority;
        goal.deadline = deadline;
        goal.progress = 0.0f;
        goal.success_criteria = success_criteria;
        goal.assigned_device = assigned_device;
        goal.status = MKGoalStatus::PENDING;
        goal.created_at = std::time(nullptr);
        goal.updated_at = goal.created_at;
        goal.category = category;
        goal.attempts = 0;
        goals.push_back(goal);
        return goal.id;
    }

    // Add a sub-goal to an existing goal
    int addSubGoal(int parent_id, const std::string& description,
                   const std::string& success_criteria) {
        int sub_id = createGoal(description, MKGoalPriority::MEDIUM, success_criteria);

        for (auto& g : goals) {
            if (g.id == parent_id) {
                g.sub_goal_ids.push_back(sub_id);
                break;
            }
        }
        return sub_id;
    }

    // Break a goal into actions (the Act phase)
    void breakIntoActions(int goal_id, const std::vector<std::string>& action_descriptions) {
        for (auto& g : goals) {
            if (g.id == goal_id) {
                for (const auto& desc : action_descriptions) {
                    MKGoalAction action;
                    action.description = desc;
                    action.status = MKGoalStatus::PENDING;
                    action.started_at = 0;
                    action.completed_at = 0;
                    action.success_score = 0.0f;
                    g.actions.push_back(action);
                }
                g.status = MKGoalStatus::IN_PROGRESS;
                g.updated_at = std::time(nullptr);
                break;
            }
        }
    }

    // Complete an action within a goal
    void completeAction(int goal_id, int action_index, const std::string& result,
                       float success_score = 1.0f) {
        for (auto& g : goals) {
            if (g.id == goal_id && action_index < (int)g.actions.size()) {
                g.actions[action_index].status = MKGoalStatus::COMPLETED;
                g.actions[action_index].result = result;
                g.actions[action_index].completed_at = std::time(nullptr);
                g.actions[action_index].success_score = success_score;

                // Update progress
                int completed = 0;
                for (const auto& a : g.actions) {
                    if (a.status == MKGoalStatus::COMPLETED) completed++;
                }
                g.progress = (float)completed / (float)g.actions.size() * 100.0f;
                g.updated_at = std::time(nullptr);
                g.attempts++;

                // Check if all actions are done
                if (completed == (int)g.actions.size()) {
                    g.status = MKGoalStatus::COMPLETED;
                    total_completed++;
                    updateSuccessRate();
                    extractLesson(g);
                }
                break;
            }
        }
    }

    // Fail a goal
    void failGoal(int goal_id, const std::string& reason) {
        for (auto& g : goals) {
            if (g.id == goal_id) {
                g.status = MKGoalStatus::FAILED;
                g.last_evaluation = reason;
                g.updated_at = std::time(nullptr);
                total_failed++;
                updateSuccessRate();
                extractLesson(g);
                break;
            }
        }
    }

    // The Think-Act-Evaluate-Learn cycle (one tick)
    std::string tick() {
        // THINK: Select next goal
        int goal_id = selectNextGoal();
        if (goal_id < 0) return "No active goals.";

        MKGoal* goal = nullptr;
        for (auto& g : goals) {
            if (g.id == goal_id) { goal = &g; break; }
        }
        if (!goal) return "Goal not found.";

        std::ostringstream result;
        result << "THINK: Selected goal #" << goal->id << " (" << goal->description << ")\n";

        // ACT: Find next pending action
        int next_action = -1;
        for (int i = 0; i < (int)goal->actions.size(); i++) {
            if (goal->actions[i].status == MKGoalStatus::PENDING) {
                next_action = i;
                break;
            }
        }

        if (next_action >= 0) {
            goal->actions[next_action].status = MKGoalStatus::IN_PROGRESS;
            goal->actions[next_action].started_at = std::time(nullptr);
            result << "ACT: Executing action: " << goal->actions[next_action].description << "\n";
        } else if (goal->actions.empty()) {
            result << "ACT: Goal has no actions defined. Break it down first.\n";
        } else {
            result << "ACT: All actions in progress or completed.\n";
        }

        // EVALUATE: Check overall progress
        result << "EVALUATE: Progress at " << (int)goal->progress << "%, "
               << "status: " << (goal->status == MKGoalStatus::IN_PROGRESS ? "in_progress" : "pending")
               << "\n";

        // LEARN: Apply any relevant lessons
        for (const auto& lesson : lessons) {
            if (lesson.context.find(goal->category) != std::string::npos) {
                result << "LEARN: Applying lesson: " << lesson.lesson << "\n";
                break;
            }
        }

        return result.str();
    }

    // Get the current highest-priority goal
    MKGoal* getCurrentGoal() {
        int id = selectNextGoal();
        for (auto& g : goals) {
            if (g.id == id) return &g;
        }
        return nullptr;
    }

    // Get all active goals
    std::vector<MKGoal> getActiveGoals() const {
        std::vector<MKGoal> active;
        for (const auto& g : goals) {
            if (g.status == MKGoalStatus::PENDING || g.status == MKGoalStatus::IN_PROGRESS) {
                active.push_back(g);
            }
        }
        return active;
    }

    // Get all lessons learned
    std::vector<MKLessonLearned> getLessons() const { return lessons; }

    // Get stats
    int goalCount() const { return static_cast<int>(goals.size()); }
    int completedCount() const { return total_completed; }
    int failedCount() const { return total_failed; }
    float successRate() const { return overall_success_rate; }

    void updateSuccessRate() {
        int total = total_completed + total_failed;
        if (total > 0) {
            overall_success_rate = (float)total_completed / (float)total;
        }
    }

    // Get summary
    std::string getSummary() const {
        std::ostringstream ss;
        ss << "=== Goal Engine ===\n";
        ss << "  Goals: " << goals.size() << " total, "
           << total_completed << " completed, " << total_failed << " failed\n";
        ss << "  Success rate: " << (int)(overall_success_rate * 100) << "%\n";
        ss << "  Lessons learned: " << lessons.size() << "\n";

        auto active = getActiveGoals();
        if (!active.empty()) {
            ss << "  Active goals:\n";
            for (const auto& g : active) {
                ss << "    [" << (int)g.priority << "] " << g.description
                   << " (" << (int)g.progress << "%)\n";
            }
        }
        return ss.str();
    }
};

#endif // MK_GOAL_ENGINE_CPP
