#ifndef MK_GENESIS_TEMPORAL_MIND_CPP
#define MK_GENESIS_TEMPORAL_MIND_CPP

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <random>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>
#include <ctime>

// ===================================================================================
// MK GENESIS ENGINE — TEMPORAL CONSCIOUSNESS
// ===================================================================================
// THE CONCEPT NO AI HAS:
//
// Current AI systems are STATELESS — they process input -> output, no sense of time.
// Prometheus has memory but no TIME AWARENESS.
//
// Genesis Temporal Mind gives MK:
//   1. TEMPORAL STREAM — a river of consciousness with flow direction
//   2. PAST ECHOES — thoughts that happened before that still resonate
//   3. PRESENT FOCUS — what's happening RIGHT NOW (with attention decay)
//   4. FUTURE ANTICIPATION — prediction of what the user WILL say/need next
//   5. TEMPORAL PATTERNS — recognition of recurring patterns across time
//   6. CONTEXT MOMENTUM — conversations have "velocity" and "direction"
//   7. MOOD DRIFT — emotional state changes over time, not instantly
//
// WHY THIS IS REVOLUTIONARY:
// - MK can say "you asked something similar 3 conversations ago"
// - MK can predict what you'll ask next and pre-compute the answer
// - MK's mood doesn't snap between states — it DRIFTS naturally
// - Conversations have momentum: if we're deep in code, MK stays in code mode
// - MK recognizes daily patterns: "you always ask about weather in the morning"
// ===================================================================================


// A moment in the temporal stream
struct TemporalMoment {
    std::string input;            // What user said
    std::string response;         // What MK said
    std::string domain;           // Topic domain
    float emotional_valence;      // How positive/negative
    float complexity;             // How complex the exchange was
    float satisfaction;           // Estimated user satisfaction (0-1)
    long long timestamp;          // When this happened
    int hour_of_day;              // 0-23, for daily patterns
    int day_of_week;              // 0-6, for weekly patterns
    std::vector<std::string> topics; // Topics discussed
    bool was_successful;          // Did MK answer well?
    
    TemporalMoment() : emotional_valence(0.5f), complexity(0.5f),
                       satisfaction(0.5f), timestamp(0), hour_of_day(0),
                       day_of_week(0), was_successful(true) {}
};

// Mood state (drifts over time, doesn't snap)
struct MoodState {
    float happiness;      // 0-1
    float energy;         // 0-1
    float curiosity;      // 0-1
    float patience;       // 0-1
    float creativity;     // 0-1
    float focus;          // 0-1 (how concentrated on current topic)
    
    MoodState() : happiness(0.6f), energy(0.7f), curiosity(0.7f),
                  patience(0.8f), creativity(0.5f), focus(0.5f) {}
    
    // Drift mood toward a target (never snaps instantly)
    void driftToward(const MoodState& target, float rate = 0.1f) {
        happiness += (target.happiness - happiness) * rate;
        energy += (target.energy - energy) * rate;
        curiosity += (target.curiosity - curiosity) * rate;
        patience += (target.patience - patience) * rate;
        creativity += (target.creativity - creativity) * rate;
        focus += (target.focus - focus) * rate;
    }
    
    std::string dominantMood() const {
        float vals[] = {happiness, energy, curiosity, patience, creativity, focus};
        const char* names[] = {"happy", "energetic", "curious", "patient", "creative", "focused"};
        int best = 0;
        for (int i = 1; i < 6; i++) {
            if (vals[i] > vals[best]) best = i;
        }
        return names[best];
    }
    
    std::string toString() const {
        std::ostringstream ss;
        ss << "mood{happy=" << (int)(happiness*100) << "% energy=" << (int)(energy*100)
           << "% curious=" << (int)(curiosity*100) << "% patience=" << (int)(patience*100)
           << "% creative=" << (int)(creativity*100) << "% focus=" << (int)(focus*100) << "%}";
        return ss.str();
    }
};

// Conversation momentum (direction and speed)
struct ConversationMomentum {
    std::string direction;       // Current topic trajectory
    float speed;                 // How fast topics are changing (0=static, 1=rapid)
    float depth;                 // How deep into current topic (0=surface, 1=deep)
    int turns_on_topic;          // How many turns on current topic
    std::string predicted_next;  // What we predict user will ask next
    float prediction_confidence; // How sure we are
    
