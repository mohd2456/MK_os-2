#ifndef MK_GENESIS_SUPERPOSITION_CPP
#define MK_GENESIS_SUPERPOSITION_CPP

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <random>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>
#include <memory>

// ===================================================================================
// MK GENESIS ENGINE — QUANTUM SUPERPOSITION
// ===================================================================================
// THE BREAKTHROUGH CONCEPT:
//
// In Prometheus, a thought is ONE thing. It produces ONE response.
// In Genesis, a thought exists in MULTIPLE STATES SIMULTANEOUSLY until
// the moment of output — then it COLLAPSES into the best state.
//
// This is not a metaphor. Each ThoughtDNA spawns multiple "quantum states"
// (different possible interpretations/responses), and ALL states are evaluated
// in parallel. The collapse function picks the winner based on:
//   - Context alignment (does it fit what's happening?)
//   - User resonance history (does the user like this type?)
//   - Adversarial scoring (did it beat competitors?)
//   - Temporal coherence (does it make sense given past/future?)
//
// WHY THIS IS BETTER:
// Prometheus picks a route then commits. If it's wrong, fallback.
// Genesis explores ALL possibilities at once, then collapses to the best.
// It's like thinking of 50 replies and picking the perfect one — instantly.
//
// DECOHERENCE: States naturally decay. The longer you wait to collapse,
// the fewer viable states remain. This creates urgency in generation.
// ===================================================================================


// A single quantum state: one possible interpretation/response
struct QuantumState {
    int source_thought_id;        // Which ThoughtDNA spawned this state
    std::string content;          // The actual text/code this state represents
    int primary_strand;           // Which strand dominates (0=sem, 1=emo, 2=log, 3=exe)
    float probability;            // Probability amplitude (0-1, sum of all states = 1)
    float coherence;              // How well-formed this state is (decays over time)
    float context_alignment;      // How well it fits the current context
    float user_resonance;         // Predicted user satisfaction
    float novelty;                // How surprising/new this is
    float energy;                 // Activation energy
    long long created_at;
    bool collapsed;               // Has this state been observed/selected?
    bool decohered;               // Has this state decayed beyond use?
    
    // Interference pattern: how this state interacts with others
    std::vector<std::pair<int, float>> interference; // state_idx -> constructive(+)/destructive(-)
    
    QuantumState() : source_thought_id(-1), primary_strand(0), probability(0.5f),
                     coherence(1.0f), context_alignment(0.5f), user_resonance(0.5f),
                     novelty(0.5f), energy(0.5f), created_at(0),
                     collapsed(false), decohered(false) {}
    
    // Total score for collapse selection
    float collapseScore() const {
        if (decohered || collapsed) return 0.0f;
        return (probability * 0.25f + coherence * 0.2f + context_alignment * 0.25f +
                user_resonance * 0.2f + novelty * 0.1f) * energy;
    }
};

// ===================================================================================
// SuperpositionField — Where all quantum states exist simultaneously
// ===================================================================================
class MKSuperpositionField {
private:
    std::vector<QuantumState> states_;
    std::mt19937 rng_;
    int totalCollapses_;
    int totalDecoherences_;
    int maxStates_;
    float decoherenceRate_;       // How fast states decay
    float interferenceStrength_;  // How much states affect each other
    
    // Context vector: the current "observation apparatus"
    std::vector<std::string> contextTokens_;
    std::string currentMood_;
    float currentUrgency_;        // High urgency = faster decoherence
    
    long long now() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

public:
    MKSuperpositionField(int maxStates = 200)
        : totalCollapses_(0), totalDecoherences_(0), maxStates_(maxStates),
          decoherenceRate_(0.02f), interferenceStrength_(0.3f),
          currentUrgency_(0.5f) {
        rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
        std::cout << "[GENESIS:QUANTUM] Superposition field initialized. Max states: "
                  << maxStates_ << "\n";
    }


    // --- Set the observation context (what we're looking for) ---
    void setContext(const std::vector<std::string>& tokens, const std::string& mood,
                    float urgency) {
        contextTokens_ = tokens;
        currentMood_ = mood;
        currentUrgency_ = std::clamp(urgency, 0.0f, 1.0f);
    }

