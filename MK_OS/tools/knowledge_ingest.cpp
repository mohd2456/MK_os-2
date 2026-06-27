// ============================================================
// MK OS - Knowledge Ingestion Tool
// Reads files from disk and converts them into MK's .mk
// knowledge graph format (source|relation|target|weight).
//
// Compiles standalone:
//   g++ -std=c++17 tools/knowledge_ingest.cpp -o mk_ingest
//
// Usage:
//   ./mk_ingest <source_directory> <output.mk>
// ============================================================

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <sstream>
#include <set>

namespace fs = std::filesystem;

// ============================================================
// Utility functions
// ============================================================

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    auto end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::vector<std::string> split_sentences(const std::string& text) {
    std::vector<std::string> sentences;
    std::regex re(R"([^.!?]+[.!?])");
    auto begin = std::sregex_iterator(text.begin(), text.end(), re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        std::string s = trim(it->str());
        if (s.size() > 5 && s.size() < 200) {
            sentences.push_back(s);
        }
    }
    return sentences;
}

// ============================================================
// Extractors
// ============================================================

struct Fact {
    std::string source;
    std::string relation;
    std::string target;
    float weight;
    
    std::string to_mk() const {
        return source + "|" + relation + "|" + target + "|" + std::to_string(weight).substr(0, 4);
    }
};

// --- Text file extraction ---
std::vector<Fact> extract_from_text(const std::string& content, const std::string& filename) {
    std::vector<Fact> facts;
    auto sentences = split_sentences(content);
    
    for (const auto& sentence : sentences) {
        // Try to extract "X is Y" patterns
        std::regex is_pattern(R"((\w[\w\s]{1,30})\s+is\s+(a |an |the )?(\w[\w\s]{1,40}))");
        std::smatch match;
        if (std::regex_search(sentence, match, is_pattern)) {
            facts.push_back({
                trim(to_lower(match[1].str())),
                "is_a",
                trim(to_lower(match[3].str())),
                0.8f
            });
        }
        
        // "X has Y" patterns
        std::regex has_pattern(R"((\w[\w\s]{1,30})\s+has\s+(a |an |the )?(\w[\w\s]{1,40}))");
        if (std::regex_search(sentence, match, has_pattern)) {
            facts.push_back({
                trim(to_lower(match[1].str())),
                "has",
                trim(to_lower(match[3].str())),
                0.8f
            });
        }
        
        // "X can Y" patterns
        std::regex can_pattern(R"((\w[\w\s]{1,30})\s+can\s+(\w[\w\s]{1,40}))");
        if (std::regex_search(sentence, match, can_pattern)) {
            facts.push_back({
                trim(to_lower(match[1].str())),
                "can",
                trim(to_lower(match[3].str())),
                0.7f
            });
        }
        
        // "X uses Y" / "X uses Y" patterns
        std::regex uses_pattern(R"((\w[\w\s]{1,30})\s+uses?\s+(\w[\w\s]{1,40}))");
        if (std::regex_search(sentence, match, uses_pattern)) {
            facts.push_back({
                trim(to_lower(match[1].str())),
                "uses",
                trim(to_lower(match[2].str())),
                0.7f
            });
        }
        
        // "X contains Y" patterns
        std::regex contains_pattern(R"((\w[\w\s]{1,30})\s+contains?\s+(\w[\w\s]{1,40}))");
        if (std::regex_search(sentence, match, contains_pattern)) {
            facts.push_back({
                trim(to_lower(match[1].str())),
                "contains",
                trim(to_lower(match[2].str())),
                0.7f
            });
        }
        
        // Store the full sentence as a general fact
        if (sentence.size() > 10 && sentence.size() < 100) {
            // Extract subject (first few words) and predicate
            std::string subject = sentence.substr(0, sentence.find(' ', sentence.find(' ') + 1));
            facts.push_back({
                to_lower(trim(subject)),
                "fact_from",
                filename,
                0.6f
            });
        }
    }
    
    return facts;
}

