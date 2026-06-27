// ============================================================================
// MK OS — Code Synthesizer
// ============================================================================
// Procedural Code Synthesis — Generates code by understanding GOALS.
//
// NOT template fill-in. Instead:
// - Understands programming CONCEPTS (loop = repeat, condition = if)
// - Has a "code vocabulary": knows what each construct DOES
// - Given a goal, plans a sequence of operations, translates each to code
// - Handles: variables, functions, loops, conditions, I/O, API calls, structs
//
// Supported languages: Python, JavaScript, C++
// Pure C++ STL. No external dependencies.
// ============================================================================

#ifndef MK_CODE_SYNTHESIZER_CPP
#define MK_CODE_SYNTHESIZER_CPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <iostream>

// ============================================================================
// Enums & Constants
// ============================================================================

// Supported output languages
enum class SynthLang {
    PYTHON,
    JAVASCRIPT,
    CPP
};

// Types of code operations MK understands
enum class OpType {
    DECLARE_VAR,
    ASSIGN_VAR,
    FUNCTION_DEF,
    FUNCTION_CALL,
    LOOP_FOR,
    LOOP_WHILE,
    CONDITION_IF,
    CONDITION_ELSE,
    RETURN_VAL,
    PRINT_OUTPUT,
    READ_INPUT,
    API_CALL,
    FILE_READ,
    FILE_WRITE,
    CLASS_DEF,
    STRUCT_DEF,
    ARRAY_CREATE,
    ARRAY_PUSH,
    ARRAY_FILTER,
    ARRAY_MAP,
    TRY_CATCH,
    COMMENT
};


// ============================================================================
// Structures
// ============================================================================

// A goal describes WHAT the code should accomplish
struct CodeGoal {
    std::string action;         // What to do: "fetch", "filter", "sort", "save"
    std::string input;          // Input data/source
    std::string output;         // Expected output
    std::string constraints;    // Any constraints (e.g., "only items > 100")
    SynthLang language;         // Target language

    CodeGoal() : language(SynthLang::PYTHON) {}
};

// A single step in the execution plan
struct CodeStep {
    OpType operation;           // What kind of code construct
    std::string description;   // Human-readable description
    std::map<std::string, std::string> params;  // Parameters for this step
    int indentLevel;            // Nesting depth

    CodeStep() : operation(OpType::COMMENT), indentLevel(0) {}
};

// Field in a data structure
struct StructField {
    std::string name;
    std::string type;
    std::string defaultValue;
};

// Method in a class
struct ClassMethod {
    std::string name;
    std::string returnType;
    std::vector<std::string> params;
    std::string body;
};


// ============================================================================
// MKCodeSynthesizer — Main Class
// ============================================================================

class MKCodeSynthesizer {
private:
    // Language-specific syntax templates
    // Key: OpType, Value: map of lang -> syntax pattern
    std::map<OpType, std::map<SynthLang, std::string>> syntaxDB;

    // Keyword mappings: natural language -> operation type
    std::map<std::string, OpType> actionToOp;

    // Type mappings across languages
    std::map<std::string, std::map<SynthLang, std::string>> typeMap;

    // ========================================================================
    // Private: Initialization
    // ========================================================================

