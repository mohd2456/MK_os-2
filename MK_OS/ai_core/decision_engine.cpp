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
    // Keywords are ordered from most specific to least specific.
    // Single common English words (like "system", "tell") require action-intent 
    // context to avoid false positives.
    void initToolRegistry() {
        toolRegistry_ = {
            // Docker (specific enough to not false-positive)
            {"docker ps", MKActionType::DOCKER_STATUS},
            {"docker status", MKActionType::DOCKER_STATUS},
            {"check docker", MKActionType::DOCKER_STATUS},
            {"check container", MKActionType::DOCKER_STATUS},
            {"list container", MKActionType::DOCKER_STATUS},
            {"container status", MKActionType::DOCKER_STATUS},
            {"docker logs", MKActionType::DOCKER_LOGS},
            {"container logs", MKActionType::DOCKER_LOGS},
            // SSH (multi-word to avoid matching casual uses)
            {"run command", MKActionType::SSH_COMMAND},
            {"ssh into", MKActionType::SSH_COMMAND},
            {"execute command", MKActionType::SSH_COMMAND},
            {"ssh command", MKActionType::SSH_COMMAND},
            {"remote command", MKActionType::SSH_COMMAND},
            // System info (require context)
            {"check cpu", MKActionType::SYSTEM_INFO},
            {"check ram", MKActionType::SYSTEM_INFO},
            {"check memory", MKActionType::SYSTEM_INFO},
            {"check disk", MKActionType::SYSTEM_INFO},
            {"system status", MKActionType::SYSTEM_INFO},
            {"system resource", MKActionType::SYSTEM_INFO},
            {"resource usage", MKActionType::SYSTEM_INFO},
            {"cpu usage", MKActionType::SYSTEM_INFO},
            {"memory usage", MKActionType::SYSTEM_INFO},
            {"disk usage", MKActionType::SYSTEM_INFO},
            {"temperature check", MKActionType::SYSTEM_INFO},
            // Graph lookup (specific)
            {"look up", MKActionType::GRAPH_LOOKUP},
            {"search graph", MKActionType::GRAPH_LOOKUP},
            {"check knowledge", MKActionType::GRAPH_LOOKUP},
            {"find fact", MKActionType::GRAPH_LOOKUP},
            {"knowledge graph", MKActionType::GRAPH_LOOKUP},
            // Destructive actions (specific intent patterns)
            {"should restart", MKActionType::CONFIRM_DESTRUCTIVE},
            {"need to restart", MKActionType::CONFIRM_DESTRUCTIVE},
            {"restart the", MKActionType::CONFIRM_DESTRUCTIVE},
            {"should delete", MKActionType::CONFIRM_DESTRUCTIVE},
            {"need to delete", MKActionType::CONFIRM_DESTRUCTIVE},
            {"delete the", MKActionType::CONFIRM_DESTRUCTIVE},
            {"should remove", MKActionType::CONFIRM_DESTRUCTIVE},
            {"remove the", MKActionType::CONFIRM_DESTRUCTIVE},
            {"should deploy", MKActionType::CONFIRM_DESTRUCTIVE},
            {"deploy the", MKActionType::CONFIRM_DESTRUCTIVE},
            {"should stop", MKActionType::CONFIRM_DESTRUCTIVE},
            {"stop the", MKActionType::CONFIRM_DESTRUCTIVE},
            {"shut down", MKActionType::CONFIRM_DESTRUCTIVE},
            {"shutdown the", MKActionType::CONFIRM_DESTRUCTIVE},
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
    // Only triggers on intent patterns, not incidental mentions of keywords.
    // "no need to restart" won't trigger, but "should restart the container" will.
    bool shouldConfirm(const std::string& thinkingOutput) const {
        std::string lower = toLower(thinkingOutput);

        // Intent patterns that indicate the thinking engine wants to perform
        // a destructive action. Must be multi-word to avoid false positives.
        static const std::vector<std::string> destructiveIntentPatterns = {
            "should restart", "need to restart", "restart the",
            "should delete", "need to delete", "delete the",
            "should remove", "need to remove", "remove the",
            "should deploy", "need to deploy", "deploy the",
            "should stop", "need to stop", "stop the",
            "should shut down", "shut down the", "shutdown the",
            "should reboot", "need to reboot", "reboot the",
            "should kill", "need to kill", "kill the",
            "should destroy", "need to destroy", "destroy the",
            "should wipe", "need to wipe", "wipe the",
            "should purge", "need to purge", "purge the",
            "should drop", "need to drop", "drop the",
        };

        for (const auto& pattern : destructiveIntentPatterns) {
            if (lower.find(pattern) != std::string::npos) {
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
            // We have real tool results — return them as the response
            response << actionResults;
        }
        // If no action results and only thinking reasoning exists, return EMPTY.
        // The thinking reasoning is INTERNAL context (instructions like "Provide a
        // friendly response...") and must NOT be shown to the user. Returning empty
        // signals to the caller that no actionable tool was executed, so the caller
        // should generate a proper user-facing response via the LLM path.

        return response.str();
    }

    // Main processing method: takes thinking output and produces final response
    // Returns non-empty ONLY when there are real tool results or graph facts to show.
    // Returns EMPTY for pure conversational actions — the caller must then generate
    // a natural user-facing response via the LLM path, using the thinking output as
    // internal context (NOT as the response itself).
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

        // Heuristic: if the user input is a simple greeting or small-talk, skip
        // tool dispatch entirely. The thinking engine's output for "Hello" might
        // incidentally contain words like "check" or "system" that would trigger
        // SYSTEM_INFO — we don't want that for casual conversation.
        bool isSimpleConversation = false;
        {
            std::string lower = toLower(userInput);
            int words = 0;
            bool inWord = false;
            for (char c : lower) {
                if (c == ' ' || c == '\t' || c == '\n') inWord = false;
                else if (!inWord) { inWord = true; words++; }
            }
            // Short inputs (<=4 words) without question marks that look like
            // greetings or small talk should not trigger tool dispatch.
            static const std::vector<std::string> greetingWords = {
                "hello", "hi", "hey", "yo", "sup", "what's up", "whats up",
                "what's good", "whats good", "howdy", "hola", "greetings",
                "good morning", "good evening", "good night", "heyy", "heyo",
                "ayy", "ayo"
            };
            if (words <= 4 && lower.find('?') == std::string::npos) {
                for (const auto& g : greetingWords) {
                    if (lower.find(g) != std::string::npos) {
                        isSimpleConversation = true;
                        break;
                    }
                }
            }
            // Also skip tool dispatch for very short non-question text
            // that doesn't explicitly ask for system/docker/ssh operations
            if (!isSimpleConversation && words <= 3 &&
                lower.find('?') == std::string::npos &&
                lower.find("docker") == std::string::npos &&
                lower.find("ssh") == std::string::npos &&
                lower.find("system") == std::string::npos &&
                lower.find("status") == std::string::npos) {
                isSimpleConversation = true;
            }
        }

        // For simple conversational inputs, return empty immediately.
        // This tells the caller: "No tool action needed, generate a natural
        // response via LLM using the thinking output as internal context."
        if (isSimpleConversation) {
            return "";
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

        // For conversational actions with graph facts, return those
        if (!graphFacts.empty()) {
            std::ostringstream factResponse;
            for (const auto& f : graphFacts) {
                factResponse << f << ". ";
            }
            return formatResponse(factResponse.str(), thinkingOutput, userInput);
        }

        // No tool results and no graph facts — return EMPTY.
        // The caller should generate a natural response via the LLM, using the
        // thinking output as internal reasoning context (NOT as the response).
        return "";
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
    // Matches if the input contains the registry keyword OR if the registry keyword contains the input
    MKActionType getActionForKeyword(const std::string& keyword) const {
        std::string lower = toLower(keyword);
        for (const auto& mapping : toolRegistry_) {
            if (lower.find(mapping.keyword) != std::string::npos ||
                mapping.keyword.find(lower) != std::string::npos) {
                return mapping.action;
            }
        }
        return MKActionType::NONE;
    }
};

#endif // MK_DECISION_ENGINE_CPP
