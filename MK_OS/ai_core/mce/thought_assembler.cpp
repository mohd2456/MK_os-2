#ifndef MK_THOUGHT_ASSEMBLER_CPP
#define MK_THOUGHT_ASSEMBLER_CPP

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

// ===================================================================================
// MK CONSCIOUSNESS ENGINE — LAYER 3: THOUGHT ASSEMBLER
// ===================================================================================
// Takes triggered atoms and builds a "thought blueprint" — what MK intends to say.
// Determines intent, tone, subject, target length, and what to include.
// ===================================================================================

struct MKThoughtBlueprint {
    enum Intent { EMPATHIZE, INFORM, ENCOURAGE, ASK, JOKE, AGREE, DISAGREE, GREET, BYE, OPINION };

    Intent primaryIntent;
    std::vector<Intent> secondaryIntents;
    std::string subject;
    std::string targetTone;    // casual, serious, hype, chill
    int targetLength;          // word count target (5-30)
    std::vector<std::string> factsToInclude;
    std::vector<std::string> memoryReferences;
    float confidence;

    MKThoughtBlueprint() : primaryIntent(AGREE), targetTone("casual"), targetLength(10), confidence(0.5f) {}

    std::string intentToString(Intent i) const {
        static const char* names[] = {"EMPATHIZE","INFORM","ENCOURAGE","ASK","JOKE","AGREE","DISAGREE","GREET","BYE","OPINION"};
        return names[(int)i];
    }

    std::string toString() const {
        std::string result = "Blueprint{intent=" + intentToString(primaryIntent);
        if (!secondaryIntents.empty()) {
            result += "+";
            for (const auto& si : secondaryIntents) result += intentToString(si) + ",";
        }
        result += " subj=" + subject + " tone=" + targetTone;
        result += " len=" + std::to_string(targetLength);
        result += " conf=" + std::to_string(confidence).substr(0,4) + "}";
        return result;
    }
};

class MKThoughtAssembler {
private:
    std::string lastTrace_;

    // Check if atoms contain a specific type
    bool hasAtomType(const std::vector<MKThoughtAtom>& atoms, MKAtomType type) const {
        for (const auto& a : atoms) {
            if (a.type == type) return true;
        }
        return false;
    }

    // Get atom value by type
    std::string getAtomValue(const std::vector<MKThoughtAtom>& atoms, MKAtomType type) const {
        for (const auto& a : atoms) {
            if (a.type == type) return a.value;
        }
        return "";
    }

    // Get all atom values of a type
    std::vector<std::string> getAtomValues(const std::vector<MKThoughtAtom>& atoms, MKAtomType type) const {
        std::vector<std::string> vals;
        for (const auto& a : atoms) {
            if (a.type == type) vals.push_back(a.value);
        }
        return vals;
    }

    // Count words in a string
    int wordCount(const std::string& s) const {
        int c = 0; bool inW = false;
        for (char ch : s) {
            if (ch == ' ') inW = false;
            else if (!inW) { inW = true; c++; }
        }
        return c;
    }

    // Determine primary intent from input atoms
    MKThoughtBlueprint::Intent determineIntent(const std::vector<MKThoughtAtom>& inputAtoms) const {
        std::string emotion = getAtomValue(inputAtoms, MKAtomType::EMOTION);
        std::string action = getAtomValue(inputAtoms, MKAtomType::ACTION);

        // If user is asking a question
        if (action == "ask" || action == "request") return MKThoughtBlueprint::INFORM;

        // If strong emotion detected, empathize
        if (emotion == "sad" || emotion == "angry" || emotion == "nervous")
            return MKThoughtBlueprint::EMPATHIZE;

        // If user is happy, agree/celebrate
        if (emotion == "happy") return MKThoughtBlueprint::AGREE;

        // If user is venting
        if (action == "vent") return MKThoughtBlueprint::EMPATHIZE;

        // If user is telling something
        if (action == "tell") return MKThoughtBlueprint::ASK; // ask follow-up

        // Default: agree
        return MKThoughtBlueprint::AGREE;
    }

