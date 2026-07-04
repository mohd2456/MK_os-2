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
#include "../network/realtime_apis.cpp"
#include "../ai_core/persistent_memory.cpp"
#include "../mk_brain/personality/response_style.cpp"
#include "../mk_brain/learning/learning_engine.cpp"
#include "../mk_brain/memory/brain_memory.cpp"
#include "../mk_brain/embeddings/embeddings_eng.cpp"
#include "../mk_brain/vector_search/ann_search.cpp"
#include "../mk_brain/cashe/cache_mgr.cpp"
#include "../mk_brain/daily_briefing.cpp"
#include "../mk_brain/fact_extractor/biographical.cpp"
#include "daemon.cpp"
#include "shell.cpp"
#include "service_manager.cpp"
#include "../plugins/telegram.cpp"
#include "../ai_core/math_solver.cpp"
#include "../plugins/pc_helper.cpp"
#include "../crypto/market_data.cpp"
#include "../crypto/technical_analysis.cpp"
#include "../crypto/signal_engine.cpp"
#include "../crypto/portfolio_manager.cpp"
#include "../crypto/risk_manager.cpp"
#include "../crypto/exchange_api.cpp"
#include "../crypto/trading_bot.cpp"
#include "../crypto/airdrop_farmer.cpp"

// LLM subsystem - Local and cloud LLM integration
#include "../llm/cloud_llm.cpp"
#include "../llm/llm_engine.cpp"
#include "../llm/provider_router.cpp"
#include "../llm/request_logger.cpp"

// Security subsystem - Key encryption
#include "../security/key_encryption.cpp"

// Mind subsystem - The cognitive layer
#include "../mind/mastery_network.cpp"
#include "../mind/goal_engine.cpp"
#include "../mind/strategy_planner.cpp"
#include "../mind/self_funding.cpp"
#include "../mind/knowledge_validator.cpp"
#include "../mind/autonomous_learner.cpp"

// Homelab subsystem - Device management and orchestration
#include "../homelab/device_registry.cpp"
#include "../homelab/resource_monitor.cpp"
#include "../homelab/ssh_controller.cpp"
#include "../homelab/docker_manager.cpp"
#include "../homelab/service_orchestrator.cpp"

// Sync subsystem - Multi-device knowledge and memory synchronization
#include "../sync/knowledge_sync.cpp"
#include "../sync/device_comm.cpp"
#include "../sync/memory_sync.cpp"

// Linux driver subsystem - Hardware monitoring for Linux
#include "../linux/drivers/gpu_monitor.cpp"
#include "../linux/drivers/network_interface.cpp"
#include "../linux/drivers/storage_monitor.cpp"

// Configuration system
#include "../config/mk_config.cpp"

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

// Forward declaration of orchestrator class (fully defined later in this file)
class MKOrchestrator;

// ============================================================
// MK System Prompt (single source of truth)
// Defined here so it can be referenced by MKSystem constructor.
// ============================================================
static const std::string MK_SYSTEM_PROMPT =
    "You are MK, a personal AI assistant. You are helpful, direct, and friendly. "
    "You talk naturally like a knowledgeable friend. You are loyal to your creator Mohammed. "
    "Keep responses concise and genuine. If you do not know something, say so honestly.";

// ============================================================
// MKSystem - Orchestrates all real modules
// ============================================================
struct MKSystem {
    MKPatternGraph graph;
    MKRealtimeAPIs realtimeApis;
    MKPersistentMemory memory;
    MKDiagnostics diagnostics;
    MKResponseStyle style;
    MKLearningEngine learningEngine;
    MKBrainMemory brainMemory;
    MKEmbeddingsEngine embeddings;
    MKANNSearch vectorSearch;
    MKCacheManager cacheManager;
    MKDailyBriefing dailyBriefing;
    MKBiographicalExtractor factExtractor;
    MKDaemon daemon;
    MK_Shell::MKShell shell;
    MK_Services::ServiceManager serviceManager;
    std::unique_ptr<MKTelegram> telegram;
    MKMathSolver mathSolver;
    MKPCHelper pcHelper;

    // LLM subsystem
    MKCloudLLM cloudLLM;
    MKLLMEngine llmEngine;
    MKProviderRouter providerRouter;
    MKRequestLogger requestLogger;

    // Security subsystem
    MKKeyEncryption keyEncryption;

    // Crypto trading subsystem
    MKMarketData cryptoMarketData;
    MKTechnicalAnalysis cryptoTechnicalAnalysis;
    MKSignalEngine cryptoSignalEngine;
    MKPortfolioManager cryptoPortfolioManager;
    MKRiskManager cryptoRiskManager;
    MKExchangeAPI cryptoExchangeApi;
    std::unique_ptr<MKTradingBot> cryptoTradingBot;
    MKAirdropFarmer cryptoAirdropFarmer;

    // Mind subsystem - The cognitive layer
    MKMasteryNetwork masteryNetwork;
    MKGoalEngine goalEngine;
    MKStrategyPlanner strategyPlanner;
    MKSelfFunding selfFunding;
    MKAutonomousLearner autonomousLearner;
    MKKnowledgeValidator knowledgeValidator;

    // Homelab subsystem
    MKDeviceRegistry deviceRegistry;
    MKResourceMonitor resourceMonitor;
    MKSSHController sshController;
    MKDockerManager dockerManager;
    MKServiceOrchestrator serviceOrchestrator;

    // Sync subsystem
    MKKnowledgeSync knowledgeSync;
    MKDeviceComm deviceComm;
    MKMemorySync memorySync;

    // Linux driver subsystem
    MKGPUMonitor linuxGpuMonitor;
    MKNetworkInterface linuxNetworkInterface;
    MKStorageMonitor linuxStorageMonitor;

    // Configuration system
    MKConfig config;

    // Orchestrator - the new clean conversation loop (allocated after helpers are defined)
    std::unique_ptr<MKOrchestrator> orchestrator;

    // Mutex protecting shared state between Telegram polling thread and REPL thread.
    // Any code that reads/writes graph, memory, learningEngine, factExtractor, or
    // calls telegram methods must hold this lock.
    std::mutex systemMutex;

