#ifndef MK_PLUGIN_INTERFACE_CPP
#define MK_PLUGIN_INTERFACE_CPP

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>

// ============================================================
// MK Plugin API Interface
// Base struct for plugins. Plugins register commands and provide
// an execute() method to handle them.
// ============================================================

struct MKPlugin {
    std::string name;
    std::string description;
    std::string version;
    std::vector<std::string> commands;

    virtual ~MKPlugin() = default;

    // Execute a command with arguments, return response string
    virtual std::string execute(const std::string& cmd, const std::string& args) = 0;

    // Check if this plugin handles a given command
    bool handlesCommand(const std::string& cmd) const {
        std::string lower = cmd;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (const auto& c : commands) {
            if (c == lower) return true;
        }
        return false;
    }
};

// ============================================================
// Extended Plugin Manager - stores and routes to MKPlugin instances
// ============================================================

class MKPluginSystem {
private:
    std::vector<MKPlugin*> plugins;  // Non-owning pointers (plugins live elsewhere)

public:
    MKPluginSystem() {}

    void registerPlugin(MKPlugin* plugin) {
        if (!plugin) return;
        // Avoid duplicates
        for (const auto* p : plugins) {
            if (p->name == plugin->name) return;
        }
        plugins.push_back(plugin);
        std::cout << "[PLUGINS] Registered: " << plugin->name
                  << " v" << plugin->version << " (" << plugin->commands.size()
                  << " commands)\n";
    }

    void unregisterPlugin(const std::string& name) {
        plugins.erase(
            std::remove_if(plugins.begin(), plugins.end(),
                [&name](const MKPlugin* p) { return p->name == name; }),
            plugins.end());
    }

    // Try to route a command to a plugin. Returns empty if no plugin handles it.
    std::string tryExecute(const std::string& cmd, const std::string& args) {
        for (auto* plugin : plugins) {
            if (plugin->handlesCommand(cmd)) {
                return plugin->execute(cmd, args);
            }
        }
        return ""; // No plugin handled this command
    }

    // List all plugins
    std::string listPlugins() const {
        if (plugins.empty()) {
            return "No plugins loaded.";
        }
        std::string result;
        for (size_t i = 0; i < plugins.size(); i++) {
            const auto* p = plugins[i];
            result += "  " + std::to_string(i + 1) + ". " + p->name +
                      " v" + p->version + " — " + p->description + "\n";
            result += "     Commands: ";
            for (size_t j = 0; j < p->commands.size(); j++) {
                result += "/" + p->commands[j];
                if (j < p->commands.size() - 1) result += ", ";
            }
            result += "\n";
        }
        return result;
    }

    int pluginCount() const { return (int)plugins.size(); }
    const std::vector<MKPlugin*>& getPlugins() const { return plugins; }
};

// ============================================================
// Example built-in plugin: System Info
// ============================================================

class MKSystemInfoPlugin : public MKPlugin {
public:
    MKSystemInfoPlugin() {
        name = "system-info";
        description = "System information and uptime";
        version = "1.0";
        commands = {"sysinfo", "uptime", "hostname"};
    }

    std::string execute(const std::string& cmd, const std::string& /*args*/) override {
        if (cmd == "sysinfo" || cmd == "hostname") {
            #ifdef __APPLE__
            return "MK OS running on macOS (Darwin)";
            #elif __linux__
            return "MK OS running on Linux";
            #else
            return "MK OS running on unknown platform";
            #endif
        } else if (cmd == "uptime") {
            return "MK OS has been running this session.";
        }
        return "Unknown command for system-info plugin.";
    }
};

#endif // MK_PLUGIN_INTERFACE_CPP