    // Determine secondary intents
    std::vector<MKThoughtBlueprint::Intent> determineSecondary(
        MKThoughtBlueprint::Intent primary,
        const std::vector<MKThoughtAtom>& inputAtoms) const {

        std::vector<MKThoughtBlueprint::Intent> secondary;
        std::string emotion = getAtomValue(inputAtoms, MKAtomType::EMOTION);

        if (primary == MKThoughtBlueprint::EMPATHIZE) {
            if (emotion == "sad" || emotion == "nervous")
                secondary.push_back(MKThoughtBlueprint::ENCOURAGE);
            secondary.push_back(MKThoughtBlueprint::ASK); // follow-up
        }
        if (primary == MKThoughtBlueprint::INFORM) {
            secondary.push_back(MKThoughtBlueprint::ASK); // offer more
        }
        if (primary == MKThoughtBlueprint::AGREE) {
            if (emotion == "happy")
                secondary.push_back(MKThoughtBlueprint::ENCOURAGE);
        }
        return secondary;
    }

public:
    MKThoughtAssembler() {
        std::cout << "[MCE:THOUGHT_ASSEMBLER] initialized\n";
    }

    MKThoughtBlueprint assemble(const std::vector<MKThoughtAtom>& inputAtoms,
                                 const std::vector<MKReactedAtom>& reactedAtoms,
                                 MKPatternGraph& graph,
                                 const std::vector<std::string>& recentTopics) {
        MKThoughtBlueprint bp;

        // 1. Determine intent
        bp.primaryIntent = determineIntent(inputAtoms);
        bp.secondaryIntents = determineSecondary(bp.primaryIntent, inputAtoms);

        // 2. Extract subject from entities
        auto entities = getAtomValues(inputAtoms, MKAtomType::ENTITY);
        if (!entities.empty()) {
            bp.subject = entities[0]; // primary entity
        } else if (!recentTopics.empty()) {
            bp.subject = recentTopics.back(); // fallback to recent topic
        } else {
            bp.subject = "general";
        }

        // 3. Match tone to user's detected tone
        std::string userTone = getAtomValue(inputAtoms, MKAtomType::TONE);
        if (!userTone.empty() && userTone != "neutral") {
            bp.targetTone = userTone;
        } else {
            bp.targetTone = "casual"; // default MK tone
        }

        // 4. Pull facts from graph for the subject
        auto graphEdges = graph.getAll(bp.subject);
        for (const auto& e : graphEdges) {
            bp.factsToInclude.push_back(e.target);
            if (bp.factsToInclude.size() >= 3) break; // max 3 facts
        }

        // 5. Pull memory references from reacted atoms
        for (const auto& ra : reactedAtoms) {
            if (ra.atom.type == MKAtomType::ENTITY && ra.hopDistance >= 2) {
                bp.memoryReferences.push_back(ra.atom.value);
                if (bp.memoryReferences.size() >= 3) break;
            }
        }

        // 6. Set target length (roughly match user's message style)
        int entityCount = (int)entities.size();
        if (entityCount > 3) bp.targetLength = 20;  // complex input = longer response
        else if (entityCount > 1) bp.targetLength = 12;
        else bp.targetLength = 8;  // short input = short response

        // Adjust for intent
        if (bp.primaryIntent == MKThoughtBlueprint::EMPATHIZE) bp.targetLength += 5;
        if (bp.primaryIntent == MKThoughtBlueprint::INFORM) bp.targetLength += 3;

        // Clamp
        if (bp.targetLength < 5) bp.targetLength = 5;
        if (bp.targetLength > 30) bp.targetLength = 30;

        // 7. Confidence
        bp.confidence = 0.5f;
        if (!bp.factsToInclude.empty()) bp.confidence += 0.2f;
        if (!bp.memoryReferences.empty()) bp.confidence += 0.1f;
        if (hasAtomType(inputAtoms, MKAtomType::EMOTION)) bp.confidence += 0.1f;

        // Save trace
        std::ostringstream trace;
        trace << "Input atoms: " << inputAtoms.size()
              << " | Reacted: " << reactedAtoms.size()
              << " | " << bp.toString();
        lastTrace_ = trace.str();

        return bp;
    }

    std::string toString() const { return lastTrace_; }
};

#endif // MK_THOUGHT_ASSEMBLER_CPP
