// ============================================================
// MK OS - Unified Entry Point
// The single boot sequence that brings the entire system online.
// Replaces all scattered main() functions with one clean startup.
// ============================================================
#ifndef MK_ENTRY_CPP
#define MK_ENTRY_CPP

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <fstream>
#include <vector>
#include <functional>
#include <memory>
#include <sstream>
#include <algorithm>

// ============================================================
// Include real module implementations
// ============================================================
#include "diagnostics.cpp"
#include "../ai_core/hre/pattern_graph.cpp"
#include "../ai_core/hre/deep_reasoner.cpp"
#include "../ai_core/hre/reasoning_chains.cpp"
#include "../ai_core/hre/composer.cpp"
#include "../ai_core/smart_router.cpp"
#include "../network/realtime_apis.cpp"
#include "../ai_core/persistent_memory.cpp"
#include "../ai_core/knowledge_integrator.cpp"
#include "../ai_core/self_improver.cpp"
#include "../mk_brain/personality/response_style.cpp"

// ============================================================
// Global state
// ============================================================
static std::atomic<bool> g_running{true};

// Signal handler for graceful shutdown
static void mk_signal_handler(int sig) {
    (void)sig;
    g_running = false;
}

// ============================================================
// MK Banner
// ============================================================
static void print_banner() {
    std::cout << R"(
    =====================================================
    |   MK OS - Hybrid Reasoning Engine v2.0            |
    |   Custom AI Operating System                      |
    |   Built for low-end hardware. Thinks like a human.|
    =====================================================
)" << std::endl;
}

// ============================================================
// MKSystem - Orchestrates all real modules
// ============================================================
struct MKSystem {
    MKPatternGraph graph;
    MKSmartRouter router;
    MKRealtimeAPIs realtimeApis;
    MKPersistentMemory memory;
    MKKnowledgeIntegrator integrator;
    MKSelfImprover improver;
    MKDiagnostics diagnostics;
    MKResponseStyle style;
    MKDeepReasoner reasoner;
    MKReasoningChains chains;
    MKComposer composer;

    MKSystem()
        : graph("ai_core/hre/knowledge_files"),
          router(),
          realtimeApis(),
          memory("mk_memory.dat", 10000, 5000),
          integrator(),
          improver(0.6f, 10000, "mk_improvement.log"),
          diagnostics(),
          style(),
          reasoner(10),
          chains("ai_core/hre/knowledge_files"),
          composer(MKComposerMode::FRIENDLY) {}
};

// ============================================================
// Helper: trim whitespace
// ============================================================
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// ============================================================
// Command Handlers
// ============================================================

static void cmd_help() {
    std::cout << "\n  MK OS Commands:\n"
              << "    /help             - Show this help\n"
              << "    /ask <query>      - Knowledge graph lookup\n"
              << "    /search <query>   - Internet search (cited answer)\n"
              << "    /status           - System diagnostics and module stats\n"
              << "    /learn <s|r|t>    - Add fact (source|relation|target)\n"
              << "    /weather <city>   - Get weather for a city\n"
              << "    /time [timezone]  - Get current time\n"
              << "    /news             - Latest tech news headlines\n"
              << "    /think <topic>    - Deep multi-hop reasoning\n"
              << "    /quit             - Save and exit\n"
              << "\n  Or just type naturally and MK will route your query.\n\n";
}

static void cmd_ask(MKSystem& sys, const std::string& query) {
    if (query.empty()) {
        std::cout << "\n  Usage: /ask <query>\n";
        return;
    }
    // Try to find info in the knowledge graph
    auto results = sys.graph.getAll(query);
    if (results.empty()) {
        // Try a path query
        auto pathResult = sys.graph.pathQuery(query, "is_a", 5);
        if (pathResult.found) {
            std::string raw = query + " is " + pathResult.answer;
            std::string formatted = sys.style.format_response(raw, query, pathResult.confidence);
            std::cout << "\n  " << formatted << "\n";
        } else {
            std::cout << "\n  No knowledge found for: " << query
                      << "\n  Try teaching me with /learn or use /search for internet.\n";
        }
    } else {
        std::string raw;
        for (const auto& edge : results) {
            raw += edge.source + " " + edge.relation + " " + edge.target + ". ";
        }
        std::string formatted = sys.style.format_response(raw, query, 0.8f);
        std::cout << "\n  " << formatted << "\n";
    }
}

