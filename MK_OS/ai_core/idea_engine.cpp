#ifndef MK_IDEA_ENGINE_CPP
#define MK_IDEA_ENGINE_CPP

#include <string>
#include <vector>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

// ============================================================
// MK IDEA ENGINE — Creative Concept Bridging
// Generates ideas by randomly connecting unrelated concepts.
// ============================================================

struct MKIdea {
    std::string conceptA;
    std::string conceptB;
    std::string bridge;
    std::string idea;
    std::string category;   // invention, business, art, science, lifestyle, tech
    float feasibility;      // 0-1
    float novelty;          // 0-1
    std::time_t timestamp;
};

enum class BridgeStrategy {
    PROPERTY_TRANSFER, FUNCTION_MERGE, SCALE_SHIFT,
    DOMAIN_SWAP, PROBLEM_SOLUTION_CROSS, COMBINATION
};
class MKIdeaEngine {
private:
    std::vector<MKIdea> history;
    int totalGenerated;
    int uniqueCount;
    float sumFeasibility;
    float sumNovelty;
    unsigned int rngState;

    unsigned int nextRand() {
        rngState ^= rngState << 13;
        rngState ^= rngState >> 17;
        rngState ^= rngState << 5;
        return rngState;
    }
    int randInt(int max) { return max <= 0 ? 0 : (int)(nextRand() % (unsigned int)max); }
    float randFloat() { return (float)(nextRand() % 1000) / 1000.0f; }

    bool isDuplicate(const std::string& a, const std::string& b) {
        for (const auto& h : history)
            if ((h.conceptA == a && h.conceptB == b) || (h.conceptA == b && h.conceptB == a))
                return true;
        return false;
    }

    std::string categorize(const std::string&, const std::string&, BridgeStrategy strat) {
        if (strat == BridgeStrategy::FUNCTION_MERGE || strat == BridgeStrategy::COMBINATION) return "invention";
        if (strat == BridgeStrategy::DOMAIN_SWAP) return "business";
        if (strat == BridgeStrategy::SCALE_SHIFT) return "science";
        if (strat == BridgeStrategy::PROBLEM_SOLUTION_CROSS) return "tech";
        static const char* cats[] = {"invention","business","art","science","lifestyle","tech"};
        return cats[randInt(6)];
    }

    float scoreFeasibility(const std::vector<MKEdge>& propsA, const std::vector<MKEdge>& propsB) {
        float base = 0.4f;
        if (!propsA.empty()) base += 0.2f;
        if (!propsB.empty()) base += 0.2f;
        if (propsA.size() > 2 && propsB.size() > 2) base += 0.1f;
        return std::min(1.0f, base + randFloat() * 0.1f);
    }

    float scoreNovelty(int nodeCount, bool hasDirectPath) {
        if (hasDirectPath) return 0.2f + randFloat() * 0.2f;
        float base = 0.5f + randFloat() * 0.3f;
        if (nodeCount > 100) base += 0.1f;
        return std::min(1.0f, base);
    }

    std::string buildBridge(const std::string& a, const std::string& b,
                            const std::vector<MKEdge>& propsA, const std::vector<MKEdge>& propsB,
                            BridgeStrategy& usedStrategy) {
        // 1. Property Transfer
        for (const auto& e : propsA) {
            if (e.relation == "has" || e.relation == "has_property") {
                usedStrategy = BridgeStrategy::PROPERTY_TRANSFER;
                return a + " has " + e.target + " -> what if " + b + " also had " + e.target;
            }
        }
        // 2. Function Merge
        std::string funcA, funcB;
        for (const auto& e : propsA)
            if (e.relation == "can" || e.relation == "does" || e.relation == "used_for") { funcA = e.target; break; }
        for (const auto& e : propsB)
            if (e.relation == "can" || e.relation == "does" || e.relation == "used_for") { funcB = e.target; break; }
        if (!funcA.empty() && !funcB.empty()) {
            usedStrategy = BridgeStrategy::FUNCTION_MERGE;
            return a + " " + funcA + " + " + b + " " + funcB + " -> merged function";
        }
        // 3. Domain Swap
        std::string domA, domB;
        for (const auto& e : propsA)
            if (e.relation == "is_a" || e.relation == "type_of" || e.relation == "domain") { domA = e.target; break; }
        for (const auto& e : propsB)
            if (e.relation == "is_a" || e.relation == "type_of" || e.relation == "domain") { domB = e.target; break; }
        if (!domA.empty() && !domB.empty() && domA != domB) {
            usedStrategy = BridgeStrategy::DOMAIN_SWAP;
            return a + " from " + domA + " applied to " + domB + " domain";
        }
        // 4. Scale Shift
        if (!propsA.empty()) {
            usedStrategy = BridgeStrategy::SCALE_SHIFT;
            return "miniaturize " + a + " + integrate with " + b;
        }
        // 5. Problem-Solution Cross
        if (!propsB.empty()) {
            usedStrategy = BridgeStrategy::PROBLEM_SOLUTION_CROSS;
            return b + "'s approach applied to solve " + a + "'s challenges";
        }
        // 6. Combination (fallback)
        usedStrategy = BridgeStrategy::COMBINATION;
        return a + " combined with " + b + " into one unified concept";
    }

