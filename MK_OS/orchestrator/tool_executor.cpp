// ============================================================
// MK OS - Tool Executor
// Dispatches parsed tool calls to the appropriate subsystem
// handler. Each handler wraps the real module with:
//   - Safety guards (device registration, dangerous cmd check)
//   - Error handling
//   - Output size capping (2000 chars for token efficiency)
// ============================================================
#ifndef MK_TOOL_EXECUTOR_CPP
#define MK_TOOL_EXECUTOR_CPP

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <climits>
#include <cstdlib>
#include <unistd.h>

// Forward declarations - these classes are defined in files included
// before this one in mk_entry.cpp:
//   MKSSHController, MKDockerManager, MKDeviceRegistry,
//   MKResourceMonitor, MKCodeRunner, MKFileReader,
//   MKRealtimeAPIs, MKLearningEngine, MKPatternGraph

// Maximum characters in a tool result (token efficiency)
static const size_t MK_TOOL_MAX_OUTPUT = 2000;

// Maximum tool calls per conversation turn
static const int MK_TOOL_MAX_CALLS_PER_TURN = 3;

// Safe directories for write_file tool
// Uses realpath() to canonicalize paths, resolving symlinks and relative components.
// Only allows writes to /tmp/ and ~/.mk_os/data/ after full canonicalization.
static bool isWritePathAllowed(const std::string& path) {
    if (path.empty()) return false;

    // Build the canonical absolute path
    std::string resolvedPath;

    // Expand ~ to HOME if present
    std::string expandedPath = path;
    const char* home = std::getenv("HOME");
    std::string homePath = home ? std::string(home) : "";

    if (!expandedPath.empty() && expandedPath[0] == '~' && !homePath.empty()) {
        expandedPath = homePath + expandedPath.substr(1);
    }

    // For relative paths, resolve against current working directory
    // realpath() handles this automatically for existing paths, but the file
    // may not exist yet. Resolve the parent directory instead.
    char resolved[PATH_MAX];

    if (expandedPath[0] != '/') {
        // Relative path: resolve CWD + path's directory component
        char cwdBuf[PATH_MAX];
        if (!getcwd(cwdBuf, sizeof(cwdBuf))) return false;
        expandedPath = std::string(cwdBuf) + "/" + expandedPath;
    }

    // Extract parent directory to resolve (file may not exist yet)
    size_t lastSlash = expandedPath.rfind('/');
    if (lastSlash == std::string::npos) return false;

    std::string parentDir = expandedPath.substr(0, lastSlash);
    std::string filename = expandedPath.substr(lastSlash + 1);

    if (parentDir.empty()) parentDir = "/";

    // Resolve the parent directory (this resolves symlinks and ..)
    char* resolvedParent = realpath(parentDir.c_str(), resolved);
    if (!resolvedParent) {
        // Parent directory doesn't exist - deny the write
        return false;
    }

    resolvedPath = std::string(resolvedParent) + "/" + filename;

    // Check against allowed prefixes (with trailing slash to prevent prefix attacks)
    // Allow /tmp/ (the trailing slash ensures /tmpevil/ is not matched)
    if (resolvedPath.find("/tmp/") == 0) return true;

    // Allow ~/.mk_os/data/
    if (!homePath.empty()) {
        std::string mkDataDir = homePath + "/.mk_os/data/";
        if (resolvedPath.find(mkDataDir) == 0) return true;
    }

    return false;
}

// Cap output to max size for token efficiency
static std::string capOutput(const std::string& output) {
    if (output.size() <= MK_TOOL_MAX_OUTPUT) return output;
    std::string truncMsg = "\n[truncated]";
    size_t limit = MK_TOOL_MAX_OUTPUT - truncMsg.size();
    return output.substr(0, limit) + truncMsg;
}

