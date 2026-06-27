#ifndef MK_CONVERSATION_ENGINE_CPP
#define MK_CONVERSATION_ENGINE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <functional>

#include "pattern_graph.cpp"

// ===================================================================================
// MK CONVERSATION ENGINE — Real Conversational AI
// ===================================================================================
// This is what makes MK feel like talking to a REAL AI assistant,
// not just a Q&A lookup system. Tracks context, resolves references,
// adapts personality, and remembers across sessions.
//
// KEY FEATURES:
//   1. Context Window — Remembers last 20 exchanges, resolves "it"/"that"/"they"
//   2. Multi-turn Reasoning — Knows what user is referring to in follow-ups
//   3. Proactive Suggestions — Suggests related topics after answering
//   4. Personality Modes — casual, technical, teacher
//   5. Emotion Detection — Detects frustration, confusion, excitement
//   6. Cross-Session Memory — Saves important context to disk
//   7. Conversation Summarizer — Generates summaries of long conversations
//   8. Intent Chaining — Decomposes multi-step commands into sequences
//
// The difference: LLMs simulate conversation through next-token prediction.
// MK ACTUALLY TRACKS state, topics, and user intent explicitly.
// ===================================================================================


// Forward declarations (MKPatternGraph included above)

// ─────────────────────────────────────────────────────────────────────────────────
//  DATA STRUCTURES
// ─────────────────────────────────────────────────────────────────────────────────

// A single exchange (user says something, MK responds)
struct MKExchange {
    int turnNumber;             // Position in conversation (1, 2, 3...)
    std::string userInput;      // What the user said
    std::string mkResponse;     // What MK replied
    std::string topic;          // Detected topic of this exchange
    std::string intent;         // question, statement, command, greeting
    std::vector<std::string> keywords; // Important words mentioned
    std::time_t timestamp;      // When this exchange happened
    float satisfaction;         // Estimated user satisfaction (0-1)
};

// Personality mode configuration
enum class MKPersonality {
    CASUAL,                     // Short, friendly, uses contractions
    TECHNICAL,                  // Precise, detailed, formal
    TEACHER                     // Step-by-step, explanatory, patient
};

// Detected user emotion
enum class MKEmotion {
    NEUTRAL,                    // Normal conversation
    FRUSTRATED,                 // "this doesn't work!", "ugh", "why won't it"
    CONFUSED,                   // "I don't understand", "what?", "huh"
    EXCITED,                    // "awesome!", "yes!", "that's great"
    CURIOUS,                    // Asking lots of questions, exploring
    IMPATIENT                   // Short messages, repeated questions
};


// A proactive suggestion to offer the user
struct MKSuggestion {
    std::string text;           // "You might also want to know about..."
    std::string relatedTopic;   // The topic being suggested
    float relevance;            // How relevant to current conversation (0-1)
};

// A chained intent (multi-step command decomposed into parts)
struct MKIntentStep {
    int order;                  // Execution order
    std::string action;         // What to do
    std::string target;         // What it acts on
    bool completed;             // Has this step been done?
    std::string result;         // Result of execution (if completed)
};

// Cross-session memory entry
struct MKSessionMemory {
    std::string topic;          // What topic was discussed
    std::string summary;        // Brief summary of the discussion
    std::vector<std::string> factsLearned;  // New facts from this conversation
    std::string userPreference; // Any user preference detected
    std::time_t sessionDate;    // When this session happened
};

// Conversation summary
struct MKConversationSummary {
    int totalTurns;
    std::vector<std::string> topicsDiscussed;
    std::vector<std::string> factsLearned;
    std::vector<std::string> questionsAsked;
    std::vector<std::string> unresolvedQuestions;
    std::string overallTopic;
    float overallSatisfaction;
};


// ─────────────────────────────────────────────────────────────────────────────────
//  THE CONVERSATION ENGINE CLASS
// ─────────────────────────────────────────────────────────────────────────────────

class MKConversationEngine {
private:
    // ═══════════════════════════════════════════════════════════════════
    //  CONTEXT WINDOW — Last 20 exchanges
    // ═══════════════════════════════════════════════════════════════════
    