static void cmd_search(MKSystem& sys, const std::string& query) {
    if (query.empty()) {
        std::cout << "\n  Usage: /search <query>\n";
        return;
    }
    auto answer = sys.integrator.buildCitedAnswer(query);
    if (!answer.formattedResponse.empty()) {
        std::cout << "\n  " << answer.formattedResponse << "\n";
    } else if (!answer.text.empty()) {
        std::cout << "\n  " << answer.text << "\n";
    } else {
        std::cout << "\n  No results found for: " << query << "\n";
    }
}

static void cmd_status(MKSystem& sys) {
    std::cout << "\n";
    std::string report = sys.diagnostics.run_full_diagnostics();
    std::cout << "  " << report << "\n";
    sys.router.printStats();
    sys.integrator.printStats();
    sys.memory.printStats();
    sys.improver.printStats();
    sys.graph.printStats();
    std::cout << "\n";
}

static void cmd_learn(MKSystem& sys, const std::string& fact) {
    if (fact.empty()) {
        std::cout << "\n  Usage: /learn source|relation|target\n";
        return;
    }
    // Parse: source|relation|target
    std::stringstream ss(fact);
    std::string source, relation, target;
    if (std::getline(ss, source, '|') && std::getline(ss, relation, '|') && std::getline(ss, target, '|')) {
        source = trim(source);
        relation = trim(relation);
        target = trim(target);
        if (source.empty() || relation.empty() || target.empty()) {
            std::cout << "\n  Invalid fact format. Use: source|relation|target\n";
            return;
        }
        sys.graph.persistNewFact(source, relation, target, 1.0f);
        std::cout << "\n  Learned: " << source << " " << relation << " " << target << "\n";
    } else {
        // Try getting just the remaining part as target
        source = trim(source);
        relation = trim(relation);
        if (!source.empty() && !relation.empty()) {
            // Two fields parsed before getline failed, use remaining
            std::cout << "\n  Invalid fact format. Use: source|relation|target\n";
        } else {
            std::cout << "\n  Invalid fact format. Use: source|relation|target\n";
        }
    }
}

static void cmd_weather(MKSystem& sys, const std::string& city) {
    if (city.empty()) {
        std::cout << "\n  Usage: /weather <city>\n";
        return;
    }
    auto data = sys.realtimeApis.getWeather(city);
    if (data.valid) {
        std::cout << "\n  Weather for " << data.city << ":\n"
                  << "    Temperature: " << data.temperature << " C\n"
                  << "    Conditions: " << data.description << "\n"
                  << "    Wind: " << data.windSpeed << " km/h\n"
                  << "    Humidity: " << data.humidity << "%\n\n";
    } else {
        std::cout << "\n  Could not fetch weather for: " << city << "\n";
    }
}

static void cmd_time(MKSystem& sys, const std::string& timezone) {
    std::string tz = timezone.empty() ? "America/New_York" : timezone;
    auto data = sys.realtimeApis.getCurrentTime(tz);
    if (data.valid) {
        std::cout << "\n  Time (" << data.timezone << "): " << data.datetime << "\n";
    } else {
        std::cout << "\n  Could not fetch time for timezone: " << tz << "\n";
    }
}

static void cmd_news(MKSystem& sys) {
    auto data = sys.realtimeApis.getTechNews(5);
    if (data.valid && !data.headlines.empty()) {
        std::cout << "\n  Latest Tech News:\n";
        int i = 1;
        for (const auto& item : data.headlines) {
            std::cout << "    " << i++ << ". " << item.title;
            if (item.score > 0) std::cout << " (score: " << item.score << ")";
            std::cout << "\n";
        }
        std::cout << "\n";
    } else {
        std::cout << "\n  Could not fetch news headlines.\n";
    }
}

static void cmd_think(MKSystem& sys, const std::string& topic) {
    if (topic.empty()) {
        std::cout << "\n  Usage: /think <topic>\n";
        return;
    }
    auto chain = sys.reasoner.think(topic, sys.graph);
    if (!chain.finalAnswer.empty()) {
        std::string formatted = sys.style.format_response(chain.finalAnswer, topic, chain.overallConfidence);
        std::cout << "\n  Deep Reasoning Result (" << chain.steps.size() << " steps, "
                  << chain.totalHops << " hops):\n"
                  << "  " << formatted << "\n";
    } else {
        std::cout << "\n  Could not find a reasoning path for: " << topic
                  << "\n  Try teaching me related facts with /learn.\n";
    }
}

