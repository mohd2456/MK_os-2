#ifndef MK_CODE_INTELLIGENCE_CPP
#define MK_CODE_INTELLIGENCE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <ctime>

// ===================================================================================
// MK CODE INTELLIGENCE ENGINE
// ===================================================================================
// THIS is how MK builds applications like Claude/Kiro without being an LLM.
//
// The secret: MK doesn't "generate" code from neural weights.
// Instead it has a MASSIVE indexed library of code patterns, structures,
// and templates loaded from the 450GB knowledge partition.
//
// How it works:
//   1. USER: "Build me a web scraper in Python"
//   2. MK searches its code knowledge base for:
//      - "web scraper" → finds patterns, libraries, structures
//      - "python" → knows syntax, idioms, imports
//   3. MK's Architecture Planner breaks it into components
//   4. MK's Code Composer assembles each component from templates
//   5. MK outputs working code assembled from proven patterns
//
// The more code knowledge you feed it (from that 450GB), the better it gets.
// It's like a programmer with a perfect memory of every codebase it's read.
// ===================================================================================

// A stored code pattern/template
struct MKCodeTemplate {
    std::string id;
    std::string name;
    std::string language;       // "python", "cpp", "javascript", "html", etc.
    std::string category;       // "function", "class", "loop", "api", "ui", etc.
    std::string description;
    std::string code;           // The actual code with {placeholders}
    std::vector<std::string> placeholders;  // What needs to be filled in
    std::vector<std::string> tags;          // Searchable tags
    float useCount;             // How often this template was used (popularity)
};

// A language syntax profile
struct MKLanguageProfile {
    std::string name;
    std::string fileExtension;
    std::string commentSingle;
    std::string commentMultiStart;
    std::string commentMultiEnd;
    std::string functionKeyword;    // "def", "void", "function"
    std::string classKeyword;       // "class"
    std::string importKeyword;      // "import", "#include", "require"
    std::string entryPoint;         // "if __name__", "int main()", etc.
    std::vector<std::string> commonImports;
    std::map<std::string, std::string> idioms;  // Common patterns for this language
};

// A project component (file in a planned project)
struct MKProjectComponent {
    std::string filename;
    std::string purpose;
    std::string language;
    std::vector<std::string> dependencies;  // Other components it needs
    std::string generatedCode;
    int priority;   // Build order
};

// A full project plan
struct MKProjectPlan {
    std::string name;
    std::string description;
    std::string mainLanguage;
    std::vector<MKProjectComponent> components;
    std::vector<std::string> externalDeps;  // Libraries needed (pip, npm, etc.)
    std::string buildCommand;
    std::string runCommand;
};

// Known bug patterns and their fixes
struct MKBugPattern {
    std::string language;
    std::string errorPattern;       // Regex-like pattern to match in error messages
    std::string cause;
    std::string fix;
    std::string example;
};

class MKCodeIntelligence {
private:
    // Template library (the core knowledge — loaded from 450GB partition)
    std::vector<MKCodeTemplate> templates;
    std::unordered_map<std::string, std::vector<int>> tagIndex;      // tag → template IDs
    std::unordered_map<std::string, std::vector<int>> langIndex;     // language → template IDs
    std::unordered_map<std::string, std::vector<int>> categoryIndex; // category → template IDs
    
    // Language profiles
    std::map<std::string, MKLanguageProfile> languages;
    
    // Bug pattern database
    std::vector<MKBugPattern> bugPatterns;
    
    // Stats
    int totalTemplates;
    int totalSearches;
    int projectsPlanned;
    std::string knowledgePath;

