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
#include <mutex>
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
#include "daemon.cpp"
#include "shell.cpp"
#include "service_manager.cpp"
#include "../plugins/telegram.cpp"
#include "../ai_core/neural_net.cpp"
#include "../mk_brain/personality/casual_responses.cpp"
#include "../ai_core/conversation_mode.cpp"
#include "../ai_core/mce/mce_engine.cpp"
#include "../ai_core/cxn/cxn_engine.cpp"
#include "../ai_core/prometheus/prometheus_engine.cpp"
#include "../ai_core/genesis/genesis_engine.cpp"
#include "../ai_core/idea_engine.cpp"
#include "../plugins/pc_helper.cpp"
#include "../crypto/market_data.cpp"
#include "../crypto/technical_analysis.cpp"
#include "../crypto/signal_engine.cpp"
#include "../crypto/portfolio_manager.cpp"
#include "../crypto/risk_manager.cpp"
#include "../crypto/exchange_api.cpp"
#include "../crypto/trading_bot.cpp"
#include "../crypto/airdrop_farmer.cpp"

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
// REPL UI Animation Helpers
// ============================================================

// Typewriter effect: prints each character with a small delay
static void typewriter(const std::string& text, int delayMs = 8) {
    for (char c : text) {
        std::cout << c;
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
}

// Gradient color array for banner (ANSI 256-color codes)
static const char* gradient_colors[] = {
    "\033[38;5;51m",  // bright cyan
    "\033[38;5;45m",  // medium cyan
    "\033[38;5;39m",  // blue-cyan
    "\033[38;5;33m",  // medium blue
    "\033[38;5;27m",  // deep blue
    "\033[38;5;33m",  // medium blue
    "\033[38;5;39m",  // blue-cyan
    "\033[38;5;45m",  // medium cyan
    "\033[38;5;51m",  // bright cyan
    "\033[38;5;87m",  // light cyan
};
static constexpr int NUM_GRADIENT_COLORS = 10;

// Print a colored separator line
static void print_separator() {
    std::cout << "  " << Color::DIM;
    for (int i = 0; i < 50; i++) {
        std::cout << gradient_colors[i % NUM_GRADIENT_COLORS] << "\xe2\x94\x80";
    }
    std::cout << Color::RESET << "\n";
}

// Thinking spinner - runs briefly to give visual feedback
static void show_thinking_spinner(int durationMs = 300) {
    const char* frames[] = {"\xe2\xa0\x8b", "\xe2\xa0\x99", "\xe2\xa0\xb9", "\xe2\xa0\xb8",
                            "\xe2\xa0\xbc", "\xe2\xa0\xb4", "\xe2\xa0\xa6", "\xe2\xa0\xa7",
                            "\xe2\xa0\x87", "\xe2\xa0\x8f"};
    int numFrames = 10;
    int elapsed = 0;
    int frameDelay = 60;
    int i = 0;
    std::cout << "  " << Color::CYAN;
    while (elapsed < durationMs) {
        std::cout << "\r  " << Color::CYAN << frames[i % numFrames] << " thinking..." << Color::RESET << "   ";
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay));
        elapsed += frameDelay;
        i++;
    }
    std::cout << "\r                         \r";
    std::cout.flush();
}

