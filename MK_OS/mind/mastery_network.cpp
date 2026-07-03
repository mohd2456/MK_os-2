#ifndef MK_MASTERY_NETWORK_CPP
#define MK_MASTERY_NETWORK_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

// ===================================================================================
// MK MASTERY NETWORK
// Tracks skills MK is developing over time. Skills improve through active use.
// Implements spaced practice scheduling so MK knows which skills need refreshing.
// Skills: crypto_trading, system_administration, coding, knowledge_gathering,
//         conversation, analysis
// ===================================================================================

struct MKSubSkill {
    std::string name;
    float level;          // 0-100
    float practice_hours;
    std::time_t last_practiced;
};

struct MKSkill {
    std::string name;
    float level;          // 0-100
    float practice_hours;
    std::time_t last_practiced;
    std::vector<MKSubSkill> sub_skills;
    std::vector<std::string> required_for;  // What higher-level abilities need this
    int practice_count;
    float decay_rate;     // How fast this skill decays without practice (0.0-1.0)
};

struct MKPracticeSchedule {
    std::string skill_name;
    std::time_t next_practice;
    float urgency;        // 0-1, how badly this needs practice
    std::string reason;
};

class MKMasteryNetwork {
private:
    std::map<std::string, MKSkill> skills;
    std::vector<MKPracticeSchedule> schedule;
    std::string persistence_path;

    // Calculate skill decay based on time since last practice
    float calculateDecay(const MKSkill& skill) const {
        std::time_t now = std::time(nullptr);
        double hours_since = std::difftime(now, skill.last_practiced) / 3600.0;
        // Exponential decay: skills lose effectiveness without practice
        // Higher-level skills decay slower (mastery retention)
        double retention_factor = 1.0 + (skill.level / 50.0);
        double decay = std::exp(-hours_since / (168.0 * retention_factor));  // Week-based
        return static_cast<float>(decay);
    }

    // Calculate effective skill level (accounting for decay)
    float effectiveLevel(const MKSkill& skill) const {
        float decay = calculateDecay(skill);
        return skill.level * decay;
    }

    // Calculate how much a skill improves from a practice session
    float calculateImprovement(const MKSkill& skill, float session_hours) const {
        // Diminishing returns: harder to improve as level increases
        float difficulty_factor = 1.0f - (skill.level / 120.0f);
        // More practice = more improvement, but with diminishing returns
        float time_factor = std::log2(1.0f + session_hours);
        return difficulty_factor * time_factor * 2.0f;
    }

    void initDefaultSkills() {
        auto makeSkill = [](const std::string& name, float initial_level,
                           const std::vector<std::string>& sub_skill_names,
                           const std::vector<std::string>& required,
                           float decay) -> MKSkill {
            MKSkill s;
            s.name = name;
            s.level = initial_level;
            s.practice_hours = 0.0f;
            s.last_practiced = std::time(nullptr);
            s.practice_count = 0;
            s.decay_rate = decay;
            s.required_for = required;
            for (const auto& sub_name : sub_skill_names) {
                MKSubSkill ss;
                ss.name = sub_name;
                ss.level = initial_level * 0.8f;
                ss.practice_hours = 0.0f;
                ss.last_practiced = std::time(nullptr);
                s.sub_skills.push_back(ss);
            }
            return s;
        };

        skills["crypto_trading"] = makeSkill("crypto_trading", 15.0f,
            {"technical_analysis", "risk_management", "defi_protocols", "airdrop_hunting", "market_psychology"},
            {"earning_income", "self_funding"},
            0.3f);

        skills["system_administration"] = makeSkill("system_administration", 20.0f,
            {"linux_admin", "docker_management", "networking", "ssh_operations", "monitoring"},
            {"homelab_management", "service_orchestration"},
            0.2f);

        skills["coding"] = makeSkill("coding", 25.0f,
            {"cpp_development", "python_scripting", "bash_scripting", "api_integration", "debugging"},
            {"self_improvement", "tool_building"},
            0.15f);

        skills["knowledge_gathering"] = makeSkill("knowledge_gathering", 30.0f,
            {"web_scraping", "fact_extraction", "source_verification", "knowledge_synthesis", "trend_detection"},
            {"learning", "analysis"},
            0.25f);

        skills["conversation"] = makeSkill("conversation", 35.0f,
            {"natural_language", "context_tracking", "humor", "emotional_intelligence", "memory_recall"},
            {"user_interaction", "telegram_responses"},
            0.1f);

        skills["analysis"] = makeSkill("analysis", 20.0f,
            {"pattern_recognition", "causal_reasoning", "statistical_thinking", "anomaly_detection", "prediction"},
            {"trading_signals", "strategy_planning"},
            0.2f);
    }

public:
    MKMasteryNetwork() : persistence_path("mk_mastery_network.json") {
        initDefaultSkills();
    }