    // Initialize language profiles with syntax knowledge
    void initLanguages() {
        // Python
        MKLanguageProfile py;
        py.name = "python";
        py.fileExtension = ".py";
        py.commentSingle = "#";
        py.commentMultiStart = "\"\"\"";
        py.commentMultiEnd = "\"\"\"";
        py.functionKeyword = "def";
        py.classKeyword = "class";
        py.importKeyword = "import";
        py.entryPoint = "if __name__ == \"__main__\":";
        py.commonImports = {"os", "sys", "json", "requests", "time", "re", 
                           "pathlib", "typing", "datetime", "collections"};
        py.idioms["list_comp"] = "[{expr} for {var} in {iterable}]";
        py.idioms["dict_comp"] = "{{k: v for k, v in {iterable}}}";
        py.idioms["with_file"] = "with open({path}, '{mode}') as f:";
        py.idioms["try_except"] = "try:\n    {code}\nexcept {exception} as e:\n    {handler}";
        py.idioms["main_guard"] = "if __name__ == \"__main__\":\n    main()";
        languages["python"] = py;

        // C++
        MKLanguageProfile cpp;
        cpp.name = "cpp";
        cpp.fileExtension = ".cpp";
        cpp.commentSingle = "//";
        cpp.commentMultiStart = "/*";
        cpp.commentMultiEnd = "*/";
        cpp.functionKeyword = "";
        cpp.classKeyword = "class";
        cpp.importKeyword = "#include";
        cpp.entryPoint = "int main() {";
        cpp.commonImports = {"<iostream>", "<string>", "<vector>", "<map>", 
                            "<fstream>", "<sstream>", "<algorithm>", "<memory>"};
        cpp.idioms["range_for"] = "for (const auto& {var} : {container}) {";
        cpp.idioms["smart_ptr"] = "std::unique_ptr<{type}> {name} = std::make_unique<{type}>({args});";
        cpp.idioms["raii_file"] = "std::ifstream {name}({path});\nif (!{name}.is_open()) return;";
        languages["cpp"] = cpp;

        // JavaScript
        MKLanguageProfile js;
        js.name = "javascript";
        js.fileExtension = ".js";
        js.commentSingle = "//";
        js.commentMultiStart = "/*";
        js.commentMultiEnd = "*/";
        js.functionKeyword = "function";
        js.classKeyword = "class";
        js.importKeyword = "import";
        js.entryPoint = "";
        js.commonImports = {"fs", "path", "http", "express", "axios", "dotenv"};
        js.idioms["arrow"] = "const {name} = ({params}) => {";
        js.idioms["async"] = "async function {name}({params}) {";
        js.idioms["fetch"] = "const response = await fetch({url});\nconst data = await response.json();";
        js.idioms["promise"] = "new Promise((resolve, reject) => {\n    {code}\n});";
        languages["javascript"] = js;

        // HTML
        MKLanguageProfile html;
        html.name = "html";
        html.fileExtension = ".html";
        html.commentSingle = "";
        html.commentMultiStart = "<!--";
        html.commentMultiEnd = "-->";
        html.functionKeyword = "";
        html.classKeyword = "";
        html.importKeyword = "";
        html.entryPoint = "<!DOCTYPE html>";
        languages["html"] = html;
    }

    // Initialize common bug patterns
    void initBugPatterns() {
        bugPatterns.push_back({"python", "IndentationError", 
            "Mixed tabs and spaces or wrong indent level",
            "Use consistent 4-space indentation throughout", ""});
        bugPatterns.push_back({"python", "NameError: name .* is not defined",
            "Variable used before assignment or typo in name",
            "Check spelling and ensure variable is defined before use", ""});
        bugPatterns.push_back({"python", "ImportError",
            "Module not installed or wrong import path",
            "Run: pip install {module_name}", ""});
        bugPatterns.push_back({"python", "TypeError.*argument",
            "Wrong number or type of arguments passed to function",
            "Check function signature and ensure correct argument types", ""});
        bugPatterns.push_back({"cpp", "undefined reference",
            "Function declared but not implemented, or missing library link",
            "Add implementation or link with -l flag", ""});
        bugPatterns.push_back({"cpp", "expected ';'",
            "Missing semicolon at end of statement",
            "Add semicolon at the indicated line", ""});
        bugPatterns.push_back({"cpp", "no matching function",
            "Function called with wrong argument types",
            "Check parameter types match the function signature", ""});
        bugPatterns.push_back({"javascript", "is not defined",
            "Variable or function used before declaration",
            "Declare with let/const before use, or check import", ""});
        bugPatterns.push_back({"javascript", "Cannot read propert",
            "Trying to access property of null/undefined",
            "Add null check: if (obj && obj.property)", ""});
        bugPatterns.push_back({"javascript", "Unexpected token",
            "Syntax error - missing bracket, comma, or wrong syntax",
            "Check for missing brackets, commas, or semicolons", ""});
    }

