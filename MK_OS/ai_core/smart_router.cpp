#ifndef MK_SMART_ROUTER_CPP
#define MK_SMART_ROUTER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <ctime>
#include <cmath>

// ===================================================================================
// MK SMART QUERY ROUTER
// Analyzes incoming queries and routes them to the optimal processing path:
//   INSTANT  - time/date/system queries -> realtime_apis (0ms overhead)
//   GRAPH    - factual lookups -> pattern_graph (microseconds)
//   SEARCH   - current events/unknown -> search_engine (seconds)
//   REASON   - complex multi-step -> deep_reasoner (milliseconds)
//   GENERATE - creative tasks -> tiny LLM (if available)
// Tracks routing decisions and accuracy to self-optimize over time.
// Implements confidence-based fallback chain: try cheap first, escalate if needed.
// ===================================================================================

enum class MKRouteType {
    INSTANT,    // Real-time/system data (time, weather, etc.)
    GRAPH,      // Knowledge graph lookup (fast, local facts)
    SEARCH,     // Internet search (slower, current events)
    REASON,     // Multi-step reasoning (complex queries)
    GENERATE,   // Creative generation (LLM, if available)
    CRYPTO,     // Crypto/trading questions and commands
    HOMELAB,    // Device/service management
    MIND        // Goals, mastery, strategy, learning
};

struct MKRoutingDecision {
    MKRouteType primaryRoute;
    MKRouteType fallbackRoute;
    float confidence;           // How confident we are in this routing
    std::string reason;         // Why this route was chosen
    std::time_t timestamp;
    bool wasSuccessful;         // Set after execution
    bool usedFallback;          // Did we have to fall back?
};

struct MKRouteStats {
    int totalRouted;
    int successful;
    int fallbackUsed;
    float avgConfidence;
};

class MKSmartRouter {
private:
    std::vector<MKRoutingDecision> history;
    std::unordered_map<std::string, int> routeCounters;  // route -> usage count

    // Configuration
    int maxHistorySize;

    // Per-route accuracy tracking
    int instantTotal, instantSuccess;
    int graphTotal, graphSuccess;
    int searchTotal, searchSuccess;
    int reasonTotal, reasonSuccess;
    int generateTotal, generateSuccess;
    int cryptoTotal, cryptoSuccess;
    int homelabTotal, homelabSuccess;
    int mindTotal, mindSuccess;

    // Keywords that indicate each route type
    std::vector<std::string> instantKeywords;
    std::vector<std::string> graphKeywords;
    std::vector<std::string> searchKeywords;
    std::vector<std::string> reasonKeywords;
    std::vector<std::string> generateKeywords;
    std::vector<std::string> cryptoKeywords;
    std::vector<std::string> homelabKeywords;
    std::vector<std::string> mindKeywords;

    // Convert string to lowercase
    std::string toLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    // Count keyword matches in query
    int countKeywordMatches(const std::string& query, const std::vector<std::string>& keywords) {
        std::string lower = toLower(query);
        int matches = 0;
        for (const auto& kw : keywords) {
            if (lower.find(kw) != std::string::npos) {
                matches++;
            }
        }
        return matches;
    }

    // Get route name as string
    std::string routeToString(MKRouteType route) {
        switch (route) {
            case MKRouteType::INSTANT:  return "INSTANT";
            case MKRouteType::GRAPH:    return "GRAPH";
            case MKRouteType::SEARCH:   return "SEARCH";
            case MKRouteType::REASON:   return "REASON";
            case MKRouteType::GENERATE: return "GENERATE";
            case MKRouteType::CRYPTO:   return "CRYPTO";
            case MKRouteType::HOMELAB:  return "HOMELAB";
            case MKRouteType::MIND:     return "MIND";
        }
        return "UNKNOWN";
    }

