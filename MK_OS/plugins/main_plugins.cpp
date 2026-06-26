#ifndef MK_MAIN_PLUGINS_CPP
#define MK_MAIN_PLUGINS_CPP

#include <iostream>
#include <string>
#include <memory>
#include "telegram.cpp"
#include "pc_helper.cpp"
#include "custom.cpp"
#include "manager.cpp"

int main() {
    std::cout << "==================================================\n";
    std::cout << "[NEXUS CORE] Initializing Master Plugin Hub Suite\n";
    std::cout << "==================================================\n\n";

    // 1. Initialize Subsystem Plugins
    MKTelegram telegram("8694681522:AAHQ0SNSAm-wMZ2enbbn-4cNF8vV3eBsPqc");
    MKPCHelper pc;
    MKCustomPlugin custom;
    MKPluginManager manager;

    // 2. Register and Track System Architecture Lifecycle
    manager.loadPlugin("Telegram");
    manager.loadPlugin("PC Helper");
    manager.loadPlugin("Custom");
    
    std::cout << "\n[HUB STATUS] Current Active Registry:\n";
    manager.listPlugins();
    std::cout << "--------------------------------------------------\n";

    // 3. Telegram Event Loop Operations
    std::string chatId = "5859347468"; 
    std::cout << "\n[EXECUTION] Initializing Telegram handshake...\n";
    telegram.sendMessage(chatId, "🚀 MK-OS Plugins Hub initialized with enhanced runtime tracking!");
    
    std::string updates = telegram.getUpdates();
    if (!updates.empty()) {
        telegram.autoReply(chatId, updates);
    } else {
        std::cout << "[WARN] Telegram update buffer empty or unreachable.\n";
    }

    // 4. Native OS Automation Tasks
    std::cout << "\n[EXECUTION] Invoking Platform Native Helpers...\n";
    pc.runCommand("echo 'MK-OS: Native environment bridge verified.'");
    pc.openFile("README.md");

    // 5. Custom Extensibility Subroutines
    std::cout << "\n[EXECUTION] Routing Specialized AI Pipelines...\n";
    custom.execute("Custom AI task from plugin hub");

    std::cout << "\n==================================================\n";
    std::cout << "[NEXUS CORE] All plugin modules executed successfully.\n";
    std::cout << "==================================================\n";

    return 0;
}

#endif // MK_MAIN_PLUGINS_CPP