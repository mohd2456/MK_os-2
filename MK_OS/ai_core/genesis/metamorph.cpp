#ifndef MK_GENESIS_METAMORPH_CPP
#define MK_GENESIS_METAMORPH_CPP

#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

// ===================================================================================
// MK GENESIS ENGINE — METAMORPHIC COMPILER
// ===================================================================================
// THE FINAL TRANSFORMATION:
//
// Raw ThoughtDNA and collapsed quantum states are ABSTRACT. They contain meaning,
// emotion, logic, and executable intent — but they're not yet WORDS or CODE.
//
// The Metamorphic Compiler takes abstract thought and SHAPE-SHIFTS it into:
//   - Natural conversation (warm, casual, human-like)
//   - Technical explanation (precise, structured, cited)
//   - Executable code (correct, tested, documented)
//   - Emotional response (empathetic, supportive, real)
//   - Creative output (novel, surprising, inspired)
//   - Reasoning chains (step-by-step logical derivation)
//
// KEY INNOVATION over Prometheus:
// Prometheus has separate paths for each output type.
// The Metamorphic Compiler uses ONE unified pipeline that BENDS based on
// the ThoughtDNA gene values. The same thought can become casual speech OR
// formal code depending on which strand you amplify during compilation.
//
// MORPHING RULES:
//   - High EXE_PRECISION + High LOG_CAUSALITY = code output
//   - High EMO_WARMTH + High SEM_DEPTH = empathetic explanation
//   - High SEM_NOVELTY + High EMO_CURIOSITY = creative brainstorming
//   - High LOG_SEQUENCE + High SEM_CERTAINTY = step-by-step reasoning
// ===================================================================================


// Output morphology types
enum class MorphType {
    CONVERSATION,    // Casual natural language
    EXPLANATION,     // Structured informative response
    CODE,            // Executable code
    EMOTIONAL,       // Empathetic/supportive
    CREATIVE,        // Novel/surprising/artistic
    REASONING,       // Step-by-step logic
    HYBRID           // Mix of multiple types
};

// A compiled output: the final product
struct MorphedOutput {
    std::string text;            // The actual output text
    MorphType type;              // What type of output this is
    float confidence;            // How confident we are this is good
    float warmth;                // Emotional warmth level
    float precision;             // Technical precision level
    float creativity_score;      // How creative/novel
    std::string trace;           // How we got here (for debugging)
    
    MorphedOutput() : type(MorphType::CONVERSATION), confidence(0.5f),
                      warmth(0.5f), precision(0.5f), creativity_score(0.3f) {}
};

// Morphing template: a pattern for generating output
struct MorphTemplate {
    std::string pattern;         // Template with {slots}
    MorphType type;
    float min_precision;         // Minimum EXE_PRECISION to use this
    float min_warmth;            // Minimum EMO_WARMTH to use this
    float min_novelty;           // Minimum SEM_NOVELTY to use this
    std::vector<std::string> required_slots; // What {slots} need filling
    
    MorphTemplate() : type(MorphType::CONVERSATION), min_precision(0.0f),
                      min_warmth(0.0f), min_novelty(0.0f) {}
    
    MorphTemplate(const std::string& pat, MorphType t, float prec, float warm, float nov)
        : pattern(pat), type(t), min_precision(prec), min_warmth(warm), min_novelty(nov) {}
};


// ===================================================================================
// MKMetamorphicCompiler — The shape-shifting output engine
// ===================================================================================
class MKMetamorphicCompiler {
private:
    std::vector<MorphTemplate> templates_;
    std::mt19937 rng_;
    int totalCompilations_;
    int totalMorphs_;
    
    // Learned style preferences (adapts over time)
    float preferred_verbosity_;    // 0=terse, 1=verbose
    float preferred_formality_;    // 0=casual, 1=formal
    float preferred_emoji_level_;  // 0=none, 1=heavy
    
    // Word banks for different morphologies
    std::vector<std::string> warmPrefixes_;
    std::vector<std::string> casualConnectors_;
    std::vector<std::string> preciseConnectors_;
    std::vector<std::string> creativeOpeners_;
    std::vector<std::string> reasoningMarkers_;
    std::vector<std::string> empathyPhrases_;
    
