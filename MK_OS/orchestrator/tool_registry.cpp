// ============================================================
// MK OS - Tool Registry
// Defines available tools the LLM can invoke via structured
// JSON tool calls. Each tool has a name, description, parameter
// schema (for the system prompt), and dispatch info.
// ============================================================
#ifndef MK_TOOL_REGISTRY_CPP
#define MK_TOOL_REGISTRY_CPP

#include <string>
#include <vector>
#include <map>
#include <functional>

// ============================================================
// Tool Definition
// ============================================================
struct MKToolDef {
    std::string name;
    std::string description;
    std::string paramSchema; // Human-readable for the LLM prompt
    bool requiresArgs;
};

// ============================================================
// Parsed Tool Call (from LLM JSON output)
// ============================================================
struct MKParsedToolCall {
    std::string tool;
    std::map<std::string, std::string> args;
    bool valid = false;
    std::string rawJson;
};

// ============================================================
// Tool Result
// ============================================================
struct MKToolResult {
    bool success;
    std::string output;
    std::string error;
};

// ============================================================
// MKToolRegistry
// ============================================================
class MKToolRegistry {
private:
    std::vector<MKToolDef> tools;
    std::map<std::string, size_t> nameIndex; // name -> index in tools vector

public:
    MKToolRegistry() {
        registerDefaultTools();
    }

    // Register a tool
    void registerTool(const std::string& name, const std::string& description,
                      const std::string& paramSchema, bool requiresArgs = true) {
        MKToolDef def;
        def.name = name;
        def.description = description;
        def.paramSchema = paramSchema;
        def.requiresArgs = requiresArgs;
        nameIndex[name] = tools.size();
        tools.push_back(def);
    }

    // Look up a tool by name
    const MKToolDef* lookup(const std::string& name) const {
        auto it = nameIndex.find(name);
        if (it == nameIndex.end()) return nullptr;
        return &tools[it->second];
    }

    // Check if a tool exists
    bool exists(const std::string& name) const {
        return nameIndex.find(name) != nameIndex.end();
    }

    // Get number of registered tools
    int toolCount() const { return static_cast<int>(tools.size()); }

    // Get all tool names
    std::vector<std::string> getToolNames() const {
        std::vector<std::string> names;
        for (const auto& t : tools) {
            names.push_back(t.name);
        }
        return names;
    }

    // Build system prompt section describing available tools
    std::string buildToolPrompt() const {
        if (tools.empty()) return "";

        std::string prompt;
        prompt += "\n\nYou have access to these tools. To use one, output ONLY a JSON object on its own line:\n";
        prompt += "{\"tool\": \"<name>\", \"args\": {<arguments>}}\n\n";
        prompt += "Available tools:\n";

        for (const auto& t : tools) {
            prompt += "- " + t.name + ": " + t.description;
            if (!t.paramSchema.empty()) {
                prompt += " Args: " + t.paramSchema;
            }
            prompt += "\n";
        }

        prompt += "\nOnly call a tool if it is needed to answer the user. ";
        prompt += "If you can answer directly from your knowledge, do so without using a tool.\n";

        return prompt;
    }

