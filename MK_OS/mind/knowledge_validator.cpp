#ifndef MK_KNOWLEDGE_VALIDATOR_CPP
#define MK_KNOWLEDGE_VALIDATOR_CPP

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <set>
#include <cmath>

// ===================================================================================
// MK KNOWLEDGE VALIDATOR - Fact Quality Gate
// ===================================================================================
// Before any new fact enters the knowledge graph, it must pass validation:
// 1. Format check (source|relation|target|weight)
// 2. Weight range validation (0.0 to 1.0)
// 3. Duplicate detection against existing knowledge
// 4. Contradiction detection with high-confidence existing facts
// 5. Source reliability scoring
//
// Returns ACCEPT, REJECT, or REVIEW for each candidate fact.
// ===================================================================================

enum class ValidationResult {
    ACCEPT,     // Fact is valid and consistent
    REJECT,     // Fact is invalid or contradicts existing knowledge
    REVIEW      // Fact needs human review (uncertain)
};

struct ValidationReport {
    ValidationResult result;
    std::string fact_line;
    std::string reason;
    float confidence;
    bool is_duplicate;
    bool has_contradiction;
    bool format_valid;
    float source_reliability;
};

// Represents an existing fact for contradiction checking
struct ExistingFact {
    std::string source;
    std::string relation;
    std::string target;
    float weight;
};

class MKKnowledgeValidator {
private:
    // Known reliable sources get higher trust scores
    std::set<std::string> trusted_sources_;
    std::set<std::string> untrusted_sources_;
    
    // Existing facts for duplicate/contradiction checking
    std::vector<ExistingFact> existing_facts_;
    
    // Statistics
    int total_validated_;
    int total_accepted_;
    int total_rejected_;
    int total_review_;

    // Normalize a string (lowercase, trim whitespace)
    std::string normalize(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        size_t start = result.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = result.find_last_not_of(" \t\r\n");
        return result.substr(start, end - start + 1);
    }

    // Check if a fact line has valid format: source|relation|target|weight
    bool isValidFormat(const std::string& line, std::string& source,
                       std::string& relation, std::string& target, float& weight) {
        if (line.empty() || line[0] == '#') return false;

        std::istringstream ss(line);
        std::string weightStr;
        
        if (!std::getline(ss, source, '|')) return false;
        if (!std::getline(ss, relation, '|')) return false;
        if (!std::getline(ss, target, '|')) return false;
        
        source = normalize(source);
        relation = normalize(relation);
        target = normalize(target);
        
        if (source.empty() || relation.empty() || target.empty()) return false;

        // Weight is optional, default to 1.0
        weight = 1.0f;
        if (std::getline(ss, weightStr, '|')) {
            weightStr = normalize(weightStr);
            if (!weightStr.empty()) {
                try {
                    weight = std::stof(weightStr);
                } catch (...) {
                    return false; // Invalid weight format
                }
            }
        }

        return true;
    }

    // Validate weight is in range [0.0, 1.0]
    bool isValidWeight(float weight) {
        return weight >= 0.0f && weight <= 1.0f;
    }

    // Check for exact duplicates
    bool isDuplicate(const std::string& source, const std::string& relation,
                     const std::string& target) {
        for (const auto& fact : existing_facts_) {
            if (fact.source == source && fact.relation == relation &&
                fact.target == target) {
                return true;
            }
        }
        return false;
    }

    // Check for contradictions with high-confidence existing facts
    // A contradiction is when the same source+relation has a conflicting target
    // with weight >= 0.9
    bool hasContradiction(const std::string& source, const std::string& relation,
                          const std::string& target, std::string& conflicting_target) {
        // Relations that can have multiple targets are not contradictions
        static const std::set<std::string> multi_target_relations = {
            "related_to", "used_for", "has_property", "example",
            "type", "can", "part_of", "contains"
        };
        
        if (multi_target_relations.count(relation) > 0) {
            return false;
        }

        // Relations that should have unique targets (contradictions possible)
        static const std::set<std::string> unique_target_relations = {
            "is_a", "created_by", "invented_by", "discovered_by",
            "capital_of", "symbol", "stands_for", "founded_in",
            "born_in", "died_in"
        };

        if (unique_target_relations.count(relation) == 0) {
            return false;
        }

        for (const auto& fact : existing_facts_) {
            if (fact.source == source && fact.relation == relation &&
                fact.target != target && fact.weight >= 0.9f) {
                conflicting_target = fact.target;
                return true;
            }
        }
        return false;
    }