    // Search templates by tags (returns sorted by relevance)
    std::vector<int> searchTemplates(const std::vector<std::string>& searchTags, 
                                      const std::string& language = "") {
        std::map<int, int> scores;  // templateId → match count
        
        for (const auto& tag : searchTags) {
            std::string lower = tag;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            
            auto it = tagIndex.find(lower);
            if (it != tagIndex.end()) {
                for (int id : it->second) {
                    scores[id]++;
                }
            }
        }
        
        // Filter by language if specified
        if (!language.empty()) {
            auto langIt = langIndex.find(language);
            if (langIt != langIndex.end()) {
                std::set<int> langSet(langIt->second.begin(), langIt->second.end());
                for (auto it = scores.begin(); it != scores.end();) {
                    if (langSet.find(it->first) == langSet.end()) {
                        it = scores.erase(it);
                    } else {
                        it->second += 2;  // Bonus for matching language
                        ++it;
                    }
                }
            }
        }
        
        // Sort by score (highest first)
        std::vector<std::pair<int, int>> sorted(scores.begin(), scores.end());
        std::sort(sorted.begin(), sorted.end(), 
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        std::vector<int> results;
        for (const auto& p : sorted) results.push_back(p.first);
        totalSearches++;
        return results;
    }

public:
    MKCodeIntelligence(const std::string& knowledgeDir = "knowledge_files")
        : totalTemplates(0), totalSearches(0), projectsPlanned(0), knowledgePath(knowledgeDir) {
        initLanguages();
        initBugPatterns();
        std::cout << "[CODE INTEL] Code intelligence engine initialized.\n";
        std::cout << "[CODE INTEL] Languages: " << languages.size() 
                  << " | Bug patterns: " << bugPatterns.size() << "\n";
    }


    // ─────────────────────────────────────────
    //  TEMPLATE MANAGEMENT
    // ─────────────────────────────────────────

    // Add a code template to the library
    void addTemplate(const std::string& name, const std::string& language,
                     const std::string& category, const std::string& description,
                     const std::string& code, const std::vector<std::string>& tags) {
        MKCodeTemplate tmpl;
        tmpl.id = language + "_" + std::to_string(totalTemplates);
        tmpl.name = name;
        tmpl.language = language;
        tmpl.category = category;
        tmpl.description = description;
        tmpl.code = code;
        tmpl.tags = tags;
        tmpl.useCount = 0;
        
        // Extract placeholders from code (anything in {curly_braces})
        size_t pos = 0;
        while ((pos = code.find('{', pos)) != std::string::npos) {
            size_t end = code.find('}', pos);
            if (end != std::string::npos) {
                tmpl.placeholders.push_back(code.substr(pos + 1, end - pos - 1));
                pos = end + 1;
            } else break;
        }
        
        int idx = templates.size();
        templates.push_back(tmpl);
        
        // Index by tags
        for (const auto& tag : tags) {
            std::string lower = tag;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            tagIndex[lower].push_back(idx);
        }
        langIndex[language].push_back(idx);
        categoryIndex[category].push_back(idx);
        totalTemplates++;
    }

    // Load templates from the knowledge partition file
    void loadTemplates(const std::string& filename) {
        std::string path = knowledgePath + "/" + filename;
        std::ifstream in(path);
        if (!in.is_open()) {
            std::cerr << "[CODE INTEL] Cannot load: " << path << "\n";
            return;
        }
        
        std::string line;
        int loaded = 0;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            // Format: name|language|category|description|code|tag1,tag2,tag3
            std::stringstream ss(line);
            std::string name, lang, cat, desc, code, tagStr;
            if (std::getline(ss, name, '|') && std::getline(ss, lang, '|') &&
                std::getline(ss, cat, '|') && std::getline(ss, desc, '|') &&
                std::getline(ss, code, '|') && std::getline(ss, tagStr, '|')) {
                
                // Replace escaped newlines
                size_t pos;
                while ((pos = code.find("\\n")) != std::string::npos) {
                    code.replace(pos, 2, "\n");
                }
                
                // Parse tags
                std::vector<std::string> tags;
                std::stringstream ts(tagStr);
                std::string tag;
                while (std::getline(ts, tag, ',')) {
                    if (!tag.empty()) tags.push_back(tag);
                }
                
                addTemplate(name, lang, cat, desc, code, tags);
                loaded++;
            }
        }
        in.close();
        std::cout << "[CODE INTEL] Loaded " << loaded << " code templates from: " << path << "\n";
    }


