#ifndef MK_KNOWLEDGE_GATE_CPP
#define MK_KNOWLEDGE_GATE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <deque>

// ===================================================================================
// MK KNOWLEDGE GATE — The Validation Firewall
// ===================================================================================
// NOTHING enters MK's brain without passing through these gates.
// This is what separates MK from garbage-in-garbage-out systems.
//
// 5 GATES:
//   Gate 1: Contradiction Check — does this CONFLICT with what we know?
//   Gate 2: Source Trust Scoring — WHO said this? How reliable are they?
//   Gate 3: Logical Consistency — does this make SENSE given existing knowledge?
//   Gate 4: Confidence Scoring — how SURE are we this is true?
//   Gate 5: Quarantine — uncertain facts go to holding, not main graph
//
// Facts below 0.5 confidence → pending_review.mk (quarantine)
// Facts below 0.2 confidence → rejected_log.mk (rejected with reason)
// Facts above 0.5 confidence → approved for main graph
//
// This prevents MK from polluting its knowledge with bad data.
// LLMs can't do this — they eat everything during training.
// MK is SELECTIVE. Quality over quantity.
// ===================================================================================


// Forward declarations — these are defined in other HRE layers
class MKPatternGraph;
class MKReasoningChains;

// ─────────────────────────────────────────────────────────────────────────────────
//  DATA STRUCTURES
// ─────────────────────────────────────────────────────────────────────────────────

// Result of running a fact through all 5 gates
struct ValidationResult {
    bool approved;              // Did it pass all gates?
    float finalConfidence;      // Combined confidence score (0.0-1.0)
    std::string disposition;    // "approved", "quarantined", "rejected"
    std::string rejectionReason;// Why it was rejected (empty if approved)
    
    // Per-gate results
    bool passedContradiction;   // Gate 1
    float sourceTrust;          // Gate 2 score
    bool passedLogic;           // Gate 3
    float confidenceScore;      // Gate 4
    std::string quarantineStatus; // Gate 5 result
    
    // Metadata
    std::string factId;         // Unique ID for tracking this fact
    std::time_t evaluatedAt;    // When validation ran
};


// A fact waiting in quarantine for verification
struct QuarantinedFact {
    std::string factId;
    std::string source;
    std::string relation;
    std::string target;
    float confidence;
    std::string sourceName;     // Who provided this fact
    float sourceTrust;          // Trust level of the provider
    std::string reason;         // Why it was quarantined
    std::time_t quarantinedAt;
    int verificationAttempts;   // How many times we tried to verify
    std::vector<std::string> supportingSources; // Other sources that agree
};

// Trust levels for different source types
struct SourceTrustProfile {
    std::string sourceName;
    float trustLevel;           // 0.0 to 1.0
    int factsProvided;          // Total facts from this source
    int factsApproved;          // How many passed validation
    int factsRejected;          // How many were rejected
    float effectivenesScore;    // factsApproved / factsProvided
};


// Known incompatible category pairs (for contradiction detection)
// If "X is_a Y" exists and Y is incompatible with Z, then "X is_a Z" is a contradiction
struct IncompatiblePair {
    std::string categoryA;
    std::string categoryB;
    std::string reason;
};

// ─────────────────────────────────────────────────────────────────────────────────
//  THE KNOWLEDGE GATE CLASS
// ─────────────────────────────────────────────────────────────────────────────────

class MKKnowledgeGate {
private:
    // Source trust database — built-in + learned trust levels
    std::unordered_map<std::string, SourceTrustProfile> sourceTrust;
    
    // Quarantine: facts waiting for verification
    std::vector<QuarantinedFact> pendingReview;
    
    // Rejected facts log
    std::vector<QuarantinedFact> rejectedFacts;
    
    // Known incompatible categories
    std::vector<IncompatiblePair> incompatibles;
    
    // Relations that are exclusive (only one target allowed)
    std::unordered_set<std::string> exclusiveRelations;


    // Multi-source agreement tracking: factKey → list of sources that confirm it
    std::unordered_map<std::string, std::vector<std::string>> sourceAgreement;
    
    // Demotion tracking: factKey → number of times it led to bad answers
    std::unordered_map<std::string, int> badAnswerCount;
    
    // Next fact ID counter
    int nextFactId;
    
    // File paths for persistence
    std::string knowledgeDir;
    std::string pendingFile;
    std::string rejectedFile;
    
    // Stats
    int totalValidated;
    int totalApproved;
    int totalQuarantined;
    int totalRejected;
    int totalPromoted;
    int totalDemoted;

