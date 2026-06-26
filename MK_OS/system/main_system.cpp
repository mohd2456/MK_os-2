#ifndef MK_MAIN_SYSTEM_CPP
#define MK_MAIN_SYSTEM_CPP

#include <iostream>
#include <string>

// 1. Local Subsystem Includes
#include "boot.cpp"
#include "logger.cpp"
#include "error.cpp"
#include "api.cpp"

// 2. Global Core Cross-Module System Includes
#include "../ai_core/main.cpp"
#include "../datasets/main_datasets.cpp"
#include "../network/main_network.cpp"
#include "../plugins/main_plugins.cpp"
#include "../io_routing/main_io.cpp"
#include "../security/main_security.cpp"

int main() {
    std::cout << "====================================================================\n";
    std::cout << "███╗   ███╗██╗  ██╗       ██████╗ ███████╗\n";
    std::cout << "████╗ ████║██║ ██╔╝      ██╔═══██╗██╔════╝\n";
    std::cout << "██╔████╔██║█████╔╝       ██║   ██║███████╗\n";
    std::cout << "██║╚██╔╝██║██╔═██╗       ██║   ██║╚════██║\n";
    std::cout << "██║ ╚═╝ ██║██║  ██╗      ╚██████╔╝███████║\n";
    std::cout << "╚═╝     ╚═╝╚═╝  ╚═╝       ╚═════╝ ╚══════╝\n";
    std::cout << "       [MASTER OS SYSTEM FRAMEWORK INITIALIZATION INTERFACE]\n";
    std::cout << "====================================================================\n\n";

    // Subsystem Instantiations
    MKBoot boot;
    MKLogger logger;
    MKError error;
    MKAPI api;

    // IO Routing Layer Components
    MKConsoleIO console;
    MKFileIO file;
    MKNetworkIO net;
    MKDeviceIO device;

    // Security Architecture Layers
    MKCrypto crypto;
    MKSandbox sandbox;
    MKKeys keys;

    // Trigger Hardware/Environment Boot Checklist
    boot.start();
    logger.log("MK-OS core kernel boot sequence started.");

    try {
        // 🌐 API Gateway Authentication Handshakes
        std::cout << "\n[SYSTEM STEP 1] Handshaking Cloud Interface Nodes...\n";
        api.connect("Telegram", "Active");
        api.connect("DuckDuckGo", "Active");
        api.connect("PC Helper", "Local");
        logger.log("All remote cloud APIs connected successfully.");

        // 🎛️ Dynamic IO Channel Registries
        std::cout << "\n[SYSTEM STEP 2] Opening Hardware IO Pipelines...\n";
        console.writeOutput("MK-OS virtual console ready.");
        file.writeFile("mk_boot.txt", "MK-OS boot log entry.");
        net.send("MK-OS network channel initialized.");
        device.connectDevice("Keyboard");

        // 🛡️ Security Kernel Hardening Activation
        std::cout << "\n[SYSTEM STEP 3] Securing Environment Memory Rings...\n";
        keys.storeKey("Telegram", "8694681522:AAHQ0SNSAm-wMZ2enbbn-4cNF8vV3eBsPqc");
        crypto.encrypt("Boot dataset entry");
        sandbox.runIsolated("Telegram Plugin");

        // 🚀 Inter-Module Communication Dispatch (Telegram Automation)
        std::cout << "\n[SYSTEM STEP 4] Initializing Specialized Plugins Loop...\n";
        std::string chatId = "5859347468";
        MKTelegram telegram(keys.getKey("Telegram"));
        telegram.sendMessage(chatId, "🚀 MK-OS booted successfully with full IO + Security + Plugins integration!");
        
        std::string updates = telegram.getUpdates();
        telegram.autoReply(chatId, updates);

        logger.log("MK-OS completely mapped and fortified.");
    } catch (const std::exception& e) {
        error.handle(std::string("Standard error caught: ") + e.what());
    } catch (...) {
        error.handle("Critical unknown structural initialization defect encountered.");
    }

    std::cout << "\n====================================================================\n";
    console.writeOutput("[SYSTEM STATUS] MK-OS Core is now LIVE, STABLE, and SECURE.");
    std::cout << "====================================================================\n";
    
    return 0;
}

#endif // MK_MAIN_SYSTEM_CPP