#ifndef MK_PROMETHEUS_DROPLET_CPP
#define MK_PROMETHEUS_DROPLET_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cmath>
#include "../safe_parse.h"

// ===================================================================================
// MK PROMETHEUS — DROPLET
// ===================================================================================
// The fundamental unit of the Prometheus fluid. A droplet can be language OR code —
// just at different precision points.
//   precision 0.0 = pure emotion/vibe
//   precision 0.5 = structured logic/reasoning
//   precision 1.0 = executable code
//
// Droplets are alive. They have vitality, temperature, bonds, and fitness.
// They evolve, reproduce, mutate, and die.
// ===================================================================================

struct MKDroplet {
    std::string content;         // The actual text/code
    float precision;             // 0.0=emotion, 0.5=logic, 1.0=code
    float vitality;              // How alive/active (0=dead, 1=thriving)
    float temperature;           // Hot=creative/mutating, cold=stable/proven
    std::vector<int> bonds;      // Indexes of bonded droplets
    std::vector<std::string> triggers;  // What activates this droplet
    std::string domain;          // Topic/field (e.g., "python", "greeting", "file_ops")
    std::string dna;             // Structural skeleton/pattern
    int generation;              // Which generation this droplet belongs to
    float fitness;               // Survival score (higher = more useful)
    float user_resonance;        // How much the user vibes with this
    long long born_at;           // Timestamp of creation
    long long last_alive;        // Last time this was activated
    int offspring_count;         // How many children spawned from this
    int kill_count;              // How many times this outcompeted others

    MKDroplet()
        : precision(0.5f), vitality(1.0f), temperature(0.5f),
          generation(0), fitness(0.5f), user_resonance(0.0f),
          born_at(0), last_alive(0), offspring_count(0), kill_count(0) {}

    bool isAlive() const { return vitality > 0.01f; }
    bool isEmotion() const { return precision < 0.3f; }
    bool isLogic() const { return precision >= 0.3f && precision < 0.7f; }
    bool isCode() const { return precision >= 0.7f; }
};

// ===================================================================================
// MKDropletPool — The living ocean of all droplets
// ===================================================================================
class MKDropletPool {
private:
    std::vector<MKDroplet> droplets_;
    std::unordered_map<std::string, std::vector<int>> triggerIndex_;  // trigger → droplet ids
    std::vector<int> emotionDroplets_;   // precision 0.0 - 0.3
    std::vector<int> logicDroplets_;     // precision 0.3 - 0.7
    std::vector<int> codeDroplets_;      // precision 0.7 - 1.0
    std::mt19937 rng_;
    int totalKilled_ = 0;
    int totalBorn_ = 0;

    long long now() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void indexDroplet(int id) {
        const auto& d = droplets_[id];
        for (const auto& t : d.triggers) {
            triggerIndex_[t].push_back(id);
        }
        if (d.isEmotion()) emotionDroplets_.push_back(id);
        else if (d.isLogic()) logicDroplets_.push_back(id);
        else codeDroplets_.push_back(id);
    }

public:
    MKDropletPool() {
        auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        rng_ = std::mt19937(static_cast<unsigned>(seed));
    }

    int addDroplet(const std::string& content, float precision,
                   const std::vector<std::string>& triggers, const std::string& domain) {
        MKDroplet d;
        d.content = content;
        d.precision = std::max(0.0f, std::min(1.0f, precision));
        d.vitality = 1.0f;
        d.temperature = 0.5f;
        d.triggers = triggers;
        d.domain = domain;
        d.dna = content.substr(0, std::min((size_t)30, content.size()));
        d.generation = 0;
        d.fitness = 0.5f;
        d.user_resonance = 0.0f;
        d.born_at = now();
        d.last_alive = d.born_at;
        d.offspring_count = 0;
        d.kill_count = 0;

        int id = (int)droplets_.size();
        droplets_.push_back(d);
        indexDroplet(id);
        totalBorn_++;
        return id;
    }

    std::vector<int> findByTrigger(const std::string& word) const {
        auto it = triggerIndex_.find(word);
        if (it != triggerIndex_.end()) return it->second;
        return {};
    }

    std::vector<int> findByPrecision(float minP, float maxP) const {
        std::vector<int> result;
        for (int i = 0; i < (int)droplets_.size(); i++) {
            if (droplets_[i].isAlive() &&
                droplets_[i].precision >= minP && droplets_[i].precision <= maxP) {
                result.push_back(i);
            }
        }
        return result;
    }

