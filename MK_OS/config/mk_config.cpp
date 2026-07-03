// ============================================================
// MK OS - Configuration System
// Loads settings from ~/.mk_os/config.json
// Falls back to sensible defaults if file is missing
// Minimal JSON parsing (no external library dependencies)
// ============================================================
#ifndef MK_CONFIG_CPP
#define MK_CONFIG_CPP

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cstdlib>

// ===================================================================================
// MK CONFIG - Centralized configuration for the entire MK OS
// ===================================================================================

struct MKConfigDevice {
    std::string hostname;
    std::string ip;
    std::string ssh_key;
};

struct MKRiskLimits {
    int max_position_pct;
    int daily_loss_limit_usd;
    int drawdown_pause_pct;
};

struct MKLearningSchedule {
    int start_hour;
    int end_hour;
};

struct MKCgroupLimits {
    int memory_mb;
    int cpu_percent;
};

class MKConfig {
private:
    // Raw key-value store for simple values
    std::map<std::string, std::string> string_values_;
    std::map<std::string, int> int_values_;
    std::map<std::string, float> float_values_;
    std::map<std::string, bool> bool_values_;
    std::vector<MKConfigDevice> devices_;
    MKRiskLimits risk_limits_;
    MKLearningSchedule learning_schedule_;
    MKCgroupLimits cgroup_limits_;

    std::string config_path_;
    bool loaded_;
    std::vector<std::string> errors_;

