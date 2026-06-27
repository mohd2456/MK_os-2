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
// ANSI Color & Style Codes
// ============================================================
namespace Color {
    // Reset
    const char* RESET   = "\033[0m";
    // Styles
    const char* BOLD    = "\033[1m";
    const char* DIM     = "\033[2m";
    const char* ITALIC  = "\033[3m";
    const char* UNDER   = "\033[4m";
    // Foreground colors
    const char* BLACK   = "\033[30m";
    const char* RED     = "\033[31m";
    const char* GREEN   = "\033[32m";
    const char* YELLOW  = "\033[33m";
    const char* BLUE    = "\033[34m";
    const char* MAGENTA = "\033[35m";
    const char* CYAN    = "\033[36m";
    const char* WHITE   = "\033[37m";
    // Bright variants
    const char* BRED    = "\033[91m";
    const char* BGREEN  = "\033[92m";
    const char* BYELLOW = "\033[93m";
    const char* BBLUE   = "\033[94m";
    const char* BMAGENTA= "\033[95m";
    const char* BCYAN   = "\033[96m";
    const char* BWHITE  = "\033[97m";
    // Background
    const char* BG_BLACK = "\033[40m";
    const char* BG_BLUE  = "\033[44m";
    const char* BG_CYAN  = "\033[46m";
}

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
#include "../mk_brain/learning/learning_engine.cpp"
#include "../mk_brain/memory/brain_memory.cpp"
#include "../mk_brain/embeddings/embeddings_eng.cpp"
#include "../mk_brain/vector_search/ann_search.cpp"
#include "../mk_brain/cashe/cache_mgr.cpp"
#include "../mk_brain/daily_briefing.cpp"
#include "../mk_brain/fact_extractor/biographical.cpp"
#include "../mk_brain/reasoning/reasoning_engine.cpp"

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
    std::cout << Color::BCYAN << Color::BOLD << R"(
    ╔══════════════════════════════════════════════════════════╗
    ║)" << Color::BWHITE << "   ███╗   ███╗██╗  ██╗     ██████╗ ███████╗             " << Color::BCYAN << R"(║
    ║)" << Color::BWHITE << "   ████╗ ████║██║ ██╔╝    ██╔═══██╗██╔════╝             " << Color::BCYAN << R"(║
    ║)" << Color::BWHITE << "   ██╔████╔██║█████╔╝     ██║   ██║███████╗             " << Color::BCYAN << R"(║
    ║)" << Color::BWHITE << "   ██║╚██╔╝██║██╔═██╗     ██║   ██║╚════██║             " << Color::BCYAN << R"(║
    ║)" << Color::BWHITE << "   ██║ ╚═╝ ██║██║  ██╗    ╚██████╔╝███████║             " << Color::BCYAN << R"(║
    ║)" << Color::BWHITE << "   ╚═╝     ╚═╝╚═╝  ╚═╝     ╚═════╝ ╚══════╝             " << Color::BCYAN << R"(║
    ║                                                          ║
    ║)" << Color::GREEN << "   Hybrid Reasoning Engine v2.0                          " << Color::BCYAN << R"( ║
    ║)" << Color::DIM << Color::WHITE << "   Accuracy-First AI • Runs Locally • Never Hallucin" << Color::BCYAN << R"(ates  ║
    ╚══════════════════════════════════════════════════════════╝
)" << Color::RESET << std::endl;
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
    MKLearningEngine learningEngine;
    MKBrainMemory brainMemory;
    MKEmbeddingsEngine embeddings;
    MKANNSearch vectorSearch;
    MKCacheManager cacheManager;
    MKDailyBriefing dailyBriefing;
    MKBiographicalExtractor factExtractor;
    MKReasoningEngine reasoningEngine;

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
          composer(MKComposerMode::FRIENDLY),
          learningEngine(),
          brainMemory(20),
          embeddings(),
          vectorSearch(128),
          cacheManager(8),
          dailyBriefing(),
          factExtractor(),
          reasoningEngine() {}
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
    std::cout << "\n"
        << Color::BOLD << Color::BCYAN << "  ╭─────────────────────────────────────────────╮\n"
        << "  │           MK OS — COMMAND REFERENCE          │\n"
        << "  ╰─────────────────────────────────────────────╯\n" << Color::RESET << "\n"
        << Color::BOLD << Color::YELLOW << "  ⚡ KNOWLEDGE" << Color::RESET << "\n"
        << "    " << Color::GREEN << "/ask" << Color::RESET << " <query>       Look up facts in knowledge graph\n"
        << "    " << Color::GREEN << "/think" << Color::RESET << " <topic>    Deep multi-hop reasoning\n"
        << "    " << Color::GREEN << "/learn" << Color::RESET << " <s|r|t>    Teach MK a new fact (source|relation|target)\n"
        << "\n"
        << Color::BOLD << Color::YELLOW << "  🌐 INTERNET" << Color::RESET << "\n"
        << "    " << Color::GREEN << "/search" << Color::RESET << " <query>   Internet search (verified + cited)\n"
        << "    " << Color::GREEN << "/weather" << Color::RESET << " <city>   Live weather for any city\n"
        << "    " << Color::GREEN << "/time" << Color::RESET << " [timezone]   Current time in any timezone\n"
        << "    " << Color::GREEN << "/news" << Color::RESET << "              Latest tech headlines\n"
        << "\n"
        << Color::BOLD << Color::YELLOW << "  🖥️  SYSTEM" << Color::RESET << "\n"
        << "    " << Color::GREEN << "/status" << Color::RESET << "            Full system diagnostics\n"
        << "    " << Color::GREEN << "/briefing" << Color::RESET << "          Daily system briefing report\n"
        << "    " << Color::GREEN << "/sync" << Color::RESET << "              Sync knowledge with GitHub\n"
        << "    " << Color::GREEN << "/quit" << Color::RESET << "              Save and exit\n"
        << "\n"
        << Color::DIM << "  Or just type naturally — MK will figure out what you mean.\n"
        << "  Example: \"what is python?\" or \"weather in london\"\n" << Color::RESET << "\n";
}