    std::vector<int> findByDomain(const std::string& domain) const {
        std::vector<int> result;
        for (int i = 0; i < (int)droplets_.size(); i++) {
            if (droplets_[i].isAlive() && droplets_[i].domain == domain) {
                result.push_back(i);
            }
        }
        return result;
    }

    void killDroplet(int id) {
        if (id >= 0 && id < (int)droplets_.size()) {
            droplets_[id].vitality = 0.0f;
            totalKilled_++;
        }
    }

    std::vector<int> getAlive() const {
        std::vector<int> result;
        for (int i = 0; i < (int)droplets_.size(); i++) {
            if (droplets_[i].isAlive()) result.push_back(i);
        }
        return result;
    }

    void age() {
        for (auto& d : droplets_) {
            if (!d.isAlive()) continue;
            long long elapsed = now() - d.last_alive;
            float decay = 0.001f * (elapsed / 60000.0f);  // Slow decay per minute unused
            d.vitality -= decay;
            if (d.vitality <= 0.0f) {
                d.vitality = 0.0f;
                totalKilled_++;
            }
        }
    }

    int mutate(int id) {
        if (id < 0 || id >= (int)droplets_.size() || !droplets_[id].isAlive()) return -1;
        const auto& parent = droplets_[id];

        std::string mutatedContent = parent.content;
        // Simple mutation: swap a random word or add variation marker
        if (mutatedContent.size() > 10) {
            std::uniform_int_distribution<int> dist(0, (int)mutatedContent.size() - 5);
            int pos = dist(rng_);
            mutatedContent.insert(pos, "~");  // Mark mutation point
        }

        float newPrecision = parent.precision + ((float)(rng_() % 100 - 50) / 500.0f);
        newPrecision = std::max(0.0f, std::min(1.0f, newPrecision));

        int childId = addDroplet(mutatedContent, newPrecision, parent.triggers, parent.domain);
        droplets_[childId].generation = parent.generation + 1;
        droplets_[childId].temperature = parent.temperature + 0.1f;
        droplets_[childId].bonds.push_back(id);
        droplets_[id].offspring_count++;
        return childId;
    }

    int reproduce(int id1, int id2) {
        if (id1 < 0 || id1 >= (int)droplets_.size()) return -1;
        if (id2 < 0 || id2 >= (int)droplets_.size()) return -1;
        if (!droplets_[id1].isAlive() || !droplets_[id2].isAlive()) return -1;

        const auto& p1 = droplets_[id1];
        const auto& p2 = droplets_[id2];

        // Combine: first half of p1 content + second half of p2 content
        std::string childContent;
        size_t mid1 = p1.content.size() / 2;
        size_t mid2 = p2.content.size() / 2;
        childContent = p1.content.substr(0, mid1) + " " + p2.content.substr(mid2);

        // Average precision, combine triggers
        float childPrecision = (p1.precision + p2.precision) / 2.0f;
        std::vector<std::string> childTriggers = p1.triggers;
        for (const auto& t : p2.triggers) {
            if (std::find(childTriggers.begin(), childTriggers.end(), t) == childTriggers.end()) {
                childTriggers.push_back(t);
            }
        }
        std::string childDomain = p1.fitness >= p2.fitness ? p1.domain : p2.domain;

        int childId = addDroplet(childContent, childPrecision, childTriggers, childDomain);
        droplets_[childId].generation = std::max(p1.generation, p2.generation) + 1;
        droplets_[childId].bonds.push_back(id1);
        droplets_[childId].bonds.push_back(id2);
        droplets_[id1].offspring_count++;
        droplets_[id2].offspring_count++;
        return childId;
    }

    void save(const std::string& path) const {
        std::ofstream f(path);
        if (!f.is_open()) return;
        f << droplets_.size() << "\n";
        for (const auto& d : droplets_) {
            f << d.precision << "|" << d.vitality << "|" << d.temperature << "|"
              << d.generation << "|" << d.fitness << "|" << d.user_resonance << "|"
              << d.born_at << "|" << d.last_alive << "|" << d.offspring_count << "|"
              << d.kill_count << "|" << d.domain << "|" << d.content << "\n";
        }
        f.close();
    }