    std::string buildIdeaSentence(const std::string& a, const std::string& b,
                                   const std::string& bridge, BridgeStrategy) {
        switch (randInt(4)) {
            case 0: return "What if " + a + " combined with " + b + "? You'd get: " + bridge;
            case 1: return "Take " + a + "'s essence and apply it to " + b + " -> " + bridge;
            case 2: return a + " meets " + b + ": " + bridge;
            default: return "Combine " + a + " with " + b + " -> " + bridge;
        }
    }

    void recordIdea(const MKIdea& idea) {
        history.push_back(idea);
        if (history.size() > 100) history.erase(history.begin());
        totalGenerated++;
        uniqueCount = (int)history.size();
        sumFeasibility += idea.feasibility;
        sumNovelty += idea.novelty;
    }

    std::vector<std::string> collectNodeNames(MKPatternGraph& graph, int cap = 200) {
        std::vector<std::string> names;
        const auto& allEdges = graph.getAllEdges();
        for (const auto& e : allEdges) {
            if (std::find(names.begin(), names.end(), e.source) == names.end()) names.push_back(e.source);
            if (std::find(names.begin(), names.end(), e.target) == names.end()) names.push_back(e.target);
            if ((int)names.size() >= cap) break;
        }
        return names;
    }

public:
    MKIdeaEngine() : totalGenerated(0), uniqueCount(0), sumFeasibility(0), sumNovelty(0) {
        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        rngState = (unsigned int)(seed ^ 0xDEADBEEF);
        if (rngState == 0) rngState = 42;
        std::cout << "[IDEA ENGINE] Creative bridging module initialized.\n";
    }

    MKIdea generateIdea(MKPatternGraph& graph) {
        MKIdea idea;
        idea.timestamp = std::time(nullptr);
        auto nodeNames = collectNodeNames(graph);
        if (nodeNames.size() < 2) {
            idea.conceptA = "creativity"; idea.conceptB = "knowledge";
            idea.bridge = "need more knowledge to bridge concepts";
            idea.idea = "Add more facts to the knowledge graph for better ideas!";
            idea.category = "lifestyle"; idea.feasibility = 0.5f; idea.novelty = 0.5f;
            recordIdea(idea); return idea;
        }
        // Pick two random unconnected nodes
        std::string a, b; int attempts = 0;
        do {
            a = nodeNames[randInt((int)nodeNames.size())];
            b = nodeNames[randInt((int)nodeNames.size())];
            attempts++;
        } while ((a == b || isDuplicate(a, b)) && attempts < 50);

        idea.conceptA = a; idea.conceptB = b;
        auto propsA = graph.getAll(a);
        auto propsB = graph.getAll(b);
        BridgeStrategy strat = BridgeStrategy::COMBINATION;
        idea.bridge = buildBridge(a, b, propsA, propsB, strat);
        idea.idea = buildIdeaSentence(a, b, idea.bridge, strat);
        idea.category = categorize(a, b, strat);
        auto pathCheck = graph.pathQuery(a, "is_a", 3);
        idea.feasibility = scoreFeasibility(propsA, propsB);
        idea.novelty = scoreNovelty(graph.nodeCount(), pathCheck.found && pathCheck.hops <= 2);
        recordIdea(idea); return idea;
    }

    std::vector<MKIdea> brainstorm(MKPatternGraph& graph, const std::string& topic, int count = 5) {
        std::vector<MKIdea> ideas;
        auto nodeNames = collectNodeNames(graph);
        for (int i = 0; i < count && !nodeNames.empty(); i++) {
            MKIdea idea;
            idea.timestamp = std::time(nullptr);
            idea.conceptA = topic;
            idea.conceptB = nodeNames[randInt((int)nodeNames.size())];
            if (idea.conceptB == topic) idea.conceptB = nodeNames[randInt((int)nodeNames.size())];
            auto propsA = graph.getAll(topic);
            auto propsB = graph.getAll(idea.conceptB);
            BridgeStrategy strat = BridgeStrategy::COMBINATION;
            idea.bridge = buildBridge(topic, idea.conceptB, propsA, propsB, strat);
            idea.idea = buildIdeaSentence(topic, idea.conceptB, idea.bridge, strat);
            idea.category = categorize(topic, idea.conceptB, strat);
            idea.feasibility = scoreFeasibility(propsA, propsB);
            idea.novelty = scoreNovelty(graph.nodeCount(), false);
            recordIdea(idea); ideas.push_back(idea);
        }
        std::sort(ideas.begin(), ideas.end(), [](const MKIdea& x, const MKIdea& y) {
            return (x.feasibility * x.novelty) > (y.feasibility * y.novelty);
        });
        return ideas;
    }

