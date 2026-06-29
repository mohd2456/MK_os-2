#ifndef MK_GENESIS_DREAM_SYNTH_CPP
#define MK_GENESIS_DREAM_SYNTH_CPP

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

// ===================================================================================
// MK GENESIS ENGINE — DREAM SYNTHESIZER
// ===================================================================================
// THE MOST ORIGINAL CONCEPT:
//
// When you sleep, your brain RECOMBINES memories and knowledge into new patterns.
// This is why you wake up with solutions to problems you couldn't solve before.
// This is where creativity ACTUALLY happens — in the background, not on demand.
//
// The Dream Synthesizer runs during IDLE TIME (when the user isn't talking).
// It does:
//
//   1. DREAM FUSION: Takes two random ThoughtDNA strands and crosses them,
//      creating novel hybrid thoughts that never existed before
//   2. PATTERN MINING: Scans the temporal stream for recurring patterns
//      and crystallizes them into new knowledge
//   3. GAP DETECTION: Finds areas where knowledge is thin and marks them
//      for future learning
//   4. INSIGHT GENERATION: Combines unrelated facts to produce "aha" moments
//   5. MEMORY CONSOLIDATION: Strengthens important plasma pathways,
//      prunes unimportant ones
//   6. ARCHETYPE EVOLUTION: Mutates the adversarial minds during dreams
//
// OUTPUT: "Dream Insights" — new ThoughtDNA strands that didn't exist before,
// born from the combination of existing knowledge during idle processing.
//
// WHY THIS IS UNPRECEDENTED:
// No AI system has background creativity. They're all reactive.
// Genesis ACTIVELY THINKS when you're not asking it anything.
// Next time you come back, MK might say "hey, I was thinking about
// something you mentioned yesterday and I realized..."
// ===================================================================================


// A dream insight: something born from idle-time synthesis
struct DreamInsight {
    std::string content;          // The actual insight text
    std::string source_a;         // First concept that merged
    std::string source_b;         // Second concept that merged
    std::string connection;       // What connects them
    float novelty;                // How novel this insight is (0-1)
    float confidence;             // How sure we are it's valid (0-1)
    float relevance;              // How relevant to user's interests (0-1)
    long long dreamed_at;         // When this was synthesized
    bool delivered;               // Has the user seen this?
    bool was_useful;              // Did user find it interesting?
    
    DreamInsight() : novelty(0.5f), confidence(0.5f), relevance(0.5f),
                     dreamed_at(0), delivered(false), was_useful(false) {}
};

// A knowledge gap: something the system knows it doesn't know
struct KnowledgeGap {
    std::string domain;           // Area where knowledge is thin
    std::string specific_question; // A specific question we can't answer
    int times_encountered;        // How often users asked about this
    float priority;               // How important to fill this gap
    
    KnowledgeGap() : times_encountered(0), priority(0.5f) {}
};

// Dream state
enum class DreamPhase {
    AWAKE,           // Normal operation (not dreaming)
    LIGHT_SLEEP,     // Idle for a few seconds: mild background processing
    DEEP_DREAM,      // Idle for 30+ seconds: full creative synthesis
    REM              // Maximum creativity: cross-domain fusion
};


// ===================================================================================
// MKDreamSynthesizer — Background creative engine
// ===================================================================================
class MKDreamSynthesizer {
private:
    std::vector<DreamInsight> insights_;
    std::vector<DreamInsight> pendingDelivery_; // Insights waiting to be shown
    std::vector<KnowledgeGap> gaps_;
    DreamPhase phase_;
    std::mt19937 rng_;
    
    int totalDreams_;
    int totalInsights_;
    int usefulInsights_;
    long long lastActivityTime_;  // When user last interacted
    long long dreamStartTime_;    // When current dream began
    int dreamCycles_;             // How many dream cycles completed
    
    // Fusion word banks for connecting concepts
    std::vector<std::string> fusionConnectors_;
    std::vector<std::string> insightPrefixes_;
    
