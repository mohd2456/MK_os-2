#ifndef MK_CRYPTO_CPP
#define MK_CRYPTO_CPP

#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <iostream>

// ============================================================
// MK Security - Crypto & Input Sanitization
// XOR-based encryption with key stretching for persistent data.
// Input sanitization strips dangerous shell characters.
// ============================================================

class MKCrypto {
private:
    std::string encryptionKey;

    // Stretch key to match data length using repeating XOR key schedule
    std::vector<uint8_t> stretchKey(size_t targetLen) const {
        std::vector<uint8_t> stretched(targetLen);
        if (encryptionKey.empty()) return stretched;

        // Simple key stretching: repeated key + position-dependent mixing
        for (size_t i = 0; i < targetLen; i++) {
            uint8_t keyByte = (uint8_t)encryptionKey[i % encryptionKey.size()];
            // Mix with position to avoid simple repeating patterns
            uint8_t mixed = keyByte ^ (uint8_t)(i * 7 + 13);
            // Additional mixing pass
            mixed = (mixed << 3) | (mixed >> 5);
            stretched[i] = mixed;
        }
        return stretched;
    }


public:
    MKCrypto() {
        // Read key from environment variable, fall back to default
        const char* envKey = std::getenv("MK_ENCRYPT_KEY");
        if (envKey && envKey[0] != '\0') {
            encryptionKey = std::string(envKey);
        } else {
            encryptionKey = "mk_default_key_2026";
        }
    }

    explicit MKCrypto(const std::string& key) : encryptionKey(key) {}

    // Encrypt data using XOR with stretched key
    std::string encrypt(const std::string& data, const std::string& key = "") const {
        std::string useKey = key.empty() ? encryptionKey : key;
        if (data.empty() || useKey.empty()) return data;

        // Temporarily set key for stretching
        MKCrypto temp(useKey);
        auto keyBytes = temp.stretchKey(data.size());

        std::string encrypted(data.size(), '\0');
        for (size_t i = 0; i < data.size(); i++) {
            encrypted[i] = (char)((uint8_t)data[i] ^ keyBytes[i]);
        }

        // Encode to hex for safe storage
        std::string hex;
        hex.reserve(encrypted.size() * 2);
        static const char hexChars[] = "0123456789abcdef";
        for (unsigned char c : encrypted) {
            hex += hexChars[c >> 4];
            hex += hexChars[c & 0x0F];
        }
        return hex;
    }

    // Decrypt data (reverse XOR)
    std::string decrypt(const std::string& hexData, const std::string& key = "") const {
        std::string useKey = key.empty() ? encryptionKey : key;
        if (hexData.empty() || useKey.empty()) return hexData;

        // Decode from hex
        if (hexData.size() % 2 != 0) return ""; // Invalid hex

        std::string encrypted;
        encrypted.reserve(hexData.size() / 2);
        for (size_t i = 0; i < hexData.size(); i += 2) {
            char high = hexData[i];
            char low = hexData[i + 1];
            uint8_t byte = 0;

            if (high >= '0' && high <= '9') byte = (uint8_t)(high - '0') << 4;
            else if (high >= 'a' && high <= 'f') byte = (uint8_t)(high - 'a' + 10) << 4;
            else return ""; // Invalid hex char

            if (low >= '0' && low <= '9') byte |= (uint8_t)(low - '0');
            else if (low >= 'a' && low <= 'f') byte |= (uint8_t)(low - 'a' + 10);
            else return ""; // Invalid hex char

            encrypted += (char)byte;
        }

        // Apply same XOR to decrypt
        MKCrypto temp(useKey);
        auto keyBytes = temp.stretchKey(encrypted.size());

        std::string decrypted(encrypted.size(), '\0');
        for (size_t i = 0; i < encrypted.size(); i++) {
            decrypted[i] = (char)((uint8_t)encrypted[i] ^ keyBytes[i]);
        }
        return decrypted;
    }

    // Sanitize input - strip dangerous characters for system() calls
    static std::string sanitizeInput(const std::string& input) {
        std::string sanitized;
        sanitized.reserve(input.size());

        for (size_t i = 0; i < input.size(); i++) {
            char c = input[i];
            // Strip dangerous shell metacharacters
            if (c == ';' || c == '|' || c == '&' || c == '$' || c == '`') {
                continue; // Skip dangerous chars
            }
            // Also strip backtick sequences and $() subshells
            if (c == '\\' && i + 1 < input.size()) {
                // Allow escaped chars but not shell escapes
                char next = input[i + 1];
                if (next == 'n' || next == 't' || next == '\\' || next == '"' || next == '\'') {
                    sanitized += c;
                    sanitized += next;
                    i++;
                    continue;
                }
                continue; // Skip other backslash sequences
            }
            sanitized += c;
        }
        return sanitized;
    }

    // Check if a string looks like it might be encrypted (hex encoded)
    static bool isEncrypted(const std::string& data) {
        if (data.empty() || data.size() % 2 != 0) return false;
        for (char c : data) {
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
        }
        return data.size() > 16; // Assume encrypted if long enough hex string
    }

    void setKey(const std::string& key) { encryptionKey = key; }
    bool hasKey() const { return !encryptionKey.empty(); }
};

#endif // MK_CRYPTO_CPP
