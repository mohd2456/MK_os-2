#ifndef MK_AUTONOMOUS_LEARNER_CPP
#define MK_AUTONOMOUS_LEARNER_CPP

#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <sstream>
#include <random>
#include <set>

// ===================================================================================
// MK AUTONOMOUS LEARNER - Curiosity-Driven Knowledge Expansion
// ===================================================================================
// During idle periods, MK picks topics with weak knowledge, gathers new facts,
// validates them, and integrates them into the knowledge graph. This creates a
// continuous learning loop that makes MK smarter over time.
//
// The learner is curiosity-driven: it explores topics adjacent to recent queries,
// prioritizing areas where knowledge gaps exist.
// ===================================================================================

struct LearningSession {
    std::string topic;
    int facts_attempted;
    int facts_accepted;
    int facts_rejected;
    std::time_t timestamp;
    std::string source;
};

struct TopicScore {
    std::string topic;
    float weakness_score;   // 0.0 = well known, 1.0 = barely known
    float curiosity_score;  // Based on adjacency to recent queries
    float combined_score;   // weakness * 0.6 + curiosity * 0.4
};

class MKAutonomousLearner {
private:
    std::vector<LearningSession> sessions_;
    std::vector<std::string> recent_queries_;
    std::vector<std::string> explored_topics_;
    std::set<std::string> known_domains_;
    int total_facts_learned_;
    int total_facts_rejected_;
    int total_sessions_;
    bool learning_active_;
    std::string log_path_;
    std::string knowledge_dir_;

    // Topic categories for exploration
    static constexpr int NUM_DOMAINS = 15;
    const char* DOMAINS[NUM_DOMAINS] = {
        "crypto_trading", "linux_systems", "networking", "ai_ml",
        "hardware", "security", "finance", "devops",
        "physics", "biology", "history", "psychology",
        "programming", "mathematics", "practical_skills"
    };

    // Subtopics for curiosity-driven exploration
    std::vector<std::string> getSubtopics(const std::string& domain) {
        if (domain == "crypto_trading") {
            return {"defi_protocols", "trading_strategies", "market_analysis",
                    "blockchain_technology", "tokenomics", "yield_farming"};
        } else if (domain == "ai_ml") {
            return {"neural_networks", "nlp", "computer_vision",
                    "reinforcement_learning", "transformers", "embeddings"};
        } else if (domain == "linux_systems") {
            return {"kernel_modules", "systemd", "containers",
                    "filesystems", "shell_scripting", "security"};
        } else if (domain == "networking") {
            return {"tcp_ip", "dns", "http", "vpn", "routing", "load_balancing"};
        } else if (domain == "security") {
            return {"encryption", "penetration_testing", "web_security",
                    "network_defense", "authentication", "zero_trust"};
        }
        return {"general", "basics", "advanced", "practical", "theory"};
    }

    // Calculate weakness score for a topic (simulated - in real system would query graph)
    float calculateWeaknessScore(const std::string& topic) {
        // If topic was explored recently, lower weakness score
        for (const auto& explored : explored_topics_) {
            if (explored == topic) return 0.2f;
        }
        // New topics have high weakness
        return 0.8f;
    }

    // Calculate curiosity score based on adjacency to recent queries
    float calculateCuriosityScore(const std::string& topic) {
        float score = 0.3f; // Base curiosity
        for (const auto& query : recent_queries_) {
            std::string lower_query = query;
            std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
            std::string lower_topic = topic;
            std::transform(lower_topic.begin(), lower_topic.end(), lower_topic.begin(), ::tolower);

            // If recent query mentions this topic or related terms
            if (lower_query.find(lower_topic) != std::string::npos ||
                lower_topic.find(lower_query) != std::string::npos) {
                score += 0.3f;
            }
        }
        return std::min(1.0f, score);
    }

public:
    MKAutonomousLearner()
        : total_facts_learned_(0),
          total_facts_rejected_(0),
          total_sessions_(0),
          learning_active_(false),
          log_path_("learning_sessions.log"),
          knowledge_dir_("ai_core/hre/knowledge_files") {}

    // Record a user query for curiosity tracking
    void recordQuery(const std::string& query) {
        recent_queries_.push_back(query);
        // Keep only last 20 queries
        if (recent_queries_.size() > 20) {
            recent_queries_.erase(recent_queries_.begin());
        }
    }

