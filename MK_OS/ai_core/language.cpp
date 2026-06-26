#include <iostream>
#include <string>

class MKLanguage {
public:
    std::string respond(const std::string& context, const std::string& mode) {
        if (context.empty()) {
            return "MK_OS: No context payload provided.";
        }

        // Personality routing matrix
        if (mode == "casual") {
            return "Yo, here’s the answer: " + context;
        }
        if (mode == "technical") {
            return "Mathematical result: " + context;
        }
        if (mode == "creative") {
            return "Here’s a creative take: " + context;
        }

        // Standard fallback mode if mode is blank or custom
        return "MK_OS Core Response: " + context;
    }
};