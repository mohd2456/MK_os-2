#ifndef MK_BIOGRAPHICAL_CPP
#define MK_BIOGRAPHICAL_CPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <regex>

// ===================================================================================
// MK BIOGRAPHICAL FACT EXTRACTOR
// Automatically parses conversations to find and index permanent facts about the
// user: name, birthday, family members, preferences, habits, locations, etc.
// Feeds extracted facts directly into the MKLearningEngine knowledge graph.
// ===================================================================================

enum class MKBioFactCategory {
    IDENTITY,       // Name, age, nationality
    FAMILY,         // Parents, siblings, spouse, children
    LOCATION,       // Where they live, work, travel
    PREFERENCE,     // Likes, dislikes, favorites
    HABIT,          // Daily routines, schedules
    WORK,           // Job, skills, projects
    HEALTH,         // Physical/mental health notes
    MEMORY,         // Things they want to remember
    RELATIONSHIP,   // Friends, colleagues
    DEVICE          // Hardware they own, specs
};

struct MKExtractedFact {
    std::string subject;
    std::string predicate;
    std::string object;
    MKBioFactCategory category;
    float confidence;       // 0.0 to 1.0, how sure we are
    std::string sourceQuote; // The exact text that triggered extraction
};

class MKBiographicalExtractor {
private:
    std::string userName;
    std::map<std::string, std::vector<MKExtractedFact>> extractedFacts;
    int totalExtracted;

    // Pattern matchers for common biographical statements
    struct PatternRule {
        std::string trigger;       // Keyword to look for
        std::string predicate;     // What relationship it implies
        MKBioFactCategory category;
        float baseConfidence;
    };

    std::vector<PatternRule> rules;

    void initRules() {
        // Identity patterns
        rules.push_back({"my name is", "has_name", MKBioFactCategory::IDENTITY, 0.95f});
        rules.push_back({"i am", "is_described_as", MKBioFactCategory::IDENTITY, 0.7f});
        rules.push_back({"i'm", "is_described_as", MKBioFactCategory::IDENTITY, 0.7f});
        rules.push_back({"years old", "has_age", MKBioFactCategory::IDENTITY, 0.9f});
        
        // Family patterns
        rules.push_back({"my mom", "has_mother", MKBioFactCategory::FAMILY, 0.9f});
        rules.push_back({"my dad", "has_father", MKBioFactCategory::FAMILY, 0.9f});
        rules.push_back({"my brother", "has_brother", MKBioFactCategory::FAMILY, 0.9f});
        rules.push_back({"my sister", "has_sister", MKBioFactCategory::FAMILY, 0.9f});
        rules.push_back({"my wife", "has_spouse", MKBioFactCategory::FAMILY, 0.9f});
        rules.push_back({"my husband", "has_spouse", MKBioFactCategory::FAMILY, 0.9f});
        
        // Location patterns
        rules.push_back({"i live in", "lives_in", MKBioFactCategory::LOCATION, 0.9f});
        rules.push_back({"i'm from", "is_from", MKBioFactCategory::LOCATION, 0.85f});
        rules.push_back({"i moved to", "lives_in", MKBioFactCategory::LOCATION, 0.85f});
        
        // Preference patterns
        rules.push_back({"i like", "likes", MKBioFactCategory::PREFERENCE, 0.8f});
        rules.push_back({"i love", "loves", MKBioFactCategory::PREFERENCE, 0.85f});
        rules.push_back({"i hate", "dislikes", MKBioFactCategory::PREFERENCE, 0.85f});
        rules.push_back({"i don't like", "dislikes", MKBioFactCategory::PREFERENCE, 0.8f});
        rules.push_back({"my favorite", "has_favorite", MKBioFactCategory::PREFERENCE, 0.9f});
        
        // Work patterns
        rules.push_back({"i work at", "works_at", MKBioFactCategory::WORK, 0.9f});
        rules.push_back({"i work as", "works_as", MKBioFactCategory::WORK, 0.9f});
        rules.push_back({"my job is", "works_as", MKBioFactCategory::WORK, 0.9f});
        rules.push_back({"i'm building", "is_building", MKBioFactCategory::WORK, 0.75f});
        
        // Habit patterns
        rules.push_back({"i usually", "usually_does", MKBioFactCategory::HABIT, 0.7f});
        rules.push_back({"every morning", "morning_routine", MKBioFactCategory::HABIT, 0.75f});
        rules.push_back({"every night", "night_routine", MKBioFactCategory::HABIT, 0.75f});
        rules.push_back({"i always", "always_does", MKBioFactCategory::HABIT, 0.8f});
        
        // Device patterns
        rules.push_back({"my laptop", "owns_device", MKBioFactCategory::DEVICE, 0.85f});
        rules.push_back({"my phone", "owns_device", MKBioFactCategory::DEVICE, 0.85f});
        rules.push_back({"my computer", "owns_device", MKBioFactCategory::DEVICE, 0.85f});
        rules.push_back({"macbook", "owns_device", MKBioFactCategory::DEVICE, 0.8f});
        
        // Memory/reminder patterns
        rules.push_back({"remember that", "should_remember", MKBioFactCategory::MEMORY, 0.95f});
        rules.push_back({"don't forget", "should_remember", MKBioFactCategory::MEMORY, 0.95f});
        rules.push_back({"keep in mind", "should_remember", MKBioFactCategory::MEMORY, 0.9f});
    }


