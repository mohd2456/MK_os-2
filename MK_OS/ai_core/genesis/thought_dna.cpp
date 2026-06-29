#ifndef MK_GENESIS_THOUGHT_DNA_CPP
#define MK_GENESIS_THOUGHT_DNA_CPP

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
#include <numeric>

// ===================================================================================
// MK GENESIS ENGINE — THOUGHT-DNA
// ===================================================================================
// The fundamental unit of Genesis. Unlike Prometheus droplets which live on a single
// precision axis, Thought-DNA is a MULTI-DIMENSIONAL HELIX that encodes:
//
//   STRAND A (Semantic Helix): What this thought MEANS
//   STRAND B (Emotional Helix): What this thought FEELS
//   STRAND C (Logical Helix):   How this thought CONNECTS
//   STRAND D (Executable Helix): What this thought DOES
//
// All four strands twist together. A thought is NEVER just code or just emotion —
// it is ALL FOUR simultaneously. The output depends on which strand you READ.
//
// KEY INNOVATION: Cross-strand mutation. When emotion strand mutates, it AFFECTS
// the code strand. When logic connects two thoughts, their emotional and semantic
// strands BLEED into each other. This creates emergent behavior no linear system can.
//
// Thought-DNA can:
//   - REPLICATE (spawn variants)
//   - MUTATE (random changes driven by environmental pressure)
//   - CROSSOVER (two thoughts merge creating a child with traits of both)
//   - TRANSCRIBE (read one strand to produce output)
//   - FOLD (compress into compact form for memory storage)
//   - UNFOLD (decompress and reconstruct full helix)
//   - RESONATE (vibrate with other DNA strands that are compatible)
//   - APOPTOSIS (programmed death when fitness drops)
// ===================================================================================


// --- Helix Dimensions ---
// Each strand has 8 genes (floating point values that encode properties)
static constexpr int GENES_PER_STRAND = 8;
static constexpr int NUM_STRANDS = 4;
static constexpr int TOTAL_GENES = GENES_PER_STRAND * NUM_STRANDS; // 32 genes per thought

// Strand indices
static constexpr int STRAND_SEMANTIC  = 0;
static constexpr int STRAND_EMOTIONAL = 1;
static constexpr int STRAND_LOGICAL   = 2;
static constexpr int STRAND_EXECUTABLE = 3;

// Gene meanings within SEMANTIC strand
static constexpr int SEM_SUBJECT    = 0;  // What it's about (hashed)
static constexpr int SEM_ABSTRACTON = 1;  // How abstract (0=concrete, 1=abstract)
static constexpr int SEM_NOVELTY    = 2;  // How new/surprising
static constexpr int SEM_DEPTH      = 3;  // How deep the thought goes
static constexpr int SEM_BREADTH    = 4;  // How many topics it touches
static constexpr int SEM_CERTAINTY  = 5;  // How sure we are
static constexpr int SEM_RELEVANCE  = 6;  // How relevant to current context
static constexpr int SEM_DENSITY    = 7;  // Information density

// Gene meanings within EMOTIONAL strand
static constexpr int EMO_VALENCE    = 0;  // Positive/negative (-1 to 1)
static constexpr int EMO_AROUSAL    = 1;  // Calm/excited
static constexpr int EMO_DOMINANCE  = 2;  // Submissive/dominant
static constexpr int EMO_WARMTH     = 3;  // Cold/warm
static constexpr int EMO_HUMOR      = 4;  // Serious/funny
static constexpr int EMO_EMPATHY    = 5;  // Self-focused/other-focused
static constexpr int EMO_CURIOSITY  = 6;  // Bored/curious
static constexpr int EMO_CONFIDENCE = 7;  // Uncertain/confident

// Gene meanings within LOGICAL strand
static constexpr int LOG_CAUSALITY  = 0;  // Cause-effect strength
static constexpr int LOG_TEMPORAL   = 1;  // Past/present/future orientation
static constexpr int LOG_CONDITIONAL= 2;  // If-then strength
static constexpr int LOG_NEGATION   = 3;  // Affirmation/negation
static constexpr int LOG_COMPARISON = 4;  // Comparison strength
static constexpr int LOG_SEQUENCE   = 5;  // Sequential ordering importance
static constexpr int LOG_RECURSION  = 6;  // Self-referential depth
static constexpr int LOG_ABSTRACTION= 7;  // Concrete/abstract logic level