// ============================================================
// MKToolExecutor
// ============================================================
class MKToolExecutor {
public:
    // Subsystem pointers (set by caller after construction)
    MKSSHController* sshController = nullptr;
    MKDockerManager* dockerManager = nullptr;
    MKDeviceRegistry* deviceRegistry = nullptr;
    MKResourceMonitor* resourceMonitor = nullptr;
    MKCodeRunner* codeRunner = nullptr;
    MKFileReader* fileReader = nullptr;
    MKRealtimeAPIs* realtimeApis = nullptr;
    MKLearningEngine* learningEngine = nullptr;
    MKPatternGraph* graph = nullptr;
    MKPaperTrading* paperTrading = nullptr;

    MKToolExecutor() = default;

    // ============================================================
    // Execute a parsed tool call and return the result
    // ============================================================
    MKToolResult execute(const MKParsedToolCall& call) {
        if (!call.valid || call.tool.empty()) {
            return {false, "", "Invalid tool call"};
        }

        // Dispatch to the appropriate handler
        if (call.tool == "ssh_exec") return handleSSHExec(call.args);
        if (call.tool == "docker_cmd") return handleDockerCmd(call.args);
        if (call.tool == "local_shell") return handleLocalShell(call.args);
        if (call.tool == "read_file") return handleReadFile(call.args);
        if (call.tool == "write_file") return handleWriteFile(call.args);
        if (call.tool == "web_search") return handleWebSearch(call.args);
        if (call.tool == "system_info") return handleSystemInfo(call.args);
        if (call.tool == "device_list") return handleDeviceList(call.args);
        if (call.tool == "learn_fact") return handleLearnFact(call.args);
        if (call.tool == "paper_trade") return handlePaperTrade(call.args);

        return {false, "", "Unknown tool: " + call.tool};
    }

private:
    // Get a required arg or return an error result
    std::string getArg(const std::map<std::string, std::string>& args,
                       const std::string& key) const {
        auto it = args.find(key);
        if (it == args.end()) return "";
        return it->second;
    }

    // ============================================================
    // ssh_exec: Run command on registered device
    // Safety: device must be in device_registry
    // ============================================================
    MKToolResult handleSSHExec(const std::map<std::string, std::string>& args) {
        std::string device = getArg(args, "device");
        std::string command = getArg(args, "command");

        if (device.empty()) return {false, "", "Missing required arg: device"};
        if (command.empty()) return {false, "", "Missing required arg: command"};

        // Safety: device must be registered
        if (!sshController || !sshController->isRegistered(device)) {
            std::string msg = "Device '" + device + "' is not registered. ";
            if (deviceRegistry) {
                auto hostnames = deviceRegistry->getAllHostnames();
                if (!hostnames.empty()) {
                    msg += "Available: ";
                    for (size_t i = 0; i < hostnames.size() && i < 5; i++) {
                        if (i > 0) msg += ", ";
                        msg += hostnames[i];
                    }
                }
            }
            return {false, "", msg};
        }

        MKSSHResult result = sshController->executeRemote(device, command);
        if (result.success) {
            return {true, capOutput(result.output), ""};
        } else {
            std::string err = result.error.empty() ? "SSH command failed" : result.error;
            if (result.timed_out) err = "Command timed out";
            return {false, capOutput(result.output), err};
        }
    }

