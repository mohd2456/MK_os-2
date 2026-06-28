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
#include "../ai_core/input_preprocessor.cpp"
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
#include "../plugins/plugin_interface.cpp"
#include "../ai_core/neural_net.cpp"
#include "../llm/llm_engine.cpp"
#include "../ai_core/correction_engine.cpp"
#include "../ai_core/math_solver.cpp"
#include "../security/crypto.cpp"
#include "task_scheduler.cpp"
#include "../tools/file_reader.cpp"
#include "../tools/code_runner.cpp"
#include "../tools/image_analyzer.cpp"
#include "../remote/pc_controller.cpp"
#include "../remote/query_server.cpp"

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
    MKDaemon daemon;
    MK_Shell::MKShell shell;
    MK_Services::ServiceManager serviceManager;
    std::unique_ptr<MKTelegram> telegram;
    MKNeuralNet neuralNet;
    MKInputPreprocessor preprocessor;
    MKLLMEngine llmEngine;
    MKCorrectionEngine correctionEngine;
    MKTaskScheduler taskScheduler;
    MKFileReader fileReader;
    MKCodeRunner codeRunner;
    MKPluginSystem pluginSystem;
    MKSystemInfoPlugin sysInfoPlugin;
    MKImageAnalyzer imageAnalyzer;
    MKMathSolver mathSolver;
    MKCrypto crypto;
    MKPCController pcController;
    MKQueryServer queryServer;

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
          neuralNet(),
          preprocessor(),
          llmEngine(),
          correctionEngine(),
          taskScheduler()
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

        // Register built-in plugins
        pluginSystem.registerPlugin(&sysInfoPlugin);

        // Wire encryption into persistent memory
        memory.setEncryption(
            [this](const std::string& data) { return crypto.encrypt(data); },
            [this](const std::string& data) { return crypto.decrypt(data); }
        );
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
        << Color::BOLD << Color::YELLOW << "  🖥️  SYSTEM" << Color::RESET << "\n"
        << "    " << Color::GREEN << "/status" << Color::RESET << "            Full system diagnostics\n"
        << "    " << Color::GREEN << "/briefing" << Color::RESET << "          Daily system briefing report\n"
        << "    " << Color::GREEN << "/shell" << Color::RESET << " <cmd>       Run a command in the MK shell\n"
        << "    " << Color::GREEN << "/services" << Color::RESET << "          List registered services & status\n"
        << "    " << Color::GREEN << "/pc" << Color::RESET << " <cmd>          Remote PC control (run /pc help)\n"
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
        {"/pc",       "Remote PC control"},
        {"/sync",     "Sync knowledge with GitHub"},
        {"/help",     "Show all commands"},
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
    // ─── PC Routing: detect "on my pc" / "on the pc" / "on my computer" ─────
    {
        std::string lower = input;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        bool pcRouted = false;

        if (lower.find("on my pc") != std::string::npos ||
            lower.find("on the pc") != std::string::npos ||
            lower.find("on my computer") != std::string::npos) {

            if (!sys.pcController.isConnected()) {
                sys.pcController.connect();
            }
            if (sys.pcController.isConnected()) {
                std::string result;
                if (lower.find("open ") != std::string::npos) {
                    // Extract app name: "open X on my pc"
                    size_t openPos = lower.find("open ");
                    size_t onPos = lower.find(" on ");
                    if (openPos != std::string::npos && onPos != std::string::npos && onPos > openPos + 5) {
                        std::string app = input.substr(openPos + 5, onPos - (openPos + 5));
                        result = sys.pcController.openApp(trim(app));
                    }
                } else if (lower.find("running") != std::string::npos ||
                           lower.find("processes") != std::string::npos ||
                           lower.find("what's running") != std::string::npos) {
                    result = sys.pcController.listProcesses();
                } else if (lower.find("system info") != std::string::npos ||
                           lower.find("specs") != std::string::npos) {
                    result = sys.pcController.getSystemInfo();
                } else if (lower.find("clipboard") != std::string::npos) {
                    result = sys.pcController.getClipboard();
                } else if (lower.find("screenshot") != std::string::npos) {
                    result = sys.pcController.screenshot();
                } else if (lower.find("read ") != std::string::npos) {
                    // "read X on my pc"
                    size_t readPos = lower.find("read ");
                    size_t onPos = lower.find(" on ");
                    if (readPos != std::string::npos && onPos != std::string::npos && onPos > readPos + 5) {
                        std::string path = input.substr(readPos + 5, onPos - (readPos + 5));
                        result = sys.pcController.readFile(trim(path));
                    }
                } else if (lower.find("files") != std::string::npos ||
                           lower.find("list") != std::string::npos) {
                    result = sys.pcController.listFiles("~");
                }

                if (!result.empty()) {
                    std::cout << "\n  " << Color::BCYAN << "[PC]" << Color::RESET
                              << " " << result;
                    if (result.back() != '\n') std::cout << "\n";
                    pcRouted = true;
                }
            } else {
                std::cout << "\n  " << Color::RED << "✗" << Color::RESET
                          << " PC agent not connected. Set MK_PC_IP and run agent.py on your PC.\n";
                pcRouted = true;
            }
        }
        if (pcRouted) return;
    }

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

    // Step 3a: LLM Status Check
    sys.llmEngine.checkAndReport();

    // Step 4: Load knowledge
    sys.graph.loadAllKnowledge();

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

    // Step 8: Try connecting to remote PC agent
    {
        const char* manualIP = std::getenv("MK_PC_IP");
        if (manualIP && manualIP[0] != '\0') {
            // Manual IP configured — connect directly (existing behavior)
            std::cout << "  " << Color::DIM << "Connecting to PC agent..." << Color::RESET << "\n";
            if (sys.pcController.connect()) {
                std::cout << "  " << Color::BGREEN << "\u2713" << Color::RESET
                          << " PC connected (" << sys.pcController.getIP()
                          << ":" << sys.pcController.getPort() << ")\n";
            } else {
                std::cout << "  " << Color::DIM
                          << "\u2139 PC agent not responding at " << manualIP
                          << ". Check agent is running."
                          << Color::RESET << "\n";
            }
        } else {
            // No manual IP — try auto-discovery
            std::cout << "  " << Color::DIM << "Scanning for PC agents on local network..."
                      << Color::RESET << "\n";
            if (sys.pcController.autoConnect()) {
                std::cout << "  " << Color::BGREEN << "\u2713" << Color::RESET
                          << " PC auto-discovered: "
                          << Color::BOLD << sys.pcController.getConnectedHostname() << Color::RESET
                          << " (" << sys.pcController.getIP()
                          << ":" << sys.pcController.getPort() << ")\n";
            } else {
                std::cout << "  " << Color::DIM
                          << "\u2139 No PC agent found. Start the agent on your PC to auto-connect."
                          << Color::RESET << "\n";
            }
        }
    }

    // Step 8b: Start Query Server (for remote CLI/Chat clients)
    {
        // Read port from env or use default
        int queryPort = 9878;
        const char* qPortEnv = std::getenv("MK_QUERY_PORT");
        if (qPortEnv && qPortEnv[0] != '\0') {
            queryPort = std::atoi(qPortEnv);
        }

        // Read token from env (shared with PC agent token)
        const char* qToken = std::getenv("MK_PC_TOKEN");
        if (qToken && qToken[0] != '\0') {
            sys.queryServer.setAuthToken(qToken);
        }

        sys.queryServer.setPort(queryPort);
        sys.queryServer.setBrainMutex(&sys.systemMutex);
        sys.queryServer.setQueryHandler([&sys](const std::string& input) -> std::string {
            return generate_ai_response(sys, input);
        });

        if (sys.queryServer.start()) {
            std::cout << "  " << Color::BGREEN << "✓" << Color::RESET
                      << " Query server listening on port " << queryPort
                      << " (for remote clients)\n";
        } else {
            std::cout << "  " << Color::YELLOW << "⚠" << Color::RESET
                      << " Query server failed to start on port " << queryPort << "\n";
        }
    }

    // Step 8c: Background PC discovery thread (continuous scan every 30s)
    std::thread pcDiscoveryThread;
    bool pcDiscoveryRunning = false;
    {
        const char* manualIP = std::getenv("MK_PC_IP");
        if (!manualIP || manualIP[0] == '\0') {
            // Only run background discovery when no manual IP is set
            pcDiscoveryRunning = true;
            pcDiscoveryThread = std::thread([&sys]() {
                bool wasConnected = sys.pcController.isConnected();
                while (g_running.load()) {
                    // Sleep 30 seconds (in small increments for responsive shutdown)
                    for (int i = 0; i < 300 && g_running.load(); i++) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    if (!g_running.load()) break;

                    bool currentlyConnected = sys.pcController.isConnected();

                    if (!currentlyConnected) {
                        // Not connected — try to discover and connect
                        if (sys.pcController.autoConnect()) {
                            std::cout << "\n  " << Color::BGREEN << "\u2713" << Color::RESET
                                      << " PC agent connected: "
                                      << Color::BOLD << sys.pcController.getConnectedHostname()
                                      << Color::RESET
                                      << " (" << sys.pcController.getIP() << ":"
                                      << sys.pcController.getPort() << ")\n";
                            std::cout << "  " << Color::BOLD << Color::BCYAN << "MK"
                                      << Color::RESET << Color::CYAN << " \u203A " << Color::RESET;
                            std::cout.flush();
                            wasConnected = true;
                        }
                    } else if (wasConnected && !currentlyConnected) {
                        // Was connected, now disconnected — notify and resume scanning
                        std::cout << "\n  " << Color::YELLOW << "\u26A0" << Color::RESET
                                  << " PC agent disconnected. Scanning..."
                                  << Color::RESET << "\n";
                        std::cout << "  " << Color::BOLD << Color::BCYAN << "MK"
                                  << Color::RESET << Color::CYAN << " \u203A " << Color::RESET;
                        std::cout.flush();
                        wasConnected = false;
                    }
                }
            });
        }
    }

    // Step 9: Start Telegram polling thread if token is available
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
    sys.daemon.addJob("task_scheduler_check", 15, [&sys]() {
        std::lock_guard<std::mutex> lock(sys.systemMutex);
        auto dueTasks = sys.taskScheduler.checkDueTasks();
        for (const auto& task : dueTasks) {
            std::cout << "\n  " << Color::BYELLOW << "⏰" << Color::RESET
                      << " Reminder: " << Color::BOLD << task.message << Color::RESET << "\n";
            std::cout << "  " << Color::BOLD << Color::BCYAN << "MK"
                      << Color::RESET << Color::CYAN << " › " << Color::RESET;
            std::cout.flush();
            // Send via Telegram if chatId is set
            if (sys.telegram && !task.chatId.empty()) {
                sys.telegram->sendMessage(task.chatId, "⏰ Reminder: " + task.message);
            }
        }
    });
    if (sys.telegram) {
        sys.daemon.addJob("telegram_poll_monitor", 60, []() {
            // Monitoring placeholder - the actual polling runs in its own thread
        });
    }

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
                } else if (input.size() > 8 && input.substr(0, 8) == "/remind ") {
                    std::string args = trim(input.substr(8));
                    // Format: /remind <time> <message>
                    // time: 5m, 1h, 2d, 30s
                    size_t spacePos = args.find(' ');
                    if (spacePos != std::string::npos) {
                        std::string timeStr = args.substr(0, spacePos);
                        std::string msg = trim(args.substr(spacePos + 1));
                        int offset = MKTaskScheduler::parseTimeOffset(timeStr);
                        if (offset > 0 && !msg.empty()) {
                            int id = sys.taskScheduler.addReminder(offset, msg);
                            std::cout << "\n  " << Color::BGREEN << "✓" << Color::RESET
                                      << " Reminder #" << id << " set for " << timeStr
                                      << " from now: " << Color::BOLD << msg << Color::RESET << "\n";
                        } else {
                            std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET
                                      << " /remind <time> <message>\n"
                                      << "  " << Color::DIM << "Example: /remind 5m check email"
                                      << Color::RESET << "\n";
                        }
                    } else {
                        std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET
                                  << " /remind <time> <message>\n"
                                  << "  " << Color::DIM << "Time: 30s, 5m, 2h, 1d"
                                  << Color::RESET << "\n";
                    }
                    commandFound = true;
                } else if (input == "/remind") {
                    std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET
                              << " /remind <time> <message>\n"
                              << "  " << Color::DIM << "Time formats: 30s, 5m, 2h, 1d"
                              << Color::RESET << "\n";
                    commandFound = true;
                } else if (input == "/schedule") {
                    auto pending = sys.taskScheduler.getDueTasks();
                    if (pending.empty()) {
                        std::cout << "\n  " << Color::DIM << "No scheduled tasks."
                                  << Color::RESET << "\n";
                    } else {
                        std::cout << "\n  " << Color::BOLD << Color::BCYAN
                                  << "  Scheduled Tasks:" << Color::RESET << "\n";
                        for (const auto& task : pending) {
                            std::time_t remaining = task.triggerTime - std::time(nullptr);
                            std::string timeLeft;
                            if (remaining < 60) timeLeft = std::to_string(remaining) + "s";
                            else if (remaining < 3600) timeLeft = std::to_string(remaining / 60) + "m";
                            else timeLeft = std::to_string(remaining / 3600) + "h";
                            std::cout << "    " << Color::CYAN << "#" << task.id << Color::RESET
                                      << " [" << timeLeft << "] " << task.message << "\n";
                        }
                    }
                    commandFound = true;
                } else if (input.size() > 6 && input.substr(0, 6) == "/read ") {
                    std::string filePath = trim(input.substr(6));
                    if (filePath.empty()) {
                        std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET
                                  << " /read <filepath>\n";
                    } else {
                        auto info = sys.fileReader.readFile(filePath);
                        std::string result = sys.fileReader.formatResult(info);
                        std::cout << "\n  " << Color::BOLD << Color::BCYAN << "📄 File Reader"
                                  << Color::RESET << "\n  " << result << "\n";
                    }
                    commandFound = true;
                } else if (input == "/read") {
                    std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET
                              << " /read <filepath>\n"
                              << "  " << Color::DIM << "Supports: .txt .md .cpp .py .json .csv .log .pdf"
                              << Color::RESET << "\n";
                    commandFound = true;
                } else if (input.size() > 5 && input.substr(0, 5) == "/run ") {
                    std::string args = trim(input.substr(5));
                    // Parse language and code
                    size_t spacePos = args.find(' ');
                    if (spacePos == std::string::npos) {
                        std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET
                                  << " /run python|bash|cpp <code>\n";
                    } else {
                        std::string lang = args.substr(0, spacePos);
                        std::string code = args.substr(spacePos + 1);
                        std::transform(lang.begin(), lang.end(), lang.begin(), ::tolower);

                        MKRunResult result;
                        if (lang == "python" || lang == "py") {
                            result = sys.codeRunner.runPython(code);
                            lang = "Python";
                        } else if (lang == "bash" || lang == "sh") {
                            // Sanitize bash commands for security
                            code = MKCrypto::sanitizeInput(code);
                            result = sys.codeRunner.runBash(code);
                            lang = "Bash";
                        } else if (lang == "cpp" || lang == "c++") {
                            result = sys.codeRunner.runCpp(code);
                            lang = "C++";
                        } else {
                            std::cout << "\n  " << Color::RED << "✗" << Color::RESET
                                      << " Unknown language: " << lang
                                      << ". Use: python, bash, or cpp\n";
                            commandFound = true;
                            continue;
                        }

                        std::cout << "\n  " << Color::BOLD << Color::BMAGENTA << "▶ " << lang
                                  << " Output:" << Color::RESET << "\n  ";
                        std::string formatted = sys.codeRunner.formatResult(result, lang);
                        // Indent output
                        for (char c : formatted) {
                            std::cout << c;
                            if (c == '\n') std::cout << "  ";
                        }
                        std::cout << "\n";
                    }
                    commandFound = true;
                } else if (input == "/run") {
                    std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET
                              << " /run <python|bash|cpp> <code>\n"
                              << "  " << Color::DIM << "Example: /run python print('hello')"
                              << Color::RESET << "\n";
                    commandFound = true;
                } else if (input.size() > 6 && input.substr(0, 6) == "/calc ") {
                    std::string expr = trim(input.substr(6));
                    if (expr.empty()) {
                        std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET
                                  << " /calc <expression>\n";
                    } else {
                        auto mathResult = sys.mathSolver.solve(expr);
                        if (mathResult.success) {
                            std::cout << "\n  " << Color::BGREEN << "🔢" << Color::RESET
                                      << " " << mathResult.answer << "\n";
                        } else {
                            std::cout << "\n  " << Color::RED << "✗" << Color::RESET
                                      << " " << mathResult.error << "\n";
                        }
                    }
                    commandFound = true;
                } else if (input == "/calc") {
                    std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET
                              << " /calc <expression>\n"
                              << "  " << Color::DIM << "Examples: /calc solve x^2+3x-4=0\n"
                              << "           /calc convert 5 miles to km\n"
                              << "           /calc what is 20% of 150\n"
                              << "           /calc sin 45"
                              << Color::RESET << "\n";
                    commandFound = true;
                } else if (input.size() > 7 && input.substr(0, 7) == "/image ") {
                    std::string imgPath = trim(input.substr(7));
                    if (imgPath.empty()) {
                        std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET
                                  << " /image <filepath>\n";
                    } else {
                        auto info = sys.imageAnalyzer.analyze(imgPath);
                        std::string result = sys.imageAnalyzer.formatResult(info);
                        std::cout << "\n  " << Color::BOLD << Color::BCYAN << "🖼 Image Analysis"
                                  << Color::RESET << "\n  " << result << "\n";
                    }
                    commandFound = true;
                } else if (input == "/image") {
                    std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET
                              << " /image <filepath>\n"
                              << "  " << Color::DIM << "Supports: PNG, JPEG, GIF, BMP"
                              << Color::RESET << "\n";
                    commandFound = true;
                } else if (input == "/plugins") {
                    std::string list = sys.pluginSystem.listPlugins();
                    std::cout << "\n  " << Color::BOLD << Color::BCYAN
                              << "  Loaded Plugins:" << Color::RESET << "\n" << list << "\n";
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

                // ─── /pc commands (Remote PC Control) ────────────────────
                if (!commandFound && input.size() >= 3 && input.substr(0, 3) == "/pc") {
                    std::string pcArgs = input.size() > 4 ? trim(input.substr(4)) : "";

                    if (pcArgs.empty() || pcArgs == "help") {
                        std::cout << "\n  " << Color::BOLD << Color::BCYAN
                                  << "  Remote PC Commands:" << Color::RESET << "\n"
                                  << "    " << Color::GREEN << "/pc run" << Color::RESET << " <cmd>     Run shell command\n"
                                  << "    " << Color::GREEN << "/pc open" << Color::RESET << " <app>    Open application\n"
                                  << "    " << Color::GREEN << "/pc read" << Color::RESET << " <path>   Read file\n"
                                  << "    " << Color::GREEN << "/pc write" << Color::RESET << " <path> <text> Write file\n"
                                  << "    " << Color::GREEN << "/pc info" << Color::RESET << "          System info\n"
                                  << "    " << Color::GREEN << "/pc processes" << Color::RESET << "     List processes\n"
                                  << "    " << Color::GREEN << "/pc kill" << Color::RESET << " <name>   Kill process\n"
                                  << "    " << Color::GREEN << "/pc files" << Color::RESET << " <dir>   List directory\n"
                                  << "    " << Color::GREEN << "/pc clipboard" << Color::RESET << "     Get clipboard\n"
                                  << "    " << Color::GREEN << "/pc screenshot" << Color::RESET << "    Take screenshot\n"
                                  << "    " << Color::GREEN << "/pc status" << Color::RESET << "        Connection status\n";
                        commandFound = true;
                    } else if (pcArgs == "status") {
                        if (sys.pcController.isConnected()) {
                            std::cout << "\n  " << Color::BGREEN << "●" << Color::RESET
                                      << " PC connected: " << sys.pcController.getIP()
                                      << ":" << sys.pcController.getPort() << "\n";
                        } else {
                            std::cout << "\n  " << Color::RED << "○" << Color::RESET
                                      << " PC not connected. Set MK_PC_IP, MK_PC_PORT, MK_PC_TOKEN.\n";
                        }
                        commandFound = true;
                    } else if (!sys.pcController.isConnected()) {
                        // Try to reconnect
                        if (!sys.pcController.connect()) {
                            std::cout << "\n  " << Color::RED << "✗" << Color::RESET
                                      << " PC agent unreachable. Is agent.py running?\n";
                            commandFound = true;
                        }
                    }

                    if (!commandFound && sys.pcController.isConnected()) {
                        std::string result;
                        if (pcArgs.substr(0, 4) == "run " && pcArgs.size() > 4) {
                            result = sys.pcController.runShell(trim(pcArgs.substr(4)));
                        } else if (pcArgs.substr(0, 5) == "open " && pcArgs.size() > 5) {
                            result = sys.pcController.openApp(trim(pcArgs.substr(5)));
                        } else if (pcArgs.substr(0, 5) == "read " && pcArgs.size() > 5) {
                            result = sys.pcController.readFile(trim(pcArgs.substr(5)));
                        } else if (pcArgs.substr(0, 6) == "write " && pcArgs.size() > 6) {
                            std::string rest = trim(pcArgs.substr(6));
                            size_t sp = rest.find(' ');
                            if (sp != std::string::npos) {
                                std::string path = rest.substr(0, sp);
                                std::string content = rest.substr(sp + 1);
                                result = sys.pcController.writeFile(path, content);
                            } else {
                                result = "Usage: /pc write <path> <content>";
                            }
                        } else if (pcArgs == "info") {
                            result = sys.pcController.getSystemInfo();
                        } else if (pcArgs == "processes") {
                            result = sys.pcController.listProcesses();
                        } else if (pcArgs.substr(0, 5) == "kill " && pcArgs.size() > 5) {
                            result = sys.pcController.killProcess(trim(pcArgs.substr(5)));
                        } else if (pcArgs.substr(0, 6) == "files " && pcArgs.size() > 6) {
                            result = sys.pcController.listFiles(trim(pcArgs.substr(6)));
                        } else if (pcArgs == "files") {
                            result = sys.pcController.listFiles(".");
                        } else if (pcArgs == "clipboard") {
                            result = sys.pcController.getClipboard();
                        } else if (pcArgs == "screenshot") {
                            result = sys.pcController.screenshot();
                        } else {
                            result = "Unknown /pc subcommand. Type /pc help.";
                        }

                        if (!result.empty()) {
                            std::cout << "\n  " << Color::BCYAN << "[PC]" << Color::RESET
                                      << " " << result;
                            if (result.back() != '\n') std::cout << "\n";
                        }
                        commandFound = true;
                    }
                }

                if (!commandFound) {
                    // Try plugins for unknown commands
                    std::string pluginCmd = input.substr(1); // Remove leading /
                    std::string pluginArgs;
                    size_t spPos = pluginCmd.find(' ');
                    if (spPos != std::string::npos) {
                        pluginArgs = pluginCmd.substr(spPos + 1);
                        pluginCmd = pluginCmd.substr(0, spPos);
                    }
                    std::string pluginResult = sys.pluginSystem.tryExecute(pluginCmd, pluginArgs);
                    if (!pluginResult.empty()) {
                        std::cout << "\n  " << Color::GREEN << "●" << Color::RESET
                                  << " " << pluginResult << "\n";
                        commandFound = true;
                    } else {
                        // Show suggestions for the partial command
                        show_slash_suggestions(input);
                        std::cout << "\n  " << Color::YELLOW << "⚠" << Color::RESET 
                                  << " Unknown command: " << Color::RED << input << Color::RESET
                                  << ". Type " << Color::GREEN << "/help" << Color::RESET << " for options.\n";
                    }
                }
            } // end of locked else block
        } else {
            // Natural language routing (acquire lock for thread safety)
            std::lock_guard<std::mutex> lock(sys.systemMutex);

            // Check for correction BEFORE normal routing
            if (sys.correctionEngine.detectCorrection(input)) {
                std::string response = sys.correctionEngine.applyCorrection(sys.graph, input);
                std::cout << "\n  " << Color::BGREEN << "✓" << Color::RESET << " " << response << "\n";
            } else if (sys.mathSolver.isMathQuery(input)) {
                // Detect math queries in natural language
                auto mathResult = sys.mathSolver.solve(input);
                if (mathResult.success) {
                    std::cout << "\n  " << Color::BGREEN << "🔢" << Color::RESET
                              << " " << mathResult.answer << "\n";
                } else {
                    handle_natural_query(sys, input);
                }
            } else {
                // Preprocess the input: clean slang, fix spelling, resolve pronouns
                auto ppResult = sys.preprocessor.process(input);
                std::string processedInput = ppResult.cleaned_text;

                // Show what MK understood (only if text was actually modified)
                if (ppResult.was_modified && processedInput != input) {
                    std::cout << "  " << Color::DIM << "(understood: " 
                              << processedInput << ")" << Color::RESET << "\n";
                }

                handle_natural_query(sys, processedInput);
            }
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

    // Stop the query server
    sys.queryServer.stop();

    // Signal and join the PC discovery thread
    if (pcDiscoveryRunning && pcDiscoveryThread.joinable()) {
        pcDiscoveryThread.join();
    }

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
    std::cout << "  " << Color::GREEN << "✓" << Color::RESET << " Memory saved. Improvement log saved. Knowledge persisted.\n";
    std::cout << "  " << Color::BOLD << Color::CYAN << "MK OS shut down cleanly. Goodbye." 
              << Color::RESET << "\n\n";

    return 0;
}

#endif // MK_ENTRY_CPP