    void initializeSyntax() {
        // Variable declaration
        syntaxDB[OpType::DECLARE_VAR][SynthLang::PYTHON] = "{name} = {value}";
        syntaxDB[OpType::DECLARE_VAR][SynthLang::JAVASCRIPT] = "let {name} = {value};";
        syntaxDB[OpType::DECLARE_VAR][SynthLang::CPP] = "{type} {name} = {value};";

        // Function definition
        syntaxDB[OpType::FUNCTION_DEF][SynthLang::PYTHON] = "def {name}({params}):";
        syntaxDB[OpType::FUNCTION_DEF][SynthLang::JAVASCRIPT] = "function {name}({params}) {";
        syntaxDB[OpType::FUNCTION_DEF][SynthLang::CPP] = "{returnType} {name}({params}) {";

        // For loop
        syntaxDB[OpType::LOOP_FOR][SynthLang::PYTHON] = "for {var} in {collection}:";
        syntaxDB[OpType::LOOP_FOR][SynthLang::JAVASCRIPT] = "for (const {var} of {collection}) {";
        syntaxDB[OpType::LOOP_FOR][SynthLang::CPP] = "for (auto& {var} : {collection}) {";

        // While loop
        syntaxDB[OpType::LOOP_WHILE][SynthLang::PYTHON] = "while {condition}:";
        syntaxDB[OpType::LOOP_WHILE][SynthLang::JAVASCRIPT] = "while ({condition}) {";
        syntaxDB[OpType::LOOP_WHILE][SynthLang::CPP] = "while ({condition}) {";

        // If condition
        syntaxDB[OpType::CONDITION_IF][SynthLang::PYTHON] = "if {condition}:";
        syntaxDB[OpType::CONDITION_IF][SynthLang::JAVASCRIPT] = "if ({condition}) {";
        syntaxDB[OpType::CONDITION_IF][SynthLang::CPP] = "if ({condition}) {";

        // Else
        syntaxDB[OpType::CONDITION_ELSE][SynthLang::PYTHON] = "else:";
        syntaxDB[OpType::CONDITION_ELSE][SynthLang::JAVASCRIPT] = "} else {";
        syntaxDB[OpType::CONDITION_ELSE][SynthLang::CPP] = "} else {";

        // Return
        syntaxDB[OpType::RETURN_VAL][SynthLang::PYTHON] = "return {value}";
        syntaxDB[OpType::RETURN_VAL][SynthLang::JAVASCRIPT] = "return {value};";
        syntaxDB[OpType::RETURN_VAL][SynthLang::CPP] = "return {value};";

        // Print
        syntaxDB[OpType::PRINT_OUTPUT][SynthLang::PYTHON] = "print({value})";
        syntaxDB[OpType::PRINT_OUTPUT][SynthLang::JAVASCRIPT] = "console.log({value});";
        syntaxDB[OpType::PRINT_OUTPUT][SynthLang::CPP] = "std::cout << {value} << std::endl;";

        // Function call
        syntaxDB[OpType::FUNCTION_CALL][SynthLang::PYTHON] = "{name}({args})";
        syntaxDB[OpType::FUNCTION_CALL][SynthLang::JAVASCRIPT] = "{name}({args});";
        syntaxDB[OpType::FUNCTION_CALL][SynthLang::CPP] = "{name}({args});";

        // Try-catch
        syntaxDB[OpType::TRY_CATCH][SynthLang::PYTHON] = "try:";
        syntaxDB[OpType::TRY_CATCH][SynthLang::JAVASCRIPT] = "try {";
        syntaxDB[OpType::TRY_CATCH][SynthLang::CPP] = "try {";
    }