    static const int MAX_CONTEXT_SIZE = 20;
    std::deque<MKExchange> contextWindow;
    int turnCounter;
    
    // ═══════════════════════════════════════════════════════════════════
    //  TOPIC & REFERENCE TRACKING
    // ═══════════════════════════════════════════════════════════════════
    
    std::string currentTopic;               // What we're currently talking about
    std::string previousTopic;              // What we were talking about before
    std::vector<std::string> mentionedEntities; // All entities mentioned in this convo
    std::unordered_map<std::string, std::string> pronounMap; // "it" → actual thing
    
    // ═══════════════════════════════════════════════════════════════════
    //  PERSONALITY & EMOTION
    // ═══════════════════════════════════════════════════════════════════
    
    MKPersonality currentPersonality;
    MKEmotion detectedEmotion;
    int frustrationCount;       // Consecutive frustrated messages
    int confusionCount;         // Consecutive confused messages
    
    // ═══════════════════════════════════════════════════════════════════
    //  CROSS-SESSION MEMORY
    // ═══════════════════════════════════════════════════════════════════
    
    std::vector<MKSessionMemory> sessionHistory;
    std::string memoryFilePath;
    
    // ═══════════════════════════════════════════════════════════════════
    //  INTENT CHAINING
    // ═══════════════════════════════════════════════════════════════════
    
    std::vector<MKIntentStep> activeChain;  // Current multi-step command
    bool chainActive;
    
    // Stats
    int totalConversations;
    int totalExchanges;
    int suggestionsOffered;
    int suggestionsAccepted;


    // ═══════════════════════════════════════════════════════════════════
    //  INTERNAL HELPERS
    // ═══════════════════════════════════════════════════════════════════

    std::string toLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    // Extract significant keywords from text (skip stop words)
    std::vector<std::string> extractKeywords(const std::string& text) {
        static std::unordered_set<std::string> stopWords = {
            "a", "an", "the", "is", "are", "was", "were", "be", "been",
            "have", "has", "had", "do", "does", "did", "will", "would",
            "could", "should", "can", "to", "of", "in", "for", "on",
            "with", "at", "by", "from", "as", "and", "but", "or", "not",
            "it", "its", "this", "that", "these", "those", "i", "me",
            "my", "we", "our", "you", "your", "he", "she", "they",
            "what", "how", "why", "where", "when", "who", "which"
        };
        
        std::vector<std::string> keywords;
        std::stringstream ss(toLower(text));
        std::string word;
        while (ss >> word) {
            // Remove punctuation from ends
            while (!word.empty() && !std::isalnum(word.back())) word.pop_back();
            while (!word.empty() && !std::isalnum(word.front())) word.erase(word.begin());
            
            if (word.size() > 2 && stopWords.find(word) == stopWords.end()) {
                keywords.push_back(word);
            }
        }
        return keywords;
    }

    // Determine the main topic from keywords
    std::string detectTopic(const std::vector<std::string>& keywords) {
        if (keywords.empty()) return currentTopic;  // Maintain previous topic
        // The longest keyword is often the most specific/topical
        std::string best;
        for (const auto& kw : keywords) {
            if (kw.size() > best.size()) best = kw;
        }
        return best;
    }


    // ═══════════════════════════════════════════════════════════════════
    //  EMOTION DETECTION
    // ═══════════════════════════════════════════════════════════════════

