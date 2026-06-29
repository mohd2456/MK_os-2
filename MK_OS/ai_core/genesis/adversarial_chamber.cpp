#ifndef MK_GENESIS_ADVERSARIAL_CHAMBER_CPP
#define MK_GENESIS_ADVERSARIAL_CHAMBER_CPP

#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>
#include <array>

// ===================================================================================
// MK GENESIS ENGINE — ADVERSARIAL EVOLUTION CHAMBER
// ===================================================================================
// THE CONCEPT:
//
// Instead of ONE mind trying to answer, MULTIPLE COMPETING MINDS race to produce
// the best response. They each have different personalities, strategies, and
// knowledge biases. After generating, they BATTLE — and the winner gets rewarded
// while losers get punished (reduced fitness, potential death).
//
// THE MINDS (Archetypes):
//   - THE SCHOLAR:  Prioritizes accuracy, cites sources, careful
//   - THE FRIEND:   Prioritizes warmth, emotional connection, casual
//   - THE ENGINEER: Prioritizes precision, code quality, efficiency
//   - THE DREAMER:  Prioritizes creativity, novel connections, surprise
//   - THE SAGE:     Prioritizes deep reasoning, philosophy, wisdom
//
// BATTLE MECHANICS:
//   1. All 5 minds generate a response candidate
//   2. Candidates are scored on: relevance, quality, novelty, user-match
//   3. Winner takes all: gets fitness boost, offspring rights
//   4. Loser penalty: fitness reduction, possible archetype mutation
//   5. Over time, the population shifts toward what the USER values
//
// WHY THIS IS REVOLUTIONARY:
// - No single point of failure: if one approach fails, another succeeds
// - Natural diversity: ensures responses never get stale or repetitive
// - Self-optimizing: the system learns WHICH STYLE you prefer
// - Emergent personality: MK's personality emerges from the competition
// ===================================================================================


// Mind archetypes
enum class Archetype {
    SCHOLAR,    // Accuracy-first, cites knowledge, careful
    FRIEND,     // Warmth-first, casual, emotionally connected
    ENGINEER,   // Precision-first, code-oriented, efficient
    DREAMER,    // Creativity-first, novel, surprising
    SAGE        // Reasoning-first, deep, philosophical
};

// A competing mind in the chamber
struct CompetingMind {
    Archetype archetype;
    std::string name;
    float fitness;              // Overall success rate (0-1)
    float aggression;           // How bold/risky the responses are
    int wins;
    int losses;
    int total_battles;
    float specialization;       // How focused on its archetype (0=generalist, 1=pure)
    
    // Gene biases: how this mind modifies ThoughtDNA
    float semantic_bias;        // Boosts semantic strand
    float emotional_bias;       // Boosts emotional strand
    float logical_bias;         // Boosts logical strand
    float executable_bias;      // Boosts executable strand
    
    CompetingMind() : archetype(Archetype::SCHOLAR), fitness(0.5f),
                      aggression(0.5f), wins(0), losses(0), total_battles(0),
                      specialization(0.7f), semantic_bias(0.5f),
                      emotional_bias(0.5f), logical_bias(0.5f),
                      executable_bias(0.5f) {}
    
    float winRate() const {
        if (total_battles == 0) return 0.5f;
        return (float)wins / (float)total_battles;
    }
};

// A battle candidate: a mind's response attempt
struct BattleCandidate {
    int mind_index;              // Which mind produced this
    Archetype archetype;
    std::string response;        // The actual response text
    float relevance_score;       // How relevant to the query (0-1)
    float quality_score;         // Quality of language/code (0-1)
    float novelty_score;         // How novel/surprising (0-1)
    float user_match_score;      // How well it matches user preferences (0-1)
    float total_score;           // Combined battle score
    
    BattleCandidate() : mind_index(-1), archetype(Archetype::SCHOLAR),
                        relevance_score(0.0f), quality_score(0.0f),
                        novelty_score(0.0f), user_match_score(0.0f),
                        total_score(0.0f) {}
};

// Battle result
struct BattleResult {
    int winner_index;
    std::string winning_response;
    Archetype winning_archetype;
    float winning_score;
    std::vector<BattleCandidate> all_candidates;
    std::string battle_trace;    // Blow-by-blow of the battle
    
    BattleResult() : winner_index(-1), winning_archetype(Archetype::SCHOLAR),
                     winning_score(0.0f) {}
};


