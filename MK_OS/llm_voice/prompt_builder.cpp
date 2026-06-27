#ifndef MK_PROMPT_BUILDER_CPP
#define MK_PROMPT_BUILDER_CPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iostream>

// ============================================================================
// MKPromptBuilder - Constructs prompts for the tiny local LLM
// Ensures model only rephrases and never adds new information
// ============================================================================

enum class PromptStyle {
    CASUAL,
    FORMAL,
    BRIEF,
    DETAILED
};

struct PromptContext {
    std::string user_name;
    std::string time_of_day;
    std::string conversation_topic;
    std::string previous_response;
    int turn_number;
};

struct BuiltPrompt {
    std::string system_prompt;
    std::string user_prompt;
    std::string full_prompt;
    int estimated_tokens;
    bool truncated;
};

class MKPromptBuilder {
private:
    int max_tokens_;
    std::map<PromptStyle, std::string> system_prompts_;
    std::map<PromptStyle, std::string> style_templates_;
    PromptContext current_context_;
    std::vector<std::string> safety_instructions_;

    void initializeSystemPrompts() {
        system_prompts_[PromptStyle::CASUAL] = 
            "You are a rephrasing assistant. Your ONLY job is to rephrase the given text "
            "in a casual, conversational tone. Do NOT add any new information. Do NOT answer "
            "questions. Do NOT provide opinions. ONLY rephrase what is given to you. "
            "Use simple words, contractions, and a relaxed tone.";

        system_prompts_[PromptStyle::FORMAL] = 
            "You are a rephrasing assistant. Your ONLY job is to rephrase the given text "
            "in a formal, professional tone. Do NOT add any new information. Do NOT answer "
            "questions. Do NOT provide opinions. ONLY rephrase what is given to you. "
            "Use proper grammar, avoid contractions, maintain professional language.";

        system_prompts_[PromptStyle::BRIEF] = 
            "You are a rephrasing assistant. Your ONLY job is to condense the given text "
            "into a shorter form while preserving meaning. Do NOT add any new information. "
            "Do NOT answer questions. Do NOT provide opinions. ONLY shorten and rephrase. "
            "Remove filler words, use concise language, aim for 50% reduction.";

        system_prompts_[PromptStyle::DETAILED] = 
            "You are a rephrasing assistant. Your ONLY job is to rephrase the given text "
            "with more detail and clarity. Do NOT add any NEW facts or information. "
            "Do NOT answer questions. ONLY expand the existing content with better "
            "explanations and structure. Add transitional phrases and clear organization.";
    }

    void initializeStyleTemplates() {
        style_templates_[PromptStyle::CASUAL] = 
            "Rephrase the following in a casual, friendly way:\n\n"
            "Original: {input}\n\n"
            "Casual version:";

        style_templates_[PromptStyle::FORMAL] = 
            "Rephrase the following formally and professionally:\n\n"
            "Original: {input}\n\n"
            "Formal version:";

        style_templates_[PromptStyle::BRIEF] = 
            "Condense the following to be brief and concise:\n\n"
            "Original: {input}\n\n"
            "Brief version:";

        style_templates_[PromptStyle::DETAILED] = 
            "Rephrase the following with more detail and clarity:\n\n"
            "Original: {input}\n\n"
            "Detailed version:";
    }

    void initializeSafetyInstructions() {
        safety_instructions_ = {
            "IMPORTANT: Never invent or add information not present in the original.",
            "IMPORTANT: Never answer questions - only rephrase them.",
            "IMPORTANT: Never express personal opinions or beliefs.",
            "IMPORTANT: If unsure, keep the original wording.",
            "IMPORTANT: Maintain the factual accuracy of the original."
        };
    }

    int estimateTokenCount(const std::string& text) const {
        // Rough estimation: ~4 characters per token for English
        return static_cast<int>(text.length()) / 4 + 1;
    }

    std::string getTimeOfDay() const {
        time_t now = time(nullptr);
        struct tm* local = localtime(&now);
        int hour = local->tm_hour;

        if (hour >= 5 && hour < 12) return "morning";
        if (hour >= 12 && hour < 17) return "afternoon";
        if (hour >= 17 && hour < 21) return "evening";
        return "night";
    }

    std::string replaceAll(const std::string& str, const std::string& from, 
                           const std::string& to) const {
        std::string result = str;
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.length(), to);
            pos += to.length();
        }
        return result;
    }

    std::string truncateText(const std::string& text, int max_chars) const {
        if (static_cast<int>(text.length()) <= max_chars) return text;
        return text.substr(0, max_chars) + "...";
    }

    std::string buildContextPrefix() const {
        std::string prefix;
        if (!current_context_.user_name.empty()) {
            prefix += "[User: " + current_context_.user_name + "] ";
        }
        if (!current_context_.time_of_day.empty()) {
            prefix += "[Time: " + current_context_.time_of_day + "] ";
        }
        if (!current_context_.conversation_topic.empty()) {
            prefix += "[Topic: " + current_context_.conversation_topic + "] ";
        }
        return prefix;
    }