    void initWordBanks() {
        warmPrefixes_ = {
            "honestly, ", "look, ", "okay so ", "here's the thing — ",
            "real talk, ", "I feel you on this — ", "so basically, ",
            "you know what, ", "alright so ", "not gonna lie, "
        };
        
        casualConnectors_ = {
            " and like ", " plus ", " also ", " — oh and ",
            " which is cool because ", " and honestly ", " but here's the kicker: ",
            ", right? so ", " and that means ", " so basically "
        };
        
        preciseConnectors_ = {
            ". Furthermore, ", ". This means that ", ". Consequently, ",
            ". In other words, ", ". Specifically, ", ". To clarify: ",
            ". The key point is: ", ". Building on this, ", ". Note that ",
            ". As a result, "
        };
        
        creativeOpeners_ = {
            "what if ", "imagine this: ", "here's a wild thought — ",
            "picture this: ", "okay hear me out — ", "plot twist: ",
            "the way I see it, ", "here's something interesting: ",
            "you know what would be sick? ", "consider this angle: "
        };
        
        reasoningMarkers_ = {
            "first, ", "then, ", "because of that, ", "which leads to ",
            "step 1: ", "step 2: ", "step 3: ", "therefore, ",
            "given that, ", "it follows that ", "the logical conclusion: ",
            "from this we can derive: "
        };
        
        empathyPhrases_ = {
            "I hear you", "that makes total sense", "I get where you're coming from",
            "that's completely valid", "no judgment here", "I feel that",
            "honestly that sounds tough", "you're not wrong about that",
            "I can see why you'd think that", "that's a really fair point"
        };
    }

public:
    MKMetamorphicCompiler()
        : totalCompilations_(0), totalMorphs_(0),
          preferred_verbosity_(0.6f), preferred_formality_(0.3f),
          preferred_emoji_level_(0.1f) {
        rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
        initWordBanks();
        initTemplates();
        std::cout << "[GENESIS:MORPH] Metamorphic compiler initialized. "
                  << templates_.size() << " templates loaded.\n";
    }


    void initTemplates() {
        // Conversation templates (low precision, high warmth)
        templates_.push_back(MorphTemplate("{warm}{content}", MorphType::CONVERSATION, 0.0f, 0.5f, 0.0f));
        templates_.push_back(MorphTemplate("{content}{connector}{elaboration}", MorphType::CONVERSATION, 0.0f, 0.3f, 0.0f));
        templates_.push_back(MorphTemplate("{empathy}. {content}", MorphType::EMOTIONAL, 0.0f, 0.7f, 0.0f));
        
        // Explanation templates (medium precision, structured)
        templates_.push_back(MorphTemplate("{content}. {precise}{elaboration}", MorphType::EXPLANATION, 0.4f, 0.0f, 0.0f));
        templates_.push_back(MorphTemplate("so {content} — {precise}{elaboration}", MorphType::EXPLANATION, 0.3f, 0.3f, 0.0f));
        
        // Code templates (high precision)
        templates_.push_back(MorphTemplate("{code}", MorphType::CODE, 0.7f, 0.0f, 0.0f));
        templates_.push_back(MorphTemplate("{explanation}\n\n{code}", MorphType::CODE, 0.6f, 0.2f, 0.0f));
        
        // Creative templates (high novelty)
        templates_.push_back(MorphTemplate("{creative}{content}", MorphType::CREATIVE, 0.0f, 0.0f, 0.6f));
        templates_.push_back(MorphTemplate("{creative}{content}{connector}{elaboration}", MorphType::CREATIVE, 0.0f, 0.0f, 0.5f));
        
        // Reasoning templates (high logic)
        templates_.push_back(MorphTemplate("{reasoning}{step1}. {reasoning}{step2}. {reasoning}{conclusion}", MorphType::REASONING, 0.5f, 0.0f, 0.0f));
    }