    // ============================================================
    // docker_cmd: Manage containers on a device
    // ============================================================
    MKToolResult handleDockerCmd(const std::map<std::string, std::string>& args) {
        std::string device = getArg(args, "device");
        std::string action = getArg(args, "action");
        std::string container = getArg(args, "container");

        if (device.empty()) return {false, "", "Missing required arg: device"};
        if (action.empty()) return {false, "", "Missing required arg: action"};

        // Safety: device must be registered
        if (!sshController || !sshController->isRegistered(device)) {
            return {false, "", "Device '" + device + "' is not registered."};
        }

        if (!dockerManager) return {false, "", "Docker manager not available"};

        // Generate the docker command
        std::string dockerCmd;
        if (action == "list") {
            dockerCmd = dockerManager->listContainersCmd();
        } else if (action == "start") {
            if (container.empty()) return {false, "", "Missing arg: container (required for start)"};
            dockerCmd = dockerManager->startContainerCmd(container);
        } else if (action == "stop") {
            if (container.empty()) return {false, "", "Missing arg: container (required for stop)"};
            dockerCmd = dockerManager->stopContainerCmd(container);
        } else if (action == "logs") {
            if (container.empty()) return {false, "", "Missing arg: container (required for logs)"};
            dockerCmd = dockerManager->getContainerLogsCmd(container, 30);
        } else {
            return {false, "", "Unknown action: " + action + ". Use: list, start, stop, logs"};
        }

        // Execute via SSH
        MKSSHResult result = sshController->executeRemote(device, dockerCmd);
        if (result.success) {
            // For list action, also parse containers
            if (action == "list") {
                dockerManager->parseContainerList(result.output, device);
            }
            return {true, capOutput(result.output), ""};
        } else {
            return {false, capOutput(result.output), result.error.empty() ? "Docker command failed" : result.error};
        }
    }

    // ============================================================
    // local_shell: Run local command with safety checks
    // Safety: uses MKCodeRunner::isDangerous() equivalent check
    // ============================================================
    MKToolResult handleLocalShell(const std::map<std::string, std::string>& args) {
        std::string command = getArg(args, "command");
        if (command.empty()) return {false, "", "Missing required arg: command"};

        if (!codeRunner) return {false, "", "Code runner not available"};

        // Use the code runner's bash execution which includes isDangerous() check
        MKRunResult result = codeRunner->runBash(command);
        if (result.success) {
            return {true, capOutput(result.stdoutOutput), ""};
        } else {
            std::string err = result.stderrOutput;
            if (err.empty()) err = "Command failed with exit code " + std::to_string(result.exitCode);
            return {false, capOutput(result.stdoutOutput), err};
        }
    }

    // ============================================================
    // read_file: Read file contents
    // ============================================================
    MKToolResult handleReadFile(const std::map<std::string, std::string>& args) {
        std::string path = getArg(args, "path");
        if (path.empty()) return {false, "", "Missing required arg: path"};

        if (!fileReader) return {false, "", "File reader not available"};

        MKFileInfo info = fileReader->readFile(path);
        if (info.success) {
            return {true, capOutput(info.content), ""};
        } else {
            return {false, "", info.error};
        }
    }

    // ============================================================
    // write_file: Write content to a file (safe directories only)
    // Safety: limited to ~/.mk_os/data/, /tmp/, or relative paths
    // ============================================================
    MKToolResult handleWriteFile(const std::map<std::string, std::string>& args) {
        std::string path = getArg(args, "path");
        std::string content = getArg(args, "content");

        if (path.empty()) return {false, "", "Missing required arg: path"};
        if (content.empty()) return {false, "", "Missing required arg: content"};

        // Safety check
        if (!isWritePathAllowed(path)) {
            return {false, "", "Write denied: path '" + path + "' is outside allowed directories. "
                    "Allowed: ~/.mk_os/data/, /tmp/, or relative paths within MK_OS."};
        }

        std::ofstream file(path);
        if (!file.is_open()) {
            return {false, "", "Cannot open file for writing: " + path};
        }
        file << content;
        file.close();

        return {true, "Written " + std::to_string(content.size()) + " bytes to " + path, ""};
    }