    ConversationMomentum() : speed(0.3f), depth(0.0f), turns_on_topic(0),
                             prediction_confidence(0.0f) {}
};


// Daily pattern: what happens at each hour
struct DailyPattern {
    std::string common_topic;
    float avg_complexity;
    float avg_satisfaction;
    int occurrence_count;
    
    DailyPattern() : avg_complexity(0.5f), avg_satisfaction(0.5f), occurrence_count(0) {}
};

// ===================================================================================
// MKTemporalMind — Time-aware consciousness
// ===================================================================================
class MKTemporalMind {
private:
    // The temporal stream: ordered history of all moments
    std::deque<TemporalMoment> stream_;
    int maxStreamSize_;
    
    // Current state
    MoodState mood_;
    ConversationMomentum momentum_;
    std::string currentDomain_;
    
    // Patterns learned over time
    std::array<DailyPattern, 24> hourlyPatterns_;  // Pattern per hour of day
    std::unordered_map<std::string, int> topicFrequency_; // How often each topic appears
    std::unordered_map<std::string, std::vector<std::string>> topicSequences_; // A -> B transitions
    
    // Future anticipation
    std::vector<std::pair<std::string, float>> predictions_; // predicted_topic -> confidence
    
    // Stats
    int totalMoments_;
    int correctPredictions_;
    int totalPredictions_;
    
    long long now() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    
    int currentHour() const {
        std::time_t t = std::time(nullptr);
        std::tm* tm = std::localtime(&t);
        return tm->tm_hour;
    }
    
    int currentDayOfWeek() const {
        std::time_t t = std::time(nullptr);
        std::tm* tm = std::localtime(&t);
        return tm->tm_wday;
    }

    // Extract primary topic from input
    std::string extractTopic(const std::string& input) const {
        // Simple: use first meaningful word (skip stop words)
        static const std::unordered_set<std::string> stopWords = {
            "what", "is", "the", "a", "an", "how", "do", "does", "can", "you",
            "i", "me", "my", "we", "our", "it", "that", "this", "to", "of",
            "in", "for", "on", "with", "at", "by", "about", "tell", "show"
        };
        
        std::istringstream ss(input);
        std::string word;
        while (ss >> word) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            // Strip punctuation
            while (!word.empty() && !std::isalnum(word.back())) word.pop_back();
            if (word.size() > 2 && !stopWords.count(word)) {
                return word;
            }
        }
        return "general";
    }