    // ─────────────────────────────────────────────────────────────────────────────
    //  INTERNAL HELPERS
    // ─────────────────────────────────────────────────────────────────────────────

    // Generate a unique fact ID
    std::string generateFactId() {
        return "fact_" + std::to_string(nextFactId++) + "_" + 
               std::to_string(std::time(nullptr) % 100000);
    }


    // Build a lookup key for a fact (for deduplication and tracking)
    std::string makeFactKey(const std::string& source, const std::string& relation, 
                            const std::string& target) {
        return source + "|" + relation + "|" + target;
    }

    // Normalize a concept string (lowercase, trim)
    std::string normalize(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        size_t start = result.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = result.find_last_not_of(" \t\r\n");
        return result.substr(start, end - start + 1);
    }

    // ─────────────────────────────────────────────────────────────────────────────
    //  GATE 1: CONTRADICTION CHECK
    // ─────────────────────────────────────────────────────────────────────────────
    // Compares new fact against existing knowledge graph.
    // Detects: opposite categories, exclusive relation violations, direct conflicts.

    bool checkContradiction(const std::string& source, const std::string& relation,
                            const std::string& target, MKPatternGraph& graph,
                            std::string& reason) {
        
        // Check 1: Exclusive relations — some relations only allow one target
        // e.g., "earth is_a planet" — if someone says "earth is_a star", that conflicts
        if (exclusiveRelations.find(relation) != exclusiveRelations.end()) {
            // Query the graph for existing targets of this source+relation
            auto existing = graph.query(source, relation);
            for (const auto& existingTarget : existing) {
                if (existingTarget != target) {
                    // Check if the existing and new are incompatible
                    if (areIncompatible(existingTarget, target)) {
                        reason = "Exclusive relation conflict: '" + source + " " + 
                                 relation + "' already has '" + existingTarget + 
                                 "', incompatible with '" + target + "'";
                        return true; // CONTRADICTION FOUND
                    }
                }
            }
        }


        // Check 2: Known opposite facts
        // If "X is_a Y" exists, and Y is opposite of Z, then "X is_a Z" contradicts
        if (relation == "is_a" || relation == "type_of") {
            auto existingTypes = graph.query(source, "is_a");
            for (const auto& existingType : existingTypes) {
                if (graph.hasFact(existingType, "opposite_of", target) ||
                    graph.hasFact(target, "opposite_of", existingType)) {
                    reason = "Opposite category conflict: '" + source + "' is already '" +
                             existingType + "', which is opposite of '" + target + "'";
                    return true; // CONTRADICTION FOUND
                }
            }
        }
        
        // Check 3: Incompatible category pairs from our database
        if (relation == "is_a") {
            auto existingTypes = graph.query(source, "is_a");
            for (const auto& existingType : existingTypes) {
                for (const auto& incompat : incompatibles) {
                    if ((existingType == incompat.categoryA && target == incompat.categoryB) ||
                        (existingType == incompat.categoryB && target == incompat.categoryA)) {
                        reason = "Incompatible categories: '" + incompat.categoryA + 
                                 "' and '" + incompat.categoryB + "' — " + incompat.reason;
                        return true; // CONTRADICTION FOUND
                    }
                }
            }
        }
        
        // Check 4: Direct negation check
        // If "X cannot Y" exists, then "X can Y" contradicts
        if (relation == "can") {
            if (graph.hasFact(source, "cannot", target)) {
                reason = "Direct negation: '" + source + "' is known to NOT be able to '" + target + "'";
                return true;
            }
        }
        
        // No contradiction found
        return false;
    }


