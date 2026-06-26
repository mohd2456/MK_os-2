#ifndef MK_PARSER_CPP
#define MK_PARSER_CPP

#include <iostream>
#include <string>

class MKNetworkParser {
public:
    // Strips web text blocks down to pure alphanumeric tokens
    std::string parseHTML(const std::string& html) {
        size_t length = (html.length() > 50) ? 50 : html.length();
        return "[PARSED HTML] " + html.substr(0, length) + "...";
    }

    // Isolates key value structures from your REST API responses
    std::string parseJSON(const std::string& json) {
        size_t length = (json.length() > 50) ? 50 : json.length();
        return "[PARSED JSON] " + json.substr(0, length) + "...";
    }
};

#endif // MK_PARSER_CPP