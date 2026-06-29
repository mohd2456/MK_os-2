#ifndef MK_PROMETHEUS_FLUID_DYNAMICS_CPP
#define MK_PROMETHEUS_FLUID_DYNAMICS_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <random>
#include <chrono>
#include <sstream>
#include <cmath>

// ===================================================================================
// MK PROMETHEUS — FLUID DYNAMICS
// ===================================================================================
// The four forces that act on droplets:
//   1. RESONANCE — what wants to be said (activation)
//   2. COHERENCE — what makes sense together (grouping)
//   3. IDENTITY  — who is speaking (personality filter)
//   4. EVOLUTION — what survives (fitness/selection)
// ===================================================================================

// Identity state: who MK "is" right now (evolves over time)
struct MKIdentityState {
    float precision_preference;  // 0=casual, 1=technical
    float verbosity;             // 0=terse, 1=verbose
    float tone;                  // 0=serious, 1=playful
    float energy;                // 0=calm, 1=hyped
    float user_skill_level;      // 0=beginner, 1=expert
    std::string code_style;      // "commented", "minimal", "verbose"

    MKIdentityState()
        : precision_preference(0.5f), verbosity(0.6f), tone(0.7f),
          energy(0.6f), user_skill_level(0.5f), code_style("commented") {}

    std::string toString() const {
        std::ostringstream ss;
        ss << "Identity { precision=" << precision_preference
           << ", verbosity=" << verbosity
           << ", tone=" << tone
           << ", energy=" << energy
           << ", user_skill=" << user_skill_level
           << ", code_style=" << code_style << " }";
        return ss.str();
    }
};

class MKFluidDynamics {
private:
    std::mt19937 rng_;
    std::vector<std::pair<int, float>> lastResonance_;  // Last resonance trace for debug

    // Tokenize input into lowercase words
    std::vector<std::string> tokenize(const std::string& input) const {
        std::vector<std::string> tokens;
        std::istringstream ss(input);
        std::string word;
        while (ss >> word) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            // Strip punctuation
            while (!word.empty() && !std::isalnum(word.back())) word.pop_back();
            while (!word.empty() && !std::isalnum(word.front())) word.erase(word.begin());
            if (!word.empty()) tokens.push_back(word);
        }
        return tokens;
    }

    long long now() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

