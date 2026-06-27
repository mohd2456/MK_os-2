#ifndef MK_CODE_GENERATOR_CPP
#define MK_CODE_GENERATOR_CPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <iostream>

// ============================================================================
// MKCodeGenerator - Produces code from natural language descriptions
// Supports Python, C++, JavaScript with built-in templates for common patterns
// ============================================================================

enum class Language {
    PYTHON,
    CPP,
    JAVASCRIPT,
    UNKNOWN
};

enum class PatternType {
    WEB_SERVER,
    FILE_IO,
    API_CLIENT,
    CRUD,
    SORTING,
    DATA_STRUCTURE,
    CLASS_DEFINITION,
    FUNCTION,
    UNIT_TEST,
    CLI_TOOL,
    UNKNOWN
};

struct GeneratedCode {
    std::string code;
    Language language;
    PatternType pattern;
    std::string description;
    std::vector<std::string> dependencies;
    int line_count;
};

struct TemplateVariable {
    std::string name;
    std::string value;
    std::string default_value;
};

class MKCodeGenerator {
private:
    std::map<std::string, std::map<Language, std::string>> templates_;
    std::map<std::string, PatternType> keyword_patterns_;

    void initializeKeywordPatterns() {
        keyword_patterns_["server"] = PatternType::WEB_SERVER;
        keyword_patterns_["web"] = PatternType::WEB_SERVER;
        keyword_patterns_["http"] = PatternType::WEB_SERVER;
        keyword_patterns_["api"] = PatternType::API_CLIENT;
        keyword_patterns_["rest"] = PatternType::API_CLIENT;
        keyword_patterns_["fetch"] = PatternType::API_CLIENT;
        keyword_patterns_["file"] = PatternType::FILE_IO;
        keyword_patterns_["read"] = PatternType::FILE_IO;
        keyword_patterns_["write"] = PatternType::FILE_IO;
        keyword_patterns_["crud"] = PatternType::CRUD;
        keyword_patterns_["database"] = PatternType::CRUD;
        keyword_patterns_["create"] = PatternType::CRUD;
        keyword_patterns_["sort"] = PatternType::SORTING;
        keyword_patterns_["algorithm"] = PatternType::SORTING;
        keyword_patterns_["class"] = PatternType::CLASS_DEFINITION;
        keyword_patterns_["object"] = PatternType::CLASS_DEFINITION;
        keyword_patterns_["function"] = PatternType::FUNCTION;
        keyword_patterns_["test"] = PatternType::UNIT_TEST;
        keyword_patterns_["cli"] = PatternType::CLI_TOOL;
        keyword_patterns_["command"] = PatternType::CLI_TOOL;
    }