    MKEmotion detectEmotion(const std::string& input) {
        std::string lower = toLower(input);
        
        // Frustration signals
        if (lower.find("doesn't work") != std::string::npos ||
            lower.find("not working") != std::string::npos ||
            lower.find("broken") != std::string::npos ||
            lower.find("ugh") != std::string::npos ||
            lower.find("damn") != std::string::npos ||
            lower.find("why won't") != std::string::npos ||
            lower.find("still not") != std::string::npos ||
            lower.find("i give up") != std::string::npos) {
            frustrationCount++;
            return MKEmotion::FRUSTRATED;
        }
        
        // Confusion signals
        if (lower.find("don't understand") != std::string::npos ||
            lower.find("confused") != std::string::npos ||
            lower.find("what do you mean") != std::string::npos ||
            lower.find("huh") != std::string::npos ||
            lower.find("makes no sense") != std::string::npos ||
            lower.find("lost") != std::string::npos ||
            (lower.size() < 5 && lower.find("?") != std::string::npos)) {
            confusionCount++;
            return MKEmotion::CONFUSED;
        }
        
        // Excitement signals
        if (lower.find("awesome") != std::string::npos ||
            lower.find("amazing") != std::string::npos ||
            lower.find("great") != std::string::npos ||
            lower.find("perfect") != std::string::npos ||
            lower.find("yes!") != std::string::npos ||
            lower.find("thank") != std::string::npos ||
            lower.find("love it") != std::string::npos) {
            frustrationCount = 0;
            confusionCount = 0;
            return MKEmotion::EXCITED;
        }
        
        // Impatience signals
        if (lower.size() < 10 && turnCounter > 3) {
            // Very short messages after many turns = impatient
            return MKEmotion::IMPATIENT;
        }
        
        // Curiosity signals (multiple questions)
        int questionCount = std::count(lower.begin(), lower.end(), '?');
        if (questionCount >= 2 || lower.find("tell me more") != std::string::npos ||
            lower.find("what else") != std::string::npos) {
            return MKEmotion::CURIOUS;
        }
        
        frustrationCount = std::max(0, frustrationCount - 1);
        confusionCount = std::max(0, confusionCount - 1);
        return MKEmotion::NEUTRAL;
    }


    // Get personality-appropriate prefix based on emotion state
    std::string getEmotionalPrefix() {
        switch (detectedEmotion) {
            case MKEmotion::FRUSTRATED:
                if (frustrationCount >= 3) return "I hear you — let me try a different approach. ";
                return "Let me help fix this. ";
            case MKEmotion::CONFUSED:
                if (confusionCount >= 2) return "Let me explain it differently. ";
                return "Here's a simpler way to look at it: ";
            case MKEmotion::EXCITED:
                return "Glad that's working! ";
            case MKEmotion::IMPATIENT:
                return "";  // Be concise, skip pleasantries
            case MKEmotion::CURIOUS:
                return "Great question! ";
            default:
                return "";
        }
    }

public:
    // ─────────────────────────────────────────────────────────────────────────────
    //  CONSTRUCTOR
    // ─────────────────────────────────────────────────────────────────────────────