    // ============================================================
    // web_search: Search for current information
    // Uses MKRealtimeAPIs (news, weather, time)
    // ============================================================
    MKToolResult handleWebSearch(const std::map<std::string, std::string>& args) {
        std::string query = getArg(args, "query");
        if (query.empty()) return {false, "", "Missing required arg: query"};

        if (!realtimeApis) return {false, "", "Realtime APIs not available"};

        std::string lower = query;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Route to appropriate API based on query content
        if (lower.find("weather") != std::string::npos) {
            // Extract city from query
            std::string city = query;
            // Remove "weather" and common prefixes
            size_t wpos = lower.find("weather");
            if (wpos != std::string::npos) {
                city = query.substr(0, wpos) + query.substr(wpos + 7);
            }
            // Remove common filler words
            for (const auto& word : {"in ", "for ", "at ", "the "}) {
                size_t pos = city.find(word);
                if (pos != std::string::npos) city.erase(pos, std::string(word).size());
            }
            // Trim
            while (!city.empty() && (city.front() == ' ' || city.front() == '\t')) city.erase(city.begin());
            while (!city.empty() && (city.back() == ' ' || city.back() == '\t')) city.pop_back();
            if (city.empty()) city = "New York";

            auto data = realtimeApis->getWeather(city);
            if (data.valid) {
                std::string result = "Weather in " + data.city + ": "
                    + std::to_string((int)data.temperature) + "C, "
                    + data.description + ", Wind: "
                    + std::to_string((int)data.windSpeed) + "km/h, Humidity: "
                    + std::to_string(data.humidity) + "%";
                return {true, result, ""};
            }
            return {false, "", "Could not fetch weather data"};
        }

        if (lower.find("time") != std::string::npos) {
            std::string tz = "UTC";
            // Try to extract timezone from query
            if (lower.find("new york") != std::string::npos) tz = "America/New_York";
            else if (lower.find("london") != std::string::npos) tz = "Europe/London";
            else if (lower.find("tokyo") != std::string::npos) tz = "Asia/Tokyo";
            else if (lower.find("paris") != std::string::npos) tz = "Europe/Paris";

            auto data = realtimeApis->getCurrentTime(tz);
            if (data.valid) {
                return {true, "Current time (" + data.timezone + "): " + data.datetime, ""};
            }
            return {false, "", "Could not fetch time data"};
        }

        if (lower.find("news") != std::string::npos || lower.find("headlines") != std::string::npos) {
            auto data = realtimeApis->getTechNews(5);
            if (data.valid && !data.headlines.empty()) {
                std::string result = "Top tech news:\n";
                int i = 1;
                for (const auto& item : data.headlines) {
                    result += std::to_string(i++) + ". " + item.title;
                    if (item.score > 0) result += " (" + std::to_string(item.score) + " pts)";
                    result += "\n";
                }
                return {true, capOutput(result), ""};
            }
            return {false, "", "Could not fetch news"};
        }

        // Generic: try news as fallback since we lack a full web search engine
        auto data = realtimeApis->getTechNews(3);
        if (data.valid && !data.headlines.empty()) {
            std::string result = "Recent tech headlines (no full web search available):\n";
            int i = 1;
            for (const auto& item : data.headlines) {
                result += std::to_string(i++) + ". " + item.title + "\n";
            }
            return {true, capOutput(result), ""};
        }

        return {false, "", "Web search not available for this query type. Try asking about weather, time, or news."};
    }

    // ============================================================
    // system_info: Local system resources
    // ============================================================
    MKToolResult handleSystemInfo(const std::map<std::string, std::string>& /*args*/) {
        if (!resourceMonitor) return {false, "", "Resource monitor not available"};

        auto res = resourceMonitor->getLocalResources();
        if (res.valid) {
            std::string info = "CPU: " + std::to_string((int)res.cpu_usage_percent) + "%, "
                + "RAM: " + std::to_string(res.available_ram_mb) + "/"
                + std::to_string(res.total_ram_mb) + " MB available, "
                + "Temperature: " + std::to_string((int)res.temperature_celsius) + "C";
            return {true, info, ""};
        }
        return {false, "", "Could not read system resources"};
    }