// ============================================================
// Natural Language Routing
// ============================================================
static void handle_natural_query(MKSystem& sys, const std::string& input) {
    // Route through MKSmartRouter
    auto decision = sys.router.route(input);
    std::string response;
    float confidence = 0.0f;
    bool answered = false;

    switch (decision.primaryRoute) {
        case MKRouteType::INSTANT: {
            // Detect what kind of instant data is needed
            std::string lower = input;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            if (lower.find("weather") != std::string::npos) {
                // Try to extract city from query
                std::string city = "New York"; // default
                // Simple extraction: last word(s) after "weather" or "in"
                size_t inPos = lower.find(" in ");
                if (inPos != std::string::npos) {
                    city = input.substr(inPos + 4);
                }
                auto data = sys.realtimeApis.getWeather(trim(city));
                if (data.valid) {
                    response = "Weather in " + data.city + ": " +
                               std::to_string((int)data.temperature) + " C, " + data.description;
                    confidence = 0.9f;
                    answered = true;
                }
            } else if (lower.find("time") != std::string::npos || lower.find("clock") != std::string::npos) {
                auto data = sys.realtimeApis.getCurrentTime("America/New_York");
                if (data.valid) {
                    response = "Current time (" + data.timezone + "): " + data.datetime;
                    confidence = 0.95f;
                    answered = true;
                }
            } else if (lower.find("news") != std::string::npos) {
                auto data = sys.realtimeApis.getTechNews(3);
                if (data.valid && !data.headlines.empty()) {
                    response = "Top headlines: ";
                    for (size_t i = 0; i < data.headlines.size(); i++) {
                        response += std::to_string(i + 1) + ") " + data.headlines[i].title;
                        if (i < data.headlines.size() - 1) response += " | ";
                    }
                    confidence = 0.9f;
                    answered = true;
                }
            }

            if (!answered) {
                // Fallback to graph if instant route cannot handle it
                auto results = sys.graph.getAll(input);
                if (!results.empty()) {
                    for (const auto& e : results) {
                        response += e.source + " " + e.relation + " " + e.target + ". ";
                    }
                    confidence = 0.7f;
                    answered = true;
                }
            }
            break;
        }
        case MKRouteType::GRAPH: {
            // Knowledge graph lookup
            auto results = sys.graph.getAll(input);
            if (!results.empty()) {
                for (const auto& e : results) {
                    response += e.source + " " + e.relation + " " + e.target + ". ";
                }
                confidence = 0.8f;
                answered = true;
            } else {
                // Try path query
                auto pathResult = sys.graph.pathQuery(input, "is_a", 5);
                if (pathResult.found) {
                    response = input + " is " + pathResult.answer;
                    confidence = pathResult.confidence;
                    answered = true;
                }
            }
            break;
        }
        case MKRouteType::SEARCH: {
            auto answer = sys.integrator.buildCitedAnswer(input);
            if (!answer.formattedResponse.empty()) {
                response = answer.formattedResponse;
                confidence = answer.confidence;
                answered = true;
            } else if (!answer.text.empty()) {
                response = answer.text;
                confidence = answer.confidence;
                answered = true;
            }
            break;
        }
        case MKRouteType::REASON: {
            auto chain = sys.reasoner.think(input, sys.graph);
            if (!chain.finalAnswer.empty()) {
                response = chain.finalAnswer;
                confidence = chain.overallConfidence;
                answered = true;
            }
            break;
        }
        case MKRouteType::GENERATE: {
            // Use composer for creative/generative tasks, fallback to graph
            MKResponseContext ctx;
            ctx.subject = input;
            ctx.is_partial = false;
            ctx.confidence = 0.5f;

            auto results = sys.graph.getAll(input);
            if (!results.empty()) {
                for (const auto& e : results) {
                    ctx.facts_found.push_back(e.source + " " + e.relation + " " + e.target);
                }
                response = sys.composer.composeAnswer(ctx);
                confidence = 0.6f;
                answered = true;
            } else {
                response = sys.composer.composeAnswer(ctx);
                confidence = 0.3f;
                answered = !response.empty();
            }
            break;
        }
    }

    // If no answer found through primary route, try fallback
    if (!answered && decision.confidence < 0.7f) {
        auto fallbackChain = sys.router.getFallbackChain(decision.primaryRoute, decision.confidence);
        for (size_t i = 1; i < fallbackChain.size() && !answered; i++) {
            if (fallbackChain[i] == MKRouteType::GRAPH) {
                auto results = sys.graph.getAll(input);
                if (!results.empty()) {
                    for (const auto& e : results) {
                        response += e.source + " " + e.relation + " " + e.target + ". ";
                    }
                    confidence = 0.6f;
                    answered = true;
                }
            } else if (fallbackChain[i] == MKRouteType::SEARCH) {
                auto answer = sys.integrator.buildCitedAnswer(input);
                if (!answer.text.empty()) {
                    response = answer.formattedResponse.empty() ? answer.text : answer.formattedResponse;
                    confidence = answer.confidence;
                    answered = true;
                }
            }
        }
    }

    // Format response through style engine
    if (answered && !response.empty()) {
        std::string formatted = sys.style.format_response(response, input, confidence);
        std::cout << "\n  " << formatted << "\n";
    } else {
        std::cout << "\n  I don't have enough knowledge to answer that yet.\n"
                  << "  Try: /learn to teach me, /search for internet lookup, or /think for reasoning.\n";
    }

    // Record outcome
    sys.router.reportOutcome(decision.primaryRoute, answered);
    sys.improver.logQueryOutcome(input, confidence, answered);
    sys.memory.recordInteraction("query", input);
    if (answered) {
        sys.memory.recordQA(input, response, confidence);
    }
}

