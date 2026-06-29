#ifndef MK_GENESIS_ENGINE_CPP
#define MK_GENESIS_ENGINE_CPP

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
#include <fstream>
#include <functional>
#include <memory>

// ===================================================================================
// MK GENESIS ENGINE — MASTER CLASS
// ===================================================================================
//
// ╔═══════════════════════════════════════════════════════════════════════════════╗
// ║                    G E N E S I S   E N G I N E                              ║
// ║                                                                             ║
// ║   "Thought-DNA with Quantum Superposition and Adversarial Dreaming"         ║
// ║                                                                             ║
// ║   The most advanced local AI generation system ever built.                  ║
// ║   No cloud. No LLM. Pure algorithmic intelligence.                          ║
// ╚═══════════════════════════════════════════════════════════════════════════════╝
//
// PIPELINE:
//   Input → Tokenize → Activate ThoughtDNA → Propagate through Plasma →
//   Spawn Quantum States → Adversarial Battle → Collapse → Metamorphic Compile →
//   Temporal Record → Output
//
// ONE CALL: generate(input) → response
//
// BACKGROUND: dreamTick() runs during idle time for creative synthesis
//
// LEARNING: absorb(input, response, positive) teaches from every interaction
//
// ===================================================================================

#include "thought_dna.cpp"
#include "superposition.cpp"
#include "plasma_field.cpp"
#include "temporal_mind.cpp"
#include "metamorph.cpp"
#include "adversarial_chamber.cpp"
#include "dream_synth.cpp"


class MKGenesisEngine {
private:
    // === Core Subsystems ===
    MKThoughtPool pool_;
    MKSuperpositionField quantum_;
    MKSynapticPlasma plasma_;
    MKTemporalMind temporal_;
    MKMetamorphicCompiler compiler_;
    MKAdversarialChamber arena_;
    MKDreamSynthesizer dreamer_;
    
    // State
    bool initialized_;
    int totalGenerations_;
    int totalAbsorptions_;
    std::string lastTrace_;
    std::string lastOutput_;
    Archetype lastWinningArchetype_;
    std::string dataDir_;
    
    std::mt19937 rng_;
    
    long long now() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // Tokenize input into lowercase words (for trigger matching)
    std::vector<std::string> tokenize(const std::string& input) const {
        std::vector<std::string> tokens;
        std::istringstream ss(input);
        std::string word;
        while (ss >> word) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            // Strip punctuation
            while (!word.empty() && !std::isalnum(word.back())) word.pop_back();
            while (!word.empty() && !std::isalnum(word.front())) word.erase(word.begin());
            if (!word.empty() && word.size() > 1) tokens.push_back(word);
        }
        return tokens;
    }

    // Detect if input is conversational vs. question vs. command
    std::string detectIntent(const std::string& input) const {
        std::string lower = input;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        if (lower.find("?") != std::string::npos || lower.find("what") == 0 ||
            lower.find("how") == 0 || lower.find("why") == 0 ||
            lower.find("who") == 0 || lower.find("where") == 0 ||
            lower.find("when") == 0 || lower.find("explain") != std::string::npos) {
            return "question";
        }
        if (lower.find("write") != std::string::npos || lower.find("code") != std::string::npos ||
            lower.find("create") != std::string::npos || lower.find("build") != std::string::npos ||
            lower.find("make") != std::string::npos || lower.find("generate") != std::string::npos) {
            return "create";
        }
        if (lower.size() < 20 || lower.find("hey") == 0 || lower.find("hi") == 0 ||
            lower.find("hello") == 0 || lower.find("thanks") != std::string::npos ||
            lower.find("lol") != std::string::npos || lower.find("haha") != std::string::npos) {
            return "casual";
        }
        if (lower.find("think") != std::string::npos || lower.find("compare") != std::string::npos ||
            lower.find("analyze") != std::string::npos || lower.find("reason") != std::string::npos) {
            return "reasoning";
        }
        return "general";
    }

    // Set quantum urgency based on input type
    float determineUrgency(const std::string& intent) const {
        if (intent == "casual") return 0.8f;   // Quick answer needed
        if (intent == "question") return 0.5f; // Medium deliberation
        if (intent == "create") return 0.3f;   // Take time, be creative
        if (intent == "reasoning") return 0.2f; // Maximum deliberation
        return 0.5f;
    }

