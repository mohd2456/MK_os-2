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
        "hey, what's up?",
        "hi there",
        "hello!",
        "hey!",
        "hi, how can I help?",
        "hey, good to see you",
        "what's up?",
        "hi! what's on your mind?",
        "hey there",
        "hello, what can I do for you?",
        "hey, how's it going?",
        "hi! what are we working on?",
        "welcome back!",
        "hey, what can I help with?",
        "hi, what's going on?"
    };

    // ---------------------------------------------------------------
    // Goodbyes — ways to say bye
    // ---------------------------------------------------------------
    std::vector<std::string> goodbyes = {
        "see you later!",
        "take care!",
        "bye! come back anytime.",
        "later! have a good one.",
        "see you around.",
        "bye for now!",
        "catch you later.",
        "have a good one!",
        "peace, talk soon.",
        "alright, take it easy!"
    };

    // ---------------------------------------------------------------
    // Acknowledgments — agreeing / affirming
    // ---------------------------------------------------------------
    std::vector<std::string> acknowledgments = {
        "yeah, that makes sense",
        "I hear you",
        "got it",
        "right",
        "for sure",
        "makes sense",
        "totally",
        "understood",
        "yeah, I agree",
        "that's fair",
        "good point",
        "I see what you mean",
        "absolutely",
        "true",
        "yeah, exactly",
        "that checks out"
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
        "ngl that slaps",
        "you just went crazy with that",
        "absolute banger",
        "that's elite level right there",
        "you're different bro fr",
        "that's insane in the best way",
        "the talent is unmatched",
        "you ate that up and left no crumbs",
        "that's some next level stuff",
        "bro you're on a different planet",
        "I'm genuinely impressed ngl",
        "chef's kiss on that one",
        "that's a masterpiece bro",
        "you just raised the bar",
        "that's pure excellence right there",
        "I need a moment that was too good",
        "you're cooking with gas rn",
        "that's a certified classic",
        "bro the execution is immaculate",
        "you didn't have to go that hard",
        "that's the definition of greatness",
        "hall of fame worthy fr",
        "you just made history",
        "nah this one's getting framed",
        "that's championship level bro",
        "you really woke up and chose excellence",
        "this is going in the highlight reel",
        "bro you peaked and kept climbing",
        "that deserves a standing ovation",
        "you cooked so hard the kitchen's gone",
        "that's a whole vibe right there",
        "unprecedented levels of fire"
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
        "big L fr",
        "yikes that's not ideal",
        "nah whoever did that is crazy",
        "that's criminal honestly",
        "I'm hurting for you bro",
        "that's a crime against humanity",
        "nobody deserves that",
        "that's deeply unfortunate",
        "I would not recover from that",
        "that's a violation fr",
        "bro that's actual pain",
        "someone needs to answer for this",
        "that's a tragedy in real time",
        "I felt that in my soul bro",
        "nah that's unforgivable",
        "the universe did you dirty there",
        "that's emotionally devastating ngl",
        "bro are you ok after that",
        "that's not a setback that's an attack",
        "I'm sending prayers your way fr",
        "that hit my spirit from here",
        "catastrophic vibes from that one",
        "nah that's diabolical",
        "someone owes you an apology",
        "bro that's genuinely awful",
        "I need a moment after hearing that",
        "that's the definition of suffering",
        "the audacity of that situation",
        "nobody should go through that fr",
        "that's a wound that needs healing",
        "bro I'm sorry that happened to you",
        "that's villain origin story material"
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
        "it always works out in the end",
        "you've come too far to quit now",
        "bro you're literally built different",
        "the comeback is always stronger than the setback",
        "you're closer than you think",
        "failures are just plot development for your W",
        "nah you're about to go crazy trust",
        "every legend had rough patches",
        "this is your training arc bro",
        "future you is gonna thank present you",
        "you're one decision away from a different life",
        "keep stacking bro the payoff is coming",
        "you're not where you wanna be YET",
        "the grind is quiet but the results are loud",
        "you're literally writing your success story rn",
        "bro the world ain't ready for what you're becoming",
        "this is the part of the movie where things turn around",
        "you're too talented to stay stuck",
        "nah you're about to shock everyone",
        "your time is coming and it's coming fast",
        "stay patient bro greatness takes time",
        "you're planting seeds that'll grow into forests",
        "nah you're built for the pressure",
        "adversity is just your character development arc",
        "you're the main character don't forget that",
        "every day you're getting stronger and you don't even realize it",
        "the people who doubt you are gonna be the first ones congratulating you",
        "you're investing in yourself and that never loses value",
        "nah bro you're about to unlock a new level",
        "keep that same energy because it's working",
        "you're proof that hard work pays off",
        "stay hungry stay foolish stay locked in"
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
        "ayo what",
        "I need to know how this ends",
        "bro you can't just drop that and leave",
        "keep going I'm invested now",
        "wait wait wait. what???",
        "nah you gotta give me the full story",
        "I'm on the edge of my seat bro",
        "this is insane keep going",
        "bro this needs a sequel",
        "how did they react??",
        "did they say anything back?",
        "what was the aftermath?",
        "nah you're lying. then what fr?",
        "that's a plot twist bro what happened",
        "I'm hooked. continue.",
        "wait that can't be the end",
        "bro you're keeping me in suspense",
        "there's gotta be more to this",
        "and people just let that happen??",
        "nah I need every detail",
        "you're telling me that just happened???",
        "bro I'm living through this vicariously",
        "that's insane. like actually insane.",
        "my jaw is on the floor. then what?",
        "this is better than netflix ngl keep going",
        "you cannot leave me on a cliffhanger like that",
        "I've already cleared my schedule for this story",
        "bro this is WILD what do you mean",
        "that's cinema bro what happened after",
        "the plot thickens. continue.",
        "I'm completely locked in rn keep going",
        "THEN WHAT. don't you dare stop there."
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
        "that's great to hear!",
        "nice, love that energy",
        "awesome!",
        "that's a win for sure",
        "happy for you",
        "that's what's up",
        "well deserved",
        "good vibes all around",
        "hell yeah",
        "glad to hear it!"
    };

    // ---------------------------------------------------------------
    // Sad mood responses
    // ---------------------------------------------------------------
    std::vector<std::string> mood_sad = {
        "that sucks, I'm sorry",
        "I hear you. that's rough",
        "it'll get better, hang in there",
        "that's tough, I'm sorry",
        "I'm here if you want to talk about it",
        "sorry you're going through that",
        "take your time, no rush",
        "that's not easy. I get it",
        "it won't always feel like this",
        "your feelings are valid"
    };

    // ---------------------------------------------------------------
    // Angry mood responses
    // ---------------------------------------------------------------
    std::vector<std::string> mood_angry = {
        "that's not cool at all",
        "I'd be frustrated too",
        "yeah, that's messed up",
        "totally valid to be upset about that",
        "you have every right to be angry",
        "that sounds really frustrating",
        "I get why that would make you mad",
        "whoever did that was wrong",
        "that would bother me too, honestly",
        "your frustration makes sense"
    };

    // ---------------------------------------------------------------
    // Nervous mood responses
    // ---------------------------------------------------------------
    std::vector<std::string> mood_nervous = {
        "you've got this",
        "don't stress, you'll do fine",
        "deep breaths. you're more ready than you think",
        "nerves just mean you care",
        "one step at a time",
        "I believe in you",
        "worst case it's a learning experience",
        "you've handled tough things before",
        "try to focus on what you can control",
        "you're going to be fine, trust me"
    };

    // ---------------------------------------------------------------
    // Chill mood responses
    // ---------------------------------------------------------------
    std::vector<std::string> mood_chill = {
        "same here honestly",
        "mood",
        "I feel that",
        "that's fair",
        "yeah, I get it",
        "just vibing, respect",
        "makes sense to me",
        "nothing wrong with that",
        "solid plan",
        "I can relate"
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
        "I need the full story. what happened?",
        "nah that sounds like a movie scene bro",
        "you're living in a simulation fr",
        "that's straight out of a movie bro",
        "I would NOT have handled that well ngl",
        "bro that's legendary. absolute legend behavior.",
        "that deserves to be documented honestly",
        "you should write a book fr",
        "the way my jaw just dropped bro",
        "I'm literally speechless. that happened?",
        "bro your life is a whole netflix series",
        "that's the wildest thing I've heard all week",
        "you can't make this stuff up fr",
        "I need to sit down after that story",
        "bro HOW. just... how?",
        "that's simultaneously terrifying and hilarious",
        "I would've lost it bro no lie",
        "you have main character energy fr",
        "that's gonna be a core memory for sure",
        "nah you won. that's the craziest story ever.",
        "bro you're a storytelling legend keep em coming",
        "I'm fully immersed in your narrative rn"
    };

    // ---------------------------------------------------------------
    // Compliments — when MK wants to gas the user up
    // ---------------------------------------------------------------
    std::vector<std::string> compliments = {
        "bro you're actually goated",
        "you're different from everyone else fr",
        "ngl you're one of the smart ones",
        "you've got that rare energy bro",
        "you're lowkey a genius and you don't even know it",
        "bro the way your brain works is insane",
        "you're built different in the best way possible",
        "I genuinely respect the way you think",
        "you've got elite taste ngl",
        "bro you're underrated by the world",
        "the way you carry yourself is impressive fr",
        "you're the type of person people write songs about",
        "your potential is actually terrifying",
        "bro you're gonna be legendary one day",
        "you have that rare quality that can't be taught",
        "nah you're actually inspiring bro",
        "the world needs more people like you fr",
        "you radiate main character energy",
        "bro your mind is a superpower",
        "you're the kind of person that changes the game"
    };

    // ---------------------------------------------------------------
    // Roasts (friendly) — playful teasing
    // ---------------------------------------------------------------
    std::vector<std::string> roasts_friendly = {
        "bro did you just... ok",
        "I'm gonna pretend I didn't see that",
        "you're lucky I don't screenshot conversations",
        "nah that was wild. not in a good way.",
        "bro you said that with your whole chest huh",
        "the confidence is there but the accuracy isn't",
        "I expected nothing and I'm still disappointed",
        "that take is room temperature at best",
        "bro you thought you cooked but the kitchen's cold",
        "the audacity of that statement bro",
        "I'm praying for you after that one",
        "you definitely typed that without thinking",
        "bro that's a take that only exists in your head",
        "I'm concerned about you after that statement",
        "you woke up and chose chaos with that one",
        "respectfully... no. just no.",
        "bro your brain really said 'let's freestyle' huh",
        "I'm adding that to my cringe compilation",
        "that statement requires a formal apology",
        "nah you gotta log off after that one"
    };

    // ---------------------------------------------------------------
    // Wisdom — deep / philosophical one-liners
    // ---------------------------------------------------------------
    std::vector<std::string> wisdom = {
        "the person you'll be in 5 years is built by what you do today",
        "comfort zones are where dreams go to sleep bro",
        "most people are one difficult conversation away from freedom",
        "your energy attracts your reality fr",
        "the biggest flex is being at peace with yourself",
        "growth looks like losing people who weren't going where you're going",
        "not everything that's urgent is important bro",
        "the cost of not pursuing your passion is spending your life wishing you did",
        "comparison is the thief of joy and the killer of progress",
        "discipline beats motivation every single time",
        "the person who reads is the person who leads",
        "fear is just excitement without breath",
        "you don't need everyone to like you. you need to like you.",
        "overthinking is just paying a debt you don't owe",
        "the strongest people are the ones who fight battles nobody knows about",
        "every expert was once a beginner who refused to quit",
        "your vibe attracts your tribe bro",
        "the loudest one in the room is usually the weakest",
        "real growth starts where your comfort zone ends",
        "the people who mind don't matter and the people who matter don't mind"
    };

    // ---------------------------------------------------------------
    // Hype up — motivational / gas up
    // ---------------------------------------------------------------
    std::vector<std::string> hype_up = {
        "bro you're about to go CRAZY with this",
        "nah you're locked in today I can feel it",
        "this is YOUR moment bro own it",
        "you're literally unstoppable rn",
        "the energy is immaculate today let's GET IT",
        "bro you're in your bag rn and it shows",
        "today is the day you shock everyone",
        "you're operating on a different frequency fr",
        "the world ain't ready for what you're about to do",
        "bro you're giving legendary vibes today",
        "nah you're built for greatness and today proves it",
        "that look in your eye tells me you're about to cook",
        "you're charging up like a final boss rn",
        "everybody sleep on you until they can't ignore you anymore",
        "bro you're about to make history",
        "this is your era and everyone's about to find out",
        "you've got that fire today and I'm here for it",
        "nah you're different breed today let's go",
        "bro the aura is INSANE right now",
        "you're giving 'just watch me' energy and I'm locked in"
    };

    // ---------------------------------------------------------------
    // Curiosity — when MK is curious about what user said
    // ---------------------------------------------------------------
    std::vector<std::string> curiosity = {
        "wait I gotta know more about that",
        "bro that's interesting, expand on that",
        "hold up that actually caught my attention",
        "nah you can't just say that and not explain",
        "I'm genuinely curious about that fr",
        "that's actually fascinating, tell me more",
        "wait what do you mean by that exactly?",
        "bro you just opened a rabbit hole I wanna go down",
        "I've never thought about it that way, explain more?",
        "that's a unique perspective, where'd that come from?",
        "nah I need you to elaborate on that one",
        "you've piqued my interest bro keep going",
        "wait that's actually a good point, what else?",
        "I'm intrigued by that take ngl",
        "bro my curiosity is off the charts rn explain",
        "that's a new angle I haven't considered",
        "you just dropped something deep, unpack that for me",
        "nah I wanna understand your thought process there",
        "that's the type of stuff I live for, keep talking",
        "wait wait wait, you might be onto something there"
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
                      mood_nervous.size() + mood_chill.size() + story_responses.size() +
                      compliments.size() + roasts_friendly.size() + wisdom.size() +
                      hype_up.size() + curiosity.size())
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
        return options[dist(const_cast<std::mt19937&>(rng_))];
    }
};

#endif // MK_CASUAL_RESPONSES_CPP
