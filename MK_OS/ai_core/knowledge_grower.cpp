#ifndef MK_KNOWLEDGE_GROWER_CPP
#define MK_KNOWLEDGE_GROWER_CPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <unordered_set>

// ============================================================
// MK Knowledge Grower — Nightly Auto-Growth
// Researches knowledge gaps, verifies facts, and adds them
// to the knowledge graph. Runs on a nightly schedule with
// safety limits and audit logging.
// ============================================================

struct MKVerifiedFact {
    std::string source;
    std::string relation;
    std::string target;
    float confidence;
    std::string topic;
    std::string researchedFrom;     // Source of info
};

struct MKGrowthStats {
    int factsAddedToday;
    int factsAddedThisWeek;
    int factsAddedAllTime;
    int topicsResearched;
    float avgConfidence;
    std::string nextScheduledGrowth;
};

class MKKnowledgeGrower {
private:
    std::string growthLog;
    std::string factsFile;
    int maxFactsPerNight;
    float minConfidence;
    int factsAddedToday;
    int factsAddedThisWeek;
    int factsAddedAllTime;
    int topicsResearchedTotal;
    float totalConfidence;
    std::time_t lastGrowthTime;
    std::unordered_set<std::string> addedFactKeys;

    // Generate a unique key for a fact (to detect duplicates)
    std::string factKey(const std::string& src, const std::string& rel,
                        const std::string& tgt) {
        return src + "|" + rel + "|" + tgt;
    }

    // Format timestamp
    std::string formatTime(std::time_t t) {
        char buf[32];
        struct tm* tm_info = std::localtime(&t);
        if (tm_info) {
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
            return std::string(buf);
        }
        return "unknown";
    }

    // Append to growth log
    void log(const std::string& message) {
        std::ofstream file(growthLog, std::ios::app);
        if (!file.is_open()) return;
        file << "[" << formatTime(std::time(nullptr)) << "] " << message << "\n";
        file.close();
    }

    // Check if a fact already exists in the graph
    bool factExistsInGraph(const MKPatternGraph& graph,
                           const std::string& src, const std::string& rel,
                           const std::string& tgt) {
        // Check our local tracking set
        if (addedFactKeys.find(factKey(src, rel, tgt)) != addedFactKeys.end()) {
            return true;
        }
        // Check the graph edges
        const auto& edges = graph.getAllEdges();
        for (const auto& e : edges) {
            if (e.source == src && e.relation == rel && e.target == tgt) {
                return true;
            }
        }
        return false;
    }

    // Load existing fact keys from the facts file
    void loadExistingFacts() {
        std::ifstream file(factsFile);
        if (!file.is_open()) return;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            // Format: source|relation|target|weight
            std::stringstream ss(line);
            std::string src, rel, tgt;
            if (std::getline(ss, src, '|') && std::getline(ss, rel, '|') &&
                std::getline(ss, tgt, '|')) {
                addedFactKeys.insert(factKey(src, rel, tgt));
            }
        }
        file.close();
    }