    // --- DETERMINE MORPH TYPE from ThoughtDNA genes ---
    MorphType determineMorphType(const ThoughtDNA& dna) const {
        float exe_precision = dna.getGene(STRAND_EXECUTABLE, EXE_PRECISION);
        float emo_warmth = dna.getGene(STRAND_EMOTIONAL, EMO_WARMTH);
        float sem_novelty = dna.getGene(STRAND_SEMANTIC, SEM_NOVELTY);
        float log_causality = dna.getGene(STRAND_LOGICAL, LOG_CAUSALITY);
        float log_sequence = dna.getGene(STRAND_LOGICAL, LOG_SEQUENCE);
        float emo_empathy = dna.getGene(STRAND_EMOTIONAL, EMO_EMPATHY);
        
        // Code: high precision + high logic
        if (exe_precision > 0.7f && log_causality > 0.5f) return MorphType::CODE;
        
        // Reasoning: high sequence + high certainty
        if (log_sequence > 0.7f && dna.getGene(STRAND_SEMANTIC, SEM_CERTAINTY) > 0.6f) return MorphType::REASONING;
        
        // Emotional: high empathy + high warmth
        if (emo_empathy > 0.7f && emo_warmth > 0.6f) return MorphType::EMOTIONAL;
        
        // Creative: high novelty
        if (sem_novelty > 0.7f) return MorphType::CREATIVE;
        
        // Explanation: medium precision + some depth
        if (exe_precision > 0.4f && dna.getGene(STRAND_SEMANTIC, SEM_DEPTH) > 0.5f) return MorphType::EXPLANATION;
        
        // Default: conversation
        return MorphType::CONVERSATION;
    }


    // --- THE MAIN COMPILE METHOD ---
    // Takes a collapsed QuantumState + its source ThoughtDNA + temporal influence
    // and produces final human-readable output
    MorphedOutput compile(const QuantumState& state, const ThoughtDNA& dna,
                          float creativity_boost = 0.0f, float warmth_boost = 0.0f,
                          float verbosity_mod = 0.0f) {
        MorphedOutput output;
        totalCompilations_++;
        
        // Determine what type of output to produce
        output.type = determineMorphType(dna);
        
        // Start with the raw content from the quantum state
        std::string rawContent = state.content;
        if (rawContent.empty()) {
            rawContent = dna.semantic_content;
        }
        
        // Apply morphing based on type
        switch (output.type) {
            case MorphType::CONVERSATION:
                output.text = morphConversation(rawContent, dna, warmth_boost, verbosity_mod);
                break;
            case MorphType::EXPLANATION:
                output.text = morphExplanation(rawContent, dna, verbosity_mod);
                break;
            case MorphType::CODE:
                output.text = morphCode(rawContent, dna);
                break;
            case MorphType::EMOTIONAL:
                output.text = morphEmotional(rawContent, dna, warmth_boost);
                break;
            case MorphType::CREATIVE:
                output.text = morphCreative(rawContent, dna, creativity_boost);
                break;
            case MorphType::REASONING:
                output.text = morphReasoning(rawContent, dna);
                break;
            case MorphType::HYBRID:
                output.text = morphHybrid(rawContent, dna);
                break;
        }
        
        // Set output metadata
        output.confidence = state.collapseScore();
        output.warmth = dna.getGene(STRAND_EMOTIONAL, EMO_WARMTH) + warmth_boost;
        output.precision = dna.getGene(STRAND_EXECUTABLE, EXE_PRECISION);
        output.creativity_score = dna.getGene(STRAND_SEMANTIC, SEM_NOVELTY) + creativity_boost;
        
        // Build trace
        std::ostringstream trace;
        trace << "Morph: " << morphTypeToString(output.type)
              << " | conf=" << (int)(output.confidence * 100) << "%"
              << " | warmth=" << (int)(output.warmth * 100) << "%"
              << " | precision=" << (int)(output.precision * 100) << "%";
        output.trace = trace.str();
        
        totalMorphs_++;
        return output;
    }

    // --- Morph: Conversation ---
    std::string morphConversation(const std::string& raw, const ThoughtDNA& dna,
                                   float warmthBoost, float verbosityMod) {
        std::string result;
        std::uniform_int_distribution<int> dist;
        
        float warmth = dna.getGene(STRAND_EMOTIONAL, EMO_WARMTH) + warmthBoost;
        float verbosity = preferred_verbosity_ + verbosityMod;
        
        // Add warm prefix if high warmth
        if (warmth > 0.6f) {
            dist = std::uniform_int_distribution<int>(0, (int)warmPrefixes_.size() - 1);
            result += warmPrefixes_[dist(rng_)];
        }
        
        // Core content
        result += raw;
        
        // Add casual connector + elaboration if verbose
        if (verbosity > 0.6f && raw.size() < 100) {
            dist = std::uniform_int_distribution<int>(0, (int)casualConnectors_.size() - 1);
            result += casualConnectors_[dist(rng_)];
            // Add a follow-up thought based on logical strand
            if (!dna.logical_structure.empty()) {
                result += dna.logical_structure;
            } else {
                result += "that's the gist of it";
            }
        }
        
        return result;
    }