// ============================================================
// MK Banner
// ============================================================
static void print_banner() {
    // Banner lines for typewriter-style gradient printing
    static const char* bannerLines[] = {
        "    \xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97",
    };
    (void)bannerLines; // used for reference, actual print below

    std::cout << "\n";
    // Print banner with gradient colors per line
    std::string lines[] = {
        "    +----------------------------------------------------------+",
        "    |   ###   ###  # #  #  #     ###   ###                    |",
        "    |   ## ## ##   # # #        #   #  #                      |",
        "    |   #  #  #    ###          #   #  ###                    |",
        "    |   #     #    # # #        #   #    #                    |",
        "    |   #     #    #  # #        ###   ###                    |",
        "    |                                                          |",
        "    |   Hybrid Reasoning Engine v2.0                           |",
        "    |   Accuracy-First AI - Runs Locally - Never Hallucinates  |",
        "    +----------------------------------------------------------+",
    };
    for (int i = 0; i < 10; i++) {
        std::cout << gradient_colors[i % NUM_GRADIENT_COLORS] << Color::BOLD;
        typewriter(lines[i], 3);
        std::cout << Color::RESET << "\n";
    }
    std::cout << Color::RESET << "\n";
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
    MKDaemon daemon;
    MK_Shell::MKShell shell;
    MK_Services::ServiceManager serviceManager;
    std::unique_ptr<MKTelegram> telegram;
    MKNeuralNet neuralNet;
    MKCasualResponses casualResponses;
    MKConversationMode conversationMode;
    MKConsciousnessEngine consciousnessEngine;
    MKCrystalNetwork crystalNetwork;
    MKPrometheus prometheus;
    MKIdeaEngine ideaEngine;
    MKPCHelper pcHelper;
    MKGenesisEngine genesis;

    // Crypto trading subsystem
    MKMarketData cryptoMarketData;
    MKTechnicalAnalysis cryptoTechnicalAnalysis;
    MKSignalEngine cryptoSignalEngine;
    MKPortfolioManager cryptoPortfolioManager;
    MKRiskManager cryptoRiskManager;
    MKExchangeAPI cryptoExchangeApi;
    std::unique_ptr<MKTradingBot> cryptoTradingBot;
    MKAirdropFarmer cryptoAirdropFarmer;

    // Mutex protecting shared state between Telegram polling thread and REPL thread.
    // Any code that reads/writes graph, memory, learningEngine, factExtractor, or
    // calls telegram methods must hold this lock.
    std::mutex systemMutex;

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
          reasoningEngine(),
          daemon(),
          shell(),
          serviceManager(),
          telegram(nullptr),
          neuralNet()
    {
        // Initialize Telegram bot if token is available
        const char* tgToken = std::getenv("MK_TELEGRAM_TOKEN");
        if (tgToken && tgToken[0] != '\0') {
            telegram = std::make_unique<MKTelegram>();
            std::cout << "  " << Color::BGREEN << "✓" << Color::RESET
                      << " Telegram bot integration enabled.\n";
        } else {
            std::cout << "  " << Color::DIM << "Note: MK_TELEGRAM_TOKEN not set. "
                      << "Telegram integration disabled." << Color::RESET << "\n";
        }

        // Initialize neural net with a tiny config (untrained, not used in hot path)
        // The GENERATE route gracefully falls back to MKComposer instead.
        neuralNet.init(256, 32, 1, 64);

        // Initialize crypto trading bot (paper mode by default)
        cryptoTradingBot = std::make_unique<MKTradingBot>(
            cryptoMarketData, cryptoTechnicalAnalysis, cryptoSignalEngine,
            cryptoPortfolioManager, cryptoRiskManager, cryptoExchangeApi);
        cryptoExchangeApi.initialize("crypto_config.txt");
    }

    ~MKSystem() = default;
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
        << Color::BOLD << Color::YELLOW << "  🔥 PROMETHEUS" << Color::RESET << "\n"
        << "    " << Color::GREEN << "/prometheus" << Color::RESET << "        Show Prometheus engine stats\n"
        << "    " << Color::GREEN << "/identity" << Color::RESET << "          Show MK's evolved identity state\n"
        << "    " << Color::GREEN << "/fluid" << Color::RESET << "             Show last fluid resonance trace\n"
        << "\n"
        << Color::BOLD << Color::YELLOW << "  🧬 GENESIS" << Color::RESET << "\n"
        << "    " << Color::GREEN << "/genesis" << Color::RESET << "           Show Genesis engine stats\n"
        << "    " << Color::GREEN << "/dream" << Color::RESET << "             Show dream insights from idle time\n"
        << "\n"
        << Color::BOLD << Color::YELLOW << "  💡 IDEAS" << Color::RESET << "\n"
        << "    " << Color::GREEN << "/idea" << Color::RESET << "              Generate a random creative idea\n"
        << "    " << Color::GREEN << "/brainstorm" << Color::RESET << " <topic> Brainstorm 5 ideas about a topic\n"
        << "    " << Color::GREEN << "/invent" << Color::RESET << " <problem>  Invent solutions for a problem\n"
        << "\n"
        << Color::BOLD << Color::YELLOW << "  🖥️  SYSTEM" << Color::RESET << "\n"
        << "    " << Color::GREEN << "/status" << Color::RESET << "            Full system diagnostics\n"
        << "    " << Color::GREEN << "/briefing" << Color::RESET << "          Daily system briefing report\n"
        << "    " << Color::GREEN << "/shell" << Color::RESET << " <cmd>       Run a command in the MK shell\n"
        << "    " << Color::GREEN << "/services" << Color::RESET << "          List registered services & status\n"
        << "    " << Color::GREEN << "/sync" << Color::RESET << "              Sync knowledge with GitHub\n"
        << "    " << Color::GREEN << "/clear" << Color::RESET << "             Clear the screen\n"
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
        {"/ask",      "Knowledge graph lookup"},
        {"/search",   "Internet search (cited)"},
        {"/think",    "Deep reasoning"},
        {"/learn",    "Teach MK a fact"},
        {"/weather",  "Live weather"},
        {"/time",     "Current time"},
        {"/news",     "Tech headlines"},
        {"/status",   "System diagnostics"},
        {"/briefing", "Daily briefing report"},
        {"/shell",    "Run MK shell command"},
        {"/services", "Show service status"},
        {"/sync",     "Sync knowledge with GitHub"},
        {"/prometheus", "Prometheus engine stats"},
        {"/genesis",  "Genesis engine stats"},
        {"/dream",    "Show dream insights"},
        {"/identity", "MK evolved identity state"},
        {"/fluid",    "Fluid resonance trace"},
        {"/help",     "Show all commands"},
        {"/clear",    "Clear the screen"},
        {"/idea",     "Generate a creative idea"},
        {"/brainstorm","Brainstorm ideas on a topic"},
        {"/invent",   "Invent solutions for a problem"},
        {"/quit",     "Save and exit"},
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
    // ---- IDEA ENGINE: Check for creative requests ----
    {
        std::string lower = input;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        bool isIdeaReq = (lower.find("give me an idea") != std::string::npos ||
                          lower.find("brainstorm") != std::string::npos ||
                          lower.find("what if") != std::string::npos ||
                          lower.find("invent something") != std::string::npos ||
                          lower.find("idea for") != std::string::npos);
        if (isIdeaReq) {
            auto ideas = sys.ideaEngine.brainstorm(sys.graph, input, 3);
            if (!ideas.empty()) {
                std::cout << "\n  " << Color::BOLD << Color::BYELLOW << "💡 Ideas:" << Color::RESET << "\n";
                for (const auto& idea : ideas) {
                    std::cout << "  " << Color::BGREEN << "→" << Color::RESET << " " << idea.idea << "\n";
                }
                std::cout << "\n";
                sys.memory.recordInteraction("idea_generation", input);
                return;
            }
        }
    }

    // ---- FRIEND MODE: Check if this is casual conversation ----
    // Before routing through smart router, catch conversational inputs
    if (sys.conversationMode.isConversation(input)) {
        std::string lower = input;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Check if it's an opinion request ("what do you think about X")
        if (sys.conversationMode.isOpinionRequest(input)) {
            std::string subject = sys.conversationMode.extractOpinionSubject(input);
            if (!subject.empty()) {
                // Query knowledge graph for facts about the subject
                std::vector<std::string> facts;
                auto results = sys.graph.getAll(subject);
                for (const auto& e : results) {
                    facts.push_back(e.source + " " + e.relation + " " + e.target);
                }
                std::string response = sys.conversationMode.generateOpinion(subject, facts, sys.casualResponses);
                std::cout << "\n  " << Color::BCYAN << "~" << Color::RESET << " " << response << "\n";
                sys.conversationMode.pushTopic(subject);
                sys.brainMemory.commitToShortTerm("mk", response);
                sys.memory.recordInteraction("conversation", input);
                return;
            }
        }

        // Generate casual response based on input type and mood
        // Priority: GENESIS -> Prometheus -> CXN Crystal Network -> MCE Consciousness -> template fallback
        std::string response;
        if (sys.genesis.isInitialized()) {
            response = sys.genesis.generate(input);
        }
        // Fall back to Prometheus if Genesis returns empty
        if (response.empty() && sys.prometheus.isInitialized()) {
            response = sys.prometheus.generate(input);
        }
        // Fall back to CXN if Prometheus returns empty
        if (response.empty() && sys.crystalNetwork.isInitialized()) {
            response = sys.crystalNetwork.generate(input);
        }
        // Fall back to MCE if CXN returns empty
        if (response.empty() && sys.consciousnessEngine.isInitialized()) {
            response = sys.consciousnessEngine.generate(input, sys.graph);
        }
        // Fall back to template system if all return empty
        if (response.empty()) {
            response = sys.conversationMode.generateResponse(input, sys.casualResponses);
        }
        
        if (!response.empty()) {
            std::cout << "\n  " << Color::BCYAN << "~" << Color::RESET << " " << response << "\n";
            
            // Record in memory
            sys.brainMemory.commitToShortTerm("user", input);
            sys.brainMemory.commitToShortTerm("mk", response);
            sys.memory.recordInteraction("conversation", input);
            
            // Absorb into Prometheus for evolution
            sys.prometheus.absorb(input, response, true);
            
            // Absorb into Genesis for evolution
            sys.genesis.absorb(input, response, true);
            
            // Absorb into consciousness engine for learning
            sys.consciousnessEngine.absorb(input, response);
            
            // Extract biographical facts passively
            sys.factExtractor.extractFromMessage(input);
            return;
        }
        // If generateResponse returned empty, fall through to normal routing
    }

    // ---- NORMAL ROUTING (question/command) ----
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
            // NOTE: neural_net.cpp is wired and initialized with a tiny config
            // (vocab=256, dim=32, layers=1, ff=64) but has NO trained weights.
            // The GENERATE route gracefully falls back to MKComposer which provides
            // template-based responses. The neural net exists for future training
            // but is NOT called in the hot path to avoid nonsense output.
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
        // Absorb into Prometheus for evolution
        sys.prometheus.absorb(input, response, true);
        sys.genesis.absorb(input, response, true);
    } else {
        sys.prometheus.absorb(input, "", false);
        sys.genesis.absorb(input, "", false);
    }

    // Passively extract biographical facts from user input
    sys.factExtractor.extractFromMessage(input);
}

