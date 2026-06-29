#ifndef MK_SHELL_CPP
#define MK_SHELL_CPP

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <deque>
#include <memory>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <functional>

namespace MK_Shell {

// ─── Token Types ────────────────────────────────────────────────────────────

enum class TokenType {
    WORD,
    PIPE,
    REDIRECT_IN,
    REDIRECT_OUT,
    REDIRECT_APPEND,
    SEMICOLON,
    AMPERSAND,
    ENV_VAR,
    STRING_LITERAL
};

struct Token {
    TokenType type;
    std::string value;
};

// ─── Parsed Command ─────────────────────────────────────────────────────────

struct Command {
    std::string name;
    std::vector<std::string> args;
    std::string input_redirect;
    std::string output_redirect;
    bool append_output = false;
    bool background = false;
};

struct Pipeline {
    std::vector<Command> commands; // commands connected by pipes
};


// ─── Command Parser ─────────────────────────────────────────────────────────

class CommandParser {
private:
    std::map<std::string, std::string>* env_vars_;

    std::string expand_variables(const std::string& input) const {
        std::string result;
        for (size_t i = 0; i < input.size(); ++i) {
            if (input[i] == '$' && i + 1 < input.size()) {
                std::string var_name;
                size_t j = i + 1;
                if (input[j] == '{') {
                    j++;
                    while (j < input.size() && input[j] != '}') {
                        var_name += input[j++];
                    }
                    if (j < input.size()) j++; // skip '}'
                } else {
                    while (j < input.size() && (std::isalnum(input[j]) || input[j] == '_')) {
                        var_name += input[j++];
                    }
                }
                auto it = env_vars_->find(var_name);
                if (it != env_vars_->end()) {
                    result += it->second;
                }
                i = j - 1;
            } else {
                result += input[i];
            }
        }
        return result;
    }

public:
    CommandParser(std::map<std::string, std::string>* env) : env_vars_(env) {}

    std::vector<Token> tokenize(const std::string& input) {
        std::vector<Token> tokens;
        size_t i = 0;
        while (i < input.size()) {
            while (i < input.size() && input[i] == ' ') i++;
            if (i >= input.size()) break;

            if (input[i] == '|') {
                tokens.push_back({TokenType::PIPE, "|"});
                i++;
            } else if (input[i] == ';') {
                tokens.push_back({TokenType::SEMICOLON, ";"});
                i++;
            } else if (input[i] == '&') {
                tokens.push_back({TokenType::AMPERSAND, "&"});
                i++;
            } else if (input[i] == '<') {
                tokens.push_back({TokenType::REDIRECT_IN, "<"});
                i++;
            } else if (input[i] == '>') {
                if (i + 1 < input.size() && input[i + 1] == '>') {
                    tokens.push_back({TokenType::REDIRECT_APPEND, ">>"});
                    i += 2;
                } else {
                    tokens.push_back({TokenType::REDIRECT_OUT, ">"});
                    i++;
                }
            } else if (input[i] == '"') {
                std::string word;
                i++;
                while (i < input.size() && input[i] != '"') {
                    if (input[i] == '\\' && i + 1 < input.size()) {
                        i++;
                    }
                    word += input[i++];
                }
                if (i < input.size()) i++; // skip closing "
                tokens.push_back({TokenType::STRING_LITERAL, expand_variables(word)});
            } else if (input[i] == '\'') {
                std::string word;
                i++;
                while (i < input.size() && input[i] != '\'') {
                    word += input[i++];
                }
                if (i < input.size()) i++;
                tokens.push_back({TokenType::STRING_LITERAL, word}); // no expansion in single quotes
            } else {
                std::string word;
                while (i < input.size() && input[i] != ' ' && input[i] != '|' &&
                       input[i] != ';' && input[i] != '&' && input[i] != '<' &&
                       input[i] != '>' && input[i] != '"' && input[i] != '\'') {
                    word += input[i++];
                }
                word = expand_variables(word);
                tokens.push_back({TokenType::WORD, word});
            }
        }
        return tokens;
    }

    Pipeline parse(const std::string& input) {
        Pipeline pipeline;
        auto tokens = tokenize(input);
        Command current_cmd;
        bool expect_redirect_in = false;
        bool expect_redirect_out = false;
        bool append_mode = false;

        for (const auto& tok : tokens) {
            if (tok.type == TokenType::PIPE) {
                if (!current_cmd.name.empty()) {
                    pipeline.commands.push_back(current_cmd);
                    current_cmd = Command();
                }
            } else if (tok.type == TokenType::REDIRECT_IN) {
                expect_redirect_in = true;
            } else if (tok.type == TokenType::REDIRECT_OUT) {
                expect_redirect_out = true;
                append_mode = false;
            } else if (tok.type == TokenType::REDIRECT_APPEND) {
                expect_redirect_out = true;
                append_mode = true;
            } else if (tok.type == TokenType::AMPERSAND) {
                current_cmd.background = true;
            } else if (tok.type == TokenType::SEMICOLON) {
                if (!current_cmd.name.empty()) {
                    pipeline.commands.push_back(current_cmd);
                    current_cmd = Command();
                }
            } else {
                // WORD or STRING_LITERAL
                if (expect_redirect_in) {
                    current_cmd.input_redirect = tok.value;
                    expect_redirect_in = false;
                } else if (expect_redirect_out) {
                    current_cmd.output_redirect = tok.value;
                    current_cmd.append_output = append_mode;
                    expect_redirect_out = false;
                } else if (current_cmd.name.empty()) {
                    current_cmd.name = tok.value;
                } else {
                    current_cmd.args.push_back(tok.value);
                }
            }
        }
        if (!current_cmd.name.empty()) {
            pipeline.commands.push_back(current_cmd);
        }
        return pipeline;
    }
};


// ─── Tab Completion ─────────────────────────────────────────────────────────

class TabCompleter {
private:
    std::vector<std::string> commands_;
    std::vector<std::string> paths_;

public:
    void register_commands(const std::vector<std::string>& cmds) {
        commands_ = cmds;
    }