// Gene meanings within EXECUTABLE strand
static constexpr int EXE_PRECISION  = 0;  // How precise/formal the output
static constexpr int EXE_COMPLEXITY = 1;  // Simple/complex execution
static constexpr int EXE_LANGUAGE   = 2;  // Output language preference (hashed)
static constexpr int EXE_STRUCTURE  = 3;  // Flat/hierarchical structure
static constexpr int EXE_VERBOSITY  = 4;  // Terse/verbose output
static constexpr int EXE_SAFETY     = 5;  // Risk level of execution
static constexpr int EXE_TESTABLE   = 6;  // Can be verified/tested
static constexpr int EXE_COMPOSABLE = 7;  // Can combine with other outputs

// ===================================================================================
// ThoughtDNA — The living helix
// ===================================================================================
struct ThoughtDNA {
    // The 4x8 gene matrix (the actual DNA)
    std::array<std::array<float, GENES_PER_STRAND>, NUM_STRANDS> helix;
    
    // Content payloads (what the DNA encodes)
    std::string semantic_content;    // The meaning in words
    std::string emotional_color;     // The feeling tag
    std::string logical_structure;   // The logical skeleton
    std::string executable_payload;  // The code/action

    // Metadata
    int generation;
    int id;
    float fitness;
    float vitality;           // 0=dead, 1=peak life
    float mutation_rate;      // How likely to mutate (adapts over time)
    float resonance_freq;     // Natural vibration frequency for matching
    long long born_at;
    long long last_activated;
    int times_used;
    int offspring_count;
    int strand_crossovers;    // How many times strands bled into each other
    
    // Ancestry
    int parent_a_id;
    int parent_b_id;
    
    // Bonds (connections to other ThoughtDNA)
    std::vector<int> bonds;
    std::vector<float> bond_strengths;
    
    // Tags for fast lookup
    std::vector<std::string> triggers;
    std::string domain;
    
    // Folded state (compressed for memory)
    bool is_folded;
    std::string folded_hash;

    ThoughtDNA() : generation(0), id(-1), fitness(0.5f), vitality(1.0f),
                   mutation_rate(0.05f), resonance_freq(0.5f), born_at(0),
                   last_activated(0), times_used(0), offspring_count(0),
                   strand_crossovers(0), parent_a_id(-1), parent_b_id(-1),
                   is_folded(false) {
        // Initialize helix with neutral values
        for (auto& strand : helix) {
            strand.fill(0.5f);
        }
    }


    // --- Strand reading ---
    float getGene(int strand, int gene) const {
        return helix[strand][gene];
    }
    
    void setGene(int strand, int gene, float value) {
        helix[strand][gene] = std::clamp(value, 0.0f, 1.0f);
    }

    // Get the dominant strand (which aspect is strongest)
    int dominantStrand() const {
        float sums[NUM_STRANDS] = {0};
        for (int s = 0; s < NUM_STRANDS; s++) {
            for (int g = 0; g < GENES_PER_STRAND; g++) {
                sums[s] += std::abs(helix[s][g] - 0.5f); // Distance from neutral
            }
        }
        return (int)(std::max_element(sums, sums + NUM_STRANDS) - sums);
    }

    // Overall activation energy (how "alive" this thought is right now)
    float activationEnergy() const {
        float energy = 0.0f;
        for (const auto& strand : helix) {
            for (float g : strand) {
                energy += std::abs(g - 0.5f);
            }
        }
        return (energy / TOTAL_GENES) * vitality;
    }

    // Compatibility score with another ThoughtDNA (0-1)
    float compatibility(const ThoughtDNA& other) const {
        float similarity = 0.0f;
        for (int s = 0; s < NUM_STRANDS; s++) {
            for (int g = 0; g < GENES_PER_STRAND; g++) {
                float diff = std::abs(helix[s][g] - other.helix[s][g]);
                similarity += (1.0f - diff);
            }
        }
        return similarity / TOTAL_GENES;
    }