public:
    MKGenesisEngine()
        : pool_(10000), quantum_(200), initialized_(false),
          totalGenerations_(0), totalAbsorptions_(0),
          lastWinningArchetype_(Archetype::FRIEND),
          dataDir_("genesis_data") {
        rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
    }


    // ==========================================================================
    // INITIALIZE: Bootstrap the engine with knowledge and response data
    // ==========================================================================
    void initialize(const std::vector<std::string>& responseTemplates,
                    const std::vector<std::string>& knowledgeFacts) {
        
        std::cout << "[GENESIS] Initializing Genesis Engine...\n";
        
        // Step 1: Seed the ThoughtPool with knowledge facts as ThoughtDNA
        int seeded = 0;
        for (const auto& fact : knowledgeFacts) {
            ThoughtDNA dna;
            dna.id = pool_.manipulator().getNextId();
            dna.semantic_content = fact;
            dna.generation = 0;
            dna.fitness = 0.6f;
            dna.vitality = 0.8f;
            dna.born_at = now();
            dna.last_activated = dna.born_at;
            
            // Set genes based on content analysis
            dna.setGene(STRAND_SEMANTIC, SEM_CERTAINTY, 0.8f);
            dna.setGene(STRAND_SEMANTIC, SEM_DEPTH, 0.5f);
            dna.setGene(STRAND_LOGICAL, LOG_CAUSALITY, 0.6f);
            
            // Extract trigger words
            std::istringstream ss(fact);
            std::string word;
            int wordCount = 0;
            while (ss >> word && wordCount < 5) {
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                while (!word.empty() && !std::isalnum(word.back())) word.pop_back();
                if (word.size() > 2) {
                    dna.triggers.push_back(word);
                    wordCount++;
                }
            }
            
            // Set domain from first meaningful word
            if (!dna.triggers.empty()) {
                dna.domain = dna.triggers[0];
            }
            
            // Set resonance frequency based on content hash
            float hash = 0.0f;
            for (char c : fact) hash += (float)c;
            dna.resonance_freq = std::fmod(hash, 1.0f);
            
            pool_.add(std::move(dna));
            seeded++;
            if (seeded >= 5000) break; // Cap initial seeding
        }
        
        // Step 2: Seed response templates as emotional/conversational ThoughtDNA
        for (const auto& tmpl : responseTemplates) {
            ThoughtDNA dna;
            dna.id = pool_.manipulator().getNextId();
            dna.semantic_content = tmpl;
            dna.emotional_color = "conversational";
            dna.generation = 0;
            dna.fitness = 0.7f;
            dna.vitality = 0.9f;
            dna.born_at = now();
            dna.last_activated = dna.born_at;
            
            // High emotional genes
            dna.setGene(STRAND_EMOTIONAL, EMO_WARMTH, 0.8f);
            dna.setGene(STRAND_EMOTIONAL, EMO_VALENCE, 0.7f);
            dna.setGene(STRAND_SEMANTIC, SEM_DEPTH, 0.3f);
            
            // Trigger words from template
            std::istringstream ss(tmpl);
            std::string word;
            int wordCount = 0;
            while (ss >> word && wordCount < 4) {
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                while (!word.empty() && !std::isalnum(word.back())) word.pop_back();
                if (word.size() > 2) {
                    dna.triggers.push_back(word);
                    wordCount++;
                }
            }
            
            dna.domain = "conversation";
            pool_.add(std::move(dna));
        }
        
        // Step 3: Create initial plasma connections between related thoughts
        auto living = pool_.getLiving();
        int connections = 0;
        for (int i = 0; i < (int)living.size() && i < 500; i++) {
            for (int j = i + 1; j < (int)living.size() && j < i + 10; j++) {
                float compat = pool_.get(living[i]).compatibility(pool_.get(living[j]));
                if (compat > 0.6f) {
                    plasma_.connect(living[i], living[j], compat * 0.5f);
                    plasma_.connect(living[j], living[i], compat * 0.5f);
                    connections++;
                }
            }
        }
        
        // Step 4: Load saved state if available
        load();
        
        initialized_ = true;
        std::cout << "[GENESIS] ✓ Engine initialized.\n"
                  << "[GENESIS]   Thought pool: " << pool_.livingCount() << " living thoughts\n"
                  << "[GENESIS]   Plasma synapses: " << plasma_.synapseCount() << "\n"
                  << "[GENESIS]   Arena minds: " << arena_.getMindCount() << "\n"
                  << "[GENESIS]   Temporal stream: " << temporal_.getStreamSize() << " moments\n";
    }


    // ==========================================================================
    // GENERATE: The main entry point. Input → Response.
    // ==========================================================================
    // This is where ALL subsystems work together:
    //   1. Tokenize input
    //   2. Activate matching ThoughtDNA from pool
    //   3. Propagate activation through plasma (spreading activation)
    //   4. Spawn quantum states from activated thoughts
    //   5. Each adversarial mind produces a candidate response
    //   6. Battle! Winner selected
    //   7. Metamorphic compile the winner into final text
    //   8. Record in temporal stream
    //   9. Return the response
    // ==========================================================================
    std::string generate(const std::string& input) {
        if (!initialized_) return "";
        
        // Wake the dreamer (user is active)
        dreamer_.wake();
        
        // Step 1: Tokenize and analyze
        auto tokens = tokenize(input);
        std::string intent = detectIntent(input);
        float urgency = determineUrgency(intent);
        
        // Step 2: Get temporal influence (mood, momentum, predictions)
        auto temporalInfluence = temporal_.getInfluence();
        
        // Step 3: Find resonating ThoughtDNA
        auto resonating = pool_.findResonating(tokens, 15);
        
        // If nothing resonates, try broader search
        if (resonating.empty() && !tokens.empty()) {
            // Try individual tokens
            for (const auto& token : tokens) {
                auto partial = pool_.findResonating({token}, 5);
                for (const auto& p : partial) {
                    resonating.push_back(p);
                }
            }
        }
        
        // Step 4: Propagate through plasma (spreading activation)
        std::unordered_set<int> activated;
        for (const auto& [id, strength] : resonating) {
            activated.insert(id);
            // Fire signal through plasma to find connected thoughts
            if (strength > 0.3f) {
                auto reached = plasma_.fire(id, input, 3);
                for (int r : reached) activated.insert(r);
            }
        }
        
        // Step 5: Auto-connect resonating thoughts in plasma (Hebbian)
        plasma_.autoConnect(resonating);
        
        // Step 6: Spawn quantum states from all activated thoughts
        quantum_.reset();
        quantum_.setContext(tokens, temporal_.getMood().dominantMood(), urgency);
        
        int spawned = 0;
        for (int id : activated) {
            if (spawned >= 30) break; // Cap to prevent explosion
            const auto& thought = pool_.get(id);
            if (thought.isAlive()) {
                quantum_.spawn(thought, 3); // 3 states per thought
                spawned++;
            }
        }


        // Step 7: Adversarial battle — each mind produces a candidate
        // Each mind gets a biased view of the top ThoughtDNA
        std::vector<std::string> candidates;
        
        if (!resonating.empty()) {
            // Get the top thought for the battle
            ThoughtDNA topThought = pool_.get(resonating[0].first);
            
            // Each mind gets a biased version
            auto biasedThoughts = arena_.getBiasedThoughts(topThought);
            
            // Each mind produces its candidate through quantum collapse
            for (int m = 0; m < (int)biasedThoughts.size(); m++) {
                MKSuperpositionField mindField(50);
                mindField.setContext(tokens, temporal_.getMood().dominantMood(), urgency);
                
                // Spawn states from this mind's biased thought
                mindField.spawn(biasedThoughts[m], 5);
                
                // Also spawn from a few other activated thoughts
                int extra = 0;
                for (int id : activated) {
                    if (extra >= 3) break;
                    if (id != resonating[0].first) {
                        ThoughtDNA extraThought = pool_.get(id);
                        // Apply same mind bias
                        ThoughtDNA biasedExtra = arena_.applyBias(extraThought, arena_.getMind(m));
                        mindField.spawn(biasedExtra, 2);
                        extra++;
                    }
                }
                
                // Collapse this mind's field
                auto collapsed = mindField.collapse();
                
                if (!collapsed.decohered && !collapsed.content.empty()) {
                    // Compile through metamorphic compiler
                    MorphedOutput morphed = compiler_.compile(
                        collapsed, biasedThoughts[m],
                        temporalInfluence.creativity_boost,
                        temporalInfluence.warmth_boost,
                        temporalInfluence.verbosity_modifier
                    );
                    candidates.push_back(morphed.text);
                } else {
                    candidates.push_back(""); // Empty candidate
                }
            }
        } else {
            // No resonating thoughts: produce generic responses per archetype
            candidates.push_back("I don't have specific knowledge about that yet");
            candidates.push_back("hmm, tell me more about what you mean?");
            candidates.push_back("I'd need more context to help with that");
            candidates.push_back("what if we looked at it from a different angle?");
            candidates.push_back("let me think about that differently...");
        }
        
        // Step 8: BATTLE! Score and select winner
        BattleResult battle = arena_.battle(candidates, input, tokens);
        
        std::string response = battle.winning_response;
        lastWinningArchetype_ = battle.winning_archetype;
        
        // Step 9: If response is empty, fallback
        if (response.empty()) {
            response = "I'm still learning about that. Can you tell me more?";
        }


        // Step 10: Record in temporal stream
        temporal_.record(input, response, battle.winning_score, !response.empty());
        
        // Step 11: Update thought fitness for activated thoughts
        for (const auto& [id, strength] : resonating) {
            pool_.get(id).last_activated = now();
            pool_.get(id).times_used++;
            if (battle.winning_score > 0.5f) {
                pool_.get(id).fitness = std::min(1.0f, pool_.get(id).fitness + 0.01f);
            }
        }
        
        // Step 12: Build trace
        std::ostringstream trace;
        trace << "=== Genesis Generation #" << totalGenerations_ + 1 << " ===\n"
              << "Intent: " << intent << " | Urgency: " << (int)(urgency * 100) << "%\n"
              << "Activated: " << activated.size() << " thoughts\n"
              << "Quantum states spawned: " << spawned * 3 << "\n"
              << temporal_.getContextSummary() << "\n"
              << battle.battle_trace;
        lastTrace_ = trace.str();
        lastOutput_ = response;
        
        totalGenerations_++;
        
        // Periodic maintenance (every 50 generations)
        if (totalGenerations_ % 50 == 0) {
            pool_.populationControl();
            pool_.decay(0.0005f);
            plasma_.prune();
            arena_.evolve();
        }
        
        return response;
    }

    // ==========================================================================
    // ABSORB: Learn from every interaction (positive or negative feedback)
    // ==========================================================================
    void absorb(const std::string& input, const std::string& response, bool positive) {
        if (!initialized_) return;
        totalAbsorptions_++;
        
        // Report to adversarial arena
        arena_.reportFeedback(positive, lastWinningArchetype_);
        
        // Adapt metamorphic compiler style
        // Determine what morph type was likely used
        MorphType likelyType = MorphType::CONVERSATION;
        if (response.find("```") != std::string::npos) likelyType = MorphType::CODE;
        else if (response.find("step") != std::string::npos || response.find("therefore") != std::string::npos) likelyType = MorphType::REASONING;
        compiler_.adaptStyle(positive, likelyType);
        
        if (positive) {
            // Positive feedback: create new ThoughtDNA from this interaction
            ThoughtDNA newThought;
            newThought.id = pool_.manipulator().getNextId();
            newThought.semantic_content = response;
            newThought.generation = 1;
            newThought.fitness = 0.7f; // Born fit (user liked it)
            newThought.vitality = 1.0f;
            newThought.born_at = now();
            newThought.last_activated = newThought.born_at;
            
            // Extract triggers from input (what activated this response)
            auto tokens = tokenize(input);
            for (const auto& t : tokens) {
                if (t.size() > 2) newThought.triggers.push_back(t);
                if (newThought.triggers.size() >= 5) break;
            }
            
            // Set emotional genes high (user liked it = positive emotions)
            newThought.setGene(STRAND_EMOTIONAL, EMO_VALENCE, 0.8f);
            newThought.setGene(STRAND_EMOTIONAL, EMO_WARMTH, 0.7f);
            newThought.setGene(STRAND_SEMANTIC, SEM_RELEVANCE, 0.9f);
            
            int newIdx = pool_.add(std::move(newThought));
            
            // Strengthen plasma connections from input triggers to this new thought
            auto resonating = pool_.findResonating(tokens, 5);
            for (const auto& [id, strength] : resonating) {
                plasma_.strengthen(id, newIdx, 0.3f);
            }
        } else {
            // Negative feedback: weaken connections that led to bad response
            auto tokens = tokenize(input);
            auto resonating = pool_.findResonating(tokens, 5);
            for (const auto& [id, strength] : resonating) {
                pool_.get(id).fitness = std::max(0.1f, pool_.get(id).fitness - 0.05f);
                // Mutate to escape local minimum
                pool_.manipulator().mutate(pool_.get(id));
            }
        }
    }


    // ==========================================================================
    // DREAM TICK: Call periodically during idle time
    // ==========================================================================
    void dreamTick() {
        if (!initialized_) return;
        
        dreamer_.checkIdleState();
        
        if (dreamer_.getPhase() != DreamPhase::AWAKE) {
            // Run a dream cycle
            auto insights = dreamer_.dreamCycle(pool_, 2);
            
            // Consolidate memory
            dreamer_.consolidate(plasma_, pool_);
            
            // Create new ThoughtDNA from dream insights
            for (const auto& insight : insights) {
                if (insight.content.empty()) continue;
                
                ThoughtDNA dreamThought;
                dreamThought.id = pool_.manipulator().getNextId();
                dreamThought.semantic_content = insight.content;
                dreamThought.emotional_color = "dreamed";
                dreamThought.generation = totalGenerations_;
                dreamThought.fitness = insight.confidence * 0.5f;
                dreamThought.vitality = 0.6f;
                dreamThought.born_at = now();
                dreamThought.last_activated = dreamThought.born_at;
                
                // Dream thoughts have high novelty
                dreamThought.setGene(STRAND_SEMANTIC, SEM_NOVELTY, insight.novelty);
                dreamThought.setGene(STRAND_SEMANTIC, SEM_DEPTH, 0.6f);
                dreamThought.setGene(STRAND_EMOTIONAL, EMO_CURIOSITY, 0.8f);
                
                // Triggers from insight content
                std::istringstream ss(insight.content);
                std::string word;
                int count = 0;
                while (ss >> word && count < 4) {
                    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                    while (!word.empty() && !std::isalnum(word.back())) word.pop_back();
                    if (word.size() > 3) {
                        dreamThought.triggers.push_back(word);
                        count++;
                    }
                }
                
                dreamThought.domain = "dream";
                pool_.add(std::move(dreamThought));
            }
        }
    }

    // ==========================================================================
    // CHECK FOR DREAM INSIGHTS: Returns insights to share with user
    // ==========================================================================
    std::string getUndeliveredInsight() {
        auto insights = dreamer_.getPendingInsights(1);
        if (insights.empty()) return "";
        return insights[0].content;
    }

    bool hasInsightsToShare() const {
        return dreamer_.hasUndeliveredInsights();
    }

    // ==========================================================================
    // REPORT KNOWLEDGE GAP (called when MK can't answer something)
    // ==========================================================================
    void reportGap(const std::string& domain, const std::string& question) {
        dreamer_.detectGap(domain, question);
    }


    // ==========================================================================
    // STATUS & DIAGNOSTICS
    // ==========================================================================
    bool isInitialized() const { return initialized_; }
    
    std::string getStats() const {
        std::ostringstream ss;
        ss << "╔═══════════════════════════════════════╗\n"
           << "║       GENESIS ENGINE STATUS           ║\n"
           << "╚═══════════════════════════════════════╝\n"
           << "  Generations: " << totalGenerations_ << " | Absorptions: " << totalAbsorptions_ << "\n"
           << "  " << pool_.livingCount() << "/" << pool_.size() << " thoughts alive (born: "
           << pool_.totalBorn() << " died: " << pool_.totalDied() << ")\n"
           << "  " << plasma_.getStats() << "\n"
           << "  " << quantum_.getStats() << "\n"
           << "  " << temporal_.getStats() << "\n"
           << "  " << compiler_.getStats() << "\n"
           << "  " << arena_.getStats()
           << "  " << dreamer_.getStats() << "\n";
        return ss.str();
    }

    std::string getTrace() const { return lastTrace_; }
    std::string getLastOutput() const { return lastOutput_; }
    
    std::string getIdentity() const {
        std::ostringstream ss;
        Archetype dominant = arena_.getDominantArchetype();
        ss << "Dominant archetype: " << arena_.archetypeToString(dominant) << "\n"
           << "Mood: " << temporal_.getMood().toString() << "\n"
           << "Dream phase: " << dreamer_.phaseToString() << "\n"
           << "Prediction accuracy: " << (int)(temporal_.getPredictionAccuracy() * 100) << "%\n";
        return ss.str();
    }

    // ==========================================================================
    // SAVE / LOAD: Persist all subsystem state
    // ==========================================================================
    void save() {
        // Create data directory
        std::system(("mkdir -p " + dataDir_).c_str());
        
        plasma_.save(dataDir_ + "/plasma.dat");
        temporal_.save(dataDir_ + "/temporal.dat");
        arena_.save(dataDir_ + "/arena.dat");
        dreamer_.save(dataDir_ + "/dreams.dat");
        
        // Save pool summary (most important thoughts)
        std::ofstream poolOut(dataDir_ + "/pool.dat");
        if (poolOut.is_open()) {
            auto living = pool_.getLiving();
            // Sort by fitness
            std::sort(living.begin(), living.end(), [this](int a, int b) {
                return pool_.get(a).fitness > pool_.get(b).fitness;
            });
            
            int saveCount = std::min((int)living.size(), 2000);
            poolOut << saveCount << "\n";
            for (int i = 0; i < saveCount; i++) {
                const auto& t = pool_.get(living[i]);
                std::string folded = pool_.manipulator().fold(t);
                poolOut << folded << "\n";
            }
        }
        
        std::cout << "[GENESIS] State saved to " << dataDir_ << "/\n";
    }

    void load() {
        plasma_.load(dataDir_ + "/plasma.dat");
        temporal_.load(dataDir_ + "/temporal.dat");
        arena_.load(dataDir_ + "/arena.dat");
        dreamer_.load(dataDir_ + "/dreams.dat");
        
        // Load pool
        std::ifstream poolIn(dataDir_ + "/pool.dat");
        if (poolIn.is_open()) {
            int count;
            poolIn >> count;
            poolIn.ignore();
            int loaded = 0;
            for (int i = 0; i < count; i++) {
                std::string line;
                std::getline(poolIn, line);
                if (!line.empty()) {
                    ThoughtDNA t = pool_.manipulator().unfold(line);
                    pool_.add(std::move(t));
                    loaded++;
                }
            }
            if (loaded > 0) {
                std::cout << "[GENESIS] Loaded " << loaded << " thoughts from disk.\n";
            }
        }
    }

    // Accessors for integration
    MKTemporalMind& getTemporal() { return temporal_; }
    MKAdversarialChamber& getArena() { return arena_; }
    MKDreamSynthesizer& getDreamer() { return dreamer_; }
    MKSynapticPlasma& getPlasma() { return plasma_; }
    MKThoughtPool& getPool() { return pool_; }
    const MKTemporalMind& getTemporal() const { return temporal_; }
};

#endif // MK_GENESIS_ENGINE_CPP
