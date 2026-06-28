// ============================================================
// MK OS - Input Preprocessor
// Sits between raw user input and the AI brain. Cleans messy,
// casual, or slang-filled text so that the keyword-based router
// and knowledge graph can understand it.
//
// Pipeline: raw input -> slang expansion -> spelling correction
//           -> intent detection -> pronoun resolution -> clean output
// ============================================================
#ifndef MK_INPUT_PREPROCESSOR_CPP
#define MK_INPUT_PREPROCESSOR_CPP

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <deque>
#include <cmath>
#include <iostream>
#include <curl/curl.h>

// ============================================================
// Intent Types
// ============================================================
enum class MKIntentType {
    QUESTION,
    COMMAND,
    LEARNING,
    GREETING,
    WEATHER,
    TIME_QUERY,
    SEARCH,
    UNKNOWN
};

// ============================================================
// Preprocessing Result
// ============================================================
struct MKPreprocessResult {
    std::string cleaned_text;
    MKIntentType detected_intent;
    float confidence;
    bool was_modified;
    std::string original_text;
};

// ============================================================
// Context Entry (for pronoun resolution)
// ============================================================
struct MKContextEntry {
    std::string user_message;
    std::string subject;       // extracted main subject
};

// ============================================================
// MKInputPreprocessor Class
// ============================================================
class MKInputPreprocessor {
private:
    // Slang/shorthand dictionary
    std::unordered_map<std::string, std::string> slangMap;

    // Words to strip entirely (casual filler, note tone)
    std::vector<std::string> stripWords;

    // Known vocabulary for fuzzy matching (from router keywords)
    std::vector<std::string> knownVocabulary;

    // Context buffer: last 5 user messages
    std::deque<MKContextEntry> contextBuffer;
    static const size_t MAX_CONTEXT = 5;

    // --------------------------------------------------------
    // Initialize slang dictionary (50+ entries)
    // --------------------------------------------------------
    void initSlangMap() {
        // Contractions and informal speech
        slangMap["gonna"] = "going to";
        slangMap["wanna"] = "want to";
        slangMap["gotta"] = "got to";
        slangMap["lemme"] = "let me";
        slangMap["gimme"] = "give me";
        slangMap["kinda"] = "kind of";
        slangMap["sorta"] = "sort of";
        slangMap["dunno"] = "don't know";
        slangMap["ain't"] = "is not";
        slangMap["y'all"] = "you all";
        slangMap["prolly"] = "probably";
        slangMap["tryna"] = "trying to";
        slangMap["shoulda"] = "should have";
        slangMap["coulda"] = "could have";
        slangMap["woulda"] = "would have";
        slangMap["hafta"] = "have to";
        slangMap["oughta"] = "ought to";
        slangMap["lotsa"] = "lots of";
        slangMap["outta"] = "out of";

        // Internet abbreviations
        slangMap["idk"] = "I don't know";
        slangMap["ngl"] = "";
        slangMap["tbh"] = "to be honest";
        slangMap["imo"] = "in my opinion";
        slangMap["imho"] = "in my humble opinion";
        slangMap["rn"] = "right now";
        slangMap["atm"] = "at the moment";
        slangMap["btw"] = "by the way";
        slangMap["iirc"] = "if I recall correctly";
        slangMap["afaik"] = "as far as I know";
        slangMap["fwiw"] = "for what it's worth";
        slangMap["smh"] = "";
        slangMap["omg"] = "";
        slangMap["brb"] = "be right back";
        slangMap["ttyl"] = "talk to you later";
        slangMap["ftw"] = "for the win";

        // Shortened words
        slangMap["ur"] = "your";
        slangMap["u"] = "you";
        slangMap["r"] = "are";
        slangMap["dat"] = "that";
        slangMap["da"] = "the";
        slangMap["bout"] = "about";
        slangMap["'bout"] = "about";
        slangMap["abt"] = "about";
        slangMap["smth"] = "something";
        slangMap["ppl"] = "people";
        slangMap["tho"] = "though";
        slangMap["doe"] = "though";
        slangMap["thru"] = "through";
        slangMap["cuz"] = "because";
        slangMap["bc"] = "because";
        slangMap["b/c"] = "because";
        slangMap["w/"] = "with";
        slangMap["w/o"] = "without";
        slangMap["wat"] = "what";
        slangMap["wut"] = "what";
        slangMap["wats"] = "what is";
        slangMap["whats"] = "what is";
        slangMap["hows"] = "how is";
        slangMap["whos"] = "who is";
        slangMap["wheres"] = "where is";
        slangMap["whens"] = "when is";
        slangMap["thx"] = "thanks";
        slangMap["ty"] = "thanks";
        slangMap["plz"] = "please";
        slangMap["pls"] = "please";
        slangMap["nah"] = "no";
        slangMap["yep"] = "yes";
        slangMap["yea"] = "yes";
        slangMap["yeah"] = "yes";
        slangMap["nope"] = "no";
        slangMap["srsly"] = "seriously";
        slangMap["rly"] = "really";
        slangMap["obv"] = "obviously";
        slangMap["def"] = "definitely";
        slangMap["prob"] = "probably";
        slangMap["diff"] = "difference";
        slangMap["info"] = "information";
        slangMap["convo"] = "conversation";
        slangMap["pic"] = "picture";
        slangMap["pics"] = "pictures";
        slangMap["msg"] = "message";
        slangMap["govt"] = "government";
        slangMap["temp"] = "temperature";
        slangMap["ez"] = "easy";
        slangMap["luv"] = "love";
        slangMap["gud"] = "good";
        slangMap["rite"] = "right";
        slangMap["enuf"] = "enough";
        slangMap["sum"] = "some";
        slangMap["evry"] = "every";
        slangMap["dey"] = "they";
        slangMap["dis"] = "this";
        slangMap["dem"] = "them";
        slangMap["wer"] = "were";
    }