    void set_paths(const std::vector<std::string>& paths) {
        paths_ = paths;
    }

    std::vector<std::string> complete(const std::string& partial, bool is_command = true) const {
        std::vector<std::string> matches;
        const auto& source = is_command ? commands_ : paths_;
        for (const auto& item : source) {
            if (item.find(partial) == 0) {
                matches.push_back(item);
            }
        }
        std::sort(matches.begin(), matches.end());
        return matches;
    }
};

// ─── Command History ────────────────────────────────────────────────────────

class CommandHistory {
private:
    std::deque<std::string> history_;
    size_t max_size_ = 1000;
    int cursor_ = -1;

public:
    void add(const std::string& cmd) {
        if (!cmd.empty() && (history_.empty() || history_.back() != cmd)) {
            history_.push_back(cmd);
            if (history_.size() > max_size_) history_.pop_front();
        }
        cursor_ = static_cast<int>(history_.size());
    }

    std::string previous() {
        if (history_.empty()) return "";
        if (cursor_ > 0) cursor_--;
        return history_[cursor_];
    }

    std::string next() {
        if (cursor_ < static_cast<int>(history_.size()) - 1) {
            cursor_++;
            return history_[cursor_];
        }
        cursor_ = static_cast<int>(history_.size());
        return "";
    }

    std::vector<std::string> get_all() const {
        return std::vector<std::string>(history_.begin(), history_.end());
    }

    std::vector<std::string> search(const std::string& query) const {
        std::vector<std::string> results;
        for (const auto& cmd : history_) {
            if (cmd.find(query) != std::string::npos) {
                results.push_back(cmd);
            }
        }
        return results;
    }

    size_t size() const { return history_.size(); }
    void clear() { history_.clear(); cursor_ = -1; }
};


// ─── MK Shell ───────────────────────────────────────────────────────────────

class MKShell {
private:
    std::map<std::string, std::string> env_vars_;
    CommandParser parser_;
    TabCompleter completer_;
    CommandHistory history_;
    std::string current_dir_;
    bool running_ = false;
    std::string last_output_;

    using BuiltinFunc = std::function<std::string(const std::vector<std::string>&)>;
    std::map<std::string, BuiltinFunc> builtins_;