    // --- Spawn quantum states from a ThoughtDNA ---
    // Each thought produces MULTIPLE possible states (different readings of its strands)
    void spawn(const ThoughtDNA& thought, int numStates = 5) {
        long long t = now();
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::normal_distribution<float> noise(0.0f, 0.1f);

        for (int i = 0; i < numStates && (int)states_.size() < maxStates_; i++) {
            QuantumState state;
            state.source_thought_id = thought.id;
            state.created_at = t;
            state.energy = thought.activationEnergy();
            
            // Each state emphasizes a different strand
            // State 0: semantic dominant, State 1: emotional, etc.
            // After 4, they become hybrid states (blends)
            if (i < NUM_STRANDS) {
                state.primary_strand = i;
                state.probability = thought.helix[i][0]; // First gene as initial prob
            } else {
                // Hybrid state: blend of two random strands
                int strandA = std::uniform_int_distribution<int>(0, 3)(rng_);
                int strandB = (strandA + 1 + std::uniform_int_distribution<int>(0, 2)(rng_)) % 4;
                state.primary_strand = (thought.helix[strandA][0] > thought.helix[strandB][0]) ?
                    strandA : strandB;
                state.probability = (thought.helix[strandA][0] + thought.helix[strandB][0]) / 2.0f;
                state.novelty += 0.2f; // Hybrids are inherently more novel
            }

            // Generate content based on primary strand emphasis
            switch (state.primary_strand) {
                case STRAND_SEMANTIC:
                    state.content = thought.semantic_content;
                    break;
                case STRAND_EMOTIONAL:
                    if (!thought.emotional_color.empty()) {
                        state.content = thought.emotional_color;
                        if (!thought.semantic_content.empty()) {
                            state.content += " — " + thought.semantic_content;
                        }
                    } else {
                        state.content = thought.semantic_content;
                    }
                    break;
                case STRAND_LOGICAL:
                    if (!thought.logical_structure.empty()) {
                        state.content = thought.logical_structure;
                    } else {
                        state.content = thought.semantic_content;
                    }
                    break;
                case STRAND_EXECUTABLE:
                    if (!thought.executable_payload.empty()) {
                        state.content = thought.executable_payload;
                    } else {
                        state.content = thought.semantic_content;
                    }
                    break;
            }
            
            // Add noise to probability (quantum uncertainty)
            state.probability = std::clamp(state.probability + noise(rng_), 0.05f, 0.95f);
            
            // Context alignment: check if state content matches context tokens
            state.context_alignment = calculateContextAlignment(state.content);
            
            // Initial coherence based on thought vitality
            state.coherence = thought.vitality;
            
            states_.push_back(std::move(state));
        }
    }


    // --- Calculate how well content aligns with current context ---
    float calculateContextAlignment(const std::string& content) const {
        if (contextTokens_.empty() || content.empty()) return 0.3f;
        
        // Tokenize content
        std::string lower = content;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        int matches = 0;
        for (const auto& token : contextTokens_) {
            if (lower.find(token) != std::string::npos) {
                matches++;
            }
        }
        
        float alignment = (float)matches / (float)contextTokens_.size();
        return std::clamp(alignment, 0.0f, 1.0f);
    }

    // --- INTERFERENCE ---
    // States interact with each other: similar states CONSTRUCTIVELY interfere
    // (boost each other), dissimilar states DESTRUCTIVELY interfere (cancel out)
    void computeInterference() {
        for (int i = 0; i < (int)states_.size(); i++) {
            if (states_[i].decohered || states_[i].collapsed) continue;
            states_[i].interference.clear();
            
            for (int j = i + 1; j < (int)states_.size(); j++) {
                if (states_[j].decohered || states_[j].collapsed) continue;
                
                // Calculate interference: same-strand = constructive, different = destructive
                float interference = 0.0f;
                if (states_[i].primary_strand == states_[j].primary_strand) {
                    // Constructive: similar content boosts both
                    interference = interferenceStrength_ * 0.5f;
                } else {
                    // Destructive: competing interpretations weaken both
                    interference = -interferenceStrength_ * 0.3f;
                }
                
                // Content similarity modulates interference
                if (!states_[i].content.empty() && !states_[j].content.empty()) {
                    // Quick similarity: shared words
                    int shared = 0;
                    std::istringstream ssI(states_[i].content);
                    std::string word;
                    while (ssI >> word) {
                        if (states_[j].content.find(word) != std::string::npos) shared++;
                    }
                    float similarity = std::min((float)shared / 5.0f, 1.0f);
                    interference *= (1.0f + similarity);
                }
                
                states_[i].interference.push_back({j, interference});
                states_[j].interference.push_back({i, interference});
            }
        }
        
        // Apply interference to probabilities
        for (auto& state : states_) {
            if (state.decohered || state.collapsed) continue;
            float totalInterference = 0.0f;
            for (const auto& [idx, strength] : state.interference) {
                totalInterference += strength;
            }
            state.probability = std::clamp(state.probability + totalInterference * 0.1f, 0.01f, 0.99f);
        }
    }


    // --- DECOHERENCE ---
    // States naturally decay over time. High urgency = faster decay.
    // This forces the system to make decisions, not deliberate forever.
    void decohere() {
        long long t = now();
        for (auto& state : states_) {
            if (state.decohered || state.collapsed) continue;
            
            long long age = t - state.created_at;
            float decay = decoherenceRate_ * (1.0f + currentUrgency_) * (age / 1000.0f);
            state.coherence -= decay;
            
            if (state.coherence <= 0.0f) {
                state.decohered = true;
                state.coherence = 0.0f;
                totalDecoherences_++;
            }
        }
    }

