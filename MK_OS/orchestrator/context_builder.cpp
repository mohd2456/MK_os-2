// ============================================================
// MK OS - Context Builder
// Produces the smallest effective prompt possible to maximize
// free API token usage. Implements smart pruning: fact scoring,
// history compression, token estimation, and progressive dropping.
// ============================================================
#ifndef MK_CONTEXT_BUILDER_CPP
#define MK_CONTEXT_BUILDER_CPP

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cmath>

// ============================================================
// MKContextBuilder
// A pure logic class that takes strings in and returns an
// optimized prompt string out. No pointers to subsystems needed.
// ============================================================

// Minimal system prompt for when token budget is tight (~30 words)
static const std::string MK_SYSTEM_PROMPT_MINIMAL =
    "You are MK, Mohammed's personal AI. Be concise, helpful, honest. "
    "Use provided facts. Call tools when needed.";

class MKContextBuilder {
public:
    struct PromptResult {
        std::string prompt;
        int estimatedTokens;
        bool usedMinimalPrompt;
        int factsIncluded;
        int historyTurnsIncluded;
    };

    // ============================================================
    // buildPrompt() - Build the smallest effective prompt
    // ============================================================
    PromptResult buildPrompt(
        const std::string& userInput,
        const std::vector<std::string>& rawFacts,
        const std::string& fullHistory,
        const std::string& toolContext,
        const std::string& fullSystemPrompt,
        const std::string& toolPrompt,
        int maxTokenBudget = 1500
    ) {
        PromptResult result;
        result.usedMinimalPrompt = false;
        result.factsIncluded = 0;
        result.historyTurnsIncluded = 0;

        // Score and select the most relevant facts
        std::vector<std::string> selectedFacts = scoreFacts(userInput, rawFacts, 5);
        result.factsIncluded = (int)selectedFacts.size();

        // Compress history
        int historyBudget = maxTokenBudget / 3; // allocate ~1/3 of budget to history
        std::string compressedHistory = compressHistory(fullHistory, historyBudget);
        result.historyTurnsIncluded = countTurns(compressedHistory);

        // Always include tool descriptions - they're short (~200 tokens)
        // and the LLM is smart enough to not use them when unnecessary
        std::string activeToolPrompt = toolPrompt;

        // Try with full system prompt first
        std::string prompt = assemblePrompt(fullSystemPrompt, activeToolPrompt,
                                            selectedFacts, compressedHistory,
                                            toolContext, userInput);
        int tokens = estimateTokens(prompt);

        // Progressive dropping if over budget
        if (tokens > maxTokenBudget) {
            // Step 1: Drop oldest history
            compressedHistory = compressHistory(fullHistory, historyBudget / 2);
            result.historyTurnsIncluded = countTurns(compressedHistory);
            prompt = assemblePrompt(fullSystemPrompt, activeToolPrompt,
                                    selectedFacts, compressedHistory,
                                    toolContext, userInput);
            tokens = estimateTokens(prompt);
        }

        if (tokens > maxTokenBudget) {
            // Step 2: Drop lowest-scored facts
            size_t reducedFactCount = selectedFacts.size() > 3 ? 3 : selectedFacts.size();
            if (reducedFactCount > 2) reducedFactCount = 2;
            std::vector<std::string> reducedFacts(selectedFacts.begin(),
                                                   selectedFacts.begin() + reducedFactCount);
            selectedFacts = reducedFacts;
            result.factsIncluded = (int)selectedFacts.size();
            prompt = assemblePrompt(fullSystemPrompt, activeToolPrompt,
                                    selectedFacts, compressedHistory,
                                    toolContext, userInput);
            tokens = estimateTokens(prompt);
        }

        if (tokens > maxTokenBudget) {
            // Step 3: Switch to minimal system prompt
            result.usedMinimalPrompt = true;
            prompt = assemblePrompt(MK_SYSTEM_PROMPT_MINIMAL, activeToolPrompt,
                                    selectedFacts, compressedHistory,
                                    toolContext, userInput);
            tokens = estimateTokens(prompt);
        }

        if (tokens > maxTokenBudget) {
            // Step 4: Drop tool prompt entirely
            prompt = assemblePrompt(MK_SYSTEM_PROMPT_MINIMAL, "",
                                    selectedFacts, compressedHistory,
                                    toolContext, userInput);
            tokens = estimateTokens(prompt);
        }

        if (tokens > maxTokenBudget) {
            // Step 5: Drop all history
            compressedHistory = "";
            result.historyTurnsIncluded = 0;
            prompt = assemblePrompt(MK_SYSTEM_PROMPT_MINIMAL, "",
                                    selectedFacts, compressedHistory,
                                    toolContext, userInput);
            tokens = estimateTokens(prompt);
        }

        if (tokens > maxTokenBudget) {
            // Step 6: Drop all facts too
            selectedFacts.clear();
            result.factsIncluded = 0;
            prompt = assemblePrompt(MK_SYSTEM_PROMPT_MINIMAL, "",
                                    selectedFacts, "",
                                    toolContext, userInput);
            tokens = estimateTokens(prompt);
        }

        result.prompt = prompt;
        result.estimatedTokens = tokens;
        return result;
    }