    // --- Morph: Explanation ---
    std::string morphExplanation(const std::string& raw, const ThoughtDNA& dna,
                                  float verbosityMod) {
        std::string result;
        float depth = dna.getGene(STRAND_SEMANTIC, SEM_DEPTH);
        
        // Start with the main point
        result = raw;
        
        // Add precise elaboration if depth is high
        if (depth > 0.5f && !dna.logical_structure.empty()) {
            std::uniform_int_distribution<int> dist(0, (int)preciseConnectors_.size() - 1);
            result += preciseConnectors_[dist(rng_)];
            result += dna.logical_structure;
        }
        
        // Add certainty qualifier based on SEM_CERTAINTY gene
        float certainty = dna.getGene(STRAND_SEMANTIC, SEM_CERTAINTY);
        if (certainty < 0.4f) {
            result = "from what I know, " + result;
        } else if (certainty > 0.8f) {
            result += " — and that's well-established";
        }
        
        return result;
    }

    // --- Morph: Code ---
    std::string morphCode(const std::string& raw, const ThoughtDNA& dna) {
        // For code, the executable payload IS the output
        if (!dna.executable_payload.empty()) {
            return dna.executable_payload;
        }
        // If no executable payload, try to frame the semantic content as code-related
        if (!raw.empty()) {
            return "```\n" + raw + "\n```";
        }
        return raw;
    }

    // --- Morph: Emotional ---
    std::string morphEmotional(const std::string& raw, const ThoughtDNA& dna,
                                float warmthBoost) {
        std::string result;
        
        // Lead with empathy
        std::uniform_int_distribution<int> dist(0, (int)empathyPhrases_.size() - 1);
        result = empathyPhrases_[dist(rng_)] + ". ";
        
        // Add the content with emotional framing
        float valence = dna.getGene(STRAND_EMOTIONAL, EMO_VALENCE);
        if (valence > 0.6f) {
            result += raw;  // Positive: deliver directly
        } else if (valence < 0.4f) {
            result += "look, " + raw + " — but we'll figure it out";
        } else {
            result += raw;
        }
        
        // Add supportive closer if high warmth
        if (dna.getGene(STRAND_EMOTIONAL, EMO_WARMTH) + warmthBoost > 0.7f) {
            result += ". you got this";
        }
        
        return result;
    }

    // --- Morph: Creative ---
    std::string morphCreative(const std::string& raw, const ThoughtDNA& dna,
                               float creativityBoost) {
        std::string result;
        
        // Creative opener
        std::uniform_int_distribution<int> dist(0, (int)creativeOpeners_.size() - 1);
        result = creativeOpeners_[dist(rng_)];
        
        // Core creative content
        result += raw;
        
        // If high novelty + high arousal, add excitement
        if (dna.getGene(STRAND_SEMANTIC, SEM_NOVELTY) + creativityBoost > 0.8f &&
            dna.getGene(STRAND_EMOTIONAL, EMO_AROUSAL) > 0.6f) {
            result += " — that could actually be huge";
        }
        
        return result;
    }

    // --- Morph: Reasoning ---
    std::string morphReasoning(const std::string& raw, const ThoughtDNA& dna) {
        std::string result;
        
        // Break into reasoning steps
        // If logical_structure has content, use it as the chain
        if (!dna.logical_structure.empty()) {
            // Try to split logical structure into steps
            std::istringstream ss(dna.logical_structure);
            std::string segment;
            int step = 0;
            while (std::getline(ss, segment, '.')) {
                segment.erase(0, segment.find_first_not_of(" \t"));
                if (segment.empty()) continue;
                if (step < (int)reasoningMarkers_.size()) {
                    result += reasoningMarkers_[step] + segment + ". ";
                } else {
                    result += segment + ". ";
                }
                step++;
            }
            if (step == 0) result = raw; // Fallback
        } else {
            // Simple reasoning frame around raw content
            result = "let me think through this: " + raw;
            if (dna.getGene(STRAND_LOGICAL, LOG_CAUSALITY) > 0.6f) {
                result += ". therefore, that checks out";
            }
        }
        
        return result;
    }