// ============================================================
// MAIN - Boot Sequence
// ============================================================
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    // Register signal handlers
    std::signal(SIGINT, mk_signal_handler);
    std::signal(SIGTERM, mk_signal_handler);

    // Step 1: Banner
    print_banner();

    // Step 2: Detect platform
    std::string platform;
    #ifdef __APPLE__
    platform = "macOS (Darwin)";
    #elif __linux__
    platform = "Linux";
    #else
    platform = "Unknown";
    #endif
    std::cout << "  Platform: " << platform << "\n";

    // Step 3: Initialize all modules
    std::cout << "  Initializing modules...\n\n";
    MKSystem sys;

    // Step 4: Load knowledge
    sys.graph.loadAllKnowledge();

    // Step 5: Load persistent memory
    sys.memory.loadFromDisk();

    // Step 6: Print status
    std::cout << "\n  System ready. Knowledge: " << sys.graph.edgeCount() << " facts | Nodes: "
              << sys.graph.nodeCount() << "\n";
    std::cout << "  Type your message or /help for commands.\n";

    // ============================================================
    // Main REPL Loop
    // ============================================================
    while (g_running) {
        std::cout << "\n  MK > ";
        std::cout.flush();

        std::string input;
        if (!std::getline(std::cin, input)) {
            // EOF
            break;
        }

        input = trim(input);
        if (input.empty()) continue;

        // Handle commands
        if (!input.empty() && input[0] == '/') {
            if (input == "/quit" || input == "/exit" || input == "/shutdown") {
                g_running = false;
            } else if (input == "/help") {
                cmd_help();
            } else if (input == "/status") {
                cmd_status(sys);
            } else if (input == "/news") {
                cmd_news(sys);
            } else if (input.size() > 5 && input.substr(0, 5) == "/ask ") {
                cmd_ask(sys, trim(input.substr(5)));
            } else if (input.size() > 8 && input.substr(0, 8) == "/search ") {
                cmd_search(sys, trim(input.substr(8)));
            } else if (input.size() > 7 && input.substr(0, 7) == "/learn ") {
                cmd_learn(sys, trim(input.substr(7)));
            } else if (input.size() > 9 && input.substr(0, 9) == "/weather ") {
                cmd_weather(sys, trim(input.substr(9)));
            } else if (input.size() > 6 && input.substr(0, 6) == "/time ") {
                cmd_time(sys, trim(input.substr(6)));
            } else if (input == "/time") {
                cmd_time(sys, "");
            } else if (input.size() > 7 && input.substr(0, 7) == "/think ") {
                cmd_think(sys, trim(input.substr(7)));
            } else {
                std::cout << "\n  Unknown command. Type /help for options.\n";
            }
            continue;
        }

        // Natural language routing
        handle_natural_query(sys, input);
    }

    // ============================================================
    // Graceful Shutdown
    // ============================================================
    std::cout << "\n  Shutting down...\n";
    sys.memory.saveToDisk();
    sys.improver.saveLog();
    std::cout << "  Memory saved. Improvement log saved.\n";
    std::cout << "  MK OS shut down cleanly. Goodbye.\n\n";

    return 0;
}

#endif // MK_ENTRY_CPP