// ============================================================
// Telegram Background Polling Loop
// ============================================================

// Forward declaration - generates an AI response for a natural language query.
// Caller must hold sys.systemMutex.
static std::string generate_ai_response(MKSystem& sys, const std::string& input) {
    auto decision = sys.router.route(input);
    std::string response;
    bool answered = false;

    switch (decision.primaryRoute) {
        case MKRouteType::GRAPH: {
            auto results = sys.graph.getAll(input);
            if (!results.empty()) {
                MKResponseContext ctx;
                ctx.subject = input;
                ctx.is_partial = false;
                ctx.confidence = 0.8f;
                for (const auto& e : results) {
                    ctx.facts_found.push_back(e.source + " " + e.relation + " " + e.target);
                }
                response = sys.composer.composeAnswer(ctx);
                answered = true;
            } else {
                auto pathResult = sys.graph.pathQuery(input, "is_a", 5);
                if (pathResult.found) {
                    response = input + " is " + pathResult.answer;
                    answered = true;
                } else {
                    auto vecResults = sys.vectorSearch.search(input, 3);
                    if (!vecResults.empty() && vecResults[0].score > 0.3f) {
                        MKResponseContext ctx;
                        ctx.subject = input;
                        ctx.is_partial = false;
                        ctx.confidence = vecResults[0].score;
                        for (const auto& vr : vecResults) {
                            if (vr.score > 0.3f) ctx.facts_found.push_back(vr.sourceText);
                        }
                        response = sys.composer.composeAnswer(ctx);
                        answered = !response.empty();
                    }
                }
            }
            break;
        }
        case MKRouteType::REASON: {
            sys.chains.deriveAll(sys.graph);
            auto chain = sys.reasoner.think(input, sys.graph);
            if (!chain.finalAnswer.empty()) {
                response = chain.finalAnswer;
                answered = true;
            }
            break;
        }
        case MKRouteType::INSTANT:
        case MKRouteType::SEARCH:
        case MKRouteType::GENERATE:
        default: {
            // For INSTANT/SEARCH/GENERATE, query the graph and compose
            auto results = sys.graph.getAll(input);
            if (!results.empty()) {
                MKResponseContext ctx;
                ctx.subject = input;
                ctx.is_partial = false;
                ctx.confidence = 0.7f;
                for (const auto& e : results) {
                    ctx.facts_found.push_back(e.source + " " + e.relation + " " + e.target);
                }
                response = sys.composer.composeAnswer(ctx);
                answered = true;
            }
            break;
        }
    }

    if (!answered || response.empty()) {
        // Fallback: try vector search for a relevant answer
        auto vecResults = sys.vectorSearch.search(input, 3);
        if (!vecResults.empty() && vecResults[0].score > 0.3f) {
            MKResponseContext ctx;
            ctx.subject = input;
            ctx.is_partial = true;
            ctx.confidence = vecResults[0].score;
            for (const auto& vr : vecResults) {
                if (vr.score > 0.3f) ctx.facts_found.push_back(vr.sourceText);
            }
            response = sys.composer.composeAnswer(ctx);
        }
        if (response.empty()) {
            response = "I don't have enough knowledge about that yet. Try asking me about science, technology, or programming.";
        }
    }

    // Record interaction
    sys.memory.recordInteraction("telegram_query", input);
    sys.factExtractor.extractFromMessage(input);

    return response;
}

