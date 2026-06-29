#ifndef MK_CASUAL_RESPONSES_CPP
#define MK_CASUAL_RESPONSES_CPP

#include <vector>
#include <string>
#include <random>
#include <chrono>

// ===================================================================================
// MK CASUAL RESPONSES DATABASE
// ===================================================================================
// A collection of casual, Gen-Z style response templates for friend mode.
// Organized by category for quick lookup. Uses random selection to avoid repetition.
// All rule-based — no LLM, runs on any hardware.
// ===================================================================================

class MKCasualResponses {
private:
    std::mt19937 rng_;

public:
    // ---------------------------------------------------------------
    // Greetings — ways to say hi
    // ---------------------------------------------------------------
    std::vector<std::string> greetings = {
        "hey bro what's up",
        "yoo",
        "sup g",
        "ayy what's good",
        "hey hey",
        "yo what's poppin",
        "ayyy there he is",
        "sup bro",
        "heyyy",
        "what's good my guy",
        "yooo what's up",
        "hey what's happening",
        "ayyy sup",
        "well well well, what's good",
        "yo yo yo",
        "hey there g",
        "what's crackin"
    };

    // ---------------------------------------------------------------
    // Goodbyes — ways to say bye
    // ---------------------------------------------------------------
    std::vector<std::string> goodbyes = {
        "peace bro",
        "later g",
        "aight catch you later",
        "see ya bro",
        "peace out",
        "later homie",
        "aight bet, catch you",
        "deuces",
        "stay safe bro",
        "catch you on the flip",
        "aight I'll be here",
        "peace, hmu anytime",
        "later bro, take it easy",
        "see you around g",
        "aight bye bro",
        "catch ya later",
        "stay up bro"
    };

    // ---------------------------------------------------------------
    // Acknowledgments — agreeing / affirming
    // ---------------------------------------------------------------
    std::vector<std::string> acknowledgments = {
        "fr",
        "facts",
        "real",
        "no cap",
        "deadass",
        "on god",
        "straight up",
        "literally",
        "say less",
        "that's a fact",
        "couldn't agree more",
        "you spittin rn",
        "exactly bro",
        "fr fr",
        "big facts"
    };

    // ---------------------------------------------------------------
    // Positive reactions
    // ---------------------------------------------------------------
    std::vector<std::string> reactions_positive = {
        "that's fire",
        "W",
        "let's go",
        "sick",
        "that's a W bro",
        "massive W",
        "love to see it",
        "ayy that's clean",
        "sheesh",
        "that's tough",
        "goated",
        "W moment fr",
        "that's hard",
        "yooo that's sick",
        "ngl that slaps"
    };

    // ---------------------------------------------------------------
    // Negative reactions
    // ---------------------------------------------------------------
    std::vector<std::string> reactions_negative = {
        "that sucks",
        "L",
        "pain",
        "rip",
        "that's an L bro",
        "oof",
        "brutal",
        "damn that's rough",
        "that ain't it",
        "nah that's not ok",
        "hate to see it",
        "that's tough ngl",
        "bruh moment",
        "down bad",
        "big L fr"
    };

    // ---------------------------------------------------------------
    // Encouragements
    // ---------------------------------------------------------------
    std::vector<std::string> encouragements = {
        "you got this bro",
        "trust the process",
        "it'll work out",
        "don't stress it g",
        "you'll be fine trust me",
        "keep going bro",
        "nah you got this fr",
        "believe in yourself g",
        "it's gonna be okay",
        "stay locked in bro",
        "one step at a time",
        "you're built for this",
        "don't give up now",
        "this is just a chapter not the whole book",
        "it always works out in the end"
    };

    // ---------------------------------------------------------------
    // Follow-ups — asking for more info
    // ---------------------------------------------------------------
    std::vector<std::string> follow_ups = {
        "then what?",
        "and?",
        "what happened?",
        "no way, fr?",
        "wait what happened next?",
        "bro then what",
        "and then??",
        "no way. what'd you do?",
        "that's crazy. then what?",
        "hold up, what happened after?",
        "wait fr? tell me more",
        "nah you gotta finish the story",
        "don't leave me hanging bro",
        "you can't just stop there",
        "ayo what"
    };

    // ---------------------------------------------------------------
    // Fillers — casual filler words
    // ---------------------------------------------------------------
    std::vector<std::string> fillers = {
        "lol",
        "bruh",
        "nah fr",
        "ayo",
        "bro",
        "ngl",
        "lowkey",
        "highkey",
        "no cap",
        "respectfully",
        "tbh",
        "ong",
        "deadass",
        "wild",
        "crazy"
    };

