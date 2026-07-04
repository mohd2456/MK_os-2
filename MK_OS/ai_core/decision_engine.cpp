// ============================================================
// MK OS - Decision/Action Engine (Layer 2)
// ============================================================
// Takes thinking output + user input, decides what actions to
// take, executes them, and formats the final response.
//
// Flow:
//   1. Parse thinking output for action keywords
//   2. Execute relevant tools (SSH, Docker, graph, system info)
//   3. Combine results with personality
//   4. Format and return the final response
//
// Tool registry maps keywords to tool functions:
//   docker, container -> Docker manager
//   ssh, run command -> SSH controller
//   system, resource -> Resource monitor
//   fact, look up, knowledge -> Graph query
//   explain, describe -> Conversational response
// ============================================================
#ifndef MK_DECISION_ENGINE_CPP
#define MK_DECISION_ENGINE_CPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <functional>

// Action types that the decision engine can dispatch
enum class MKActionType {
    DOCKER_STATUS,
    DOCKER_LOGS,
    SSH_COMMAND,
    SYSTEM_INFO,
    GRAPH_LOOKUP,
    EXPLAIN,
    CONVERSATIONAL,
    CONFIRM_DESTRUCTIVE,
    NONE
};

// Parsed action from thinking output
struct MKParsedAction {
    MKActionType type;
    std::string keyword;
    std::string params;    // Additional parameters extracted from context
};

class MKDecisionEngine {
private:
    MKPatternGraph* graph_;
    MKSSHController* sshController_;
    MKDockerManager* dockerManager_;
    MKResourceMonitor* resourceMonitor_;

    // Keyword to action type mapping
    struct KeywordMapping {
        std::string keyword;
        MKActionType action;
    };

    std::vector<KeywordMapping> toolRegistry_;

    // Initialize the tool registry
    void initToolRegistry() {
        toolRegistry_ = {
            {"docker", MKActionType::DOCKER_STATUS},
            {"container", MKActionType::DOCKER_STATUS},
            {"containers", MKActionType::DOCKER_STATUS},
            {"docker ps", MKActionType::DOCKER_STATUS},
            {"docker logs", MKActionType::DOCKER_LOGS},
            {"logs", MKActionType::DOCKER_LOGS},
            {"ssh", MKActionType::SSH_COMMAND},
            {"run command", MKActionType::SSH_COMMAND},
            {"execute", MKActionType::SSH_COMMAND},
            {"system", MKActionType::SYSTEM_INFO},
            {"resource", MKActionType::SYSTEM_INFO},
            {"cpu", MKActionType::SYSTEM_INFO},
            {"memory", MKActionType::SYSTEM_INFO},
            {"ram", MKActionType::SYSTEM_INFO},
            {"temperature", MKActionType::SYSTEM_INFO},
            {"disk", MKActionType::SYSTEM_INFO},
            {"fact", MKActionType::GRAPH_LOOKUP},
            {"look up", MKActionType::GRAPH_LOOKUP},
            {"knowledge", MKActionType::GRAPH_LOOKUP},
            {"search graph", MKActionType::GRAPH_LOOKUP},
            {"explain", MKActionType::EXPLAIN},
            {"describe", MKActionType::EXPLAIN},
            {"tell", MKActionType::EXPLAIN},
            {"restart", MKActionType::CONFIRM_DESTRUCTIVE},
            {"delete", MKActionType::CONFIRM_DESTRUCTIVE},
            {"remove", MKActionType::CONFIRM_DESTRUCTIVE},
            {"deploy", MKActionType::CONFIRM_DESTRUCTIVE},
            {"stop", MKActionType::CONFIRM_DESTRUCTIVE},
            {"shutdown", MKActionType::CONFIRM_DESTRUCTIVE},
        };
    }

    // Convert string to lowercase
    static std::string toLower(const std::string& s) {
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }

