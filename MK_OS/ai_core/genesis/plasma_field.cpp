#ifndef MK_GENESIS_PLASMA_FIELD_CPP
#define MK_GENESIS_PLASMA_FIELD_CPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <queue>
#include <functional>

// ===================================================================================
// MK GENESIS ENGINE — SYNAPTIC PLASMA
// ===================================================================================
// The MEDIUM through which ThoughtDNA strands interact.
//
// In the brain, synapses are the gaps between neurons where signals cross.
// In Genesis, the Synaptic Plasma is a LIVING FIELD that:
//
//   1. CONDUCTS signals between ThoughtDNA strands (propagation)
//   2. AMPLIFIES strong signals and DAMPENS weak ones (gain control)
//   3. FORMS PATHWAYS that strengthen with use (Hebbian learning)
//   4. DISSOLVES pathways that aren't used (synaptic pruning)
//   5. CREATES STORMS when many thoughts activate at once (brainstorming)
//   6. CRYSTALLIZES stable patterns into permanent knowledge
//
// WHY THIS IS BETTER THAN PROMETHEUS:
// Prometheus has no concept of HOW thoughts connect — they just have bonds.
// Genesis Plasma creates DYNAMIC, WEIGHTED, ADAPTIVE pathways that learn
// which thought-combinations produce good results. The plasma itself is
// intelligent — it routes signals to the best destinations based on history.
//
// PLASMA STORMS:
// When many ThoughtDNA strands activate simultaneously, the plasma enters
// "storm mode" — rapid signal propagation, reduced inhibition, increased
// cross-strand bleed. This is equivalent to creative brainstorming or
// "shower thoughts" — when you're not trying, connections form naturally.
// ===================================================================================


// A synapse: a weighted connection between two thoughts
struct Synapse {
    int from_id;
    int to_id;
    float weight;             // Connection strength (0-1)
    float plasticity;         // How easily this connection changes
    int times_fired;          // How many times signal passed through
    long long last_fired;     // When last used
    long long created_at;
    bool permanent;           // Crystallized (will never be pruned)
    
    Synapse() : from_id(-1), to_id(-1), weight(0.5f), plasticity(0.8f),
                times_fired(0), last_fired(0), created_at(0), permanent(false) {}
    
    Synapse(int from, int to, float w)
        : from_id(from), to_id(to), weight(w), plasticity(0.8f),
          times_fired(0), last_fired(0), created_at(0), permanent(false) {
        created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        last_fired = created_at;
    }
};

// A signal propagating through the plasma
struct PlasmaSignal {
    int origin_thought_id;     // Where the signal started
    int current_thought_id;    // Where it is now
    float strength;            // Current signal strength (decays as it travels)
    int hops;                  // How many synapses it has crossed
    int max_hops;              // Maximum travel distance
    std::vector<int> path;     // Thoughts visited
    std::string payload;       // What the signal carries (trigger word, concept)
    
    PlasmaSignal() : origin_thought_id(-1), current_thought_id(-1),
                     strength(1.0f), hops(0), max_hops(5) {}
};

// Plasma storm state
enum class PlasmaMode {
    CALM,           // Normal signal routing
    EXCITED,        // Elevated activity, faster propagation
    STORM,          // Maximum creativity, all inhibition removed
    CRYSTALLIZING   // Solidifying patterns into permanent connections
};


// ===================================================================================
// MKSynapticPlasma — The living connective field
// ===================================================================================
class MKSynapticPlasma {
private:
    // Synapse storage: from_id -> list of synapses
    std::unordered_map<int, std::vector<Synapse>> connections_;
    // Reverse index: to_id -> list of source IDs
    std::unordered_map<int, std::vector<int>> reverseIndex_;
    
    // Active signals currently propagating
    std::vector<PlasmaSignal> activeSignals_;
    
    // Plasma state
    PlasmaMode mode_;
    float temperature_;        // Overall plasma temperature (affects propagation)
    float inhibition_;         // Global inhibition (0=none, 1=full block)
    int totalSynapses_;
    int totalSignalsFired_;
    int totalCrystallized_;
    int stormCount_;
    
    // Hebbian learning parameters
    float hebbianRate_;        // How fast connections strengthen
    float pruneThreshold_;     // Below this weight, synapses die
    float crystallizeThreshold_; // Above this, synapses become permanent
    