static void telegram_poll_loop(MKSystem& sys) {
    long lastUpdateId = 0;

    while (g_running.load()) {
        if (!sys.telegram) break;

        std::string response;
        {
            std::lock_guard<std::mutex> lock(sys.systemMutex);
            response = sys.telegram->getUpdates(lastUpdateId);
        }

        // Basic JSON parsing for Telegram Bot API response
        // Format: {"ok":true,"result":[{"update_id":123,"message":{"chat":{"id":456},"text":"hello"}}]}
        // We parse multiple updates by searching for "update_id" occurrences
        size_t searchPos = 0;
        while (true) {
            // Find next update_id
            size_t uidPos = response.find("\"update_id\":", searchPos);
            if (uidPos == std::string::npos) break;

            // Find the boundary of this update: the next "update_id" position (or end of string)
            size_t nextUidPos = response.find("\"update_id\":", uidPos + 12);
            size_t updateBoundary = (nextUidPos != std::string::npos) ? nextUidPos : response.size();

            // Extract update_id number
            size_t numStart = uidPos + 12; // length of "\"update_id\":"
            while (numStart < response.size() && (response[numStart] == ' ' || response[numStart] == ':'))
                numStart++;
            size_t numEnd = numStart;
            while (numEnd < response.size() && std::isdigit(response[numEnd]))
                numEnd++;
            if (numEnd == numStart) { searchPos = numEnd + 1; continue; }

            long updateId = std::stol(response.substr(numStart, numEnd - numStart));

            // Skip if we already processed this update
            if (updateId < lastUpdateId) {
                searchPos = numEnd;
                continue;
            }
            lastUpdateId = updateId + 1;

            // Extract chat id: look for "chat":{"id": within this update boundary
            std::string chatId;
            size_t chatPos = response.find("\"chat\"", numEnd);
            if (chatPos != std::string::npos && chatPos < updateBoundary) {
                size_t cidPos = response.find("\"id\":", chatPos);
                if (cidPos != std::string::npos && cidPos < chatPos + 100 && cidPos < updateBoundary) {
                    size_t cidStart = cidPos + 5;
                    while (cidStart < response.size() && (response[cidStart] == ' '))
                        cidStart++;
                    size_t cidEnd = cidStart;
                    // Handle negative chat IDs (groups)
                    if (cidEnd < response.size() && response[cidEnd] == '-') cidEnd++;
                    while (cidEnd < response.size() && std::isdigit(response[cidEnd]))
                        cidEnd++;
                    if (cidEnd > cidStart)
                        chatId = response.substr(cidStart, cidEnd - cidStart);
                }
            }

            // Extract message text: look for "text":" within this update boundary
            std::string msgText;
            size_t textPos = response.find("\"text\":\"", numEnd);
            if (textPos != std::string::npos && textPos < updateBoundary) {
                size_t txtStart = textPos + 8;
                size_t txtEnd = txtStart;
                while (txtEnd < response.size() && txtEnd < updateBoundary && response[txtEnd] != '"') {
                    if (response[txtEnd] == '\\') txtEnd++; // skip escaped chars
                    txtEnd++;
                }
                if (txtEnd > txtStart)
                    msgText = response.substr(txtStart, txtEnd - txtStart);
            }

            // Process the message if we have chat_id and text
            if (!chatId.empty() && !msgText.empty()) {
                std::lock_guard<std::mutex> lock(sys.systemMutex);
                if (msgText[0] == '/') {
                    // Command-based messages use autoReply
                    sys.telegram->autoReply(chatId, msgText);
                } else {
                    // Route natural language through the AI pipeline
                    std::string reply = generate_ai_response(sys, msgText);
                    sys.telegram->sendMessage(chatId, reply);
                }
            }

            searchPos = (nextUidPos != std::string::npos) ? nextUidPos : response.size();
        }

        // Sleep between polls (2 seconds)
        for (int i = 0; i < 20 && g_running.load(); i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
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
    std::cout << "  " << Color::DIM << "Platform:" << Color::RESET << " " << Color::BOLD << platform << Color::RESET << "\n";

    // Step 3: Initialize all modules
    std::cout << "  " << Color::DIM << "Initializing modules..." << Color::RESET << "\n\n";
    MKSystem sys;

    // Step 4: Load knowledge
    sys.graph.loadAllKnowledge();
    sys.graph.loadFromFile("../../../crypto/crypto_knowledge.mk");

    // Step 4a: Index all knowledge into vector search for semantic retrieval
    {
        const auto& allEdges = sys.graph.getAllEdges();
        int indexed = 0;
        for (const auto& edge : allEdges) {
            std::string sourceText = edge.source + " " + edge.relation + " " + edge.target;
            std::string label = edge.source + "_" + edge.relation + "_" + edge.target;
            auto embedding = sys.vectorSearch.textToEmbedding(sourceText);
            sys.vectorSearch.addVector(label, sourceText, embedding);
            indexed++;
        }
        if (indexed > 0) {
            sys.vectorSearch.buildIndex();
        }
        std::cout << "  " << Color::BGREEN << "✓" << Color::RESET 
                  << " Indexed " << indexed << " facts into vector search.\n";
    }

    // Step 4b: Restore learning engine knowledge
    sys.learningEngine.restore();

    // Step 4b-shared: Collect casual response texts once (shared across engine bootstraps)
    std::vector<std::string> sharedCasualTexts;
    {
        auto appendVec = [&](const std::vector<std::string>& v) {
            sharedCasualTexts.insert(sharedCasualTexts.end(), v.begin(), v.end());
        };
        appendVec(sys.casualResponses.greetings);
        appendVec(sys.casualResponses.goodbyes);
        appendVec(sys.casualResponses.acknowledgments);
        appendVec(sys.casualResponses.reactions_positive);
        appendVec(sys.casualResponses.reactions_negative);
        appendVec(sys.casualResponses.encouragements);
        appendVec(sys.casualResponses.follow_ups);
        appendVec(sys.casualResponses.mood_happy);
        appendVec(sys.casualResponses.mood_sad);
        appendVec(sys.casualResponses.mood_angry);
        appendVec(sys.casualResponses.mood_chill);
    }

    // Step 4c: Initialize MK Consciousness Engine
    sys.consciousnessEngine.initialize(sys.graph, sys.casualResponses);

    // Step 4d: Initialize CXN Crystal Network
    {
        // CXN uses fewer knowledge facts (cap at 300)
        std::vector<std::string> knowledgeFacts;
        const auto& edges = sys.graph.getAllEdges();
        for (const auto& e : edges) {
            knowledgeFacts.push_back(e.source + " " + e.relation + " " + e.target);
            if (knowledgeFacts.size() > 300) break;
        }

        sys.crystalNetwork.initialize(sharedCasualTexts, knowledgeFacts);
    }

    // Step 4e: Initialize Prometheus Engine
    {
        // Prometheus uses more knowledge facts (cap at 500)
        std::vector<std::string> knowledgeFacts;
        const auto& prometheusEdges = sys.graph.getAllEdges();
        for (const auto& e : prometheusEdges) {
            knowledgeFacts.push_back(e.source + " " + e.relation + " " + e.target);
            if (knowledgeFacts.size() > 500) break;
        }

        // Get code fragments from bootstrap
        std::vector<std::string> codeFrags = getCodeFragmentStrings();

        sys.prometheus.initialize(sharedCasualTexts, knowledgeFacts, codeFrags);
        sys.prometheus.load();  // Restore identity state if saved
    }

    // Step 4f: Initialize Genesis Engine (next-gen AI)
    {
        std::vector<std::string> genesisKnowledge;
        const auto& genesisEdges = sys.graph.getAllEdges();
        for (const auto& e : genesisEdges) {
            genesisKnowledge.push_back(e.source + " " + e.relation + " " + e.target);
            if (genesisKnowledge.size() > 3000) break;
        }
        sys.genesis.initialize(sharedCasualTexts, genesisKnowledge);
    }

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

    // Step 7: Register services
    sys.serviceManager.register_service("knowledge_graph", "mk_graph",
        {}, "Pattern graph and knowledge store");
    sys.serviceManager.register_service("reasoning_engine", "mk_reason",
        {"knowledge_graph"}, "Deep multi-hop reasoning");
    sys.serviceManager.register_service("learning_engine", "mk_learn",
        {"knowledge_graph"}, "Fact learning and persistence");
    sys.serviceManager.register_service("vector_search", "mk_vector",
        {"knowledge_graph"}, "Approximate nearest neighbor search");
    sys.serviceManager.register_service("realtime_apis", "mk_apis",
        {}, "Weather, time, news API access");
    if (sys.telegram) {
        sys.serviceManager.register_service("telegram_bot", "mk_telegram",
            {"realtime_apis"}, "Telegram bot polling and replies");
    }
    sys.serviceManager.start_all();
    std::cout << "  " << Color::BGREEN << "✓" << Color::RESET << " Services: "
              << Color::BOLD << sys.serviceManager.running_count() << Color::RESET
              << "/" << sys.serviceManager.total_count() << " running\n";

    // Step 8: Start Telegram polling thread if token is available
    std::thread telegramThread;
    bool telegramThreadRunning = false;
    if (sys.telegram) {
        telegramThread = std::thread(telegram_poll_loop, std::ref(sys));
        telegramThreadRunning = true;
        std::cout << "  " << Color::BGREEN << "✓" << Color::RESET
                  << " Telegram polling thread started.\n";
    }

    // Register telegram polling as a daemon job for monitoring
    sys.daemon.addJob("health_check", 30, [&sys]() {
        sys.serviceManager.run_health_checks();
    });
    sys.daemon.addJob("genesis_dream", 10, [&sys]() {
        sys.genesis.dreamTick();
    });
    if (sys.telegram) {
        sys.daemon.addJob("telegram_poll_monitor", 60, []() {
            // Monitoring placeholder - the actual polling runs in its own thread
        });
    }

    // ============================================================
    // First-boot: offer LLM model download
    // ============================================================
    {
        const std::string modelPath = "llm/models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf";
        const std::string dismissPath = ".model_prompt_dismissed";
        bool modelExists = false;
        bool dismissed = false;

        {
            std::ifstream mf(modelPath);
            modelExists = mf.good();
        }
        {
            std::ifstream df(dismissPath);
            dismissed = df.good();
        }

        if (!modelExists && !dismissed) {
            std::cout << "\n"
                << Color::BOLD << Color::BCYAN
                << "  ╭─────────────────────────────────────────────╮\n"
                << "  │  Want MK to be better at understanding you? │\n"
                << "  │                                             │\n"
                << "  │  Download a small AI model (638MB) that     │\n"
                << "  │  helps MK understand messy text and slang.  │\n"
                << "  │  MK works fine without it, just less smart  │\n"
                << "  │  at parsing casual language.                │\n"
                << "  │                                             │\n"
                << "  │  [y] Yes, download now                      │\n"
                << "  │  [n] No thanks                              │\n"
                << "  │  [l] Ask me later                           │\n"
                << "  ╰─────────────────────────────────────────────╯\n"
                << Color::RESET << "\n"
                << "  Your choice: ";
            std::cout.flush();

            std::string choice;
            std::getline(std::cin, choice);
            choice = trim(choice);
            std::transform(choice.begin(), choice.end(), choice.begin(), ::tolower);

            if (choice == "y" || choice == "yes") {
                std::cout << "\n  " << Color::DIM << "Downloading model..." << Color::RESET << "\n";
                system("bash llm/download_model.sh");
                std::cout << "  " << Color::BGREEN << "✓" << Color::RESET
                          << " Model downloaded! MK is now smarter at understanding you.\n";
            } else if (choice == "n" || choice == "no") {
                {
                    std::ofstream df(dismissPath);
                    df << "dismissed" << std::endl;
                }
                std::cout << "  " << Color::DIM
                          << "Got it. You can always run ./llm/download_model.sh later."
                          << Color::RESET << "\n";
            } else {
                // "l", "later", empty, or anything else
                std::cout << "  " << Color::DIM
                          << "No problem. I'll ask again next time."
                          << Color::RESET << "\n";
            }
        }
    }

    // ============================================================
    // Main REPL Loop
    // ============================================================
    unsigned int interaction_count = 0;
    const unsigned int AUTO_SAVE_INTERVAL = 50;
    auto sessionStartTime = std::chrono::steady_clock::now();
    unsigned int queryCount = 0;
    unsigned int factsLearned = 0;

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
                // Show session statistics before quitting
                auto sessionEndTime = std::chrono::steady_clock::now();
                auto durationSec = std::chrono::duration_cast<std::chrono::seconds>(
                    sessionEndTime - sessionStartTime).count();
                int minutes = static_cast<int>(durationSec / 60);
                int seconds = static_cast<int>(durationSec % 60);
                std::cout << "\n";
                print_separator();
                std::cout << "  " << Color::BOLD << Color::BCYAN << "Session Stats" << Color::RESET << "\n";
                std::cout << "  " << Color::DIM << "Duration:       " << Color::RESET << minutes << "m " << seconds << "s\n";
                std::cout << "  " << Color::DIM << "Total queries:  " << Color::RESET << queryCount << "\n";
                std::cout << "  " << Color::DIM << "Facts learned:  " << Color::RESET << factsLearned << "\n";
                std::cout << "  " << Color::DIM << "Interactions:   " << Color::RESET << interaction_count << "\n";
                print_separator();
                g_running = false; commandFound = true;
            } else if (input == "/clear") {
                // Clear screen with ANSI escape code
                std::cout << "\033[2J\033[H";
                std::cout.flush();
                commandFound = true;
            } else if (input == "/help") {
                cmd_help(); commandFound = true;
            } else {
                // Acquire system mutex for all commands that access shared state.
                // This protects against concurrent modification from the Telegram polling thread.
                std::lock_guard<std::mutex> lock(sys.systemMutex);

                if (input == "/status") {
                    cmd_status(sys); commandFound = true;
                } else if (input == "/sync" || input.substr(0, 6) == "/sync ") {
                    // Sync knowledge with GitHub via system command
                    std::string syncArg = "pull";
                    if (input.size() > 6) syncArg = trim(input.substr(6));
                    std::cout << "\n  " << Color::BCYAN << "⟳" << Color::RESET 
                              << " Syncing knowledge with GitHub (" << syncArg << ")...\n";
                    if (syncArg == "push") {
                        int result = std::system("git add ai_core/hre/knowledge_files/learned_facts.mk ai_core/hre/knowledge_files/personal_facts.mk 2>/dev/null && git commit -m 'sync: MK auto-push knowledge' 2>/dev/null && git push 2>/dev/null");
                        if (result == 0) {
                            std::cout << "  " << Color::GREEN << "✓" << Color::RESET 
                                      << " Knowledge pushed to GitHub successfully.\n";
                        } else {
                            std::cout << "  " << Color::YELLOW << "⚠" << Color::RESET 
                                      << " Push failed (no changes or no git access). Use mk_sync tool for full sync.\n";
                        }
                    } else {
                        int result = std::system("git pull --rebase 2>/dev/null");
                        if (result == 0) {
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
                    cmd_learn(sys, trim(input.substr(7))); factsLearned++; commandFound = true;
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
                    show_thinking_spinner(300);
                    cmd_think(sys, trim(input.substr(7))); queryCount++; commandFound = true;
                } else if (input == "/think") {
                    cmd_think(sys, ""); commandFound = true;
                } else if (input == "/briefing") {
                    cmd_briefing(sys); commandFound = true;
                } else if (input == "/trace") {
                    std::string trace = sys.consciousnessEngine.explain();
                    std::cout << "\n  " << Color::BOLD << Color::BMAGENTA << "🧠 Thought Trace" << Color::RESET << "\n";
                    std::cout << "  " << Color::DIM << trace << Color::RESET << "\n";
                    commandFound = true;
                } else if (input == "/echo") {
                    std::string profile = sys.consciousnessEngine.getEchoProfile();
                    std::cout << "\n  " << Color::BOLD << Color::BCYAN << "🪞 Echo Memory" << Color::RESET << "\n";
                    std::cout << "  " << Color::DIM << profile << Color::RESET << "\n";
                    commandFound = true;
                } else if (input == "/crystals") {
                    std::string stats = sys.crystalNetwork.getStats();
                    std::string trace = sys.crystalNetwork.explain();
                    std::cout << "\n  " << Color::BOLD << Color::BMAGENTA << "💎 Crystal Network" << Color::RESET << "\n";
                    std::cout << "  " << Color::DIM << stats << Color::RESET << "\n";
                    if (!trace.empty() && trace != "No CXN trace yet.") {
                        std::cout << "\n  " << Color::BOLD << "Last trace:" << Color::RESET << "\n";
                        std::cout << "  " << Color::DIM << trace << Color::RESET << "\n";
                    }
                    commandFound = true;
                } else if (input == "/prometheus") {
                    std::string stats = sys.prometheus.getStats();
                    std::cout << "\n  " << Color::BOLD << Color::BRED << "🔥 Prometheus Engine" << Color::RESET << "\n";
                    std::cout << "  " << Color::DIM << stats << Color::RESET << "\n";
                    commandFound = true;
                } else if (input == "/genesis") {
                    std::string stats = sys.genesis.getStats();
                    std::cout << "\n  " << Color::BOLD << Color::BGREEN << "🧬 Genesis Engine" << Color::RESET << "\n";
                    std::cout << "  " << Color::DIM << stats << Color::RESET << "\n";
                    commandFound = true;
                } else if (input == "/dream") {
                    std::string insight = sys.genesis.getUndeliveredInsight();
                    if (!insight.empty()) {
                        std::cout << "\n  " << Color::BOLD << Color::BMAGENTA << "💭 Dream Insight" << Color::RESET << "\n";
                        std::cout << "  " << Color::BCYAN << "→" << Color::RESET << " " << insight << "\n";
                    } else {
                        std::cout << "\n  " << Color::DIM << "No new dream insights yet. MK dreams during idle time." << Color::RESET << "\n";
                    }
                    commandFound = true;
                } else if (input == "/identity") {
                    std::string id = sys.prometheus.getIdentity();
                    std::cout << "\n  " << Color::BOLD << Color::BMAGENTA << "🪞 Evolved Identity" << Color::RESET << "\n";
                    std::cout << "  " << Color::DIM << id << Color::RESET << "\n";
                    commandFound = true;
                } else if (input == "/fluid") {
                    std::string trace = sys.prometheus.getFluidTrace();
                    std::cout << "\n  " << Color::BOLD << Color::BCYAN << "💧 Fluid Resonance Trace" << Color::RESET << "\n";
                    std::cout << "  " << Color::DIM << trace << Color::RESET << "\n";
                    std::string thought = sys.prometheus.explain();
                    if (!thought.empty()) {
                        std::cout << "\n  " << Color::BOLD << "Thought trace:" << Color::RESET << "\n";
                        std::cout << "  " << Color::DIM << thought << Color::RESET << "\n";
                    }
                    commandFound = true;
                } else if (input == "/idea") {
                    auto idea = sys.ideaEngine.generateIdea(sys.graph);
                    std::cout << "\n  " << Color::BOLD << Color::BYELLOW << "💡 IDEA #" << sys.ideaEngine.getHistory().size() << Color::RESET << "\n";
                    std::cout << "  " << Color::DIM << "─────────────────────────────────" << Color::RESET << "\n";
                    std::cout << "  Concept A: " << Color::BOLD << idea.conceptA << Color::RESET << "\n";
                    std::cout << "  Concept B: " << Color::BOLD << idea.conceptB << Color::RESET << "\n";
                    std::cout << "  Bridge: " << Color::CYAN << idea.bridge << Color::RESET << "\n\n";
                    std::cout << "  " << Color::BGREEN << "→" << Color::RESET << " \"" << idea.idea << "\"\n\n";
                    int fBar = (int)(idea.feasibility * 10);
                    int nBar = (int)(idea.novelty * 10);
                    std::cout << "  Feasibility: " << Color::GREEN;
                    for (int i = 0; i < 10; i++) std::cout << (i < fBar ? "\xe2\x96\x88" : "\xe2\x96\x91");
                    std::cout << Color::RESET << " " << (int)(idea.feasibility * 100) << "%\n";
                    std::cout << "  Novelty:     " << Color::CYAN;
                    for (int i = 0; i < 10; i++) std::cout << (i < nBar ? "\xe2\x96\x88" : "\xe2\x96\x91");
                    std::cout << Color::RESET << " " << (int)(idea.novelty * 100) << "%\n";
                    std::cout << "  Category:    " << Color::MAGENTA << idea.category << Color::RESET << "\n";
                    std::cout << "  " << Color::DIM << "─────────────────────────────────" << Color::RESET << "\n";
                    commandFound = true;
                } else if (input.size() > 12 && input.substr(0, 12) == "/brainstorm ") {
                    std::string topic = trim(input.substr(12));
                    if (topic.empty()) {
                        std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET << " /brainstorm <topic>\n";
                    } else {
                        auto ideas = sys.ideaEngine.brainstorm(sys.graph, topic, 5);
                        std::cout << "\n  " << Color::BOLD << Color::BYELLOW << "💡 BRAINSTORM: " << topic << Color::RESET << "\n";
                        std::cout << "  " << Color::DIM << "═════════════════════════════════" << Color::RESET << "\n";
                        int n = 1;
                        for (const auto& idea : ideas) {
                            std::cout << "\n  " << Color::BOLD << n++ << "." << Color::RESET << " " << idea.idea << "\n";
                            std::cout << "     " << Color::DIM << "[" << idea.category << " | feasibility: "
                                      << (int)(idea.feasibility * 100) << "% | novelty: "
                                      << (int)(idea.novelty * 100) << "%]" << Color::RESET << "\n";
                        }
                        std::cout << "\n  " << Color::DIM << "═════════════════════════════════" << Color::RESET << "\n";
                    }
                    commandFound = true;
                } else if (input == "/brainstorm") {
                    std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET << " /brainstorm <topic>\n"
                              << "  " << Color::DIM << "Example: /brainstorm renewable energy" << Color::RESET << "\n";
                    commandFound = true;
                } else if (input.size() > 8 && input.substr(0, 8) == "/invent ") {
                    std::string problem = trim(input.substr(8));
                    if (problem.empty()) {
                        std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET << " /invent <problem>\n";
                    } else {
                        auto ideas = sys.ideaEngine.inventFor(sys.graph, problem);
                        std::cout << "\n  " << Color::BOLD << Color::BYELLOW << "🔧 INVENTIONS for: " << problem << Color::RESET << "\n";
                        std::cout << "  " << Color::DIM << "═════════════════════════════════" << Color::RESET << "\n";
                        int n = 1;
                        for (const auto& idea : ideas) {
                            std::cout << "\n  " << Color::BOLD << n++ << "." << Color::RESET << " " << idea.idea << "\n";
                            std::cout << "     " << Color::DIM << "[feasibility: "
                                      << (int)(idea.feasibility * 100) << "% | novelty: "
                                      << (int)(idea.novelty * 100) << "%]" << Color::RESET << "\n";
                        }
                        std::cout << "\n  " << Color::DIM << "═════════════════════════════════" << Color::RESET << "\n";
                    }
                    commandFound = true;
                } else if (input == "/invent") {
                    std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET << " /invent <problem>\n"
                              << "  " << Color::DIM << "Example: /invent I need to stay cool in summer" << Color::RESET << "\n";
                    commandFound = true;
                } else if (input.size() > 7 && input.substr(0, 7) == "/shell ") {
                    std::string shellCmd = trim(input.substr(7));
                    if (shellCmd.empty()) {
                        std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET
                                  << " /shell <command>\n";
                    } else {
                        std::string result = sys.shell.execute(shellCmd);
                        if (!result.empty()) {
                            std::cout << "\n" << result;
                            if (result.back() != '\n') std::cout << "\n";
                        }
                    }
                    commandFound = true;
                } else if (input == "/shell") {
                    std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET
                              << " /shell <command>\n"
                              << "  " << Color::DIM << "Example: /shell echo hello world"
                              << Color::RESET << "\n";
                    commandFound = true;
                } else if (input == "/services") {
                    auto statuses = sys.serviceManager.get_all_status();
                    if (statuses.empty()) {
                        std::cout << "\n  " << Color::YELLOW << "○" << Color::RESET
                                  << " No services registered.\n";
                    } else {
                        std::cout << "\n  " << Color::BOLD << Color::BCYAN
                                  << "  ╭─────────────────────────────────────╮\n"
                                  << "  │         REGISTERED SERVICES         │\n"
                                  << "  ╰─────────────────────────────────────╯\n"
                                  << Color::RESET;
                        for (const auto& [name, status] : statuses) {
                            const char* statusIcon = "?";
                            const char* statusColor = Color::WHITE;
                            std::string statusLabel;
                            switch (status) {
                                case MK_Services::ServiceStatus::RUNNING:
                                    statusIcon = "●"; statusColor = Color::BGREEN;
                                    statusLabel = "RUNNING"; break;
                                case MK_Services::ServiceStatus::STOPPED:
                                    statusIcon = "○"; statusColor = Color::RED;
                                    statusLabel = "STOPPED"; break;
                                case MK_Services::ServiceStatus::STARTING:
                                    statusIcon = "◐"; statusColor = Color::YELLOW;
                                    statusLabel = "STARTING"; break;
                                case MK_Services::ServiceStatus::CRASHED:
                                    statusIcon = "✗"; statusColor = Color::BRED;
                                    statusLabel = "CRASHED"; break;
                                case MK_Services::ServiceStatus::DISABLED:
                                    statusIcon = "⊘"; statusColor = Color::DIM;
                                    statusLabel = "DISABLED"; break;
                                default:
                                    statusIcon = "?"; statusColor = Color::DIM;
                                    statusLabel = "UNKNOWN"; break;
                            }
                            std::cout << "    " << statusColor << statusIcon << Color::RESET
                                      << " " << Color::BOLD << name << Color::RESET
                                      << " " << Color::DIM << "[" << statusLabel << "]"
                                      << Color::RESET << "\n";
                        }
                        std::cout << "\n  " << Color::DIM << "Total: "
                                  << sys.serviceManager.running_count() << "/"
                                  << sys.serviceManager.total_count() << " running"
                                  << Color::RESET << "\n";
                    }
                    commandFound = true;
                }

                if (!commandFound) {
                    // Show suggestions for the partial command
                    show_slash_suggestions(input);
                    std::cout << "\n  " << Color::YELLOW << "⚠" << Color::RESET 
                              << " Unknown command: " << Color::RED << input << Color::RESET
                              << ". Type " << Color::GREEN << "/help" << Color::RESET << " for options.\n";
                }
            } // end of locked else block
        } else {
            // Natural language routing (acquire lock for thread safety)
            std::lock_guard<std::mutex> lock(sys.systemMutex);
            show_thinking_spinner(200);
            handle_natural_query(sys, input);
            queryCount++;
        }

        // Track dialog context for all interactions (lock for thread safety)
        {
            std::lock_guard<std::mutex> lock(sys.systemMutex);
            sys.brainMemory.commitToShortTerm("user", input);
        }

        // Periodic auto-save every N interactions to limit data loss on crash
        interaction_count++;
        if (interaction_count % AUTO_SAVE_INTERVAL == 0) {
            std::lock_guard<std::mutex> lock(sys.systemMutex);
            sys.memory.saveToDisk();
            sys.improver.saveLog();
        }
    }

    // ============================================================
    // Graceful Shutdown
    // ============================================================
    std::cout << "\n  " << Color::YELLOW << "⏻" << Color::RESET << " Shutting down...\n";

    // Signal and join the telegram polling thread
    if (telegramThreadRunning && telegramThread.joinable()) {
        std::cout << "  " << Color::DIM << "Stopping Telegram polling..." << Color::RESET << "\n";
        telegramThread.join();
        std::cout << "  " << Color::GREEN << "✓" << Color::RESET << " Telegram thread stopped.\n";
    }

    // Shut down daemon and services
    sys.daemon.shutdown();
    sys.serviceManager.shutdown_all();

    sys.memory.saveToDisk();
    sys.improver.saveLog();
    sys.learningEngine.persist();
    sys.consciousnessEngine.save();
    sys.crystalNetwork.save("cxn_crystals.dat");
    sys.prometheus.save();
    sys.genesis.save();
    std::cout << "  " << Color::GREEN << "✓" << Color::RESET << " Memory saved. Improvement log saved. Knowledge persisted. Genesis state saved.\n";
    std::cout << "  " << Color::BOLD << Color::CYAN << "MK OS shut down cleanly. Goodbye." 
              << Color::RESET << "\n\n";

    return 0;
}

#endif // MK_ENTRY_CPP