    // ─────────────────────────────────────────
    //  ARCHITECTURE PLANNER
    // ─────────────────────────────────────────

    // Plan a full project from a description
    MKProjectPlan planProject(const std::string& description, const std::string& language) {
        MKProjectPlan plan;
        plan.name = "mk_project_" + std::to_string(projectsPlanned++);
        plan.description = description;
        plan.mainLanguage = language;
        
        std::string lower = description;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        // Detect project type from description keywords and plan components
        if (lower.find("web") != std::string::npos || lower.find("site") != std::string::npos ||
            lower.find("frontend") != std::string::npos) {
            plan.components.push_back({"index.html", "Main HTML page", "html", {}, "", 1});
            plan.components.push_back({"style.css", "Styling", "css", {"index.html"}, "", 2});
            plan.components.push_back({"app.js", "Main application logic", "javascript", {"index.html"}, "", 3});
            if (lower.find("api") != std::string::npos || lower.find("server") != std::string::npos) {
                plan.components.push_back({"server.js", "Backend API server", "javascript", {}, "", 1});
                plan.externalDeps.push_back("express");
            }
        }
        else if (lower.find("api") != std::string::npos || lower.find("server") != std::string::npos ||
                 lower.find("backend") != std::string::npos) {
            if (language == "python") {
                plan.components.push_back({"app.py", "Main application entry point", "python", {}, "", 1});
                plan.components.push_back({"routes.py", "API route handlers", "python", {"app.py"}, "", 2});
                plan.components.push_back({"models.py", "Data models", "python", {}, "", 1});
                plan.components.push_back({"config.py", "Configuration", "python", {}, "", 1});
                plan.components.push_back({"requirements.txt", "Dependencies", "text", {}, "", 1});
                plan.externalDeps = {"flask", "requests"};
                plan.buildCommand = "pip install -r requirements.txt";
                plan.runCommand = "python app.py";
            } else {
                plan.components.push_back({"server.js", "Main server", "javascript", {}, "", 1});
                plan.components.push_back({"routes/index.js", "Route handlers", "javascript", {"server.js"}, "", 2});
                plan.components.push_back({"package.json", "Dependencies", "json", {}, "", 1});
                plan.externalDeps = {"express", "cors", "dotenv"};
                plan.runCommand = "node server.js";
            }
        }
        else if (lower.find("scraper") != std::string::npos || lower.find("crawl") != std::string::npos) {
            plan.components.push_back({"scraper.py", "Main scraper logic", "python", {}, "", 1});
            plan.components.push_back({"parser.py", "HTML/data parsing", "python", {"scraper.py"}, "", 2});
            plan.components.push_back({"storage.py", "Save results to file/DB", "python", {"scraper.py"}, "", 2});
            plan.components.push_back({"config.py", "URLs and settings", "python", {}, "", 1});
            plan.externalDeps = {"requests", "beautifulsoup4"};
            plan.runCommand = "python scraper.py";
        }
        else if (lower.find("bot") != std::string::npos || lower.find("telegram") != std::string::npos) {
            plan.components.push_back({"bot.py", "Main bot loop", "python", {}, "", 1});
            plan.components.push_back({"handlers.py", "Command handlers", "python", {"bot.py"}, "", 2});
            plan.components.push_back({"config.py", "Bot token and settings", "python", {}, "", 1});
            plan.externalDeps = {"python-telegram-bot"};
            plan.runCommand = "python bot.py";
        }
        else if (lower.find("game") != std::string::npos) {
            plan.components.push_back({"main.py", "Game loop", "python", {}, "", 1});
            plan.components.push_back({"player.py", "Player class", "python", {"main.py"}, "", 2});
            plan.components.push_back({"world.py", "Game world/map", "python", {"main.py"}, "", 2});
            plan.components.push_back({"renderer.py", "Display/rendering", "python", {"main.py"}, "", 2});
            plan.externalDeps = {"pygame"};
            plan.runCommand = "python main.py";
        }
        else {
            // Generic application
            std::string ext = (language == "python") ? ".py" : 
                              (language == "javascript") ? ".js" : ".cpp";
            plan.components.push_back({"main" + ext, "Main entry point", language, {}, "", 1});
            plan.components.push_back({"utils" + ext, "Utility functions", language, {"main" + ext}, "", 2});
            plan.components.push_back({"config" + ext, "Configuration", language, {}, "", 1});
        }
        
        std::cout << "[CODE INTEL] Project planned: " << plan.components.size() 
                  << " components | Language: " << language << "\n";
        return plan;
    }