    std::mt19937 rng_;

    long long now() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

public:
    MKSynapticPlasma()
        : mode_(PlasmaMode::CALM), temperature_(0.5f), inhibition_(0.3f),
          totalSynapses_(0), totalSignalsFired_(0), totalCrystallized_(0),
          stormCount_(0), hebbianRate_(0.05f), pruneThreshold_(0.05f),
          crystallizeThreshold_(0.9f) {
        rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
        std::cout << "[GENESIS:PLASMA] Synaptic plasma field initialized.\n";
    }

    // --- Connect two thoughts with a synapse ---
    void connect(int fromId, int toId, float weight = 0.5f) {
        Synapse syn(fromId, toId, weight);
        connections_[fromId].push_back(syn);
        reverseIndex_[toId].push_back(fromId);
        totalSynapses_++;
    }

    // --- Check if connection exists ---
    bool hasConnection(int fromId, int toId) const {
        auto it = connections_.find(fromId);
        if (it == connections_.end()) return false;
        for (const auto& syn : it->second) {
            if (syn.to_id == toId) return true;
        }
        return false;
    }

    // --- Get connection weight ---
    float getWeight(int fromId, int toId) const {
        auto it = connections_.find(fromId);
        if (it == connections_.end()) return 0.0f;
        for (const auto& syn : it->second) {
            if (syn.to_id == toId) return syn.weight;
        }
        return 0.0f;
    }


    // --- FIRE: Send a signal from one thought outward ---
    // Returns all thought IDs reached by the signal
    std::vector<int> fire(int fromId, const std::string& payload, int maxHops = 4) {
        PlasmaSignal signal;
        signal.origin_thought_id = fromId;
        signal.current_thought_id = fromId;
        signal.strength = 1.0f;
        signal.max_hops = maxHops;
        signal.payload = payload;
        signal.path.push_back(fromId);
        
        std::vector<int> reached;
        std::queue<PlasmaSignal> queue;
        queue.push(signal);
        std::unordered_set<int> visited;
        visited.insert(fromId);
        
        // Mode-dependent propagation
        float propagationMultiplier = 1.0f;
        float decayRate = 0.3f;
        switch (mode_) {
            case PlasmaMode::CALM:
                propagationMultiplier = 1.0f; decayRate = 0.3f; break;
            case PlasmaMode::EXCITED:
                propagationMultiplier = 1.5f; decayRate = 0.2f; break;
            case PlasmaMode::STORM:
                propagationMultiplier = 3.0f; decayRate = 0.1f; break;
            case PlasmaMode::CRYSTALLIZING:
                propagationMultiplier = 0.5f; decayRate = 0.5f; break;
        }
        
        while (!queue.empty()) {
            PlasmaSignal current = queue.front();
            queue.pop();
            
            if (current.hops >= current.max_hops) continue;
            if (current.strength < 0.1f) continue;
            
            // Check for inhibition
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            if (dist(rng_) < inhibition_ && mode_ != PlasmaMode::STORM) continue;
            
            // Find all outgoing synapses
            auto it = connections_.find(current.current_thought_id);
            if (it == connections_.end()) continue;
            
            for (auto& syn : it->second) {
                if (visited.count(syn.to_id)) continue;
                
                // Signal strength after crossing synapse
                float transmitted = current.strength * syn.weight * propagationMultiplier;
                transmitted *= (1.0f - decayRate); // Distance decay
                
                if (transmitted < 0.05f) continue; // Too weak
                
                // Fire!
                visited.insert(syn.to_id);
                reached.push_back(syn.to_id);
                
                // Hebbian learning: "cells that fire together wire together"
                syn.weight = std::clamp(syn.weight + hebbianRate_, 0.0f, 1.0f);
                syn.times_fired++;
                syn.last_fired = now();
                
                // Propagate further
                PlasmaSignal next;
                next.origin_thought_id = signal.origin_thought_id;
                next.current_thought_id = syn.to_id;
                next.strength = transmitted;
                next.hops = current.hops + 1;
                next.max_hops = current.max_hops;
                next.payload = current.payload;
                next.path = current.path;
                next.path.push_back(syn.to_id);
                queue.push(next);
            }
        }
        
        totalSignalsFired_++;
        return reached;
    }


