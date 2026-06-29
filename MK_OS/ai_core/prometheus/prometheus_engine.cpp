#ifndef MK_PROMETHEUS_ENGINE_CPP
#define MK_PROMETHEUS_ENGINE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <fstream>
#include <cstdlib>

#include "droplet.cpp"
#include "fluid_dynamics.cpp"
#include "birth_chamber.cpp"
#include "code_fragments.cpp"

// ===================================================================================
// MK PROMETHEUS ENGINE — Master Class
// ===================================================================================
// ONE API. Handles EVERYTHING.
// Unified fluid intelligence: conversation AND code flow through the same system.
// The precision spectrum determines what comes out — not separate code paths.
//
// Fallback chain: Prometheus → CXN → MCE → Templates
// ===================================================================================

class MKPrometheus {
private:
    MKDropletPool pool_;
    MKFluidDynamics dynamics_;
    MKBirthChamber chamber_;
    MKIdentityState identity_;
    bool initialized_ = false;
    std::string dataDir_ = "prometheus_data";
    int totalGenerations_ = 0;
    int conversationCount_ = 0;
    int codeCount_ = 0;
    std::vector<int> lastUsedDroplets_;   // Track for evolution
    std::string lastTrace_;               // Last thought trace

    long long now() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // Extract goal description from logic-level droplets
    std::string extractGoal(const std::vector<std::pair<int, float>>& vibrating) const {
        std::string goal;
        for (const auto& [id, strength] : vibrating) {
            const auto& d = pool_.get(id);
            if (d.isLogic()) {
                goal += d.content + " ";
                if (goal.size() > 200) break;
            }
        }
        return goal;
    }

    // Gather code fragment IDs from high-precision vibrating droplets
    std::vector<int> gatherCodeDroplets(const std::vector<std::pair<int, float>>& vibrating) const {
        std::vector<int> codeIds;
        for (const auto& [id, strength] : vibrating) {
            if (pool_.get(id).isCode()) {
                codeIds.push_back(id);
            }
        }
        return codeIds;
    }

    // Gather goal droplet IDs from logic-precision vibrating droplets
    std::vector<int> gatherGoalDroplets(const std::vector<std::pair<int, float>>& vibrating) const {
        std::vector<int> goalIds;
        for (const auto& [id, strength] : vibrating) {
            if (pool_.get(id).isLogic()) {
                goalIds.push_back(id);
            }
        }
        return goalIds;
    }

    // Assemble response from clusters of droplets
    std::string assembleFromClusters(const std::vector<std::vector<int>>& clusters) const {
        std::string response;
        for (const auto& cluster : clusters) {
            for (int id : cluster) {
                const auto& d = pool_.get(id);
                if (!d.content.empty()) {
                    if (!response.empty() && response.back() != '\n' && response.back() != ' ') {
                        response += " ";
                    }
                    response += d.content;
                }
            }
        }
        return response;
    }

    // Apply temperature-based mutations to output text
    std::string temperatureAdjust(const std::string& text) const {
        // If identity is hot (high energy), allow creative variation
        if (identity_.energy < 0.7f) return text;

        // Simple temperature effect: occasionally capitalize for emphasis
        std::string result = text;
        if (result.size() > 20 && identity_.energy > 0.8f) {
            // Add an exclamation if high energy
            if (result.back() == '.') result.back() = '!';
        }
        return result;
    }

    // Build thought trace for debugging
    void buildTrace(const std::vector<std::pair<int, float>>& vibrating,
                    bool codeNeeded, const std::string& output) {
        std::ostringstream ss;
        ss << "=== Prometheus Thought Trace ===\n";
        ss << "Vibrating droplets: " << vibrating.size() << "\n";

        int emotionCount = 0, logicCount = 0, codeCount = 0;
        for (const auto& [id, strength] : vibrating) {
            if (pool_.get(id).isEmotion()) emotionCount++;
            else if (pool_.get(id).isLogic()) logicCount++;
            else codeCount++;
        }
        ss << "  Emotion: " << emotionCount << ", Logic: " << logicCount
           << ", Code: " << codeCount << "\n";
        ss << "Code generation triggered: " << (codeNeeded ? "YES" : "NO") << "\n";
        ss << "Identity: " << identity_.toString() << "\n";
        ss << "Output length: " << output.size() << " chars\n";

        if (!vibrating.empty()) {
            ss << "Top 5 resonating:\n";
            for (int i = 0; i < std::min(5, (int)vibrating.size()); i++) {
                const auto& [id, strength] = vibrating[i];
                ss << "  [" << id << "] str=" << strength
                   << " \"" << pool_.get(id).content.substr(0, 50) << "...\"\n";
            }
        }
        lastTrace_ = ss.str();
    }

public:
    MKPrometheus() = default;