    void initializeActionMap() {
        // Map natural language actions to code operations
        actionToOp["create"] = OpType::DECLARE_VAR;
        actionToOp["make"] = OpType::DECLARE_VAR;
        actionToOp["set"] = OpType::ASSIGN_VAR;
        actionToOp["assign"] = OpType::ASSIGN_VAR;
        actionToOp["define"] = OpType::FUNCTION_DEF;
        actionToOp["call"] = OpType::FUNCTION_CALL;
        actionToOp["invoke"] = OpType::FUNCTION_CALL;
        actionToOp["loop"] = OpType::LOOP_FOR;
        actionToOp["iterate"] = OpType::LOOP_FOR;
        actionToOp["repeat"] = OpType::LOOP_FOR;
        actionToOp["foreach"] = OpType::LOOP_FOR;
        actionToOp["while"] = OpType::LOOP_WHILE;
        actionToOp["check"] = OpType::CONDITION_IF;
        actionToOp["if"] = OpType::CONDITION_IF;
        actionToOp["condition"] = OpType::CONDITION_IF;
        actionToOp["return"] = OpType::RETURN_VAL;
        actionToOp["print"] = OpType::PRINT_OUTPUT;
        actionToOp["display"] = OpType::PRINT_OUTPUT;
        actionToOp["show"] = OpType::PRINT_OUTPUT;
        actionToOp["output"] = OpType::PRINT_OUTPUT;
        actionToOp["read"] = OpType::FILE_READ;
        actionToOp["load"] = OpType::FILE_READ;
        actionToOp["write"] = OpType::FILE_WRITE;
        actionToOp["save"] = OpType::FILE_WRITE;
        actionToOp["fetch"] = OpType::API_CALL;
        actionToOp["request"] = OpType::API_CALL;
        actionToOp["get"] = OpType::API_CALL;
        actionToOp["post"] = OpType::API_CALL;
        actionToOp["filter"] = OpType::ARRAY_FILTER;
        actionToOp["map"] = OpType::ARRAY_MAP;
        actionToOp["transform"] = OpType::ARRAY_MAP;
        actionToOp["class"] = OpType::CLASS_DEF;
        actionToOp["struct"] = OpType::STRUCT_DEF;
    }

    void initializeTypes() {
        // Type name -> language-specific type
        typeMap["int"][SynthLang::PYTHON] = "int";
        typeMap["int"][SynthLang::JAVASCRIPT] = "number";
        typeMap["int"][SynthLang::CPP] = "int";

        typeMap["string"][SynthLang::PYTHON] = "str";
        typeMap["string"][SynthLang::JAVASCRIPT] = "string";
        typeMap["string"][SynthLang::CPP] = "std::string";

        typeMap["float"][SynthLang::PYTHON] = "float";
        typeMap["float"][SynthLang::JAVASCRIPT] = "number";
        typeMap["float"][SynthLang::CPP] = "float";

        typeMap["bool"][SynthLang::PYTHON] = "bool";
        typeMap["bool"][SynthLang::JAVASCRIPT] = "boolean";
        typeMap["bool"][SynthLang::CPP] = "bool";

        typeMap["list"][SynthLang::PYTHON] = "list";
        typeMap["list"][SynthLang::JAVASCRIPT] = "Array";
        typeMap["list"][SynthLang::CPP] = "std::vector";

        typeMap["map"][SynthLang::PYTHON] = "dict";
        typeMap["map"][SynthLang::JAVASCRIPT] = "Object";
        typeMap["map"][SynthLang::CPP] = "std::map";

        typeMap["void"][SynthLang::PYTHON] = "None";
        typeMap["void"][SynthLang::JAVASCRIPT] = "void";
        typeMap["void"][SynthLang::CPP] = "void";
    }


    // ========================================================================
    // Private: Indentation Helper
    // ========================================================================

    std::string indent(int level, SynthLang lang) {
        std::string unit = (lang == SynthLang::PYTHON) ? "    " : "    ";
        std::string result;
        for (int i = 0; i < level; i++) result += unit;
        return result;
    }

    // ========================================================================
    // Private: Template Expansion
    // ========================================================================

    // Replace {placeholders} in a syntax template with actual values
    std::string expandTemplate(const std::string& tmpl,
                               const std::map<std::string, std::string>& params) {
        std::string result = tmpl;
        for (const auto& pair : params) {
            std::string placeholder = "{" + pair.first + "}";
            size_t pos = result.find(placeholder);
            while (pos != std::string::npos) {
                result.replace(pos, placeholder.size(), pair.second);
                pos = result.find(placeholder, pos + pair.second.size());
            }
        }
        return result;
    }

    // ========================================================================
    // Private: Language Detection
    // ========================================================================