    // --- STORM: Trigger a plasma storm (brainstorming mode) ---
    void triggerStorm() {
        mode_ = PlasmaMode::STORM;
        inhibition_ = 0.0f;
        temperature_ = 1.0f;
        stormCount_++;
        std::cout << "[GENESIS:PLASMA] ⚡ PLASMA STORM #" << stormCount_ << " triggered!\n";
    }

    // --- CALM: Return to normal operation ---
    void calm() {
        mode_ = PlasmaMode::CALM;
        inhibition_ = 0.3f;
        temperature_ = 0.5f;
    }

    // --- EXCITE: Mild excitation (focused thinking) ---
    void excite() {
        mode_ = PlasmaMode::EXCITED;
        inhibition_ = 0.15f;
        temperature_ = 0.7f;
    }

    // --- CRYSTALLIZE: Solidify strongest patterns ---
    void crystallize() {
        mode_ = PlasmaMode::CRYSTALLIZING;
        int crystallized = 0;
        
        for (auto& [fromId, synapses] : connections_) {
            for (auto& syn : synapses) {
                if (!syn.permanent && syn.weight >= crystallizeThreshold_) {
                    syn.permanent = true;
                    crystallized++;
                    totalCrystallized_++;
                }
            }
        }
        
        if (crystallized > 0) {
            std::cout << "[GENESIS:PLASMA] 💎 Crystallized " << crystallized
                      << " synapses into permanent connections.\n";
        }
        
        // Return to calm after crystallizing
        calm();
    }

    // --- PRUNE: Remove weak, unused synapses ---
    void prune() {
        long long t = now();
        int pruned = 0;
        
        for (auto& [fromId, synapses] : connections_) {
            synapses.erase(
                std::remove_if(synapses.begin(), synapses.end(),
                    [&](const Synapse& s) {
                        if (s.permanent) return false; // Never prune permanent
                        // Prune if weight is below threshold AND hasn't fired recently
                        bool weak = s.weight < pruneThreshold_;
                        bool stale = (t - s.last_fired) > 300000; // 5 min unused
                        if (weak || stale) { pruned++; return true; }
                        return false;
                    }),
                synapses.end()
            );
        }
        
        totalSynapses_ -= pruned;
        if (pruned > 0) {
            std::cout << "[GENESIS:PLASMA] Pruned " << pruned << " weak synapses.\n";
        }
    }

    // --- DECAY: Natural weight decay for non-permanent synapses ---
    void decay(float rate = 0.001f) {
        for (auto& [fromId, synapses] : connections_) {
            for (auto& syn : synapses) {
                if (!syn.permanent) {
                    syn.weight -= rate;
                    if (syn.weight < 0.0f) syn.weight = 0.0f;
                }
            }
        }
    }


    // --- STRENGTHEN: Reinforce a specific connection (positive feedback) ---
    void strengthen(int fromId, int toId, float amount = 0.1f) {
        auto it = connections_.find(fromId);
        if (it == connections_.end()) return;
        for (auto& syn : it->second) {
            if (syn.to_id == toId) {
                syn.weight = std::clamp(syn.weight + amount, 0.0f, 1.0f);
                syn.plasticity *= 0.99f; // Less plastic as it strengthens
                return;
            }
        }
        // If no existing connection, create one
        connect(fromId, toId, amount);
    }

    // --- WEAKEN: Reduce a specific connection (negative feedback) ---
    void weaken(int fromId, int toId, float amount = 0.1f) {
        auto it = connections_.find(fromId);
        if (it == connections_.end()) return;
        for (auto& syn : it->second) {
            if (syn.to_id == toId) {
                syn.weight = std::clamp(syn.weight - amount, 0.0f, 1.0f);
                return;
            }
        }
    }