// --- Code file extraction (C++, Python, JavaScript) ---
std::vector<Fact> extract_from_code(const std::string& content, const std::string& filename) {
    std::vector<Fact> facts;
    std::string lang;
    
    if (filename.ends_with(".cpp") || filename.ends_with(".h") || filename.ends_with(".hpp")) {
        lang = "cpp";
    } else if (filename.ends_with(".py")) {
        lang = "python";
    } else if (filename.ends_with(".js") || filename.ends_with(".ts")) {
        lang = "javascript";
    } else {
        lang = "code";
    }
    
    std::istringstream stream(content);
    std::string line;
    std::set<std::string> seen; // Avoid duplicates
    
    while (std::getline(stream, line)) {
        std::smatch match;
        
        // C++ class declarations
        std::regex cpp_class(R"(class\s+(\w+))");
        if (std::regex_search(line, match, cpp_class)) {
            std::string cls = match[1].str();
            if (seen.insert(cls).second) {
                facts.push_back({cls, "is_a", "class", 1.0f});
                facts.push_back({cls, "defined_in", filename, 0.9f});
                facts.push_back({cls, "language", lang, 1.0f});
            }
        }
        
        // C++ / JS function declarations
        std::regex func_decl(R"((?:void|int|bool|string|float|double|auto)\s+(\w+)\s*\()");
        if (std::regex_search(line, match, func_decl)) {
            std::string func = match[1].str();
            if (seen.insert(func).second) {
                facts.push_back({func, "is_a", "function", 1.0f});
                facts.push_back({func, "defined_in", filename, 0.9f});
            }
        }
        
        // Python function definitions
        std::regex py_def(R"(def\s+(\w+)\s*\()");
        if (std::regex_search(line, match, py_def)) {
            std::string func = match[1].str();
            if (seen.insert(func).second) {
                facts.push_back({func, "is_a", "python_function", 1.0f});
                facts.push_back({func, "defined_in", filename, 0.9f});
            }
        }
        
        // Python class definitions
        std::regex py_class(R"(class\s+(\w+))");
        if (std::regex_search(line, match, py_class)) {
            std::string cls = match[1].str();
            if (seen.insert(cls).second) {
                facts.push_back({cls, "is_a", "python_class", 1.0f});
                facts.push_back({cls, "defined_in", filename, 0.9f});
            }
        }
        
        // Import/include statements
        std::regex include_stmt(R"(#include\s*[<"]([^>"]+)[>"])");
        if (std::regex_search(line, match, include_stmt)) {
            std::string header = match[1].str();
            if (seen.insert("inc_" + header).second) {
                facts.push_back({filename, "includes", header, 0.8f});
            }
        }
        
        std::regex import_stmt(R"((?:import|from)\s+([\w.]+))");
        if (std::regex_search(line, match, import_stmt)) {
            std::string module = match[1].str();
            if (seen.insert("imp_" + module).second) {
                facts.push_back({filename, "imports", module, 0.8f});
            }
        }
        
        // JavaScript const/let/var with function
        std::regex js_func(R"((?:const|let|var)\s+(\w+)\s*=\s*(?:async\s*)?\()");
        if (std::regex_search(line, match, js_func)) {
            std::string func = match[1].str();
            if (seen.insert(func).second) {
                facts.push_back({func, "is_a", "js_function", 1.0f});
                facts.push_back({func, "defined_in", filename, 0.9f});
            }
        }
        
        // TODO/FIXME comments
        std::regex todo_pattern(R"((?://|#)\s*(?:TODO|FIXME|HACK|NOTE):\s*(.+))");
        if (std::regex_search(line, match, todo_pattern)) {
            facts.push_back({filename, "has_todo", trim(match[1].str()), 0.5f});
        }
    }
    
    // File-level facts
    facts.push_back({filename, "is_a", lang + "_file", 1.0f});
    
    return facts;
}

// --- Markdown file extraction ---
std::vector<Fact> extract_from_markdown(const std::string& content, const std::string& filename) {
    std::vector<Fact> facts;
    std::istringstream stream(content);
    std::string line;
    std::string current_header;
    
    while (std::getline(stream, line)) {
        // Headers become topics
        std::regex header_re(R"(^#{1,6}\s+(.+))");
        std::smatch match;
        if (std::regex_search(line, match, header_re)) {
            current_header = trim(to_lower(match[1].str()));
            facts.push_back({current_header, "is_a", "topic", 0.9f});
            facts.push_back({current_header, "documented_in", filename, 0.8f});
            continue;
        }
        
        // Definition patterns: "**term**: definition" or "- **term**: definition"
        std::regex def_re(R"(\*\*(\w[\w\s]{1,30})\*\*[:\s]+(.{5,100}))");
        if (std::regex_search(line, match, def_re)) {
            std::string term = trim(to_lower(match[1].str()));
            std::string definition = trim(match[2].str());
            facts.push_back({term, "defined_as", definition, 0.9f});
            if (!current_header.empty()) {
                facts.push_back({term, "related_to", current_header, 0.7f});
            }
        }
        
        // List items under headers become related facts
        std::regex list_re(R"(^[\s]*[-*]\s+(.{5,100}))");
        if (std::regex_search(line, match, list_re) && !current_header.empty()) {
            std::string item = trim(to_lower(match[1].str()));
            if (item.size() > 3 && item.size() < 80) {
                facts.push_back({current_header, "includes", item, 0.6f});
            }
        }
        
        // Also extract sentences from paragraph text
        if (!line.empty() && line[0] != '#' && line[0] != '-' && line[0] != '*') {
            auto sentence_facts = extract_from_text(line, filename);
            facts.insert(facts.end(), sentence_facts.begin(), sentence_facts.end());
        }
    }
    
    return facts;
}

// ============================================================
// Directory Scanner
// ============================================================

struct Stats {
    int files_processed = 0;
    int facts_extracted = 0;
    int txt_files = 0;
    int code_files = 0;
    int md_files = 0;
    int skipped = 0;
};