public:
    MKKnowledgeGrower(const std::string& logPath = "mk_growth.log",
                      const std::string& factsPath =
                          "ai_core/hre/knowledge_files/learned_facts.mk",
                      int maxFacts = 50, float minConf = 0.6f)
        : growthLog(logPath), factsFile(factsPath),
          maxFactsPerNight(maxFacts), minConfidence(minConf),
          factsAddedToday(0), factsAddedThisWeek(0), factsAddedAllTime(0),
          topicsResearchedTotal(0), totalConfidence(0.0f), lastGrowthTime(0) {
        loadExistingFacts();
        std::cout << "[GROWER] Knowledge grower initialized. "
                  << "Max/night: " << maxFactsPerNight
                  << " | Min confidence: " << (int)(minConfidence * 100)
                  << "% | Tracked facts: " << addedFactKeys.size() << "\n";
    }

    // Get top N unresearched knowledge gaps
    std::vector<MKKnowledgeGap> getTopGaps(MKSelfImprover& improver,
                                           int maxTopics = 10) {
        return improver.getUnresearchedGaps(maxTopics);
    }

    // Research a topic using the knowledge integrator
    std::vector<MKVerifiedFact> researchTopic(
            const std::string& topic,
            MKKnowledgeIntegrator& integrator) {
        std::vector<MKVerifiedFact> verified;

        std::cout << "  " << Color::BCYAN << "\u25CF" << Color::RESET
                  << " Researching: " << Color::BOLD << topic
                  << Color::RESET << "\n";

        // Use the integrator's full pipeline (search + verify)
        auto learnedFacts = integrator.learnFromInternet(topic);
        if (learnedFacts.empty()) {
            std::cout << "  " << Color::DIM
                      << "  No results found for topic." << Color::RESET << "\n";
            log("RESEARCH: no results for \"" + topic + "\"");
            return verified;
        }

        // Convert learned facts to our verified format
        for (const auto& fact : learnedFacts) {
            if (!fact.verified) continue;
            if (fact.confidence < minConfidence) continue;

            // Extract simple fact structure from text
            // Look for "X is Y" or "X has Y" patterns in the snippet
            std::string text = fact.text;
            size_t isPos = text.find(" is ");
            size_t hasPos = text.find(" has ");
            size_t locPos = text.find(" located in ");

            std::string src, rel, tgt;
            if (isPos != std::string::npos && isPos > 0 && isPos < 60) {
                src = text.substr(0, isPos);
                rel = "is_a";
                tgt = text.substr(isPos + 4);
            } else if (hasPos != std::string::npos && hasPos > 0 && hasPos < 60) {
                src = text.substr(0, hasPos);
                rel = "has";
                tgt = text.substr(hasPos + 5);
            } else if (locPos != std::string::npos && locPos > 0 && locPos < 60) {
                src = text.substr(0, locPos);
                rel = "located_in";
                tgt = text.substr(locPos + 12);
            } else {
                continue; // Skip facts we can't parse into graph edges
            }

            // Trim to reasonable length
            if (src.size() > 50) src = src.substr(0, 50);
            if (tgt.size() > 80) {
                size_t period = tgt.find('.');
                if (period != std::string::npos && period < 80) {
                    tgt = tgt.substr(0, period);
                } else {
                    tgt = tgt.substr(0, 80);
                }
            }

            MKVerifiedFact vf;
            vf.source = src;
            vf.relation = rel;
            vf.target = tgt;
            vf.confidence = fact.confidence;
            vf.topic = topic;
            vf.researchedFrom = fact.source;
            verified.push_back(vf);
        }

        log("RESEARCH: found " + std::to_string(verified.size()) +
            " potential facts for \"" + topic + "\"");
        return verified;
    }

    // Add verified facts to the knowledge graph
    int addVerifiedFacts(MKPatternGraph& graph,
                         const std::vector<MKVerifiedFact>& facts,
                         const std::string& topic) {
        int added = 0;
        for (const auto& fact : facts) {
            // Safety: check daily limit
            if (factsAddedToday >= maxFactsPerNight) {
                std::cout << "  " << Color::YELLOW << "\u26A0" << Color::RESET
                          << " Daily limit reached (" << maxFactsPerNight
                          << " facts). Stopping.\n";
                log("LIMIT: daily max reached (" +
                    std::to_string(maxFactsPerNight) + ")");
                break;
            }

            // Safety: skip low confidence
            if (fact.confidence < minConfidence) continue;

            // Safety: skip duplicates
            if (factExistsInGraph(graph, fact.source, fact.relation, fact.target)) {
                continue;
            }

            // Add to graph and persist
            graph.persistNewFact(fact.source, fact.relation, fact.target,
                                fact.confidence);
            addedFactKeys.insert(factKey(fact.source, fact.relation, fact.target));
            factsAddedToday++;
            factsAddedAllTime++;
            totalConfidence += fact.confidence;
            added++;
        }

        if (added > 0) {
            std::cout << "  " << Color::GREEN << "  +" << added
                      << Color::RESET << " facts added about " << topic << "\n";
            log("ADDED: " + std::to_string(added) + " facts about \"" + topic + "\"");
        }
        return added;
    }

    // Full nightly growth pipeline
    std::string runNightlyGrowth(MKPatternGraph& graph,
                                 MKSelfImprover& improver,
                                 MKKnowledgeIntegrator& integrator,
                                 int maxTopics = 10) {
        std::cout << "\n  " << Color::BOLD << Color::BGREEN
                  << "\u2699 Nightly Knowledge Growth" << Color::RESET << "\n";
        log("NIGHTLY: starting growth session");

        // Reset daily counter
        factsAddedToday = 0;
        int totalAdded = 0;
        int topicsProcessed = 0;

        // Step 1: Get top knowledge gaps
        auto gaps = getTopGaps(improver, maxTopics);
        if (gaps.empty()) {
            std::cout << "  " << Color::DIM
                      << "No knowledge gaps to research."
                      << Color::RESET << "\n";
            log("NIGHTLY: no gaps found");
            return "No knowledge gaps to research.";
        }

        std::cout << "  " << Color::BCYAN << "\u25CF" << Color::RESET
                  << " Found " << gaps.size() << " topics to research.\n";

        // Step 2: Research each gap topic
        for (const auto& gap : gaps) {
            if (factsAddedToday >= maxFactsPerNight) break;

            auto facts = researchTopic(gap.topic, integrator);

            // Step 3: Add verified facts
            int added = addVerifiedFacts(graph, facts, gap.topic);
            totalAdded += added;
            topicsProcessed++;

            // Step 4: Mark gap as researched
            if (!facts.empty()) {
                improver.markGapResearched(gap.query);
                if (added > 0) {
                    improver.markGapResolved(gap.query);
                }
            }

            topicsResearchedTotal++;
        }

        lastGrowthTime = std::time(nullptr);
        factsAddedThisWeek += totalAdded;

        // Generate summary
        std::string summary = "Learned " + std::to_string(totalAdded) +
                              " new facts about " + std::to_string(topicsProcessed) +
                              " topics.";
        std::cout << "\n  " << Color::BGREEN << "\u2713" << Color::RESET
                  << " " << summary << "\n";
        log("NIGHTLY: complete - " + summary);
        return summary;
    }

    // Get growth report
    std::string getGrowthReport() {
        std::string report;
        report += Color::BOLD;
        report += "  Knowledge Growth Report\n";
        report += Color::RESET;
        report += "  Facts added today:     " +
                  std::to_string(factsAddedToday) + "\n";
        report += "  Facts added this week: " +
                  std::to_string(factsAddedThisWeek) + "\n";
        report += "  Facts added all time:  " +
                  std::to_string(factsAddedAllTime) + "\n";
        report += "  Topics researched:     " +
                  std::to_string(topicsResearchedTotal) + "\n";

        float avgConf = (factsAddedAllTime > 0) ?
            (totalConfidence / (float)factsAddedAllTime) : 0.0f;
        report += "  Avg confidence:        " +
                  std::to_string((int)(avgConf * 100)) + "%\n";

        report += "  Max facts/night:       " +
                  std::to_string(maxFactsPerNight) + "\n";
        report += "  Min confidence:        " +
                  std::to_string((int)(minConfidence * 100)) + "%\n";
        report += "  Last growth:           " +
                  (lastGrowthTime > 0 ? formatTime(lastGrowthTime) : "never") + "\n";
        report += "  Next scheduled:        2:00 AM (nightly)\n";
        return report;
    }

    MKGrowthStats getStats() const {
        MKGrowthStats stats;
        stats.factsAddedToday = factsAddedToday;
        stats.factsAddedThisWeek = factsAddedThisWeek;
        stats.factsAddedAllTime = factsAddedAllTime;
        stats.topicsResearched = topicsResearchedTotal;
        stats.avgConfidence = (factsAddedAllTime > 0) ?
            (totalConfidence / (float)factsAddedAllTime) : 0.0f;
        stats.nextScheduledGrowth = "2:00 AM";
        return stats;
    }

    void printStats() const {
        std::cout << "  " << Color::BOLD << Color::BGREEN
                  << "  Knowledge Grower:" << Color::RESET << "\n";
        std::cout << "    Facts added (total): " << factsAddedAllTime << "\n";
        std::cout << "    Topics researched: " << topicsResearchedTotal << "\n";
        std::cout << "    Max/night: " << maxFactsPerNight << "\n";
    }
};

#endif // MK_KNOWLEDGE_GROWER_CPP