    // Resonance: how much two DNAs "want" to interact
    // Based on complementary frequencies, not just similarity
    float resonanceWith(const ThoughtDNA& other) const {
        // Complementary = frequencies that harmonize (ratios near simple fractions)
        float freqRatio = resonance_freq / (other.resonance_freq + 0.001f);
        float harmonics[] = {1.0f, 0.5f, 2.0f, 1.5f, 0.667f, 0.75f, 1.333f};
        float bestHarmonic = 99.0f;
        for (float h : harmonics) {
            float dist = std::abs(freqRatio - h);
            if (dist < bestHarmonic) bestHarmonic = dist;
        }
        float harmonicScore = 1.0f - std::min(bestHarmonic, 1.0f);
        
        // Also consider strand complementarity (opposites attract for crossover)
        float complement = 0.0f;
        for (int s = 0; s < NUM_STRANDS; s++) {
            for (int g = 0; g < GENES_PER_STRAND; g++) {
                // Complementary: one high where other is low
                complement += std::abs(helix[s][g] - (1.0f - other.helix[s][g]));
            }
        }
        float complementScore = 1.0f - (complement / TOTAL_GENES);
        
        return (harmonicScore * 0.6f + complementScore * 0.4f) * vitality * other.vitality;
    }

    // Is this thought still alive?
    bool isAlive() const { return vitality > 0.01f && !is_folded; }
    
    // Is this thought mostly about emotion?
    bool isEmotional() const { return dominantStrand() == STRAND_EMOTIONAL; }
    
    // Is this thought mostly about logic?
    bool isLogical() const { return dominantStrand() == STRAND_LOGICAL; }
    
    // Is this thought mostly executable?
    bool isExecutable() const { return dominantStrand() == STRAND_EXECUTABLE; }
    
    // Is this thought mostly semantic/meaning?
    bool isSemantic() const { return dominantStrand() == STRAND_SEMANTIC; }
};


// ===================================================================================
// ThoughtDNA Manipulator — Mutation, Crossover, Transcription
// ===================================================================================
class MKThoughtManipulator {
private:
    std::mt19937 rng_;
    int nextId_;
    int totalMutations_;
    int totalCrossovers_;
    int totalTranscriptions_;

    long long now() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

public:
    MKThoughtManipulator() : nextId_(0), totalMutations_(0),
                             totalCrossovers_(0), totalTranscriptions_(0) {
        rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
    }

    int getNextId() { return nextId_++; }
    void setNextId(int id) { nextId_ = id; }

    // --- MUTATE ---
    // Random mutation with cross-strand bleed
    // KEY INNOVATION: mutation in one strand BLEEDS into adjacent strands
    void mutate(ThoughtDNA& dna) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::normal_distribution<float> noise(0.0f, dna.mutation_rate);
        
        // Pick a random strand to mutate
        int targetStrand = std::uniform_int_distribution<int>(0, NUM_STRANDS - 1)(rng_);
        int targetGene = std::uniform_int_distribution<int>(0, GENES_PER_STRAND - 1)(rng_);
        
        // Primary mutation
        float delta = noise(rng_);
        dna.helix[targetStrand][targetGene] = std::clamp(
            dna.helix[targetStrand][targetGene] + delta, 0.0f, 1.0f);
        
        // CROSS-STRAND BLEED: adjacent strands feel the mutation (at 30% strength)
        float bleed = delta * 0.3f;
        int leftStrand = (targetStrand + NUM_STRANDS - 1) % NUM_STRANDS;
        int rightStrand = (targetStrand + 1) % NUM_STRANDS;
        dna.helix[leftStrand][targetGene] = std::clamp(
            dna.helix[leftStrand][targetGene] + bleed, 0.0f, 1.0f);
        dna.helix[rightStrand][targetGene] = std::clamp(
            dna.helix[rightStrand][targetGene] + bleed, 0.0f, 1.0f);
        
        dna.strand_crossovers++;
        totalMutations_++;
        