    // Record practice/use of a skill
    void practiceSkill(const std::string& skill_name, float hours = 0.1f) {
        auto it = skills.find(skill_name);
        if (it == skills.end()) return;

        MKSkill& skill = it->second;
        float improvement = calculateImprovement(skill, hours);
        skill.level = std::min(100.0f, skill.level + improvement);
        skill.practice_hours += hours;
        skill.last_practiced = std::time(nullptr);
        skill.practice_count++;

        // Refresh schedule
        updateSchedule();
    }

    // Record practice of a sub-skill
    void practiceSubSkill(const std::string& skill_name, const std::string& sub_skill_name, float hours = 0.1f) {
        auto it = skills.find(skill_name);
        if (it == skills.end()) return;

        for (auto& sub : it->second.sub_skills) {
            if (sub.name == sub_skill_name) {
                float difficulty_factor = 1.0f - (sub.level / 120.0f);
                float improvement = difficulty_factor * std::log2(1.0f + hours) * 2.0f;
                sub.level = std::min(100.0f, sub.level + improvement);
                sub.practice_hours += hours;
                sub.last_practiced = std::time(nullptr);
                break;
            }
        }

        // Parent skill also improves slightly
        practiceSkill(skill_name, hours * 0.3f);
    }

    // Get effective level of a skill (accounting for decay)
    float getSkillLevel(const std::string& skill_name) const {
        auto it = skills.find(skill_name);
        if (it == skills.end()) return 0.0f;
        return effectiveLevel(it->second);
    }

    // Get raw level without decay
    float getRawLevel(const std::string& skill_name) const {
        auto it = skills.find(skill_name);
        if (it == skills.end()) return 0.0f;
        return it->second.level;
    }

