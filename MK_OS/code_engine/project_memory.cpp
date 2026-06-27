#ifndef MK_PROJECT_MEMORY_CPP
#define MK_PROJECT_MEMORY_CPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>

// ============================================================================
// MKProjectMemory - Indexes a codebase for intelligent code navigation
// Parses .py, .cpp, .js files to extract functions, classes, and imports
// ============================================================================

enum class SymbolType {
    FUNCTION,
    CLASS,
    METHOD,
    IMPORT,
    VARIABLE,
    CONSTANT
};

struct CodeSymbol {
    std::string name;
    SymbolType type;
    std::string file_path;
    int line_number;
    std::string signature;
    std::string parent_class;
};

struct FileInfo {
    std::string path;
    std::string language;
    int line_count;
    std::vector<CodeSymbol> symbols;
    std::vector<std::string> imports;
    std::string brief_description;
};

struct SearchResult {
    CodeSymbol symbol;
    float relevance_score;
};

class MKProjectMemory {
private:
    std::string root_directory_;
    std::vector<FileInfo> indexed_files_;
    std::vector<CodeSymbol> all_symbols_;
    std::map<std::string, std::vector<CodeSymbol>> symbol_index_;
    std::set<std::string> supported_extensions_;
    bool indexed_;

    void initSupportedExtensions() {
        supported_extensions_ = {".py", ".cpp", ".cc", ".h", ".hpp", ".js", ".ts"};
    }

    std::string getExtension(const std::string& path) const {
        size_t pos = path.find_last_of('.');
        if (pos != std::string::npos) return path.substr(pos);
        return "";
    }

    std::string getLanguage(const std::string& ext) const {
        if (ext == ".py") return "python";
        if (ext == ".cpp" || ext == ".cc" || ext == ".h" || ext == ".hpp") return "cpp";
        if (ext == ".js") return "javascript";
        if (ext == ".ts") return "typescript";
        return "unknown";
    }

    bool isSupported(const std::string& path) const {
        return supported_extensions_.count(getExtension(path)) > 0;
    }