    // --------------------------------------------------------
    // Initialize strip words (casual fillers)
    // --------------------------------------------------------
    void initStripWords() {
        stripWords = {
            "yo", "bro", "fam", "bruh", "dude", "man",
            "lol", "lmao", "rofl", "haha", "hehe",
            "like", "literally", "basically", "honestly"
        };
    }

    // --------------------------------------------------------
    // Initialize known vocabulary for fuzzy matching
    // --------------------------------------------------------
    void initVocabulary() {
        knownVocabulary = {
            // Router keywords
            "weather", "time", "search", "what", "where", "when", "why", "how",
            "tell", "explain", "define", "compare", "learn", "remember",
            "news", "temperature", "forecast", "timezone", "current",
            "who", "which", "show", "find", "get", "give", "help",
            // Graph keywords
            "capital", "population", "president", "inventor", "discovered",
            "founded", "element", "planet", "country", "language", "species",
            "formula", "equation", "theorem",
            // Reasoning keywords
            "analyze", "evaluate", "solve", "calculate", "derive", "prove",
            "difference", "advantages", "disadvantages", "consequence",
            // General
            "please", "thanks", "hello", "about", "think", "know",
            "write", "create", "generate", "compose", "story", "code",
            "today", "tomorrow", "yesterday", "morning", "evening",
            "night", "world", "earth", "science", "history", "technology"
        };
    }

    // --------------------------------------------------------
    // Levenshtein distance between two strings
    // --------------------------------------------------------
    int levenshteinDistance(const std::string& s1, const std::string& s2) {
        size_t len1 = s1.size();
        size_t len2 = s2.size();

        // Quick bounds check for performance
        if (len1 == 0) return (int)len2;
        if (len2 == 0) return (int)len1;
        int diff = (int)len1 - (int)len2;
        if (diff > 2 || diff < -2) return 3; // early exit if too different

        // Use two-row approach for memory efficiency
        std::vector<int> prev(len2 + 1);
        std::vector<int> curr(len2 + 1);

        for (size_t j = 0; j <= len2; j++) prev[j] = (int)j;

        for (size_t i = 1; i <= len1; i++) {
            curr[0] = (int)i;
            for (size_t j = 1; j <= len2; j++) {
                int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
                curr[j] = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
            }
            std::swap(prev, curr);
        }
        return prev[len2];
    }

    // --------------------------------------------------------
    // Convert string to lowercase
    // --------------------------------------------------------
    std::string toLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    // --------------------------------------------------------
    // Split text into words
    // --------------------------------------------------------
    std::vector<std::string> splitWords(const std::string& text) {
        std::vector<std::string> words;
        std::istringstream stream(text);
        std::string word;
        while (stream >> word) {
            words.push_back(word);
        }
        return words;
    }