// ===================================================================================
// MKAdversarialChamber — The battle arena
// ===================================================================================
class MKAdversarialChamber {
private:
    std::vector<CompetingMind> minds_;
    std::mt19937 rng_;
    int totalBattles_;
    int totalGenerations_;
    
    // User preference tracking (learned over time)
    float userPrefers_accuracy_;    // 0-1
    float userPrefers_warmth_;      // 0-1
    float userPrefers_code_;        // 0-1
    float userPrefers_creativity_;  // 0-1
    float userPrefers_depth_;       // 0-1
    
    // Battle history for meta-learning
    std::vector<Archetype> recentWinners_;
    static const int MAX_HISTORY = 100;

    void initMinds() {
        // THE SCHOLAR
        CompetingMind scholar;
        scholar.archetype = Archetype::SCHOLAR;
        scholar.name = "Scholar";
        scholar.aggression = 0.3f;
        scholar.specialization = 0.8f;
        scholar.semantic_bias = 0.9f;
        scholar.emotional_bias = 0.3f;
        scholar.logical_bias = 0.7f;
        scholar.executable_bias = 0.3f;
        minds_.push_back(scholar);
        
        // THE FRIEND
        CompetingMind friend_mind;
        friend_mind.archetype = Archetype::FRIEND;
        friend_mind.name = "Friend";
        friend_mind.aggression = 0.4f;
        friend_mind.specialization = 0.8f;
        friend_mind.semantic_bias = 0.5f;
        friend_mind.emotional_bias = 0.95f;
        friend_mind.logical_bias = 0.2f;
        friend_mind.executable_bias = 0.1f;
        minds_.push_back(friend_mind);
        
        // THE ENGINEER
        CompetingMind engineer;
        engineer.archetype = Archetype::ENGINEER;
        engineer.name = "Engineer";
        engineer.aggression = 0.5f;
        engineer.specialization = 0.85f;
        engineer.semantic_bias = 0.4f;
        engineer.emotional_bias = 0.1f;
        engineer.logical_bias = 0.8f;
        engineer.executable_bias = 0.95f;
        minds_.push_back(engineer);
        
        // THE DREAMER
        CompetingMind dreamer;
        dreamer.archetype = Archetype::DREAMER;
        dreamer.name = "Dreamer";
        dreamer.aggression = 0.8f;
        dreamer.specialization = 0.75f;
        dreamer.semantic_bias = 0.7f;
        dreamer.emotional_bias = 0.6f;
        dreamer.logical_bias = 0.3f;
        dreamer.executable_bias = 0.2f;
        minds_.push_back(dreamer);
        
        // THE SAGE
        CompetingMind sage;
        sage.archetype = Archetype::SAGE;
        sage.name = "Sage";
        sage.aggression = 0.2f;
        sage.specialization = 0.9f;
        sage.semantic_bias = 0.8f;
        sage.emotional_bias = 0.4f;
        sage.logical_bias = 0.95f;
        sage.executable_bias = 0.3f;
        minds_.push_back(sage);
    }

public:
    MKAdversarialChamber()
        : totalBattles_(0), totalGenerations_(0),
          userPrefers_accuracy_(0.5f), userPrefers_warmth_(0.5f),
          userPrefers_code_(0.3f), userPrefers_creativity_(0.5f),
          userPrefers_depth_(0.5f) {
        rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
        initMinds();
        std::cout << "[GENESIS:ARENA] Adversarial chamber initialized. "
                  << minds_.size() << " competing minds ready.\n";
    }


    // --- Apply mind's bias to a ThoughtDNA ---
    // Each mind "sees" the thought differently based on its biases
    ThoughtDNA applyBias(const ThoughtDNA& original, const CompetingMind& mind) const {
        ThoughtDNA biased = original;
        
        // Amplify strands based on mind's biases
        for (int g = 0; g < GENES_PER_STRAND; g++) {
            biased.helix[STRAND_SEMANTIC][g] *= mind.semantic_bias;
            biased.helix[STRAND_EMOTIONAL][g] *= mind.emotional_bias;
            biased.helix[STRAND_LOGICAL][g] *= mind.logical_bias;
            biased.helix[STRAND_EXECUTABLE][g] *= mind.executable_bias;
            
            // Clamp
            biased.helix[STRAND_SEMANTIC][g] = std::clamp(biased.helix[STRAND_SEMANTIC][g], 0.0f, 1.0f);
            biased.helix[STRAND_EMOTIONAL][g] = std::clamp(biased.helix[STRAND_EMOTIONAL][g], 0.0f, 1.0f);
            biased.helix[STRAND_LOGICAL][g] = std::clamp(biased.helix[STRAND_LOGICAL][g], 0.0f, 1.0f);
            biased.helix[STRAND_EXECUTABLE][g] = std::clamp(biased.helix[STRAND_EXECUTABLE][g], 0.0f, 1.0f);
        }
        
        // Aggression affects mutation rate
        biased.mutation_rate *= (0.5f + mind.aggression);
        
        return biased;
    }

