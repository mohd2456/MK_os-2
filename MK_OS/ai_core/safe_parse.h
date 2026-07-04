#ifndef MK_SAFE_PARSE_H
#define MK_SAFE_PARSE_H

// ===================================================================================
// MK SAFE PARSE
// ===================================================================================
// Numeric parsing helpers that NEVER throw. std::stof/stoi/stol/stod raise
// std::invalid_argument / std::out_of_range on malformed or empty input. When those
// exceptions escape a persistence/deserialization path they abort the whole process
// (SIGABRT) — MK would boot fine the first time (no state file) but crash on every
// launch afterward once a partial/corrupt state file exists on disk.
//
// State loaders must never crash on a corrupt/partial file: parse defensively, use a
// sane default for the bad token, and keep going (skip-and-continue).
// ===================================================================================

#include <string>
#include <cstdlib>

namespace mk {

// Parse a float; returns `fallback` on malformed/empty/out-of-range input.
inline float safeStof(const std::string& s, float fallback = 0.0f) {
    try {
        return std::stof(s);
    } catch (...) {
        return fallback;
    }
}

// Parse a double; returns `fallback` on malformed/empty/out-of-range input.
inline double safeStod(const std::string& s, double fallback = 0.0) {
    try {
        return std::stod(s);
    } catch (...) {
        return fallback;
    }
}

// Parse an int; returns `fallback` on malformed/empty/out-of-range input.
inline int safeStoi(const std::string& s, int fallback = 0) {
    try {
        return std::stoi(s);
    } catch (...) {
        return fallback;
    }
}

// Parse a long; returns `fallback` on malformed/empty/out-of-range input.
inline long safeStol(const std::string& s, long fallback = 0L) {
    try {
        return std::stol(s);
    } catch (...) {
        return fallback;
    }
}

// Parse a long long; returns `fallback` on malformed/empty/out-of-range input.
inline long long safeStoll(const std::string& s, long long fallback = 0LL) {
    try {
        return std::stoll(s);
    } catch (...) {
        return fallback;
    }
}

} // namespace mk

#endif // MK_SAFE_PARSE_H
