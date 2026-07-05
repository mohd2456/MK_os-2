// ============================================================
// MK OS - Orchestrator: Conversation Loop
// The single clean LLM-call architecture. All user messages flow
// through MKOrchestrator::respond() which:
//   1. Gathers context (knowledge graph facts, conversation history)
//   2. Builds ONE tight prompt (system + tools + facts + history + user)
//   3. Sends ONE LLM call (local first, then cloud)
//   4. Sanitizes the response
//   5. Detects tool-call JSON in output, executes tools, feeds results back
//   6. Returns the final response
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
//   MKLearningEngine, MKBiographicalExtractor,
//   MKToolRegistry, MKToolExecutor, MKCodeRunner, MKFileReader,
//   MKRealtimeAPIs

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

    // Tool framework (set by caller after construction)
    MKToolRegistry* toolRegistry = nullptr;
    MKToolExecutor* toolExecutor = nullptr;

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

        // Step 1b: Inject personal context (top personal facts about the user)
        std::vector<std::string> personalFacts = gatherPersonalFacts(3);
        for (const auto& pf : personalFacts) {
            // Only add if not already in relevant facts
            bool exists = false;
            for (const auto& rf : relevantFacts) {
                if (rf == pf) { exists = true; break; }
            }
            if (!exists) {
                relevantFacts.insert(relevantFacts.begin(), pf);
            }
        }

        // Step 2: Get conversation history
        std::string history;
        if (brainMemory) {
            history = brainMemory->getContextString();
        }

        // Step 3: Build the prompt (includes tool descriptions if available)
        std::string prompt = buildPrompt(userInput, relevantFacts, history);

        // Step 4: Send ONE LLM call (local first, then cloud)
        std::string response = callLLM(userInput, prompt, relevantFacts, history);

        // Step 5: Sanitize (defined at file scope in mk_entry.cpp)
        if (!response.empty()) {
            response = sanitizeLLMResponse(response, userInput);
        }

        // Step 6: Check for tool-call markers and execute (max 3 per turn)
        if (config.enableToolCalls && !response.empty()) {
            int toolCallCount = 0;
            while (toolCallCount < MK_TOOL_MAX_CALLS_PER_TURN) {
                MKParsedToolCall parsedCall = parseToolCallFromResponse(response);
                if (!parsedCall.valid) break;

                std::string toolResult = executeToolFromParsed(parsedCall);
                if (toolResult.empty()) break;

                // Feed tool result back for synthesis
                response = synthesizeWithToolResult(userInput, response, toolResult,
                                                    parsedCall, relevantFacts, history);
                toolCallCount++;

                // If synthesis failed or next response has no more tool calls, stop
                if (response.empty()) break;
            }
        }

        // Step 6b: Strip any remaining raw tool-call JSON from the response
        // This prevents leaking {"tool": ...} output to the user
        if (!response.empty()) {
            response = stripToolCallJson(response);
        }

        // Step 7: If LLM failed entirely, use honest fallback
        if (response.empty()) {
            response = buildFallbackResponse(relevantFacts);
        }

        // Step 8: Record interaction in memory
        if (brainMemory) {
            brainMemory->commitToShortTerm("user", userInput);
            brainMemory->commitToShortTerm("mk", response);
        }

        // Step 9: Passively extract biographical facts
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
    // gatherPersonalFacts() - Query learning engine for personal facts
    // Returns top personal facts about Mohammed/user for prompt injection
    // ============================================================
    std::vector<std::string> gatherPersonalFacts(size_t maxFacts = 5) {
        std::vector<std::string> personalFacts;
        if (!learningEngine) return personalFacts;

        // Query facts indexed under "user" and "mohammed"
        auto userFacts = learningEngine->queryFacts("user");
        auto mohammedFacts = learningEngine->queryFacts("mohammed");

        // Merge and deduplicate
        std::vector<MKFact> allPersonal;
        allPersonal.insert(allPersonal.end(), userFacts.begin(), userFacts.end());
        allPersonal.insert(allPersonal.end(), mohammedFacts.begin(), mohammedFacts.end());

        // Sort by confidence (highest first), then by access count
        std::sort(allPersonal.begin(), allPersonal.end(),
                  [](const MKFact& a, const MKFact& b) {
                      if (a.confidence != b.confidence)
                          return (int)a.confidence > (int)b.confidence;
                      return a.accessCount > b.accessCount;
                  });

        // Format as terse strings
        for (const auto& fact : allPersonal) {
            if (personalFacts.size() >= maxFacts) break;
            std::string formatted = fact.subject + " " + fact.predicate + " " + fact.object;
            // Avoid duplicates
            bool duplicate = false;
            for (const auto& existing : personalFacts) {
                if (existing == formatted) { duplicate = true; break; }
            }
            if (!duplicate) {
                personalFacts.push_back(formatted);
            }
        }

        return personalFacts;
    }

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
    // buildPrompt() - Construct one tight prompt for the LLM
    // Uses MKContextBuilder for token-efficient prompt construction
    // ============================================================
    std::string buildPrompt(const std::string& input,
                            const std::vector<std::string>& facts,
                            const std::string& history) {
        MKContextBuilder builder;

        // Get tool prompt if available
        std::string toolPromptStr;
        if (toolRegistry && config.enableToolCalls) {
            toolPromptStr = toolRegistry->buildToolPrompt();
        }

        // Get the system prompt
        std::string sysPrompt;
        if (systemPrompt) {
            sysPrompt = *systemPrompt;
        }

        // Build optimized prompt via ContextBuilder
        MKContextBuilder::PromptResult result = builder.buildPrompt(
            input,
            facts,
            history,
            "",  // no tool context for initial prompt
            sysPrompt,
            toolPromptStr,
            1500  // default token budget
        );

        return result.prompt;
    }

    // ============================================================
    // callLLM() - Try local LLM first, then cloud. ONE call.
    // ============================================================
    std::string callLLM(const std::string& input,
                        const std::string& prompt,
                        const std::vector<std::string>& facts,
                        const std::string& history) {
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
        // Use the same prompt that MKContextBuilder assembled (respects token budget)
        // We call generate(userPrompt, systemPrompt) directly so the pre-built
        // prompt is sent as-is, avoiding double-assembly of facts/history.
        if (cloudLLM && cloudLLM->isAvailable()) {
            auto genStart = std::chrono::steady_clock::now();
            response = cloudLLM->generate(input, prompt);
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
    // parseToolCallFromResponse() - Use MKToolRegistry to parse
    // ============================================================
    MKParsedToolCall parseToolCallFromResponse(const std::string& response) {
        if (toolRegistry) {
            return toolRegistry->parseToolCall(response);
        }
        MKParsedToolCall empty;
        return empty;
    }

    // ============================================================
    // executeToolFromParsed() - Use MKToolExecutor to dispatch
    // ============================================================
    std::string executeToolFromParsed(const MKParsedToolCall& call) {
        if (!toolExecutor || !call.valid) return "";

        MKToolResult result = toolExecutor->execute(call);
        if (result.success) {
            return result.output;
        } else {
            return "Error: " + result.error;
        }
    }

    // ============================================================
    // synthesizeWithToolResult() - Feed tool result back to LLM
    // ============================================================
    std::string synthesizeWithToolResult(const std::string& input,
                                         const std::string& originalResponse,
                                         const std::string& toolResult,
                                         const MKParsedToolCall& call,
                                         const std::vector<std::string>& facts,
                                         const std::string& history) {
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
        prompt += "Tool '" + call.tool + "' returned: " + toolResult + "\n";
        prompt += "MK (use the tool result to answer the user naturally):";

        std::string response;
        if (llmEngine && llmEngine->isAvailable()) {
            response = llmEngine->generate(prompt);
        }
        if (response.empty() && cloudLLM && cloudLLM->isAvailable()) {
            // Use generate() directly with the already-assembled prompt as systemPrompt.
            // This avoids double-assembly of facts/history that generateWithContext() does.
            response = cloudLLM->generate(input, prompt);
        }

        if (!response.empty()) {
            response = sanitizeLLMResponse(response, input);
        }

        // If synthesis failed, strip the tool call JSON and append result
        if (response.empty()) {
            std::string cleaned = originalResponse;
            if (!call.rawJson.empty()) {
                size_t jsonPos = cleaned.find(call.rawJson);
                if (jsonPos != std::string::npos) {
                    cleaned.erase(jsonPos, call.rawJson.size());
                }
            }
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
        if (!relevantFacts.empty()) {
            std::string factBased;
            for (size_t i = 0; i < relevantFacts.size() && i < 3; i++) {
                if (!factBased.empty()) factBased += " Also, ";
                factBased += relevantFacts[i];
            }
            factBased += ".";
            return "Here's what I know: " + factBased;
        }

        return "I can't reach my language model right now, so I can't answer that "
               "properly. Try again in a moment, or check /status for provider health.";
    }

    // ============================================================
    // stripToolCallJson() - Remove any raw tool-call JSON from text
    // Looks for {"tool": ...} patterns and removes the entire JSON object.
    // This prevents debug/tool JSON from leaking to the user.
    // ============================================================
    std::string stripToolCallJson(const std::string& text) {
        std::string result = text;

        // Repeatedly find and remove tool-call JSON objects
        while (true) {
            // Find any tool-call pattern
            size_t pos = result.find("{\"tool\":");
            if (pos == std::string::npos) pos = result.find("{ \"tool\":");
            if (pos == std::string::npos) pos = result.find("{\"tool\" :");
            if (pos == std::string::npos) pos = result.find("{  \"tool\":");
            if (pos == std::string::npos) pos = result.find("{\n\"tool\":");
            if (pos == std::string::npos) pos = result.find("{\n  \"tool\":");
            if (pos == std::string::npos) break;

            // Find the matching closing brace by tracking depth
            int braceDepth = 0;
            size_t endPos = pos;
            bool found = false;
            for (size_t i = pos; i < result.size(); i++) {
                if (result[i] == '{') braceDepth++;
                else if (result[i] == '}') {
                    braceDepth--;
                    if (braceDepth == 0) {
                        endPos = i + 1;
                        found = true;
                        break;
                    }
                }
            }

            if (!found) break; // Malformed JSON, stop trying

            // Remove the JSON object and any surrounding whitespace/newlines
            result.erase(pos, endPos - pos);

            // Clean up any leftover whitespace at the removal point
            while (pos < result.size() && (result[pos] == ' ' || result[pos] == '\t' || result[pos] == '\n' || result[pos] == '\r')) {
                result.erase(pos, 1);
            }
        }

        // Also strip markdown code fences that may have wrapped tool calls
        // e.g. ```json\n...\n```
        while (true) {
            size_t fenceStart = result.find("```json");
            if (fenceStart == std::string::npos) fenceStart = result.find("```");
            if (fenceStart == std::string::npos) break;

            size_t contentStart = result.find('\n', fenceStart);
            if (contentStart == std::string::npos) break;
            contentStart++;

            size_t fenceEnd = result.find("```", contentStart);
            if (fenceEnd == std::string::npos) break;

            // Check if the content between fences looks like a tool call
            std::string fenceContent = result.substr(contentStart, fenceEnd - contentStart);
            if (fenceContent.find("\"tool\"") != std::string::npos) {
                // Remove the entire fenced block
                size_t removeEnd = fenceEnd + 3;
                // Skip trailing newline
                if (removeEnd < result.size() && result[removeEnd] == '\n') removeEnd++;
                result.erase(fenceStart, removeEnd - fenceStart);
            } else {
                break; // Not a tool call fence, stop
            }
        }

        // Return result without global trimming to preserve multi-paragraph whitespace
        return result;
    }
};

#endif // MK_ORCHESTRATOR_CPP