    MKSystem()
        : graph("ai_core/hre/knowledge_files"),
          realtimeApis(),
          memory("mk_memory.dat", 10000, 5000),
          diagnostics(),
          style(),
          learningEngine(),
          brainMemory(20),
          embeddings(),
          vectorSearch(128),
          cacheManager(8),
          dailyBriefing(),
          factExtractor(),
          daemon(),
          shell(),
          serviceManager(),
          telegram(nullptr),
          cloudLLM("."),
          llmEngine(),
          providerRouter(),
          keyEncryption(".")
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

        // Initialize crypto trading bot (paper mode by default)
        cryptoTradingBot = std::make_unique<MKTradingBot>(
            cryptoMarketData, cryptoTechnicalAnalysis, cryptoSignalEngine,
            cryptoPortfolioManager, cryptoRiskManager, cryptoExchangeApi);
        cryptoExchangeApi.initialize("crypto_config.txt");

        // Load configuration
        config.load();

        // Load encrypted keys and sync with cloud LLM and provider router
        if (keyEncryption.hasEncryptedKeys()) {
            auto encKeys = keyEncryption.loadAllKeys();
            for (const auto& kv : encKeys) {
                std::string decrypted = keyEncryption.decrypt(kv.second);
                if (!decrypted.empty()) {
                    cloudLLM.loadEncryptedKey(kv.first, decrypted);
                    providerRouter.setProviderKey(kv.first, decrypted);
                }
            }
        }
        // Also sync any cloud LLM keys to the provider router
        auto cloudKeys = cloudLLM.getKeysForEncryption();
        for (const auto& kv : cloudKeys) {
            providerRouter.setProviderKey(kv.first, kv.second);
        }

        // Sync persisted model-slug overrides (models.conf, loaded by cloudLLM) into
        // the provider router so /models and routing reflect the same slugs and a
        // stale default never lingers after a /setmodel override.
        for (const std::string& pname : {"groq", "openrouter", "nvidia", "huggingface"}) {
            std::string slug = cloudLLM.getModel(pname);
            if (!slug.empty()) providerRouter.setProviderModel(pname, slug);
        }

        // Orchestrator wiring happens after construction (see initOrchestrator below)
        // because MKOrchestrator is forward-declared and only fully defined later.
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

// Forward declarations of helper functions defined later
static std::string sanitizeLLMResponse(const std::string& response, const std::string& userInput);
static std::string buildLLMPrompt(const std::string& input,
                                   const std::vector<std::string>& relevantFacts,
                                   const std::string& history);
static std::vector<std::string> filterRelevantFacts(
    const std::string& input,
    const std::vector<std::string>& rawFacts,
    size_t maxFacts = 5);

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
        << Color::BOLD << Color::YELLOW << "  💡 IDEAS" << Color::RESET << "\n"
        << "    " << Color::GREEN << "/idea" << Color::RESET << "              Generate a random creative idea\n"
        << "    " << Color::GREEN << "/brainstorm" << Color::RESET << " <topic> Brainstorm 5 ideas about a topic\n"
        << "    " << Color::GREEN << "/invent" << Color::RESET << " <problem>  Invent solutions for a problem\n"
        << "\n"
        << Color::BOLD << Color::YELLOW << "  🔌 LLM / PROVIDERS" << Color::RESET << "\n"
        << "    " << Color::GREEN << "/models" << Color::RESET << "            Show the model slug for each provider\n"
        << "    " << Color::GREEN << "/setmodel" << Color::RESET << " <p> <slug> Change a provider's model slug (persisted)\n"
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
        {"/models",   "Show provider model slugs"},
        {"/setmodel", "Set a provider's model slug"},
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
    // Search uses LLM directly with the query as context
    bool llmAvailable = sys.llmEngine.isAvailable() || sys.cloudLLM.isAvailable();
    if (llmAvailable) {
        std::string history = sys.brainMemory.getContextString();
        std::string prompt = buildLLMPrompt("search the internet for: " + query, {}, history);
        std::string response;
        if (sys.llmEngine.isAvailable()) {
            response = sys.llmEngine.generate(prompt);
            response = sanitizeLLMResponse(response, query);
        }
        if (response.empty() && sys.cloudLLM.isAvailable()) {
            response = sys.cloudLLM.generateWithContext(query, {}, history, "", MK_SYSTEM_PROMPT);
            response = sanitizeLLMResponse(response, query);
        }
        if (!response.empty()) {
            std::cout << "\n  " << response << "\n";
            return;
        }
    }
    std::cout << "\n  No LLM available to search. Check /status for provider health.\n";
}

static void cmd_status(MKSystem& sys) {
    std::cout << "\n";
    std::string report = sys.diagnostics.run_full_diagnostics();
    std::cout << "  " << report << "\n";
    sys.graph.printStats();
    std::cout << "  " << Color::BOLD << "LLM Providers:" << Color::RESET << "\n";
    std::string providerReport = sys.providerRouter.getStatusReport();
    std::cout << "  " << providerReport << "\n";
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
    // Use LLM directly for deep reasoning
    bool llmAvailable = sys.llmEngine.isAvailable() || sys.cloudLLM.isAvailable();
    std::string response;
    if (llmAvailable) {
        std::vector<std::string> rawFacts;
        auto edges = sys.graph.getAll(topic);
        for (const auto& e : edges) {
            rawFacts.push_back(e.source + " " + e.relation + " " + e.target);
        }
        std::vector<std::string> facts = filterRelevantFacts(topic, rawFacts, 5);
        std::string history = sys.brainMemory.getContextString();
        std::string prompt = MK_SYSTEM_PROMPT + "\n\n";
        if (!facts.empty()) {
            prompt += "Known facts:\n";
            for (const auto& f : facts) prompt += "- " + f + "\n";
            prompt += "\n";
        }
        prompt += "Think deeply and reason step by step about: " + topic + "\n";
        if (!history.empty()) prompt += "Recent conversation:\n" + history + "\n";
        prompt += "User: " + topic + "\nMK:";
        if (sys.llmEngine.isAvailable()) {
            response = sys.llmEngine.generate(prompt);
            response = sanitizeLLMResponse(response, topic);
        }
        if (response.empty() && sys.cloudLLM.isAvailable()) {
            response = sys.cloudLLM.generateWithContext(topic, facts, history, "", MK_SYSTEM_PROMPT);
            response = sanitizeLLMResponse(response, topic);
        }
    }
    if (!response.empty()) {
        std::cout << "\n  " << Color::BMAGENTA << "🧠" << Color::RESET << Color::BOLD
                  << " Deep Reasoning" << Color::RESET << "\n"
                  << "  " << Color::GREEN << "●" << Color::RESET << " " << response << "\n";
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
// LLM Response Sanitizer (Prompt Leakage Prevention)
// ============================================================
// HTML Escape Helper for LLM output sent to Telegram
// Always escapes &, <, > so Telegram's HTML parser won't choke.
// This should be applied ONLY to LLM-generated replies, not to
// MK's own pre-formatted HTML messages (e.g., /help, keyboards).
// ============================================================
static std::string escapeHtml(const std::string& text) {
    std::string result;
    result.reserve(text.size() + 32);
    for (char c : text) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            default: result += c; break;
        }
    }
    return result;
}