    // ─────────────────────────────────────────
    //  CODE COMPOSER — Assembles working code
    // ─────────────────────────────────────────

    // Fill a template's placeholders with actual values
    std::string fillTemplate(const MKCodeTemplate& tmpl, 
                             const std::map<std::string, std::string>& values) {
        std::string result = tmpl.code;
        for (const auto& kv : values) {
            std::string placeholder = "{" + kv.first + "}";
            size_t pos;
            while ((pos = result.find(placeholder)) != std::string::npos) {
                result.replace(pos, placeholder.size(), kv.second);
            }
        }
        return result;
    }

    // Generate a complete function in any supported language
    std::string generateFunction(const std::string& language, const std::string& name,
                                  const std::vector<std::string>& params,
                                  const std::string& body, const std::string& returnType = "") {
        std::stringstream code;
        auto langIt = languages.find(language);
        if (langIt == languages.end()) return "// Unsupported language: " + language;
        
        const auto& lang = langIt->second;
        
        if (language == "python") {
            code << "def " << name << "(";
            for (size_t i = 0; i < params.size(); i++) {
                code << params[i];
                if (i < params.size() - 1) code << ", ";
            }
            code << "):\n";
            // Indent body
            std::stringstream bs(body);
            std::string line;
            while (std::getline(bs, line)) {
                code << "    " << line << "\n";
            }
        }
        else if (language == "cpp") {
            code << (returnType.empty() ? "void" : returnType) << " " << name << "(";
            for (size_t i = 0; i < params.size(); i++) {
                code << params[i];
                if (i < params.size() - 1) code << ", ";
            }
            code << ") {\n";
            std::stringstream bs(body);
            std::string line;
            while (std::getline(bs, line)) {
                code << "    " << line << "\n";
            }
            code << "}\n";
        }
        else if (language == "javascript") {
            code << "function " << name << "(";
            for (size_t i = 0; i < params.size(); i++) {
                code << params[i];
                if (i < params.size() - 1) code << ", ";
            }
            code << ") {\n";
            std::stringstream bs(body);
            std::string line;
            while (std::getline(bs, line)) {
                code << "    " << line << "\n";
            }
            code << "}\n";
        }
        return code.str();
    }

