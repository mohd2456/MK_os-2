#ifndef MK_STRATEGY_PLANNER_CPP
#define MK_STRATEGY_PLANNER_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <cmath>

// ===================================================================================
// MK STRATEGY PLANNER
// Long-term planning for MK. Maintains daily/weekly/monthly goals.
// Plans earning strategies (which crypto to trade, what skills to develop).
// Allocates resources across devices. Adjusts strategy based on results.
// ===================================================================================

enum class MKStrategyTimeframe {
    DAILY,
    WEEKLY,
    MONTHLY
};

struct MKStrategyGoal {
    int id;
    std::string description;
    MKStrategyTimeframe timeframe;
    float target_value;      // What we want to achieve (e.g., $100 profit)
    float current_value;     // Current progress toward target
    std::string metric;      // What we measure (profit_usd, skills_improved, tasks_completed)
    std::time_t created_at;
    std::time_t deadline;
    bool completed;
    std::string assigned_device;
};

struct MKResourceAllocation {
    std::string device_name;
    std::string task_type;       // trading, learning, monitoring, syncing
    float cpu_allocation;        // 0-100 percent
    float priority;              // 0-1
    std::time_t valid_until;
};

struct MKStrategyResult {
    std::string strategy_name;
    float expected_return;
    float actual_return;
    std::time_t period_start;
    std::time_t period_end;
    std::string notes;
};

struct MKEarningStrategy {
    std::string name;
    std::string description;
    float estimated_daily_return;
    float risk_level;            // 0-1
    std::string required_skill;
    float min_skill_level;
    std::vector<std::string> required_devices;
    bool active;
};

class MKStrategyPlanner {
private:
    std::vector<MKStrategyGoal> daily_goals;
    std::vector<MKStrategyGoal> weekly_goals;
    std::vector<MKStrategyGoal> monthly_goals;
    std::vector<MKResourceAllocation> allocations;
    std::vector<MKStrategyResult> history;
    std::vector<MKEarningStrategy> strategies;
    int next_goal_id;

    void initDefaultStrategies() {
        strategies.push_back({"crypto_swing_trade",
            "Swing trading on 4h/1d timeframes using technical analysis signals",
            25.0f, 0.6f, "crypto_trading", 30.0f, {"main_rig"}, true});

        strategies.push_back({"airdrop_farming",
            "Qualifying for token airdrops by interacting with protocols",
            5.0f, 0.2f, "crypto_trading", 15.0f, {"main_rig", "phone"}, true});

        strategies.push_back({"defi_yield",
            "Providing liquidity and staking for passive yield",
            15.0f, 0.4f, "crypto_trading", 40.0f, {"main_rig"}, false});

        strategies.push_back({"knowledge_arbitrage",
            "Gathering and synthesizing information faster than competitors",
            0.0f, 0.1f, "knowledge_gathering", 20.0f, {"main_rig", "phone"}, true});

        strategies.push_back({"system_optimization",
            "Optimizing homelab for lower costs and better performance",
            2.0f, 0.1f, "system_administration", 25.0f, {"main_rig"}, true});
    }

public:
    MKStrategyPlanner() : next_goal_id(1) {
        initDefaultStrategies();
    }

    // Add a strategic goal
    int addGoal(const std::string& description, MKStrategyTimeframe timeframe,
                float target_value, const std::string& metric,
                const std::string& device = "any") {
        MKStrategyGoal goal;
        goal.id = next_goal_id++;
        goal.description = description;
        goal.timeframe = timeframe;
        goal.target_value = target_value;
        goal.current_value = 0.0f;
        goal.metric = metric;
        goal.created_at = std::time(nullptr);
        goal.completed = false;
        goal.assigned_device = device;

        // Set deadline based on timeframe
        switch (timeframe) {
            case MKStrategyTimeframe::DAILY:
                goal.deadline = goal.created_at + 86400;
                daily_goals.push_back(goal);
                break;
            case MKStrategyTimeframe::WEEKLY:
                goal.deadline = goal.created_at + 604800;
                weekly_goals.push_back(goal);
                break;
            case MKStrategyTimeframe::MONTHLY:
                goal.deadline = goal.created_at + 2592000;
                monthly_goals.push_back(goal);
                break;
        }
        return goal.id;
    }

    // Update progress on a goal
    void updateGoalProgress(int goal_id, float new_value) {
        auto update = [&](std::vector<MKStrategyGoal>& goals) {
            for (auto& g : goals) {
                if (g.id == goal_id) {
                    g.current_value = new_value;
                    if (new_value >= g.target_value) g.completed = true;
                    return true;
                }
            }
            return false;
        };
        if (update(daily_goals)) return;
        if (update(weekly_goals)) return;
        update(monthly_goals);
    }