    // --- Score a candidate response ---
    void scoreCandidate(BattleCandidate& candidate, const std::string& query,
                        const std::vector<std::string>& contextTokens) {
        const std::string& response = candidate.response;
        if (response.empty()) {
            candidate.total_score = 0.0f;
            return;
        }
        
        // RELEVANCE: Does the response relate to the query?
        {
            std::string lowerResp = response;
            std::transform(lowerResp.begin(), lowerResp.end(), lowerResp.begin(), ::tolower);
            int matches = 0;
            for (const auto& token : contextTokens) {
                if (lowerResp.find(token) != std::string::npos) matches++;
            }
            candidate.relevance_score = contextTokens.empty() ? 0.5f :
                std::min((float)matches / (float)contextTokens.size() * 1.5f, 1.0f);
        }
        
        // QUALITY: Length, variety, coherence heuristics
        {
            float score = 0.5f;
            // Good length (not too short, not too long)
            if (response.size() >= 15 && response.size() <= 500) score += 0.2f;
            else if (response.size() >= 5 && response.size() <= 1000) score += 0.1f;
            else if (response.size() < 5) score -= 0.3f;
            
            // Word variety
            std::unordered_map<std::string, int> wordCounts;
            std::istringstream ss(response);
            std::string word;
            int totalWords = 0;
            while (ss >> word) {
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                wordCounts[word]++;
                totalWords++;
            }
            if (totalWords > 0) {
                float variety = (float)wordCounts.size() / (float)totalWords;
                score += variety * 0.3f;
            }
            
            candidate.quality_score = std::clamp(score, 0.0f, 1.0f);
        }
        
        // NOVELTY: Is this different from recent responses?
        {
            // Simple: longer and more varied = more novel
            candidate.novelty_score = std::min((float)response.size() / 200.0f, 0.5f);
            // Boost for creative punctuation/structure
            if (response.find("—") != std::string::npos) candidate.novelty_score += 0.1f;
            if (response.find("?") != std::string::npos) candidate.novelty_score += 0.1f;
            if (response.find("!") != std::string::npos) candidate.novelty_score += 0.05f;
            candidate.novelty_score = std::clamp(candidate.novelty_score, 0.0f, 1.0f);
        }
        
        // USER MATCH: How well does this match learned preferences?
        {
            float match = 0.5f;
            switch (candidate.archetype) {
                case Archetype::SCHOLAR: match += userPrefers_accuracy_ * 0.3f; break;
                case Archetype::FRIEND:  match += userPrefers_warmth_ * 0.3f; break;
                case Archetype::ENGINEER: match += userPrefers_code_ * 0.3f; break;
                case Archetype::DREAMER: match += userPrefers_creativity_ * 0.3f; break;
                case Archetype::SAGE:    match += userPrefers_depth_ * 0.3f; break;
            }
            // Win rate bonus: minds that historically win get a slight edge
            match += minds_[candidate.mind_index].winRate() * 0.1f;
            candidate.user_match_score = std::clamp(match, 0.0f, 1.0f);
        }
        
        // TOTAL: Weighted combination
        candidate.total_score = candidate.relevance_score * 0.3f +
                               candidate.quality_score * 0.25f +
                               candidate.novelty_score * 0.15f +
                               candidate.user_match_score * 0.3f;
    }