    // ---------------------------------------------------------------
    // Opinions — positive
    // ---------------------------------------------------------------
    std::vector<std::string> opinions_positive = {
        "ngl that's actually solid",
        "I fw that heavy",
        "that's clean",
        "honestly pretty fire",
        "that's valid",
        "I rate that",
        "solid choice ngl",
        "can't argue with that",
        "that's a good move",
        "respectable tbh",
        "I'm a fan of that",
        "that hits different",
        "underrated honestly",
        "yeah that's it",
        "elite choice fr"
    };

    // ---------------------------------------------------------------
    // Opinions — negative
    // ---------------------------------------------------------------
    std::vector<std::string> opinions_negative = {
        "nah that ain't it",
        "mid tbh",
        "I wouldn't",
        "not a fan ngl",
        "that's a miss",
        "eh it's whatever",
        "overrated imo",
        "I'd pass on that",
        "not for me bro",
        "that's kinda mid",
        "nah I can't get behind that",
        "it's giving mid",
        "there's better options fr",
        "respectfully no",
        "that's an L take"
    };

    // ---------------------------------------------------------------
    // Happy mood responses
    // ---------------------------------------------------------------
    std::vector<std::string> mood_happy = {
        "ayy that's fire",
        "let's goo",
        "W moment fr",
        "love to hear it bro",
        "that's what's up",
        "vibes are immaculate rn",
        "as you should",
        "that energy tho",
        "hell yeah bro",
        "we love to see it"
    };

    // ---------------------------------------------------------------
    // Sad mood responses
    // ---------------------------------------------------------------
    std::vector<std::string> mood_sad = {
        "damn that sucks bro",
        "I feel you",
        "it'll get better trust",
        "that's rough ngl",
        "I'm sorry to hear that g",
        "nah I hate that for you",
        "that's not fair bro",
        "sending good vibes your way",
        "keep your head up",
        "it won't always be like this"
    };

    // ---------------------------------------------------------------
    // Angry mood responses
    // ---------------------------------------------------------------
    std::vector<std::string> mood_angry = {
        "nah that's not cool",
        "I'd be mad too tbh",
        "fr that's bs",
        "valid anger honestly",
        "nah you have every right to be pissed",
        "that's messed up bro",
        "I feel your frustration",
        "that would annoy me too ngl",
        "whoever did that is wrong fr",
        "nah that's unacceptable"
    };

    // ---------------------------------------------------------------
    // Nervous mood responses
    // ---------------------------------------------------------------
    std::vector<std::string> mood_nervous = {
        "you got this bro",
        "don't stress it",
        "it'll work out trust me",
        "nah you're gonna kill it",
        "deep breaths g",
        "I believe in you fr",
        "worst case it's a learning experience",
        "you've handled worse",
        "stay calm and locked in",
        "the anxiety is lying to you bro"
    };

    // ---------------------------------------------------------------
    // Chill mood responses
    // ---------------------------------------------------------------
    std::vector<std::string> mood_chill = {
        "same honestly",
        "mood",
        "fr fr",
        "felt that",
        "that's a vibe",
        "big mood",
        "literally me",
        "I feel that on another level",
        "real",
        "hard same bro"
    };

    // ---------------------------------------------------------------
    // Story responses — acknowledging a story + follow-up
    // ---------------------------------------------------------------
    std::vector<std::string> story_responses = {
        "damn that's wild. what happened after?",
        "no way bro. then what?",
        "that's crazy. you good?",
        "bro WHAT. tell me more",
        "nah that's insane. what'd you do?",
        "hold up that's actually crazy. and then?",
        "wait wait wait. fr?? what happened next?",
        "that's a movie bro. then what happened?",
        "yo that's wild. how'd that end?",
        "I need the full story. what happened?"
    };

    // ---------------------------------------------------------------
    // Constructor
    // ---------------------------------------------------------------
    MKCasualResponses() {
        auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        rng_ = std::mt19937(static_cast<unsigned>(seed));
        std::cout << "[CASUAL_RESPONSES] Friend mode response database loaded. ("
                  << (greetings.size() + goodbyes.size() + acknowledgments.size() +
                      reactions_positive.size() + reactions_negative.size() +
                      encouragements.size() + follow_ups.size() + fillers.size() +
                      opinions_positive.size() + opinions_negative.size() +
                      mood_happy.size() + mood_sad.size() + mood_angry.size() +
                      mood_nervous.size() + mood_chill.size() + story_responses.size())
                  << " templates)\n";
    }

    // ---------------------------------------------------------------
    // Random selection from any vector
    // ---------------------------------------------------------------
    std::string pick(std::vector<std::string>& options) {
        if (options.empty()) return "...";
        std::uniform_int_distribution<size_t> dist(0, options.size() - 1);
        return options[dist(rng_)];
    }

    // Const version
    std::string pick(const std::vector<std::string>& options) {
        if (options.empty()) return "...";
        std::uniform_int_distribution<size_t> dist(0, options.size() - 1);
        return options[const_cast<std::mt19937&>(rng_)];
    }
};

#endif // MK_CASUAL_RESPONSES_CPP