    // Generate a complete class
    std::string generateClass(const std::string& language, const std::string& name,
                               const std::vector<std::string>& methods,
                               const std::vector<std::string>& properties) {
        std::stringstream code;
        
        if (language == "python") {
            code << "class " << name << ":\n";
            code << "    def __init__(self";
            for (const auto& prop : properties) code << ", " << prop;
            code << "):\n";
            for (const auto& prop : properties) {
                code << "        self." << prop << " = " << prop << "\n";
            }
            code << "\n";
            for (const auto& method : methods) {
                code << "    def " << method << "(self):\n";
                code << "        pass  # TODO: implement\n\n";
            }
        }
        else if (language == "cpp") {
            code << "class " << name << " {\nprivate:\n";
            for (const auto& prop : properties) {
                code << "    std::string " << prop << ";\n";
            }
            code << "\npublic:\n";
            code << "    " << name << "(";
            for (size_t i = 0; i < properties.size(); i++) {
                code << "const std::string& " << properties[i];
                if (i < properties.size() - 1) code << ", ";
            }
            code << ") : ";
            for (size_t i = 0; i < properties.size(); i++) {
                code << properties[i] << "(" << properties[i] << ")";
                if (i < properties.size() - 1) code << ", ";
            }
            code << " {}\n\n";
            for (const auto& method : methods) {
                code << "    void " << method << "() {\n        // TODO\n    }\n\n";
            }
            code << "};\n";
        }
        else if (language == "javascript") {
            code << "class " << name << " {\n";
            code << "    constructor(";
            for (size_t i = 0; i < properties.size(); i++) {
                code << properties[i];
                if (i < properties.size() - 1) code << ", ";
            }
            code << ") {\n";
            for (const auto& prop : properties) {
                code << "        this." << prop << " = " << prop << ";\n";
            }
            code << "    }\n\n";
            for (const auto& method : methods) {
                code << "    " << method << "() {\n        // TODO\n    }\n\n";
            }
            code << "}\n";
        }
        return code.str();
    }


    // ─────────────────────────────────────────
    //  FULL PROJECT GENERATION
    // ─────────────────────────────────────────

    // Generate all files for a planned project
    std::map<std::string, std::string> generateProject(const MKProjectPlan& plan) {
        std::map<std::string, std::string> files;
        
        for (const auto& comp : plan.components) {
            std::stringstream code;
            
            auto langIt = languages.find(comp.language);
            if (langIt != languages.end()) {
                const auto& lang = langIt->second;
                
                // Add header comment
                if (!lang.commentSingle.empty()) {
                    code << lang.commentSingle << " " << comp.filename << " — " << comp.purpose << "\n";
                    code << lang.commentSingle << " Generated by MK Code Intelligence\n";
                    code << lang.commentSingle << " Project: " << plan.name << "\n\n";
                }
                
                // Add common imports based on detected needs
                for (const auto& imp : lang.commonImports) {
                    if (comp.purpose.find("http") != std::string::npos ||
                        comp.purpose.find("api") != std::string::npos ||
                        comp.purpose.find("server") != std::string::npos) {
                        code << lang.importKeyword << " " << imp << "\n";
                    }
                }
                code << "\n";
                
                // Search for relevant templates
                std::vector<std::string> searchTags;
                std::stringstream words(comp.purpose);
                std::string word;
                while (words >> word) {
                    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                    searchTags.push_back(word);
                }
                
                auto found = searchTemplates(searchTags, comp.language);
                
                // Use matched templates
                if (!found.empty()) {
                    for (int i = 0; i < std::min(3, (int)found.size()); i++) {
                        code << templates[found[i]].code << "\n\n";
                    }
                }
                
                // Add entry point if this is main file
                if (comp.filename.find("main") != std::string::npos ||
                    comp.filename.find("app") != std::string::npos) {
                    if (!lang.entryPoint.empty()) {
                        code << "\n" << lang.entryPoint << "\n";
                        if (comp.language == "python") {
                            code << "    main()\n";
                        } else if (comp.language == "cpp") {
                            code << "    return 0;\n}\n";
                        }
                    }
                }
            }
            
            files[comp.filename] = code.str();
        }
        
        std::cout << "[CODE INTEL] Generated " << files.size() << " files for project.\n";
        return files;
    }