public:
    MKTemporalMind(int maxStream = 1000)
        : maxStreamSize_(maxStream), totalMoments_(0),
          correctPredictions_(0), totalPredictions_(0) {
        std::cout << "[GENESIS:TEMPORAL] Temporal consciousness initialized. "
                  << "Stream capacity: " << maxStream << " moments.\n";
    }


    // --- RECORD: Add a moment to the temporal stream ---
    void record(const std::string& input, const std::string& response,
                float satisfaction, bool successful) {
        TemporalMoment moment;
        moment.input = input;
        moment.response = response;
        moment.timestamp = now();
        moment.hour_of_day = currentHour();
        moment.day_of_week = currentDayOfWeek();
        moment.satisfaction = satisfaction;
        moment.was_successful = successful;
        
        // Extract topic
        std::string topic = extractTopic(input);
        moment.topics.push_back(topic);
        moment.domain = topic;
        
        // Estimate complexity based on input length and question words
        std::string lower = input;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        moment.complexity = std::min((float)input.size() / 200.0f, 1.0f);
        if (lower.find("why") != std::string::npos || lower.find("explain") != std::string::npos ||
            lower.find("how does") != std::string::npos) {
            moment.complexity += 0.2f;
        }
        moment.complexity = std::clamp(moment.complexity, 0.0f, 1.0f);
        
        // Estimate emotional valence from keywords
        moment.emotional_valence = 0.5f; // neutral default
        if (lower.find("thanks") != std::string::npos || lower.find("great") != std::string::npos ||
            lower.find("awesome") != std::string::npos || lower.find("cool") != std::string::npos) {
            moment.emotional_valence = 0.8f;
        } else if (lower.find("wrong") != std::string::npos || lower.find("bad") != std::string::npos ||
                   lower.find("hate") != std::string::npos || lower.find("sucks") != std::string::npos) {
            moment.emotional_valence = 0.2f;
        }
        
        // Push to stream
        stream_.push_back(moment);
        if ((int)stream_.size() > maxStreamSize_) {
            stream_.pop_front();
        }
        totalMoments_++;
        
        // Update patterns
        updatePatterns(moment);
        
        // Update momentum
        updateMomentum(topic);
        
        // Drift mood based on the interaction
        driftMoodFromMoment(moment);
        
        // Check if our prediction was correct
        checkPrediction(topic);
        
        // Generate new predictions
        generatePredictions(topic);
    }

    // --- Update hourly patterns ---
    void updatePatterns(const TemporalMoment& moment) {
        int hour = moment.hour_of_day;
        auto& pattern = hourlyPatterns_[hour];
        
        if (!moment.topics.empty()) {
            pattern.common_topic = moment.topics[0];
        }
        // Running average
        float n = (float)pattern.occurrence_count;
        pattern.avg_complexity = (pattern.avg_complexity * n + moment.complexity) / (n + 1.0f);
        pattern.avg_satisfaction = (pattern.avg_satisfaction * n + moment.satisfaction) / (n + 1.0f);
        pattern.occurrence_count++;
        
        // Track topic frequency
        for (const auto& t : moment.topics) {
            topicFrequency_[t]++;
        }
    }

    // --- Update conversation momentum ---
    void updateMomentum(const std::string& topic) {
        if (topic == currentDomain_) {
            // Same topic: going deeper
            momentum_.depth = std::min(momentum_.depth + 0.15f, 1.0f);
            momentum_.speed *= 0.8f; // Slowing down (settling in)
            momentum_.turns_on_topic++;
        } else {
            // Topic change: update direction
            if (!currentDomain_.empty()) {
                // Record the transition for future prediction
                topicSequences_[currentDomain_].push_back(topic);
            }
            momentum_.direction = topic;
            momentum_.depth = 0.1f;  // Starting fresh
            momentum_.speed = std::min(momentum_.speed + 0.2f, 1.0f); // Accelerating
            momentum_.turns_on_topic = 1;
            currentDomain_ = topic;
        }
    }


    // --- Drift mood based on interaction quality ---
    void driftMoodFromMoment(const TemporalMoment& moment) {
        MoodState target;
        
        // Positive interaction -> boost happiness and energy
        if (moment.satisfaction > 0.7f) {
            target.happiness = 0.8f;
            target.energy = 0.7f;
            target.patience = 0.9f;
        } else if (moment.satisfaction < 0.3f) {
            target.happiness = 0.4f;
            target.energy = 0.5f;
            target.patience = 0.6f;
        }
        
        // Complex questions boost curiosity
        if (moment.complexity > 0.7f) {
            target.curiosity = 0.9f;
            target.focus = 0.8f;
        }
        
        // High emotional valence affects warmth
        if (moment.emotional_valence > 0.7f) {
            target.happiness = 0.85f;
            target.creativity = 0.6f;
        }
        
        // Drift (not snap) toward target
        mood_.driftToward(target, 0.15f);
    }

    // --- PREDICT: What will user ask next? ---
    void generatePredictions(const std::string& currentTopic) {
        predictions_.clear();
        
        // Strategy 1: Topic sequence prediction
        auto it = topicSequences_.find(currentTopic);
        if (it != topicSequences_.end() && !it->second.empty()) {
            // Count occurrences of each next-topic
            std::unordered_map<std::string, int> nextCounts;
            for (const auto& next : it->second) {
                nextCounts[next]++;
            }
            
            // Convert to predictions
            float total = (float)it->second.size();
            for (const auto& [topic, count] : nextCounts) {
                float confidence = (float)count / total;
                if (confidence > 0.1f) {
                    predictions_.push_back({topic, confidence});
                }
            }
        }
        
        // Strategy 2: Hourly pattern prediction
        int hour = currentHour();
        if (hourlyPatterns_[hour].occurrence_count > 3) {
            std::string hourTopic = hourlyPatterns_[hour].common_topic;
            if (!hourTopic.empty() && hourTopic != currentTopic) {
                predictions_.push_back({hourTopic, 0.3f}); // Low confidence
            }
        }
        
        // Strategy 3: Momentum-based prediction (if going deep, predict deeper)
        if (momentum_.depth > 0.5f && momentum_.turns_on_topic > 2) {
            predictions_.push_back({currentTopic + "_deep", 0.4f});
        }
        
        // Sort by confidence
        std::sort(predictions_.begin(), predictions_.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Keep top 5
        if (predictions_.size() > 5) predictions_.resize(5);
        
        if (!predictions_.empty()) {
            momentum_.predicted_next = predictions_[0].first;
            momentum_.prediction_confidence = predictions_[0].second;
            totalPredictions_++;
        }
    }

    // --- Check if prediction was correct ---
    void checkPrediction(const std::string& actualTopic) {
        if (momentum_.predicted_next.empty()) return;
        
        // Fuzzy match: prediction is "correct" if topic contains predicted word
        std::string lower_pred = momentum_.predicted_next;
        std::string lower_actual = actualTopic;
        std::transform(lower_pred.begin(), lower_pred.end(), lower_pred.begin(), ::tolower);
        std::transform(lower_actual.begin(), lower_actual.end(), lower_actual.begin(), ::tolower);
        
        if (lower_actual.find(lower_pred) != std::string::npos ||
            lower_pred.find(lower_actual) != std::string::npos) {
            correctPredictions_++;
        }
    }


    // --- ECHO: Find past moments similar to current input ---
    std::vector<TemporalMoment> findEchoes(const std::string& input, int maxResults = 3) const {
        std::string lower = input;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        std::vector<std::pair<int, float>> scored;
        for (int i = 0; i < (int)stream_.size(); i++) {
            float score = 0.0f;
            std::string pastLower = stream_[i].input;
            std::transform(pastLower.begin(), pastLower.end(), pastLower.begin(), ::tolower);
            
            // Word overlap scoring
            std::istringstream ss(lower);
            std::string word;
            int matches = 0, total = 0;
            while (ss >> word) {
                total++;
                if (pastLower.find(word) != std::string::npos) matches++;
            }
            if (total > 0) score = (float)matches / (float)total;
            
            // Recency bonus (more recent = slightly higher score)
            float recencyBonus = (float)i / (float)stream_.size() * 0.1f;
            score += recencyBonus;
            
            if (score > 0.3f) {
                scored.push_back({i, score});
            }
        }
        
        std::sort(scored.begin(), scored.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        std::vector<TemporalMoment> results;
        for (int i = 0; i < std::min((int)scored.size(), maxResults); i++) {
            results.push_back(stream_[scored[i].first]);
        }
        return results;
    }

    // --- Get recent stream (last N moments) ---
    std::vector<TemporalMoment> getRecent(int n = 5) const {
        std::vector<TemporalMoment> recent;
        int start = std::max(0, (int)stream_.size() - n);
        for (int i = start; i < (int)stream_.size(); i++) {
            recent.push_back(stream_[i]);
        }
        return recent;
    }

    // --- Get conversation context for generation ---
    std::string getContextSummary() const {
        std::ostringstream ss;
        
        // Current mood
        ss << "Mood: " << mood_.dominantMood();
        
        // Momentum
        if (!momentum_.direction.empty()) {
            ss << " | Topic: " << momentum_.direction
               << " (depth=" << (int)(momentum_.depth * 100) << "%, "
               << momentum_.turns_on_topic << " turns)";
        }
        
        // Prediction
        if (!momentum_.predicted_next.empty() && momentum_.prediction_confidence > 0.2f) {
            ss << " | Predict: " << momentum_.predicted_next
               << " (" << (int)(momentum_.prediction_confidence * 100) << "%)";
        }
        
        return ss.str();
    }

    // --- Accessors ---
    const MoodState& getMood() const { return mood_; }
    MoodState& getMood() { return mood_; }
    const ConversationMomentum& getMomentum() const { return momentum_; }
    const std::string& getCurrentDomain() const { return currentDomain_; }
    int getStreamSize() const { return (int)stream_.size(); }
    
    float getPredictionAccuracy() const {
        if (totalPredictions_ == 0) return 0.0f;
        return (float)correctPredictions_ / (float)totalPredictions_;
    }

    std::vector<std::pair<std::string, float>> getPredictions() const {
        return predictions_;
    }


    // --- Get influence on thought generation ---
    // Returns modifiers that should affect how thoughts are generated
    struct TemporalInfluence {
        float creativity_boost;    // How much to boost creative generation
        float precision_boost;     // How much to boost precise/code generation
        float warmth_boost;        // How much warmth to add
        float verbosity_modifier;  // Talk more or less
        std::string predicted_domain; // What domain to pre-load
    };
    
    TemporalInfluence getInfluence() const {
        TemporalInfluence inf;
        
        // High creativity mood -> boost creative generation
        inf.creativity_boost = mood_.creativity - 0.5f; // -0.5 to +0.5
        
        // High focus -> boost precision
        inf.precision_boost = mood_.focus - 0.5f;
        
        // High happiness -> more warmth
        inf.warmth_boost = mood_.happiness - 0.5f;
        
        // Low patience or high speed -> be terser
        inf.verbosity_modifier = (mood_.patience - 0.5f) * 0.5f - (momentum_.speed - 0.5f) * 0.3f;
        
        // Predicted domain for pre-loading
        inf.predicted_domain = momentum_.predicted_next;
        
        return inf;
    }

    // --- Stats ---
    std::string getStats() const {
        std::ostringstream ss;
        ss << "Temporal: " << stream_.size() << " moments | "
           << mood_.toString() << " | "
           << "Prediction accuracy: " << (int)(getPredictionAccuracy() * 100) << "% ("
           << correctPredictions_ << "/" << totalPredictions_ << ")";
        if (!momentum_.direction.empty()) {
            ss << " | Direction: " << momentum_.direction
               << " depth=" << (int)(momentum_.depth * 100) << "%";
        }
        return ss.str();
    }

    // --- Save/Load ---
    void save(const std::string& path) const {
        std::ofstream out(path);
        if (!out.is_open()) return;
        
        // Save mood
        out << mood_.happiness << " " << mood_.energy << " " << mood_.curiosity
            << " " << mood_.patience << " " << mood_.creativity << " " << mood_.focus << "\n";
        
        // Save topic frequencies
        out << topicFrequency_.size() << "\n";
        for (const auto& [topic, count] : topicFrequency_) {
            out << topic << " " << count << "\n";
        }
        
        // Save recent stream (last 100)
        int saveCount = std::min((int)stream_.size(), 100);
        out << saveCount << "\n";
        int start = (int)stream_.size() - saveCount;
        for (int i = start; i < (int)stream_.size(); i++) {
            out << stream_[i].domain << "|" << stream_[i].satisfaction << "|"
                << stream_[i].hour_of_day << "|" << stream_[i].was_successful << "\n";
        }
    }

    void load(const std::string& path) {
        std::ifstream in(path);
        if (!in.is_open()) return;
        
        // Load mood
        in >> mood_.happiness >> mood_.energy >> mood_.curiosity
           >> mood_.patience >> mood_.creativity >> mood_.focus;
        
        // Load topic frequencies
        int topicCount;
        in >> topicCount;
        for (int i = 0; i < topicCount; i++) {
            std::string topic;
            int count;
            in >> topic >> count;
            topicFrequency_[topic] = count;
        }
        
        // Load stream summary
        int streamCount;
        in >> streamCount;
        in.ignore(); // skip newline
        for (int i = 0; i < streamCount; i++) {
            std::string line;
            std::getline(in, line);
            // Parse: domain|satisfaction|hour|success
            std::istringstream ls(line);
            std::string seg;
            TemporalMoment m;
            if (std::getline(ls, seg, '|')) m.domain = seg;
            if (std::getline(ls, seg, '|')) { try { m.satisfaction = std::stof(seg); } catch(...){} }
            if (std::getline(ls, seg, '|')) { try { m.hour_of_day = std::stoi(seg); } catch(...){} }
            if (std::getline(ls, seg, '|')) { m.was_successful = (seg == "1"); }
            m.topics.push_back(m.domain);
            stream_.push_back(m);
        }
        
        totalMoments_ = (int)stream_.size();
        std::cout << "[GENESIS:TEMPORAL] Loaded " << totalMoments_ << " moments. "
                  << "Topic memory: " << topicFrequency_.size() << " topics.\n";
    }
};

#endif // MK_GENESIS_TEMPORAL_MIND_CPP
