#ifndef MK_LLM_INTERFACE_CPP
#define MK_LLM_INTERFACE_CPP

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <chrono>
#include <thread>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <fstream>

// ============================================================================
// MKLLMInterface - Manages a local LLM for rephrasing user responses
// Supports token-by-token generation with callbacks and template fallback
// ============================================================================

enum class RephraseStyle {
    CASUAL,
    TECHNICAL,
    FRIENDLY,
    BRIEF
};

struct ModelState {
    std::string model_path;
    std::string model_name;
    bool loaded;
    size_t memory_usage_mb;
    int context_length;
    float temperature;
};

struct GenerationResult {
    std::string output;
    int tokens_generated;
    double generation_time_ms;
    bool from_fallback;
};

using TokenCallback = std::function<void(const std::string& token, int index)>;

class MKLLMInterface {
private:
    ModelState state_;
    bool model_available_;
    float temperature_;
    int max_tokens_;
    std::map<RephraseStyle, std::string> style_prompts_;
    std::map<RephraseStyle, std::vector<std::string>> fallback_templates_;
    TokenCallback token_callback_;

    void initializeStylePrompts() {
        style_prompts_[RephraseStyle::CASUAL] = "Rephrase this casually: ";
        style_prompts_[RephraseStyle::TECHNICAL] = "Rephrase this technically: ";
        style_prompts_[RephraseStyle::FRIENDLY] = "Rephrase this in a friendly way: ";
        style_prompts_[RephraseStyle::BRIEF] = "Rephrase this briefly: ";
    }

    void initializeFallbackTemplates() {
        fallback_templates_[RephraseStyle::CASUAL] = {
            "So basically, {content}",
            "Here's the deal - {content}",
            "Alright so {content}",
            "In simple terms, {content}",
            "Long story short: {content}"
        };
        fallback_templates_[RephraseStyle::TECHNICAL] = {
            "To elaborate precisely: {content}",
            "From a technical standpoint: {content}",
            "The specification indicates: {content}",
            "In technical terms: {content}",
            "The implementation detail: {content}"
        };
        fallback_templates_[RephraseStyle::FRIENDLY] = {
            "Hey! So {content}",
            "Great question! {content}",
            "Happy to help! {content}",
            "Sure thing! {content}",
            "Of course! {content}"
        };
        fallback_templates_[RephraseStyle::BRIEF] = {
            "{content}",
            "TL;DR: {content}",
            "Short answer: {content}",
            "Basically: {content}",
            "In brief: {content}"
        };
    }

    std::string buildPrompt(const std::string& raw_answer, RephraseStyle style) {
        std::string prompt;
        prompt += style_prompts_[style];
        prompt += "[" + raw_answer + "]\n";
        prompt += "Rephrased:";
        return prompt;
    }

    std::string applyFallbackTemplate(const std::string& raw_answer, RephraseStyle style) {
        auto& templates = fallback_templates_[style];
        if (templates.empty()) return raw_answer;

        size_t hash_val = 0;
        for (char c : raw_answer) {
            hash_val = hash_val * 31 + static_cast<size_t>(c);
        }
        size_t index = hash_val % templates.size();
        std::string result = templates[index];

        size_t pos = result.find("{content}");
        if (pos != std::string::npos) {
            result.replace(pos, 9, raw_answer);
        }
        return result;
    }