    // ─────────────────────────────────────────
    //  BUG DIAGNOSIS
    // ─────────────────────────────────────────

    // Given an error message, find likely cause and fix
    std::string diagnoseBug(const std::string& errorMessage, const std::string& language = "") {
        std::stringstream diagnosis;
        std::string lower = errorMessage;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        for (const auto& bug : bugPatterns) {
            if (!language.empty() && bug.language != language) continue;
            
            std::string lowerPattern = bug.errorPattern;
            std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);
            
            if (lower.find(lowerPattern) != std::string::npos) {
                diagnosis << "LIKELY CAUSE: " << bug.cause << "\n";
                diagnosis << "SUGGESTED FIX: " << bug.fix << "\n";
                return diagnosis.str();
            }
        }
        
        return "Unknown error pattern. Try checking syntax and variable names.";
    }

    // ─────────────────────────────────────────
    //  FILE READER — Understands existing code
    // ─────────────────────────────────────────

    // Read and analyze a code file's structure
    std::string analyzeFile(const std::string& filename) {
        std::ifstream in(filename);
        if (!in.is_open()) return "Cannot open file: " + filename;
        
        std::stringstream analysis;
        std::string line;
        int lineCount = 0;
        int funcCount = 0;
        int classCount = 0;
        int importCount = 0;
        std::vector<std::string> functions;
        std::vector<std::string> classes;
        
        // Detect language from extension
        std::string ext = filename.substr(filename.rfind('.'));
        std::string detectedLang = "unknown";
        for (const auto& kv : languages) {
            if (kv.second.fileExtension == ext) {
                detectedLang = kv.first;
                break;
            }
        }
        
        while (std::getline(in, line)) {
            lineCount++;
            std::string trimmed = line;
            size_t start = trimmed.find_first_not_of(" \t");
            if (start != std::string::npos) trimmed = trimmed.substr(start);
            
            if (detectedLang == "python") {
                if (trimmed.find("def ") == 0) { funcCount++; functions.push_back(trimmed); }
                if (trimmed.find("class ") == 0) { classCount++; classes.push_back(trimmed); }
                if (trimmed.find("import ") == 0 || trimmed.find("from ") == 0) importCount++;
            }
            else if (detectedLang == "cpp" || detectedLang == "javascript") {
                if (trimmed.find("function ") != std::string::npos) { funcCount++; functions.push_back(trimmed); }
                if (trimmed.find("class ") == 0) { classCount++; classes.push_back(trimmed); }
                if (trimmed.find("#include") == 0 || trimmed.find("import ") == 0) importCount++;
            }
        }
        in.close();
        
        analysis << "FILE: " << filename << " (" << detectedLang << ")\n";
        analysis << "Lines: " << lineCount << " | Functions: " << funcCount 
                 << " | Classes: " << classCount << " | Imports: " << importCount << "\n";
        if (!classes.empty()) {
            analysis << "Classes: ";
            for (const auto& c : classes) analysis << c << "; ";
            analysis << "\n";
        }
        return analysis.str();
    }

    // ─────────────────────────────────────────
    //  STATS
    // ─────────────────────────────────────────

    void printStats() const {
        std::cout << "\n--- [CODE INTELLIGENCE] ---\n";
        std::cout << "  Templates: " << totalTemplates << "\n";
        std::cout << "  Languages: " << languages.size() << "\n";
        std::cout << "  Bug Patterns: " << bugPatterns.size() << "\n";
        std::cout << "  Searches: " << totalSearches << "\n";
        std::cout << "  Projects Planned: " << projectsPlanned << "\n";
        std::cout << "---------------------------\n";
    }
};

#endif // MK_CODE_INTELLIGENCE_CPP