    // Get the natural fallback for each route type
    MKRouteType getFallbackFor(MKRouteType primary) {
        switch (primary) {
            case MKRouteType::INSTANT:  return MKRouteType::SEARCH;
            case MKRouteType::GRAPH:    return MKRouteType::SEARCH;
            case MKRouteType::SEARCH:   return MKRouteType::REASON;
            case MKRouteType::REASON:   return MKRouteType::SEARCH;
            case MKRouteType::GENERATE: return MKRouteType::REASON;
            case MKRouteType::CRYPTO:   return MKRouteType::GRAPH;
            case MKRouteType::HOMELAB:  return MKRouteType::GRAPH;
            case MKRouteType::MIND:     return MKRouteType::GRAPH;
        }
        return MKRouteType::SEARCH;
    }

    // Prune old history entries
    void pruneHistory() {
        if ((int)history.size() > maxHistorySize) {
            int toRemove = (int)history.size() - maxHistorySize;
            history.erase(history.begin(), history.begin() + toRemove);
        }
    }

    // Calculate route accuracy (success rate)
    float getRouteAccuracy(MKRouteType route) {
        int total = 0, success = 0;
        switch (route) {
            case MKRouteType::INSTANT:  total = instantTotal; success = instantSuccess; break;
            case MKRouteType::GRAPH:    total = graphTotal; success = graphSuccess; break;
            case MKRouteType::SEARCH:   total = searchTotal; success = searchSuccess; break;
            case MKRouteType::REASON:   total = reasonTotal; success = reasonSuccess; break;
            case MKRouteType::GENERATE: total = generateTotal; success = generateSuccess; break;
            case MKRouteType::CRYPTO:   total = cryptoTotal; success = cryptoSuccess; break;
            case MKRouteType::HOMELAB:  total = homelabTotal; success = homelabSuccess; break;
            case MKRouteType::MIND:     total = mindTotal; success = mindSuccess; break;
        }
        if (total == 0) return 0.5f; // Unknown accuracy defaults to 50%
        return (float)success / (float)total;
    }

    void initKeywords() {
        // INSTANT: time, date, weather, system status queries
        instantKeywords = {
            "time", "date", "clock", "today", "now", "weather", "temperature",
            "forecast", "timezone", "day of week", "current", "what time",
            "exchange rate", "currency", "news", "headlines", "ip address",
            "geolocation", "where am i"
        };

        // GRAPH: factual lookups, definitions, relationships
        graphKeywords = {
            "who is", "define", "meaning", "capital of",
            "population", "president", "inventor", "discovered", "founded",
            "element", "planet", "country", "language", "species",
            "formula", "equation", "theorem", "law of", "fact"
        };

        // SEARCH: current events, recent news, things that change
        searchKeywords = {
            "latest", "recent", "breaking", "update", "happening",
            "this year", "this month", "this week", "2024", "2025", "2026",
            "new release", "announced", "launched", "controversy",
            "score", "results", "election", "market", "stock price"
        };

        // REASON: complex analysis, multi-step problems
        reasonKeywords = {
            "why", "how does", "explain", "compare", "difference between",
            "advantages", "disadvantages", "analyze", "evaluate", "solve",
            "calculate", "derive", "prove", "step by step", "reasoning",
            "if then", "consequence", "implication", "trade-off"
        };

        // GENERATE: creative tasks, writing, brainstorming
        generateKeywords = {
            "write", "create", "generate", "compose", "draft",
            "story", "poem", "essay", "code", "script",
            "brainstorm", "ideas", "suggest", "imagine", "creative",
            "design", "invent", "name for", "slogan", "tagline"
        };

        // CRYPTO: cryptocurrency, trading, blockchain
        cryptoKeywords = {
            "crypto", "bitcoin", "ethereum", "portfolio", "trade",
            "signal", "price", "coin", "defi", "airdrop",
            "token", "blockchain", "wallet", "exchange", "staking",
            "yield", "liquidity", "nft", "altcoin", "mining"
        };

        // HOMELAB: device management, servers, containers
        homelabKeywords = {
            "device", "server", "docker", "container", "ssh",
            "homelab", "service", "deploy", "raspberry", "vm",
            "monitor", "uptime", "cluster", "network", "host"
        };

        // MIND: goals, mastery, strategy, learning, earning
        mindKeywords = {
            "goal", "plan", "strategy", "mastery", "skill",
            "learn", "improve", "earn", "money", "funding",
            "progress", "objective", "milestone", "growth", "schedule"
        };
    }

public:
    MKSmartRouter(int maxHistory = 5000)
        : maxHistorySize(maxHistory),
          instantTotal(0), instantSuccess(0),
          graphTotal(0), graphSuccess(0),
          searchTotal(0), searchSuccess(0),
          reasonTotal(0), reasonSuccess(0),
          generateTotal(0), generateSuccess(0),
          cryptoTotal(0), cryptoSuccess(0),
          homelabTotal(0), homelabSuccess(0),
          mindTotal(0), mindSuccess(0) {
        initKeywords();
        std::cout << "[ROUTER] Smart query router initialized.\n";
        std::cout << "[ROUTER] Routes: INSTANT | GRAPH | SEARCH | REASON | GENERATE | CRYPTO | HOMELAB | MIND\n";
    }