        // Adaptive mutation rate: successful thoughts mutate less
        if (dna.fitness > 0.7f) {
            dna.mutation_rate *= 0.95f; // Stabilize winners
        } else if (dna.fitness < 0.3f) {
            dna.mutation_rate *= 1.1f;  // Shake up losers
        }
        dna.mutation_rate = std::clamp(dna.mutation_rate, 0.01f, 0.3f);
    }


    // --- CROSSOVER ---
    // Two ThoughtDNAs merge to create a child
    // Innovation: not just gene-level crossover but STRAND-LEVEL mixing
    // Child can get semantic from parent A, emotion from parent B, etc.
    ThoughtDNA crossover(const ThoughtDNA& parentA, const ThoughtDNA& parentB) {
        ThoughtDNA child;
        child.id = getNextId();
        child.generation = std::max(parentA.generation, parentB.generation) + 1;
        child.born_at = now();
        child.last_activated = child.born_at;
        child.parent_a_id = parentA.id;
        child.parent_b_id = parentB.id;

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Strategy selection: 40% strand-swap, 40% gene-interleave, 20% blend
        float strategy = dist(rng_);
        
        if (strategy < 0.4f) {
            // STRAND SWAP: each strand comes wholly from one parent
            for (int s = 0; s < NUM_STRANDS; s++) {
                const auto& donor = (dist(rng_) < 0.5f) ? parentA : parentB;
                child.helix[s] = donor.helix[s];
            }
        } else if (strategy < 0.8f) {
            // GENE INTERLEAVE: alternate genes from each parent
            for (int s = 0; s < NUM_STRANDS; s++) {
                for (int g = 0; g < GENES_PER_STRAND; g++) {
                    child.helix[s][g] = (g % 2 == 0) ?
                        parentA.helix[s][g] : parentB.helix[s][g];
                }
            }
        } else {
            // BLEND: weighted average based on fitness
            float totalFit = parentA.fitness + parentB.fitness + 0.001f;
            float wA = parentA.fitness / totalFit;
            float wB = parentB.fitness / totalFit;
            for (int s = 0; s < NUM_STRANDS; s++) {
                for (int g = 0; g < GENES_PER_STRAND; g++) {
                    child.helix[s][g] = parentA.helix[s][g] * wA + parentB.helix[s][g] * wB;
                }
            }
        }

        // Content inheritance: semantic from higher-fitness parent, executable from more precise
        if (parentA.fitness >= parentB.fitness) {
            child.semantic_content = parentA.semantic_content;
            child.logical_structure = parentA.logical_structure;
        } else {
            child.semantic_content = parentB.semantic_content;
            child.logical_structure = parentB.logical_structure;
        }
        
        // Executable from the more precise parent
        if (parentA.getGene(STRAND_EXECUTABLE, EXE_PRECISION) >=
            parentB.getGene(STRAND_EXECUTABLE, EXE_PRECISION)) {
            child.executable_payload = parentA.executable_payload;
        } else {
            child.executable_payload = parentB.executable_payload;
        }
        
        // Emotion blends (both parents' feelings mix)
        child.emotional_color = parentA.emotional_color + "+" + parentB.emotional_color;
        
        // Inherit triggers from both parents (union)
        child.triggers = parentA.triggers;
        for (const auto& t : parentB.triggers) {
            if (std::find(child.triggers.begin(), child.triggers.end(), t) == child.triggers.end()) {
                child.triggers.push_back(t);
            }
        }
        
        // Resonance frequency: harmonic mean of parents
        child.resonance_freq = 2.0f * parentA.resonance_freq * parentB.resonance_freq /
            (parentA.resonance_freq + parentB.resonance_freq + 0.001f);
        
        // Mutation rate: average with slight increase (new life is volatile)
        child.mutation_rate = (parentA.mutation_rate + parentB.mutation_rate) / 2.0f * 1.1f;
        child.mutation_rate = std::clamp(child.mutation_rate, 0.01f, 0.2f);

        // Initial fitness: slight below parents average (must prove itself)
        child.fitness = (parentA.fitness + parentB.fitness) / 2.0f * 0.9f;
        
        totalCrossovers_++;
        return child;
    }


    // --- TRANSCRIBE ---
    // Read specific strands to produce output text
    // This is where thought becomes language/code
    std::string transcribe(const ThoughtDNA& dna, int primaryStrand) {
        totalTranscriptions_++;
        
        // The output is shaped by ALL strands, but led by the primary
        std::string output;
        
        switch (primaryStrand) {
            case STRAND_SEMANTIC:
                output = dna.semantic_content;
                // Emotional bleed: if high warmth, add softening words
                if (dna.getGene(STRAND_EMOTIONAL, EMO_WARMTH) > 0.7f) {
                    if (!output.empty()) output = "honestly, " + output;
                }
                // Logical bleed: if high certainty, strengthen claims
                if (dna.getGene(STRAND_LOGICAL, LOG_CAUSALITY) > 0.7f &&
                    dna.getGene(STRAND_SEMANTIC, SEM_CERTAINTY) > 0.7f) {
                    output += " — and that's definitely the case";
                }
                break;
                
            case STRAND_EMOTIONAL:
                output = dna.emotional_color;
                // Semantic bleed: ground emotion in meaning
                if (!dna.semantic_content.empty() && dna.getGene(STRAND_SEMANTIC, SEM_DEPTH) > 0.5f) {
                    output += " about " + dna.semantic_content;
                }
                break;
                
            case STRAND_LOGICAL:
                output = dna.logical_structure;
                // Executable bleed: if high precision, add formal structure
                if (dna.getGene(STRAND_EXECUTABLE, EXE_PRECISION) > 0.7f) {
                    output = "therefore: " + output;
                }
                break;
                
            case STRAND_EXECUTABLE:
                output = dna.executable_payload;
                // No bleed into code — code must be pure
                break;
        }
        
        return output;
    }

    // --- FOLD ---
    // Compress ThoughtDNA for long-term memory storage
    std::string fold(const ThoughtDNA& dna) {
        std::ostringstream ss;
        ss << dna.id << "|";
        for (int s = 0; s < NUM_STRANDS; s++) {
            for (int g = 0; g < GENES_PER_STRAND; g++) {
                ss << (int)(dna.helix[s][g] * 255) << ",";
            }
        }
        ss << "|" << dna.semantic_content << "|" << dna.emotional_color
           << "|" << dna.logical_structure << "|" << dna.executable_payload
           << "|" << dna.fitness << "|" << dna.generation;
        return ss.str();
    }

    // --- UNFOLD ---
    // Decompress from storage back into full ThoughtDNA
    ThoughtDNA unfold(const std::string& folded) {
        ThoughtDNA dna;
        // Parse the folded format (id|genes|sem|emo|log|exe|fitness|gen)
        std::istringstream ss(folded);
        std::string segment;
        
        // ID
        if (std::getline(ss, segment, '|')) {
            try { dna.id = std::stoi(segment); } catch(...) { dna.id = getNextId(); }
        }
        
        // Genes
        if (std::getline(ss, segment, '|')) {
            std::istringstream gs(segment);
            std::string val;
            int idx = 0;
            while (std::getline(gs, val, ',') && idx < TOTAL_GENES) {
                try {
                    int v = std::stoi(val);
                    dna.helix[idx / GENES_PER_STRAND][idx % GENES_PER_STRAND] = v / 255.0f;
                } catch(...) {}
                idx++;
            }
        }
        
        // Content fields
        if (std::getline(ss, segment, '|')) dna.semantic_content = segment;
        if (std::getline(ss, segment, '|')) dna.emotional_color = segment;
        if (std::getline(ss, segment, '|')) dna.logical_structure = segment;
        if (std::getline(ss, segment, '|')) dna.executable_payload = segment;
        if (std::getline(ss, segment, '|')) {
            try { dna.fitness = std::stof(segment); } catch(...) {}
        }
        if (std::getline(ss, segment, '|')) {
            try { dna.generation = std::stoi(segment); } catch(...) {}
        }
        
        dna.is_folded = false;
        dna.born_at = now();
        dna.last_activated = dna.born_at;
        return dna;
    }

    // --- APOPTOSIS ---
    // Programmed cell death: mark thought for removal
    void apoptosis(ThoughtDNA& dna) {
        dna.vitality = 0.0f;
        dna.is_folded = true;
    }

    // --- Stats ---
    int getTotalMutations() const { return totalMutations_; }
    int getTotalCrossovers() const { return totalCrossovers_; }
    int getTotalTranscriptions() const { return totalTranscriptions_; }
    int getCurrentId() const { return nextId_; }
};