    // --- GET STRONGEST CONNECTIONS: Find most important links from a thought ---
    std::vector<std::pair<int, float>> getStrongest(int fromId, int maxResults = 10) const {
        std::vector<std::pair<int, float>> results;
        auto it = connections_.find(fromId);
        if (it == connections_.end()) return results;
        
        for (const auto& syn : it->second) {
            results.push_back({syn.to_id, syn.weight});
        }
        
        std::sort(results.begin(), results.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        if ((int)results.size() > maxResults) results.resize(maxResults);
        return results;
    }

    // --- FIND PATH: BFS shortest path between two thoughts ---
    std::vector<int> findPath(int fromId, int toId, int maxDepth = 6) const {
        if (fromId == toId) return {fromId};
        
        std::queue<std::vector<int>> queue;
        std::unordered_set<int> visited;
        queue.push({fromId});
        visited.insert(fromId);
        
        while (!queue.empty()) {
            auto path = queue.front();
            queue.pop();
            
            if ((int)path.size() > maxDepth) continue;
            int current = path.back();
            
            auto it = connections_.find(current);
            if (it == connections_.end()) continue;
            
            for (const auto& syn : it->second) {
                if (syn.to_id == toId) {
                    path.push_back(toId);
                    return path;
                }
                if (!visited.count(syn.to_id) && syn.weight > 0.1f) {
                    visited.insert(syn.to_id);
                    auto newPath = path;
                    newPath.push_back(syn.to_id);
                    queue.push(newPath);
                }
            }
        }
        
        return {}; // No path found
    }

    // --- AUTO-CONNECT: Create connections between resonating thoughts ---
    void autoConnect(const std::vector<std::pair<int, float>>& resonating) {
        // Connect all resonating thoughts to each other (weighted by resonance)
        for (int i = 0; i < (int)resonating.size(); i++) {
            for (int j = i + 1; j < (int)resonating.size(); j++) {
                int idA = resonating[i].first;
                int idB = resonating[j].first;
                float strength = resonating[i].second * resonating[j].second;
                
                if (!hasConnection(idA, idB) && strength > 0.1f) {
                    connect(idA, idB, strength * 0.5f);
                    connect(idB, idA, strength * 0.5f); // Bidirectional
                }
            }
        }
    }


    // --- Stats & Diagnostics ---
    PlasmaMode getMode() const { return mode_; }
    
    std::string getModeString() const {
        switch (mode_) {
            case PlasmaMode::CALM: return "CALM";
            case PlasmaMode::EXCITED: return "EXCITED";
            case PlasmaMode::STORM: return "STORM";
            case PlasmaMode::CRYSTALLIZING: return "CRYSTALLIZING";
        }
        return "UNKNOWN";
    }

    int synapseCount() const { return totalSynapses_; }
    int permanentCount() const { return totalCrystallized_; }
    float getTemperature() const { return temperature_; }

    std::string getStats() const {
        std::ostringstream ss;
        ss << "Plasma: mode=" << getModeString()
           << " | synapses=" << totalSynapses_
           << " | permanent=" << totalCrystallized_
           << " | signals_fired=" << totalSignalsFired_
           << " | storms=" << stormCount_
           << " | temp=" << (int)(temperature_ * 100) << "%"
           << " | inhibition=" << (int)(inhibition_ * 100) << "%";
        return ss.str();
    }

    // Save plasma state to disk
    void save(const std::string& path) const {
        std::ofstream out(path);
        if (!out.is_open()) return;
        
        out << totalSynapses_ << "\n";
        for (const auto& [fromId, synapses] : connections_) {
            for (const auto& syn : synapses) {
                if (syn.permanent || syn.weight > 0.3f) { // Only save significant ones
                    out << syn.from_id << " " << syn.to_id << " "
                        << syn.weight << " " << syn.times_fired << " "
                        << (syn.permanent ? 1 : 0) << "\n";
                }
            }
        }
    }

    // Load plasma state from disk
    void load(const std::string& path) {
        std::ifstream in(path);
        if (!in.is_open()) return;
        
        int count;
        in >> count;
        
        int from, to, fired, perm;
        float weight;
        while (in >> from >> to >> weight >> fired >> perm) {
            Synapse syn(from, to, weight);
            syn.times_fired = fired;
            syn.permanent = (perm == 1);
            connections_[from].push_back(syn);
            reverseIndex_[to].push_back(from);
            totalSynapses_++;
            if (syn.permanent) totalCrystallized_++;
        }
        
        std::cout << "[GENESIS:PLASMA] Loaded " << totalSynapses_ << " synapses ("
                  << totalCrystallized_ << " permanent).\n";
    }
};

#endif // MK_GENESIS_PLASMA_FIELD_CPP