    // ============================================================
    // estimateTokens() - Simple chars/4 approximation
    // ============================================================
    int estimateTokens(const std::string& text) {
        if (text.empty()) return 0;
        // Rough approximation: 1 token ~= 4 characters for English text
        return (int)((text.size() + 3) / 4);
    }

    // ============================================================
    // scoreFacts() - Score and filter facts by relevance
    // Scoring criteria:
    //   (a) keyword overlap with user input
    //   (b) personal relevance (facts about mohammed/mk rank higher)
    //   (c) brevity bonus (shorter facts are more token-efficient)
    // ============================================================
    std::vector<std::string> scoreFacts(const std::string& input,
                                         const std::vector<std::string>& facts,
                                         size_t maxFacts) {
        if (facts.empty()) return {};
        if (maxFacts == 0) return {};

        // Tokenize input into meaningful words
        std::vector<std::string> queryWords = tokenizeInput(input);
        if (queryWords.empty() && !facts.empty()) {
            // If no meaningful words, return first few facts (recency bias)
            std::vector<std::string> result;
            for (size_t i = 0; i < facts.size() && i < maxFacts; i++) {
                result.push_back(facts[i]);
            }
            return result;
        }

        struct ScoredFact {
            std::string fact;
            float score;
        };

        std::vector<ScoredFact> scored;
        for (const auto& fact : facts) {
            float score = computeFactScore(fact, queryWords);
            if (score > 0.0f) {
                scored.push_back({fact, score});
            }
        }

        // Sort by score descending
        std::sort(scored.begin(), scored.end(),
                  [](const ScoredFact& a, const ScoredFact& b) {
                      return a.score > b.score;
                  });

        // Format as terse bullets and return top N
        std::vector<std::string> result;
        for (size_t i = 0; i < scored.size() && i < maxFacts; i++) {
            result.push_back(scored[i].fact);
        }
        return result;
    }

    // ============================================================
    // compressHistory() - Compress conversation history
    // Keep last 2 turns verbatim. For older turns, compress to a
    // summary line: "Previously: [topic1, topic2]"
    // ============================================================
    std::string compressHistory(const std::string& fullHistory, int maxTokens) {
        if (fullHistory.empty()) return "";

        // Parse history into individual turns
        std::vector<std::string> turns = parseTurns(fullHistory);
        if (turns.empty()) return "";

        // If history already fits in budget, return as-is
        if (estimateTokens(fullHistory) <= maxTokens) {
            return fullHistory;
        }

        // Keep last 2 turns verbatim
        int verbatimCount = 2;
        if ((int)turns.size() <= verbatimCount) {
            // All turns fit as verbatim, just truncate if needed
            std::string result;
            for (const auto& turn : turns) {
                result += turn + "\n";
            }
            return result;
        }

        // Compress older turns into a summary
        std::vector<std::string> olderTurns(turns.begin(), turns.end() - verbatimCount);
        std::vector<std::string> recentTurns(turns.end() - verbatimCount, turns.end());

        // Extract topics from older turns
        std::vector<std::string> topics = extractTopics(olderTurns);

        std::string compressed;
        if (!topics.empty()) {
            compressed = "Previously discussed: ";
            for (size_t i = 0; i < topics.size(); i++) {
                if (i > 0) compressed += ", ";
                compressed += topics[i];
            }
            compressed += "\n";
        }

        // Append recent turns verbatim
        for (const auto& turn : recentTurns) {
            compressed += turn + "\n";
        }

        // If still over budget, drop the summary
        if (estimateTokens(compressed) > maxTokens) {
            std::string recentOnly;
            for (const auto& turn : recentTurns) {
                recentOnly += turn + "\n";
            }
            return recentOnly;
        }

        return compressed;
    }

private:
    // ============================================================
    // tokenizeInput() - Extract meaningful words from user input
    // ============================================================
    std::vector<std::string> tokenizeInput(const std::string& input) {
        static const std::vector<std::string> stopWords = {
            "a", "an", "the", "is", "are", "was", "were", "be", "been",
            "what", "who", "where", "when", "why", "how", "which",
            "do", "does", "did", "will", "would", "could", "should",
            "can", "may", "might", "shall", "to", "of", "in", "on",
            "at", "by", "for", "with", "from", "up", "about", "into",
            "it", "its", "this", "that", "these", "those", "i", "me",
            "my", "you", "your", "he", "she", "they", "we", "us",
            "and", "or", "but", "not", "no", "so", "if", "than",
            "tell", "show", "give", "get", "know", "think"
        };

        std::string lower = input;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        std::vector<std::string> words;
        std::istringstream ss(lower);
        std::string word;
        while (ss >> word) {
            // Remove trailing punctuation
            while (!word.empty() && (word.back() == '?' || word.back() == '.' ||
                   word.back() == '!' || word.back() == ',')) {
                word.pop_back();
            }
            if (word.size() < 2) continue;
            bool isStop = false;
            for (const auto& sw : stopWords) {
                if (word == sw) { isStop = true; break; }
            }
            if (!isStop) words.push_back(word);
        }
        return words;
    }