    // --------------------------------------------------------
    // Join words back into a string
    // --------------------------------------------------------
    std::string joinWords(const std::vector<std::string>& words) {
        std::string result;
        for (size_t i = 0; i < words.size(); i++) {
            if (i > 0) result += " ";
            result += words[i];
        }
        return result;
    }

    // --------------------------------------------------------
    // Strip punctuation from a word for lookup (preserve original)
    // --------------------------------------------------------
    std::string stripPunctuation(const std::string& word) {
        std::string result;
        for (char c : word) {
            if (std::isalnum(c) || c == '\'' || c == '/') {
                result += c;
            }
        }
        return result;
    }

    // --------------------------------------------------------
    // Check if a word is a strip word (casual filler)
    // --------------------------------------------------------
    bool isStripWord(const std::string& lower_word) {
        for (const auto& sw : stripWords) {
            if (lower_word == sw) return true;
        }
        return false;
    }

    // --------------------------------------------------------
    // Apply slang dictionary expansion
    // --------------------------------------------------------
    std::string expandSlang(const std::string& text, bool& modified) {
        std::vector<std::string> words = splitWords(text);
        std::vector<std::string> result;

        for (const auto& word : words) {
            std::string cleaned = stripPunctuation(word);
            std::string lower = toLower(cleaned);

            // Check strip words first
            if (isStripWord(lower)) {
                modified = true;
                continue; // skip this word
            }

            // Check slang dictionary
            auto it = slangMap.find(lower);
            if (it != slangMap.end()) {
                modified = true;
                if (!it->second.empty()) {
                    result.push_back(it->second);
                }
                // If empty string, the word is stripped
            } else {
                result.push_back(word);
            }
        }

        return joinWords(result);
    }

    // --------------------------------------------------------
    // Apply fuzzy spelling correction
    // --------------------------------------------------------
    std::string correctSpelling(const std::string& text, bool& modified) {
        std::vector<std::string> words = splitWords(text);
        std::vector<std::string> result;

        for (const auto& word : words) {
            std::string lower = toLower(word);

            // Skip very short words (1-2 chars) and already-known words
            if (lower.size() <= 2) {
                result.push_back(word);
                continue;
            }

            // Check if it already matches a known vocabulary word
            bool known = false;
            for (const auto& vocab : knownVocabulary) {
                if (lower == vocab) {
                    known = true;
                    break;
                }
            }
            if (known) {
                result.push_back(word);
                continue;
            }

            // Check if it is in the slang map (already processed)
            if (slangMap.find(lower) != slangMap.end()) {
                result.push_back(word);
                continue;
            }

            // Try fuzzy matching against known vocabulary
            std::string bestMatch;
            int bestDist = 3; // threshold: only correct if distance <= 2

            for (const auto& vocab : knownVocabulary) {
                // Quick length check for performance
                int lenDiff = (int)lower.size() - (int)vocab.size();
                if (lenDiff > 2 || lenDiff < -2) continue;

                int dist = levenshteinDistance(lower, vocab);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestMatch = vocab;
                }
                if (bestDist == 1) break; // close enough, stop searching
            }

            if (!bestMatch.empty() && bestDist <= 2) {
                modified = true;
                result.push_back(bestMatch);
            } else {
                result.push_back(word);
            }
        }