    // ─── Route a Query ─────────────────────────────────────────────────────────
    // Analyzes the query and returns the optimal routing decision
    MKRoutingDecision route(const std::string& query) {
        MKRoutingDecision decision;
        decision.timestamp = std::time(nullptr);
        decision.wasSuccessful = false;
        decision.usedFallback = false;

        // Score each route based on keyword matches
        int instantScore = countKeywordMatches(query, instantKeywords);
        int graphScore = countKeywordMatches(query, graphKeywords);
        int searchScore = countKeywordMatches(query, searchKeywords);
        int reasonScore = countKeywordMatches(query, reasonKeywords);
        int generateScore = countKeywordMatches(query, generateKeywords);
        int cryptoScore = countKeywordMatches(query, cryptoKeywords);
        int homelabScore = countKeywordMatches(query, homelabKeywords);
        int mindScore = countKeywordMatches(query, mindKeywords);

        // Apply accuracy-based weighting (routes that historically work get a boost)
        float instantWeighted = (float)instantScore * (0.5f + getRouteAccuracy(MKRouteType::INSTANT));
        float graphWeighted = (float)graphScore * (0.5f + getRouteAccuracy(MKRouteType::GRAPH));
        float searchWeighted = (float)searchScore * (0.5f + getRouteAccuracy(MKRouteType::SEARCH));
        float reasonWeighted = (float)reasonScore * (0.5f + getRouteAccuracy(MKRouteType::REASON));
        float generateWeighted = (float)generateScore * (0.5f + getRouteAccuracy(MKRouteType::GENERATE));
        float cryptoWeighted = (float)cryptoScore * (0.5f + getRouteAccuracy(MKRouteType::CRYPTO));
        float homelabWeighted = (float)homelabScore * (0.5f + getRouteAccuracy(MKRouteType::HOMELAB));
        float mindWeighted = (float)mindScore * (0.5f + getRouteAccuracy(MKRouteType::MIND));

        // Find the highest scoring route
        float maxScore = 0.0f;
        MKRouteType bestRoute = MKRouteType::GRAPH; // Default to graph for factual queries

        if (instantWeighted > maxScore) { maxScore = instantWeighted; bestRoute = MKRouteType::INSTANT; }
        if (graphWeighted > maxScore) { maxScore = graphWeighted; bestRoute = MKRouteType::GRAPH; }
        if (searchWeighted > maxScore) { maxScore = searchWeighted; bestRoute = MKRouteType::SEARCH; }
        if (reasonWeighted > maxScore) { maxScore = reasonWeighted; bestRoute = MKRouteType::REASON; }
        if (generateWeighted > maxScore) { maxScore = generateWeighted; bestRoute = MKRouteType::GENERATE; }
        if (cryptoWeighted > maxScore) { maxScore = cryptoWeighted; bestRoute = MKRouteType::CRYPTO; }
        if (homelabWeighted > maxScore) { maxScore = homelabWeighted; bestRoute = MKRouteType::HOMELAB; }
        if (mindWeighted > maxScore) { maxScore = mindWeighted; bestRoute = MKRouteType::MIND; }

        // Calculate confidence based on how much the winner leads
        float totalScore = instantWeighted + graphWeighted + searchWeighted
                         + reasonWeighted + generateWeighted
                         + cryptoWeighted + homelabWeighted + mindWeighted;
        if (totalScore > 0.0f) {
            decision.confidence = maxScore / totalScore;
        } else {
            decision.confidence = 0.3f; // Low confidence when no keywords match
            bestRoute = MKRouteType::GRAPH; // Default to knowledge graph
        }

        decision.primaryRoute = bestRoute;
        decision.fallbackRoute = getFallbackFor(bestRoute);
        decision.reason = "Best keyword match: " + routeToString(bestRoute)
                        + " (score: " + std::to_string(maxScore) + ")";

        // Track the decision
        history.push_back(decision);
        routeCounters[routeToString(bestRoute)]++;
        pruneHistory();

        std::cout << "[ROUTER] Query: \"" << query.substr(0, 50) << "...\" -> "
                  << routeToString(bestRoute) << " (confidence: "
                  << (int)(decision.confidence * 100) << "%, fallback: "
                  << routeToString(decision.fallbackRoute) << ")\n";

        return decision;
    }