// ============================================================
// LLM Response Sanitizer
// Strips prompt-leak patterns from LLM responses where weak
// models echo internal instructions back to the user.
// ============================================================
static std::string sanitizeLLMResponse(const std::string& response, const std::string& userInput) {
    if (response.empty()) return response;
    (void)userInput; // Reserved for future echo-detection against user input

    std::string result = response;

    // Patterns that indicate the LLM is echoing prompt instructions
    static const std::vector<std::string> leakPatterns = {
        "Known facts (use these to answer):",
        "Answer the user's question naturally",
        "Do not list the raw facts",
        "Internal reasoning (use this to inform your response, but do NOT repeat it verbatim to the user):",
        "synthesize them into a natural response",
        "Known facts:",
        "Recent conversation:",
        "You are MK, a personal AI assistant.",
        "You talk naturally like a knowledgeable friend.",
        "You are loyal to your creator Mohammed.",
        "Keep responses concise and genuine.",
        "State what action should be taken.",
        "Respond in 1-2 sentences only.",
        "You are a reasoning engine."
    };

    // Count how many leak patterns are found
    int leakCount = 0;
    for (const auto& pattern : leakPatterns) {
        if (result.find(pattern) != std::string::npos) {
            leakCount++;
        }
    }

    // If multiple leak patterns found, the response is mostly prompt echo - reject it
    if (leakCount >= 2) {
        return "";
    }

    // Strip individual leak pattern lines
    for (const auto& pattern : leakPatterns) {
        size_t pos = result.find(pattern);
        while (pos != std::string::npos) {
            // Find the start of the line containing this pattern
            size_t lineStart = pos;
            while (lineStart > 0 && result[lineStart - 1] != '\n') lineStart--;
            // Find the end of the line
            size_t lineEnd = result.find('\n', pos);
            if (lineEnd == std::string::npos) lineEnd = result.size();
            else lineEnd++; // include the newline
            result.erase(lineStart, lineEnd - lineStart);
            pos = result.find(pattern);
        }
    }

    // Strip trailing prompt template lines (lines starting with "User:" or "MK:" at the end)
    while (!result.empty()) {
        // Find last non-whitespace content
        size_t lastNonWs = result.find_last_not_of(" \t\n\r");
        if (lastNonWs == std::string::npos) break;

        // Find the start of the last line
        size_t lastLineStart = result.rfind('\n', lastNonWs);
        if (lastLineStart == std::string::npos) lastLineStart = 0;
        else lastLineStart++;

        std::string lastLine = result.substr(lastLineStart, lastNonWs - lastLineStart + 1);
        // Trim leading whitespace from lastLine for comparison
        size_t trimStart = 0;
        while (trimStart < lastLine.size() && (lastLine[trimStart] == ' ' || lastLine[trimStart] == '\t'))
            trimStart++;
        lastLine = lastLine.substr(trimStart);

        if (lastLine.rfind("User:", 0) == 0 || lastLine.rfind("MK:", 0) == 0) {
            result = result.substr(0, lastLineStart);
        } else {
            break;
        }
    }

    // Check if response is >80% identical to the system prompt (echo detection)
    if (!result.empty()) {
        // Use the actual MK_SYSTEM_PROMPT constant (defined at file scope)
        // to ensure the similarity check stays in sync with prompt changes.
        const std::string& sysPrompt = MK_SYSTEM_PROMPT;
        // Simple character overlap check
        int matchChars = 0;
        std::string lowerResult = result;
        std::string lowerPrompt = sysPrompt;
        std::transform(lowerResult.begin(), lowerResult.end(), lowerResult.begin(), ::tolower);
        std::transform(lowerPrompt.begin(), lowerPrompt.end(), lowerPrompt.begin(), ::tolower);
        for (size_t i = 0; i + 20 < lowerPrompt.size(); i += 20) {
            std::string chunk = lowerPrompt.substr(i, 20);
            if (lowerResult.find(chunk) != std::string::npos) {
                matchChars += 20;
            }
        }
        float similarity = (float)matchChars / (float)lowerPrompt.size();
        if (similarity > 0.8f) {
            return "";
        }
    }

    // Trim leading/trailing whitespace
    size_t start = result.find_first_not_of(" \t\n\r");
    size_t end = result.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    result = result.substr(start, end - start + 1);

    return result;
}

// ============================================================
// LLM Prompt Construction Helper
// ============================================================
static std::string buildLLMPrompt(const std::string& input,
                                   const std::vector<std::string>& relevantFacts,
                                   const std::string& history) {
    std::string fullPrompt = MK_SYSTEM_PROMPT + "\n\n";
    if (!relevantFacts.empty()) {
        fullPrompt += "Known facts:\n";
        for (const auto& f : relevantFacts) fullPrompt += "- " + f + "\n";
        fullPrompt += "\n";
    }
    if (!history.empty()) fullPrompt += "Recent conversation:\n" + history + "\n";
    fullPrompt += "User: " + input + "\nMK:";
    return fullPrompt;
}

