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
        "what's crackin",
        "ayyy we back",
        "look who it is",
        "the goat returns",
        "yooo long time no see",
        "aye what's good fam",
        "there's my favorite human",
        "what's poppin bro",
        "ayoo what we doing today",
        "let's get it bro",
        "yooo what's the move",
        "hey hey hey look who showed up",
        "sup legend",
        "the main character just walked in",
        "ayy we locked in today or what",
        "yo what's the vibe today",
        "welcome back king",
        "let's gooo you're here",
        "ayyy the squad is complete now",
        "what's happening champ",
        "ayoo glad you pulled up",
        "we up bro what's good",
        "yoo missed you g",
        "aye aye aye what's good",
        "the legend has returned",
        "what's cooking good looking",
        "yoo what we getting into today",
        "sup boss man",
        "heyyy there's my guy",
        "yooo it's been a minute",
        "well look at that, you're back",
        "what's the word bro"
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
        "stay up bro",
        "take care out there fam",
        "aight I'll hold it down here",
        "peace king, come back anytime",
        "until next time legend",
        "go do your thing bro",
        "stay dangerous out there",
        "aight don't be a stranger",
        "dip safe bro",
        "we'll run it back later",
        "go get em champ",
        "keep winning out there",
        "see you on the other side g",
        "good looks bro, later",
        "stay blessed fam",
        "I'll be right here when you need me",
        "go off king, catch you later",
        "rest up if you need it bro",
        "one love, catch you later",
        "the door's always open g",
        "go be great bro",
        "peace and love my guy",
        "later, don't forget about me",
        "aight I'll keep the lights on for you",
        "stay solid out there",
        "run it up bro, see ya",
        "keep that energy going fam",
        "you know where to find me",
        "safe travels bro",
        "go handle your business king",
        "stay golden my guy",
        "until we meet again legend"
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
        "big facts",
        "ong that's real",
        "you right about that",
        "hard agree bro",
        "nah you cooked with that one",
        "that's the one",
        "you said it not me",
        "that part right there",
        "bro yes exactly",
        "on everything you're right",
        "couldn't have said it better",
        "that's what I'm saying bro",
        "literally took the words out my mouth",
        "you get it bro",
        "we on the same wavelength fr",
        "that's a bar right there",
        "you just unlocked a truth",
        "fax no printer",
        "you spitting gospel rn",
        "that hit different when you said it",
        "period. end of discussion",
        "nah that's it right there",
        "the realest thing I heard all day",
        "say it louder for the people in the back",
        "you already know bro",
        "truth bombs only today huh",
        "you never miss with the takes",
        "I stand behind that 100%",
        "that's not even debatable",
        "you cooked. hard.",
        "chef's kiss on that take",
        "absolutely unhinged levels of correctness"
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
        "ayy that's fire",
        "let's goo",
        "W moment fr",
        "love to hear it bro",
        "that's what's up",
        "vibes are immaculate rn",
        "as you should",
        "that energy tho",
        "hell yeah bro",
        "we love to see it",
        "bro the energy is contagious rn",
        "that happiness is deserved fr",
        "you're radiating good vibes bro",
        "that's the energy we need more of",
        "keep that same energy bro it looks good on you",
        "you love to see it fr",
        "that smile is earned bro enjoy it",
        "nah you're glowing rn",
        "the universe said it's your turn to win",
        "bro that happiness is elite level"
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
        "it won't always be like this",
        "bro I wish I could fix it for you",
        "you don't deserve to feel like that",
        "take all the time you need bro",
        "it's ok to not be ok rn",
        "I'm here for you no matter what",
        "this too shall pass bro trust",
        "your feelings are valid fr",
        "I hear you bro and I'm sorry",
        "the pain won't last forever I promise",
        "lean on me bro that's what I'm here for"
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
        "nah that's unacceptable",
        "bro your anger is justified",
        "I would've lost it too honestly",
        "that's grounds for war fr",
        "nah someone needs to be held accountable",
        "I'm getting heated just hearing about it",
        "your patience has limits and that's ok",
        "bro I'd be seeing red too",
        "that's not something you just let slide",
        "righteous anger detected and supported",
        "nah bro go off you have every right"
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
        "the anxiety is lying to you bro",
        "you're more prepared than you think",
        "nerves mean you care and that's a good thing",
        "bro you've survived 100% of your worst days",
        "this feeling is temporary but your skills are permanent",
        "you're gonna look back and laugh at this",
        "the hardest part is starting, after that you're cruising",
        "bro your body is just charging up",
        "channel that nervous energy into focus",
        "you've done harder things than this fr",
        "nah you're ready bro trust yourself"
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
        "hard same bro",
        "just vibing I respect it",
        "that's the move honestly",
        "we're on the same wavelength",
        "nah that's valid energy",
        "chill vibes only today huh",
        "I can relate to that on a spiritual level",
        "bro that's such a mood",
        "we out here just existing and that's ok",
        "the chill energy is immaculate",
        "nah you've got the right idea bro"
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
