// ============================================================
// MK OS - Orchestrator: Conversation Loop
// The single clean LLM-call architecture. All user messages flow
// through MKOrchestrator::respond() which:
//   1. Gathers context (knowledge graph facts, conversation history)
//   2. Checks for tool-call opportunities (system info, devices, etc.)
//   3. Builds ONE tight prompt (system + facts + history + user msg)
//   4. Sends ONE LLM call (local first, then cloud)
//   5. Sanitizes the response
//   6. Detects tool-call markers in the LLM output for execution
//   7. Returns the final response
// ============================================================
#ifndef MK_ORCHESTRATOR_CPP
#define MK_ORCHESTRATOR_CPP

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <chrono>

// ============================================================
// MKOrchestrator
// Replaces the old multi-engine routing (SmartRouter, DeepReasoner,
// ReasoningChains, Composer, NeuralNet, IdeaEngine, ConversationMode,
// KnowledgeIntegrator, SelfImprover). All reasoning is delegated to
// the LLM; MK controls what context it sees and what tools it can use.
// ============================================================

// Forward declarations - these are defined in other compilation units
// that are #included before this file in mk_entry.cpp:
//   MKPatternGraph, MKBrainMemory, MKCloudLLM, MKLLMEngine,
//   MKProviderRouter, MKRequestLogger, MKResourceMonitor,
//   MKDeviceRegistry, MKSSHController, MKDockerManager,
//   MKLearningEngine, MKBiographicalExtractor

// Tool call result from LLM output parsing
struct MKToolCall {
    std::string tool;
    std::string args;
    bool valid = false;
};

// Orchestrator configuration
struct MKOrchestratorConfig {
    size_t maxFacts = 5;         // Max knowledge graph facts to include
    size_t maxHistoryTurns = 10; // Max conversation history turns
    bool enableToolCalls = true; // Whether to detect/execute tool calls
};

class MKOrchestrator {
public:
    // References to the subsystems the orchestrator coordinates
    // These are set by the caller (MKSystem) after construction.
    MKPatternGraph* graph = nullptr;
    MKBrainMemory* brainMemory = nullptr;
    MKCloudLLM* cloudLLM = nullptr;
    MKLLMEngine* llmEngine = nullptr;
    MKProviderRouter* providerRouter = nullptr;
    MKRequestLogger* requestLogger = nullptr;
    MKResourceMonitor* resourceMonitor = nullptr;
    MKDeviceRegistry* deviceRegistry = nullptr;
    MKLearningEngine* learningEngine = nullptr;
    MKBiographicalExtractor* factExtractor = nullptr;

    // The system prompt (shared with the rest of MK)
    const std::string* systemPrompt = nullptr;

    MKOrchestratorConfig config;

    MKOrchestrator() = default;

    // ============================================================
    // respond() - The single entry point for all natural language
    // ============================================================
    std::string respond(const std::string& userInput) {
        if (userInput.empty()) return "";

        // Step 1: Gather relevant facts from knowledge graph
        std::vector<std::string> relevantFacts = gatherFacts(userInput);

        // Step 2: Get conversation history
        std::string history;
        if (brainMemory) {
            history = brainMemory->getContextString();
        }

        // Step 3: Get tool/system context if the query touches those topics
        std::string toolContext = gatherToolContext(userInput);

        // Step 4: Build the prompt
        std::string prompt = buildPrompt(userInput, relevantFacts, history, toolContext);

        // Step 5: Send ONE LLM call (local first, then cloud)
        std::string response = callLLM(userInput, prompt, relevantFacts, history, toolContext);

        // Step 6: Sanitize (defined at file scope in mk_entry.cpp)
        if (!response.empty()) {
            response = sanitizeLLMResponse(response, userInput);
        }

        // Step 7: Check for tool-call markers in the response
        if (config.enableToolCalls && !response.empty()) {
            MKToolCall toolCall = detectToolCall(response);
            if (toolCall.valid) {
                std::string toolResult = executeToolCall(toolCall);
                if (!toolResult.empty()) {
                    // Feed tool result back for synthesis
                    response = synthesizeWithToolResult(userInput, response, toolResult,
                                                        relevantFacts, history);
                }
            }
        }

        // Step 8: If LLM failed entirely, use honest fallback
        if (response.empty()) {
            response = buildFallbackResponse(relevantFacts);
        }

        // Step 9: Record interaction in memory
        if (brainMemory) {
            brainMemory->commitToShortTerm("user", userInput);
            brainMemory->commitToShortTerm("mk", response);
        }

        // Step 10: Passively extract biographical facts
        if (factExtractor) {
            factExtractor->extractFromMessage(userInput);
        }

        return response;
    }