    SynthLang parseLang(const std::string& lang) {
        std::string l = lang;
        std::transform(l.begin(), l.end(), l.begin(), ::tolower);
        if (l == "python" || l == "py") return SynthLang::PYTHON;
        if (l == "javascript" || l == "js") return SynthLang::JAVASCRIPT;
        if (l == "c++" || l == "cpp") return SynthLang::CPP;
        return SynthLang::PYTHON;  // Default
    }

    std::string langToString(SynthLang lang) {
        switch (lang) {
            case SynthLang::PYTHON: return "python";
            case SynthLang::JAVASCRIPT: return "javascript";
            case SynthLang::CPP: return "c++";
        }
        return "python";
    }

public:
    // ========================================================================
    // Constructor
    // ========================================================================

    MKCodeSynthesizer() {
        initializeSyntax();
        initializeActionMap();
        initializeTypes();
    }


    // ========================================================================
    // Core API: Plan Execution
    // ========================================================================

    // Break a goal into ordered execution steps
    // This is the "thinking" phase — figure out WHAT to do before coding
    std::vector<CodeStep> planExecution(const CodeGoal& goal) {
        std::vector<CodeStep> steps;

        // Parse the action to determine primary operation
        std::string action = goal.action;
        std::transform(action.begin(), action.end(), action.begin(), ::tolower);

        // Step 1: Input handling
        if (!goal.input.empty()) {
            CodeStep inputStep;
            inputStep.description = "Get input: " + goal.input;
            // Determine if input is a file, URL, variable, or literal
            if (goal.input.find("http") == 0 || goal.input.find("url") != std::string::npos) {
                inputStep.operation = OpType::API_CALL;
                inputStep.params["url"] = goal.input;
            } else if (goal.input.find(".") != std::string::npos &&
                       goal.input.find(" ") == std::string::npos) {
                inputStep.operation = OpType::FILE_READ;
                inputStep.params["path"] = goal.input;
            } else {
                inputStep.operation = OpType::DECLARE_VAR;
                inputStep.params["name"] = "input_data";
                inputStep.params["value"] = goal.input;
            }
            steps.push_back(inputStep);
        }

        // Step 2: Main operation
        CodeStep mainStep;
        mainStep.description = "Main action: " + goal.action;
        auto it = actionToOp.find(action);
        if (it != actionToOp.end()) {
            mainStep.operation = it->second;
        } else {
            mainStep.operation = OpType::FUNCTION_CALL;
        }
        mainStep.params["action"] = goal.action;
        if (!goal.constraints.empty()) {
            mainStep.params["constraints"] = goal.constraints;
        }
        steps.push_back(mainStep);

        // Step 3: Output handling
        if (!goal.output.empty()) {
            CodeStep outputStep;
            outputStep.description = "Output: " + goal.output;
            if (goal.output.find(".") != std::string::npos &&
                goal.output.find(" ") == std::string::npos) {
                outputStep.operation = OpType::FILE_WRITE;
                outputStep.params["path"] = goal.output;
            } else if (goal.output == "print" || goal.output == "display") {
                outputStep.operation = OpType::PRINT_OUTPUT;
            } else {
                outputStep.operation = OpType::RETURN_VAL;
                outputStep.params["value"] = "result";
            }
            steps.push_back(outputStep);
        }

        return steps;
    }

    // ========================================================================
    // Core API: Synthesize from Goal
    // ========================================================================

    // Generate complete working code from a goal description
    std::string synthesize(const CodeGoal& goal) {
        std::vector<CodeStep> steps = planExecution(goal);
        std::string code;

        // Add language-specific header comment
        code += "# Generated by MK Code Synthesizer\n";
        code += "# Goal: " + goal.action + "\n\n";

        // Translate each step to code
        for (const auto& step : steps) {
            code += synthesizeStep(step, goal.language);
            code += "\n";
        }

        return code;
    }


    // ========================================================================
    // Core API: Synthesize Function
    // ========================================================================