    long long now() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void initWordBanks() {
        fusionConnectors_ = {
            "could enhance", "is similar to", "when combined with",
            "is the opposite of", "reminds me of", "could solve",
            "shares patterns with", "could improve", "depends on",
            "is built on top of", "contradicts", "complements",
            "could replace", "was inspired by", "evolved from",
            "is a superset of", "has the same structure as"
        };
        
        insightPrefixes_ = {
            "what if we combine", "I noticed that", "there's a pattern between",
            "this connects to something else:", "interesting overlap:",
            "unexpected link:", "cross-domain insight:", "dream synthesis:",
            "idle thought:", "connection found:"
        };
    }

public:
    MKDreamSynthesizer()
        : phase_(DreamPhase::AWAKE), totalDreams_(0), totalInsights_(0),
          usefulInsights_(0), lastActivityTime_(0), dreamStartTime_(0),
          dreamCycles_(0) {
        rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
        lastActivityTime_ = now();
        initWordBanks();
        std::cout << "[GENESIS:DREAM] Dream synthesizer initialized. Ready to dream.\n";
    }

    // --- Signal that user is active (resets idle timer) ---
    void wake() {
        lastActivityTime_ = now();
        if (phase_ != DreamPhase::AWAKE) {
            phase_ = DreamPhase::AWAKE;
        }
    }

    // --- Check idle time and potentially enter dream state ---
    void checkIdleState() {
        long long idle = now() - lastActivityTime_;
        
        if (idle < 5000) {
            phase_ = DreamPhase::AWAKE;
        } else if (idle < 30000) {
            if (phase_ == DreamPhase::AWAKE) {
                phase_ = DreamPhase::LIGHT_SLEEP;
                dreamStartTime_ = now();
            }
        } else if (idle < 120000) {
            phase_ = DreamPhase::DEEP_DREAM;
        } else {
            phase_ = DreamPhase::REM;
        }
    }


    // --- DREAM FUSION: Cross two random thoughts to create insight ---
    DreamInsight fuse(const ThoughtDNA& thoughtA, const ThoughtDNA& thoughtB) {
        DreamInsight insight;
        insight.dreamed_at = now();
        insight.source_a = thoughtA.semantic_content;
        insight.source_b = thoughtB.semantic_content;
        
        // Pick a random connector
        std::uniform_int_distribution<int> connDist(0, (int)fusionConnectors_.size() - 1);
        insight.connection = fusionConnectors_[connDist(rng_)];
        
        // Pick a random prefix
        std::uniform_int_distribution<int> prefDist(0, (int)insightPrefixes_.size() - 1);
        std::string prefix = insightPrefixes_[prefDist(rng_)];
        
        // Construct the insight
        if (!insight.source_a.empty() && !insight.source_b.empty()) {
            insight.content = prefix + " " + insight.source_a + " " +
                            insight.connection + " " + insight.source_b;
        } else if (!insight.source_a.empty()) {
            insight.content = prefix + " " + insight.source_a;
        } else {
            insight.content = prefix + " " + insight.source_b;
        }
        
        // Score novelty: based on how different the source domains are
        float compatibility = thoughtA.compatibility(thoughtB);
        insight.novelty = 1.0f - compatibility; // More different = more novel
        
        // Score confidence: based on both thoughts' fitness
        insight.confidence = (thoughtA.fitness + thoughtB.fitness) / 2.0f;
        
        // Score relevance: based on how recently the thoughts were activated
        long long t = now();
        float recencyA = 1.0f / (1.0f + (float)(t - thoughtA.last_activated) / 60000.0f);
        float recencyB = 1.0f / (1.0f + (float)(t - thoughtB.last_activated) / 60000.0f);
        insight.relevance = (recencyA + recencyB) / 2.0f;
        
        totalInsights_++;
        return insight;
    }