    // Extract the object (value) after a trigger phrase
    std::string extractAfterTrigger(const std::string& text, const std::string& trigger) {
        size_t pos = text.find(trigger);
        if (pos == std::string::npos) return "";
        
        size_t start = pos + trigger.size();
        // Skip leading whitespace
        while (start < text.size() && text[start] == ' ') start++;
        
        // Extract until end of sentence or common delimiters
        std::string value;
        for (size_t i = start; i < text.size(); i++) {
            char c = text[i];
            if (c == '.' || c == ',' || c == '!' || c == '?' || c == '\n') break;
            value += c;
        }
        
        // Trim trailing whitespace
        while (!value.empty() && value.back() == ' ') value.pop_back();
        return value;
    }

public:
    MKBiographicalExtractor() : userName("user"), totalExtracted(0) {
        initRules();
        std::cout << "[FACT EXTRACTOR] Biographical parser initialized with " 
                  << rules.size() << " extraction patterns.\n";
    }
    
    void setUserName(const std::string& name) { userName = name; }
    
    // Main extraction function: scans a message for biographical facts
    std::vector<MKExtractedFact> extractFromMessage(const std::string& message) {
        std::vector<MKExtractedFact> found;
        
        // Convert to lowercase for matching
        std::string lower = message;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        for (const auto& rule : rules) {
            size_t pos = lower.find(rule.trigger);
            if (pos != std::string::npos) {
                std::string value = extractAfterTrigger(lower, rule.trigger);
                if (value.empty() || value.size() > 100) continue; // Skip garbage
                
                MKExtractedFact fact;
                fact.subject = userName;
                fact.predicate = rule.predicate;
                fact.object = value;
                fact.category = rule.category;
                fact.confidence = rule.baseConfidence;
                fact.sourceQuote = message.substr(
                    std::max((int)pos - 5, 0), 
                    std::min((int)(rule.trigger.size() + value.size() + 15), (int)message.size()));
                
                found.push_back(fact);
                totalExtracted++;
                
                std::cout << "[FACT EXTRACTED] " << fact.subject << " " 
                          << fact.predicate << " \"" << fact.object 
                          << "\" (confidence=" << fact.confidence << ")\n";
            }
        }
        
        // Store in category index
        for (const auto& fact : found) {
            std::string catKey = std::to_string((int)fact.category);
            extractedFacts[catKey].push_back(fact);
        }
        
        return found;
    }
    
    // Extract from a full conversation history
    std::vector<MKExtractedFact> extractFromConversation(const std::vector<std::string>& messages) {
        std::vector<MKExtractedFact> allFacts;
        for (const auto& msg : messages) {
            auto facts = extractFromMessage(msg);
            allFacts.insert(allFacts.end(), facts.begin(), facts.end());
        }
        return allFacts;
    }
    
    // Get all facts of a specific category
    std::vector<MKExtractedFact> getFactsByCategory(MKBioFactCategory category) {
        std::string key = std::to_string((int)category);
        if (extractedFacts.find(key) != extractedFacts.end()) {
            return extractedFacts[key];
        }
        return {};
    }
    
    // Build a user profile summary from all extracted facts
    std::string buildProfileSummary() {
        std::stringstream summary;
        summary << "=== MK USER PROFILE ===\n";
        summary << "Subject: " << userName << "\n\n";
        
        auto printCategory = [&](MKBioFactCategory cat, const std::string& label) {
            auto facts = getFactsByCategory(cat);
            if (!facts.empty()) {
                summary << "[" << label << "]\n";
                for (const auto& f : facts) {
                    summary << "  - " << f.predicate << ": " << f.object << "\n";
                }
                summary << "\n";
            }
        };
        
        printCategory(MKBioFactCategory::IDENTITY, "IDENTITY");
        printCategory(MKBioFactCategory::FAMILY, "FAMILY");
        printCategory(MKBioFactCategory::LOCATION, "LOCATION");
        printCategory(MKBioFactCategory::PREFERENCE, "PREFERENCES");
        printCategory(MKBioFactCategory::WORK, "WORK");
        printCategory(MKBioFactCategory::HABIT, "HABITS");
        printCategory(MKBioFactCategory::DEVICE, "DEVICES");
        printCategory(MKBioFactCategory::MEMORY, "REMEMBERED");
        
        return summary.str();
    }
    
    int getTotalExtracted() const { return totalExtracted; }
    
    void printStats() const {
        std::cout << "[FACT EXTRACTOR] Total facts extracted: " << totalExtracted
                  << " | Categories active: " << extractedFacts.size() << "\n";
    }
};

#endif // MK_BIOGRAPHICAL_CPP
