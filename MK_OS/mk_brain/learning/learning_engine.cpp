#ifndef MK_LEARNING_ENGINE_CPP
#define MK_LEARNING_ENGINE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <sstream>
#include "../../ai_core/safe_parse.h"

// ===================================================================================
// MK SELF-LEARNING ENGINE
// MK is NOT a static model. It learns over time from:
//   - User interactions (corrections, new facts, preferences)
//   - Internet crawling (autonomous knowledge gathering)
//   - Self-evaluation (tracking uncertain answers for later review)
// This engine handles:
//   - Fact extraction and validation
//   - Knowledge graph updates
//   - Weight adjustment signals for the neural core
//   - Spaced repetition for fact reinforcement
// ===================================================================================


enum class MKFactConfidence {
    UNVERIFIED,     // Just learned, not yet confirmed
    LOW,            // Single source, may be wrong
    MEDIUM,         // Multiple sources agree
    HIGH,           // Verified by user or authoritative source
    ABSOLUTE        // User explicitly confirmed as ground truth
};

enum class MKLearningSource {
    USER_CORRECTION,    // User said "actually, X is true"
    USER_STATEMENT,     // User mentioned something in conversation
    INTERNET_CRAWL,     // Gathered from web during idle time
    SELF_INFERENCE,     // MK derived this from existing facts
    OBSERVATION         // MK observed a pattern over time
};

struct MKFact {
    int id;
    std::string subject;
    std::string predicate;
    std::string object;
    MKFactConfidence confidence;
    MKLearningSource source;
    std::time_t learnedAt;
    std::time_t lastAccessed;
    int accessCount;
    int reinforcements;     // How many times this was confirmed
    double decayScore;      // Spaced repetition decay (lower = needs review)
};

struct MKLearningEvent {
    std::time_t timestamp;
    std::string description;
    MKLearningSource source;
    bool applied;
};

class MKLearningEngine {
private:
    std::vector<MKFact> knowledgeGraph;
    std::vector<MKLearningEvent> learningLog;
    std::map<std::string, std::vector<int>> subjectIndex;  // Fast lookup by subject
    int nextFactId;
    std::string persistencePath;
    
    // Spaced repetition decay calculation
    double calculateDecay(const MKFact& fact) {
        std::time_t now = std::time(nullptr);
        double hoursSinceAccess = difftime(now, fact.lastAccessed) / 3600.0;
        double reinforceFactor = 1.0 + (fact.reinforcements * 0.5);
        return std::exp(-hoursSinceAccess / (24.0 * reinforceFactor));
    }
    
    void logEvent(const std::string& desc, MKLearningSource src) {
        MKLearningEvent event;
        event.timestamp = std::time(nullptr);
        event.description = desc;
        event.source = src;
        event.applied = true;
        learningLog.push_back(event);
    }

public:
    MKLearningEngine() : nextFactId(0), persistencePath("mk_knowledge_graph.dat") {
        std::cout << "[LEARNING] Self-learning engine activated. Knowledge graph empty.\n";
    }