    // --- COLLAPSE ---
    // THE MOMENT OF TRUTH: observe the superposition and collapse to one state.
    // Returns the winning state's content and updates all state metadata.
    // 
    // Collapse strategy:
    //   1. Score all living states
    //   2. Apply probability-weighted random selection (not just max!)
    //      This means even lower-probability states can win sometimes = creativity
    //   3. Mark winner as collapsed, mark all others as decohered
    //   4. Return the winner
    QuantumState collapse() {
        // Apply interference first
        computeInterference();
        
        // Apply decoherence
        decohere();
        
        // Collect living states
        std::vector<int> living;
        for (int i = 0; i < (int)states_.size(); i++) {
            if (!states_[i].decohered && !states_[i].collapsed && !states_[i].content.empty()) {
                living.push_back(i);
            }
        }
        
        if (living.empty()) {
            // All states decohered — return empty
            QuantumState empty;
            empty.decohered = true;
            return empty;
        }
        
        // Calculate collapse scores
        std::vector<float> scores;
        float totalScore = 0.0f;
        for (int idx : living) {
            float s = states_[idx].collapseScore();
            scores.push_back(s);
            totalScore += s;
        }
        
        // Probability-weighted selection (roulette wheel)
        int winner = living[0]; // default
        if (totalScore > 0.0f) {
            std::uniform_real_distribution<float> dist(0.0f, totalScore);
            float spin = dist(rng_);
            float cumulative = 0.0f;
            for (int i = 0; i < (int)living.size(); i++) {
                cumulative += scores[i];
                if (spin <= cumulative) {
                    winner = living[i];
                    break;
                }
            }
        }
        
        // Collapse!
        states_[winner].collapsed = true;
        totalCollapses_++;
        
        // Decohere all other states from same spawn
        for (int i = 0; i < (int)states_.size(); i++) {
            if (i != winner && !states_[i].collapsed) {
                states_[i].decohered = true;
            }
        }
        
        return states_[winner];
    }


    // --- MULTI-COLLAPSE ---
    // Collapse multiple states keeping top N (for responses that need multiple parts)
    std::vector<QuantumState> multiCollapse(int n = 3) {
        std::vector<QuantumState> results;
        
        for (int i = 0; i < n; i++) {
            // Collect living states
            std::vector<int> living;
            for (int j = 0; j < (int)states_.size(); j++) {
                if (!states_[j].decohered && !states_[j].collapsed && !states_[j].content.empty()) {
                    living.push_back(j);
                }
            }
            if (living.empty()) break;
            
            // Score and pick winner
            float bestScore = -1.0f;
            int winner = living[0];
            for (int idx : living) {
                float s = states_[idx].collapseScore();
                if (s > bestScore) {
                    bestScore = s;
                    winner = idx;
                }
            }
            
            states_[winner].collapsed = true;
            results.push_back(states_[winner]);
        }
        
        totalCollapses_ += (int)results.size();
        return results;
    }

    // --- RESET ---
    // Clear all states for a new generation cycle
    void reset() {
        states_.clear();
    }

    // --- Boost a specific state's probability (external signal, like user preference) ---
    void boostState(int stateIdx, float amount) {
        if (stateIdx >= 0 && stateIdx < (int)states_.size()) {
            states_[stateIdx].probability = std::clamp(
                states_[stateIdx].probability + amount, 0.0f, 1.0f);
            states_[stateIdx].user_resonance += amount * 0.5f;
        }
    }

    // --- Set user resonance for all states matching a pattern ---
    void setUserResonance(const std::string& pattern, float resonance) {
        std::string lower = pattern;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (auto& state : states_) {
            std::string content_lower = state.content;
            std::transform(content_lower.begin(), content_lower.end(),
                          content_lower.begin(), ::tolower);
            if (content_lower.find(lower) != std::string::npos) {
                state.user_resonance = std::clamp(resonance, 0.0f, 1.0f);
            }
        }
    }

    // --- Stats ---
    int stateCount() const { return (int)states_.size(); }
    int livingCount() const {
        int count = 0;
        for (const auto& s : states_) {
            if (!s.decohered && !s.collapsed) count++;
        }
        return count;
    }
    int getTotalCollapses() const { return totalCollapses_; }
    int getTotalDecoherences() const { return totalDecoherences_; }
    
    std::string getStats() const {
        std::ostringstream ss;
        ss << "Quantum Field: " << states_.size() << " total states, "
           << livingCount() << " alive | Collapses: " << totalCollapses_
           << " | Decoherences: " << totalDecoherences_
           << " | Urgency: " << (int)(currentUrgency_ * 100) << "%";
        return ss.str();
    }

    std::string getTrace() const {
        std::ostringstream ss;
        ss << "=== Superposition Trace ===\n";
        int shown = 0;
        for (const auto& s : states_) {
            if (s.collapsed) {
                ss << "  [COLLAPSED] strand=" << s.primary_strand
                   << " prob=" << (int)(s.probability * 100) << "%"
                   << " score=" << (int)(s.collapseScore() * 100)
                   << " content=\"" << s.content.substr(0, 60) << "\"\n";
                shown++;
            }
            if (shown >= 5) break;
        }
        return ss.str();
    }
};

#endif // MK_GENESIS_SUPERPOSITION_CPP