        return joinWords(result);
    }

    // --------------------------------------------------------
    // Detect intent from the cleaned text
    // --------------------------------------------------------
    MKIntentType detectIntent(const std::string& text, float& confidence) {
        std::string lower = toLower(text);

        // Greeting detection
        std::vector<std::string> greetings = {
            "hi", "hey", "hello", "what's up", "whats up", "how are you",
            "howdy", "sup", "good morning", "good evening", "good afternoon",
            "greetings", "hola", "yo"
        };
        for (const auto& g : greetings) {
            if (lower == g || lower.find(g) == 0) {
                confidence = 0.9f;
                return MKIntentType::GREETING;
            }
        }

        // Weather detection: location + weather words
        std::vector<std::string> weatherWords = {
            "weather", "temperature", "rain", "snow", "sunny", "cloudy",
            "forecast", "humid", "wind", "storm", "cold", "hot"
        };
        for (const auto& w : weatherWords) {
            if (lower.find(w) != std::string::npos) {
                confidence = 0.85f;
                return MKIntentType::WEATHER;
            }
        }

        // Time detection: timezone/city + time words
        std::vector<std::string> timeWords = {
            "time", "clock", "date", "timezone", "hour", "day"
        };
        bool hasTimeWord = false;
        for (const auto& t : timeWords) {
            if (lower.find(t) != std::string::npos) {
                hasTimeWord = true;
                break;
            }
        }
        if (hasTimeWord) {
            confidence = 0.8f;
            return MKIntentType::TIME_QUERY;
        }

        // Learning detection
        std::vector<std::string> learnPatterns = {
            "remember", "know that", "is a", "has a", "learn", "teach"
        };
        for (const auto& lp : learnPatterns) {
            if (lower.find(lp) != std::string::npos) {
                confidence = 0.75f;
                return MKIntentType::LEARNING;
            }
        }

        // Question detection: starts with question words or ends with ?
        std::vector<std::string> questionWords = {
            "what", "where", "when", "why", "how", "who", "which",
            "is", "are", "do", "does", "can", "could", "would", "will"
        };
        if (!lower.empty() && lower.back() == '?') {
            confidence = 0.9f;
            return MKIntentType::QUESTION;
        }
        for (const auto& qw : questionWords) {
            if (lower.find(qw) == 0 || lower.find(qw + " ") == 0) {
                confidence = 0.8f;
                return MKIntentType::QUESTION;
            }
        }

        // Command detection: starts with imperative verbs
        std::vector<std::string> commandVerbs = {
            "tell", "show", "find", "get", "explain", "define", "describe",
            "list", "give", "search", "look up", "open", "run", "start",
            "stop", "help", "calculate", "solve", "compare", "analyze"
        };
        for (const auto& cv : commandVerbs) {
            if (lower.find(cv) == 0) {
                confidence = 0.8f;
                return MKIntentType::COMMAND;
            }
        }

        // Search intent for current events patterns
        std::vector<std::string> searchPatterns = {
            "latest", "recent", "news", "update", "happening", "trending"
        };
        for (const auto& sp : searchPatterns) {
            if (lower.find(sp) != std::string::npos) {
                confidence = 0.7f;
                return MKIntentType::SEARCH;
            }
        }

        // Default: unknown
        confidence = 0.4f;
        return MKIntentType::UNKNOWN;
    }

    // --------------------------------------------------------
    // Extract the main subject from text (simple heuristic)
    // --------------------------------------------------------
    std::string extractSubject(const std::string& text) {
        std::string lower = toLower(text);
        std::vector<std::string> words = splitWords(lower);

        // Skip question/command words and prepositions to find nouns
        std::vector<std::string> skipWords = {
            "what", "where", "when", "why", "how", "who", "which",
            "is", "are", "do", "does", "can", "could", "would", "will",
            "tell", "show", "find", "get", "explain", "the", "a", "an",
            "me", "about", "of", "in", "on", "at", "to", "for", "with",
            "please", "thanks", "and", "or", "but", "if", "then"
        };

        // Find the first "content" word that is likely the subject
        for (const auto& w : words) {
            bool skip = false;
            for (const auto& sw : skipWords) {
                if (w == sw) { skip = true; break; }
            }
            if (!skip && w.size() > 2) {
                return w;
            }
        }

        // Fallback: return the whole text trimmed
        if (!words.empty()) return words.back();
        return text;
    }

    // --------------------------------------------------------
    // Resolve pronouns using context buffer
    // --------------------------------------------------------
    std::string resolvePronouns(const std::string& text, bool& modified) {
        if (contextBuffer.empty()) return text;

        std::vector<std::string> pronouns = {
            "it", "that", "this", "they", "he", "she", "them"
        };

        std::string lower = toLower(text);
        std::vector<std::string> words = splitWords(text);
        bool hasUnresolvedPronoun = false;
        size_t pronounIdx = 0;

        // Check if text contains a pronoun that could reference context
        for (size_t i = 0; i < words.size(); i++) {
            std::string wLower = toLower(stripPunctuation(words[i]));
            for (const auto& p : pronouns) {
                if (wLower == p) {
                    hasUnresolvedPronoun = true;
                    pronounIdx = i;
                    break;
                }
            }
            if (hasUnresolvedPronoun) break;
        }

        if (!hasUnresolvedPronoun) return text;

        // Check if the pronoun has an obvious referent in the SAME sentence
        // (e.g., "tell me what it means" after mentioning something in same sentence)
        // If so, don't resolve -- only resolve when pronoun is ambiguous
        // Simple heuristic: if sentence is short (< 6 words), pronoun likely refers to context
        if (words.size() >= 8) return text;

        // Get the most recent subject from context
        std::string recentSubject;
        for (auto rit = contextBuffer.rbegin(); rit != contextBuffer.rend(); ++rit) {
            if (!rit->subject.empty()) {
                recentSubject = rit->subject;
                break;
            }
        }

        if (recentSubject.empty()) return text;

        // Replace the pronoun with the subject
        words[pronounIdx] = recentSubject;
        modified = true;
        return joinWords(words);
    }