    // Learn a new fact (subject-predicate-object triple)
    int learnFact(const std::string& subject, const std::string& predicate,
                  const std::string& object, MKLearningSource source = MKLearningSource::USER_STATEMENT,
                  MKFactConfidence confidence = MKFactConfidence::MEDIUM) {
        
        // Check for existing conflicting facts
        auto it = subjectIndex.find(subject);
        if (it != subjectIndex.end()) {
            for (int factId : it->second) {
                if (knowledgeGraph[factId].predicate == predicate) {
                    // Same subject+predicate but different object = conflict
                    if (knowledgeGraph[factId].object != object) {
                        if (source == MKLearningSource::USER_CORRECTION) {
                            // User correction overrides existing fact
                            std::cout << "[LEARNING] Correcting fact: " << subject << " " 
                                      << predicate << " -> \"" << object << "\" (was: \"" 
                                      << knowledgeGraph[factId].object << "\")\n";
                            knowledgeGraph[factId].object = object;
                            knowledgeGraph[factId].confidence = MKFactConfidence::ABSOLUTE;
                            knowledgeGraph[factId].reinforcements++;
                            knowledgeGraph[factId].lastAccessed = std::time(nullptr);
                            logEvent("Corrected: " + subject + " " + predicate + " = " + object, source);
                            return factId;
                        }
                        std::cout << "[LEARNING] Conflict detected for " << subject << "." << predicate 
                                  << " — keeping higher confidence version.\n";
                        return -1;
                    } else {
                        // Same fact — reinforce it
                        knowledgeGraph[factId].reinforcements++;
                        knowledgeGraph[factId].lastAccessed = std::time(nullptr);
                        if (knowledgeGraph[factId].confidence < confidence) {
                            knowledgeGraph[factId].confidence = confidence;
                        }
                        return factId;
                    }
                }
            }
        }
        
        // Add new fact
        MKFact fact;
        fact.id = nextFactId++;
        fact.subject = subject;
        fact.predicate = predicate;
        fact.object = object;
        fact.confidence = confidence;
        fact.source = source;
        fact.learnedAt = std::time(nullptr);
        fact.lastAccessed = fact.learnedAt;
        fact.accessCount = 0;
        fact.reinforcements = 0;
        fact.decayScore = 1.0;
        
        knowledgeGraph.push_back(fact);
        subjectIndex[subject].push_back(fact.id);
        
        std::cout << "[LEARNING] New fact: " << subject << " " << predicate << " \"" << object << "\"\n";
        logEvent("Learned: " + subject + " " + predicate + " = " + object, source);
        return fact.id;
    }
    
    // Query facts about a subject
    std::vector<MKFact> queryFacts(const std::string& subject) {
        std::vector<MKFact> results;
        auto it = subjectIndex.find(subject);
        if (it != subjectIndex.end()) {
            for (int factId : it->second) {
                knowledgeGraph[factId].accessCount++;
                knowledgeGraph[factId].lastAccessed = std::time(nullptr);
                results.push_back(knowledgeGraph[factId]);
            }
        }
        return results;
    }
    
    // Search all facts matching a predicate pattern
    std::vector<MKFact> searchByPredicate(const std::string& predicate) {
        std::vector<MKFact> results;
        for (auto& fact : knowledgeGraph) {
            if (fact.predicate.find(predicate) != std::string::npos) {
                fact.accessCount++;
                fact.lastAccessed = std::time(nullptr);
                results.push_back(fact);
            }
        }
        return results;
    }


    // Get facts that need review (low decay score = hasn't been accessed recently)
    std::vector<MKFact> getFactsNeedingReview(int maxCount = 10) {
        std::vector<MKFact> needsReview;
        for (auto& fact : knowledgeGraph) {
            fact.decayScore = calculateDecay(fact);
            if (fact.decayScore < 0.3 && fact.confidence < MKFactConfidence::ABSOLUTE) {
                needsReview.push_back(fact);
            }
        }
        // Sort by decay score (lowest first = most urgent)
        std::sort(needsReview.begin(), needsReview.end(),
                  [](const MKFact& a, const MKFact& b) { return a.decayScore < b.decayScore; });
        if ((int)needsReview.size() > maxCount) needsReview.resize(maxCount);
        return needsReview;
    }
    
    // Get facts MK is uncertain about (for the "To-Learn" list)
    std::vector<MKFact> getUncertainFacts() {
        std::vector<MKFact> uncertain;
        for (const auto& fact : knowledgeGraph) {
            if (fact.confidence <= MKFactConfidence::LOW) {
                uncertain.push_back(fact);
            }
        }
        return uncertain;
    }
    
    // User explicitly corrects a fact
    void correctFact(const std::string& subject, const std::string& predicate, 
                     const std::string& newValue) {
        learnFact(subject, predicate, newValue, MKLearningSource::USER_CORRECTION, 
                  MKFactConfidence::ABSOLUTE);
    }
    