    // =======================================================================
    // INITIALIZE — Bootstrap the entire Prometheus system
    // =======================================================================
    void initialize(const std::vector<std::string>& casualResponses,
                    const std::vector<std::string>& knowledgeFacts,
                    const std::vector<std::string>& codeFragments) {
        std::cout << "[PROMETHEUS] Initializing Prometheus Engine...\n";

        // Create data directory
        std::system(("mkdir -p " + dataDir_).c_str());

        // Try to load saved state first
        std::string poolPath = dataDir_ + "/pool.dat";
        std::ifstream check(poolPath);
        if (check.good()) {
            check.close();
            pool_.load(poolPath);
            std::cout << "[PROMETHEUS] Loaded saved state: " << pool_.size()
                      << " droplets (" << pool_.aliveCount() << " alive)\n";
        } else {
            // Bootstrap pool with initial droplets
            pool_.bootstrap(casualResponses, knowledgeFacts, codeFragments);
        }

        // Set identity defaults
        identity_.precision_preference = 0.5f;
        identity_.verbosity = 0.6f;
        identity_.tone = 0.7f;
        identity_.energy = 0.6f;
        identity_.user_skill_level = 0.5f;
        identity_.code_style = "commented";

        initialized_ = true;
        std::cout << "[PROMETHEUS] Engine online. Pool: " << pool_.size()
                  << " droplets | Alive: " << pool_.aliveCount()
                  << " | Generations: " << pool_.maxGeneration() << "\n";
    }

    // =======================================================================
    // GENERATE — THE main method. Handles BOTH conversation AND code.
    // =======================================================================
    std::string generate(const std::string& input, void* graph = nullptr) {
        (void)graph;  // Reserved for future knowledge graph integration
        if (!initialized_) return "";
        if (input.empty()) return "";

        // Step 1: RESONATE — find vibrating droplets
        auto vibrating = dynamics_.resonate(input, pool_);
        if (vibrating.empty()) return "";  // Nothing resonated → let fallback handle

        // Step 2: Detect if code is needed
        bool codeNeeded = dynamics_.isCodeNeeded(vibrating, pool_);

        std::string output;
        lastUsedDroplets_.clear();

        if (codeNeeded) {
            // ===== CODE PATH =====
            codeCount_++;

            // Step 3a: Extract goal from logic droplets
            auto goalIds = gatherGoalDroplets(vibrating);
            std::string goalDesc = extractGoal(vibrating);

            // Step 3b: Gather code fragments from high-precision droplets
            auto codeIds = gatherCodeDroplets(vibrating);

            // Step 3c: BirthChamber conceive → candidates
            auto candidates = chamber_.conceive(goalIds, codeIds, pool_);

            if (!candidates.empty()) {
                // Step 3d: Generate test cases and run tournament
                auto tests = chamber_.generateTestCases(goalDesc);
                auto winner = chamber_.tournament(candidates, tests);

                if (winner.compiled && !winner.code.empty()) {
                    // Step 3e: Style with identity
                    output = chamber_.style(winner.code, identity_);

                    // Step 3f: Surround with explanation from low-precision droplets
                    std::string explanation;
                    for (const auto& [id, strength] : vibrating) {
                        if (pool_.get(id).isEmotion() || pool_.get(id).isLogic()) {
                            if (pool_.get(id).fitness > 0.4f) {
                                explanation += pool_.get(id).content + " ";
                                if (explanation.size() > 100) break;
                            }
                        }
                    }
                    if (!explanation.empty()) {
                        output = explanation + "\n\n```" + winner.language + "\n" + output + "\n```";
                    } else {
                        output = "```" + winner.language + "\n" + output + "\n```";
                    }

                    // Track used droplets
                    for (int id : goalIds) lastUsedDroplets_.push_back(id);
                    for (int id : codeIds) lastUsedDroplets_.push_back(id);
                }
            }

            // If code generation failed, fall through to conversation path
            if (output.empty()) {
                codeNeeded = false;
            }
        }

        if (!codeNeeded) {
            // ===== CONVERSATION PATH =====
            conversationCount_++;

            // Step 4a: COHERE — group vibrating droplets into clusters
            auto clusters = dynamics_.cohere(vibrating, pool_);

            // Step 4b: Apply IDENTITY to select/trim
            clusters = dynamics_.applyIdentity(clusters, identity_, pool_);

            // Step 4c: ASSEMBLE — stitch cluster contents in coherence order
            output = assembleFromClusters(clusters);

            // Step 4d: Temperature adjust — if hot, allow mutations
            output = temperatureAdjust(output);

            // Track used droplets
            for (const auto& cluster : clusters) {
                for (int id : cluster) lastUsedDroplets_.push_back(id);
            }
        }

        // Build trace for debugging
        buildTrace(vibrating, codeNeeded, output);
        totalGenerations_++;

        return output;
    }