// Show command suggestions when user types just "/"
static void show_slash_suggestions(const std::string& partial) {
    struct CmdInfo {
        const char* cmd;
        const char* desc;
    };
    static const CmdInfo all_commands[] = {
        {"/ask",     "Knowledge graph lookup"},
        {"/search",  "Internet search (cited)"},
        {"/think",   "Deep reasoning"},
        {"/learn",   "Teach MK a fact"},
        {"/weather", "Live weather"},
        {"/time",    "Current time"},
        {"/news",    "Tech headlines"},
        {"/status",  "System diagnostics"},
        {"/briefing","Daily briefing report"},
        {"/sync",    "Sync knowledge with GitHub"},
        {"/help",    "Show all commands"},
        {"/quit",    "Save and exit"},
    };

    std::vector<const CmdInfo*> matches;
    for (const auto& ci : all_commands) {
        std::string cmd_str(ci.cmd);
        if (cmd_str.find(partial) == 0) {
            matches.push_back(&ci);
        }
    }

    if (matches.empty()) return;

    std::cout << Color::DIM << "\n  Available commands:\n";
    for (const auto* m : matches) {
        std::cout << "    " << Color::CYAN << m->cmd << Color::RESET 
                  << Color::DIM << "  — " << m->desc << Color::RESET << "\n";
    }
    std::cout << Color::RESET;
}