public:
    MKPromptBuilder() : max_tokens_(512) {
        initializeSystemPrompts();
        initializeStyleTemplates();
        initializeSafetyInstructions();
        current_context_.turn_number = 0;
        current_context_.time_of_day = getTimeOfDay();
    }

    explicit MKPromptBuilder(int max_tokens) : max_tokens_(max_tokens) {
        initializeSystemPrompts();
        initializeStyleTemplates();
        initializeSafetyInstructions();
        current_context_.turn_number = 0;
        current_context_.time_of_day = getTimeOfDay();
    }

    BuiltPrompt buildPrompt(const std::string& input, PromptStyle style) {
        BuiltPrompt result;
        result.truncated = false;

        // Build system prompt
        result.system_prompt = system_prompts_[style];
        for (const auto& safety : safety_instructions_) {
            result.system_prompt += "\n" + safety;
        }

        // Build user prompt from template
        std::string user_template = style_templates_[style];
        result.user_prompt = replaceAll(user_template, "{input}", input);

        // Add context if available
        std::string context_prefix = buildContextPrefix();
        if (!context_prefix.empty()) {
            result.user_prompt = context_prefix + "\n" + result.user_prompt;
        }

        // Combine into full prompt
        result.full_prompt = "### System:\n" + result.system_prompt + "\n\n"
                           + "### User:\n" + result.user_prompt + "\n\n"
                           + "### Assistant:\n";

        // Check token budget
        result.estimated_tokens = estimateTokenCount(result.full_prompt);
        if (result.estimated_tokens > max_tokens_) {
            // Truncate input to fit within budget
            int available_chars = (max_tokens_ - estimateTokenCount(result.system_prompt) - 50) * 4;
            std::string truncated_input = truncateText(input, std::max(50, available_chars));
            result.user_prompt = replaceAll(style_templates_[style], "{input}", truncated_input);
            result.full_prompt = "### System:\n" + result.system_prompt + "\n\n"
                               + "### User:\n" + result.user_prompt + "\n\n"
                               + "### Assistant:\n";
            result.estimated_tokens = estimateTokenCount(result.full_prompt);
            result.truncated = true;
        }

        current_context_.turn_number++;
        return result;
    }

    void setContext(const PromptContext& context) {
        current_context_ = context;
        if (current_context_.time_of_day.empty()) {
            current_context_.time_of_day = getTimeOfDay();
        }
    }

    void setUserName(const std::string& name) {
        current_context_.user_name = name;
    }

    void setConversationTopic(const std::string& topic) {
        current_context_.conversation_topic = topic;
    }

    void setMaxTokens(int tokens) {
        max_tokens_ = std::max(64, std::min(4096, tokens));
    }

    int getMaxTokens() const { return max_tokens_; }

    std::string getSystemPrompt(PromptStyle style) const {
        auto it = system_prompts_.find(style);
        if (it != system_prompts_.end()) return it->second;
        return "";
    }

    std::string getStyleName(PromptStyle style) const {
        switch (style) {
            case PromptStyle::CASUAL: return "casual";
            case PromptStyle::FORMAL: return "formal";
            case PromptStyle::BRIEF: return "brief";
            case PromptStyle::DETAILED: return "detailed";
            default: return "unknown";
        }
    }

    PromptStyle parseStyle(const std::string& style_str) const {
        std::string lower = style_str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "casual") return PromptStyle::CASUAL;
        if (lower == "formal") return PromptStyle::FORMAL;
        if (lower == "brief") return PromptStyle::BRIEF;
        if (lower == "detailed") return PromptStyle::DETAILED;
        return PromptStyle::CASUAL;
    }

    void addCustomSystemInstruction(const std::string& instruction) {
        safety_instructions_.push_back("IMPORTANT: " + instruction);
    }

    PromptContext getContext() const { return current_context_; }

    void resetContext() {
        current_context_ = PromptContext();
        current_context_.turn_number = 0;
        current_context_.time_of_day = getTimeOfDay();
    }

    int estimatePromptTokens(const std::string& input, PromptStyle style) const {
        std::string system = system_prompts_.at(style);
        std::string user_tmpl = style_templates_.at(style);
        std::string full = system + replaceAll(user_tmpl, "{input}", input);
        return estimateTokenCount(full);
    }

    bool willFitInBudget(const std::string& input, PromptStyle style) const {
        return estimatePromptTokens(input, style) <= max_tokens_;
    }
};

#endif // MK_PROMPT_BUILDER_CPP