    std::string readFile(const std::string& path) const {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        return std::string((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
    }

    int countLines(const std::string& content) const {
        return std::count(content.begin(), content.end(), '\n') + 1;
    }

    std::vector<CodeSymbol> parsePython(const std::string& content, const std::string& path) {
        std::vector<CodeSymbol> symbols;
        std::istringstream stream(content);
        std::string line;
        int line_num = 0;
        std::string current_class;

        while (std::getline(stream, line)) {
            line_num++;
            std::string trimmed = line;
            size_t start = trimmed.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            trimmed = trimmed.substr(start);

            if (trimmed.find("def ") == 0) {
                CodeSymbol sym;
                sym.type = (start > 0 && !current_class.empty()) ? SymbolType::METHOD : SymbolType::FUNCTION;
                size_t name_start = 4;
                size_t name_end = trimmed.find('(', name_start);
                if (name_end != std::string::npos) {
                    sym.name = trimmed.substr(name_start, name_end - name_start);
                    sym.signature = trimmed.substr(0, trimmed.find(':'));
                }
                sym.file_path = path;
                sym.line_number = line_num;
                sym.parent_class = current_class;
                symbols.push_back(sym);
            }
            else if (trimmed.find("class ") == 0) {
                CodeSymbol sym;
                sym.type = SymbolType::CLASS;
                size_t name_start = 6;
                size_t name_end = trimmed.find_first_of("(:", name_start);
                if (name_end != std::string::npos) {
                    sym.name = trimmed.substr(name_start, name_end - name_start);
                } else {
                    sym.name = trimmed.substr(name_start);
                }
                sym.file_path = path;
                sym.line_number = line_num;
                current_class = sym.name;
                symbols.push_back(sym);
            }
            else if (trimmed.find("import ") == 0 || trimmed.find("from ") == 0) {
                CodeSymbol sym;
                sym.type = SymbolType::IMPORT;
                sym.name = trimmed;
                sym.file_path = path;
                sym.line_number = line_num;
                symbols.push_back(sym);
            }

            // Reset class context on dedent
            if (start == 0 && !current_class.empty() && trimmed.find("class ") != 0) {
                current_class.clear();
            }
        }
        return symbols;
    }

    std::vector<CodeSymbol> parseCpp(const std::string& content, const std::string& path) {
        std::vector<CodeSymbol> symbols;
        std::istringstream stream(content);
        std::string line;
        int line_num = 0;
        std::string current_class;

        while (std::getline(stream, line)) {
            line_num++;
            std::string trimmed = line;
            size_t start = trimmed.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            trimmed = trimmed.substr(start);

            if (trimmed.find("class ") == 0 || trimmed.find("struct ") == 0) {
                CodeSymbol sym;
                sym.type = SymbolType::CLASS;
                size_t name_start = (trimmed[0] == 'c') ? 6 : 7;
                size_t name_end = trimmed.find_first_of(" {:", name_start);
                if (name_end != std::string::npos) {
                    sym.name = trimmed.substr(name_start, name_end - name_start);
                }
                sym.file_path = path;
                sym.line_number = line_num;
                current_class = sym.name;
                symbols.push_back(sym);
            }
            else if (trimmed.find("#include") == 0) {
                CodeSymbol sym;
                sym.type = SymbolType::IMPORT;
                sym.name = trimmed;
                sym.file_path = path;
                sym.line_number = line_num;
                symbols.push_back(sym);
            }
            else if (trimmed.find("(") != std::string::npos && trimmed.find(";") == std::string::npos &&
                     trimmed.find("{") != std::string::npos && trimmed.find("if") == std::string::npos &&
                     trimmed.find("for") == std::string::npos && trimmed.find("while") == std::string::npos) {
                // Likely a function definition
                CodeSymbol sym;
                sym.type = SymbolType::FUNCTION;
                size_t paren = trimmed.find('(');
                size_t name_end = paren;
                size_t name_start = trimmed.find_last_of(" \t*&", paren - 1);
                if (name_start != std::string::npos) {
                    sym.name = trimmed.substr(name_start + 1, name_end - name_start - 1);
                }
                sym.signature = trimmed.substr(0, trimmed.find('{'));
                sym.file_path = path;
                sym.line_number = line_num;
                sym.parent_class = current_class;
                symbols.push_back(sym);
            }
        }
        return symbols;
    }

    std::vector<CodeSymbol> parseJavaScript(const std::string& content, const std::string& path) {
        std::vector<CodeSymbol> symbols;
        std::istringstream stream(content);
        std::string line;
        int line_num = 0;

        while (std::getline(stream, line)) {
            line_num++;
            std::string trimmed = line;
            size_t start = trimmed.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            trimmed = trimmed.substr(start);

            if (trimmed.find("function ") == 0) {
                CodeSymbol sym;
                sym.type = SymbolType::FUNCTION;
                size_t name_start = 9;
                size_t name_end = trimmed.find('(', name_start);
                if (name_end != std::string::npos) {
                    sym.name = trimmed.substr(name_start, name_end - name_start);
                    sym.signature = trimmed.substr(0, trimmed.find('{'));
                }
                sym.file_path = path;
                sym.line_number = line_num;
                symbols.push_back(sym);
            }
            else if (trimmed.find("class ") == 0) {
                CodeSymbol sym;
                sym.type = SymbolType::CLASS;
                size_t name_start = 6;
                size_t name_end = trimmed.find_first_of(" {", name_start);
                if (name_end != std::string::npos) {
                    sym.name = trimmed.substr(name_start, name_end - name_start);
                }
                sym.file_path = path;
                sym.line_number = line_num;
                symbols.push_back(sym);
            }
            else if (trimmed.find("const ") == 0 || trimmed.find("let ") == 0) {
                if (trimmed.find("=>") != std::string::npos || trimmed.find("function") != std::string::npos) {
                    CodeSymbol sym;
                    sym.type = SymbolType::FUNCTION;
                    size_t eq = trimmed.find('=');
                    size_t name_start = trimmed.find(' ') + 1;
                    if (eq != std::string::npos && name_start < eq) {
                        sym.name = trimmed.substr(name_start, eq - name_start - 1);
                    }
                    sym.file_path = path;
                    sym.line_number = line_num;
                    symbols.push_back(sym);
                }
            }
            else if (trimmed.find("require(") != std::string::npos || trimmed.find("import ") == 0) {
                CodeSymbol sym;
                sym.type = SymbolType::IMPORT;
                sym.name = trimmed;
                sym.file_path = path;
                sym.line_number = line_num;
                symbols.push_back(sym);
            }
        }
        return symbols;
    }

    void scanDirectory(const std::string& dir_path) {
        DIR* dir = opendir(dir_path.c_str());
        if (!dir) return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            if (name[0] == '.') continue; // skip hidden

            std::string full_path = dir_path + "/" + name;
            struct stat st;
            if (stat(full_path.c_str(), &st) != 0) continue;

            if (S_ISDIR(st.st_mode)) {
                // Skip common non-source directories
                if (name != "node_modules" && name != ".git" && name != "__pycache__" &&
                    name != "build" && name != "dist") {
                    scanDirectory(full_path);
                }
            } else if (isSupported(full_path)) {
                indexFile(full_path);
            }
        }
        closedir(dir);
    }

    void indexFile(const std::string& path) {
        std::string content = readFile(path);
        if (content.empty()) return;

        FileInfo info;
        info.path = path;
        info.language = getLanguage(getExtension(path));
        info.line_count = countLines(content);

        if (info.language == "python") {
            info.symbols = parsePython(content, path);
        } else if (info.language == "cpp") {
            info.symbols = parseCpp(content, path);
        } else if (info.language == "javascript" || info.language == "typescript") {
            info.symbols = parseJavaScript(content, path);
        }

        for (const auto& sym : info.symbols) {
            all_symbols_.push_back(sym);
            symbol_index_[sym.name].push_back(sym);
            if (sym.type == SymbolType::IMPORT) {
                info.imports.push_back(sym.name);
            }
        }
        indexed_files_.push_back(info);
    }

public:
    MKProjectMemory() : indexed_(false) {
        initSupportedExtensions();
    }

    void indexDirectory(const std::string& path) {
        root_directory_ = path;
        indexed_files_.clear();
        all_symbols_.clear();
        symbol_index_.clear();

        std::cout << "[MKProjectMemory] Indexing: " << path << std::endl;
        scanDirectory(path);
        indexed_ = true;
        std::cout << "[MKProjectMemory] Indexed " << indexed_files_.size() << " files, "
                  << all_symbols_.size() << " symbols" << std::endl;
    }

    std::vector<SearchResult> findFunction(const std::string& name) const {
        std::vector<SearchResult> results;
        std::string lower_name = name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

        for (const auto& sym : all_symbols_) {
            if (sym.type != SymbolType::FUNCTION && sym.type != SymbolType::METHOD) continue;
            std::string lower_sym = sym.name;
            std::transform(lower_sym.begin(), lower_sym.end(), lower_sym.begin(), ::tolower);

            if (lower_sym == lower_name) {
                results.push_back({sym, 1.0f});
            } else if (lower_sym.find(lower_name) != std::string::npos) {
                results.push_back({sym, 0.7f});
            }
        }

        std::sort(results.begin(), results.end(),
                  [](const SearchResult& a, const SearchResult& b) {
                      return a.relevance_score > b.relevance_score;
                  });
        return results;
    }

    std::vector<SearchResult> findClass(const std::string& name) const {
        std::vector<SearchResult> results;
        std::string lower_name = name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

        for (const auto& sym : all_symbols_) {
            if (sym.type != SymbolType::CLASS) continue;
            std::string lower_sym = sym.name;
            std::transform(lower_sym.begin(), lower_sym.end(), lower_sym.begin(), ::tolower);

            if (lower_sym == lower_name) {
                results.push_back({sym, 1.0f});
            } else if (lower_sym.find(lower_name) != std::string::npos) {
                results.push_back({sym, 0.6f});
            }
        }
        return results;
    }

    std::vector<FileInfo> getFileStructure() const {
        return indexed_files_;
    }

    bool isIndexed() const { return indexed_; }
    int getFileCount() const { return static_cast<int>(indexed_files_.size()); }
    int getSymbolCount() const { return static_cast<int>(all_symbols_.size()); }
    std::string getRootDirectory() const { return root_directory_; }
};

#endif // MK_PROJECT_MEMORY_CPP