    // ============================================================
    // computeFactScore() - Score a single fact against query words
    // ============================================================
    float computeFactScore(const std::string& fact,
                           const std::vector<std::string>& queryWords) {
        std::string factLower = fact;
        std::transform(factLower.begin(), factLower.end(), factLower.begin(), ::tolower);

        float score = 0.0f;

        // (a) Keyword overlap
        int matchCount = 0;
        for (const auto& qw : queryWords) {
            if (factLower.find(qw) != std::string::npos) {
                matchCount++;
            }
        }
        if (matchCount == 0) return 0.0f; // No relevance at all
        score += (float)matchCount * 2.0f;

        // (b) Personal relevance boost (facts about Mohammed/MK rank higher)
        if (factLower.find("mohammed") != std::string::npos ||
            factLower.find("mk") != std::string::npos) {
            score += 3.0f;
        }

        // (c) Brevity bonus (shorter facts are more token-efficient)
        // Facts under 50 chars get a small bonus
        if (fact.size() < 50) {
            score += 1.0f;
        } else if (fact.size() > 100) {
            score -= 0.5f;
        }

        return score;
    }

    // ============================================================
    // parseTurns() - Split history string into individual turns
    // ============================================================
    std::vector<std::string> parseTurns(const std::string& history) {
        std::vector<std::string> turns;
        std::istringstream ss(history);
        std::string line;
        while (std::getline(ss, line)) {
            // Trim whitespace
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            size_t end = line.find_last_not_of(" \t\r\n");
            std::string trimmed = line.substr(start, end - start + 1);
            if (!trimmed.empty()) {
                turns.push_back(trimmed);
            }
        }
        return turns;
    }

    // ============================================================
    // extractTopics() - Extract key topics from conversation turns
    // ============================================================
    std::vector<std::string> extractTopics(const std::vector<std::string>& turns) {
        // Extract meaningful nouns/topics from turns
        // Simple approach: find content words after "user:" prefixes
        std::vector<std::string> topics;
        std::vector<std::string> seen; // deduplicate

        for (const auto& turn : turns) {
            std::string lower = turn;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            // Only extract from user messages
            if (lower.find("user:") != 0) continue;

            // Get the message content after "user: "
            size_t colonPos = lower.find(':');
            if (colonPos == std::string::npos || colonPos + 2 >= lower.size()) continue;
            std::string content = lower.substr(colonPos + 2);

            // Tokenize and find meaningful words
            std::vector<std::string> words = tokenizeInput(content);
            for (const auto& w : words) {
                if (w.size() >= 3) {
                    bool alreadySeen = false;
                    for (const auto& s : seen) {
                        if (s == w) { alreadySeen = true; break; }
                    }
                    if (!alreadySeen) {
                        seen.push_back(w);
                        topics.push_back(w);
                        // Cap at 5 topics for brevity
                        if (topics.size() >= 5) return topics;
                    }
                }
            }
        }
        return topics;
    }

    // ============================================================
    // countTurns() - Count number of turns in compressed history
    // ============================================================
    int countTurns(const std::string& history) {
        if (history.empty()) return 0;
        int count = 0;
        std::istringstream ss(history);
        std::string line;
        while (std::getline(ss, line)) {
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            std::string trimmed = line.substr(start);
            if (trimmed.find("user:") == 0 || trimmed.find("mk:") == 0) {
                count++;
            }
        }
        return count;
    }

    // ============================================================
    // queryMightNeedTools() - Always returns true since tools are
    // always included now. Kept for API compatibility.
    // ============================================================
    bool queryMightNeedTools(const std::string& input) {
        (void)input;
        return true;
    }

    // ============================================================
    // assemblePrompt() - Put all pieces together into one string
    // ============================================================
    std::string assemblePrompt(const std::string& sysPrompt,
                               const std::string& toolPromptStr,
                               const std::vector<std::string>& facts,
                               const std::string& history,
                               const std::string& toolContext,
                               const std::string& userInput) {
        std::string prompt = sysPrompt;

        // Tool descriptions (if included)
        if (!toolPromptStr.empty()) {
            prompt += "\n" + toolPromptStr;
        }

        prompt += "\n\n";

        // Facts as terse bullets
        if (!facts.empty()) {
            prompt += "Facts:\n";
            for (const auto& f : facts) {
                prompt += "- " + f + "\n";
            }
            prompt += "\n";
        }

        // Tool execution context (results from previous tool calls in this turn)
        if (!toolContext.empty()) {
            prompt += "Tool result: " + toolContext + "\n\n";
        }

        // Conversation history
        if (!history.empty()) {
            prompt += history + "\n";
        }

        // User message and response marker
        prompt += "User: " + userInput + "\nMK:";

        return prompt;
    }
};

#endif // MK_CONTEXT_BUILDER_CPP