    // ============================================================
    // respondForTelegram() - Same as respond() but with latency logging
    // ============================================================
    std::string respondForTelegram(const std::string& userInput) {
        auto startTime = std::chrono::steady_clock::now();

        std::string reply = respond(userInput);

        auto endTime = std::chrono::steady_clock::now();
        float latencyMs = (float)std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime).count();

        // Log the request
        if (providerRouter && requestLogger) {
            int tokenEstimate = (int)userInput.size() / 4;
            std::string provider = providerRouter->pickProvider(
                MKRoutingUrgency::HIGH, tokenEstimate);
            if (!provider.empty()) {
                requestLogger->logRequest(provider, userInput.substr(0, 40),
                                          tokenEstimate, latencyMs, !reply.empty());
            }
        }

        if (reply.empty()) {
            reply = "I'm processing that but couldn't generate a response. Try again or rephrase.";
        }

        return reply;
    }

private:
    // ============================================================
    // gatherFacts() - Query knowledge graph for relevant context
    // ============================================================
    std::vector<std::string> gatherFacts(const std::string& input) {
        if (!graph) return {};

        // Get all edges matching the input keywords
        auto edges = graph->getAll(input);
        std::vector<std::string> rawFacts;
        for (const auto& e : edges) {
            rawFacts.push_back(e.source + " " + e.relation + " " + e.target);
        }

        // Filter for relevance (defined at file scope in mk_entry.cpp)
        return filterRelevantFacts(input, rawFacts, config.maxFacts);
    }

    // ============================================================
    // gatherToolContext() - Check if query touches system/device topics
    // ============================================================
    std::string gatherToolContext(const std::string& input) {
        std::string lower = input;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string context;

        // System/resource queries
        if (lower.find("system") != std::string::npos || lower.find("cpu") != std::string::npos ||
            lower.find("ram") != std::string::npos || lower.find("memory") != std::string::npos ||
            lower.find("resource") != std::string::npos || lower.find("temperature") != std::string::npos) {
            if (resourceMonitor) {
                auto res = resourceMonitor->getLocalResources();
                if (res.valid) {
                    context += "System resources: CPU " + std::to_string((int)res.cpu_usage_percent) + "%, "
                             + "RAM " + std::to_string(res.available_ram_mb) + "/"
                             + std::to_string(res.total_ram_mb) + "MB, "
                             + "Temp " + std::to_string((int)res.temperature_celsius) + "C. ";
                }
            }
        }

        // Docker/container queries
        if (lower.find("docker") != std::string::npos || lower.find("container") != std::string::npos) {
            context += "Docker manager is available. Use SSH controller to run docker commands on devices. ";
        }

        // Device/homelab queries
        if (lower.find("device") != std::string::npos || lower.find("homelab") != std::string::npos ||
            lower.find("server") != std::string::npos) {
            if (deviceRegistry) {
                auto devices = deviceRegistry->getOnlineDevices();
                int total = deviceRegistry->deviceCount();
                if (total > 0) {
                    context += "Homelab: " + std::to_string(total) + " devices registered, "
                             + std::to_string(devices.size()) + " online. ";
                }
            }
        }

        return context;
    }

    // ============================================================
    // buildPrompt() - Construct one tight prompt for the LLM
    // ============================================================
    std::string buildPrompt(const std::string& input,
                            const std::vector<std::string>& facts,
                            const std::string& history,
                            const std::string& toolContext) {
        std::string prompt;
        if (systemPrompt) {
            prompt = *systemPrompt;
        }

        // Add tool context if available
        if (!toolContext.empty()) {
            prompt += "\n\nSystem info: " + toolContext;
        }

        prompt += "\n\n";

        // Add relevant facts (token-efficient: only include if non-empty)
        if (!facts.empty()) {
            prompt += "Known facts:\n";
            for (const auto& f : facts) {
                prompt += "- " + f + "\n";
            }
            prompt += "\n";
        }

        // Add conversation history (token-efficient: skip if empty)
        if (!history.empty()) {
            prompt += "Recent conversation:\n" + history + "\n";
        }

        // User message and response marker
        prompt += "User: " + input + "\nMK:";

        return prompt;
    }

    // ============================================================
    // callLLM() - Try local LLM first, then cloud. ONE call.
    // ============================================================
    std::string callLLM(const std::string& input,
                        const std::string& prompt,
                        const std::vector<std::string>& facts,
                        const std::string& history,
                        const std::string& toolContext) {
        std::string response;
        bool llmConfigured = (llmEngine && llmEngine->isAvailable()) ||
                             (cloudLLM && cloudLLM->isAvailable());

        if (!llmConfigured) return "";

        // Try local LLM first (fastest, free, no rate limits)
        if (llmEngine && llmEngine->isAvailable()) {
            auto genStart = std::chrono::steady_clock::now();
            response = llmEngine->generate(prompt);
            auto genEnd = std::chrono::steady_clock::now();
            float genLatency = (float)std::chrono::duration_cast<std::chrono::milliseconds>(
                genEnd - genStart).count();
            if (requestLogger) {
                requestLogger->logRequest("local", "generate: " + input.substr(0, 40),
                                          (int)prompt.size() / 4, genLatency, !response.empty());
            }
            if (!response.empty()) return response;
        }

        // Fall back to cloud LLM
        if (cloudLLM && cloudLLM->isAvailable()) {
            std::string enhancedPrompt;
            if (systemPrompt) {
                enhancedPrompt = *systemPrompt;
                if (!toolContext.empty()) {
                    enhancedPrompt += "\n\nSystem info: " + toolContext;
                }
            }

            auto genStart = std::chrono::steady_clock::now();
            response = cloudLLM->generateWithContext(input, facts, history, toolContext, enhancedPrompt);
            auto genEnd = std::chrono::steady_clock::now();
            float genLatency = (float)std::chrono::duration_cast<std::chrono::milliseconds>(
                genEnd - genStart).count();
            if (requestLogger) {
                std::string cloudProvider = cloudLLM->getProviderName();
                requestLogger->logRequest(cloudProvider, "cloud: " + input.substr(0, 40),
                                          (int)input.size() / 4, genLatency, !response.empty());
            }
        }

        return response;
    }

    // ============================================================
    // detectToolCall() - Parse tool-call markers from LLM output
    // Looks for JSON blocks: {"tool": "ssh", "args": "..."}
    // ============================================================
    MKToolCall detectToolCall(const std::string& response) {
        MKToolCall result;

        // Look for tool call pattern: ```tool\n{"tool":"...", "args":"..."}\n```
        // or inline {"tool":"...", "args":"..."}
        size_t toolStart = response.find("{\"tool\":");
        if (toolStart == std::string::npos) {
            toolStart = response.find("{ \"tool\":");
        }
        if (toolStart == std::string::npos) return result;

        // Find the closing brace
        size_t braceDepth = 0;
        size_t toolEnd = toolStart;
        for (size_t i = toolStart; i < response.size(); i++) {
            if (response[i] == '{') braceDepth++;
            else if (response[i] == '}') {
                braceDepth--;
                if (braceDepth == 0) {
                    toolEnd = i + 1;
                    break;
                }
            }
        }

        if (toolEnd <= toolStart) return result;

        std::string json = response.substr(toolStart, toolEnd - toolStart);

        // Simple manual JSON parse for "tool" and "args" fields
        size_t toolFieldPos = json.find("\"tool\"");
        if (toolFieldPos == std::string::npos) return result;

        // Extract tool name
        size_t colonPos = json.find(':', toolFieldPos + 6);
        if (colonPos == std::string::npos) return result;
        size_t nameStart = json.find('"', colonPos + 1);
        if (nameStart == std::string::npos) return result;
        nameStart++;
        size_t nameEnd = json.find('"', nameStart);
        if (nameEnd == std::string::npos) return result;
        result.tool = json.substr(nameStart, nameEnd - nameStart);

        // Extract args
        size_t argsFieldPos = json.find("\"args\"");
        if (argsFieldPos != std::string::npos) {
            size_t argsColon = json.find(':', argsFieldPos + 6);
            if (argsColon != std::string::npos) {
                size_t argsStart = json.find('"', argsColon + 1);
                if (argsStart != std::string::npos) {
                    argsStart++;
                    size_t argsEnd = json.find('"', argsStart);
                    if (argsEnd != std::string::npos) {
                        result.args = json.substr(argsStart, argsEnd - argsStart);
                    }
                }
            }
        }

        result.valid = !result.tool.empty();
        return result;
    }

    // ============================================================
    // executeToolCall() - Execute a detected tool call
    // Currently supports: system_info, devices, docker_ps
    // Future: ssh, file_ops, web_search
    // ============================================================
    std::string executeToolCall(const MKToolCall& call) {
        if (call.tool == "system_info") {
            if (resourceMonitor) {
                auto res = resourceMonitor->getLocalResources();
                if (res.valid) {
                    return "CPU: " + std::to_string((int)res.cpu_usage_percent) + "%, "
                         + "RAM: " + std::to_string(res.available_ram_mb) + "/"
                         + std::to_string(res.total_ram_mb) + "MB, "
                         + "Temp: " + std::to_string((int)res.temperature_celsius) + "C";
                }
            }
            return "System info unavailable.";
        }

        if (call.tool == "devices") {
            if (deviceRegistry) {
                auto devices = deviceRegistry->getOnlineDevices();
                int total = deviceRegistry->deviceCount();
                std::string info = std::to_string(total) + " devices registered, "
                                 + std::to_string(devices.size()) + " online.";
                for (const auto& dev : devices) {
                    info += " " + dev.hostname + "(" + dev.ip + ")";
                }
                return info;
            }
            return "No devices registered.";
        }

        // Future tool calls will be added here (ssh, docker, file_ops, web_search)
        return "";
    }

    // ============================================================
    // synthesizeWithToolResult() - Feed tool result back to LLM
    // ============================================================
    std::string synthesizeWithToolResult(const std::string& input,
                                         const std::string& originalResponse,
                                         const std::string& toolResult,
                                         const std::vector<std::string>& facts,
                                         const std::string& history) {
        // Build a follow-up prompt with the tool result
        std::string prompt;
        if (systemPrompt) prompt = *systemPrompt;
        prompt += "\n\n";
        if (!facts.empty()) {
            prompt += "Known facts:\n";
            for (const auto& f : facts) prompt += "- " + f + "\n";
            prompt += "\n";
        }
        if (!history.empty()) prompt += "Recent conversation:\n" + history + "\n";
        prompt += "User: " + input + "\n";
        prompt += "Tool result: " + toolResult + "\n";
        prompt += "MK (use the tool result to answer naturally):";

        std::string response;
        if (llmEngine && llmEngine->isAvailable()) {
            response = llmEngine->generate(prompt);
        }
        if (response.empty() && cloudLLM && cloudLLM->isAvailable()) {
            response = cloudLLM->generateWithContext(
                input + "\nTool result: " + toolResult,
                facts, history, toolResult,
                systemPrompt ? *systemPrompt : "");
        }

        if (!response.empty()) {
            response = sanitizeLLMResponse(response, input);
        }

        // If synthesis failed, strip the tool call JSON from original and append result
        if (response.empty()) {
            // Remove the JSON tool call block from the original response
            std::string cleaned = originalResponse;
            size_t jsonStart = cleaned.find("{\"tool\":");
            if (jsonStart == std::string::npos) jsonStart = cleaned.find("{ \"tool\":");
            if (jsonStart != std::string::npos) {
                size_t jsonEnd = cleaned.find('}', jsonStart);
                if (jsonEnd != std::string::npos) {
                    cleaned = cleaned.substr(0, jsonStart) + cleaned.substr(jsonEnd + 1);
                }
            }
            // Trim and append tool result
            size_t start = cleaned.find_first_not_of(" \t\n\r");
            size_t end = cleaned.find_last_not_of(" \t\n\r");
            if (start != std::string::npos) {
                cleaned = cleaned.substr(start, end - start + 1);
            } else {
                cleaned = "";
            }
            if (!cleaned.empty()) {
                return cleaned + "\n\n" + toolResult;
            }
            return toolResult;
        }

        return response;
    }

    // ============================================================
    // buildFallbackResponse() - Honest degradation when LLM is down
    // ============================================================
    std::string buildFallbackResponse(const std::vector<std::string>& relevantFacts) {
        // If we have relevant facts from the graph, present them simply
        if (!relevantFacts.empty()) {
            std::string factBased;
            for (size_t i = 0; i < relevantFacts.size() && i < 3; i++) {
                if (!factBased.empty()) factBased += " Also, ";
                factBased += relevantFacts[i];
            }
            factBased += ".";
            return "Here's what I know: " + factBased;
        }

        // No facts, no LLM - be honest
        return "I can't reach my language model right now, so I can't answer that "
               "properly. Try again in a moment, or check /status for provider health.";
    }
};

#endif // MK_ORCHESTRATOR_CPP