    MKConversationEngine(const std::string& memoryFile = "knowledge_files/conversation_memory.mk")
        : turnCounter(0), currentTopic(""), previousTopic(""),
          currentPersonality(MKPersonality::CASUAL), detectedEmotion(MKEmotion::NEUTRAL),
          frustrationCount(0), confusionCount(0), memoryFilePath(memoryFile),
          chainActive(false), totalConversations(0), totalExchanges(0),
          suggestionsOffered(0), suggestionsAccepted(0) {
        
        std::cout << "[CONVERSATION] Conversational AI engine initialized.\n";
        std::cout << "[CONVERSATION] Context window: " << MAX_CONTEXT_SIZE << " exchanges\n";
        loadSessionMemory();
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  CORE: PROCESS USER INPUT (called before generating response)
    // ─────────────────────────────────────────────────────────────────────────────

    // Call this with every user input. Returns resolved text with references expanded.
    std::string processInput(const std::string& userInput) {
        totalExchanges++;
        turnCounter++;
        
        // Detect emotion
        detectedEmotion = detectEmotion(userInput);
        
        // Extract keywords and detect topic
        auto keywords = extractKeywords(userInput);
        std::string newTopic = detectTopic(keywords);
        
        if (newTopic != currentTopic && !newTopic.empty()) {
            previousTopic = currentTopic;
            currentTopic = newTopic;
        }
        
        // Track mentioned entities
        for (const auto& kw : keywords) {
            mentionedEntities.push_back(kw);
        }
        // Keep entity list manageable
        if (mentionedEntities.size() > 50) {
            mentionedEntities.erase(mentionedEntities.begin(), 
                                     mentionedEntities.begin() + 25);
        }
        
        // Resolve references ("it", "that", "the one I mentioned")
        std::string resolved = resolveReferences(userInput);
        
        return resolved;
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  REFERENCE RESOLUTION — "it", "that", "they", "the one I mentioned"
    // ─────────────────────────────────────────────────────────────────────────────

    std::string resolveReferences(const std::string& input) {
        std::string result = input;
        std::string lower = toLower(input);
        
        // Resolve "it" — refers to the most recent singular entity discussed
        if (lower.find(" it ") != std::string::npos || lower.find(" it?") != std::string::npos ||
            lower.find(" it.") != std::string::npos || lower == "it") {
            auto lastRef = pronounMap.find("it");
            if (lastRef != pronounMap.end()) {
                // Replace "it" with the actual entity for query purposes
                // (Keep original for display, but use resolved for graph queries)
                pronounMap["_resolved_it"] = lastRef->second;
            }
        }
        
        // Resolve "that" — refers to the thing just mentioned or discussed
        if (lower.find(" that ") != std::string::npos || lower.find("that?") != std::string::npos) {
            if (!mentionedEntities.empty()) {
                pronounMap["_resolved_that"] = mentionedEntities.back();
            }
        }
        
        // Resolve "they" — refers to a previously mentioned group or plural entity
        if (lower.find(" they ") != std::string::npos || lower.find(" them ") != std::string::npos) {
            auto lastRef = pronounMap.find("they");
            if (lastRef != pronounMap.end()) {
                pronounMap["_resolved_they"] = lastRef->second;
            }
        }
        
        // Resolve "the one I mentioned" / "what I said" / "earlier"
        if (lower.find("earlier") != std::string::npos || 
            lower.find("i mentioned") != std::string::npos ||
            lower.find("i said") != std::string::npos) {
            if (!previousTopic.empty()) {
                pronounMap["_resolved_earlier"] = previousTopic;
            }
        }
        
        return result;
    }

    // Get the resolved reference for a pronoun (call this when building graph queries)
    std::string getResolvedReference(const std::string& pronoun) {
        std::string key = "_resolved_" + toLower(pronoun);
        auto it = pronounMap.find(key);
        if (it != pronounMap.end()) return it->second;
        // Try direct lookup
        auto direct = pronounMap.find(toLower(pronoun));
        if (direct != pronounMap.end()) return direct->second;
        return "";
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  RECORD EXCHANGE — Store the completed exchange in context window
    // ─────────────────────────────────────────────────────────────────────────────

    void recordExchange(const std::string& userInput, const std::string& mkResponse,
                        const std::string& intent, float satisfaction = 0.7f) {
        MKExchange ex;
        ex.turnNumber = turnCounter;
        ex.userInput = userInput;
        ex.mkResponse = mkResponse;
        ex.topic = currentTopic;
        ex.intent = intent;
        ex.keywords = extractKeywords(userInput);
        ex.timestamp = std::time(nullptr);
        ex.satisfaction = satisfaction;
        
        contextWindow.push_back(ex);
        if ((int)contextWindow.size() > MAX_CONTEXT_SIZE) {
            contextWindow.pop_front();
        }
        
        // Update pronoun map — the main subject of this exchange becomes "it"
        if (!ex.keywords.empty()) {
            pronounMap["it"] = ex.keywords[0];
            if (ex.keywords.size() > 1) {
                pronounMap["they"] = ex.keywords[0] + " and " + ex.keywords[1];
            }
        }
    }

    // ─────────────────────────────────────────────────────────────────────────────
    //  PROACTIVE SUGGESTIONS
    // ─────────────────────────────────────────────────────────────────────────────

    // Generate suggestions based on current topic (call after answering)
    std::vector<MKSuggestion> generateSuggestions(MKPatternGraph& graph) {
        std::vector<MKSuggestion> suggestions;
        suggestionsOffered++;
        
        if (currentTopic.empty()) return suggestions;
        
        // Find related concepts in the graph
        auto related = graph.getAll(currentTopic);
        for (const auto& edge : related) {
            // Don't suggest what we already discussed
            bool alreadyDiscussed = false;
            for (const auto& ex : contextWindow) {
                if (ex.topic == edge.target) { alreadyDiscussed = true; break; }
            }
            if (alreadyDiscussed) continue;
            
            MKSuggestion sg;
            sg.relatedTopic = edge.target;
            sg.relevance = edge.weight;
            
            if (edge.relation == "is_a") {
                sg.text = "By the way, " + currentTopic + " is a type of " + edge.target + 
                          ". Want to know more about " + edge.target + "?";
            } else if (edge.relation == "has") {
                sg.text = "Also, " + currentTopic + " has " + edge.target + 
                          ". Interested in that?";
            } else if (edge.relation == "can") {
                sg.text = "Fun fact: " + currentTopic + " can " + edge.target + ".";
            } else {
                sg.text = "Related: " + currentTopic + " " + edge.relation + " " + edge.target + ".";
            }
            
            suggestions.push_back(sg);
            if (suggestions.size() >= 3) break;  // Max 3 suggestions
        }
        
        return suggestions;
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  PERSONALITY MODES
    // ─────────────────────────────────────────────────────────────────────────────

    void setPersonality(MKPersonality mode) {
        currentPersonality = mode;
        std::string modeName;
        switch (mode) {
            case MKPersonality::CASUAL: modeName = "casual"; break;
            case MKPersonality::TECHNICAL: modeName = "technical"; break;
            case MKPersonality::TEACHER: modeName = "teacher"; break;
        }
        std::cout << "[CONVERSATION] Personality set to: " << modeName << "\n";
    }

    MKPersonality getPersonality() const { return currentPersonality; }

    // Adapt response style based on current personality
    std::string adaptStyle(const std::string& rawAnswer) {
        if (rawAnswer.empty()) return rawAnswer;
        
        std::string prefix = getEmotionalPrefix();
        
        switch (currentPersonality) {
            case MKPersonality::CASUAL:
                // Keep it short and friendly
                return prefix + rawAnswer;
                
            case MKPersonality::TECHNICAL:
                // Add precision markers
                return prefix + "Specifically: " + rawAnswer;
                
            case MKPersonality::TEACHER:
                // Add explanatory framing
                return prefix + "Let me explain: " + rawAnswer + 
                       " Does that make sense?";
        }
        return prefix + rawAnswer;
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  INTENT CHAINING — Multi-step command decomposition
    // ─────────────────────────────────────────────────────────────────────────────

    // Decompose a multi-step command into sequential actions
    // e.g., "Build me X and then deploy it" → [build X, deploy X]
    std::vector<MKIntentStep> decomposeChain(const std::string& input) {
        std::vector<MKIntentStep> steps;
        std::string lower = toLower(input);
        
        // Split on chain words: "and then", "then", "after that", "and"
        std::vector<std::string> chainWords = {"and then", "then", "after that", 
                                                "next", "finally", "also"};
        
        std::vector<std::string> parts;
        std::string remaining = lower;
        
        for (const auto& cw : chainWords) {
            size_t pos = remaining.find(cw);
            if (pos != std::string::npos) {
                parts.push_back(remaining.substr(0, pos));
                remaining = remaining.substr(pos + cw.size());
            }
        }
        if (!remaining.empty()) parts.push_back(remaining);
        
        // If no chain words found, treat as single action
        if (parts.empty()) parts.push_back(lower);
        
        int order = 1;
        for (const auto& part : parts) {
            // Trim the part
            std::string trimmed = part;
            size_t start = trimmed.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            trimmed = trimmed.substr(start);
            
            MKIntentStep step;
            step.order = order++;
            step.completed = false;
            
            // Extract action verb and target
            std::stringstream ss(trimmed);
            std::string firstWord;
            ss >> firstWord;
            step.action = firstWord;
            std::getline(ss, step.target);
            if (!step.target.empty() && step.target[0] == ' ') {
                step.target = step.target.substr(1);
            }
            
            steps.push_back(step);
        }
        
        return steps;
    }

    // Start a new intent chain
    void startChain(const std::string& input) {
        activeChain = decomposeChain(input);
        chainActive = !activeChain.empty() && activeChain.size() > 1;
        if (chainActive) {
            std::cout << "[CONVERSATION] Intent chain started: " 
                      << activeChain.size() << " steps\n";
        }
    }

    // Get the next uncompleted step in the chain
    MKIntentStep* getNextChainStep() {
        for (auto& step : activeChain) {
            if (!step.completed) return &step;
        }
        chainActive = false;
        return nullptr;
    }

    // Mark current chain step as complete
    void completeChainStep(const std::string& result) {
        for (auto& step : activeChain) {
            if (!step.completed) {
                step.completed = true;
                step.result = result;
                break;
            }
        }
        // Check if chain is done
        bool allDone = true;
        for (const auto& step : activeChain) {
            if (!step.completed) { allDone = false; break; }
        }
        if (allDone) chainActive = false;
    }

    bool isChainActive() const { return chainActive; }


    // ─────────────────────────────────────────────────────────────────────────────
    //  CONVERSATION SUMMARIZER
    // ─────────────────────────────────────────────────────────────────────────────

    MKConversationSummary summarize() {
        MKConversationSummary summary;
        summary.totalTurns = turnCounter;
        summary.overallSatisfaction = 0.0f;
        
        std::unordered_set<std::string> topics;
        float totalSatisfaction = 0.0f;
        
        for (const auto& ex : contextWindow) {
            if (!ex.topic.empty()) topics.insert(ex.topic);
            totalSatisfaction += ex.satisfaction;
            
            if (ex.intent == "question") {
                summary.questionsAsked.push_back(ex.userInput);
            }
            if (ex.intent == "statement") {
                // Statements might be new facts
                summary.factsLearned.push_back(ex.userInput);
            }
        }
        
        for (const auto& t : topics) summary.topicsDiscussed.push_back(t);
        
        if (!contextWindow.empty()) {
            summary.overallSatisfaction = totalSatisfaction / contextWindow.size();
        }
        summary.overallTopic = currentTopic;
        
        return summary;
    }

    // Generate a text summary of the conversation
    std::string generateSummaryText() {
        auto summary = summarize();
        std::stringstream ss;
        ss << "Conversation Summary (" << summary.totalTurns << " turns):\n";
        ss << "  Main topic: " << summary.overallTopic << "\n";
        ss << "  Topics discussed: ";
        for (const auto& t : summary.topicsDiscussed) ss << t << ", ";
        ss << "\n";
        ss << "  Questions asked: " << summary.questionsAsked.size() << "\n";
        ss << "  Overall satisfaction: " << (int)(summary.overallSatisfaction * 100) << "%\n";
        return ss.str();
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  CROSS-SESSION MEMORY — Persist important conversation context
    // ─────────────────────────────────────────────────────────────────────────────

    // Save current session's key information to disk
    void saveSessionMemory() {
        auto summary = summarize();
        
        MKSessionMemory mem;
        mem.topic = summary.overallTopic;
        mem.summary = generateSummaryText();
        mem.factsLearned = summary.factsLearned;
        mem.sessionDate = std::time(nullptr);
        
        // Detect user preferences from conversation
        for (const auto& ex : contextWindow) {
            std::string lower = toLower(ex.userInput);
            if (lower.find("i like") != std::string::npos ||
                lower.find("i prefer") != std::string::npos ||
                lower.find("i want") != std::string::npos) {
                mem.userPreference = ex.userInput;
                break;
            }
        }
        
        sessionHistory.push_back(mem);
        
        // Write to disk
        std::ofstream out(memoryFilePath, std::ios::app);
        if (out.is_open()) {
            out << "# Session: " << mem.sessionDate << "\n";
            out << "session_topic|was_about|" << mem.topic << "|1.0\n";
            if (!mem.userPreference.empty()) {
                out << "user|preference|" << mem.userPreference << "|1.0\n";
            }
            for (const auto& fact : mem.factsLearned) {
                out << "conversation|learned|" << fact << "|0.8\n";
            }
            out << "\n";
            out.close();
        }
        
        std::cout << "[CONVERSATION] Session memory saved to: " << memoryFilePath << "\n";
    }

    // Load previous session memories from disk
    void loadSessionMemory() {
        std::ifstream in(memoryFilePath);
        if (!in.is_open()) return;
        
        std::string line;
        int loaded = 0;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            // Parse session facts (just count them for now)
            loaded++;
        }
        in.close();
        
        if (loaded > 0) {
            std::cout << "[CONVERSATION] Loaded " << loaded 
                      << " session memory entries from previous sessions.\n";
        }
    }

    // Check if we discussed something in a previous session
    std::string recallPreviousSession(const std::string& topic) {
        std::string lower = toLower(topic);
        for (const auto& mem : sessionHistory) {
            if (toLower(mem.topic).find(lower) != std::string::npos) {
                return "We discussed " + mem.topic + " before. " + mem.summary;
            }
        }
        return "";
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  CONTEXT QUERIES — Access the conversation context
    // ─────────────────────────────────────────────────────────────────────────────

    // Get the current topic being discussed
    std::string getCurrentTopic() const { return currentTopic; }
    
    // Get the previous topic (for "what were we talking about before?")
    std::string getPreviousTopic() const { return previousTopic; }
    
    // Get the detected emotion
    MKEmotion getDetectedEmotion() const { return detectedEmotion; }
    
    // Get the last N exchanges
    std::vector<MKExchange> getRecentExchanges(int count = 5) {
        std::vector<MKExchange> recent;
        int start = std::max(0, (int)contextWindow.size() - count);
        for (int i = start; i < (int)contextWindow.size(); i++) {
            recent.push_back(contextWindow[i]);
        }
        return recent;
    }

    // Get all entities mentioned in this conversation
    std::vector<std::string> getMentionedEntities() const { return mentionedEntities; }
    
    // Reset conversation (new conversation started)
    void newConversation() {
        // Save current session first
        if (turnCounter > 0) {
            saveSessionMemory();
        }
        
        contextWindow.clear();
        turnCounter = 0;
        currentTopic = "";
        previousTopic = "";
        mentionedEntities.clear();
        pronounMap.clear();
        activeChain.clear();
        chainActive = false;
        frustrationCount = 0;
        confusionCount = 0;
        detectedEmotion = MKEmotion::NEUTRAL;
        totalConversations++;
        
        std::cout << "[CONVERSATION] New conversation started. (Total: " 
                  << totalConversations << ")\n";
    }

    // ─────────────────────────────────────────────────────────────────────────────
    //  STATS & DEBUG
    // ─────────────────────────────────────────────────────────────────────────────

    void printStats() const {
        std::string emotionStr;
        switch (detectedEmotion) {
            case MKEmotion::NEUTRAL: emotionStr = "neutral"; break;
            case MKEmotion::FRUSTRATED: emotionStr = "frustrated"; break;
            case MKEmotion::CONFUSED: emotionStr = "confused"; break;
            case MKEmotion::EXCITED: emotionStr = "excited"; break;
            case MKEmotion::CURIOUS: emotionStr = "curious"; break;
            case MKEmotion::IMPATIENT: emotionStr = "impatient"; break;
        }
        std::string persStr;
        switch (currentPersonality) {
            case MKPersonality::CASUAL: persStr = "casual"; break;
            case MKPersonality::TECHNICAL: persStr = "technical"; break;
            case MKPersonality::TEACHER: persStr = "teacher"; break;
        }
        
        std::cout << "\n--- [CONVERSATION ENGINE] ---\n";
        std::cout << "  Current topic: " << currentTopic << "\n";
        std::cout << "  Turn: " << turnCounter << " / context: " 
                  << contextWindow.size() << "\n";
        std::cout << "  Personality: " << persStr << "\n";
        std::cout << "  Detected emotion: " << emotionStr << "\n";
        std::cout << "  Total conversations: " << totalConversations << "\n";
        std::cout << "  Total exchanges: " << totalExchanges << "\n";
        std::cout << "  Entities tracked: " << mentionedEntities.size() << "\n";
        std::cout << "  Chain active: " << (chainActive ? "yes" : "no") << "\n";
        std::cout << "-----------------------------\n";
    }
};

#endif // MK_CONVERSATION_ENGINE_CPP