static void cmd_ask(MKSystem& sys, const std::string& query) {
    if (query.empty()) {
        std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET << " /ask <query>\n";
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
            std::cout << "\n  " << Color::GREEN << "●" << Color::RESET << " " << formatted << "\n";
        } else {
            std::cout << "\n  " << Color::YELLOW << "○" << Color::RESET 
                      << " No knowledge found for: " << Color::BOLD << query << Color::RESET
                      << "\n  " << Color::DIM << "Try /learn to teach me or /search for internet." 
                      << Color::RESET << "\n";
        }
    } else {
        std::string raw;
        for (const auto& edge : results) {
            raw += edge.source + " " + edge.relation + " " + edge.target + ". ";
        }
        std::string formatted = sys.style.format_response(raw, query, 0.8f);
        std::cout << "\n  " << Color::GREEN << "●" << Color::RESET << " " << formatted << "\n";
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
        std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET 
                  << " /learn source|relation|target\n"
                  << "  " << Color::DIM << "Example: /learn earth|has|moon" << Color::RESET << "\n";
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
        sys.learningEngine.learnFact(source, relation, target);
        std::cout << "\n  " << Color::BGREEN << "✓" << Color::RESET << " Learned: " 
                  << Color::BOLD << source << Color::RESET << " " 
                  << Color::CYAN << relation << Color::RESET << " " 
                  << Color::BOLD << target << Color::RESET << "\n";
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
        std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET << " /weather <city>\n";
        return;
    }
    auto data = sys.realtimeApis.getWeather(city);
    if (data.valid) {
        std::cout << "\n  " << Color::BOLD << Color::BCYAN << "☁ Weather" << Color::RESET 
                  << " for " << Color::BOLD << data.city << Color::RESET << ":\n"
                  << "    " << Color::BYELLOW << "🌡" << Color::RESET << " Temperature: " 
                  << Color::BOLD << data.temperature << " °C" << Color::RESET << "\n"
                  << "    " << Color::WHITE << "☀" << Color::RESET << " Conditions:  " << data.description << "\n"
                  << "    " << Color::BBLUE << "💨" << Color::RESET << " Wind:        " << data.windSpeed << " km/h\n"
                  << "    " << Color::BLUE << "💧" << Color::RESET << " Humidity:    " << data.humidity << "%\n\n";
    } else {
        std::cout << "\n  " << Color::RED << "✗" << Color::RESET 
                  << " Could not fetch weather for: " << city << "\n";
    }
}

// Helper: extract timezone from natural language text
static std::string extract_timezone(const std::string& text) {
    // Map of common city/region names to IANA timezone identifiers
    struct TZMapping {
        const char* keyword;
        const char* tz;
    };
    static const TZMapping mappings[] = {
        {"tokyo", "Asia/Tokyo"},
        {"japan", "Asia/Tokyo"},
        {"london", "Europe/London"},
        {"uk", "Europe/London"},
        {"paris", "Europe/Paris"},
        {"france", "Europe/Paris"},
        {"berlin", "Europe/Berlin"},
        {"germany", "Europe/Berlin"},
        {"sydney", "Australia/Sydney"},
        {"australia", "Australia/Sydney"},
        {"los angeles", "America/Los_Angeles"},
        {"la", "America/Los_Angeles"},
        {"california", "America/Los_Angeles"},
        {"pacific", "America/Los_Angeles"},
        {"chicago", "America/Chicago"},
        {"central", "America/Chicago"},
        {"denver", "America/Denver"},
        {"mountain", "America/Denver"},
        {"new york", "America/New_York"},
        {"eastern", "America/New_York"},
        {"dubai", "Asia/Dubai"},
        {"mumbai", "Asia/Kolkata"},
        {"india", "Asia/Kolkata"},
        {"kolkata", "Asia/Kolkata"},
        {"beijing", "Asia/Shanghai"},
        {"shanghai", "Asia/Shanghai"},
        {"china", "Asia/Shanghai"},
        {"seoul", "Asia/Seoul"},
        {"korea", "Asia/Seoul"},
        {"moscow", "Europe/Moscow"},
        {"russia", "Europe/Moscow"},
        {"cairo", "Africa/Cairo"},
        {"singapore", "Asia/Singapore"},
        {"hong kong", "Asia/Hong_Kong"},
        {"toronto", "America/Toronto"},
        {"utc", "UTC"},
        {"gmt", "UTC"},
    };

    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    for (const auto& m : mappings) {
        if (lower.find(m.keyword) != std::string::npos) {
            return m.tz;
        }
    }

    // If the text contains a slash (like "America/Chicago"), treat it as a raw timezone
    if (text.find('/') != std::string::npos) {
        return trim(text);
    }

    return "";
}

static void cmd_time(MKSystem& sys, const std::string& timezone) {
    std::string tz = timezone.empty() ? "America/New_York" : timezone;
    // Try to resolve natural language timezone if it doesn't look like an IANA identifier
    if (!timezone.empty() && timezone.find('/') == std::string::npos) {
        std::string resolved = extract_timezone(timezone);
        if (!resolved.empty()) {
            tz = resolved;
        }
    }
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
        std::cout << "\n  " << Color::BOLD << Color::BMAGENTA << "📰 Tech News" << Color::RESET << "\n";
        int i = 1;
        for (const auto& item : data.headlines) {
            std::cout << "    " << Color::CYAN << i++ << "." << Color::RESET << " " << item.title;
            if (item.score > 0) std::cout << Color::DIM << " (" << item.score << " pts)" << Color::RESET;
            std::cout << "\n";
        }
        std::cout << "\n";
    } else {
        std::cout << "\n  " << Color::RED << "✗" << Color::RESET 
                  << " Could not fetch news headlines.\n";
    }
}