    // Parse thinking output to extract action keywords
    std::vector<MKParsedAction> parseThinkingOutput(const std::string& thinkingOutput) const {
        std::vector<MKParsedAction> actions;
        std::string lower = toLower(thinkingOutput);

        for (const auto& mapping : toolRegistry_) {
            if (lower.find(mapping.keyword) != std::string::npos) {
                MKParsedAction action;
                action.type = mapping.action;
                action.keyword = mapping.keyword;
                action.params = "";
                actions.push_back(action);
            }
        }

        // If no specific actions found, default to conversational
        if (actions.empty()) {
            MKParsedAction fallback;
            fallback.type = MKActionType::CONVERSATIONAL;
            fallback.keyword = "respond";
            fallback.params = "";
            actions.push_back(fallback);
        }

        // Deduplicate by action type (keep first match)
        std::vector<MKParsedAction> unique;
        std::vector<MKActionType> seen;
        for (const auto& a : actions) {
            bool found = false;
            for (const auto& s : seen) {
                if (s == a.type) { found = true; break; }
            }
            if (!found) {
                unique.push_back(a);
                seen.push_back(a.type);
            }
        }

        return unique;
    }

public:
    MKDecisionEngine()
        : graph_(nullptr), sshController_(nullptr),
          dockerManager_(nullptr), resourceMonitor_(nullptr) {
        initToolRegistry();
    }

    void setGraph(MKPatternGraph* graph) { graph_ = graph; }
    void setSSHController(MKSSHController* ssh) { sshController_ = ssh; }
    void setDockerManager(MKDockerManager* docker) { dockerManager_ = docker; }
    void setResourceMonitor(MKResourceMonitor* monitor) { resourceMonitor_ = monitor; }

