#ifndef MK_PROJECT_INDEXER_CPP
#define MK_PROJECT_INDEXER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <regex>

// ===========================================================================
// MK PROJECT INDEXER - Full Project Structure Analyzer
// ===========================================================================
// Indexes entire project directories. Walks filesystem, identifies file
// types, extracts function/class signatures, builds a searchable index.
// Used for 'find where X is defined' and 'show all files that use Y'.
// ===========================================================================

namespace fs = std::filesystem;

// A symbol found in the project (function, class, variable, etc.)
struct MKSymbol {
    std::string name;
    std::string type;           // function, class, variable, method, import
    std::string file_path;
    int line_number;
    std::string signature;      // Full signature for functions
    std::string parent_class;   // If method, which class it belongs to
    std::string language;
};

// A file entry in the project index
struct MKFileEntry {
    std::string path;
    std::string relative_path;
    std::string language;
    std::string extension;
    size_t size_bytes;
    int line_count;
    std::vector<std::string> imports;
    std::vector<MKSymbol> symbols;
    std::string last_modified;
};

// Project-wide symbol table
struct MKSymbolTable {
    std::vector<MKSymbol> all_symbols;
    std::unordered_map<std::string, std::vector<int>> name_index;  // name -> indices
    std::unordered_map<std::string, std::vector<int>> type_index;  // type -> indices
    std::unordered_map<std::string, std::vector<int>> file_index;  // file -> indices
};

// Complete project index
struct MKProjectIndex {
    std::string root_path;
    std::string project_name;
    int total_files;
    int total_lines;
    size_t total_bytes;
    std::vector<MKFileEntry> files;
    MKSymbolTable symbol_table;
    std::unordered_map<std::string, int> language_stats;  // language -> file count
    std::vector<std::string> ignored_patterns;
};

class MKProjectIndexer {
private:
    MKProjectIndex index;
    bool indexed;

    // Determine language from file extension
    std::string detectLanguage(const std::string& ext) {
        static std::unordered_map<std::string, std::string> ext_map = {
            {".py", "python"}, {".js", "javascript"}, {".ts", "typescript"},
            {".cpp", "cpp"}, {".hpp", "cpp"}, {".h", "cpp"}, {".c", "c"},
            {".rs", "rust"}, {".go", "go"}, {".java", "java"},
            {".rb", "ruby"}, {".php", "php"}, {".swift", "swift"},
            {".kt", "kotlin"}, {".scala", "scala"}, {".hs", "haskell"},
            {".sh", "bash"}, {".sql", "sql"}, {".html", "html"},
            {".css", "css"}, {".json", "json"}, {".yaml", "yaml"},
            {".yml", "yaml"}, {".md", "markdown"}, {".xml", "xml"},
        };
        auto it = ext_map.find(ext);
        return (it != ext_map.end()) ? it->second : "unknown";
    }

    // Check if path should be ignored
    bool shouldIgnore(const std::string& path) {
        for (const auto& pattern : index.ignored_patterns) {
            if (path.find(pattern) != std::string::npos) return true;
        }
        return false;
    }

    // Count lines in a file
    int countLines(const std::string& filepath) {
        std::ifstream file(filepath);
        int count = 0;
        std::string line;
        while (std::getline(file, line)) count++;
        return count;
    }

    // Extract symbols from Python file
    std::vector<MKSymbol> extractPythonSymbols(const std::string& filepath) {
        std::vector<MKSymbol> symbols;
        std::ifstream file(filepath);
        std::string line;
        int line_num = 0;
        std::string current_class;

        while (std::getline(file, line)) {
            line_num++;
            // Detect class definitions
            if (line.find("class ") == 0 || line.find("class ") != std::string::npos) {
                size_t start = line.find("class ") + 6;
                size_t end = line.find_first_of("(:", start);
                if (end != std::string::npos) {
                    std::string name = line.substr(start, end - start);
                    symbols.push_back({name, "class", filepath, line_num, line, "", "python"});
                    current_class = name;
                }
            }
            // Detect function definitions
            if (line.find("def ") != std::string::npos) {
                size_t start = line.find("def ") + 4;
                size_t end = line.find('(', start);
                if (end != std::string::npos) {
                    std::string name = line.substr(start, end - start);
                    std::string type = current_class.empty() ? "function" : "method";
                    symbols.push_back({name, type, filepath, line_num, line, current_class, "python"});
                }
            }
            // Detect imports
            if (line.find("import ") == 0 || line.find("from ") == 0) {
                symbols.push_back({line, "import", filepath, line_num, line, "", "python"});
            }
            // Reset class context on non-indented lines
            if (!line.empty() && line[0] != ' ' && line[0] != '\t' &&
                line.find("class ") == std::string::npos) {
                current_class = "";
            }
        }
        return symbols;
    }