    void initializeTemplates() {
        // Web Server Templates
        templates_["web_server"][Language::PYTHON] = 
            "from http.server import HTTPServer, BaseHTTPRequestHandler\n"
            "import json\n\n"
            "class {class_name}Handler(BaseHTTPRequestHandler):\n"
            "    def do_GET(self):\n"
            "        self.send_response(200)\n"
            "        self.send_header('Content-type', 'application/json')\n"
            "        self.end_headers()\n"
            "        response = {'message': '{message}', 'status': 'ok'}\n"
            "        self.wfile.write(json.dumps(response).encode())\n\n"
            "    def do_POST(self):\n"
            "        content_length = int(self.headers['Content-Length'])\n"
            "        body = self.rfile.read(content_length)\n"
            "        data = json.loads(body)\n"
            "        self.send_response(201)\n"
            "        self.send_header('Content-type', 'application/json')\n"
            "        self.end_headers()\n"
            "        self.wfile.write(json.dumps({'received': data}).encode())\n\n"
            "if __name__ == '__main__':\n"
            "    server = HTTPServer(('localhost', {port}), {class_name}Handler)\n"
            "    print('Server running on port {port}')\n"
            "    server.serve_forever()\n";

        templates_["web_server"][Language::JAVASCRIPT] = 
            "const http = require('http');\n\n"
            "const server = http.createServer((req, res) => {\n"
            "    if (req.method === 'GET') {\n"
            "        res.writeHead(200, {'Content-Type': 'application/json'});\n"
            "        res.end(JSON.stringify({message: '{message}', status: 'ok'}));\n"
            "    } else if (req.method === 'POST') {\n"
            "        let body = '';\n"
            "        req.on('data', chunk => body += chunk);\n"
            "        req.on('end', () => {\n"
            "            res.writeHead(201, {'Content-Type': 'application/json'});\n"
            "            res.end(JSON.stringify({received: JSON.parse(body)}));\n"
            "        });\n"
            "    }\n"
            "});\n\n"
            "server.listen({port}, () => console.log('Server on port {port}'));\n";

        // File IO Templates
        templates_["file_io"][Language::PYTHON] = 
            "import os\n\n"
            "class {class_name}:\n"
            "    def __init__(self, filepath):\n"
            "        self.filepath = filepath\n\n"
            "    def read(self):\n"
            "        with open(self.filepath, 'r') as f:\n"
            "            return f.read()\n\n"
            "    def write(self, content):\n"
            "        with open(self.filepath, 'w') as f:\n"
            "            f.write(content)\n\n"
            "    def append(self, content):\n"
            "        with open(self.filepath, 'a') as f:\n"
            "            f.write(content)\n\n"
            "    def exists(self):\n"
            "        return os.path.exists(self.filepath)\n\n"
            "    def delete(self):\n"
            "        if self.exists():\n"
            "            os.remove(self.filepath)\n";

        templates_["file_io"][Language::CPP] = 
            "#include <fstream>\n#include <string>\n#include <iostream>\n\n"
            "class {class_name} {\nprivate:\n    std::string filepath_;\n\n"
            "public:\n    {class_name}(const std::string& path) : filepath_(path) {}\n\n"
            "    std::string read() {\n"
            "        std::ifstream file(filepath_);\n"
            "        std::string content((std::istreambuf_iterator<char>(file)),\n"
            "                            std::istreambuf_iterator<char>());\n"
            "        return content;\n    }\n\n"
            "    void write(const std::string& content) {\n"
            "        std::ofstream file(filepath_);\n"
            "        file << content;\n    }\n\n"
            "    void append(const std::string& content) {\n"
            "        std::ofstream file(filepath_, std::ios::app);\n"
            "        file << content;\n    }\n};\n";

        // API Client Templates
        templates_["api_client"][Language::PYTHON] = 
            "import urllib.request\nimport json\n\n"
            "class {class_name}:\n"
            "    def __init__(self, base_url):\n"
            "        self.base_url = base_url\n\n"
            "    def get(self, endpoint, headers=None):\n"
            "        url = self.base_url + endpoint\n"
            "        req = urllib.request.Request(url, headers=headers or {})\n"
            "        with urllib.request.urlopen(req) as response:\n"
            "            return json.loads(response.read())\n\n"
            "    def post(self, endpoint, data, headers=None):\n"
            "        url = self.base_url + endpoint\n"
            "        h = headers or {'Content-Type': 'application/json'}\n"
            "        body = json.dumps(data).encode()\n"
            "        req = urllib.request.Request(url, data=body, headers=h)\n"
            "        with urllib.request.urlopen(req) as response:\n"
            "            return json.loads(response.read())\n";

        // Sorting Templates
        templates_["sorting"][Language::CPP] = 
            "#include <vector>\n#include <algorithm>\n#include <iostream>\n\n"
            "template<typename T>\n"
            "std::vector<T> bubbleSort(std::vector<T> arr) {\n"
            "    for (size_t i = 0; i < arr.size(); i++)\n"
            "        for (size_t j = 0; j < arr.size()-i-1; j++)\n"
            "            if (arr[j] > arr[j+1]) std::swap(arr[j], arr[j+1]);\n"
            "    return arr;\n}\n\n"
            "template<typename T>\n"
            "std::vector<T> quickSort(std::vector<T> arr) {\n"
            "    if (arr.size() <= 1) return arr;\n"
            "    T pivot = arr[arr.size()/2];\n"
            "    std::vector<T> left, middle, right;\n"
            "    for (const auto& x : arr) {\n"
            "        if (x < pivot) left.push_back(x);\n"
            "        else if (x == pivot) middle.push_back(x);\n"
            "        else right.push_back(x);\n"
            "    }\n"
            "    auto sorted_left = quickSort(left);\n"
            "    auto sorted_right = quickSort(right);\n"
            "    sorted_left.insert(sorted_left.end(), middle.begin(), middle.end());\n"
            "    sorted_left.insert(sorted_left.end(), sorted_right.begin(), sorted_right.end());\n"
            "    return sorted_left;\n}\n";
    }