    // Pick the best topic to learn about next
    TopicScore pickNextTopic() {
        std::vector<TopicScore> candidates;

        for (int i = 0; i < NUM_DOMAINS; i++) {
            std::string domain(DOMAINS[i]);
            auto subtopics = getSubtopics(domain);

            for (const auto& sub : subtopics) {
                TopicScore ts;
                ts.topic = sub;
                ts.weakness_score = calculateWeaknessScore(sub);
                ts.curiosity_score = calculateCuriosityScore(sub);
                ts.combined_score = ts.weakness_score * 0.6f + ts.curiosity_score * 0.4f;
                candidates.push_back(ts);
            }
        }

        // Sort by combined score (highest first)
        std::sort(candidates.begin(), candidates.end(),
            [](const TopicScore& a, const TopicScore& b) {
                return a.combined_score > b.combined_score;
            });

        // Add some randomness - pick from top 5
        if (candidates.empty()) {
            return {"general", 0.5f, 0.5f, 0.5f};
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        int top_n = std::min(5, (int)candidates.size());
        std::uniform_int_distribution<> dis(0, top_n - 1);
        return candidates[dis(gen)];
    }

    // Start a learning session for a topic
    LearningSession startSession(const std::string& topic) {
        learning_active_ = true;
        LearningSession session;
        session.topic = topic;
        session.facts_attempted = 0;
        session.facts_accepted = 0;
        session.facts_rejected = 0;
        session.timestamp = std::time(nullptr);
        session.source = "autonomous_exploration";
        return session;
    }

    // Record that a fact was validated and accepted
    void factAccepted(LearningSession& session) {
        session.facts_accepted++;
        session.facts_attempted++;
        total_facts_learned_++;
    }

    // Record that a fact was rejected
    void factRejected(LearningSession& session) {
        session.facts_rejected++;
        session.facts_attempted++;
        total_facts_rejected_++;
    }

    // End a learning session
    void endSession(LearningSession& session) {
        sessions_.push_back(session);
        explored_topics_.push_back(session.topic);
        total_sessions_++;
        learning_active_ = false;

        // Keep explored topics manageable
        if (explored_topics_.size() > 100) {
            explored_topics_.erase(explored_topics_.begin(),
                                   explored_topics_.begin() + 50);
        }

        // Log the session
        logSession(session);
    }

    // Generate a Python scraper command for a topic
    std::string generateScraperCommand(const std::string& topic) {
        return "python3 tools/web_scraper.py --topic \"" + topic +
               "\" --format mk --max-facts 50";
    }

    // Parse facts from scraper output (format: source|relation|target|weight per line)
    std::vector<std::string> parseScraperOutput(const std::string& output) {
        std::vector<std::string> facts;
        std::istringstream stream(output);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.empty() || line[0] == '#') continue;
            // Basic validation: must have at least 3 pipe separators
            int pipes = 0;
            for (char c : line) {
                if (c == '|') pipes++;
            }
            if (pipes >= 2) {
                facts.push_back(line);
            }
        }
        return facts;
    }

    // Check if it's a good time to learn (system is idle)
    bool shouldLearn(float cpu_usage, float memory_usage) {
        // Don't learn if system is busy
        if (cpu_usage > 70.0f) return false;
        if (memory_usage > 85.0f) return false;
        if (learning_active_) return false;
        return true;
    }

    // Get statistics about learning activity
    std::string getStats() const {
        std::ostringstream ss;
        ss << "Learning Sessions: " << total_sessions_
           << " | Facts Learned: " << total_facts_learned_
           << " | Rejected: " << total_facts_rejected_
           << " | Acceptance Rate: ";
        int total = total_facts_learned_ + total_facts_rejected_;
        if (total > 0) {
            ss << (int)(100.0f * total_facts_learned_ / total) << "%";
        } else {
            ss << "N/A";
        }
        return ss.str();
    }

    // Get recent learning history
    std::vector<LearningSession> getRecentSessions(int count = 5) const {
        std::vector<LearningSession> recent;
        int start = std::max(0, (int)sessions_.size() - count);
        for (int i = start; i < (int)sessions_.size(); i++) {
            recent.push_back(sessions_[i]);
        }
        return recent;
    }

    bool isActive() const { return learning_active_; }
    int totalFactsLearned() const { return total_facts_learned_; }
    int totalSessions() const { return total_sessions_; }

private:
    void logSession(const LearningSession& session) {
        std::ofstream log(log_path_, std::ios::app);
        if (log.is_open()) {
            log << "[" << session.timestamp << "] "
                << "Topic: " << session.topic
                << " | Attempted: " << session.facts_attempted
                << " | Accepted: " << session.facts_accepted
                << " | Rejected: " << session.facts_rejected
                << " | Source: " << session.source
                << "\n";
            log.close();
        }
    }
};

#endif // MK_AUTONOMOUS_LEARNER_CPP