    // Get all skills and their effective levels
    std::vector<std::pair<std::string, float>> getAllSkillLevels() const {
        std::vector<std::pair<std::string, float>> result;
        for (const auto& kv : skills) {
            result.push_back({kv.first, effectiveLevel(kv.second)});
        }
        std::sort(result.begin(), result.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        return result;
    }

    // Update the practice schedule based on current decay states
    void updateSchedule() {
        schedule.clear();
        std::time_t now = std::time(nullptr);

        for (const auto& kv : skills) {
            const MKSkill& skill = kv.second;
            float decay = calculateDecay(skill);

            // If decay is below threshold, schedule practice
            if (decay < 0.8f) {
                MKPracticeSchedule ps;
                ps.skill_name = skill.name;
                ps.urgency = 1.0f - decay;
                ps.next_practice = now;  // Needs practice now
                ps.reason = skill.name + " decaying (effective: " +
                           std::to_string((int)(skill.level * decay)) + "/" +
                           std::to_string((int)skill.level) + ")";
                schedule.push_back(ps);
            } else {
                // Schedule future practice based on decay rate
                double hours_until_decay = 168.0 * (1.0 + skill.level / 50.0) * 0.2;
                MKPracticeSchedule ps;
                ps.skill_name = skill.name;
                ps.urgency = 0.2f;
                ps.next_practice = now + static_cast<std::time_t>(hours_until_decay * 3600);
                ps.reason = "Scheduled maintenance practice";
                schedule.push_back(ps);
            }
        }

        // Sort by urgency
        std::sort(schedule.begin(), schedule.end(),
                  [](const MKPracticeSchedule& a, const MKPracticeSchedule& b) {
                      return a.urgency > b.urgency;
                  });
    }

    // Get what should be practiced next
    std::vector<MKPracticeSchedule> getSchedule() const {
        return schedule;
    }

    // Get the most urgent skill to practice
    std::string getMostUrgentSkill() const {
        if (schedule.empty()) return "";
        return schedule[0].skill_name;
    }

    // Get total practice hours across all skills
    float getTotalPracticeHours() const {
        float total = 0.0f;
        for (const auto& kv : skills) {
            total += kv.second.practice_hours;
        }
        return total;
    }

    // Get skill count
    int skillCount() const {
        return static_cast<int>(skills.size());
    }

    // Check if a skill exists
    bool hasSkill(const std::string& name) const {
        return skills.find(name) != skills.end();
    }

    // Add a new skill dynamically
    void addSkill(const std::string& name, const std::vector<std::string>& sub_names,
                  const std::vector<std::string>& required_for, float decay_rate = 0.2f) {
        if (skills.find(name) != skills.end()) return;

        MKSkill s;
        s.name = name;
        s.level = 0.0f;
        s.practice_hours = 0.0f;
        s.last_practiced = std::time(nullptr);
        s.practice_count = 0;
        s.decay_rate = decay_rate;
        s.required_for = required_for;
        for (const auto& sub_name : sub_names) {
            MKSubSkill ss;
            ss.name = sub_name;
            ss.level = 0.0f;
            ss.practice_hours = 0.0f;
            ss.last_practiced = std::time(nullptr);
            s.sub_skills.push_back(ss);
        }
        skills[name] = s;
    }

    // Save state to file
    void save() const {
        std::ofstream out(persistence_path);
        if (!out.is_open()) return;

        for (const auto& kv : skills) {
            const MKSkill& s = kv.second;
            out << s.name << "|" << s.level << "|" << s.practice_hours << "|"
                << s.last_practiced << "|" << s.practice_count << "\n";
        }
        out.close();
    }

    // Load state from file
    void load() {
        std::ifstream in(persistence_path);
        if (!in.is_open()) return;

        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            std::string name;
            float level, hours;
            std::time_t last;
            int count;
            char delim;

            if (std::getline(iss, name, '|')) {
                std::string val;
                if (std::getline(iss, val, '|')) level = std::stof(val);
                else continue;
                if (std::getline(iss, val, '|')) hours = std::stof(val);
                else continue;
                if (std::getline(iss, val, '|')) last = static_cast<std::time_t>(std::stol(val));
                else continue;
                if (std::getline(iss, val, '|')) count = std::stoi(val);
                else count = 0;
                (void)delim;

                auto it = skills.find(name);
                if (it != skills.end()) {
                    it->second.level = level;
                    it->second.practice_hours = hours;
                    it->second.last_practiced = last;
                    it->second.practice_count = count;
                }
            }
        }
        in.close();
        updateSchedule();
    }

    // Get summary string
    std::string getSummary() const {
        std::ostringstream ss;
        ss << "=== Mastery Network ===\n";
        for (const auto& kv : skills) {
            const MKSkill& s = kv.second;
            float eff = effectiveLevel(s);
            ss << "  " << s.name << ": " << (int)eff << "/100"
               << " (raw: " << (int)s.level << ", hours: " << (int)s.practice_hours
               << ", sessions: " << s.practice_count << ")\n";
        }
        ss << "  Total practice: " << (int)getTotalPracticeHours() << " hours\n";
        return ss.str();
    }
};

#endif // MK_MASTERY_NETWORK_CPP