    // ============================================================
    // Parse a tool call from LLM output
    // Looks for: {"tool": "name", "args": {...}}
    // ============================================================
    MKParsedToolCall parseToolCall(const std::string& response) const {
        MKParsedToolCall result;

        // Look for tool call pattern
        size_t toolStart = response.find("{\"tool\":");
        if (toolStart == std::string::npos) {
            toolStart = response.find("{ \"tool\":");
        }
        if (toolStart == std::string::npos) {
            toolStart = response.find("{\"tool\" :");
        }
        if (toolStart == std::string::npos) return result;

        // Find matching closing brace
        int braceDepth = 0;
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
        result.rawJson = json;

        // Extract tool name
        result.tool = extractJsonString(json, "tool");
        if (result.tool.empty()) return result;

        // Check tool exists
        if (!exists(result.tool)) return result;

        // Extract args object
        size_t argsPos = json.find("\"args\"");
        if (argsPos != std::string::npos) {
            size_t argsObjStart = json.find('{', argsPos);
            if (argsObjStart != std::string::npos) {
                // Find the matching close brace for the args object
                int depth = 0;
                size_t argsObjEnd = argsObjStart;
                for (size_t i = argsObjStart; i < json.size(); i++) {
                    if (json[i] == '{') depth++;
                    else if (json[i] == '}') {
                        depth--;
                        if (depth == 0) {
                            argsObjEnd = i + 1;
                            break;
                        }
                    }
                }

                std::string argsJson = json.substr(argsObjStart, argsObjEnd - argsObjStart);
                result.args = parseArgsObject(argsJson);
            }
        }

        result.valid = true;
        return result;
    }

private:
    // Register the default set of MK tools
    void registerDefaultTools() {
        registerTool("ssh_exec",
                     "Run a command on a registered homelab device via SSH.",
                     "{\"device\": \"<hostname>\", \"command\": \"<shell command>\"}",
                     true);

        registerTool("docker_cmd",
                     "Manage Docker containers on a device (list, start, stop, logs).",
                     "{\"device\": \"<hostname>\", \"action\": \"list|start|stop|logs\", \"container\": \"<name>\"}",
                     true);

        registerTool("local_shell",
                     "Run a local shell command (with safety checks).",
                     "{\"command\": \"<shell command>\"}",
                     true);

        registerTool("read_file",
                     "Read the contents of a file.",
                     "{\"path\": \"<file path>\"}",
                     true);

        registerTool("write_file",
                     "Write content to a file (limited to safe directories).",
                     "{\"path\": \"<file path>\", \"content\": \"<text>\"}",
                     true);

        registerTool("web_search",
                     "Get current information from the internet (news, weather, time).",
                     "{\"query\": \"<search query>\"}",
                     true);

        registerTool("system_info",
                     "Get local system resource info (CPU, RAM, temperature).",
                     "{}",
                     false);

        registerTool("device_list",
                     "List all registered homelab devices and their status.",
                     "{}",
                     false);

        registerTool("learn_fact",
                     "Teach MK a new fact to remember permanently.",
                     "{\"subject\": \"<entity>\", \"relation\": \"<relationship>\", \"object\": \"<value>\"}",
                     true);
    }

    // Extract a string value from a JSON object for a given key
    std::string extractJsonString(const std::string& json, const std::string& key) const {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";

        pos += search.size();
        // Skip whitespace and colon
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t'))
            pos++;
        if (pos >= json.size()) return "";

        if (json[pos] != '"') return "";
        pos++; // skip opening quote

        std::string value;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                pos++;
                if (json[pos] == 'n') value += '\n';
                else if (json[pos] == 't') value += '\t';
                else if (json[pos] == '"') value += '"';
                else if (json[pos] == '\\') value += '\\';
                else value += json[pos];
            } else {
                value += json[pos];
            }
            pos++;
        }
        return value;
    }

    // Parse a JSON args object into key-value string pairs
    std::map<std::string, std::string> parseArgsObject(const std::string& json) const {
        std::map<std::string, std::string> result;

        // Simple approach: find all "key": "value" pairs
        size_t pos = 1; // skip opening brace
        while (pos < json.size()) {
            // Find next key
            size_t keyStart = json.find('"', pos);
            if (keyStart == std::string::npos || keyStart >= json.size() - 1) break;
            keyStart++;
            size_t keyEnd = json.find('"', keyStart);
            if (keyEnd == std::string::npos) break;

            std::string key = json.substr(keyStart, keyEnd - keyStart);
            pos = keyEnd + 1;

            // Skip to colon and whitespace
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t'))
                pos++;

            if (pos >= json.size()) break;

            // Extract value (handle strings and simple values)
            if (json[pos] == '"') {
                pos++; // skip opening quote
                std::string value;
                while (pos < json.size() && json[pos] != '"') {
                    if (json[pos] == '\\' && pos + 1 < json.size()) {
                        pos++;
                        if (json[pos] == 'n') value += '\n';
                        else if (json[pos] == 't') value += '\t';
                        else if (json[pos] == '"') value += '"';
                        else if (json[pos] == '\\') value += '\\';
                        else value += json[pos];
                    } else {
                        value += json[pos];
                    }
                    pos++;
                }
                if (pos < json.size()) pos++; // skip closing quote
                result[key] = value;
            } else {
                // Non-string value (number, bool, null)
                size_t valStart = pos;
                while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != ' ')
                    pos++;
                result[key] = json.substr(valStart, pos - valStart);
            }

            // Skip comma
            while (pos < json.size() && (json[pos] == ',' || json[pos] == ' ' || json[pos] == '\t'))
                pos++;
        }

        return result;
    }
};

#endif // MK_TOOL_REGISTRY_CPP