// ===================================================================================
// ThoughtPool — The genome of all living thoughts
// ===================================================================================
class MKThoughtPool {
private:
    std::vector<ThoughtDNA> thoughts_;
    std::unordered_map<std::string, std::vector<int>> triggerIndex_;
    std::unordered_map<std::string, std::vector<int>> domainIndex_;
    MKThoughtManipulator manipulator_;
    int maxPopulation_;
    int totalBorn_;
    int totalDied_;

public:
    MKThoughtPool(int maxPop = 10000) : maxPopulation_(maxPop), totalBorn_(0), totalDied_(0) {}

    // Add a new thought to the pool
    int add(ThoughtDNA dna) {
        if (dna.id < 0) dna.id = manipulator_.getNextId();
        int idx = (int)thoughts_.size();
        thoughts_.push_back(std::move(dna));
        
        // Index by triggers
        for (const auto& t : thoughts_[idx].triggers) {
            triggerIndex_[t].push_back(idx);
        }
        // Index by domain
        if (!thoughts_[idx].domain.empty()) {
            domainIndex_[thoughts_[idx].domain].push_back(idx);
        }
        
        totalBorn_++;
        return idx;
    }

    // Get thought by index
    ThoughtDNA& get(int idx) { return thoughts_[idx]; }
    const ThoughtDNA& get(int idx) const { return thoughts_[idx]; }

