#ifndef MK_MCE_ENGINE_CPP
#define MK_MCE_ENGINE_CPP

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

// ===================================================================================
// MK CONSCIOUSNESS ENGINE — MASTER CLASS
// ===================================================================================
// Combines all 5 layers into one simple API:
//   1. Thought Atoms (decompose input)
//   2. Chain Reactor (trigger related memories)
//   3. Thought Assembler (build response blueprint)
//   4. Word Weaver (generate sentence from word graph)
//   5. Echo Memory (adapt to user's speech patterns)
// ===================================================================================

#include "thought_atoms.cpp"
#include "chain_reactor.cpp"
#include "thought_assembler.cpp"
#include "word_weaver.cpp"
#include "echo_memory.cpp"

class MKConsciousnessEngine {
private:
    MKAtomDecomposer decomposer_;
    MKChainReactor reactor_;
    MKThoughtAssembler assembler_;
    MKWordWeaver weaver_;
    MKEchoMemory echo_;

    // Last generation trace for /trace command
    std::string lastTrace_;
    std::string lastInput_;
    std::string lastOutput_;
    std::vector<std::string> recentTopics_;
    bool initialized_;

    static const size_t MAX_RECENT_TOPICS = 5;

    std::string saveDir_;

public:
    MKConsciousnessEngine() : initialized_(false), saveDir_("mce_data") {
    }

    // Initialize with bootstrap data
    void initialize(MKPatternGraph& graph, MKCasualResponses& casual) {
        // Collect all casual response templates for word graph bootstrap
        std::vector<std::string> allResponses;
        auto addVec = [&](const std::vector<std::string>& v) {
            for (const auto& s : v) allResponses.push_back(s);
        };
        addVec(casual.greetings);
        addVec(casual.goodbyes);
        addVec(casual.acknowledgments);
        addVec(casual.reactions_positive);
        addVec(casual.reactions_negative);
        addVec(casual.encouragements);
        addVec(casual.follow_ups);
        addVec(casual.opinions_positive);
        addVec(casual.opinions_negative);
        addVec(casual.mood_happy);
        addVec(casual.mood_sad);
        addVec(casual.mood_angry);
        addVec(casual.mood_nervous);
        addVec(casual.mood_chill);
        addVec(casual.story_responses);

        // Collect knowledge facts for word graph
        std::vector<std::string> knowledgeFacts;
        auto allEdges = graph.getAllEdges();
        for (const auto& e : allEdges) {
            knowledgeFacts.push_back(e.source + " " + e.relation + " " + e.target);
            if (knowledgeFacts.size() > 500) break; // cap to avoid slow boot
        }

        // Bootstrap the word weaver
        weaver_.bootstrap(allResponses, knowledgeFacts);

        // Connect echo memory to word graph
        echo_.setWordGraph(&weaver_.getGraph());

        // Try to load saved state
        load();

        initialized_ = true;
        std::cout << "[MCE:ENGINE] MK Consciousness Engine initialized. "
                  << "Word graph: " << weaver_.getGraphSize() << " nodes | "
                  << "Memory: " << reactor_.getMemorySize() << " atoms | "
                  << "Echo: " << echo_.getTotalMessages() << " absorbed\n";
    }

    // THE main method: generate a response
    std::string generate(const std::string& input, MKPatternGraph& graph) {
        if (!initialized_) return "";

        std::ostringstream trace;
        lastInput_ = input;

        // 1. Decompose input into atoms
        auto atoms = decomposer_.decompose(input);
        trace << "Atoms: " << atoms.size() << " [";
        for (size_t i = 0; i < atoms.size() && i < 5; i++) {
            trace << atoms[i].toString();
            if (i + 1 < atoms.size() && i + 1 < 5) trace << " ";
        }
        trace << "]\n";

        // 2. Run chain reaction
        auto reacted = reactor_.react(atoms, graph);
        trace << "Reacted: " << reacted.size() << " triggered atoms\n";

        // 3. Assemble thought blueprint
        auto blueprint = assembler_.assemble(atoms, reacted, graph, recentTopics_);
        trace << "Blueprint: " << blueprint.toString() << "\n";

        // 4. Adjust blueprint with echo memory (user preferences)
        echo_.adjustBlueprint(blueprint);
        trace << "After echo adjust: tone=" << blueprint.targetTone
              << " len=" << blueprint.targetLength << "\n";

        // 5. Weave words into a sentence
        std::string response = weaver_.weave(blueprint);
        trace << "Output: " << response;

        // Store results
        lastOutput_ = response;
        lastTrace_ = trace.str();

        // Add atoms to reactor memory for future chain reactions
        reactor_.addAtoms(atoms);

        // Track topic
        if (!blueprint.subject.empty() && blueprint.subject != "general") {
            recentTopics_.push_back(blueprint.subject);
            if (recentTopics_.size() > MAX_RECENT_TOPICS) {
                recentTopics_.erase(recentTopics_.begin());
            }
        }

        // Feed the response back into word graph
        weaver_.feed(response, {"mk", "generated"});

        return response;
    }

    // Call after each exchange to grow echo memory
    void absorb(const std::string& input, const std::string& response) {
        if (!initialized_) return;
        // Assume user continued (positive) — caller can override
        echo_.absorb(input, response, true);
        // Also feed user input into word graph
        weaver_.feed(input, {"user", "conversation"});
    }

    // Shows the last thought trace
    std::string explain() const {
        if (lastTrace_.empty()) return "No generation trace yet.";
        return lastTrace_;
    }

    // Get echo profile
    std::string getEchoProfile() const {
        return echo_.getProfile();
    }

    // Save all persistent state
    void save() const {
        reactor_.save(saveDir_ + "/reactor_memory.dat");
        weaver_.save(saveDir_ + "/word_graph.dat");
        echo_.save(saveDir_ + "/echo_profile.dat");
    }

    // Load persistent state
    void load() {
        reactor_.load(saveDir_ + "/reactor_memory.dat");
        weaver_.load(saveDir_ + "/word_graph.dat");
        echo_.load(saveDir_ + "/echo_profile.dat");
    }

    // Stats summary
    std::string getStats() const {
        std::ostringstream ss;
        ss << "MCE Stats:\n"
           << "  Word graph: " << weaver_.getGraphSize() << " nodes\n"
           << "  Memory atoms: " << reactor_.getMemorySize() << "\n"
           << "  Echo messages: " << echo_.getTotalMessages() << "\n"
           << "  Echo vocab: " << echo_.getVocabSize() << " unique words\n"
           << "  Recent topics: ";
        for (size_t i = 0; i < recentTopics_.size(); i++) {
            ss << recentTopics_[i];
            if (i + 1 < recentTopics_.size()) ss << ", ";
        }
        return ss.str();
    }

    bool isInitialized() const { return initialized_; }
};

#endif // MK_MCE_ENGINE_CPP