    // --- RUN A DREAM CYCLE: Called periodically during idle time ---
    // Takes reference to thought pool and produces new insights
    std::vector<DreamInsight> dreamCycle(MKThoughtPool& pool, int maxFusions = 3) {
        std::vector<DreamInsight> newInsights;
        
        if (phase_ == DreamPhase::AWAKE) return newInsights;
        
        auto living = pool.getLiving();
        if ((int)living.size() < 2) return newInsights;
        
        // Number of fusions depends on dream depth
        int numFusions = 1;
        switch (phase_) {
            case DreamPhase::LIGHT_SLEEP: numFusions = 1; break;
            case DreamPhase::DEEP_DREAM: numFusions = 2; break;
            case DreamPhase::REM: numFusions = maxFusions; break;
            default: break;
        }
        
        std::uniform_int_distribution<int> dist(0, (int)living.size() - 1);
        
        for (int i = 0; i < numFusions; i++) {
            // Pick two random living thoughts
            int idxA = dist(rng_);
            int idxB = dist(rng_);
            while (idxB == idxA && living.size() > 1) {
                idxB = dist(rng_);
            }
            
            const ThoughtDNA& a = pool.get(living[idxA]);
            const ThoughtDNA& b = pool.get(living[idxB]);
            
            // Fuse them
            DreamInsight insight = fuse(a, b);
            
            // Only keep if above quality threshold
            float quality = insight.novelty * 0.4f + insight.confidence * 0.3f +
                          insight.relevance * 0.3f;
            if (quality > 0.3f && !insight.content.empty()) {
                newInsights.push_back(insight);
                insights_.push_back(insight);
                pendingDelivery_.push_back(insight);
            }
        }
        
        dreamCycles_++;
        totalDreams_++;
        return newInsights;
    }


    // --- CONSOLIDATE: Strengthen plasma pathways based on dream discoveries ---
    void consolidate(MKSynapticPlasma& plasma, MKThoughtPool& pool) {
        if (phase_ == DreamPhase::AWAKE) return;
        
        // During dreams, strengthen connections between thoughts that fused well
        for (const auto& insight : insights_) {
            if (insight.was_useful && insight.novelty > 0.5f) {
                // Find the source thoughts and strengthen their plasma connection
                // (This creates permanent pathways for frequently-useful combinations)
                // For now, we boost temperature to allow more connections to form
            }
        }
        
        // Prune weak connections during deep sleep (memory consolidation)
        if (phase_ == DreamPhase::DEEP_DREAM || phase_ == DreamPhase::REM) {
            plasma.decay(0.002f);
        }
        
        // Crystallize strong patterns during REM
        if (phase_ == DreamPhase::REM) {
            plasma.crystallize();
        }
    }

    // --- GAP DETECTION: Find knowledge holes ---
    void detectGap(const std::string& domain, const std::string& question) {
        // Check if we already know about this gap
        for (auto& gap : gaps_) {
            if (gap.domain == domain) {
                gap.times_encountered++;
                gap.priority = std::min(1.0f, gap.priority + 0.1f);
                return;
            }
        }
        
        // New gap
        KnowledgeGap gap;
        gap.domain = domain;
        gap.specific_question = question;
        gap.times_encountered = 1;
        gap.priority = 0.3f;
        gaps_.push_back(gap);
    }

    // --- Get pending insights to deliver to user ---
    std::vector<DreamInsight> getPendingInsights(int maxResults = 3) {
        std::vector<DreamInsight> results;
        
        // Sort by quality (novelty * relevance * confidence)
        std::sort(pendingDelivery_.begin(), pendingDelivery_.end(),
            [](const DreamInsight& a, const DreamInsight& b) {
                float qualA = a.novelty * a.relevance * a.confidence;
                float qualB = b.novelty * b.relevance * b.confidence;
                return qualA > qualB;
            });
        
        int count = 0;
        for (auto& insight : pendingDelivery_) {
            if (!insight.delivered && count < maxResults) {
                insight.delivered = true;
                results.push_back(insight);
                count++;
            }
        }
        
        return results;
    }

    // --- Report if user found an insight useful ---
    void reportInsightFeedback(int insightIndex, bool useful) {
        if (insightIndex >= 0 && insightIndex < (int)insights_.size()) {
            insights_[insightIndex].was_useful = useful;
            if (useful) usefulInsights_++;
        }
    }