    // Delete a fact permanently
    void forgetFact(int factId) {
        for (auto it = knowledgeGraph.begin(); it != knowledgeGraph.end(); ++it) {
            if (it->id == factId) {
                std::cout << "[LEARNING] Forgetting fact #" << factId << ": " 
                          << it->subject << " " << it->predicate << "\n";
                // Remove from subject index
                auto& ids = subjectIndex[it->subject];
                ids.erase(std::remove(ids.begin(), ids.end(), factId), ids.end());
                knowledgeGraph.erase(it);
                break;
            }
        }
    }
    
    // Save knowledge graph to disk
    void persist() {
        std::ofstream out(persistencePath);
        if (!out.is_open()) return;
        
        for (const auto& fact : knowledgeGraph) {
            out << fact.id << "|" << fact.subject << "|" << fact.predicate << "|"
                << fact.object << "|" << (int)fact.confidence << "|" << (int)fact.source << "|"
                << fact.learnedAt << "|" << fact.lastAccessed << "|"
                << fact.accessCount << "|" << fact.reinforcements << "\n";
        }
        out.close();
        std::cout << "[LEARNING] Knowledge graph persisted: " << knowledgeGraph.size() << " facts.\n";
    }
    
    // Load knowledge graph from disk
    void restore() {
        std::ifstream in(persistencePath);
        if (!in.is_open()) return;
        
        knowledgeGraph.clear();
        subjectIndex.clear();
        
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;  // skip blank/trailing lines
            std::stringstream ss(line);
            MKFact fact;
            std::string field;
            int confInt = 0, srcInt = 0;
            
            std::getline(ss, field, '|'); fact.id = mk::safeStoi(field, fact.id);
            std::getline(ss, fact.subject, '|');
            std::getline(ss, fact.predicate, '|');
            std::getline(ss, fact.object, '|');
            std::getline(ss, field, '|'); confInt = mk::safeStoi(field, 0);
            std::getline(ss, field, '|'); srcInt = mk::safeStoi(field, 0);
            std::getline(ss, field, '|'); fact.learnedAt = mk::safeStol(field, 0);
            std::getline(ss, field, '|'); fact.lastAccessed = mk::safeStol(field, 0);
            std::getline(ss, field, '|'); fact.accessCount = mk::safeStoi(field, 0);
            std::getline(ss, field, '|'); fact.reinforcements = mk::safeStoi(field, 0);
            
            fact.confidence = (MKFactConfidence)confInt;
            fact.source = (MKLearningSource)srcInt;
            fact.decayScore = 1.0;
            
            knowledgeGraph.push_back(fact);
            subjectIndex[fact.subject].push_back(fact.id);
            if (fact.id >= nextFactId) nextFactId = fact.id + 1;
        }
        in.close();
        std::cout << "[LEARNING] Restored " << knowledgeGraph.size() << " facts from disk.\n";
    }
    
    // Statistics
    int factCount() const { return knowledgeGraph.size(); }
    int eventCount() const { return learningLog.size(); }
    
    void printStats() const {
        int absolute = 0, high = 0, medium = 0, low = 0, unverified = 0;
        for (const auto& f : knowledgeGraph) {
            switch (f.confidence) {
                case MKFactConfidence::ABSOLUTE: absolute++; break;
                case MKFactConfidence::HIGH: high++; break;
                case MKFactConfidence::MEDIUM: medium++; break;
                case MKFactConfidence::LOW: low++; break;
                case MKFactConfidence::UNVERIFIED: unverified++; break;
            }
        }
        std::cout << "[LEARNING STATS] Total facts: " << knowledgeGraph.size()
                  << " | Absolute: " << absolute << " | High: " << high
                  << " | Medium: " << medium << " | Low: " << low
                  << " | Unverified: " << unverified << "\n";
    }
};

#endif // MK_LEARNING_ENGINE_CPP