    // Trim whitespace
    std::string trim(const std::string& s) const {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    // Remove quotes from a string value
    std::string unquote(const std::string& s) const {
        std::string trimmed = trim(s);
        if (trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"') {
            return trimmed.substr(1, trimmed.size() - 2);
        }
        return trimmed;
    }

    // Simple JSON parser for flat and simple nested structures
    // Handles: strings, numbers, booleans, simple objects, arrays of objects
    void parseJson(const std::string& json) {
        // State: read key-value pairs at the top level
        size_t pos = 0;
        size_t len = json.size();

        // Skip to first '{'
        while (pos < len && json[pos] != '{') pos++;
        if (pos >= len) {
            errors_.push_back("Invalid JSON: no opening brace found");
            return;
        }
        pos++; // skip '{'

        parseObject(json, pos, "");
    }

    // Parse a JSON object starting after '{'
    void parseObject(const std::string& json, size_t& pos, const std::string& prefix) {
        size_t len = json.size();

        while (pos < len) {
            skipWhitespace(json, pos);
            if (pos >= len) break;

            // End of object
            if (json[pos] == '}') { pos++; return; }
            if (json[pos] == ',') { pos++; continue; }

            // Read key
            std::string key = readString(json, pos);
            if (key.empty()) { pos++; continue; }

            skipWhitespace(json, pos);
            if (pos >= len || json[pos] != ':') { pos++; continue; }
            pos++; // skip ':'
            skipWhitespace(json, pos);

            std::string fullKey = prefix.empty() ? key : (prefix + "." + key);

            // Determine value type
            if (pos >= len) break;

            if (json[pos] == '"') {
                // String value
                std::string val = readString(json, pos);
                string_values_[fullKey] = val;
            } else if (json[pos] == '{') {
                // Nested object
                pos++; // skip '{'
                parseObject(json, pos, fullKey);
            } else if (json[pos] == '[') {
                // Array
                parseArray(json, pos, fullKey);
            } else if (json[pos] == 't' || json[pos] == 'f') {
                // Boolean
                if (json.substr(pos, 4) == "true") {
                    bool_values_[fullKey] = true;
                    pos += 4;
                } else if (json.substr(pos, 5) == "false") {
                    bool_values_[fullKey] = false;
                    pos += 5;
                } else {
                    pos++;
                }
            } else if (json[pos] == '-' || std::isdigit(json[pos])) {
                // Number
                std::string numStr = readNumber(json, pos);
                if (numStr.find('.') != std::string::npos) {
                    try { float_values_[fullKey] = std::stof(numStr); } catch (...) {}
                } else {
                    try { int_values_[fullKey] = std::stoi(numStr); } catch (...) {}
                }
            } else if (json[pos] == 'n') {
                // null
                pos += 4;
            } else {
                pos++;
            }
        }
    }

    void parseArray(const std::string& json, size_t& pos, const std::string& key) {
        pos++; // skip '['
        size_t len = json.size();
        int index = 0;

        while (pos < len) {
            skipWhitespace(json, pos);
            if (pos >= len) break;
            if (json[pos] == ']') { pos++; return; }
            if (json[pos] == ',') { pos++; continue; }

            if (json[pos] == '{') {
                // Array of objects
                pos++;
                std::string itemPrefix = key + "[" + std::to_string(index) + "]";
                parseObject(json, pos, itemPrefix);
                index++;
            } else if (json[pos] == '"') {
                std::string val = readString(json, pos);
                string_values_[key + "[" + std::to_string(index) + "]"] = val;
                index++;
            } else {
                pos++;
            }
        }
    }

    void skipWhitespace(const std::string& json, size_t& pos) const {
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
               json[pos] == '\r' || json[pos] == '\n')) {
            pos++;
        }
    }

    std::string readString(const std::string& json, size_t& pos) const {
        if (pos >= json.size() || json[pos] != '"') return "";
        pos++; // skip opening quote
        std::string result;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                pos++;
                switch (json[pos]) {
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    case 'r': result += '\r'; break;
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    default: result += json[pos]; break;
                }
            } else {
                result += json[pos];
            }
            pos++;
        }
        if (pos < json.size()) pos++; // skip closing quote
        return result;
    }

    std::string readNumber(const std::string& json, size_t& pos) const {
        std::string result;
        while (pos < json.size() && (std::isdigit(json[pos]) || json[pos] == '.' ||
               json[pos] == '-' || json[pos] == 'e' || json[pos] == 'E' || json[pos] == '+')) {
            result += json[pos];
            pos++;
        }
        return result;
    }

    // Apply parsed values to structured config
    void applyParsedValues() {
        // Risk limits
        auto it_int = int_values_.find("risk_limits.max_position_pct");
        if (it_int != int_values_.end()) risk_limits_.max_position_pct = it_int->second;

        it_int = int_values_.find("risk_limits.daily_loss_limit_usd");
        if (it_int != int_values_.end()) risk_limits_.daily_loss_limit_usd = it_int->second;

        it_int = int_values_.find("risk_limits.drawdown_pause_pct");
        if (it_int != int_values_.end()) risk_limits_.drawdown_pause_pct = it_int->second;

        // Learning schedule
        it_int = int_values_.find("learning_schedule.start_hour");
        if (it_int != int_values_.end()) learning_schedule_.start_hour = it_int->second;

        it_int = int_values_.find("learning_schedule.end_hour");
        if (it_int != int_values_.end()) learning_schedule_.end_hour = it_int->second;

        // Cgroup limits
        it_int = int_values_.find("cgroup_limits.memory_mb");
        if (it_int != int_values_.end()) cgroup_limits_.memory_mb = it_int->second;

        it_int = int_values_.find("cgroup_limits.cpu_percent");
        if (it_int != int_values_.end()) cgroup_limits_.cpu_percent = it_int->second;

        // Sync interval
        it_int = int_values_.find("sync_interval_sec");
        if (it_int != int_values_.end()) int_values_["sync_interval_sec"] = it_int->second;

        // Devices array
        int idx = 0;
        while (true) {
            std::string prefix = "devices[" + std::to_string(idx) + "]";
            auto hostname_it = string_values_.find(prefix + ".hostname");
            if (hostname_it == string_values_.end()) break;

            MKConfigDevice dev;
            dev.hostname = hostname_it->second;

            auto ip_it = string_values_.find(prefix + ".ip");
            if (ip_it != string_values_.end()) dev.ip = ip_it->second;

            auto key_it = string_values_.find(prefix + ".ssh_key");
            if (key_it != string_values_.end()) dev.ssh_key = key_it->second;

            devices_.push_back(dev);
            idx++;
        }
    }

    // Set sensible defaults
    void setDefaults() {
        string_values_["telegram_token"] = "";
        string_values_["exchange_api_key"] = "";
        string_values_["exchange_api_secret"] = "";
        string_values_["trading_mode"] = "paper";

        int_values_["sync_interval_sec"] = 300;

        risk_limits_.max_position_pct = 20;
        risk_limits_.daily_loss_limit_usd = 100;
        risk_limits_.drawdown_pause_pct = 10;

        learning_schedule_.start_hour = 2;
        learning_schedule_.end_hour = 6;

        cgroup_limits_.memory_mb = 4096;
        cgroup_limits_.cpu_percent = 80;

        loaded_ = false;
    }