public:
    // --------------------------------------------------------
    // Constructor
    // --------------------------------------------------------
    MKInputPreprocessor() {
        initSlangMap();
        initStripWords();
        initVocabulary();
        std::cout << "[PREPROCESSOR] Input preprocessing engine initialized.\n";
    }

    // --------------------------------------------------------
    // Main processing pipeline (rule-based, fast)
    // --------------------------------------------------------
    MKPreprocessResult process(const std::string& input) {
        MKPreprocessResult result;
        result.original_text = input;
        result.was_modified = false;
        result.confidence = 0.0f;

        if (input.empty()) {
            result.cleaned_text = input;
            result.detected_intent = MKIntentType::UNKNOWN;
            result.confidence = 0.0f;
            return result;
        }

        bool modified = false;
        std::string text = input;

        // Step 1: Expand slang/shorthand
        text = expandSlang(text, modified);

        // Step 2: Fuzzy spelling correction
        text = correctSpelling(text, modified);

        // Step 3: Pronoun resolution from context
        text = resolvePronouns(text, modified);

        // Step 4: Detect intent
        float intentConfidence = 0.0f;
        MKIntentType intent = detectIntent(text, intentConfidence);

        // Build result
        result.cleaned_text = text;
        result.detected_intent = intent;
        result.confidence = intentConfidence;
        result.was_modified = modified;

        // Update context buffer
        MKContextEntry entry;
        entry.user_message = text;
        entry.subject = extractSubject(text);
        contextBuffer.push_back(entry);
        if (contextBuffer.size() > MAX_CONTEXT) {
            contextBuffer.pop_front();
        }

        return result;
    }

    // --------------------------------------------------------
    // Optional: preprocess with Ollama model
    // Falls back to rule-based if Ollama is unavailable.
    // Only called if rule-based confidence is low (< 0.5).
    // Note: requires MKHTTP to be available (included after this file).
    // The caller should use process() directly; this method is provided
    // for future integration when Ollama support is fully wired.
    // --------------------------------------------------------
    MKPreprocessResult preprocessWithModel(const std::string& input) {
        // Ollama integration: attempt a connection to localhost:11434
        // Using libcurl directly to avoid include-order dependency on MKHTTP
        CURL* curl = curl_easy_init();
        if (!curl) {
            return process(input);
        }

        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/tags");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1L); // 1 second timeout
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
            ((std::string*)userp)->append((char*)contents, size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        // If connection failed or no models found, fall back to rule-based
        if (res != CURLE_OK || response.empty() || response.find("models") == std::string::npos) {
            return process(input);
        }

        // Ollama is available -- for now, still use rule-based processing
        // as the POST endpoint requires more complex setup.
        // Future: send text to Ollama for AI-powered cleaning.
        return process(input);
    }

    // --------------------------------------------------------
    // Get intent as a readable string
    // --------------------------------------------------------
    static std::string intentToString(MKIntentType intent) {
        switch (intent) {
            case MKIntentType::QUESTION:   return "QUESTION";
            case MKIntentType::COMMAND:    return "COMMAND";
            case MKIntentType::LEARNING:   return "LEARNING";
            case MKIntentType::GREETING:   return "GREETING";
            case MKIntentType::WEATHER:    return "WEATHER";
            case MKIntentType::TIME_QUERY: return "TIME_QUERY";
            case MKIntentType::SEARCH:     return "SEARCH";
            case MKIntentType::UNKNOWN:    return "UNKNOWN";
        }
        return "UNKNOWN";
    }

    // --------------------------------------------------------
    // Clear context buffer (e.g., on session reset)
    // --------------------------------------------------------
    void clearContext() {
        contextBuffer.clear();
    }
};

#endif // MK_INPUT_PREPROCESSOR_CPP