    // Allocate resources to a device for a task
    void allocateResources(const std::string& device, const std::string& task_type,
                          float cpu_percent, float priority, int hours = 24) {
        MKResourceAllocation alloc;
        alloc.device_name = device;
        alloc.task_type = task_type;
        alloc.cpu_allocation = cpu_percent;
        alloc.priority = priority;
        alloc.valid_until = std::time(nullptr) + (hours * 3600);
        allocations.push_back(alloc);
    }

    // Get active resource allocations for a device
    std::vector<MKResourceAllocation> getDeviceAllocations(const std::string& device) const {
        std::vector<MKResourceAllocation> result;
        std::time_t now = std::time(nullptr);
        for (const auto& a : allocations) {
            if (a.device_name == device && a.valid_until > now) {
                result.push_back(a);
            }
        }
        return result;
    }

    // Record a strategy result for learning
    void recordResult(const std::string& strategy_name, float expected, float actual,
                     const std::string& notes = "") {
        MKStrategyResult r;
        r.strategy_name = strategy_name;
        r.expected_return = expected;
        r.actual_return = actual;
        r.period_start = std::time(nullptr) - 86400;
        r.period_end = std::time(nullptr);
        r.notes = notes;
        history.push_back(r);
    }

    // Get active earning strategies
    std::vector<MKEarningStrategy> getActiveStrategies() const {
        std::vector<MKEarningStrategy> active;
        for (const auto& s : strategies) {
            if (s.active) active.push_back(s);
        }
        return active;
    }

    // Get best strategy given current skill levels
    std::string getBestStrategy(const std::map<std::string, float>& skill_levels) const {
        float best_score = -1.0f;
        std::string best_name;

        for (const auto& s : strategies) {
            if (!s.active) continue;
            auto it = skill_levels.find(s.required_skill);
            float current_skill = (it != skill_levels.end()) ? it->second : 0.0f;
            if (current_skill < s.min_skill_level) continue;

            // Score: return potential adjusted by risk and skill match
            float skill_bonus = (current_skill - s.min_skill_level) / 100.0f;
            float score = s.estimated_daily_return * (1.0f - s.risk_level * 0.5f) * (1.0f + skill_bonus);
            if (score > best_score) {
                best_score = score;
                best_name = s.name;
            }
        }
        return best_name;
    }

    // Adjust strategy based on historical results
    void adjustStrategies() {
        if (history.empty()) return;

        for (auto& strategy : strategies) {
            float total_expected = 0.0f;
            float total_actual = 0.0f;
            int count = 0;

            for (const auto& r : history) {
                if (r.strategy_name == strategy.name) {
                    total_expected += r.expected_return;
                    total_actual += r.actual_return;
                    count++;
                }
            }

            if (count >= 3) {
                float ratio = total_actual / (total_expected + 0.01f);
                // Adjust estimated return based on actual performance
                strategy.estimated_daily_return *= (0.7f + 0.3f * ratio);
                // Disable strategies that consistently underperform
                if (ratio < 0.3f && count >= 5) {
                    strategy.active = false;
                }
            }
        }
    }

    // Get goals by timeframe
    std::vector<MKStrategyGoal> getDailyGoals() const { return daily_goals; }
    std::vector<MKStrategyGoal> getWeeklyGoals() const { return weekly_goals; }
    std::vector<MKStrategyGoal> getMonthlyGoals() const { return monthly_goals; }

    // Get total goal count
    int goalCount() const {
        return static_cast<int>(daily_goals.size() + weekly_goals.size() + monthly_goals.size());
    }

    // Get summary
    std::string getSummary() const {
        std::ostringstream ss;
        ss << "=== Strategy Planner ===\n";
        ss << "  Daily goals: " << daily_goals.size() << "\n";
        ss << "  Weekly goals: " << weekly_goals.size() << "\n";
        ss << "  Monthly goals: " << monthly_goals.size() << "\n";
        ss << "  Active strategies: ";
        int active = 0;
        for (const auto& s : strategies) if (s.active) active++;
        ss << active << "/" << strategies.size() << "\n";
        ss << "  Resource allocations: " << allocations.size() << "\n";
        ss << "  Historical results: " << history.size() << "\n";
        return ss.str();
    }
};

#endif // MK_STRATEGY_PLANNER_CPP