    // ─── Report Outcome ────────────────────────────────────────────────────────
    // Called after a query is processed to track if routing was correct
    void reportOutcome(MKRouteType route, bool success) {
        switch (route) {
            case MKRouteType::INSTANT:
                instantTotal++; if (success) instantSuccess++; break;
            case MKRouteType::GRAPH:
                graphTotal++; if (success) graphSuccess++; break;
            case MKRouteType::SEARCH:
                searchTotal++; if (success) searchSuccess++; break;
            case MKRouteType::REASON:
                reasonTotal++; if (success) reasonSuccess++; break;
            case MKRouteType::GENERATE:
                generateTotal++; if (success) generateSuccess++; break;
            case MKRouteType::CRYPTO:
                cryptoTotal++; if (success) cryptoSuccess++; break;
            case MKRouteType::HOMELAB:
                homelabTotal++; if (success) homelabSuccess++; break;
            case MKRouteType::MIND:
                mindTotal++; if (success) mindSuccess++; break;
        }

        // Update the most recent history entry
        if (!history.empty()) {
            history.back().wasSuccessful = success;
        }
    }

    // Report that a fallback was used
    void reportFallback(MKRouteType originalRoute, MKRouteType fallbackRoute, bool success) {
        reportOutcome(originalRoute, false);  // Primary failed
        reportOutcome(fallbackRoute, success); // Fallback result

        if (!history.empty()) {
            history.back().usedFallback = true;
        }

        std::cout << "[ROUTER] Fallback triggered: " << routeToString(originalRoute)
                  << " -> " << routeToString(fallbackRoute)
                  << " (success: " << (success ? "yes" : "no") << ")\n";
    }

    // ─── Confidence-Based Fallback Chain ───────────────────────────────────────
    // Returns an ordered list of routes to try (cheapest first)
    std::vector<MKRouteType> getFallbackChain(MKRouteType primaryRoute, float confidence) {
        std::vector<MKRouteType> chain;
        chain.push_back(primaryRoute);

        // If confidence is low, add more fallbacks
        if (confidence < 0.7f) {
            chain.push_back(getFallbackFor(primaryRoute));
        }
        if (confidence < 0.4f) {
            // Very low confidence: try multiple routes
            if (primaryRoute != MKRouteType::GRAPH) chain.push_back(MKRouteType::GRAPH);
            if (primaryRoute != MKRouteType::SEARCH) chain.push_back(MKRouteType::SEARCH);
        }

        // Remove duplicates
        std::vector<MKRouteType> unique;
        for (const auto& r : chain) {
            bool found = false;
            for (const auto& u : unique) {
                if (u == r) { found = true; break; }
            }
            if (!found) unique.push_back(r);
        }

        return unique;
    }

    // ─── Route Statistics ──────────────────────────────────────────────────────