    // Extract symbols from C/C++ file
    std::vector<MKSymbol> extractCppSymbols(const std::string& filepath) {
        std::vector<MKSymbol> symbols;
        std::ifstream file(filepath);
        std::string line;
        int line_num = 0;
        std::string current_class;

        while (std::getline(file, line)) {
            line_num++;
            // Detect class/struct
            if (line.find("class ") != std::string::npos || line.find("struct ") != std::string::npos) {
                std::string keyword = (line.find("class ") != std::string::npos) ? "class " : "struct ";
                size_t start = line.find(keyword) + keyword.length();
                size_t end = line.find_first_of(" {:", start);
                if (end != std::string::npos && start < end) {
                    std::string name = line.substr(start, end - start);
                    symbols.push_back({name, "class", filepath, line_num, line, "", "cpp"});
                    current_class = name;
                }
            }
            // Detect function definitions (simplified heuristic)
            if (line.find('(') != std::string::npos && line.find(';') == std::string::npos &&
                line.find("#") != 0 && line.find("//") != 0) {
                size_t paren = line.find('(');
                size_t name_start = line.rfind(' ', paren);
                if (name_start != std::string::npos && name_start < paren) {
                    std::string name = line.substr(name_start + 1, paren - name_start - 1);
                    if (!name.empty() && name != "if" && name != "while" && name != "for" &&
                        name != "switch" && name != "return" && name != "catch") {
                        symbols.push_back({name, "function", filepath, line_num, line, current_class, "cpp"});
                    }
                }
            }
            // Detect includes
            if (line.find("#include") == 0) {
                symbols.push_back({line, "import", filepath, line_num, line, "", "cpp"});
            }
        }
        return symbols;
    }

    // Extract symbols from JavaScript file
    std::vector<MKSymbol> extractJSSymbols(const std::string& filepath) {
        std::vector<MKSymbol> symbols;
        std::ifstream file(filepath);
        std::string line;
        int line_num = 0;

        while (std::getline(file, line)) {
            line_num++;
            // Function declarations
            if (line.find("function ") != std::string::npos) {
                size_t start = line.find("function ") + 9;
                size_t end = line.find('(', start);
                if (end != std::string::npos) {
                    std::string name = line.substr(start, end - start);
                    symbols.push_back({name, "function", filepath, line_num, line, "", "javascript"});
                }
            }
            // Arrow functions assigned to const/let
            if ((line.find("const ") != std::string::npos || line.find("let ") != std::string::npos) &&
                line.find("=>") != std::string::npos) {
                size_t start = line.find_first_of(" ") + 1;
                size_t end = line.find(' ', start);
                if (end != std::string::npos) {
                    std::string name = line.substr(start, end - start);
                    symbols.push_back({name, "function", filepath, line_num, line, "", "javascript"});
                }
            }
            // Class declarations
            if (line.find("class ") != std::string::npos) {
                size_t start = line.find("class ") + 6;
                size_t end = line.find_first_of(" {", start);
                if (end != std::string::npos) {
                    std::string name = line.substr(start, end - start);
                    symbols.push_back({name, "class", filepath, line_num, line, "", "javascript"});
                }
            }
            // Require/import
            if (line.find("require(") != std::string::npos || line.find("import ") == 0) {
                symbols.push_back({line, "import", filepath, line_num, line, "", "javascript"});
            }
        }
        return symbols;
    }

    // Index a single file
    MKFileEntry indexFile(const std::string& filepath, const std::string& root) {
        MKFileEntry entry;
        entry.path = filepath;
        entry.relative_path = filepath.substr(root.length());
        entry.extension = fs::path(filepath).extension().string();
        entry.language = detectLanguage(entry.extension);
        entry.size_bytes = fs::file_size(filepath);
        entry.line_count = countLines(filepath);

        // Extract symbols based on language
        if (entry.language == "python") {
            entry.symbols = extractPythonSymbols(filepath);
        } else if (entry.language == "cpp" || entry.language == "c") {
            entry.symbols = extractCppSymbols(filepath);
        } else if (entry.language == "javascript" || entry.language == "typescript") {
            entry.symbols = extractJSSymbols(filepath);
        }

        // Extract imports
        for (const auto& sym : entry.symbols) {
            if (sym.type == "import") entry.imports.push_back(sym.name);
        }
        return entry;
    }