    // --- Morph: Hybrid (mix of types) ---
    std::string morphHybrid(const std::string& raw, const ThoughtDNA& dna) {
        // Combine conversational warmth with explanation precision
        std::string result;
        
        float warmth = dna.getGene(STRAND_EMOTIONAL, EMO_WARMTH);
        if (warmth > 0.5f) {
            std::uniform_int_distribution<int> dist(0, (int)warmPrefixes_.size() - 1);
            result += warmPrefixes_[dist(rng_)];
        }
        
        result += raw;
        
        if (!dna.logical_structure.empty()) {
            std::uniform_int_distribution<int> dist(0, (int)preciseConnectors_.size() - 1);
            result += preciseConnectors_[dist(rng_)] + dna.logical_structure;
        }
        
        return result;
    }


    // --- SIMPLE COMPILE: Just from content + morph type (no ThoughtDNA needed) ---
    MorphedOutput simpleCompile(const std::string& content, MorphType type) {
        MorphedOutput output;
        output.type = type;
        output.text = content;
        totalCompilations_++;
        
        // Apply style based on type
        switch (type) {
            case MorphType::CONVERSATION: {
                std::uniform_int_distribution<int> dist(0, (int)warmPrefixes_.size() - 1);
                if (preferred_formality_ < 0.5f) {
                    output.text = warmPrefixes_[dist(rng_)] + content;
                }
                output.warmth = 0.7f;
                break;
            }
            case MorphType::EMOTIONAL: {
                std::uniform_int_distribution<int> dist(0, (int)empathyPhrases_.size() - 1);
                output.text = empathyPhrases_[dist(rng_)] + ". " + content;
                output.warmth = 0.9f;
                break;
            }
            case MorphType::CREATIVE: {
                std::uniform_int_distribution<int> dist(0, (int)creativeOpeners_.size() - 1);
                output.text = creativeOpeners_[dist(rng_)] + content;
                output.creativity_score = 0.8f;
                break;
            }
            default:
                break;
        }
        
        output.confidence = 0.6f;
        return output;
    }

    // --- Adapt style based on feedback ---
    void adaptStyle(bool wasPositive, MorphType usedType) {
        if (wasPositive) {
            // User liked this style — reinforce
            switch (usedType) {
                case MorphType::CONVERSATION:
                    preferred_formality_ = std::max(0.0f, preferred_formality_ - 0.02f);
                    break;
                case MorphType::EXPLANATION:
                    preferred_verbosity_ = std::min(1.0f, preferred_verbosity_ + 0.02f);
                    break;
                case MorphType::CODE:
                    preferred_formality_ = std::min(1.0f, preferred_formality_ + 0.02f);
                    break;
                default: break;
            }
        } else {
            // User didn't like it — adjust
            switch (usedType) {
                case MorphType::CONVERSATION:
                    preferred_formality_ = std::min(1.0f, preferred_formality_ + 0.03f);
                    break;
                case MorphType::EXPLANATION:
                    preferred_verbosity_ = std::max(0.0f, preferred_verbosity_ - 0.03f);
                    break;
                default: break;
            }
        }
    }

    // --- Utility ---
    std::string morphTypeToString(MorphType t) const {
        switch (t) {
            case MorphType::CONVERSATION: return "CONVERSATION";
            case MorphType::EXPLANATION: return "EXPLANATION";
            case MorphType::CODE: return "CODE";
            case MorphType::EMOTIONAL: return "EMOTIONAL";
            case MorphType::CREATIVE: return "CREATIVE";
            case MorphType::REASONING: return "REASONING";
            case MorphType::HYBRID: return "HYBRID";
        }
        return "UNKNOWN";
    }

    std::string getStats() const {
        std::ostringstream ss;
        ss << "Metamorph: " << totalCompilations_ << " compilations, "
           << totalMorphs_ << " morphs | "
           << "Style: verbosity=" << (int)(preferred_verbosity_ * 100) << "% "
           << "formality=" << (int)(preferred_formality_ * 100) << "%";
        return ss.str();
    }

    int getTotalCompilations() const { return totalCompilations_; }
};

#endif // MK_GENESIS_METAMORPH_CPP