    std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::istringstream stream(text);
        std::string word;
        while (stream >> word) {
            tokens.push_back(word + " ");
        }
        return tokens;
    }

    void simulateTokenGeneration(const std::string& output, TokenCallback callback) {
        std::vector<std::string> tokens = tokenize(output);
        for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
            if (callback) {
                callback(tokens[i], i);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    }

    std::string simulateModelInference(const std::string& prompt) {
        // Simulates what a local LLM would return
        // In production this would call into GGML/llama.cpp
        size_t start_pos = prompt.find("[");
        size_t end_pos = prompt.find("]");
        if (start_pos != std::string::npos && end_pos != std::string::npos) {
            std::string content = prompt.substr(start_pos + 1, end_pos - start_pos - 1);
            // Simulate model processing - slightly rearrange words
            return content;
        }
        return "I understand your request.";
    }

    std::string truncateToMaxTokens(const std::string& text) {
        std::vector<std::string> tokens = tokenize(text);
        if (static_cast<int>(tokens.size()) <= max_tokens_) {
            return text;
        }
        std::string result;
        for (int i = 0; i < max_tokens_; ++i) {
            result += tokens[i];
        }
        return result;
    }

public:
    MKLLMInterface() 
        : model_available_(false), temperature_(0.7f), max_tokens_(256) {
        state_.loaded = false;
        state_.memory_usage_mb = 0;
        state_.context_length = 2048;
        state_.temperature = 0.7f;
        initializeStylePrompts();
        initializeFallbackTemplates();
    }

    ~MKLLMInterface() {
        if (state_.loaded) {
            unloadModel();
        }
    }

    bool loadModel(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "[MKLLMInterface] Failed to open model: " << path << std::endl;
            model_available_ = false;
            return false;
        }

        size_t file_size = static_cast<size_t>(file.tellg());
        file.close();

        state_.model_path = path;
        state_.model_name = path.substr(path.find_last_of("/\\") + 1);
        state_.memory_usage_mb = file_size / (1024 * 1024);
        state_.loaded = true;
        model_available_ = true;

        std::cout << "[MKLLMInterface] Loaded model: " << state_.model_name
                  << " (" << state_.memory_usage_mb << " MB)" << std::endl;
        return true;
    }

    GenerationResult rephrase(const std::string& raw_answer, RephraseStyle style) {
        GenerationResult result;
        auto start = std::chrono::high_resolution_clock::now();

        if (!model_available_ || !state_.loaded) {
            // Fallback to template-based rephrasing
            result.output = applyFallbackTemplate(raw_answer, style);
            result.from_fallback = true;
            result.tokens_generated = static_cast<int>(tokenize(result.output).size());
        } else {
            std::string prompt = buildPrompt(raw_answer, style);
            std::string generated = simulateModelInference(prompt);
            result.output = truncateToMaxTokens(generated);
            result.from_fallback = false;
            result.tokens_generated = static_cast<int>(tokenize(result.output).size());
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.generation_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

        // Fire token callback for streaming
        if (token_callback_) {
            simulateTokenGeneration(result.output, token_callback_);
        }

        return result;
    }

    void unloadModel() {
        if (state_.loaded) {
            std::cout << "[MKLLMInterface] Unloading model: " << state_.model_name << std::endl;
            state_.loaded = false;
            state_.model_path.clear();
            state_.model_name.clear();
            state_.memory_usage_mb = 0;
            model_available_ = false;
        }
    }

    void setTokenCallback(TokenCallback callback) {
        token_callback_ = callback;
    }

    void setTemperature(float temp) {
        temperature_ = std::max(0.0f, std::min(2.0f, temp));
        state_.temperature = temperature_;
    }

    void setMaxTokens(int tokens) {
        max_tokens_ = std::max(1, std::min(4096, tokens));
    }

    bool isModelLoaded() const { return state_.loaded; }
    std::string getModelName() const { return state_.model_name; }
    size_t getMemoryUsage() const { return state_.memory_usage_mb; }

    std::string getStyleName(RephraseStyle style) const {
        switch (style) {
            case RephraseStyle::CASUAL: return "casual";
            case RephraseStyle::TECHNICAL: return "technical";
            case RephraseStyle::FRIENDLY: return "friendly";
            case RephraseStyle::BRIEF: return "brief";
            default: return "unknown";
        }
    }

    RephraseStyle parseStyle(const std::string& style_str) const {
        std::string lower = style_str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "casual") return RephraseStyle::CASUAL;
        if (lower == "technical") return RephraseStyle::TECHNICAL;
        if (lower == "friendly") return RephraseStyle::FRIENDLY;
        if (lower == "brief") return RephraseStyle::BRIEF;
        return RephraseStyle::CASUAL; // default
    }

    ModelState getModelState() const { return state_; }
};

#endif // MK_LLM_INTERFACE_CPP