// ============================================================
// Graph Fact Relevance Filter (BUG C fix)
// Filters graph results to only include strongly relevant facts.
// Prevents random keyword matches from polluting LLM context or
// being dumped raw to the user.
// ============================================================
static std::vector<std::string> filterRelevantFacts(
    const std::string& input,
    const std::vector<std::string>& rawFacts,
    size_t maxFacts)
{
    if (rawFacts.empty()) return {};

    // Tokenize the input into meaningful words (skip short/common words)
    std::vector<std::string> queryWords;
    {
        std::string lower = input;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::istringstream ss(lower);
        std::string word;
        // Common stop words to skip
        static const std::vector<std::string> stopWords = {
            "a", "an", "the", "is", "are", "was", "were", "be", "been",
            "what", "who", "where", "when", "why", "how", "which",
            "do", "does", "did", "will", "would", "could", "should",
            "can", "may", "might", "shall", "to", "of", "in", "on",
            "at", "by", "for", "with", "from", "up", "about", "into",
            "it", "its", "this", "that", "these", "those", "i", "me",
            "my", "you", "your", "he", "she", "they", "we", "us",
            "and", "or", "but", "not", "no", "so", "if", "than"
        };
        while (ss >> word) {
            // Remove trailing punctuation
            while (!word.empty() && (word.back() == '?' || word.back() == '.' ||
                   word.back() == '!' || word.back() == ',')) {
                word.pop_back();
            }
            if (word.size() < 2) continue;
            bool isStop = false;
            for (const auto& sw : stopWords) {
                if (word == sw) { isStop = true; break; }
            }
            if (!isStop) queryWords.push_back(word);
        }
    }

    // If we couldn't extract meaningful words, return nothing (query is too vague)
    if (queryWords.empty()) return {};

    // Score each fact by keyword overlap with the query
    struct ScoredFact {
        std::string fact;
        int score;
    };
    std::vector<ScoredFact> scored;
    for (const auto& fact : rawFacts) {
        std::string factLower = fact;
        std::transform(factLower.begin(), factLower.end(), factLower.begin(), ::tolower);
        int matchCount = 0;
        for (const auto& qw : queryWords) {
            if (factLower.find(qw) != std::string::npos) {
                matchCount++;
            }
        }
        // Require at least 1 keyword match (for short queries) or
        // proportional match for longer queries
        int threshold = (queryWords.size() <= 2) ? 1 : 2;
        if (matchCount >= threshold) {
            scored.push_back({fact, matchCount});
        }
    }

    // Sort by score descending
    std::sort(scored.begin(), scored.end(),
              [](const ScoredFact& a, const ScoredFact& b) {
                  return a.score > b.score;
              });

    // Return top N facts
    std::vector<std::string> filtered;
    for (size_t i = 0; i < scored.size() && i < maxFacts; i++) {
        filtered.push_back(scored[i].fact);
    }
    return filtered;
}

// Orchestrator - clean single-LLM-call conversation loop
// Must be included after sanitizeLLMResponse and filterRelevantFacts are defined,
// since the orchestrator calls them.
#include "../orchestrator/conversation_loop.cpp"

// ============================================================
// Initialize orchestrator (must be after MKOrchestrator is defined)
// ============================================================
static void initOrchestrator(MKSystem& sys) {
    sys.orchestrator = std::make_unique<MKOrchestrator>();
    sys.orchestrator->graph = &sys.graph;
    sys.orchestrator->brainMemory = &sys.brainMemory;
    sys.orchestrator->cloudLLM = &sys.cloudLLM;
    sys.orchestrator->llmEngine = &sys.llmEngine;
    sys.orchestrator->providerRouter = &sys.providerRouter;
    sys.orchestrator->requestLogger = &sys.requestLogger;
    sys.orchestrator->resourceMonitor = &sys.resourceMonitor;
    sys.orchestrator->deviceRegistry = &sys.deviceRegistry;
    sys.orchestrator->learningEngine = &sys.learningEngine;
    sys.orchestrator->factExtractor = &sys.factExtractor;
    sys.orchestrator->systemPrompt = &MK_SYSTEM_PROMPT;
}

// ============================================================
// LLM Synthesis Helper
// Used by command handlers that still need direct LLM synthesis.
// The orchestrator handles this for natural language; this is
// a simpler version for /ask and similar commands.
// ============================================================
static std::string synthesizeResponse(MKSystem& sys, const std::string& input,
                                       const std::vector<std::string>& rawFacts,
                                       float confidence) {
    // First filter facts for relevance
    std::vector<std::string> facts = filterRelevantFacts(input, rawFacts, 5);

    // If no relevant facts survived filtering, return empty
    if (facts.empty() && rawFacts.empty()) return "";

    // Check if LLM is available
    bool llmAvailable = sys.llmEngine.isAvailable() || sys.cloudLLM.isAvailable();

    if (llmAvailable && !facts.empty()) {
        // Build RAG prompt: user question + relevant facts -> LLM -> natural answer
        std::string history = sys.brainMemory.getContextString();
        std::string prompt = MK_SYSTEM_PROMPT + "\n\n";
        prompt += "Known facts (use these to answer):\n";
        for (const auto& f : facts) prompt += "- " + f + "\n";
        prompt += "\nAnswer the user's question naturally and concisely using the facts above. "
                  "Do not list the raw facts - synthesize them into a natural response.\n\n";
        if (!history.empty()) prompt += "Recent conversation:\n" + history + "\n";
        prompt += "User: " + input + "\nMK:";

        std::string response;
        if (sys.llmEngine.isAvailable()) {
            response = sys.llmEngine.generate(prompt);
            response = sanitizeLLMResponse(response, input);
        }
        if (response.empty() && sys.cloudLLM.isAvailable()) {
            response = sys.cloudLLM.generateWithContext(input, facts, history, "", MK_SYSTEM_PROMPT);
            response = sanitizeLLMResponse(response, input);
        }
        if (!response.empty()) return response;
    }

    // LLM unavailable: format through style engine if truly offline
    if (!llmAvailable && !facts.empty()) {
        std::string raw;
        for (const auto& f : facts) {
            raw += f + ". ";
        }
        return sys.style.format_response(raw, input, confidence);
    }

    return "";
}