    MKRouteStats getRouteStats(MKRouteType route) {
        MKRouteStats stats;
        stats.totalRouted = 0;
        stats.successful = 0;
        stats.fallbackUsed = 0;
        stats.avgConfidence = 0.0f;

        switch (route) {
            case MKRouteType::INSTANT:
                stats.totalRouted = instantTotal; stats.successful = instantSuccess; break;
            case MKRouteType::GRAPH:
                stats.totalRouted = graphTotal; stats.successful = graphSuccess; break;
            case MKRouteType::SEARCH:
                stats.totalRouted = searchTotal; stats.successful = searchSuccess; break;
            case MKRouteType::REASON:
                stats.totalRouted = reasonTotal; stats.successful = reasonSuccess; break;
            case MKRouteType::GENERATE:
                stats.totalRouted = generateTotal; stats.successful = generateSuccess; break;
            case MKRouteType::CRYPTO:
                stats.totalRouted = cryptoTotal; stats.successful = cryptoSuccess; break;
            case MKRouteType::HOMELAB:
                stats.totalRouted = homelabTotal; stats.successful = homelabSuccess; break;
            case MKRouteType::MIND:
                stats.totalRouted = mindTotal; stats.successful = mindSuccess; break;
        }

        // Count fallbacks from history
        for (const auto& h : history) {
            if (h.primaryRoute == route && h.usedFallback) {
                stats.fallbackUsed++;
            }
        }

        return stats;
    }

    // Get the overall routing accuracy
    float getOverallAccuracy() {
        int totalAll = instantTotal + graphTotal + searchTotal + reasonTotal
                     + generateTotal + cryptoTotal + homelabTotal + mindTotal;
        int successAll = instantSuccess + graphSuccess + searchSuccess + reasonSuccess
                       + generateSuccess + cryptoSuccess + homelabSuccess + mindSuccess;
        if (totalAll == 0) return 0.0f;
        return (float)successAll / (float)totalAll;
    }

    // Get the total number of routing decisions made
    int getTotalDecisions() const {
        return (int)history.size();
    }

    // Get the most frequently used route
    std::string getMostUsedRoute() const {
        std::string best = "GRAPH";
        int bestCount = 0;
        for (const auto& pair : routeCounters) {
            if (pair.second > bestCount) {
                bestCount = pair.second;
                best = pair.first;
            }
        }
        return best;
    }

    void printStats() const {
        int totalAll = instantTotal + graphTotal + searchTotal + reasonTotal
                     + generateTotal + cryptoTotal + homelabTotal + mindTotal;
        int successAll = instantSuccess + graphSuccess + searchSuccess + reasonSuccess
                       + generateSuccess + cryptoSuccess + homelabSuccess + mindSuccess;
        float accuracy = (totalAll > 0) ? (float)successAll / (float)totalAll * 100.0f : 0.0f;

        std::cout << "[ROUTER STATS] Total decisions: " << history.size()
                  << " | Overall accuracy: " << (int)accuracy << "%\n";
        std::cout << "  INSTANT:  " << instantTotal << " routed, " << instantSuccess << " successful\n";
        std::cout << "  GRAPH:    " << graphTotal << " routed, " << graphSuccess << " successful\n";
        std::cout << "  SEARCH:   " << searchTotal << " routed, " << searchSuccess << " successful\n";
        std::cout << "  REASON:   " << reasonTotal << " routed, " << reasonSuccess << " successful\n";
        std::cout << "  GENERATE: " << generateTotal << " routed, " << generateSuccess << " successful\n";
        std::cout << "  CRYPTO:   " << cryptoTotal << " routed, " << cryptoSuccess << " successful\n";
        std::cout << "  HOMELAB:  " << homelabTotal << " routed, " << homelabSuccess << " successful\n";
        std::cout << "  MIND:     " << mindTotal << " routed, " << mindSuccess << " successful\n";
        std::cout << "  Most used route: " << getMostUsedRoute() << "\n";
    }
};

#endif // MK_SMART_ROUTER_CPP