    // Build symbol table from all files
    void buildSymbolTable() {
        index.symbol_table.all_symbols.clear();
        index.symbol_table.name_index.clear();
        index.symbol_table.type_index.clear();
        index.symbol_table.file_index.clear();

        for (const auto& file : index.files) {
            for (const auto& sym : file.symbols) {
                int idx = static_cast<int>(index.symbol_table.all_symbols.size());
                index.symbol_table.all_symbols.push_back(sym);
                index.symbol_table.name_index[sym.name].push_back(idx);
                index.symbol_table.type_index[sym.type].push_back(idx);
                index.symbol_table.file_index[sym.file_path].push_back(idx);
            }
        }
    }

public:
    MKProjectIndexer() : indexed(false) {
        index.ignored_patterns = {
            "node_modules", ".git", "__pycache__", ".pyc", "target/debug",
            "build/", "dist/", ".o", ".so", ".dylib", "venv", ".env"
        };
    }

    // Index an entire directory recursively
    MKProjectIndex indexProject(const std::string& root_path) {
        index.root_path = root_path;
        index.project_name = fs::path(root_path).filename().string();
        index.total_files = 0;
        index.total_lines = 0;
        index.total_bytes = 0;
        index.files.clear();
        index.language_stats.clear();

        try {
            for (const auto& entry : fs::recursive_directory_iterator(root_path)) {
                if (!entry.is_regular_file()) continue;
                std::string path = entry.path().string();
                if (shouldIgnore(path)) continue;

                MKFileEntry file_entry = indexFile(path, root_path);
                index.total_files++;
                index.total_lines += file_entry.line_count;
                index.total_bytes += file_entry.size_bytes;
                index.language_stats[file_entry.language]++;
                index.files.push_back(file_entry);
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "[MKProjectIndexer] Error: " << e.what() << std::endl;
        }

        buildSymbolTable();
        indexed = true;
        return index;
    }

    // Search for a symbol by name
    std::vector<MKSymbol> findSymbol(const std::string& name) {
        std::vector<MKSymbol> results;
        if (!indexed) return results;
        std::string lower_name = name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

        for (const auto& sym : index.symbol_table.all_symbols) {
            std::string lower_sym = sym.name;
            std::transform(lower_sym.begin(), lower_sym.end(), lower_sym.begin(), ::tolower);
            if (lower_sym.find(lower_name) != std::string::npos) {
                results.push_back(sym);
            }
        }
        return results;
    }

    // Find all files that use/import a given module
    std::vector<std::string> findUsages(const std::string& module_name) {
        std::vector<std::string> results;
        if (!indexed) return results;
        for (const auto& file : index.files) {
            for (const auto& imp : file.imports) {
                if (imp.find(module_name) != std::string::npos) {
                    results.push_back(file.relative_path);
                    break;
                }
            }
        }
        return results;
    }

    // Get project summary
    std::string getSummary() const {
        if (!indexed) return "Project not indexed yet.";
        std::stringstream ss;
        ss << "Project: " << index.project_name << "\n";
        ss << "Root: " << index.root_path << "\n";
        ss << "Files: " << index.total_files << "\n";
        ss << "Lines: " << index.total_lines << "\n";
        ss << "Size: " << (index.total_bytes / 1024) << " KB\n";
        ss << "Symbols: " << index.symbol_table.all_symbols.size() << "\n";
        ss << "Languages:\n";
        for (const auto& [lang, count] : index.language_stats) {
            ss << "  " << lang << ": " << count << " files\n";
        }
        return ss.str();
    }

    // List all functions in the project
    std::vector<MKSymbol> listFunctions() {
        std::vector<MKSymbol> funcs;
        if (!indexed) return funcs;
        auto it = index.symbol_table.type_index.find("function");
        if (it != index.symbol_table.type_index.end()) {
            for (int idx : it->second) {
                funcs.push_back(index.symbol_table.all_symbols[idx]);
            }
        }
        return funcs;
    }

    // List all classes
    std::vector<MKSymbol> listClasses() {
        std::vector<MKSymbol> classes;
        if (!indexed) return classes;
        auto it = index.symbol_table.type_index.find("class");
        if (it != index.symbol_table.type_index.end()) {
            for (int idx : it->second) {
                classes.push_back(index.symbol_table.all_symbols[idx]);
            }
        }
        return classes;
    }

    // Check if indexed
    bool isIndexed() const { return indexed; }
    const MKProjectIndex& getIndex() const { return index; }

    void printStats() const {
        std::cout << "[MKProjectIndexer] " << (indexed ? "Indexed" : "Not indexed");
        if (indexed) {
            std::cout << " - " << index.total_files << " files, "
                      << index.symbol_table.all_symbols.size() << " symbols";
        }
        std::cout << std::endl;
    }
};

#endif // MK_PROJECT_INDEXER_CPP