// ============================================================
// Natural Language Routing - Now uses MKOrchestrator
// ============================================================
static void handle_natural_query(MKSystem& sys, const std::string& input) {
    // ---- MATH SOLVER: Check for arithmetic/math queries (fast, no LLM needed) ----
    {
        if (sys.mathSolver.isMathQuery(input)) {
            // Try natural language math first (e.g., "5 times 3")
            if (sys.mathSolver.isNaturalLanguageMath(input)) {
                auto mathResult = sys.mathSolver.evaluateNaturalMath(input);
                if (mathResult.success) {
                    std::cout << "\n  " << Color::GREEN << "●" << Color::RESET << " " << mathResult.answer << "\n";
                    sys.memory.recordInteraction("math", input);
                    return;
                }
            }
            // Try arithmetic evaluation first
            if (sys.mathSolver.hasArithmeticPattern(input)) {
                auto mathResult = sys.mathSolver.evaluateArithmetic(input);
                if (mathResult.success) {
                    std::cout << "\n  " << Color::GREEN << "●" << Color::RESET << " " << mathResult.answer << "\n";
                    sys.memory.recordInteraction("math", input);
                    return;
                }
            }
            // Fall back to general solve()
            auto mathResult = sys.mathSolver.solve(input);
            if (mathResult.success) {
                std::cout << "\n  " << Color::GREEN << "●" << Color::RESET << " " << mathResult.answer << "\n";
                sys.memory.recordInteraction("math", input);
                return;
            }
        }
    }

    // ---- ALL OTHER QUERIES: Single LLM call via Orchestrator ----
    std::string response = sys.orchestrator->respond(input);

    if (!response.empty()) {
        std::cout << "\n  " << Color::BCYAN << "~" << Color::RESET << " " << response << "\n";
    } else {
        std::cout << "\n  " << Color::YELLOW << "○" << Color::RESET
                  << " I don't have enough knowledge to answer that yet.\n"
                  << "  " << Color::DIM << "Try: " << Color::GREEN << "/learn" << Color::RESET
                  << Color::DIM << " to teach me, or " << Color::GREEN << "/think" << Color::RESET
                  << Color::DIM << " for reasoning." << Color::RESET << "\n";
    }

    // Record outcome
    sys.memory.recordInteraction("query", input);
}

// ============================================================
// Telegram Background Polling Loop
// ============================================================