    // Find thoughts that resonate with input triggers
    std::vector<std::pair<int, float>> findResonating(const std::vector<std::string>& triggers,
                                                       int maxResults = 20) const {
        std::unordered_map<int, float> scores;
        
        for (const auto& trigger : triggers) {
            auto it = triggerIndex_.find(trigger);
            if (it != triggerIndex_.end()) {
                for (int idx : it->second) {
                    if (thoughts_[idx].isAlive()) {
                        scores[idx] += thoughts_[idx].activationEnergy();
                    }
                }
            }
        }
        
        // Sort by score
        std::vector<std::pair<int, float>> results(scores.begin(), scores.end());
        std::sort(results.begin(), results.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        if ((int)results.size() > maxResults) results.resize(maxResults);
        return results;
    }

    // Find thoughts by domain
    std::vector<int> findByDomain(const std::string& domain, int maxResults = 20) const {
        std::vector<int> results;
        auto it = domainIndex_.find(domain);
        if (it != domainIndex_.end()) {
            for (int idx : it->second) {
                if (thoughts_[idx].isAlive()) {
                    results.push_back(idx);
                    if ((int)results.size() >= maxResults) break;
                }
            }
        }
        return results;
    }

    // Get all living thoughts
    std::vector<int> getLiving() const {
        std::vector<int> alive;
        for (int i = 0; i < (int)thoughts_.size(); i++) {
            if (thoughts_[i].isAlive()) alive.push_back(i);
        }
        return alive;
    }

    // Population pressure: cull weakest if over capacity
    void populationControl() {
        auto alive = getLiving();
        if ((int)alive.size() <= maxPopulation_) return;

        // Sort by fitness (ascending — weakest first)
        std::sort(alive.begin(), alive.end(), [this](int a, int b) {
            return thoughts_[a].fitness < thoughts_[b].fitness;
        });

        // Kill the weakest 20%
        int toKill = (int)alive.size() - maxPopulation_;
        for (int i = 0; i < toKill; i++) {
            manipulator_.apoptosis(thoughts_[alive[i]]);
            totalDied_++;
        }
    }

    // Natural decay: reduce vitality of unused thoughts
    void decay(float rate = 0.001f) {
        long long currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        for (auto& t : thoughts_) {
            if (!t.isAlive()) continue;
            long long age = currentTime - t.last_activated;
            if (age > 60000) { // Inactive for more than 60 seconds
                t.vitality -= rate;
                if (t.vitality <= 0.0f) {
                    manipulator_.apoptosis(t);
                    totalDied_++;
                }
            }
        }
    }

    MKThoughtManipulator& manipulator() { return manipulator_; }
    int size() const { return (int)thoughts_.size(); }
    int livingCount() const { return (int)getLiving().size(); }
    int totalBorn() const { return totalBorn_; }
    int totalDied() const { return totalDied_; }
};

#endif // MK_GENESIS_THOUGHT_DNA_CPP