    bool load(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) return false;
        int count = 0;
        f >> count;
        f.ignore();
        for (int i = 0; i < count; i++) {
            std::string line;
            if (!std::getline(f, line)) break;
            // Parse back (simplified — real implementation would be more robust)
            // Parse defensively — a malformed/partial line must never crash the
            // loader. Bad numeric tokens fall back to the field's default value.
            MKDroplet d;
            std::istringstream ss(line);
            std::string token;
            if (std::getline(ss, token, '|')) d.precision = mk::safeStof(token, d.precision);
            if (std::getline(ss, token, '|')) d.vitality = mk::safeStof(token, d.vitality);
            if (std::getline(ss, token, '|')) d.temperature = mk::safeStof(token, d.temperature);
            if (std::getline(ss, token, '|')) d.generation = mk::safeStoi(token, d.generation);
            if (std::getline(ss, token, '|')) d.fitness = mk::safeStof(token, d.fitness);
            if (std::getline(ss, token, '|')) d.user_resonance = mk::safeStof(token, d.user_resonance);
            if (std::getline(ss, token, '|')) d.born_at = mk::safeStoll(token, d.born_at);
            if (std::getline(ss, token, '|')) d.last_alive = mk::safeStoll(token, d.last_alive);
            if (std::getline(ss, token, '|')) d.offspring_count = mk::safeStoi(token, d.offspring_count);
            if (std::getline(ss, token, '|')) d.kill_count = mk::safeStoi(token, d.kill_count);
            if (std::getline(ss, token, '|')) d.domain = token;
            if (std::getline(ss, token)) d.content = token;
            int id = (int)droplets_.size();
            droplets_.push_back(d);
            indexDroplet(id);
        }
        f.close();
        return true;
    }

    void bootstrap(const std::vector<std::string>& casualResponses,
                   const std::vector<std::string>& knowledgeFacts,
                   const std::vector<std::string>& codeFragments) {
        // Add casual responses as emotion-level droplets (precision 0.1-0.3)
        for (const auto& r : casualResponses) {
            std::vector<std::string> triggers;
            // Extract first 2 words as triggers
            std::istringstream ss(r);
            std::string word;
            int wc = 0;
            while (ss >> word && wc < 2) {
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                triggers.push_back(word);
                wc++;
            }
            float p = 0.1f + (float)(rng_() % 20) / 100.0f;
            addDroplet(r, p, triggers, "casual");
        }

        // Add knowledge facts as logic-level droplets (precision 0.4-0.6)
        for (const auto& fact : knowledgeFacts) {
            std::vector<std::string> triggers;
            std::istringstream ss(fact);
            std::string word;
            int wc = 0;
            while (ss >> word && wc < 3) {
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                triggers.push_back(word);
                wc++;
            }
            float p = 0.4f + (float)(rng_() % 20) / 100.0f;
            addDroplet(fact, p, triggers, "knowledge");
        }

        // Add code fragments as code-level droplets (precision 0.8-1.0)
        for (const auto& code : codeFragments) {
            std::vector<std::string> triggers;
            std::istringstream ss(code);
            std::string word;
            int wc = 0;
            while (ss >> word && wc < 3) {
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                triggers.push_back(word);
                wc++;
            }
            float p = 0.8f + (float)(rng_() % 20) / 100.0f;
            p = std::min(1.0f, p);
            addDroplet(code, p, triggers, "code");
        }

        std::cout << "[PROMETHEUS] DropletPool bootstrapped: " << droplets_.size()
                  << " droplets (" << emotionDroplets_.size() << " emotion, "
                  << logicDroplets_.size() << " logic, "
                  << codeDroplets_.size() << " code)\n";
    }

    // Accessors
    const MKDroplet& get(int id) const { return droplets_[id]; }
    MKDroplet& getMut(int id) { return droplets_[id]; }
    int size() const { return (int)droplets_.size(); }
    int aliveCount() const {
        int c = 0;
        for (const auto& d : droplets_) if (d.isAlive()) c++;
        return c;
    }
    int getTotalBorn() const { return totalBorn_; }
    int getTotalKilled() const { return totalKilled_; }
    int maxGeneration() const {
        int m = 0;
        for (const auto& d : droplets_) m = std::max(m, d.generation);
        return m;
    }
    const std::vector<int>& getEmotionIds() const { return emotionDroplets_; }
    const std::vector<int>& getLogicIds() const { return logicDroplets_; }
    const std::vector<int>& getCodeIds() const { return codeDroplets_; }
};

#endif // MK_PROMETHEUS_DROPLET_CPP