    // =======================================================================
    // ABSORB — Learn from interaction (feed evolution)
    // =======================================================================
    void absorb(const std::string& input, const std::string& response, bool userContinued) {
        if (!initialized_) return;

        // Feed input as new droplets (medium precision — they're user thought)
        if (!input.empty() && input.size() > 5) {
            std::vector<std::string> triggers;
            std::istringstream ss(input);
            std::string word;
            int wc = 0;
            while (ss >> word && wc < 3) {
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                triggers.push_back(word);
                wc++;
            }
            pool_.addDroplet(input, 0.4f, triggers, "user_input");
        }

        // Feed response as new droplets (varies by content)
        if (!response.empty() && response.size() > 10) {
            float precision = 0.3f;
            if (response.find("```") != std::string::npos) precision = 0.8f;
            else if (response.find("def ") != std::string::npos) precision = 0.7f;

            std::vector<std::string> triggers;
            std::istringstream ss(response);
            std::string word;
            int wc = 0;
            while (ss >> word && wc < 2) {
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                triggers.push_back(word);
                wc++;
            }
            pool_.addDroplet(response.substr(0, 200), precision, triggers, "generated");
        }

        // Run EVOLUTION force on used droplets
        dynamics_.evolve(lastUsedDroplets_, userContinued, pool_);

        // Update identity based on interaction patterns
        if (userContinued) {
            // User engaged — reinforce current identity
            identity_.energy = std::min(1.0f, identity_.energy + 0.01f);
        } else {
            // User changed topic — cool down, become more adaptive
            identity_.energy = std::max(0.0f, identity_.energy - 0.02f);
            identity_.precision_preference = std::max(0.0f, identity_.precision_preference - 0.05f);
        }

        // Periodic aging
        if (totalGenerations_ % 10 == 0) {
            pool_.age();
        }
    }

    // =======================================================================
    // EXPLAIN — Show thought trace
    // =======================================================================
    std::string explain() const {
        if (lastTrace_.empty()) return "No Prometheus trace yet. Generate something first.";
        return lastTrace_;
    }

    // =======================================================================
    // GET IDENTITY — Show current personality state
    // =======================================================================
    std::string getIdentity() const {
        return identity_.toString();
    }

    // =======================================================================
    // GET STATS — Pool size, alive count, generations, domains
    // =======================================================================
    std::string getStats() const {
        std::ostringstream ss;
        ss << "=== Prometheus Engine Stats ===\n";
        ss << "Pool size: " << pool_.size() << " total droplets\n";
        ss << "Alive: " << pool_.aliveCount() << "\n";
        ss << "Born: " << pool_.getTotalBorn() << " | Killed: " << pool_.getTotalKilled() << "\n";
        ss << "Max generation: " << pool_.maxGeneration() << "\n";
        ss << "Conversations: " << conversationCount_ << " | Code: " << codeCount_ << "\n";
        ss << "Total generations: " << totalGenerations_ << "\n";
        ss << "Emotion droplets: " << pool_.getEmotionIds().size() << "\n";
        ss << "Logic droplets: " << pool_.getLogicIds().size() << "\n";
        ss << "Code droplets: " << pool_.getCodeIds().size() << "\n";
        ss << chamber_.getStats() << "\n";
        ss << identity_.toString() << "\n";
        return ss.str();
    }

    // =======================================================================
    // GET FLUID TRACE — Show last resonance
    // =======================================================================
    std::string getFluidTrace() {
        return dynamics_.getResonanceTrace(pool_);
    }

    // =======================================================================
    // SAVE / LOAD — Persist to disk
    // =======================================================================
    void save() {
        if (!initialized_) return;
        std::system(("mkdir -p " + dataDir_).c_str());
        pool_.save(dataDir_ + "/pool.dat");

        // Save identity state
        std::ofstream f(dataDir_ + "/identity.dat");
        if (f.is_open()) {
            f << identity_.precision_preference << "\n"
              << identity_.verbosity << "\n"
              << identity_.tone << "\n"
              << identity_.energy << "\n"
              << identity_.user_skill_level << "\n"
              << identity_.code_style << "\n";
            f.close();
        }
        std::cout << "[PROMETHEUS] State saved to " << dataDir_ << "/\n";
    }

    void load() {
        std::string idPath = dataDir_ + "/identity.dat";
        std::ifstream f(idPath);
        if (f.is_open()) {
            f >> identity_.precision_preference;
            f >> identity_.verbosity;
            f >> identity_.tone;
            f >> identity_.energy;
            f >> identity_.user_skill_level;
            f >> identity_.code_style;
            f.close();
        }
    }

    // =======================================================================
    // IS INITIALIZED
    // =======================================================================
    bool isInitialized() const { return initialized_; }
};

#endif // MK_PROMETHEUS_ENGINE_CPP