    // Score source reliability based on known patterns
    float scoreSourceReliability(const std::string& source_name) {
        if (trusted_sources_.count(source_name) > 0) return 1.0f;
        if (untrusted_sources_.count(source_name) > 0) return 0.2f;
        
        // Heuristic: longer, more specific sources tend to be more reliable
        float score = 0.5f;
        if (source_name.length() > 3) score += 0.1f;
        if (source_name.find('_') != std::string::npos) score += 0.1f;
        
        return std::min(1.0f, score);
    }

public:
    MKKnowledgeValidator()
        : total_validated_(0), total_accepted_(0),
          total_rejected_(0), total_review_(0) {
        // Initialize trusted sources
        trusted_sources_ = {
            "wikipedia", "arxiv", "official_docs", "textbook",
            "peer_reviewed", "established_fact"
        };
        untrusted_sources_ = {
            "unknown", "unverified", "social_media", "rumor"
        };
    }

    // Load existing facts for duplicate/contradiction checking
    void loadExistingFacts(const std::vector<ExistingFact>& facts) {
        existing_facts_ = facts;
    }

    // Add a single existing fact for checking
    void addExistingFact(const std::string& source, const std::string& relation,
                         const std::string& target, float weight) {
        ExistingFact fact;
        fact.source = normalize(source);
        fact.relation = normalize(relation);
        fact.target = normalize(target);
        fact.weight = weight;
        existing_facts_.push_back(fact);
    }

    // Validate a single fact line
    ValidationReport validate(const std::string& fact_line,
                              const std::string& data_source = "unknown") {
        ValidationReport report;
        report.fact_line = fact_line;
        report.is_duplicate = false;
        report.has_contradiction = false;
        report.format_valid = false;
        report.source_reliability = scoreSourceReliability(data_source);
        report.confidence = 0.0f;

        total_validated_++;

        // Step 1: Format validation
        std::string source, relation, target;
        float weight;
        if (!isValidFormat(fact_line, source, relation, target, weight)) {
            report.result = ValidationResult::REJECT;
            report.reason = "Invalid format. Expected: source|relation|target|weight";
            total_rejected_++;
            return report;
        }
        report.format_valid = true;

        // Step 2: Weight range validation
        if (!isValidWeight(weight)) {
            report.result = ValidationResult::REJECT;
            report.reason = "Weight out of range. Must be 0.0 to 1.0, got: " +
                            std::to_string(weight);
            total_rejected_++;
            return report;
        }

        // Step 3: Duplicate detection
        if (isDuplicate(source, relation, target)) {
            report.is_duplicate = true;
            report.result = ValidationResult::REJECT;
            report.reason = "Duplicate fact already exists in knowledge base";
            total_rejected_++;
            return report;
        }

        // Step 4: Contradiction detection
        std::string conflicting;
        if (hasContradiction(source, relation, target, conflicting)) {
            report.has_contradiction = true;
            report.result = ValidationResult::REJECT;
            report.reason = "Contradicts existing high-confidence fact: " +
                            source + "|" + relation + "|" + conflicting;
            total_rejected_++;
            return report;
        }

        // Step 5: Compute overall confidence
        float format_score = 1.0f;  // Already passed
        float weight_score = (weight >= 0.5f) ? 1.0f : 0.7f;
        float reliability = report.source_reliability;
        report.confidence = (format_score * 0.2f + weight_score * 0.3f +
                             reliability * 0.5f);

        // Decision based on confidence threshold
        if (report.confidence >= 0.6f) {
            report.result = ValidationResult::ACCEPT;
            report.reason = "Fact passed all validation checks";
            total_accepted_++;
        } else if (report.confidence >= 0.4f) {
            report.result = ValidationResult::REVIEW;
            report.reason = "Low confidence, needs human review";
            total_review_++;
        } else {
            report.result = ValidationResult::REJECT;
            report.reason = "Confidence too low: " + std::to_string(report.confidence);
            total_rejected_++;
        }

        return report;
    }

    // Validate a batch of fact lines
    std::vector<ValidationReport> validateBatch(const std::vector<std::string>& facts,
                                                const std::string& data_source = "unknown") {
        std::vector<ValidationReport> reports;
        for (const auto& fact : facts) {
            reports.push_back(validate(fact, data_source));
        }
        return reports;
    }

    // Get only accepted facts from a batch
    std::vector<std::string> filterAccepted(const std::vector<ValidationReport>& reports) {
        std::vector<std::string> accepted;
        for (const auto& report : reports) {
            if (report.result == ValidationResult::ACCEPT) {
                accepted.push_back(report.fact_line);
            }
        }
        return accepted;
    }

    // Get statistics
    std::string getStats() const {
        std::ostringstream ss;
        ss << "Validated: " << total_validated_
           << " | Accepted: " << total_accepted_
           << " | Rejected: " << total_rejected_
           << " | Review: " << total_review_;
        if (total_validated_ > 0) {
            ss << " | Accept Rate: "
               << (int)(100.0f * total_accepted_ / total_validated_) << "%";
        }
        return ss.str();
    }

    int totalValidated() const { return total_validated_; }
    int totalAccepted() const { return total_accepted_; }
    int totalRejected() const { return total_rejected_; }
    int totalReview() const { return total_review_; }
};

#endif // MK_KNOWLEDGE_VALIDATOR_CPP