public:
    MKConfig() : loaded_(false) {
        // Default config path
        const char* home = std::getenv("HOME");
        if (home) {
            config_path_ = std::string(home) + "/.mk_os/config.json";
        } else {
            config_path_ = "config.json";
        }
        setDefaults();
    }

    explicit MKConfig(const std::string& path) : config_path_(path), loaded_(false) {
        setDefaults();
    }

    // Load configuration from file
    bool load() {
        errors_.clear();

        std::ifstream file(config_path_);
        if (!file.is_open()) {
            // File missing is OK, defaults are used
            std::cout << "[CONFIG] Config file not found at " << config_path_
                      << ". Using defaults.\n";
            return true;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        if (content.empty()) {
            errors_.push_back("Config file is empty");
            return false;
        }

        parseJson(content);
        applyParsedValues();
        loaded_ = true;

        std::cout << "[CONFIG] Configuration loaded from " << config_path_ << "\n";
        return errors_.empty();
    }

    // Load from a specific JSON string (useful for testing)
    bool loadFromString(const std::string& json) {
        errors_.clear();
        setDefaults();

        if (json.empty()) {
            errors_.push_back("Empty JSON string");
            return false;
        }

        parseJson(json);
        applyParsedValues();
        loaded_ = true;

        return errors_.empty();
    }

    // Validate configuration
    std::vector<std::string> validate() const {
        std::vector<std::string> validation_errors;

        // Check risk limits are reasonable
        if (risk_limits_.max_position_pct <= 0 || risk_limits_.max_position_pct > 100) {
            validation_errors.push_back("max_position_pct must be 1-100");
        }
        if (risk_limits_.daily_loss_limit_usd < 0) {
            validation_errors.push_back("daily_loss_limit_usd must be >= 0");
        }
        if (risk_limits_.drawdown_pause_pct <= 0 || risk_limits_.drawdown_pause_pct > 100) {
            validation_errors.push_back("drawdown_pause_pct must be 1-100");
        }

        // Check learning schedule
        if (learning_schedule_.start_hour < 0 || learning_schedule_.start_hour > 23) {
            validation_errors.push_back("learning_schedule.start_hour must be 0-23");
        }
        if (learning_schedule_.end_hour < 0 || learning_schedule_.end_hour > 23) {
            validation_errors.push_back("learning_schedule.end_hour must be 0-23");
        }

        // Check cgroup limits
        if (cgroup_limits_.memory_mb <= 0) {
            validation_errors.push_back("cgroup_limits.memory_mb must be > 0");
        }
        if (cgroup_limits_.cpu_percent <= 0 || cgroup_limits_.cpu_percent > 100) {
            validation_errors.push_back("cgroup_limits.cpu_percent must be 1-100");
        }

        // Check trading mode
        std::string mode = getTradingMode();
        if (mode != "paper" && mode != "live" && mode != "analysis") {
            validation_errors.push_back("trading_mode must be 'paper', 'live', or 'analysis'");
        }

        return validation_errors;
    }

    // ========================================================
    // Typed Getters
    // ========================================================

    std::string getString(const std::string& key, const std::string& defaultValue = "") const {
        auto it = string_values_.find(key);
        if (it != string_values_.end()) return it->second;
        return defaultValue;
    }

    int getInt(const std::string& key, int defaultValue = 0) const {
        auto it = int_values_.find(key);
        if (it != int_values_.end()) return it->second;
        return defaultValue;
    }

    float getFloat(const std::string& key, float defaultValue = 0.0f) const {
        auto it = float_values_.find(key);
        if (it != float_values_.end()) return it->second;
        return defaultValue;
    }

    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto it = bool_values_.find(key);
        if (it != bool_values_.end()) return it->second;
        return defaultValue;
    }

    // ========================================================
    // Convenience Getters
    // ========================================================

    std::string getTelegramToken() const { return getString("telegram_token"); }
    std::string getExchangeApiKey() const { return getString("exchange_api_key"); }
    std::string getExchangeApiSecret() const { return getString("exchange_api_secret"); }
    std::string getTradingMode() const { return getString("trading_mode", "paper"); }
    int getSyncInterval() const { return getInt("sync_interval_sec", 300); }

    MKRiskLimits getRiskLimits() const { return risk_limits_; }
    MKLearningSchedule getLearningSchedule() const { return learning_schedule_; }
    MKCgroupLimits getCgroupLimits() const { return cgroup_limits_; }
    std::vector<MKConfigDevice> getDevices() const { return devices_; }

    // ========================================================
    // State Queries
    // ========================================================

    bool isLoaded() const { return loaded_; }
    std::string getConfigPath() const { return config_path_; }
    std::vector<std::string> getErrors() const { return errors_; }

    // Check if we're in learning hours
    bool isLearningHour() const {
        std::time_t now = std::time(nullptr);
        struct tm* timeinfo = std::localtime(&now);
        int hour = timeinfo->tm_hour;

        if (learning_schedule_.start_hour <= learning_schedule_.end_hour) {
            return hour >= learning_schedule_.start_hour && hour < learning_schedule_.end_hour;
        } else {
            // Wraps around midnight (e.g., 22:00 - 06:00)
            return hour >= learning_schedule_.start_hour || hour < learning_schedule_.end_hour;
        }
    }

    void printConfig() const {
        std::cout << "[CONFIG] Current configuration:\n";
        std::cout << "  Trading mode: " << getTradingMode() << "\n";
        std::cout << "  Sync interval: " << getSyncInterval() << "s\n";
        std::cout << "  Risk limits: max_position=" << risk_limits_.max_position_pct
                  << "%, daily_loss=$" << risk_limits_.daily_loss_limit_usd
                  << ", drawdown_pause=" << risk_limits_.drawdown_pause_pct << "%\n";
        std::cout << "  Learning schedule: " << learning_schedule_.start_hour
                  << ":00 - " << learning_schedule_.end_hour << ":00\n";
        std::cout << "  Cgroup limits: mem=" << cgroup_limits_.memory_mb
                  << "MB, cpu=" << cgroup_limits_.cpu_percent << "%\n";
        std::cout << "  Devices: " << devices_.size() << " configured\n";
    }
};

#endif // MK_CONFIG_CPP