std::vector<Fact> scan_directory(const std::string& dir_path, Stats& stats) {
    std::vector<Fact> all_facts;
    
    for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
        if (!entry.is_regular_file()) continue;
        
        std::string path = entry.path().string();
        std::string filename = entry.path().filename().string();
        std::string ext = entry.path().extension().string();
        
        // Skip binary files, hidden files, and build artifacts
        if (filename[0] == '.' || ext == ".o" || ext == ".bin" || 
            ext == ".exe" || ext == ".so" || ext == ".dylib" ||
            ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" ||
            ext == ".zip" || ext == ".tar" || ext == ".gz") {
            stats.skipped++;
            continue;
        }
        
        // Read file content
        std::ifstream file(path);
        if (!file.is_open()) {
            stats.skipped++;
            continue;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // Skip very large files (> 1MB)
        if (content.size() > 1048576) {
            stats.skipped++;
            continue;
        }
        
        std::vector<Fact> facts;
        
        if (ext == ".txt") {
            facts = extract_from_text(content, filename);
            stats.txt_files++;
        } else if (ext == ".cpp" || ext == ".c" || ext == ".h" || ext == ".hpp" ||
                   ext == ".py" || ext == ".js" || ext == ".ts") {
            facts = extract_from_code(content, filename);
            stats.code_files++;
        } else if (ext == ".md" || ext == ".markdown") {
            facts = extract_from_markdown(content, filename);
            stats.md_files++;
        } else {
            // Try as plain text for other text-like files
            facts = extract_from_text(content, filename);
            stats.txt_files++;
        }
        
        all_facts.insert(all_facts.end(), facts.begin(), facts.end());
        stats.files_processed++;
        stats.facts_extracted += facts.size();
        
        // Progress indicator
        if (stats.files_processed % 10 == 0) {
            std::cout << "  [" << stats.files_processed << " files | " 
                      << stats.facts_extracted << " facts]\r" << std::flush;
        }
    }
    
    return all_facts;
}

// ============================================================
// Main
// ============================================================

void print_usage() {
    std::cout << R"(
  MK Knowledge Ingest Tool
  ─────────────────────────
  Reads files from disk and converts them into MK's
  knowledge graph format (source|relation|target|weight).

  Usage:
    ./mk_ingest <source_directory> <output.mk>

  Supported file types:
    .txt         → Sentence extraction (is_a, has, can, uses)
    .cpp/.py/.js → Code intelligence (classes, functions, imports)
    .md          → Documentation (headers, definitions, lists)

  Output format:
    source|relation|target|weight

  Example:
    ./mk_ingest ~/Documents/notes knowledge_files/learned.mk
    ./mk_ingest ./src knowledge_files/code_knowledge.mk
)" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage();
        return 1;
    }
    
    std::string source_dir = argv[1];
    std::string output_file = argv[2];
    
    // Validate source directory
    if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
        std::cerr << "  Error: '" << source_dir << "' is not a valid directory.\n";
        return 1;
    }
    
    std::cout << "\n  MK Knowledge Ingest\n";
    std::cout << "  ───────────────────\n";
    std::cout << "  Source: " << source_dir << "\n";
    std::cout << "  Output: " << output_file << "\n";
    std::cout << "  Scanning...\n\n";
    
    // Scan and extract
    Stats stats;
    auto facts = scan_directory(source_dir, stats);
    
    // Deduplicate facts
    std::set<std::string> seen;
    std::vector<Fact> unique_facts;
    for (const auto& f : facts) {
        std::string key = f.source + "|" + f.relation + "|" + f.target;
        if (seen.insert(key).second) {
            unique_facts.push_back(f);
        }
    }
    
    // Write output (append mode)
    std::ofstream out(output_file, std::ios::app);
    if (!out.is_open()) {
        std::cerr << "  Error: Cannot open output file '" << output_file << "'\n";
        return 1;
    }
    
    out << "\n# ═══ Ingested from: " << source_dir << " ═══\n";
    out << "# Date: " << __DATE__ << " " << __TIME__ << "\n";
    out << "# Files processed: " << stats.files_processed << "\n\n";
    
    for (const auto& fact : unique_facts) {
        out << fact.to_mk() << "\n";
    }
    
    out.close();
    
    // Print summary
    std::cout << "\n\n  ═══ Ingestion Complete ═══\n";
    std::cout << "  Files processed:  " << stats.files_processed << "\n";
    std::cout << "    Text files:     " << stats.txt_files << "\n";
    std::cout << "    Code files:     " << stats.code_files << "\n";
    std::cout << "    Markdown files: " << stats.md_files << "\n";
    std::cout << "    Skipped:        " << stats.skipped << "\n";
    std::cout << "  Facts extracted:  " << stats.facts_extracted << "\n";
    std::cout << "  Unique facts:     " << unique_facts.size() << "\n";
    std::cout << "  Written to:       " << output_file << "\n\n";
    
    return 0;
}