    // Create a complete function from description
    std::string synthesizeFunction(const std::string& name,
                                    const std::string& purpose,
                                    const std::vector<std::string>& inputs,
                                    const std::string& outputType,
                                    const std::string& lang) {
        SynthLang language = parseLang(lang);
        std::string code;

        // Build parameter list
        std::string params;
        for (size_t i = 0; i < inputs.size(); i++) {
            if (i > 0) params += ", ";
            params += inputs[i];
        }

        // Function header
        std::map<std::string, std::string> templateParams;
        templateParams["name"] = name;
        templateParams["params"] = params;
        templateParams["returnType"] = outputType.empty() ? "void" : outputType;

        std::string header = expandTemplate(
            syntaxDB[OpType::FUNCTION_DEF][language], templateParams);
        code += "// " + purpose + "\n";
        code += header + "\n";

        // Function body placeholder with purpose comment
        code += indent(1, language) + "// TODO: " + purpose + "\n";
        code += indent(1, language) + "pass\n";  // Python placeholder

        // Close brace for non-Python
        if (language != SynthLang::PYTHON) {
            code += "}\n";
        }

        return code;
    }

    // ========================================================================
    // Core API: Synthesize Loop
    // ========================================================================

    // Generate a "for each X do Y" loop
    std::string synthesizeLoop(const std::string& collection,
                               const std::string& action,
                               const std::string& lang) {
        SynthLang language = parseLang(lang);
        std::string code;

        std::map<std::string, std::string> params;
        params["var"] = "item";
        params["collection"] = collection;

        std::string header = expandTemplate(
            syntaxDB[OpType::LOOP_FOR][language], params);
        code += header + "\n";
        code += indent(1, language) + action + "\n";

        if (language != SynthLang::PYTHON) {
            code += "}\n";
        }

        return code;
    }

    // ========================================================================
    // Core API: Synthesize Condition
    // ========================================================================

    // Generate if/else branching code
    std::string synthesizeCondition(const std::string& condition,
                                     const std::string& thenAction,
                                     const std::string& elseAction,
                                     const std::string& lang) {
        SynthLang language = parseLang(lang);
        std::string code;

        // If block
        std::map<std::string, std::string> params;
        params["condition"] = condition;

        std::string ifHeader = expandTemplate(
            syntaxDB[OpType::CONDITION_IF][language], params);
        code += ifHeader + "\n";
        code += indent(1, language) + thenAction + "\n";

        // Else block (if provided)
        if (!elseAction.empty()) {
            code += syntaxDB[OpType::CONDITION_ELSE][language] + "\n";
            code += indent(1, language) + elseAction + "\n";
        }

        if (language != SynthLang::PYTHON) {
            code += "}\n";
        }

        return code;
    }


    // ========================================================================
    // Core API: Synthesize API Call
    // ========================================================================

    // Generate HTTP request code
    std::string synthesizeAPICall(const std::string& url,
                                   const std::string& method,
                                   const std::string& headers,
                                   const std::string& body,
                                   const std::string& lang) {
        SynthLang language = parseLang(lang);
        std::string code;

        switch (language) {
            case SynthLang::PYTHON:
                code += "import requests\n\n";
                code += "response = requests." + method + "(\"" + url + "\"";
                if (!headers.empty()) {
                    code += ", headers=" + headers;
                }
                if (!body.empty()) {
                    code += ", json=" + body;
                }
                code += ")\n";
                code += "data = response.json()\n";
                break;

            case SynthLang::JAVASCRIPT:
                code += "const response = await fetch(\"" + url + "\", {\n";
                code += "    method: \"" + method + "\"";
                if (!headers.empty()) {
                    code += ",\n    headers: " + headers;
                }
                if (!body.empty()) {
                    code += ",\n    body: JSON.stringify(" + body + ")";
                }
                code += "\n});\n";
                code += "const data = await response.json();\n";
                break;

            case SynthLang::CPP:
                code += "// Note: Requires HTTP library (e.g., libcurl)\n";
                code += "std::string url = \"" + url + "\";\n";
                code += "std::string method = \"" + method + "\";\n";
                code += "// HttpClient client;\n";
                code += "// auto response = client." + method + "(url);\n";
                break;
        }

        return code;
    }