    // --- BATTLE: All minds compete, winner is selected ---
    BattleResult battle(const std::vector<std::string>& candidates,
                        const std::string& query,
                        const std::vector<std::string>& contextTokens) {
        BattleResult result;
        
        // Create battle candidates from each mind's response
        for (int i = 0; i < (int)candidates.size() && i < (int)minds_.size(); i++) {
            BattleCandidate bc;
            bc.mind_index = i;
            bc.archetype = minds_[i].archetype;
            bc.response = candidates[i];
            
            // Score this candidate
            scoreCandidate(bc, query, contextTokens);
            result.all_candidates.push_back(bc);
        }
        
        // Find the winner (highest total_score)
        float bestScore = -1.0f;
        int winnerIdx = 0;
        for (int i = 0; i < (int)result.all_candidates.size(); i++) {
            if (result.all_candidates[i].total_score > bestScore) {
                bestScore = result.all_candidates[i].total_score;
                winnerIdx = i;
            }
        }
        
        result.winner_index = winnerIdx;
        result.winning_response = result.all_candidates[winnerIdx].response;
        result.winning_archetype = result.all_candidates[winnerIdx].archetype;
        result.winning_score = bestScore;
        
        // Update mind fitness
        for (int i = 0; i < (int)result.all_candidates.size(); i++) {
            if (i == winnerIdx) {
                minds_[i].wins++;
                minds_[i].fitness = std::min(1.0f, minds_[i].fitness + 0.02f);
            } else {
                minds_[i].losses++;
                minds_[i].fitness = std::max(0.1f, minds_[i].fitness - 0.005f);
            }
            minds_[i].total_battles++;
        }
        
        // Record winner in history
        recentWinners_.push_back(result.winning_archetype);
        if ((int)recentWinners_.size() > MAX_HISTORY) {
            recentWinners_.erase(recentWinners_.begin());
        }
        
        // Build battle trace
        std::ostringstream trace;
        trace << "=== Battle #" << totalBattles_ + 1 << " ===\n";
        for (const auto& c : result.all_candidates) {
            trace << "  " << archetypeToString(c.archetype) << ": "
                  << "rel=" << (int)(c.relevance_score * 100) << "% "
                  << "qual=" << (int)(c.quality_score * 100) << "% "
                  << "nov=" << (int)(c.novelty_score * 100) << "% "
                  << "match=" << (int)(c.user_match_score * 100) << "% "
                  << "TOTAL=" << (int)(c.total_score * 100) << "%"
                  << (c.mind_index == winnerIdx ? " <-- WINNER" : "") << "\n";
        }
        result.battle_trace = trace.str();
        
        totalBattles_++;
        return result;
    }

    // --- Get biased ThoughtDNA for each mind ---
    std::vector<ThoughtDNA> getBiasedThoughts(const ThoughtDNA& original) {
        std::vector<ThoughtDNA> biased;
        for (const auto& mind : minds_) {
            biased.push_back(applyBias(original, mind));
        }
        return biased;
    }

    // --- Get the dominant archetype (most wins recently) ---
    Archetype getDominantArchetype() const {
        if (recentWinners_.empty()) return Archetype::FRIEND;
        
        int counts[5] = {0};
        for (auto a : recentWinners_) {
            counts[(int)a]++;
        }
        int best = 0;
        for (int i = 1; i < 5; i++) {
            if (counts[i] > counts[best]) best = i;
        }
        return (Archetype)best;
    }


    // --- Report user feedback (positive or negative) ---
    void reportFeedback(bool positive, Archetype winningArchetype) {
        if (positive) {
            switch (winningArchetype) {
                case Archetype::SCHOLAR: userPrefers_accuracy_ = std::min(1.0f, userPrefers_accuracy_ + 0.03f); break;
                case Archetype::FRIEND:  userPrefers_warmth_ = std::min(1.0f, userPrefers_warmth_ + 0.03f); break;
                case Archetype::ENGINEER: userPrefers_code_ = std::min(1.0f, userPrefers_code_ + 0.03f); break;
                case Archetype::DREAMER: userPrefers_creativity_ = std::min(1.0f, userPrefers_creativity_ + 0.03f); break;
                case Archetype::SAGE:    userPrefers_depth_ = std::min(1.0f, userPrefers_depth_ + 0.03f); break;
            }
        } else {
            switch (winningArchetype) {
                case Archetype::SCHOLAR: userPrefers_accuracy_ = std::max(0.0f, userPrefers_accuracy_ - 0.05f); break;
                case Archetype::FRIEND:  userPrefers_warmth_ = std::max(0.0f, userPrefers_warmth_ - 0.05f); break;
                case Archetype::ENGINEER: userPrefers_code_ = std::max(0.0f, userPrefers_code_ - 0.05f); break;
                case Archetype::DREAMER: userPrefers_creativity_ = std::max(0.0f, userPrefers_creativity_ - 0.05f); break;
                case Archetype::SAGE:    userPrefers_depth_ = std::max(0.0f, userPrefers_depth_ - 0.05f); break;
            }
        }
    }