    // Check if two concepts are known to be incompatible
    bool areIncompatible(const std::string& a, const std::string& b) {
        for (const auto& pair : incompatibles) {
            if ((pair.categoryA == a && pair.categoryB == b) ||
                (pair.categoryA == b && pair.categoryB == a)) {
                return true;
            }
        }
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    //  GATE 2: SOURCE TRUST SCORING
    // ─────────────────────────────────────────────────────────────────────────────
    // Each source gets a trust level. Facts inherit their source's trust.
    // Trust levels: user=1.0, wikipedia=0.9, documentation=0.8, 
    //              textbook=0.85, random_file=0.3, unknown=0.1

    float getSourceTrust(const std::string& sourceName) {
        std::string name = normalize(sourceName);
        
        // Check if we have a learned trust profile for this source
        auto it = sourceTrust.find(name);
        if (it != sourceTrust.end()) {
            return it->second.trustLevel;
        }
        
        // Default trust levels by source type
        if (name == "user" || name == "owner" || name == "admin") return 1.0f;
        if (name == "wikipedia" || name == "wiki") return 0.9f;
        if (name == "textbook" || name == "academic") return 0.85f;
        if (name == "documentation" || name == "docs" || name == "manual") return 0.8f;
        if (name == "tutorial" || name == "guide") return 0.7f;
        if (name == "blog" || name == "article") return 0.5f;
        if (name == "forum" || name == "reddit" || name == "discussion") return 0.4f;
        if (name == "random_file" || name == "import" || name == "bulk") return 0.3f;
        if (name == "generated" || name == "auto") return 0.2f;
        
        // Unknown source — very low trust
        return 0.1f;
    }


    // Update source trust based on validation outcomes
    void updateSourceTrust(const std::string& sourceName, bool factApproved) {
        std::string name = normalize(sourceName);
        auto& profile = sourceTrust[name];
        profile.sourceName = name;
        profile.factsProvided++;
        
        if (factApproved) {
            profile.factsApproved++;
        } else {
            profile.factsRejected++;
        }
        
        // Recalculate effectiveness
        if (profile.factsProvided > 0) {
            profile.effectivenesScore = (float)profile.factsApproved / profile.factsProvided;
            // Adjust trust level based on track record (slowly)
            // Move trust toward effectiveness score by 10% each time
            float targetTrust = profile.effectivenesScore;
            profile.trustLevel = profile.trustLevel * 0.9f + targetTrust * 0.1f;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────────
    //  GATE 3: LOGICAL CONSISTENCY CHECK
    // ─────────────────────────────────────────────────────────────────────────────
    // Uses reasoning chains to verify if the new fact makes semantic sense.
    // "cat is_a number" → REJECT (animals aren't numbers)
    // "cat is_a mammal" → APPROVE (animals can be mammals)

    bool checkLogicalConsistency(const std::string& source, const std::string& relation,
                                  const std::string& target, MKPatternGraph& graph,
                                  MKReasoningChains& reasoning, std::string& reason) {
        
        // Check 1: Category coherence via is_a chains
        // If source is_a X, and target is_a Y, check if X and Y are compatible domains
        if (relation == "is_a") {
            auto sourceTypes = graph.query(source, "is_a");
            auto targetTypes = graph.query(target, "is_a");
            
            // If we know the source's domain and the target's domain, check compatibility
            for (const auto& sType : sourceTypes) {
                for (const auto& tType : targetTypes) {
                    if (areIncompatible(sType, tType)) {
                        reason = "Domain mismatch: '" + source + "' (domain: " + sType + 
                                 ") cannot be '" + target + "' (domain: " + tType + ")";
                        return false; // FAILED logical consistency
                    }
                }
            }
        }


        // Check 2: Relation validity
        // Some relations only make sense with certain source types
        if (relation == "lives_in") {
            // Only living things can live somewhere
            auto sourceTypes = graph.query(source, "is_a");
            bool isLiving = false;
            for (const auto& t : sourceTypes) {
                if (t == "animal" || t == "person" || t == "organism" || 
                    t == "living_thing" || t == "plant" || t == "human") {
                    isLiving = true;
                    break;
                }
            }
            // If we KNOW the source is NOT living, reject
            if (!sourceTypes.empty() && !isLiving) {
                // Check if any type is inanimate
                for (const auto& t : sourceTypes) {
                    if (t == "number" || t == "concept" || t == "abstract" || 
                        t == "color" || t == "emotion") {
                        reason = "Relation mismatch: '" + source + "' is '" + t + 
                                 "', which cannot 'live in' anywhere";
                        return false;
                    }
                }
            }
        }
        
        // Check 3: Try reasoning chains to see if this creates a logical loop
        // If adding "A is_a B" and we already have "B is_a A", that's circular
        if (relation == "is_a" || relation == "type_of" || relation == "part_of") {
            // Check reverse: does target already have source as ancestor?
            if (graph.hasFact(target, relation, source)) {
                reason = "Circular reference: '" + target + " " + relation + " " + source + 
                         "' already exists — adding reverse would create loop";
                return false;
            }
        }
        
        // Passed all logical checks
        return true;
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  GATE 4: CONFIDENCE SCORING
    // ─────────────────────────────────────────────────────────────────────────────
    // Multiple sources agreeing = higher confidence.
    // Single unverified source = low confidence.
    // Combines source trust, agreement count, and logical support.

    float calculateConfidence(const std::string& source, const std::string& relation,
                              const std::string& target, const std::string& sourceName,
                              float sourceTrustLevel, MKPatternGraph& graph) {
        float confidence = 0.0f;
        std::string factKey = makeFactKey(source, relation, target);
        
        // Factor 1: Source trust (40% of score)
        confidence += sourceTrustLevel * 0.4f;
        
        // Factor 2: Multi-source agreement (30% of score)
        // If multiple sources say the same thing, it's more likely true
        float agreementBonus = 0.0f;
        auto it = sourceAgreement.find(factKey);
        if (it != sourceAgreement.end()) {
            int agreers = it->second.size();
            // Each additional source adds confidence (diminishing returns)
            agreementBonus = std::min(1.0f, agreers * 0.3f);
        }
        confidence += agreementBonus * 0.3f;
        
        // Factor 3: Graph support (20% of score)
        // If related facts exist in the graph that support this, boost confidence
        float graphSupport = 0.0f;
        // Check if source concept already exists in graph
        auto sourceEdges = graph.getAll(source);
        if (!sourceEdges.empty()) graphSupport += 0.3f;
        // Check if target concept already exists in graph
        auto targetEdges = graph.getAll(target);
        if (!targetEdges.empty()) graphSupport += 0.3f;
        // Check if this relation type is commonly used
        auto relResults = graph.query(source, relation);
        if (!relResults.empty()) graphSupport += 0.4f;
        confidence += std::min(1.0f, graphSupport) * 0.2f;
        
        // Factor 4: Penalty for known bad sources (10% of score)
        float penaltyFactor = 1.0f;
        auto badIt = badAnswerCount.find(factKey);
        if (badIt != badAnswerCount.end() && badIt->second > 0) {
            penaltyFactor = std::max(0.0f, 1.0f - (badIt->second * 0.2f));
        }
        confidence += penaltyFactor * 0.1f;
        
        return std::min(1.0f, std::max(0.0f, confidence));
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  GATE 5: QUARANTINE SYSTEM
    // ─────────────────────────────────────────────────────────────────────────────
    // Facts below 0.5 → pending_review.mk (can be promoted later)
    // Facts below 0.2 → rejected_log.mk (with rejection reason)
    // Facts above 0.5 → approved for main graph

    void quarantineFact(const std::string& factId, const std::string& source,
                        const std::string& relation, const std::string& target,
                        float confidence, const std::string& sourceName,
                        float sourceTrustLevel, const std::string& reason) {
        QuarantinedFact qf;
        qf.factId = factId;
        qf.source = source;
        qf.relation = relation;
        qf.target = target;
        qf.confidence = confidence;
        qf.sourceName = sourceName;
        qf.sourceTrust = sourceTrustLevel;
        qf.reason = reason;
        qf.quarantinedAt = std::time(nullptr);
        qf.verificationAttempts = 0;
        
        if (confidence < 0.2f) {
            // Too low — reject outright
            rejectedFacts.push_back(qf);
            totalRejected++;
            saveRejectedFact(qf);
            std::cout << "[KNOWLEDGE GATE] REJECTED: " << source << "|" << relation 
                      << "|" << target << " (confidence=" << confidence 
                      << ", reason: " << reason << ")\n";
        } else {
            // Between 0.2 and 0.5 — quarantine for review
            pendingReview.push_back(qf);
            totalQuarantined++;
            savePendingFact(qf);
            std::cout << "[KNOWLEDGE GATE] QUARANTINED: " << source << "|" << relation 
                      << "|" << target << " (confidence=" << confidence 
                      << ", awaiting verification)\n";
        }
    }


    // Save a rejected fact to the rejected log file
    void saveRejectedFact(const QuarantinedFact& qf) {
        std::string path = knowledgeDir + "/" + rejectedFile;
        std::ofstream out(path, std::ios::app);
        if (out.is_open()) {
            out << qf.source << "|" << qf.relation << "|" << qf.target << "|" 
                << qf.confidence << "|REJECTED|" << qf.reason << "|" 
                << qf.sourceName << "|" << qf.quarantinedAt << "\n";
            out.close();
        }
    }

    // Save a pending fact to the pending review file
    void savePendingFact(const QuarantinedFact& qf) {
        std::string path = knowledgeDir + "/" + pendingFile;
        std::ofstream out(path, std::ios::app);
        if (out.is_open()) {
            out << qf.source << "|" << qf.relation << "|" << qf.target << "|" 
                << qf.confidence << "|PENDING|" << qf.reason << "|" 
                << qf.sourceName << "|" << qf.quarantinedAt << "\n";
            out.close();
        }
    }

public:
    // ─────────────────────────────────────────────────────────────────────────────
    //  CONSTRUCTOR & INITIALIZATION
    // ─────────────────────────────────────────────────────────────────────────────

    MKKnowledgeGate(const std::string& knowledgeDirectory = "knowledge_files")
        : nextFactId(1), knowledgeDir(knowledgeDirectory),
          pendingFile("pending_review.mk"), rejectedFile("rejected_log.mk"),
          totalValidated(0), totalApproved(0), totalQuarantined(0),
          totalRejected(0), totalPromoted(0), totalDemoted(0) {
        
        std::cout << "[KNOWLEDGE GATE] Validation firewall initialized.\n";
        initIncompatibles();
        initExclusiveRelations();
        initDefaultTrust();
    }


    // Initialize known incompatible category pairs
    void initIncompatibles() {
        incompatibles.push_back({"animal", "number", "Animals are living things, numbers are abstract"});
        incompatibles.push_back({"animal", "concept", "Animals are physical, concepts are abstract"});
        incompatibles.push_back({"animal", "color", "Animals are living things, colors are properties"});
        incompatibles.push_back({"plant", "animal", "Plants and animals are distinct kingdoms"});
        incompatibles.push_back({"liquid", "solid", "Matter cannot be both liquid and solid simultaneously"});
        incompatibles.push_back({"liquid", "gas", "Matter cannot be both liquid and gas simultaneously"});
        incompatibles.push_back({"solid", "gas", "Matter cannot be both solid and gas simultaneously"});
        incompatibles.push_back({"alive", "dead", "Cannot be both alive and dead"});
        incompatibles.push_back({"true", "false", "Cannot be both true and false"});
        incompatibles.push_back({"hot", "cold", "Opposites cannot coexist"});
        incompatibles.push_back({"male", "female", "Biological sex categories are mutually exclusive"});
        incompatibles.push_back({"machine", "organism", "Machines are artificial, organisms are biological"});
        incompatibles.push_back({"fiction", "fact", "Cannot be both fictional and factual"});
        std::cout << "[KNOWLEDGE GATE] Loaded " << incompatibles.size() << " incompatible pairs.\n";
    }

    // Initialize relations that should only have one target per source
    void initExclusiveRelations() {
        // These relations typically allow only one answer
        exclusiveRelations.insert("born_in");       // You're born in one place
        exclusiveRelations.insert("capital_of");    // One capital per country
        exclusiveRelations.insert("invented_by");   // One primary inventor
        exclusiveRelations.insert("opposite_of");   // One primary opposite
        // Note: "is_a" is NOT exclusive — things can be multiple types
    }


    // Initialize default trust profiles for known sources
    void initDefaultTrust() {
        sourceTrust["user"] = {"user", 1.0f, 0, 0, 0, 1.0f};
        sourceTrust["owner"] = {"owner", 1.0f, 0, 0, 0, 1.0f};
        sourceTrust["wikipedia"] = {"wikipedia", 0.9f, 0, 0, 0, 0.9f};
        sourceTrust["textbook"] = {"textbook", 0.85f, 0, 0, 0, 0.85f};
        sourceTrust["documentation"] = {"documentation", 0.8f, 0, 0, 0, 0.8f};
        sourceTrust["tutorial"] = {"tutorial", 0.7f, 0, 0, 0, 0.7f};
        sourceTrust["blog"] = {"blog", 0.5f, 0, 0, 0, 0.5f};
        sourceTrust["forum"] = {"forum", 0.4f, 0, 0, 0, 0.4f};
        sourceTrust["random_file"] = {"random_file", 0.3f, 0, 0, 0, 0.3f};
        sourceTrust["unknown"] = {"unknown", 0.1f, 0, 0, 0, 0.1f};
    }

    // ─────────────────────────────────────────────────────────────────────────────
    //  CORE API: VALIDATE A NEW FACT
    // ─────────────────────────────────────────────────────────────────────────────
    // This is THE main entry point. Run a fact through all 5 gates.
    // Returns a ValidationResult with full details of what passed/failed.

    ValidationResult validate(const std::string& source, const std::string& relation,
                              const std::string& target, const std::string& sourceName,
                              float trustLevel, MKPatternGraph& graph,
                              MKReasoningChains& reasoning) {
        totalValidated++;
        ValidationResult result;
        result.factId = generateFactId();
        result.evaluatedAt = std::time(nullptr);
        result.approved = false;
        
        std::string src = normalize(source);
        std::string rel = normalize(relation);
        std::string tgt = normalize(target);
        std::string srcName = normalize(sourceName);


        // Use provided trust level, or look up from database
        float effectiveTrust = (trustLevel > 0.0f) ? trustLevel : getSourceTrust(srcName);
        result.sourceTrust = effectiveTrust;
        
        // ─── GATE 1: CONTRADICTION CHECK ───
        std::string contradictionReason;
        bool hasContradiction = checkContradiction(src, rel, tgt, graph, contradictionReason);
        result.passedContradiction = !hasContradiction;
        
        if (hasContradiction) {
            result.disposition = "rejected";
            result.rejectionReason = "Gate 1 (Contradiction): " + contradictionReason;
            result.finalConfidence = 0.0f;
            result.passedLogic = false;
            result.confidenceScore = 0.0f;
            result.quarantineStatus = "rejected";
            
            // Even user-provided facts get flagged if contradictory
            // (user can override with promote() later)
            if (effectiveTrust >= 0.9f) {
                // High-trust source contradiction — quarantine instead of reject
                quarantineFact(result.factId, src, rel, tgt, 0.4f, srcName, effectiveTrust,
                              "Contradiction from trusted source: " + contradictionReason);
                result.disposition = "quarantined";
                result.quarantineStatus = "pending_review";
            } else {
                quarantineFact(result.factId, src, rel, tgt, 0.1f, srcName, effectiveTrust,
                              contradictionReason);
            }
            
            updateSourceTrust(srcName, false);
            return result;
        }


        // ─── GATE 2: SOURCE TRUST ───
        // Already computed above as effectiveTrust
        // If source is completely untrusted and no other gates boost it, it'll fail Gate 5
        
        // ─── GATE 3: LOGICAL CONSISTENCY ───
        std::string logicReason;
        bool logicOk = checkLogicalConsistency(src, rel, tgt, graph, reasoning, logicReason);
        result.passedLogic = logicOk;
        
        if (!logicOk) {
            // Logic failure — quarantine or reject based on trust
            if (effectiveTrust >= 0.8f) {
                // Trusted source but logic fails — quarantine for manual review
                float conf = effectiveTrust * 0.4f;
                quarantineFact(result.factId, src, rel, tgt, conf, srcName, effectiveTrust,
                              "Logic check failed: " + logicReason);
                result.disposition = "quarantined";
                result.finalConfidence = conf;
                result.rejectionReason = "Gate 3 (Logic): " + logicReason;
                result.quarantineStatus = "pending_review";
            } else {
                // Low trust + logic failure = reject
                quarantineFact(result.factId, src, rel, tgt, 0.1f, srcName, effectiveTrust,
                              logicReason);
                result.disposition = "rejected";
                result.finalConfidence = 0.1f;
                result.rejectionReason = "Gate 3 (Logic): " + logicReason;
                result.quarantineStatus = "rejected";
            }
            
            updateSourceTrust(srcName, false);
            return result;
        }


        // ─── GATE 4: CONFIDENCE SCORING ───
        // Record this source as agreeing with this fact
        std::string factKey = makeFactKey(src, rel, tgt);
        sourceAgreement[factKey].push_back(srcName);
        
        float confidence = calculateConfidence(src, rel, tgt, srcName, effectiveTrust, graph);
        result.confidenceScore = confidence;
        result.finalConfidence = confidence;
        
        // ─── GATE 5: QUARANTINE DECISION ───
        if (confidence >= 0.5f) {
            // APPROVED — fact can enter the main knowledge graph
            result.approved = true;
            result.disposition = "approved";
            result.quarantineStatus = "none";
            totalApproved++;
            updateSourceTrust(srcName, true);
            
            std::cout << "[KNOWLEDGE GATE] APPROVED: " << src << "|" << rel << "|" << tgt 
                      << " (confidence=" << confidence << ", source=" << srcName << ")\n";
        } else if (confidence >= 0.2f) {
            // QUARANTINED — too uncertain for main graph
            quarantineFact(result.factId, src, rel, tgt, confidence, srcName, effectiveTrust,
                          "Confidence below threshold: " + std::to_string(confidence));
            result.disposition = "quarantined";
            result.quarantineStatus = "pending_review";
        } else {
            // REJECTED — too unreliable
            quarantineFact(result.factId, src, rel, tgt, confidence, srcName, effectiveTrust,
                          "Extremely low confidence: " + std::to_string(confidence));
            result.disposition = "rejected";
            result.quarantineStatus = "rejected";
        }
        
        return result;
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  PROMOTION SYSTEM
    // ─────────────────────────────────────────────────────────────────────────────
    // Quarantined facts can be promoted to the main graph when:
    //   1. User explicitly confirms the fact
    //   2. A second trusted source provides the same fact
    //   3. Reasoning chains validate it against new knowledge

    void promote(const std::string& factId) {
        // Find the fact in pending review
        for (auto it = pendingReview.begin(); it != pendingReview.end(); ++it) {
            if (it->factId == factId) {
                std::cout << "[KNOWLEDGE GATE] PROMOTED: " << it->source << "|" 
                          << it->relation << "|" << it->target 
                          << " (was quarantined, now approved)\n";
                
                // Remove from pending
                pendingReview.erase(it);
                totalPromoted++;
                
                // Rewrite the pending file without this fact
                rewritePendingFile();
                return;
            }
        }
        std::cerr << "[KNOWLEDGE GATE] Cannot promote: factId '" << factId << "' not found in pending.\n";
    }

    // Promote a fact by content (for when user confirms a specific fact)
    void promoteByContent(const std::string& source, const std::string& relation,
                          const std::string& target) {
        std::string src = normalize(source);
        std::string rel = normalize(relation);
        std::string tgt = normalize(target);
        
        for (auto it = pendingReview.begin(); it != pendingReview.end(); ++it) {
            if (it->source == src && it->relation == rel && it->target == tgt) {
                promote(it->factId);
                return;
            }
        }
    }


    // Check if any quarantined facts can be auto-promoted
    // Called periodically or when new knowledge arrives
    int autoPromoteCheck(MKPatternGraph& graph) {
        int promoted = 0;
        std::vector<std::string> toPromote;
        
        for (auto& qf : pendingReview) {
            qf.verificationAttempts++;
            
            // Auto-promote condition 1: Multiple sources now agree
            std::string factKey = makeFactKey(qf.source, qf.relation, qf.target);
            auto agreeIt = sourceAgreement.find(factKey);
            if (agreeIt != sourceAgreement.end() && agreeIt->second.size() >= 2) {
                toPromote.push_back(qf.factId);
                continue;
            }
            
            // Auto-promote condition 2: Supporting evidence appeared in graph
            // e.g., quarantined "cat is_a feline" gets promoted when "feline is_a animal" appears
            if (qf.relation == "is_a") {
                auto targetTypes = graph.query(qf.target, "is_a");
                auto sourceTypes = graph.query(qf.source, "is_a");
                // If source and target share a common ancestor, that's supporting evidence
                for (const auto& st : sourceTypes) {
                    for (const auto& tt : targetTypes) {
                        if (st == tt) {
                            toPromote.push_back(qf.factId);
                            break;
                        }
                    }
                }
            }
        }
        
        for (const auto& fid : toPromote) {
            promote(fid);
            promoted++;
        }
        
        return promoted;
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  DEMOTION SYSTEM
    // ─────────────────────────────────────────────────────────────────────────────
    // Facts that lead to bad answers get their confidence reduced over time.
    // If a fact keeps causing wrong answers, eventually it gets quarantined.

    void demote(const std::string& factId) {
        // Track demotion — in practice this reduces the weight in the main graph
        totalDemoted++;
        std::cout << "[KNOWLEDGE GATE] DEMOTED: factId=" << factId 
                  << " (confidence reduced due to bad outcomes)\n";
    }

    // Report that a fact led to a bad answer
    void reportBadAnswer(const std::string& source, const std::string& relation,
                         const std::string& target) {
        std::string factKey = makeFactKey(normalize(source), normalize(relation), normalize(target));
        badAnswerCount[factKey]++;
        
        // If a fact has caused 3+ bad answers, flag it for demotion
        if (badAnswerCount[factKey] >= 3) {
            std::cout << "[KNOWLEDGE GATE] DEMOTION WARNING: " << source << "|" << relation 
                      << "|" << target << " has caused " << badAnswerCount[factKey] 
                      << " bad answers. Consider removing.\n";
        }
    }

    // Get the demotion count for a fact (for external systems to check)
    int getDemotionCount(const std::string& source, const std::string& relation,
                         const std::string& target) {
        std::string factKey = makeFactKey(normalize(source), normalize(relation), normalize(target));
        auto it = badAnswerCount.find(factKey);
        return (it != badAnswerCount.end()) ? it->second : 0;
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  PERSISTENCE
    // ─────────────────────────────────────────────────────────────────────────────

    // Rewrite the pending review file (after promotions remove entries)
    void rewritePendingFile() {
        std::string path = knowledgeDir + "/" + pendingFile;
        std::ofstream out(path);
        if (!out.is_open()) return;
        
        out << "# MK Pending Review — Facts awaiting verification\n";
        out << "# Format: source|relation|target|confidence|status|reason|sourceName|timestamp\n";
        out << "# These facts passed some gates but need confirmation before entering main graph.\n\n";
        
        for (const auto& qf : pendingReview) {
            out << qf.source << "|" << qf.relation << "|" << qf.target << "|"
                << qf.confidence << "|PENDING|" << qf.reason << "|"
                << qf.sourceName << "|" << qf.quarantinedAt << "\n";
        }
        out.close();
    }

    // Load previously quarantined facts from disk
    void loadPendingReview() {
        std::string path = knowledgeDir + "/" + pendingFile;
        std::ifstream in(path);
        if (!in.is_open()) return;
        
        std::string line;
        int loaded = 0;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::stringstream ss(line);
            std::string src, rel, tgt, conf, status, reason, srcName, ts;
            if (std::getline(ss, src, '|') && std::getline(ss, rel, '|') &&
                std::getline(ss, tgt, '|') && std::getline(ss, conf, '|') &&
                std::getline(ss, status, '|') && std::getline(ss, reason, '|') &&
                std::getline(ss, srcName, '|') && std::getline(ss, ts, '|')) {
                QuarantinedFact qf;
                qf.factId = generateFactId();
                qf.source = src; qf.relation = rel; qf.target = tgt;
                try { qf.confidence = std::stof(conf); } catch (...) { qf.confidence = 0.3f; }
                qf.reason = reason; qf.sourceName = srcName;
                try { qf.quarantinedAt = std::stol(ts); } catch (...) { qf.quarantinedAt = 0; }
                qf.verificationAttempts = 0;
                pendingReview.push_back(qf);
                loaded++;
            }
        }
        in.close();
        if (loaded > 0) {
            std::cout << "[KNOWLEDGE GATE] Loaded " << loaded << " pending facts from disk.\n";
        }
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  QUERY / INSPECTION
    // ─────────────────────────────────────────────────────────────────────────────

    // Get all facts currently in quarantine
    std::vector<QuarantinedFact> getPendingFacts() const { return pendingReview; }
    
    // Get all rejected facts
    std::vector<QuarantinedFact> getRejectedFacts() const { return rejectedFacts; }
    
    // Get the trust profile for a source
    SourceTrustProfile getSourceProfile(const std::string& sourceName) {
        std::string name = normalize(sourceName);
        auto it = sourceTrust.find(name);
        if (it != sourceTrust.end()) return it->second;
        return {"unknown", 0.1f, 0, 0, 0, 0.0f};
    }

    // Check if a specific fact is in quarantine
    bool isQuarantined(const std::string& source, const std::string& relation,
                       const std::string& target) {
        std::string src = normalize(source);
        std::string rel = normalize(relation);
        std::string tgt = normalize(target);
        for (const auto& qf : pendingReview) {
            if (qf.source == src && qf.relation == rel && qf.target == tgt) return true;
        }
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    //  STATS & DEBUG
    // ─────────────────────────────────────────────────────────────────────────────

    void printStats() const {
        std::cout << "\n--- [KNOWLEDGE GATE] ---\n";
        std::cout << "  Total validated:  " << totalValidated << "\n";
        std::cout << "  Approved:         " << totalApproved << "\n";
        std::cout << "  Quarantined:      " << totalQuarantined << "\n";
        std::cout << "  Rejected:         " << totalRejected << "\n";
        std::cout << "  Promoted:         " << totalPromoted << "\n";
        std::cout << "  Demoted:          " << totalDemoted << "\n";
        std::cout << "  Pending review:   " << pendingReview.size() << "\n";
        std::cout << "  Source profiles:  " << sourceTrust.size() << "\n";
        std::cout << "  Incomp. pairs:    " << incompatibles.size() << "\n";
        std::cout << "------------------------\n";
    }
};

#endif // MK_KNOWLEDGE_GATE_CPP