    std::vector<MKIdea> inventFor(MKPatternGraph& graph, const std::string& problem) {
        std::vector<MKIdea> ideas;
        const auto& allEdges = graph.getAllEdges();
        std::vector<std::string> relatedConcepts, otherConcepts;
        std::string probLower = problem;
        std::transform(probLower.begin(), probLower.end(), probLower.begin(), ::tolower);
        for (const auto& e : allEdges) {
            if (probLower.find(e.source) != std::string::npos || probLower.find(e.target) != std::string::npos) {
                relatedConcepts.push_back(e.source); relatedConcepts.push_back(e.target);
            } else {
                if (std::find(otherConcepts.begin(), otherConcepts.end(), e.source) == otherConcepts.end())
                    otherConcepts.push_back(e.source);
            }
            if (otherConcepts.size() > 150) break;
        }
        if (relatedConcepts.empty()) relatedConcepts.push_back(problem);
        if (otherConcepts.empty()) otherConcepts.push_back("technology");
        int count = std::min(5, (int)otherConcepts.size());
        for (int i = 0; i < count; i++) {
            MKIdea idea;
            idea.timestamp = std::time(nullptr);
            idea.conceptA = relatedConcepts[randInt((int)relatedConcepts.size())];
            idea.conceptB = otherConcepts[randInt((int)otherConcepts.size())];
            auto propsA = graph.getAll(idea.conceptA);
            auto propsB = graph.getAll(idea.conceptB);
            idea.bridge = idea.conceptB + "'s approach applied to solve: " + problem;
            idea.idea = "For \"" + problem + "\": combine " + idea.conceptA + " with " + idea.conceptB + " -> " + idea.bridge;
            idea.category = "invention";
            idea.feasibility = scoreFeasibility(propsA, propsB);
            idea.novelty = scoreNovelty(graph.nodeCount(), false);
            recordIdea(idea); ideas.push_back(idea);
        }
        std::sort(ideas.begin(), ideas.end(), [](const MKIdea& x, const MKIdea& y) {
            return (x.feasibility * x.novelty) > (y.feasibility * y.novelty);
        });
        return ideas;
    }

    std::vector<MKIdea> getHistory() const { return history; }
    void saveIdeas(const std::string& filename) {
        std::ofstream out(filename);
        if (!out.is_open()) return;
        for (const auto& idea : history)
            out << idea.conceptA << "|" << idea.conceptB << "|" << idea.bridge << "|"
                << idea.idea << "|" << idea.category << "|" << idea.feasibility << "|"
                << idea.novelty << "|" << idea.timestamp << "\n";
        out.close();
    }
    void loadIdeas(const std::string& filename) {
        std::ifstream in(filename);
        if (!in.is_open()) return;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            MKIdea idea; std::string feas, nov, ts;
            if (std::getline(ss, idea.conceptA, '|') && std::getline(ss, idea.conceptB, '|') &&
                std::getline(ss, idea.bridge, '|') && std::getline(ss, idea.idea, '|') &&
                std::getline(ss, idea.category, '|') && std::getline(ss, feas, '|') &&
                std::getline(ss, nov, '|') && std::getline(ss, ts, '|')) {
                try { idea.feasibility = std::stof(feas); } catch (...) { idea.feasibility = 0.5f; }
                try { idea.novelty = std::stof(nov); } catch (...) { idea.novelty = 0.5f; }
                try { idea.timestamp = (std::time_t)std::stol(ts); } catch (...) { idea.timestamp = 0; }
                history.push_back(idea);
            }
        }
        in.close(); uniqueCount = (int)history.size();
    }

    void printStats() const {
        float avgF = totalGenerated > 0 ? sumFeasibility / totalGenerated : 0.0f;
        float avgN = totalGenerated > 0 ? sumNovelty / totalGenerated : 0.0f;
        std::cout << "\n--- [IDEA ENGINE] ---\n" << "  Total: " << totalGenerated
                  << " | Unique: " << uniqueCount << " | Avg feas: " << (int)(avgF*100)
                  << "% | Avg novelty: " << (int)(avgN*100) << "%\n---------------------\n";
    }
};

#endif // MK_IDEA_ENGINE_CPP