    void register_builtins() {
        builtins_["echo"] = [](const std::vector<std::string>& args) -> std::string {
            std::string result;
            for (size_t i = 0; i < args.size(); ++i) {
                result += args[i];
                if (i + 1 < args.size()) result += " ";
            }
            return result + "\n";
        };

        builtins_["pwd"] = [this](const std::vector<std::string>&) -> std::string {
            return current_dir_ + "\n";
        };

        builtins_["cd"] = [this](const std::vector<std::string>& args) -> std::string {
            if (args.empty()) {
                current_dir_ = env_vars_["HOME"];
            } else {
                std::string target = args[0];
                if (target[0] != '/') {
                    target = current_dir_ + "/" + target;
                }
                current_dir_ = target;
            }
            env_vars_["PWD"] = current_dir_;
            return "";
        };

        builtins_["ls"] = [this](const std::vector<std::string>& args) -> std::string {
            std::string dir = args.empty() ? current_dir_ : args[0];
            return "[listing of " + dir + "]\n";
        };

        builtins_["cat"] = [](const std::vector<std::string>& args) -> std::string {
            if (args.empty()) return "cat: missing operand\n";
            return "[contents of " + args[0] + "]\n";
        };

        builtins_["clear"] = [](const std::vector<std::string>&) -> std::string {
            return "\033[2J\033[H";
        };

        builtins_["history"] = [this](const std::vector<std::string>&) -> std::string {
            std::string result;
            auto all = history_.get_all();
            for (size_t i = 0; i < all.size(); ++i) {
                result += std::to_string(i + 1) + "  " + all[i] + "\n";
            }
            return result;
        };

        builtins_["help"] = [](const std::vector<std::string>&) -> std::string {
            std::string result = "MK Shell Built-in Commands:\n";
            result += "  ls       - List directory contents\n";
            result += "  cd       - Change directory\n";
            result += "  cat      - Display file contents\n";
            result += "  echo     - Print text\n";
            result += "  pwd      - Print working directory\n";
            result += "  help     - Show this help\n";
            result += "  clear    - Clear the screen\n";
            result += "  history  - Show command history\n";
            result += "  exit     - Exit the shell\n";
            result += "\nMK-Specific Commands:\n";
            result += "  mk_learn   - Teach MK something new\n";
            result += "  mk_status  - Show MK system status\n";
            result += "  mk_think   - Ask MK to reason about something\n";
            result += "  mk_build   - Build/compile MK components\n";
            result += "  mk_search  - Search MK knowledge base\n";
            return result;
        };

        builtins_["exit"] = [this](const std::vector<std::string>&) -> std::string {
            running_ = false;
            return "Goodbye.\n";
        };

        // MK-specific commands
        builtins_["mk_learn"] = [](const std::vector<std::string>& args) -> std::string {
            if (args.empty()) return "Usage: mk_learn <fact>\n";
            std::string fact;
            for (const auto& a : args) fact += a + " ";
            return "MK learned: " + fact + "\n";
        };

        builtins_["mk_status"] = [](const std::vector<std::string>&) -> std::string {
            std::string result = "=== MK System Status ===\n";
            result += "AI Core: ACTIVE\n";
            result += "Memory: NOMINAL\n";
            result += "Network: CONNECTED\n";
            result += "Uptime: RUNNING\n";
            return result;
        };

        builtins_["mk_think"] = [](const std::vector<std::string>& args) -> std::string {
            if (args.empty()) return "Usage: mk_think <query>\n";
            std::string query;
            for (const auto& a : args) query += a + " ";
            return "MK is thinking about: " + query + "\n[Reasoning in progress...]\n";
        };

        builtins_["mk_build"] = [](const std::vector<std::string>& args) -> std::string {
            std::string target = args.empty() ? "all" : args[0];
            return "Building MK component: " + target + "\n[Build started...]\n";
        };

        builtins_["mk_search"] = [](const std::vector<std::string>& args) -> std::string {
            if (args.empty()) return "Usage: mk_search <query>\n";
            std::string query;
            for (const auto& a : args) query += a + " ";
            return "Searching knowledge base for: " + query + "\n";
        };
    }

public:
    MKShell() : parser_(&env_vars_) {
        env_vars_["HOME"] = "/home/mk";
        env_vars_["PATH"] = "/mk/bin:/mk/system:/mk/plugins";
        env_vars_["MK_MODE"] = "interactive";
        env_vars_["USER"] = "mk";
        env_vars_["SHELL"] = "/mk/system/shell";
        current_dir_ = env_vars_["HOME"];
        env_vars_["PWD"] = current_dir_;

        register_builtins();

        // Set up tab completer with known commands
        std::vector<std::string> cmd_names;
        for (const auto& p : builtins_) cmd_names.push_back(p.first);
        completer_.register_commands(cmd_names);
    }

    // ── Execute ──

    std::string execute(const std::string& input) {
        if (input.empty()) return "";
        history_.add(input);

        Pipeline pipeline = parser_.parse(input);
        std::string pipe_input;
        std::string final_output;

        for (size_t i = 0; i < pipeline.commands.size(); ++i) {
            auto& cmd = pipeline.commands[i];
            std::string output = execute_command(cmd, pipe_input);
            if (i + 1 < pipeline.commands.size()) {
                pipe_input = output; // pipe to next
            } else {
                final_output = output;
            }
        }

        last_output_ = final_output;
        return final_output;
    }

    std::string execute_command(const Command& cmd, const std::string& /*stdin_data*/ = "") {
        auto it = builtins_.find(cmd.name);
        if (it != builtins_.end()) {
            return it->second(cmd.args);
        }
        return "mk_shell: " + cmd.name + ": command not found\n";
    }

    // ── Script Execution ──

    std::string execute_script(const std::string& script_content) {
        std::string output;
        std::istringstream stream(script_content);
        std::string line;
        while (std::getline(stream, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;
            // Trim whitespace
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            line = line.substr(start);
            output += execute(line);
        }
        return output;
    }

    // ── Tab Completion ──

    std::vector<std::string> tab_complete(const std::string& partial) {
        return completer_.complete(partial, true);
    }

    // ── Environment Variables ──

    void set_env(const std::string& key, const std::string& value) {
        env_vars_[key] = value;
    }

    std::string get_env(const std::string& key) const {
        auto it = env_vars_.find(key);
        return (it != env_vars_.end()) ? it->second : "";
    }

    std::map<std::string, std::string> get_all_env() const { return env_vars_; }

    // ── History ──

    CommandHistory& get_history() { return history_; }
    const CommandHistory& get_history() const { return history_; }

    // ── Prompt ──

    std::string get_prompt() const {
        return env_vars_.at("USER") + "@mk:" + current_dir_ + "$ ";
    }

    // ── State ──

    bool is_running() const { return running_; }
    void start() { running_ = true; }
    void stop() { running_ = false; }
    std::string get_current_dir() const { return current_dir_; }
};

} // namespace MK_Shell

#endif // MK_SHELL_CPP