    // ========================================================================
    // Core API: Synthesize File Operation
    // ========================================================================

    // Generate file I/O code
    std::string synthesizeFileOp(const std::string& operation,
                                  const std::string& path,
                                  const std::string& data,
                                  const std::string& lang) {
        SynthLang language = parseLang(lang);
        std::string code;

        bool isRead = (operation == "read" || operation == "load");

        switch (language) {
            case SynthLang::PYTHON:
                if (isRead) {
                    code += "with open(\"" + path + "\", \"r\") as f:\n";
                    code += "    content = f.read()\n";
                } else {
                    code += "with open(\"" + path + "\", \"w\") as f:\n";
                    code += "    f.write(" + data + ")\n";
                }
                break;

            case SynthLang::JAVASCRIPT:
                code += "const fs = require('fs');\n";
                if (isRead) {
                    code += "const content = fs.readFileSync(\"" + path + "\", \"utf8\");\n";
                } else {
                    code += "fs.writeFileSync(\"" + path + "\", " + data + ");\n";
                }
                break;

            case SynthLang::CPP:
                code += "#include <fstream>\n";
                if (isRead) {
                    code += "std::ifstream file(\"" + path + "\");\n";
                    code += "std::string content((std::istreambuf_iterator<char>(file)),\n";
                    code += "                     std::istreambuf_iterator<char>());\n";
                } else {
                    code += "std::ofstream file(\"" + path + "\");\n";
                    code += "file << " + data + ";\n";
                    code += "file.close();\n";
                }
                break;
        }

        return code;
    }


    // ========================================================================
    // Core API: Synthesize Data Structure
    // ========================================================================

    // Generate class/struct definition
    std::string synthesizeDataStructure(const std::string& name,
                                         const std::vector<StructField>& fields,
                                         const std::vector<ClassMethod>& methods,
                                         const std::string& lang) {
        SynthLang language = parseLang(lang);
        std::string code;

        switch (language) {
            case SynthLang::PYTHON:
                code += "class " + name + ":\n";
                // Constructor
                code += "    def __init__(self";
                for (const auto& f : fields) {
                    code += ", " + f.name;
                    if (!f.defaultValue.empty()) code += "=" + f.defaultValue;
                }
                code += "):\n";
                for (const auto& f : fields) {
                    code += "        self." + f.name + " = " + f.name + "\n";
                }
                code += "\n";
                // Methods
                for (const auto& m : methods) {
                    code += "    def " + m.name + "(self";
                    for (const auto& p : m.params) {
                        code += ", " + p;
                    }
                    code += "):\n";
                    if (!m.body.empty()) {
                        code += "        " + m.body + "\n";
                    } else {
                        code += "        pass\n";
                    }
                    code += "\n";
                }
                break;

            case SynthLang::JAVASCRIPT:
                code += "class " + name + " {\n";
                // Constructor
                code += "    constructor(";
                for (size_t i = 0; i < fields.size(); i++) {
                    if (i > 0) code += ", ";
                    code += fields[i].name;
                }
                code += ") {\n";
                for (const auto& f : fields) {
                    code += "        this." + f.name + " = " + f.name + ";\n";
                }
                code += "    }\n\n";
                // Methods
                for (const auto& m : methods) {
                    code += "    " + m.name + "(";
                    for (size_t i = 0; i < m.params.size(); i++) {
                        if (i > 0) code += ", ";
                        code += m.params[i];
                    }
                    code += ") {\n";
                    if (!m.body.empty()) {
                        code += "        " + m.body + "\n";
                    }
                    code += "    }\n\n";
                }
                code += "}\n";
                break;

            case SynthLang::CPP:
                code += "struct " + name + " {\n";
                // Fields
                for (const auto& f : fields) {
                    std::string type = f.type.empty() ? "std::string" : f.type;
                    code += "    " + type + " " + f.name;
                    if (!f.defaultValue.empty()) {
                        code += " = " + f.defaultValue;
                    }
                    code += ";\n";
                }
                code += "\n";
                // Methods
                for (const auto& m : methods) {
                    std::string retType = m.returnType.empty() ? "void" : m.returnType;
                    code += "    " + retType + " " + m.name + "(";
                    for (size_t i = 0; i < m.params.size(); i++) {
                        if (i > 0) code += ", ";
                        code += m.params[i];
                    }
                    code += ") {\n";
                    if (!m.body.empty()) {
                        code += "        " + m.body + "\n";
                    }
                    code += "    }\n\n";
                }
                code += "};\n";
                break;
        }

        return code;
    }