    // --- EVOLVE: Mutate underperforming minds ---
    void evolve() {
        totalGenerations_++;
        
        for (auto& mind : minds_) {
            if (mind.winRate() < 0.2f && mind.total_battles > 10) {
                // Underperforming: mutate biases
                std::uniform_real_distribution<float> noise(-0.1f, 0.1f);
                mind.semantic_bias = std::clamp(mind.semantic_bias + noise(rng_), 0.0f, 1.0f);
                mind.emotional_bias = std::clamp(mind.emotional_bias + noise(rng_), 0.0f, 1.0f);
                mind.logical_bias = std::clamp(mind.logical_bias + noise(rng_), 0.0f, 1.0f);
                mind.executable_bias = std::clamp(mind.executable_bias + noise(rng_), 0.0f, 1.0f);
                mind.aggression = std::clamp(mind.aggression + noise(rng_), 0.1f, 0.9f);
                
                // Reset battle stats after evolution
                mind.wins = 0;
                mind.losses = 0;
                mind.total_battles = 0;
            }
        }
    }

    // --- Utility ---
    std::string archetypeToString(Archetype a) const {
        switch (a) {
            case Archetype::SCHOLAR: return "SCHOLAR";
            case Archetype::FRIEND: return "FRIEND";
            case Archetype::ENGINEER: return "ENGINEER";
            case Archetype::DREAMER: return "DREAMER";
            case Archetype::SAGE: return "SAGE";
        }
        return "UNKNOWN";
    }

    int getMindCount() const { return (int)minds_.size(); }
    int getTotalBattles() const { return totalBattles_; }
    
    const CompetingMind& getMind(int idx) const { return minds_[idx]; }

    std::string getStats() const {
        std::ostringstream ss;
        ss << "Arena: " << totalBattles_ << " battles, gen " << totalGenerations_ << " | ";
        ss << "Dominant: " << archetypeToString(getDominantArchetype()) << " | ";
        ss << "Prefs: acc=" << (int)(userPrefers_accuracy_*100) << "% "
           << "warm=" << (int)(userPrefers_warmth_*100) << "% "
           << "code=" << (int)(userPrefers_code_*100) << "% "
           << "cre=" << (int)(userPrefers_creativity_*100) << "% "
           << "deep=" << (int)(userPrefers_depth_*100) << "%\n";
        for (const auto& m : minds_) {
            ss << "  " << m.name << ": W=" << m.wins << " L=" << m.losses
               << " fit=" << (int)(m.fitness*100) << "% winrate="
               << (int)(m.winRate()*100) << "%\n";
        }
        return ss.str();
    }

    // --- Save/Load ---
    void save(const std::string& path) const {
        std::ofstream out(path);
        if (!out.is_open()) return;
        
        out << userPrefers_accuracy_ << " " << userPrefers_warmth_ << " "
            << userPrefers_code_ << " " << userPrefers_creativity_ << " "
            << userPrefers_depth_ << "\n";
        out << minds_.size() << "\n";
        for (const auto& m : minds_) {
            out << (int)m.archetype << " " << m.fitness << " " << m.aggression << " "
                << m.wins << " " << m.losses << " " << m.total_battles << " "
                << m.semantic_bias << " " << m.emotional_bias << " "
                << m.logical_bias << " " << m.executable_bias << "\n";
        }
    }

    void load(const std::string& path) {
        std::ifstream in(path);
        if (!in.is_open()) return;
        
        in >> userPrefers_accuracy_ >> userPrefers_warmth_
           >> userPrefers_code_ >> userPrefers_creativity_ >> userPrefers_depth_;
        
        int count;
        in >> count;
        for (int i = 0; i < count && i < (int)minds_.size(); i++) {
            int arch;
            in >> arch >> minds_[i].fitness >> minds_[i].aggression
               >> minds_[i].wins >> minds_[i].losses >> minds_[i].total_battles
               >> minds_[i].semantic_bias >> minds_[i].emotional_bias
               >> minds_[i].logical_bias >> minds_[i].executable_bias;
        }
        
        std::cout << "[GENESIS:ARENA] Loaded battle state. Total battles: "
                  << totalBattles_ << "\n";
    }
};

#endif // MK_GENESIS_ADVERSARIAL_CHAMBER_CPP