static void cmd_think(MKSystem& sys, const std::string& topic) {
    if (topic.empty()) {
        std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET << " /think <topic>\n";
        return;
    }
    std::cout << "  " << Color::DIM << "Thinking..." << Color::RESET << "\n";
    auto chain = sys.reasoner.think(topic, sys.graph);
    if (!chain.finalAnswer.empty()) {
        std::string formatted = sys.style.format_response(chain.finalAnswer, topic, chain.overallConfidence);
        std::cout << "\n  " << Color::BMAGENTA << "🧠" << Color::RESET << Color::BOLD 
                  << " Deep Reasoning" << Color::RESET << Color::DIM 
                  << " (" << chain.steps.size() << " steps, " << chain.totalHops << " hops)" 
                  << Color::RESET << "\n"
                  << "  " << Color::GREEN << "●" << Color::RESET << " " << formatted << "\n";
    } else {
        std::cout << "\n  " << Color::YELLOW << "○" << Color::RESET 
                  << " No reasoning path found for: " << Color::BOLD << topic << Color::RESET
                  << "\n  " << Color::DIM << "Try teaching related facts with /learn." 
                  << Color::RESET << "\n";
    }
}

static void cmd_briefing(MKSystem& sys) {
    MKSystemSnapshot snapshot;
    snapshot.cpuTempC = 42.0f;
    snapshot.batteryPercent = 100.0f;
    snapshot.onAC = true;
    snapshot.diskUsedPercent = 35.0f;
    snapshot.freeStorageMB = 50000;
    snapshot.activeProcesses = 12;
    snapshot.uptimeHours = 1;

    MKBuildProgress builds;
    builds.totalBuilds = 1;
    builds.successfulBuilds = 1;
    builds.lastBuildFile = "mk_os";
    builds.lastBuildStatus = "OK";
    builds.lastBuildTime = std::time(nullptr);

    MKLearningProgress learning;
    learning.factsLearnedToday = sys.learningEngine.factCount();
    learning.totalFacts = (int)sys.graph.edgeCount() + sys.learningEngine.factCount();
    learning.uncertainFacts = 0;
    learning.recentTopics = {"system", "knowledge", "ai"};

    std::string briefing = sys.dailyBriefing.generate("User", snapshot, builds, learning);
    std::cout << "\n" << briefing << "\n";
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
                    // Strip trailing noise words
                    static const std::vector<std::string> noise = {
                        " today", " tonight", " now", " right now",
                        " this week", " this weekend", " tomorrow",
                        " currently", " please", " lately"
                    };
                    std::string cityLower = city;
                    std::transform(cityLower.begin(), cityLower.end(), cityLower.begin(), ::tolower);
                    for (const auto& word : noise) {
                        size_t pos = cityLower.find(word);
                        if (pos != std::string::npos) {
                            city = city.substr(0, pos);
                            cityLower = cityLower.substr(0, pos);
                        }
                    }
                    city = trim(city);
                }
                auto data = sys.realtimeApis.getWeather(trim(city));
                if (data.valid) {
                    response = "Weather in " + data.city + ": " +
                               std::to_string((int)data.temperature) + " C, " + data.description;
                    confidence = 0.9f;
                    answered = true;
                }
            } else if (lower.find("time") != std::string::npos || lower.find("clock") != std::string::npos) {
                // Try to extract timezone from the natural language query
                std::string tz = extract_timezone(input);
                if (tz.empty()) {
                    tz = "America/New_York"; // default fallback
                }
                auto data = sys.realtimeApis.getCurrentTime(tz);
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
                } else {
                    // Fallback to vector search
                    auto vecResults = sys.vectorSearch.search(input, 3);
                    if (!vecResults.empty() && vecResults[0].score > 0.3f) {
                        response = "";
                        for (const auto& vr : vecResults) {
                            if (vr.score > 0.3f) {
                                response += vr.sourceText + " ";
                            }
                        }
                        confidence = vecResults[0].score;
                        answered = !response.empty();
                    }
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
            // Enrich the graph with rule-based inferences before deep reasoning
            sys.chains.deriveAll(sys.graph);
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
        std::cout << "\n  " << Color::GREEN << "●" << Color::RESET << " " << formatted << "\n";
    } else {
        std::cout << "\n  " << Color::YELLOW << "○" << Color::RESET 
                  << " I don't have enough knowledge to answer that yet.\n"
                  << "  " << Color::DIM << "Try: " << Color::GREEN << "/learn" << Color::RESET 
                  << Color::DIM << " to teach me, " << Color::GREEN << "/search" << Color::RESET
                  << Color::DIM << " for internet, or " << Color::GREEN << "/think" << Color::RESET 
                  << Color::DIM << " for reasoning." << Color::RESET << "\n";
    }

    // Record outcome
    sys.router.reportOutcome(decision.primaryRoute, answered);
    sys.improver.logQueryOutcome(input, confidence, answered);
    sys.memory.recordInteraction("query", input);
    if (answered) {
        sys.memory.recordQA(input, response, confidence);
    }

    // Passively extract biographical facts from user input
    sys.factExtractor.extractFromMessage(input);
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
    std::cout << "  " << Color::DIM << "Platform:" << Color::RESET << " " << Color::BOLD << platform << Color::RESET << "\n";

    // Step 3: Initialize all modules
    std::cout << "  " << Color::DIM << "Initializing modules..." << Color::RESET << "\n\n";
    MKSystem sys;

    // Step 4: Load knowledge
    sys.graph.loadAllKnowledge();

    // Step 4b: Restore learning engine knowledge
    sys.learningEngine.restore();

    // Step 4c: Check if daily briefing should be generated
    if (sys.dailyBriefing.shouldGenerate()) {
        std::cout << "\n  " << Color::BMAGENTA << "📋" << Color::RESET 
                  << Color::BOLD << " Daily Briefing Available" << Color::RESET
                  << Color::DIM << " — type /briefing to view" << Color::RESET << "\n";
    }

    // Step 5: Load persistent memory
    sys.memory.loadFromDisk();

    // Step 6: Print status
    std::cout << "\n  " << Color::BGREEN << "✓" << Color::RESET << " System ready. "
              << Color::BOLD << "Knowledge: " << Color::BCYAN << sys.graph.edgeCount() << Color::RESET
              << " facts | " << Color::BOLD << "Nodes: " << Color::BCYAN << sys.graph.nodeCount() 
              << Color::RESET << "\n";
    std::cout << "  " << Color::DIM << "Type your message or " << Color::GREEN << "/help" 
              << Color::RESET << Color::DIM << " for commands." << Color::RESET << "\n";

    // ============================================================
    // Main REPL Loop
    // ============================================================
    unsigned int interaction_count = 0;
    const unsigned int AUTO_SAVE_INTERVAL = 50;

    while (g_running) {
        std::cout << "\n  " << Color::BOLD << Color::BCYAN << "MK" 
                  << Color::RESET << Color::CYAN << " › " << Color::RESET;
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
            // If user typed just "/" alone, show all commands
            if (input == "/") {
                show_slash_suggestions("/");
                continue;
            }
            // If user typed a partial command that doesn't match exactly, suggest
            bool commandFound = false;

            if (input == "/quit" || input == "/exit" || input == "/shutdown") {
                g_running = false; commandFound = true;
            } else if (input == "/help") {
                cmd_help(); commandFound = true;
            } else if (input == "/status") {
                cmd_status(sys); commandFound = true;
            } else if (input == "/sync" || input.substr(0, 6) == "/sync ") {
                // Sync knowledge with GitHub via system command
                std::string syncArg = "pull";
                if (input.size() > 6) syncArg = trim(input.substr(6));
                std::cout << "\n  " << Color::BCYAN << "⟳" << Color::RESET 
                          << " Syncing knowledge with GitHub (" << syncArg << ")...\n";
                std::string cmd = "cd " + std::string("ai_core/hre/knowledge_files") + " && ";
                if (syncArg == "push") {
                    // Use git to push learned facts
                    int result = std::system("git add ai_core/hre/knowledge_files/learned_facts.mk ai_core/hre/knowledge_files/personal_facts.mk 2>/dev/null && git commit -m 'sync: MK auto-push knowledge' 2>/dev/null && git push 2>/dev/null");
                    if (result == 0) {
                        std::cout << "  " << Color::GREEN << "✓" << Color::RESET 
                                  << " Knowledge pushed to GitHub successfully.\n";
                    } else {
                        std::cout << "  " << Color::YELLOW << "⚠" << Color::RESET 
                                  << " Push failed (no changes or no git access). Use mk_sync tool for full sync.\n";
                    }
                } else {
                    // Pull latest knowledge
                    int result = std::system("git pull --rebase 2>/dev/null");
                    if (result == 0) {
                        // Reload knowledge
                        std::cout << "  " << Color::GREEN << "✓" << Color::RESET 
                                  << " Pulled latest from GitHub. Reloading knowledge...\n";
                        sys.graph.loadAllKnowledge();
                        std::cout << "  " << Color::GREEN << "✓" << Color::RESET
                                  << " Knowledge reloaded: " << sys.graph.edgeCount() << " facts.\n";
                    } else {
                        std::cout << "  " << Color::YELLOW << "⚠" << Color::RESET 
                                  << " Pull failed. Use mk_sync tool for full GitHub API sync.\n";
                    }
                }
                commandFound = true;
            } else if (input == "/news") {
                cmd_news(sys); commandFound = true;
            } else if (input.size() > 5 && input.substr(0, 5) == "/ask ") {
                cmd_ask(sys, trim(input.substr(5))); commandFound = true;
            } else if (input == "/ask") {
                cmd_ask(sys, ""); commandFound = true;
            } else if (input.size() > 8 && input.substr(0, 8) == "/search ") {
                cmd_search(sys, trim(input.substr(8))); commandFound = true;
            } else if (input == "/search") {
                cmd_search(sys, ""); commandFound = true;
            } else if (input.size() > 7 && input.substr(0, 7) == "/learn ") {
                cmd_learn(sys, trim(input.substr(7))); commandFound = true;
            } else if (input == "/learn") {
                cmd_learn(sys, ""); commandFound = true;
            } else if (input.size() > 9 && input.substr(0, 9) == "/weather ") {
                cmd_weather(sys, trim(input.substr(9))); commandFound = true;
            } else if (input == "/weather") {
                cmd_weather(sys, ""); commandFound = true;
            } else if (input.size() > 6 && input.substr(0, 6) == "/time ") {
                cmd_time(sys, trim(input.substr(6))); commandFound = true;
            } else if (input == "/time") {
                cmd_time(sys, ""); commandFound = true;
            } else if (input.size() > 7 && input.substr(0, 7) == "/think ") {
                cmd_think(sys, trim(input.substr(7))); commandFound = true;
            } else if (input == "/think") {
                cmd_think(sys, ""); commandFound = true;
            } else if (input == "/briefing") {
                cmd_briefing(sys); commandFound = true;
            }

            if (!commandFound) {
                // Show suggestions for the partial command
                show_slash_suggestions(input);
                std::cout << "\n  " << Color::YELLOW << "⚠" << Color::RESET 
                          << " Unknown command: " << Color::RED << input << Color::RESET
                          << ". Type " << Color::GREEN << "/help" << Color::RESET << " for options.\n";
            }
        } else {
            // Natural language routing
            handle_natural_query(sys, input);
        }

        // Track dialog context for all interactions
        sys.brainMemory.commitToShortTerm("user", input);

        // Periodic auto-save every N interactions to limit data loss on crash
        interaction_count++;
        if (interaction_count % AUTO_SAVE_INTERVAL == 0) {
            sys.memory.saveToDisk();
            sys.improver.saveLog();
        }
    }

    // ============================================================
    // Graceful Shutdown
    // ============================================================
    std::cout << "\n  " << Color::YELLOW << "⏻" << Color::RESET << " Shutting down...\n";
    sys.memory.saveToDisk();
    sys.improver.saveLog();
    sys.learningEngine.persist();
    std::cout << "  " << Color::GREEN << "✓" << Color::RESET << " Memory saved. Improvement log saved. Knowledge persisted.\n";
    std::cout << "  " << Color::BOLD << Color::CYAN << "MK OS shut down cleanly. Goodbye." 
              << Color::RESET << "\n\n";

    return 0;
}

#endif // MK_ENTRY_CPP
