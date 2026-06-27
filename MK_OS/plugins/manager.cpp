#ifndef MK_MANAGER_CPP
#define MK_MANAGER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

class MKPluginManager {
private:
    std::vector<std::string> activePlugins;

public:
    MKPluginManager() {
        std::cout << "[PLUGIN - MANAGER] Initializing central tracking registry registry...\n";
    }

    // Registers a new plugin name string into the active tracking cluster
    void loadPlugin(const std::string& pluginName) {
        // Ensure we don't add duplicates
        if (std::find(activePlugins.begin(), activePlugins.end(), pluginName) == activePlugins.end()) {
            activePlugins.push_back(pluginName);
            std::cout << "[MANAGER] Successfully loaded and hooked plugin: [" << pluginName << "]\n";
        }
    }

    // Iterates through and lists all currently loaded plugin modules
    void listPlugins() const {
        if (activePlugins.empty()) {
            std::cout << "[MANAGER] Warning: No active plugins registered in the workspace registry.\n";
            return;
        }

        for (size_t i = 0; i < activePlugins.size(); ++i) {
            std::cout << "  " << (i + 1) << ". [" << activePlugins[i] << "] -> Ready\n";
        }
    }
};

#endif // MK_MANAGER_CPP