    PatternType detectPattern(const std::string& description) const {
        std::string lower = description;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        std::map<PatternType, int> scores;
        for (const auto& kv : keyword_patterns_) {
            if (lower.find(kv.first) != std::string::npos) {
                scores[kv.second]++;
            }
        }

        if (scores.empty()) return PatternType::FUNCTION;

        PatternType best = PatternType::UNKNOWN;
        int best_score = 0;
        for (const auto& kv : scores) {
            if (kv.second > best_score) {
                best_score = kv.second;
                best = kv.first;
            }
        }
        return best;
    }

    Language parseLanguage(const std::string& lang) const {
        std::string lower = lang;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "python" || lower == "py") return Language::PYTHON;
        if (lower == "c++" || lower == "cpp") return Language::CPP;
        if (lower == "javascript" || lower == "js") return Language::JAVASCRIPT;
        return Language::UNKNOWN;
    }

    std::string patternToTemplateKey(PatternType pattern) const {
        switch (pattern) {
            case PatternType::WEB_SERVER: return "web_server";
            case PatternType::FILE_IO: return "file_io";
            case PatternType::API_CLIENT: return "api_client";
            case PatternType::SORTING: return "sorting";
            default: return "";
        }
    }

    std::string replaceVars(const std::string& tmpl, 
                            const std::vector<TemplateVariable>& vars) const {
        std::string result = tmpl;
        for (const auto& var : vars) {
            std::string placeholder = "{" + var.name + "}";
            std::string value = var.value.empty() ? var.default_value : var.value;
            size_t pos;
            while ((pos = result.find(placeholder)) != std::string::npos) {
                result.replace(pos, placeholder.length(), value);
            }
        }
        return result;
    }

public:
    MKCodeGenerator() {
        initializeKeywordPatterns();
        initializeTemplates();
    }

    GeneratedCode generate(const std::string& description, const std::string& language) {
        GeneratedCode result;
        result.language = parseLanguage(language);
        result.pattern = detectPattern(description);
        result.description = description;

        std::string key = patternToTemplateKey(result.pattern);
        if (!key.empty() && templates_.count(key) && templates_[key].count(result.language)) {
            std::vector<TemplateVariable> vars = {
                {"class_name", "MyApp", "App"},
                {"message", "Hello World", "Hello"},
                {"port", "8080", "8080"}
            };
            result.code = replaceVars(templates_[key][result.language], vars);
        } else {
            result.code = generateGenericCode(description, result.language);
        }

        result.line_count = std::count(result.code.begin(), result.code.end(), '\n') + 1;
        return result;
    }

    std::string fillTemplate(const std::string& tmpl, 
                             const std::vector<TemplateVariable>& vars) {
        return replaceVars(tmpl, vars);
    }

    std::string generateGenericCode(const std::string& desc, Language lang) const {
        switch (lang) {
            case Language::PYTHON:
                return "# " + desc + "\n\ndef main():\n    # TODO: implement\n    pass\n\nif __name__ == '__main__':\n    main()\n";
            case Language::CPP:
                return "// " + desc + "\n#include <iostream>\n\nint main() {\n    // TODO: implement\n    return 0;\n}\n";
            case Language::JAVASCRIPT:
                return "// " + desc + "\n\nfunction main() {\n    // TODO: implement\n}\n\nmain();\n";
            default:
                return "// " + desc + "\n// Unsupported language\n";
        }
    }

    std::string getLanguageName(Language lang) const {
        switch (lang) {
            case Language::PYTHON: return "Python";
            case Language::CPP: return "C++";
            case Language::JAVASCRIPT: return "JavaScript";
            default: return "Unknown";
        }
    }

    std::vector<std::string> getSupportedLanguages() const {
        return {"Python", "C++", "JavaScript"};
    }

    std::vector<std::string> getAvailablePatterns() const {
        return {"web_server", "file_io", "api_client", "crud", "sorting",
                "data_structure", "class", "function", "unit_test", "cli_tool"};
    }
};

#endif // MK_CODE_GENERATOR_CPP