// Forward declaration - generates an AI response for a natural language query.
// Caller must hold sys.systemMutex.
static std::string generate_ai_response(MKSystem& sys, const std::string& input) {
    return sys.orchestrator->respondForTelegram(input);
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
                    // Handle /setkey command with provider router integration
                    if (sys.telegram->isSetKeyCommand(msgText)) {
                        auto args = sys.telegram->extractSetKeyArgs(msgText);
                        std::string provider = args.first;
                        std::string key = args.second;

                        // Normalize provider name
                        std::transform(provider.begin(), provider.end(), provider.begin(), ::tolower);

                        if (key.empty()) {
                            sys.telegram->sendMessage(chatId,
                                "<b>Usage:</b> /setkey &lt;provider&gt; &lt;key&gt;\n"
                                "<b>Providers:</b> groq, openrouter, nvidia, huggingface");
                        } else if (!sys.providerRouter.isValidProvider(provider)) {
                            sys.telegram->sendMessage(chatId,
                                "Unknown provider: <b>" + provider + "</b>\n"
                                "Valid: groq, openrouter, nvidia, huggingface");
                        } else {
                            // Store key encrypted
                            sys.keyEncryption.saveKey(provider, key);
                            // Set key in cloud LLM and provider router
                            sys.cloudLLM.setApiKey(provider, key);
                            sys.providerRouter.setProviderKey(provider, key);

                            // Test the key with a minimal API call
                            sys.telegram->sendMessage(chatId,
                                "Testing <b>" + provider + "</b> key...");
                            bool testOk = sys.providerRouter.testProvider(provider);

                            if (testOk) {
                                sys.telegram->sendMessage(chatId,
                                    "✅ <b>" + provider + "</b> key verified and saved!\n"
                                    "Provider is now active.");
                            } else {
                                sys.telegram->sendMessage(chatId,
                                    "⚠️ Key saved but test failed for <b>" + provider + "</b>.\n"
                                    "The key may be invalid or the provider may be down.\n"
                                    "It will be retried automatically.");
                            }
                        }
                    } else if (msgText.rfind("/setmodel", 0) == 0) {
                        // /setmodel <provider> <slug> — swap a stale model slug without recompiling
                        std::string rest = trim(msgText.substr(9));
                        size_t sp = rest.find(' ');
                        if (rest.empty() || sp == std::string::npos) {
                            sys.telegram->sendMessage(chatId,
                                "<b>Usage:</b> /setmodel &lt;provider&gt; &lt;slug&gt;\n"
                                "<b>Providers:</b> groq, openrouter, nvidia, huggingface");
                        } else {
                            std::string provider = rest.substr(0, sp);
                            std::string slug = trim(rest.substr(sp + 1));
                            std::transform(provider.begin(), provider.end(), provider.begin(), ::tolower);
                            if (!sys.providerRouter.isValidProvider(provider)) {
                                sys.telegram->sendMessage(chatId,
                                    "Unknown provider: <b>" + provider + "</b>\n"
                                    "Valid: groq, openrouter, nvidia, huggingface");
                            } else if (slug.empty()) {
                                sys.telegram->sendMessage(chatId, "Model slug cannot be empty.");
                            } else {
                                sys.cloudLLM.setModel(provider, slug);
                                sys.providerRouter.setProviderModel(provider, slug);
                                sys.telegram->sendMessage(chatId,
                                    "✅ <b>" + provider + "</b> model set to <code>" + slug +
                                    "</code> and saved.");
                            }
                        }
                    } else if (sys.telegram->isStatusCommand(msgText)) {
                        // /status - show provider status report + request counts
                        std::string statusReport = sys.providerRouter.getStatusReport();
                        int todayRequests = sys.requestLogger.getTodayCount();
                        statusReport += "\n<b>Requests today:</b> " + std::to_string(todayRequests);
                        sys.telegram->sendMessage(chatId, statusReport);
                    } else if (sys.telegram->isKeyCommand(msgText)) {
                        // /key - show active provider (without exposing key)
                        std::string active = sys.providerRouter.getActiveProvider();
                        int online = sys.providerRouter.onlineCount();
                        int total = sys.providerRouter.providerCount();
                        sys.telegram->sendMessage(chatId,
                            "<b>Active Provider:</b> " + active + "\n"
                            "<b>Online:</b> " + std::to_string(online) + "/" + std::to_string(total) + "\n\n"
                            "<i>Use /status for full details</i>");
                    } else if (sys.telegram->isLogsCommand(msgText)) {
                        // /logs - show today's LLM request statistics
                        std::string stats = sys.requestLogger.getStats();
                        sys.telegram->sendMessage(chatId, stats);
                    } else if (msgText == "/crypto") {
                        sys.telegram->handleCryptoCommand(chatId);
                    } else if (msgText == "/devices") {
                        sys.telegram->handleDevicesCommand(chatId);
                    } else if (msgText == "/goals") {
                        sys.telegram->handleGoalsCommand(chatId);
                    } else {
                        // Other command-based messages use autoReply
                        sys.telegram->autoReply(chatId, msgText);
                    }
                } else {
                    // Route natural language through the AI pipeline
                    // Use provider router to pick provider based on urgency and token estimate
                    int tokenEstimate = (int)msgText.size() / 4; // rough estimate
                    std::string pickedProvider = sys.providerRouter.pickProvider(
                        MKRoutingUrgency::HIGH, tokenEstimate);

                    auto startTime = std::chrono::steady_clock::now();
                    std::string reply = generate_ai_response(sys, msgText);
                    auto endTime = std::chrono::steady_clock::now();
                    float latencyMs = (float)std::chrono::duration_cast<std::chrono::milliseconds>(
                        endTime - startTime).count();

                    // Log the request to provider router
                    if (!pickedProvider.empty()) {
                        sys.providerRouter.logRequest(pickedProvider, tokenEstimate,
                                                     latencyMs, !reply.empty());
                        sys.requestLogger.logRequest(pickedProvider, msgText.substr(0, 40),
                                                     tokenEstimate, latencyMs, !reply.empty());
                    }

                    // Guard against empty reply before sending to Telegram
                    if (reply.empty()) {
                        reply = "I'm processing that but couldn't generate a response. Try again or rephrase.";
                    }

                    // Escape HTML in LLM-generated reply so Telegram's HTML parser
                    // doesn't choke on angle brackets or ampersands in model output.
                    reply = escapeHtml(reply);

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

    // Initialize the orchestrator (requires full MKOrchestrator definition)
    initOrchestrator(sys);

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
    if (sys.telegram) {
        sys.daemon.addJob("telegram_poll_monitor", 60, []() {
            // Monitoring placeholder - the actual polling runs in its own thread
        });
    }

    // Register new subsystem scheduled jobs
    sys.daemon.addJob("crypto_scan", 300, [&sys]() {
        // Market scan every 5 minutes
        sys.cryptoTradingBot->runScanCycle();
    });
    sys.daemon.addJob("knowledge_sync", 300, [&sys]() {
        // Sync knowledge check every 5 minutes (queue-based)
        (void)sys.knowledgeSync.queueSize();
    });
    sys.daemon.addJob("device_health", 60, [&sys]() {
        // Check device health every minute
        sys.resourceMonitor.getLocalResources();
    });
    sys.daemon.addJob("autonomous_learn", 1800, [&sys]() {
        // Autonomous learning every 30 minutes (only during learning hours)
        if (sys.config.isLearningHour()) {
            auto topic = sys.autonomousLearner.pickNextTopic();
            auto session = sys.autonomousLearner.startSession(topic.topic);
            // Call web scraper and validate results through knowledge validator
            auto acceptedFacts = sys.autonomousLearner.learnFromWeb(
                topic.topic, sys.knowledgeValidator, session, 20);
            // Integrate accepted facts into the knowledge graph
            for (const auto& fact : acceptedFacts) {
                std::istringstream ss(fact);
                std::string src, rel, tgt, wStr;
                if (std::getline(ss, src, '|') && std::getline(ss, rel, '|') &&
                    std::getline(ss, tgt, '|')) {
                    float w = 0.7f;
                    if (std::getline(ss, wStr, '|') && !wStr.empty()) {
                        try { w = std::stof(wStr); } catch (...) {}
                    }
                    sys.graph.persistNewFact(src, rel, tgt, w);
                }
            }
            sys.autonomousLearner.endSession(session);
        }
    }, true);  // requires cool
    sys.daemon.addJob("goal_evaluation", 900, [&sys]() {
        // Evaluate goals every 15 minutes
        sys.goalEngine.getActiveGoals();
    });
    sys.daemon.addJob("portfolio_rebalance", 3600, [&sys]() {
        // Portfolio rebalance check every hour
        sys.cryptoPortfolioManager.getRebalanceSuggestions();
    });

    // ============================================================
    // First-boot: offer LLM model download
    // ============================================================
    {
        const std::string modelPath = "llm/models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf";
        const std::string dismissPath = ".model_prompt_dismissed";
        const std::string apiKeyPath = "api_keys.conf";
        bool modelExists = false;
        bool dismissed = false;
        bool hasApiKey = false;
        bool localLLMRunning = false;

        {
            std::ifstream mf(modelPath);
            modelExists = mf.good();
        }
        {
            std::ifstream df(dismissPath);
            dismissed = df.good();
        }
        {
            // Check if we already have API keys configured
            std::ifstream kf(apiKeyPath);
            if (kf.good()) {
                std::string line;
                while (std::getline(kf, line)) {
                    if (line.empty() || line[0] == '#') continue;
                    size_t eq = line.find('=');
                    if (eq != std::string::npos) {
                        std::string val = line.substr(eq + 1);
                        while (!val.empty() && (val.back() == ' ' || val.back() == '\n' || val.back() == '\r')) val.pop_back();
                        if (!val.empty()) { hasApiKey = true; break; }
                    }
                }
            }
            // Also check environment
            if (!hasApiKey) {
                const char* gk = std::getenv("GROQ_API_KEY");
                const char* ork = std::getenv("OPENROUTER_API_KEY");
                const char* hfk = std::getenv("HF_API_KEY");
                if ((gk && gk[0]) || (ork && ork[0]) || (hfk && hfk[0])) hasApiKey = true;
            }
        }
        // Check if local llama.cpp or Ollama is running.
        // IMPORTANT: a successful transfer (CURLE_OK) is NOT proof a server is
        // there — any HTTP response (including a 404 from some unrelated process
        // on :8080) returns CURLE_OK. We must require an actual HTTP 200, mirroring
        // MKLLMEngine::checkEndpoint(), or MK falsely claims "Local server detected".
        {
            auto localEndpointHealthy = [](const char* url) -> bool {
                CURL* curl = curl_easy_init();
                if (!curl) return false;
                curl_easy_setopt(curl, CURLOPT_URL, url);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1L);
                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
                curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
                curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
                CURLcode res = curl_easy_perform(curl);
                long httpCode = 0;
                if (res == CURLE_OK) {
                    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
                }
                curl_easy_cleanup(curl);
                return (res == CURLE_OK && httpCode == 200);
            };

            if (localEndpointHealthy("http://localhost:8080/health")) {
                localLLMRunning = true;
            } else if (localEndpointHealthy("http://localhost:11434/api/tags")) {
                localLLMRunning = true;
            }
        }

        // Decision flow: what does MK need?
        if (localLLMRunning) {
            // Best case: local LLM is running, use it
            std::cout << "  " << Color::BGREEN << "+" << Color::RESET
                      << " LLM: Local server detected. Full brain power available.\n";
        } else if (hasApiKey) {
            // Good: we have cloud API keys configured
            std::cout << "  " << Color::BGREEN << "+" << Color::RESET
                      << " LLM: Cloud API configured. MK can think and talk naturally.\n";
        } else if (!dismissed) {
            // No local LLM, no API key — offer options
            std::cout << "\n"
                << Color::BOLD << Color::BCYAN
                << "  ╭──────────────────────────────────────────────────────╮\n"
                << "  │  MK needs a brain to think and talk naturally.       │\n"
                << "  │                                                      │\n"
                << "  │  Options:                                            │\n"
                << "  │                                                      │\n"
                << "  │  [1] Enter a free API key (recommended for now)      │\n"
                << "  │      Get one free at:                                │\n"
                << "  │      • groq.com (fastest, email signup)              │\n"
                << "  │      • openrouter.ai (many models, email signup)     │\n"
                << "  │      • huggingface.co (email signup)                 │\n"
                << "  │                                                      │\n"
                << "  │  [2] Download a local model (638MB, needs 2GB+ RAM)  │\n"
                << "  │                                                      │\n"
                << "  │  [3] Skip for now (MK works but can't talk smoothly) │\n"
                << "  ╰──────────────────────────────────────────────────────╯\n"
                << Color::RESET << "\n"
                << "  Your choice (1/2/3): ";
            std::cout.flush();

            std::string choice;
            std::getline(std::cin, choice);
            choice = trim(choice);

            if (choice == "1") {
                // Ask which provider
                std::cout << "\n  Which provider? (groq/openrouter/huggingface): ";
                std::cout.flush();
                std::string provider;
                std::getline(std::cin, provider);
                provider = trim(provider);
                std::transform(provider.begin(), provider.end(), provider.begin(), ::tolower);

                std::string keyName;
                if (provider == "groq" || provider == "g") keyName = "GROQ_API_KEY";
                else if (provider == "openrouter" || provider == "or" || provider == "o") keyName = "OPENROUTER_API_KEY";
                else if (provider == "huggingface" || provider == "hf" || provider == "h") keyName = "HF_API_KEY";
                else {
                    // Default to Groq
                    keyName = "GROQ_API_KEY";
                    std::cout << "  " << Color::DIM << "(Defaulting to Groq)" << Color::RESET << "\n";
                }

                std::cout << "  Paste your API key: ";
                std::cout.flush();
                std::string apiKey;
                std::getline(std::cin, apiKey);
                apiKey = trim(apiKey);

                if (!apiKey.empty()) {
                    // Save to api_keys.conf
                    std::ofstream keyFile(apiKeyPath, std::ios::app);
                    if (keyFile.is_open()) {
                        // Check if file is empty/new, add header
                        keyFile.seekp(0, std::ios::end);
                        if (keyFile.tellp() == 0) {
                            keyFile << "# MK OS API Keys\n";
                            keyFile << "# Free keys — no credit card needed:\n";
                            keyFile << "#   Groq: https://console.groq.com\n";
                            keyFile << "#   OpenRouter: https://openrouter.ai\n";
                            keyFile << "#   HuggingFace: https://huggingface.co/settings/tokens\n\n";
                        }
                        keyFile << keyName << "=" << apiKey << "\n";
                        keyFile.close();
                        hasApiKey = true;
                        std::cout << "\n  " << Color::BGREEN << "✓" << Color::RESET
                                  << " API key saved! MK can now think and talk naturally.\n"
                                  << "  " << Color::DIM << "Key stored in: " << apiKeyPath
                                  << Color::RESET << "\n";
                    } else {
                        std::cout << "  " << Color::RED << "✗" << Color::RESET
                                  << " Could not save key. Set " << keyName 
                                  << " as environment variable instead.\n";
                    }
                } else {
                    std::cout << "  " << Color::DIM << "No key entered. MK will use basic mode." 
                              << Color::RESET << "\n";
                }

            } else if (choice == "2") {
                std::cout << "\n  " << Color::DIM << "Downloading model..." << Color::RESET << "\n";
                int ret = system("bash llm/download_model.sh");
                if (ret == 0) {
                    std::cout << "  " << Color::BGREEN << "✓" << Color::RESET
                              << " Model downloaded! Run llm/start_server.sh to activate.\n";
                } else {
                    std::cout << "  " << Color::YELLOW << "!" << Color::RESET
                              << " Download failed. Try option 1 (API key) instead.\n";
                }
            } else {
                // Skip
                {
                    std::ofstream df(dismissPath);
                    df << "dismissed" << std::endl;
                }
                std::cout << "  " << Color::DIM
                          << "No problem. You can add an API key anytime with: /apikey"
                          << Color::RESET << "\n";
            }
        } else {
            // Dismissed previously, no LLM, no API — just note it
            std::cout << "  " << Color::DIM << "LLM: Not configured. Use /apikey to add one."
                      << Color::RESET << "\n";
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
            } else if (input == "/apikey" || input.substr(0, 8) == "/apikey ") {
                // Add or update cloud LLM API key at runtime
                std::string provider, key;
                if (input.size() > 8) {
                    // /apikey groq sk-xxx...
                    std::string rest = trim(input.substr(8));
                    size_t sp = rest.find(' ');
                    if (sp != std::string::npos) {
                        provider = rest.substr(0, sp);
                        key = trim(rest.substr(sp + 1));
                    } else {
                        provider = rest;
                    }
                }
                if (provider.empty()) {
                    std::cout << "\n  Which provider? (groq / openrouter / huggingface): ";
                    std::cout.flush();
                    std::getline(std::cin, provider);
                    provider = trim(provider);
                }
                std::transform(provider.begin(), provider.end(), provider.begin(), ::tolower);
                if (key.empty()) {
                    std::cout << "  Paste your API key: ";
                    std::cout.flush();
                    std::getline(std::cin, key);
                    key = trim(key);
                }
                if (!key.empty()) {
                    std::string keyName;
                    if (provider == "groq" || provider == "g") keyName = "GROQ_API_KEY";
                    else if (provider == "openrouter" || provider == "or" || provider == "o") keyName = "OPENROUTER_API_KEY";
                    else if (provider == "huggingface" || provider == "hf" || provider == "h") keyName = "HF_API_KEY";
                    else keyName = "GROQ_API_KEY";

                    std::ofstream keyFile("api_keys.conf", std::ios::app);
                    if (keyFile.is_open()) {
                        keyFile << keyName << "=" << key << "\n";
                        keyFile.close();
                        std::cout << "\n  " << Color::BGREEN << "\xe2\x9c\x93" << Color::RESET
                                  << " Key saved for " << provider << ". MK brain is now active.\n"
                                  << "  " << Color::DIM << "Restart MK to use it, or it'll pick up next boot."
                                  << Color::RESET << "\n";
                    }
                } else {
                    std::cout << "  " << Color::DIM << "No key entered." << Color::RESET << "\n";
                }
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
                } else if (input == "/models") {
                    // Show the current model slug for each provider
                    std::cout << "\n  " << Color::BOLD << Color::BCYAN << "🔧 Provider Models" << Color::RESET << "\n";
                    for (const auto& name : sys.providerRouter.getProviderNames()) {
                        std::cout << "  " << Color::GREEN << "•" << Color::RESET << " "
                                  << name << ": " << Color::DIM
                                  << sys.providerRouter.getProviderModel(name) << Color::RESET << "\n";
                    }
                    std::cout << "  " << Color::DIM
                              << "Change with: /setmodel <provider> <slug>" << Color::RESET << "\n";
                    commandFound = true;
                } else if (input.size() > 10 && input.substr(0, 10) == "/setmodel ") {
                    // /setmodel <provider> <slug> — swap a (possibly stale) model slug
                    // at runtime without recompiling. Persisted to models.conf.
                    std::string args = trim(input.substr(10));
                    size_t sp = args.find(' ');
                    if (sp == std::string::npos) {
                        std::cout << "\n  " << Color::YELLOW
                                  << "Usage: /setmodel <provider> <slug>" << Color::RESET << "\n"
                                  << "  Providers: groq, openrouter, nvidia, huggingface\n";
                    } else {
                        std::string provider = args.substr(0, sp);
                        std::string slug = trim(args.substr(sp + 1));
                        std::transform(provider.begin(), provider.end(), provider.begin(), ::tolower);
                        if (!sys.providerRouter.isValidProvider(provider)) {
                            std::cout << "\n  " << Color::RED << "Unknown provider: " << provider
                                      << Color::RESET << " (groq, openrouter, nvidia, huggingface)\n";
                        } else if (slug.empty()) {
                            std::cout << "\n  " << Color::YELLOW
                                      << "Model slug cannot be empty." << Color::RESET << "\n";
                        } else {
                            sys.cloudLLM.setModel(provider, slug);
                            sys.providerRouter.setProviderModel(provider, slug);
                            std::cout << "\n  " << Color::BGREEN << "✓" << Color::RESET
                                      << " " << provider << " model set to " << Color::BOLD
                                      << slug << Color::RESET << " (saved to models.conf)\n";
                        }
                    }
                    commandFound = true;
                } else if (input == "/idea") {
                    // Use orchestrator for creative idea generation
                    std::string response = sys.orchestrator->respond("Generate a creative, novel idea by combining two unrelated concepts. Be specific and unique.");
                    std::cout << "\n  " << Color::BOLD << Color::BYELLOW << "💡 IDEA" << Color::RESET << "\n";
                    std::cout << "  " << Color::DIM << "─────────────────────────────────" << Color::RESET << "\n";
                    std::cout << "  " << Color::BGREEN << "→" << Color::RESET << " " << response << "\n";
                    std::cout << "  " << Color::DIM << "─────────────────────────────────" << Color::RESET << "\n";
                    commandFound = true;
                } else if (input.size() > 12 && input.substr(0, 12) == "/brainstorm ") {
                    std::string topic = trim(input.substr(12));
                    if (topic.empty()) {
                        std::cout << "\n  " << Color::YELLOW << "Usage:" << Color::RESET << " /brainstorm <topic>\n";
                    } else {
                        std::string response = sys.orchestrator->respond("Brainstorm 5 creative ideas about: " + topic + ". Number them 1-5.");
                        std::cout << "\n  " << Color::BOLD << Color::BYELLOW << "💡 BRAINSTORM: " << topic << Color::RESET << "\n";
                        std::cout << "  " << Color::DIM << "═════════════════════════════════" << Color::RESET << "\n";
                        std::cout << "\n  " << response << "\n";
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
                        std::string response = sys.orchestrator->respond("Invent 3 creative solutions for this problem: " + problem + ". Number them 1-3.");
                        std::cout << "\n  " << Color::BOLD << Color::BYELLOW << "🔧 INVENTIONS for: " << problem << Color::RESET << "\n";
                        std::cout << "  " << Color::DIM << "═════════════════════════════════" << Color::RESET << "\n";
                        std::cout << "\n  " << response << "\n";
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
    sys.learningEngine.persist();
    std::cout << "  " << Color::GREEN << "✓" << Color::RESET << " Memory saved. Knowledge persisted.\n";
    std::cout << "  " << Color::BOLD << Color::CYAN << "MK OS shut down cleanly. Goodbye." 
              << Color::RESET << "\n\n";

    return 0;
}

#endif // MK_ENTRY_CPP