    // Check if an action requires user confirmation before executing
    bool shouldConfirm(const std::string& thinkingOutput) const {
        std::string lower = toLower(thinkingOutput);

        // Destructive keywords that need confirmation
        static const std::vector<std::string> destructiveKeywords = {
            "restart", "delete", "remove", "deploy", "stop",
            "shutdown", "reboot", "kill", "destroy", "wipe",
            "format", "purge", "drop"
        };

        for (const auto& keyword : destructiveKeywords) {
            if (lower.find(keyword) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    // Execute a tool action based on parsed action type
    std::string executeToolAction(MKActionType actionType, const std::string& userInput) const {
        switch (actionType) {
            case MKActionType::DOCKER_STATUS: {
                if (dockerManager_) {
                    std::string cmd = dockerManager_->listContainersCmd();
                    return "Docker command: " + cmd + " (would execute via SSH to target device)";
                }
                return "Docker manager not available.";
            }
            case MKActionType::DOCKER_LOGS: {
                if (dockerManager_) {
                    return "Would fetch container logs via SSH.";
                }
                return "Docker manager not available.";
            }
            case MKActionType::SSH_COMMAND: {
                if (sshController_) {
                    return "SSH controller ready with " +
                           std::to_string(sshController_->deviceCount()) + " hosts configured.";
                }
                return "SSH controller not available.";
            }
            case MKActionType::SYSTEM_INFO: {
                if (resourceMonitor_) {
                    auto res = resourceMonitor_->getLocalResources();
                    if (res.valid) {
                        std::ostringstream info;
                        info << "CPU: " << (int)res.cpu_usage_percent << "%, "
                             << "RAM: " << res.available_ram_mb << "/" << res.total_ram_mb << "MB, "
                             << "Temp: " << (int)res.temperature_celsius << "C, "
                             << "Cores: " << res.cpu_cores;
                        return info.str();
                    }
                }
                return "Resource monitor not available.";
            }
            case MKActionType::GRAPH_LOOKUP: {
                if (graph_) {
                    auto results = graph_->getAll(userInput);
                    if (!results.empty()) {
                        std::ostringstream facts;
                        for (const auto& e : results) {
                            facts << e.source << " " << e.relation << " " << e.target << ". ";
                        }
                        return facts.str();
                    }
                    return "No facts found in knowledge graph for this query.";
                }
                return "Knowledge graph not available.";
            }
            case MKActionType::EXPLAIN: {
                // For explain actions, gather graph facts
                if (graph_) {
                    auto results = graph_->getAll(userInput);
                    if (!results.empty()) {
                        std::ostringstream explanation;
                        explanation << "Based on what I know: ";
                        for (const auto& e : results) {
                            explanation << e.source << " " << e.relation << " " << e.target << ". ";
                        }
                        return explanation.str();
                    }
                }
                return "";
            }
            case MKActionType::CONFIRM_DESTRUCTIVE: {
                return "CONFIRM_NEEDED";
            }
            case MKActionType::CONVERSATIONAL:
            case MKActionType::NONE:
            default:
                return "";
        }
    }

    // Format the final response with personality
    std::string formatResponse(const std::string& actionResults,
                               const std::string& thinkingReasoning,
                               const std::string& userInput) const {
        std::ostringstream response;

        if (actionResults == "CONFIRM_NEEDED") {
            response << "That sounds like it could be a destructive action. "
                     << "Want me to go ahead with that? Just confirm and I'll do it.";
            return response.str();
        }

        if (!actionResults.empty()) {
            response << actionResults;
        } else if (!thinkingReasoning.empty()) {
            // Use the thinking reasoning as the basis for response
            // The reasoning tells us what to do; format it naturally
            response << thinkingReasoning;
        }

        return response.str();
    }

    // Main processing method: takes thinking output and produces final response
    std::string process(const std::string& userInput,
                        const std::string& thinkingOutput,
                        const std::vector<std::string>& graphFacts,
                        const std::string& conversationHistory) const {
        (void)conversationHistory; // Used for future context-aware responses

        // If thinking output is empty, we cannot make decisions
        if (thinkingOutput.empty()) {
            return "";
        }

        // Check if confirmation is needed first
        if (shouldConfirm(thinkingOutput)) {
            return formatResponse("CONFIRM_NEEDED", thinkingOutput, userInput);
        }

        // Parse thinking output for action keywords
        std::vector<MKParsedAction> actions = parseThinkingOutput(thinkingOutput);

        // Execute actions and collect results
        std::string combinedResults;
        for (const auto& action : actions) {
            if (action.type == MKActionType::CONVERSATIONAL) continue;

            std::string result = executeToolAction(action.type, userInput);
            if (!result.empty() && result != "CONFIRM_NEEDED") {
                if (!combinedResults.empty()) combinedResults += " ";
                combinedResults += result;
            }
        }

        // If we got tool results, format them
        if (!combinedResults.empty()) {
            return formatResponse(combinedResults, thinkingOutput, userInput);
        }

        // For conversational actions, use graph facts if available
        if (!graphFacts.empty()) {
            std::ostringstream factResponse;
            for (const auto& f : graphFacts) {
                factResponse << f << ". ";
            }
            return formatResponse(factResponse.str(), thinkingOutput, userInput);
        }

        // Return thinking reasoning as-is (the model already told us what to do)
        return formatResponse("", thinkingOutput, userInput);
    }

    // Get the list of registered tool keywords
    std::vector<std::string> getRegisteredKeywords() const {
        std::vector<std::string> keywords;
        for (const auto& mapping : toolRegistry_) {
            keywords.push_back(mapping.keyword);
        }
        return keywords;
    }

    // Check if a keyword maps to a specific action type
    MKActionType getActionForKeyword(const std::string& keyword) const {
        std::string lower = toLower(keyword);
        for (const auto& mapping : toolRegistry_) {
            if (lower.find(mapping.keyword) != std::string::npos) {
                return mapping.action;
            }
        }
        return MKActionType::NONE;
    }
};

#endif // MK_DECISION_ENGINE_CPP