public:
    MKFluidDynamics() {
        auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        rng_ = std::mt19937(static_cast<unsigned>(seed));
    }

    // =======================================================================
    // FORCE 1: RESONANCE — What wants to be said
    // Find droplets that vibrate in response to input
    // Returns: vector of (droplet_id, vibration_strength) sorted by strength
    // =======================================================================
    std::vector<std::pair<int, float>> resonate(const std::string& input, MKDropletPool& pool) {
        std::vector<std::pair<int, float>> vibrating;
        auto tokens = tokenize(input);
        if (tokens.empty()) return vibrating;

        std::unordered_map<int, float> scores;

        // Score each droplet based on trigger overlap
        for (const auto& token : tokens) {
            auto matches = pool.findByTrigger(token);
            for (int id : matches) {
                if (!pool.get(id).isAlive()) continue;
                const auto& d = pool.get(id);

                float triggerScore = 1.0f / (float)d.triggers.size();  // Specificity bonus
                float fitnessBonus = d.fitness * 0.3f;
                float recencyBonus = 0.0f;
                long long elapsed = now() - d.last_alive;
                if (elapsed < 60000) recencyBonus = 0.2f;  // Active in last minute
                else if (elapsed < 300000) recencyBonus = 0.1f;  // Active in last 5 min

                float vitalityFactor = d.vitality;
                float resonanceBonus = d.user_resonance * 0.2f;

                float score = (triggerScore + fitnessBonus + recencyBonus + resonanceBonus)
                              * vitalityFactor;
                scores[id] += score;
            }
        }

        // Domain matching bonus: if input mentions a known domain
        std::vector<std::string> domainKeywords = {
            "python", "cpp", "c++", "bash", "javascript", "code", "program",
            "function", "class", "loop", "file", "sort", "search", "network",
            "error", "bug", "fix", "help", "explain", "how"
        };
        std::string domain;
        for (const auto& token : tokens) {
            for (const auto& dk : domainKeywords) {
                if (token == dk) { domain = dk; break; }
            }
            if (!domain.empty()) break;
        }
        if (!domain.empty()) {
            auto domainDroplets = pool.findByDomain(domain);
            for (int id : domainDroplets) {
                if (pool.get(id).isAlive()) {
                    scores[id] += 0.3f;
                }
            }
        }

        // Convert to sorted vector
        for (const auto& [id, score] : scores) {
            vibrating.push_back({id, score});
        }
        std::sort(vibrating.begin(), vibrating.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        // Keep top 30
        if (vibrating.size() > 30) vibrating.resize(30);

        // Update last_alive for vibrating droplets
        for (const auto& [id, score] : vibrating) {
            pool.getMut(id).last_alive = now();
        }

        lastResonance_ = vibrating;
        return vibrating;
    }

    // =======================================================================
    // FORCE 2: COHERENCE — What makes sense together
    // Group vibrating droplets into clusters of bonded/related droplets
    // Returns: clusters ordered by precision (emotion → logic → code)
    // =======================================================================
    std::vector<std::vector<int>> cohere(const std::vector<std::pair<int, float>>& vibrating,
                                         MKDropletPool& pool) {
        if (vibrating.empty()) return {};

        // Build adjacency from bonds
        std::unordered_set<int> vibratingSet;
        for (const auto& [id, _] : vibrating) vibratingSet.insert(id);

        // Group by bonds (union-find style, simplified)
        std::unordered_map<int, int> parent;
        for (const auto& [id, _] : vibrating) parent[id] = id;

        auto find = [&](int x) -> int {
            while (parent[x] != x) { parent[x] = parent[parent[x]]; x = parent[x]; }
            return x;
        };
        auto unite = [&](int a, int b) {
            int ra = find(a), rb = find(b);
            if (ra != rb) parent[ra] = rb;
        };

        // Unite droplets that share bonds
        for (const auto& [id, _] : vibrating) {
            const auto& d = pool.get(id);
            for (int bondId : d.bonds) {
                if (vibratingSet.count(bondId)) {
                    unite(id, bondId);
                }
            }
            // Also unite by same domain
            for (const auto& [otherId, __] : vibrating) {
                if (otherId != id && pool.get(otherId).domain == d.domain &&
                    std::abs(pool.get(otherId).precision - d.precision) < 0.3f) {
                    unite(id, otherId);
                }
            }
        }

        // Collect clusters
        std::unordered_map<int, std::vector<int>> clusterMap;
        for (const auto& [id, _] : vibrating) {
            clusterMap[find(id)].push_back(id);
        }

        std::vector<std::vector<int>> clusters;
        for (auto& [root, members] : clusterMap) {
            clusters.push_back(std::move(members));
        }

        // Sort clusters by average precision (emotion first → logic → code)
        std::sort(clusters.begin(), clusters.end(),
                  [&pool](const std::vector<int>& a, const std::vector<int>& b) {
            float avgA = 0, avgB = 0;
            for (int id : a) avgA += pool.get(id).precision;
            for (int id : b) avgB += pool.get(id).precision;
            avgA /= (float)a.size();
            avgB /= (float)b.size();
            return avgA < avgB;
        });

        // Sort within each cluster by fitness
        for (auto& cluster : clusters) {
            std::sort(cluster.begin(), cluster.end(),
                      [&pool](int a, int b) {
                return pool.get(a).fitness > pool.get(b).fitness;
            });
        }

        return clusters;
    }

    // =======================================================================
    // FORCE 3: IDENTITY — Who is speaking
    // Apply personality/identity to filter and adjust cluster selection
    // =======================================================================
    std::vector<std::vector<int>> applyIdentity(const std::vector<std::vector<int>>& clusters,
                                                 const MKIdentityState& identity,
                                                 MKDropletPool& pool) {
        if (clusters.empty()) return clusters;

        std::vector<std::vector<int>> filtered;

        // Determine how many clusters to keep based on verbosity
        int maxClusters = (int)(clusters.size() * (0.3f + identity.verbosity * 0.7f));
        maxClusters = std::max(1, maxClusters);

        for (int i = 0; i < (int)clusters.size() && i < maxClusters; i++) {
            const auto& cluster = clusters[i];
            std::vector<int> kept;

            for (int id : cluster) {
                const auto& d = pool.get(id);

                // If user is beginner and this is code: keep explanations
                if (identity.user_skill_level < 0.3f && d.isCode()) {
                    // Only keep if code has high fitness (proven patterns)
                    if (d.fitness > 0.6f) kept.push_back(id);
                    continue;
                }

                // If user is advanced and this is basic explanation: skip
                if (identity.user_skill_level > 0.7f && d.isEmotion() && d.fitness < 0.5f) {
                    continue;
                }

                // If terse mode: only keep high-fitness droplets
                if (identity.verbosity < 0.3f && d.fitness < 0.4f) {
                    continue;
                }

                kept.push_back(id);
            }

            // Limit cluster size based on verbosity
            int maxPerCluster = 2 + (int)(identity.verbosity * 5);
            if ((int)kept.size() > maxPerCluster) kept.resize(maxPerCluster);

            if (!kept.empty()) filtered.push_back(kept);
        }

        return filtered;
    }

    // =======================================================================
    // FORCE 4: EVOLUTION — What survives
    // Update fitness, trigger mutations and reproduction
    // =======================================================================
    void evolve(const std::vector<int>& usedDroplets, bool userContinued, MKDropletPool& pool) {
        // Reward or punish used droplets based on user reaction
        for (int id : usedDroplets) {
            if (id < 0 || id >= pool.size()) continue;
            auto& d = pool.getMut(id);

            if (userContinued) {
                // User engaged → boost fitness
                d.fitness = std::min(1.0f, d.fitness + 0.05f);
                d.user_resonance = std::min(1.0f, d.user_resonance + 0.1f);
                d.vitality = std::min(1.0f, d.vitality + 0.1f);
            } else {
                // User changed topic → reduce fitness
                d.fitness = std::max(0.0f, d.fitness - 0.03f);
                d.user_resonance = std::max(0.0f, d.user_resonance - 0.05f);
                d.temperature += 0.1f;  // Gets hotter (more likely to mutate)
            }
        }

        // Temperature-based mutations on random droplets
        auto alive = pool.getAlive();
        if (alive.size() > 10) {
            std::uniform_int_distribution<int> dist(0, (int)alive.size() - 1);
            int mutationCount = std::max(1, (int)(alive.size() / 100));  // 1% mutation rate

            for (int i = 0; i < mutationCount; i++) {
                int idx = alive[dist(rng_)];
                const auto& d = pool.get(idx);
                // Hot droplets mutate more easily
                if (d.temperature > 0.6f) {
                    pool.mutate(idx);
                }
            }
        }

        // Reproduce high-fitness neighbors
        if (alive.size() > 20) {
            std::vector<int> fitDroplets;
            for (int id : alive) {
                if (pool.get(id).fitness > 0.7f) fitDroplets.push_back(id);
            }
            if (fitDroplets.size() >= 2) {
                std::uniform_int_distribution<int> dist(0, (int)fitDroplets.size() - 1);
                int reproCount = std::max(1, (int)(fitDroplets.size() / 20));
                for (int i = 0; i < reproCount; i++) {
                    int a = fitDroplets[dist(rng_)];
                    int b = fitDroplets[dist(rng_)];
                    if (a != b) pool.reproduce(a, b);
                }
            }
        }

        // Kill low-vitality droplets (natural selection)
        for (int id : alive) {
            const auto& d = pool.get(id);
            if (d.vitality < 0.1f && d.fitness < 0.2f) {
                pool.killDroplet(id);
            }
        }
    }

    // =======================================================================
    // UTILITY: Detect if code generation is needed
    // Returns true if high-precision droplets dominate the resonance
    // =======================================================================
    bool isCodeNeeded(const std::vector<std::pair<int, float>>& vibrating,
                      MKDropletPool& pool) const {
        if (vibrating.empty()) return false;

        float codeWeight = 0.0f;
        float totalWeight = 0.0f;

        for (const auto& [id, strength] : vibrating) {
            totalWeight += strength;
            if (pool.get(id).isCode()) {
                codeWeight += strength;
            }
        }

        if (totalWeight <= 0.0f) return false;
        return (codeWeight / totalWeight) > 0.4f;
    }

    // =======================================================================
    // DEBUG: Get last resonance trace as string
    // =======================================================================
    std::string getResonanceTrace(MKDropletPool& pool) const {
        if (lastResonance_.empty()) return "No resonance trace yet.";

        std::ostringstream ss;
        ss << "Last resonance (" << lastResonance_.size() << " vibrating):\n";
        int shown = 0;
        for (const auto& [id, strength] : lastResonance_) {
            if (shown >= 10) { ss << "  ... and " << (lastResonance_.size() - 10) << " more\n"; break; }
            const auto& d = pool.get(id);
            ss << "  [" << id << "] strength=" << strength
               << " precision=" << d.precision
               << " fitness=" << d.fitness
               << " domain=" << d.domain
               << " content=\"" << d.content.substr(0, 40) << "...\"\n";
            shown++;
        }
        return ss.str();
    }
};

#endif // MK_PROMETHEUS_FLUID_DYNAMICS_CPP