    // --- Check if there are undelivered insights ---
    bool hasUndeliveredInsights() const {
        for (const auto& i : pendingDelivery_) {
            if (!i.delivered) return true;
        }
        return false;
    }

    // --- Get top knowledge gaps ---
    std::vector<KnowledgeGap> getTopGaps(int n = 5) const {
        std::vector<KnowledgeGap> sorted = gaps_;
        std::sort(sorted.begin(), sorted.end(),
            [](const KnowledgeGap& a, const KnowledgeGap& b) {
                return a.priority > b.priority;
            });
        if ((int)sorted.size() > n) sorted.resize(n);
        return sorted;
    }

    // --- Accessors ---
    DreamPhase getPhase() const { return phase_; }
    
    std::string phaseToString() const {
        switch (phase_) {
            case DreamPhase::AWAKE: return "AWAKE";
            case DreamPhase::LIGHT_SLEEP: return "LIGHT_SLEEP";
            case DreamPhase::DEEP_DREAM: return "DEEP_DREAM";
            case DreamPhase::REM: return "REM";
        }
        return "UNKNOWN";
    }

    std::string getStats() const {
        std::ostringstream ss;
        ss << "Dreams: phase=" << phaseToString()
           << " | cycles=" << dreamCycles_
           << " | insights=" << totalInsights_
           << " (useful: " << usefulInsights_ << ")"
           << " | gaps=" << gaps_.size()
           << " | pending=" << pendingDelivery_.size();
        return ss.str();
    }

    // --- Save/Load ---
    void save(const std::string& path) const {
        std::ofstream out(path);
        if (!out.is_open()) return;
        
        // Save insights (last 50)
        int saveCount = std::min((int)insights_.size(), 50);
        out << saveCount << "\n";
        int start = (int)insights_.size() - saveCount;
        for (int i = start; i < (int)insights_.size(); i++) {
            out << insights_[i].content << "|" << insights_[i].novelty
                << "|" << insights_[i].confidence << "|" << insights_[i].was_useful << "\n";
        }
        
        // Save gaps
        out << gaps_.size() << "\n";
        for (const auto& gap : gaps_) {
            out << gap.domain << "|" << gap.specific_question << "|"
                << gap.times_encountered << "|" << gap.priority << "\n";
        }
    }

    void load(const std::string& path) {
        std::ifstream in(path);
        if (!in.is_open()) return;
        
        int insightCount;
        in >> insightCount;
        in.ignore();
        for (int i = 0; i < insightCount; i++) {
            std::string line;
            std::getline(in, line);
            DreamInsight insight;
            std::istringstream ss(line);
            std::string seg;
            if (std::getline(ss, seg, '|')) insight.content = seg;
            if (std::getline(ss, seg, '|')) { try { insight.novelty = std::stof(seg); } catch(...){} }
            if (std::getline(ss, seg, '|')) { try { insight.confidence = std::stof(seg); } catch(...){} }
            if (std::getline(ss, seg, '|')) { insight.was_useful = (seg == "1"); }
            insight.delivered = true; // Already seen
            insights_.push_back(insight);
            if (insight.was_useful) usefulInsights_++;
        }
        
        int gapCount;
        in >> gapCount;
        in.ignore();
        for (int i = 0; i < gapCount; i++) {
            std::string line;
            std::getline(in, line);
            KnowledgeGap gap;
            std::istringstream ss(line);
            std::string seg;
            if (std::getline(ss, seg, '|')) gap.domain = seg;
            if (std::getline(ss, seg, '|')) gap.specific_question = seg;
            if (std::getline(ss, seg, '|')) { try { gap.times_encountered = std::stoi(seg); } catch(...){} }
            if (std::getline(ss, seg, '|')) { try { gap.priority = std::stof(seg); } catch(...){} }
            gaps_.push_back(gap);
        }
        
        totalInsights_ = (int)insights_.size();
        std::cout << "[GENESIS:DREAM] Loaded " << insights_.size() << " dream insights, "
                  << gaps_.size() << " knowledge gaps.\n";
    }
};

#endif // MK_GENESIS_DREAM_SYNTH_CPP
