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
#include <sys/wait.h>

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

    // Allow ~/.mk_os/tools/
    if (!homePath.empty()) {
        std::string mkToolsDir = homePath + "/.mk_os/tools/";
        if (resolvedPath.find(mkToolsDir) == 0) return true;
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
    MKToolRegistry* toolRegistry = nullptr;

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
        if (call.tool == "browse_url") return handleBrowseUrl(call.args);
        if (call.tool == "create_tool") return handleCreateTool(call.args);

        // Check for custom tools
        if (toolRegistry && toolRegistry->exists(call.tool)) return handleCustomTool(call.tool, call.args);

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
            std::string tz = "";
            // Comprehensive city-to-timezone mapping
            // US cities
            if (lower.find("new york") != std::string::npos || lower.find("nyc") != std::string::npos) tz = "America/New_York";
            else if (lower.find("fort worth") != std::string::npos) tz = "America/Chicago";
            else if (lower.find("dallas") != std::string::npos) tz = "America/Chicago";
            else if (lower.find("houston") != std::string::npos) tz = "America/Chicago";
            else if (lower.find("san antonio") != std::string::npos) tz = "America/Chicago";
            else if (lower.find("austin") != std::string::npos) tz = "America/Chicago";
            else if (lower.find("chicago") != std::string::npos) tz = "America/Chicago";
            else if (lower.find("los angeles") != std::string::npos || lower.find("la") != std::string::npos) tz = "America/Los_Angeles";
            else if (lower.find("san francisco") != std::string::npos) tz = "America/Los_Angeles";
            else if (lower.find("seattle") != std::string::npos) tz = "America/Los_Angeles";
            else if (lower.find("portland") != std::string::npos) tz = "America/Los_Angeles";
            else if (lower.find("denver") != std::string::npos) tz = "America/Denver";
            else if (lower.find("phoenix") != std::string::npos) tz = "America/Phoenix";
            else if (lower.find("miami") != std::string::npos) tz = "America/New_York";
            else if (lower.find("atlanta") != std::string::npos) tz = "America/New_York";
            else if (lower.find("boston") != std::string::npos) tz = "America/New_York";
            else if (lower.find("philadelphia") != std::string::npos) tz = "America/New_York";
            else if (lower.find("washington") != std::string::npos || lower.find("d.c.") != std::string::npos) tz = "America/New_York";
            else if (lower.find("detroit") != std::string::npos) tz = "America/Detroit";
            else if (lower.find("minneapolis") != std::string::npos) tz = "America/Chicago";
            else if (lower.find("nashville") != std::string::npos) tz = "America/Chicago";
            else if (lower.find("las vegas") != std::string::npos) tz = "America/Los_Angeles";
            else if (lower.find("salt lake") != std::string::npos) tz = "America/Denver";
            else if (lower.find("anchorage") != std::string::npos) tz = "America/Anchorage";
            else if (lower.find("honolulu") != std::string::npos || lower.find("hawaii") != std::string::npos) tz = "Pacific/Honolulu";
            // International cities
            else if (lower.find("london") != std::string::npos) tz = "Europe/London";
            else if (lower.find("paris") != std::string::npos) tz = "Europe/Paris";
            else if (lower.find("berlin") != std::string::npos) tz = "Europe/Berlin";
            else if (lower.find("madrid") != std::string::npos) tz = "Europe/Madrid";
            else if (lower.find("rome") != std::string::npos) tz = "Europe/Rome";
            else if (lower.find("moscow") != std::string::npos) tz = "Europe/Moscow";
            else if (lower.find("dubai") != std::string::npos) tz = "Asia/Dubai";
            else if (lower.find("mumbai") != std::string::npos || lower.find("india") != std::string::npos) tz = "Asia/Kolkata";
            else if (lower.find("tokyo") != std::string::npos || lower.find("japan") != std::string::npos) tz = "Asia/Tokyo";
            else if (lower.find("shanghai") != std::string::npos || lower.find("beijing") != std::string::npos || lower.find("china") != std::string::npos) tz = "Asia/Shanghai";
            else if (lower.find("sydney") != std::string::npos) tz = "Australia/Sydney";
            else if (lower.find("melbourne") != std::string::npos) tz = "Australia/Melbourne";
            else if (lower.find("toronto") != std::string::npos) tz = "America/Toronto";
            else if (lower.find("vancouver") != std::string::npos) tz = "America/Vancouver";
            else if (lower.find("mexico") != std::string::npos) tz = "America/Mexico_City";
            else if (lower.find("sao paulo") != std::string::npos || lower.find("brazil") != std::string::npos) tz = "America/Sao_Paulo";
            else if (lower.find("singapore") != std::string::npos) tz = "Asia/Singapore";
            else if (lower.find("hong kong") != std::string::npos) tz = "Asia/Hong_Kong";
            else if (lower.find("seoul") != std::string::npos || lower.find("korea") != std::string::npos) tz = "Asia/Seoul";
            else if (lower.find("bangkok") != std::string::npos) tz = "Asia/Bangkok";
            else if (lower.find("cairo") != std::string::npos) tz = "Africa/Cairo";
            else if (lower.find("lagos") != std::string::npos) tz = "Africa/Lagos";
            else if (lower.find("johannesburg") != std::string::npos) tz = "Africa/Johannesburg";

            // If no city matched, default to UTC
            if (tz.empty()) tz = "UTC";

            auto data = realtimeApis->getCurrentTime(tz);
            if (data.valid) {
                std::string result = "Current time (" + data.timezone + "): " + data.datetime;
                if (tz == "UTC") {
                    result += "\n(Note: city not recognized in timezone database. Showing UTC. Try specifying a known city or timezone.)";
                }
                return {true, result, ""};
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

        // Generic: try DuckDuckGo Instant Answer API for factual queries
        {
            MKHTTP ddgHttp;
            // URL encode the query
            std::string encodedQuery;
            for (char c : query) {
                if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                    encodedQuery += c;
                } else if (c == ' ') {
                    encodedQuery += '+';
                } else {
                    char hex[4];
                    std::snprintf(hex, sizeof(hex), "%%%02X", (unsigned char)c);
                    encodedQuery += hex;
                }
            }
            std::string ddgUrl = "https://api.duckduckgo.com/?q=" + encodedQuery + "&format=json&no_html=1";
            std::string ddgResponse = ddgHttp.get(ddgUrl);

            if (!ddgResponse.empty()) {
                // Parse DuckDuckGo response - look for "AbstractText" or "Answer" fields
                std::string abstractText;
                std::string answer;

                // Extract "AbstractText" value
                size_t atPos = ddgResponse.find("\"AbstractText\"");
                if (atPos != std::string::npos) {
                    size_t valStart = ddgResponse.find("\"", atPos + 14);
                    if (valStart != std::string::npos) {
                        valStart++;
                        size_t valEnd = valStart;
                        while (valEnd < ddgResponse.size() && ddgResponse[valEnd] != '"') {
                            if (ddgResponse[valEnd] == '\\') valEnd++;
                            valEnd++;
                        }
                        abstractText = ddgResponse.substr(valStart, valEnd - valStart);
                    }
                }

                // Extract "Answer" value
                size_t ansPos = ddgResponse.find("\"Answer\"");
                if (ansPos != std::string::npos) {
                    size_t valStart = ddgResponse.find("\"", ansPos + 8);
                    if (valStart != std::string::npos) {
                        valStart++;
                        size_t valEnd = valStart;
                        while (valEnd < ddgResponse.size() && ddgResponse[valEnd] != '"') {
                            if (ddgResponse[valEnd] == '\\') valEnd++;
                            valEnd++;
                        }
                        answer = ddgResponse.substr(valStart, valEnd - valStart);
                    }
                }

                // Prefer Answer, then AbstractText
                if (!answer.empty()) {
                    return {true, capOutput("DuckDuckGo: " + answer), ""};
                }
                if (!abstractText.empty()) {
                    return {true, capOutput("DuckDuckGo: " + abstractText), ""};
                }
            }
        }

        return {false, "", "Web search could not find an answer for this query. Try asking about weather, time, or news."};
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

    // ============================================================
    // create_tool: Create a new custom tool
    // ============================================================
    MKToolResult handleCreateTool(const std::map<std::string, std::string>& args) {
        std::string name = getArg(args, "name");
        std::string description = getArg(args, "description");
        std::string language = getArg(args, "language");
        std::string code = getArg(args, "code");
        std::string argsStr = getArg(args, "args");

        if (name.empty()) return {false, "", "Missing required arg: name"};
        if (description.empty()) return {false, "", "Missing required arg: description"};
        if (language.empty()) return {false, "", "Missing required arg: language"};
        if (code.empty()) return {false, "", "Missing required arg: code"};

        // Validate name: alphanumeric + underscore, 3-30 chars
        if (name.size() < 3 || name.size() > 30) {
            return {false, "", "Tool name must be 3-30 characters long"};
        }
        for (char c : name) {
            if (!std::isalnum(c) && c != '_') {
                return {false, "", "Tool name must contain only alphanumeric characters and underscores"};
            }
        }

        // Validate language
        if (language != "bash" && language != "python") {
            return {false, "", "Language must be 'bash' or 'python'"};
        }

        // Check for duplicate: if tool already exists in registry, reject
        if (toolRegistry && toolRegistry->exists(name)) {
            return {false, "", "Tool '" + name + "' already exists. Choose a different name."};
        }

        // Get HOME directory
        const char* home = std::getenv("HOME");
        if (!home) return {false, "", "Cannot determine HOME directory"};
        std::string homePath(home);

        // Create tools directory
        std::string toolsDir = homePath + "/.mk_os/tools";
        std::string mkdirCmd = "mkdir -p " + toolsDir;
        FILE* mkdirPipe = popen(mkdirCmd.c_str(), "r");
        if (mkdirPipe) pclose(mkdirPipe);

        // Determine script extension and path
        std::string ext = (language == "bash") ? ".sh" : ".py";
        std::string scriptPath = toolsDir + "/" + name + ext;

        // Write script file
        std::ofstream scriptFile(scriptPath);
        if (!scriptFile.is_open()) {
            return {false, "", "Cannot write script to: " + scriptPath};
        }
        if (language == "bash") {
            scriptFile << "#!/bin/bash\n" << code << "\n";
        } else {
            scriptFile << "#!/usr/bin/env python3\n" << code << "\n";
        }
        scriptFile.close();

        // Make it executable
        std::string chmodCmd = "chmod +x " + scriptPath;
        FILE* chmodPipe = popen(chmodCmd.c_str(), "r");
        if (chmodPipe) pclose(chmodPipe);

        // Build paramSchema from args string
        std::string paramSchema = "{}";
        if (!argsStr.empty()) {
            paramSchema = "{";
            bool first = true;
            std::string argName;
            for (size_t i = 0; i <= argsStr.size(); i++) {
                if (i == argsStr.size() || argsStr[i] == ',') {
                    // Trim the arg name
                    size_t as = argName.find_first_not_of(" \t");
                    size_t ae = argName.find_last_not_of(" \t");
                    if (as != std::string::npos) {
                        std::string trimmed = argName.substr(as, ae - as + 1);
                        if (!trimmed.empty()) {
                            if (!first) paramSchema += ", ";
                            paramSchema += "\"" + trimmed + "\": \"<value>\"";
                            first = false;
                        }
                    }
                    argName.clear();
                } else {
                    argName += argsStr[i];
                }
            }
            paramSchema += "}";
        }

        // Read existing manifest or start fresh
        std::string manifestPath = toolsDir + "/manifest.json";
        std::string manifestContent;
        {
            std::ifstream mf(manifestPath);
            if (mf.is_open()) {
                manifestContent = std::string((std::istreambuf_iterator<char>(mf)),
                                              std::istreambuf_iterator<char>());
                mf.close();
            }
        }

        // Build new manifest entry
        std::string newEntry = "{\"name\": \"" + name + "\", \"description\": \"" + description +
                               "\", \"paramSchema\": \"" + paramSchema + "\", \"language\": \"" +
                               language + "\", \"scriptPath\": \"" + scriptPath + "\"}";

        // Append to manifest (simple approach: rebuild the array)
        std::string newManifest;
        if (manifestContent.empty() || manifestContent.find('[') == std::string::npos) {
            newManifest = "[\n  " + newEntry + "\n]";
        } else {
            // Find the closing bracket and insert before it
            size_t closeBracket = manifestContent.rfind(']');
            if (closeBracket == std::string::npos) {
                newManifest = "[\n  " + newEntry + "\n]";
            } else {
                std::string before = manifestContent.substr(0, closeBracket);
                // Trim trailing whitespace
                size_t lastContent = before.find_last_not_of(" \t\n\r");
                if (lastContent != std::string::npos) {
                    before = before.substr(0, lastContent + 1);
                }
                newManifest = before + ",\n  " + newEntry + "\n]";
            }
        }

        // Write manifest
        std::ofstream mfOut(manifestPath);
        if (!mfOut.is_open()) {
            return {false, "", "Cannot write manifest.json"};
        }
        mfOut << newManifest;
        mfOut.close();

        // Register in the tool registry
        if (toolRegistry) {
            toolRegistry->registerCustomTool(name, description, paramSchema);
        }

        return {true, "Created custom tool '" + name + "' (" + language + "). Script saved to " +
                scriptPath + ". You can now call it with {\"tool\": \"" + name + "\", \"args\": {...}}", ""};
    }

    // ============================================================
    // handleCustomTool: Execute a custom tool by reading manifest
    // Safety: validates scriptPath resolves inside ~/.mk_os/tools/
    //         and checks script content against isDangerous patterns
    // ============================================================
    MKToolResult handleCustomTool(const std::string& toolName, const std::map<std::string, std::string>& args) {
        const char* home = std::getenv("HOME");
        if (!home) return {false, "", "Cannot determine HOME directory"};
        std::string homePath(home);

        std::string manifestPath = homePath + "/.mk_os/tools/manifest.json";
        std::ifstream mf(manifestPath);
        if (!mf.is_open()) {
            return {false, "", "Custom tool manifest not found"};
        }
        std::string manifestContent((std::istreambuf_iterator<char>(mf)),
                                     std::istreambuf_iterator<char>());
        mf.close();

        // Find the tool entry in manifest
        std::string scriptPath;
        std::string language;

        size_t pos = 0;
        while (pos < manifestContent.size()) {
            size_t objStart = manifestContent.find('{', pos);
            if (objStart == std::string::npos) break;

            int depth = 0;
            size_t objEnd = objStart;
            for (size_t i = objStart; i < manifestContent.size(); i++) {
                if (manifestContent[i] == '{') depth++;
                else if (manifestContent[i] == '}') {
                    depth--;
                    if (depth == 0) { objEnd = i + 1; break; }
                }
            }
            if (objEnd <= objStart) break;

            std::string obj = manifestContent.substr(objStart, objEnd - objStart);

            // Check if this entry matches our tool name
            size_t namePos = obj.find("\"name\"");
            if (namePos != std::string::npos) {
                // Extract value after "name":
                size_t valStart = obj.find('"', namePos + 6);
                if (valStart != std::string::npos) {
                    valStart++;
                    size_t colonPos = obj.find(':', namePos + 6);
                    // Re-find: skip to after colon
                    size_t afterColon = obj.find('"', colonPos + 1);
                    if (afterColon != std::string::npos) {
                        afterColon++;
                        size_t valEnd = obj.find('"', afterColon);
                        if (valEnd != std::string::npos) {
                            std::string foundName = obj.substr(afterColon, valEnd - afterColon);
                            if (foundName == toolName) {
                                // Extract scriptPath
                                size_t spPos = obj.find("\"scriptPath\"");
                                if (spPos != std::string::npos) {
                                    size_t spColon = obj.find(':', spPos + 12);
                                    size_t spValStart = obj.find('"', spColon + 1);
                                    if (spValStart != std::string::npos) {
                                        spValStart++;
                                        size_t spValEnd = obj.find('"', spValStart);
                                        if (spValEnd != std::string::npos) {
                                            scriptPath = obj.substr(spValStart, spValEnd - spValStart);
                                        }
                                    }
                                }
                                // Extract language
                                size_t lgPos = obj.find("\"language\"");
                                if (lgPos != std::string::npos) {
                                    size_t lgColon = obj.find(':', lgPos + 10);
                                    size_t lgValStart = obj.find('"', lgColon + 1);
                                    if (lgValStart != std::string::npos) {
                                        lgValStart++;
                                        size_t lgValEnd = obj.find('"', lgValStart);
                                        if (lgValEnd != std::string::npos) {
                                            language = obj.substr(lgValStart, lgValEnd - lgValStart);
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }

            pos = objEnd;
        }

        if (scriptPath.empty()) {
            return {false, "", "Custom tool '" + toolName + "' not found in manifest"};
        }

        // Safety check 1: Validate scriptPath resolves inside ~/.mk_os/tools/
        // This prevents a corrupted manifest or symlink attack from executing
        // arbitrary paths outside the tools directory.
        std::string allowedDir = homePath + "/.mk_os/tools/";
        {
            char resolvedBuf[PATH_MAX];
            char* resolved = realpath(scriptPath.c_str(), resolvedBuf);
            if (!resolved) {
                return {false, "", "Custom tool script not found: " + scriptPath};
            }
            std::string resolvedPath(resolved);
            if (resolvedPath.find(allowedDir) != 0) {
                return {false, "", "Blocked: custom tool script path resolves outside ~/.mk_os/tools/"};
            }
        }

        // Safety check 2: Read script content and check against dangerous patterns
        // Custom tools bypass codeRunner->runBash() so we replicate the isDangerous check.
        {
            std::ifstream scriptFile(scriptPath);
            if (!scriptFile.is_open()) {
                return {false, "", "Cannot read custom tool script: " + scriptPath};
            }
            std::string scriptContent((std::istreambuf_iterator<char>(scriptFile)),
                                       std::istreambuf_iterator<char>());
            scriptFile.close();

            if (codeRunner && codeRunner->isDangerousCmd(scriptContent)) {
                return {false, "", "Blocked: custom tool script contains dangerous commands"};
            }
        }

        // Build command with env vars for each arg
        std::string envPrefix = "env ";
        for (const auto& kv : args) {
            // Convert arg name to uppercase for env var
            std::string envName = "MK_ARG_";
            for (char c : kv.first) {
                envName += std::toupper(c);
            }
            // Escape single quotes in value
            std::string escapedVal;
            for (char c : kv.second) {
                if (c == '\'') escapedVal += "'\\''";
                else escapedVal += c;
            }
            envPrefix += envName + "='" + escapedVal + "' ";
        }

        // Build execution command with timeout
        std::string cmd = "timeout 10 " + envPrefix;
        if (language == "python") {
            cmd += "python3 " + scriptPath;
        } else {
            cmd += "bash " + scriptPath;
        }
        cmd += " 2>&1";

        // Execute with popen
        std::string output;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return {false, "", "Failed to execute custom tool script"};
        }

        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
            if (output.size() > MK_TOOL_MAX_OUTPUT) break;
        }
        int status = pclose(pipe);
        int exitCode = WEXITSTATUS(status);

        if (exitCode == 124) {
            return {false, capOutput(output), "Custom tool timed out (10s limit)"};
        }
        if (exitCode != 0 && output.empty()) {
            return {false, "", "Custom tool exited with code " + std::to_string(exitCode)};
        }

        return {exitCode == 0, capOutput(output), exitCode != 0 ? "Exited with code " + std::to_string(exitCode) : ""};
    }

    // ============================================================
    // browse_url: Fetch a webpage and return plain text
    // Safety: reject private/local IPs
    // ============================================================

    // Write callback for browse_url curl with size limit
    static size_t browseWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        if (!userp) return 0;
        std::string* buf = static_cast<std::string*>(userp);
        size_t totalSize = size * nmemb;
        // Enforce 500KB max download
        if (buf->size() + totalSize > 512000) {
            size_t remaining = 512000 - buf->size();
            if (remaining > 0) buf->append((char*)contents, remaining);
            return 0; // abort transfer
        }
        buf->append((char*)contents, totalSize);
        return totalSize;
    }

    // Strip HTML tags and collapse whitespace
    static std::string stripHtml(const std::string& html) {
        std::string result;
        result.reserve(html.size() / 2);
        bool inTag = false;
        bool inScript = false;
        bool inStyle = false;
        bool lastWasSpace = false;

        for (size_t i = 0; i < html.size(); i++) {
            if (html[i] == '<') {
                // Check if this is opening a script or style tag
                std::string tagCheck;
                for (size_t j = i + 1; j < html.size() && j < i + 10 && html[j] != '>' && html[j] != ' '; j++) {
                    tagCheck += std::tolower(html[j]);
                }
                if (tagCheck.find("script") == 0) inScript = true;
                if (tagCheck.find("style") == 0) inStyle = true;
                if (tagCheck.find("/script") == 0) inScript = false;
                if (tagCheck.find("/style") == 0) inStyle = false;
                inTag = true;
                continue;
            }
            if (html[i] == '>') {
                inTag = false;
                continue;
            }
            if (inTag || inScript || inStyle) continue;

            // Collapse whitespace
            char c = html[i];
            if (c == '\n' || c == '\r' || c == '\t') c = ' ';
            if (c == ' ') {
                if (!lastWasSpace && !result.empty()) {
                    result += ' ';
                    lastWasSpace = true;
                }
            } else {
                result += c;
                lastWasSpace = false;
            }
        }

        // Trim leading/trailing whitespace
        size_t start = result.find_first_not_of(' ');
        size_t end = result.find_last_not_of(' ');
        if (start == std::string::npos) return "";
        return result.substr(start, end - start + 1);
    }

    // Check if a hostname resolves to a private/local IP
    static bool isPrivateUrl(const std::string& url) {
        // Extract hostname from URL
        std::string host;
        size_t schemeEnd = url.find("://");
        if (schemeEnd == std::string::npos) return true;
        size_t hostStart = schemeEnd + 3;
        size_t hostEnd = url.find_first_of(":/", hostStart);
        if (hostEnd == std::string::npos) hostEnd = url.size();
        host = url.substr(hostStart, hostEnd - hostStart);

        // Convert to lowercase
        std::string lower;
        for (char c : host) lower += std::tolower(c);

        // Block localhost
        if (lower == "localhost" || lower == "localhost.localdomain") return true;

        // Block private IP ranges
        // 127.x.x.x
        if (lower.find("127.") == 0) return true;
        // 10.x.x.x
        if (lower.find("10.") == 0) return true;
        // 192.168.x.x
        if (lower.find("192.168.") == 0) return true;
        // 172.16-31.x.x
        if (lower.find("172.") == 0) {
            size_t dotPos = lower.find('.', 4);
            if (dotPos != std::string::npos) {
                std::string secondOctet = lower.substr(4, dotPos - 4);
                try {
                    int octet = std::stoi(secondOctet);
                    if (octet >= 16 && octet <= 31) return true;
                } catch (...) {}
            }
        }
        // 169.254.x.x (link-local)
        if (lower.find("169.254.") == 0) return true;
        // [::1] or ::1 (IPv6 loopback)
        if (lower == "::1" || lower == "[::1]") return true;
        // 0.0.0.0
        if (lower == "0.0.0.0") return true;

        return false;
    }

    MKToolResult handleBrowseUrl(const std::map<std::string, std::string>& args) {
        std::string url = getArg(args, "url");
        if (url.empty()) return {false, "", "Missing required arg: url"};

        // Validate URL scheme
        if (url.find("http://") != 0 && url.find("https://") != 0) {
            return {false, "", "Invalid URL: must start with http:// or https://"};
        }

        // Safety: reject private/local URLs
        if (isPrivateUrl(url)) {
            return {false, "", "Blocked: cannot browse private/local network addresses"};
        }

        // Fetch the page using libcurl
        CURL* curl = curl_easy_init();
        if (!curl) return {false, "", "Failed to initialize HTTP client"};

        std::string responseBody;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, browseWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "MK-OS/1.0");

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK && responseBody.empty()) {
            return {false, "", "Failed to fetch URL: " + std::string(curl_easy_strerror(res))};
        }

        if (responseBody.empty()) {
            return {false, "", "URL returned empty response"};
        }

        // Strip HTML and clean up text
        std::string text = stripHtml(responseBody);

        if (text.empty()) {
            return {false, "", "Page contained no readable text content"};
        }

        return {true, capOutput(text), ""};
    }
};

#endif // MK_TOOL_EXECUTOR_CPP