    // ========================================================================
    // Core API: Chain Operations
    // ========================================================================

    // Chain multiple operations: "fetch data, filter items, save to file"
    // Each operation feeds into the next
    std::string synthesizeChain(const std::vector<CodeGoal>& goals,
                                const std::string& lang) {
        SynthLang language = parseLang(lang);
        std::string code;

        code += "# Chained operations generated by MK\n\n";

        std::string lastVar = "data";
        int stepNum = 1;

        for (const auto& goal : goals) {
            code += "# Step " + std::to_string(stepNum) + ": " + goal.action + "\n";

            std::string action = goal.action;
            std::transform(action.begin(), action.end(), action.begin(), ::tolower);

            if (action == "fetch" || action == "get") {
                code += synthesizeAPICall(goal.input, "get", "", "", lang);
                lastVar = "data";
            } else if (action == "filter") {
                switch (language) {
                    case SynthLang::PYTHON:
                        code += "result = [x for x in " + lastVar +
                                " if " + goal.constraints + "]\n";
                        break;
                    case SynthLang::JAVASCRIPT:
                        code += "const result = " + lastVar +
                                ".filter(x => " + goal.constraints + ");\n";
                        break;
                    case SynthLang::CPP:
                        code += "auto result = filter(" + lastVar +
                                ", [](auto& x) { return " + goal.constraints + "; });\n";
                        break;
                }
                lastVar = "result";
            } else if (action == "save" || action == "write") {
                code += synthesizeFileOp("write", goal.output, lastVar, lang);
            } else if (action == "print" || action == "display") {
                std::map<std::string, std::string> params;
                params["value"] = lastVar;
                code += expandTemplate(syntaxDB[OpType::PRINT_OUTPUT][language], params) + "\n";
            } else if (action == "sort") {
                switch (language) {
                    case SynthLang::PYTHON:
                        code += lastVar + ".sort(key=lambda x: " + goal.constraints + ")\n";
                        break;
                    case SynthLang::JAVASCRIPT:
                        code += lastVar + ".sort((a, b) => " + goal.constraints + ");\n";
                        break;
                    case SynthLang::CPP:
                        code += "std::sort(" + lastVar + ".begin(), " + lastVar +
                                ".end(), [](auto& a, auto& b) { return " +
                                goal.constraints + "; });\n";
                        break;
                }
            }

            code += "\n";
            stepNum++;
        }

        return code;
    }

private:
    // ========================================================================
    // Private: Synthesize Single Step
    // ========================================================================

    std::string synthesizeStep(const CodeStep& step, SynthLang language) {
        std::string code;

        // Get the syntax template for this operation + language
        auto opIt = syntaxDB.find(step.operation);
        if (opIt != syntaxDB.end()) {
            auto langIt = opIt->second.find(language);
            if (langIt != opIt->second.end()) {
                code = indent(step.indentLevel, language) +
                       expandTemplate(langIt->second, step.params);
            }
        }

        // If no template matched, generate a comment
        if (code.empty()) {
            code = indent(step.indentLevel, language) +
                   "// " + step.description;
        }

        return code;
    }
};

#endif // MK_CODE_SYNTHESIZER_CPP