    // ============================================================
    // device_list: List registered homelab devices
    // ============================================================
    MKToolResult handleDeviceList(const std::map<std::string, std::string>& /*args*/) {
        if (!deviceRegistry) return {false, "", "Device registry not available"};

        int total = deviceRegistry->deviceCount();
        if (total == 0) {
            return {true, "No devices registered. Use device_registry to add devices.", ""};
        }

        auto hostnames = deviceRegistry->getAllHostnames();
        auto onlineDevices = deviceRegistry->getOnlineDevices();

        std::string info = std::to_string(total) + " devices registered, "
            + std::to_string(onlineDevices.size()) + " online:\n";

        for (const auto& hostname : hostnames) {
            const MKDevice* dev = deviceRegistry->getDevice(hostname);
            if (dev) {
                info += "- " + dev->hostname + " (" + dev->ip + ") "
                    + dev->os_type + " " + std::to_string(dev->cpu_cores) + " cores, "
                    + std::to_string(dev->ram_mb) + "MB RAM\n";
            }
        }

        return {true, capOutput(info), ""};
    }

    // ============================================================
    // learn_fact: Teach MK a new fact
    // ============================================================
    MKToolResult handleLearnFact(const std::map<std::string, std::string>& args) {
        std::string subject = getArg(args, "subject");
        std::string relation = getArg(args, "relation");
        std::string object = getArg(args, "object");

        if (subject.empty()) return {false, "", "Missing required arg: subject"};
        if (relation.empty()) return {false, "", "Missing required arg: relation"};
        if (object.empty()) return {false, "", "Missing required arg: object"};

        // Learn via learning engine
        if (learningEngine) {
            learningEngine->learnFact(subject, relation, object);
        }

        // Also persist to knowledge graph
        if (graph) {
            graph->persistNewFact(subject, relation, object, 1.0f);
        }

        return {true, "Learned: " + subject + " " + relation + " " + object, ""};
    }

    // ============================================================
    // paper_trade: Paper trade meme coins
    // ============================================================
    MKToolResult handlePaperTrade(const std::map<std::string, std::string>& args) {
        std::string action = getArg(args, "action");
        std::string symbol = getArg(args, "symbol");
        std::string amountStr = getArg(args, "amount");

        if (action.empty()) return {false, "", "Missing required arg: action. Use: buy, sell, portfolio, history, stats"};

        if (!paperTrading) return {false, "", "Paper trading module not available"};

        // Dispatch by action
        if (action == "buy") {
            if (symbol.empty()) return {false, "", "Missing arg: symbol (CoinGecko coin ID, e.g. 'dogecoin', 'shiba-inu')"};
            double amount = 0.0;
            if (!amountStr.empty()) {
                try { amount = std::stod(amountStr); } catch (...) {
                    return {false, "", "Invalid amount: " + amountStr};
                }
            }
            if (amount <= 0.0) return {false, "", "Amount must be positive (USD to spend)"};
            auto result = paperTrading->buy(symbol, amount);
            if (result.success) return {true, result.message, ""};
            return {false, "", result.error};
        }

        if (action == "sell") {
            if (symbol.empty()) return {false, "", "Missing arg: symbol (CoinGecko coin ID)"};
            double amount = 0.0;
            if (!amountStr.empty()) {
                try { amount = std::stod(amountStr); } catch (...) {
                    return {false, "", "Invalid amount: " + amountStr};
                }
            }
            // amount=0 means sell all
            auto result = paperTrading->sell(symbol, amount);
            if (result.success) return {true, result.message, ""};
            return {false, "", result.error};
        }

        if (action == "portfolio") {
            return {true, paperTrading->getPortfolio(), ""};
        }

        if (action == "history") {
            int n = 10;
            if (!amountStr.empty()) {
                try { n = std::stoi(amountStr); } catch (...) { n = 10; }
            }
            return {true, paperTrading->getTradeHistory(n), ""};
        }

        if (action == "stats") {
            return {true, paperTrading->getStats(), ""};
        }

        return {false, "", "Unknown action: " + action + ". Use: buy, sell, portfolio, history, stats"};
    }
};

#endif // MK_TOOL_EXECUTOR_CPP
