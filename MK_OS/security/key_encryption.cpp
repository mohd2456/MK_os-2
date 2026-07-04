// ============================================================
// MK OS - API Key Encryption (Basic Obfuscation Layer)
// ============================================================
// Encrypts API keys using XOR cipher with a device-unique seed.
// NOT military-grade crypto - this is obfuscation to prevent
// plaintext keys sitting on disk. Better than nothing.
//
// Keys are stored in encrypted_keys.conf as PROVIDER_NAME=<hex>
// ============================================================
#ifndef MK_KEY_ENCRYPTION_CPP
#define MK_KEY_ENCRYPTION_CPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <map>

#ifdef __APPLE__
#include <unistd.h>
#endif

#ifdef __linux__
#include <unistd.h>
#endif

class MKKeyEncryption {
private:
    std::string seed_;
    std::string configPath_;

    // Generate a device-unique seed from hostname + username hash
    std::string generateSeed() {
        std::string raw;

        // Get hostname
        char hostname[256] = {0};
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            raw += std::string(hostname);
        } else {
            raw += "mk_default_host";
        }

        // Get username
        const char* user = std::getenv("USER");
        if (!user) user = std::getenv("USERNAME");
        if (user) {
            raw += std::string(user);
        } else {
            raw += "mk_user";
        }

        // Simple hash: DJB2 style
        unsigned long hash = 5381;
        for (char c : raw) {
            hash = ((hash << 5) + hash) + (unsigned char)c;
        }

        // Convert hash to a repeatable key string
        std::string seedStr;
        for (int i = 0; i < 32; i++) {
            seedStr += (char)(33 + ((hash >> (i % 8)) + i * 7) % 93);
            hash = hash * 6364136223846793005ULL + 1442695040888963407ULL;
        }

        return seedStr;
    }

    // XOR cipher using the seed (repeating key)
    std::string xorCipher(const std::string& input) {
        std::string output;
        output.reserve(input.size());
        for (size_t i = 0; i < input.size(); i++) {
            output += (char)(input[i] ^ seed_[i % seed_.size()]);
        }
        return output;
    }

    // Convert binary string to hex
    std::string toHex(const std::string& data) {
        std::ostringstream oss;
        for (unsigned char c : data) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        }
        return oss.str();
    }

    // Convert hex string to binary
    std::string fromHex(const std::string& hex) {
        std::string result;
        for (size_t i = 0; i + 1 < hex.size(); i += 2) {
            std::string byteStr = hex.substr(i, 2);
            char byte = (char)std::stoul(byteStr, nullptr, 16);
            result += byte;
        }
        return result;
    }

public:
    MKKeyEncryption(const std::string& configPath = ".")
        : configPath_(configPath) {
        seed_ = generateSeed();
    }

    // Encrypt plaintext key -> hex string
    std::string encrypt(const std::string& plaintext) {
        if (plaintext.empty()) return "";
        std::string ciphertext = xorCipher(plaintext);
        return toHex(ciphertext);
    }

    // Decrypt hex-encoded encrypted key -> plaintext
    std::string decrypt(const std::string& hexEncrypted) {
        if (hexEncrypted.empty()) return "";
        std::string ciphertext = fromHex(hexEncrypted);
        return xorCipher(ciphertext);
    }

    // Save a key to encrypted_keys.conf
    void saveKey(const std::string& providerName, const std::string& apiKey) {
        // Read existing keys first
        std::map<std::string, std::string> keys = loadAllKeys();
        keys[providerName] = encrypt(apiKey);

        // Write all keys back
        std::string filePath = configPath_ + "/encrypted_keys.conf";
        std::ofstream file(filePath);
        if (!file.is_open()) return;

        file << "# MK OS Encrypted API Keys\n";
        file << "# DO NOT EDIT - keys are encrypted with device-specific seed\n\n";

        for (const auto& kv : keys) {
            file << kv.first << "=" << kv.second << "\n";
        }
        file.close();
    }

    // Load a specific key from encrypted_keys.conf (returns decrypted)
    std::string loadKey(const std::string& providerName) {
        std::map<std::string, std::string> keys = loadAllKeys();
        auto it = keys.find(providerName);
        if (it != keys.end()) {
            return decrypt(it->second);
        }
        return "";
    }

    // Load all encrypted keys (returns map of name -> encrypted_hex)
    std::map<std::string, std::string> loadAllKeys() {
        std::map<std::string, std::string> keys;
        std::string filePath = configPath_ + "/encrypted_keys.conf";
        std::ifstream file(filePath);
        if (!file.is_open()) return keys;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string name = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            // Trim
            while (!val.empty() && (val.back() == ' ' || val.back() == '\n' || val.back() == '\r'))
                val.pop_back();
            while (!name.empty() && name.back() == ' ')
                name.pop_back();
            while (!name.empty() && name.front() == ' ')
                name.erase(name.begin());
            if (!name.empty() && !val.empty()) {
                keys[name] = val;
            }
        }
        return keys;
    }

    // Check if encrypted keys file exists and has content
    bool hasEncryptedKeys() {
        std::string filePath = configPath_ + "/encrypted_keys.conf";
        std::ifstream file(filePath);
        if (!file.is_open()) return false;
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line[0] != '#') return true;
        }
        return false;
    }

    // Get the config path
    std::string getConfigPath() const { return configPath_; }
};

#endif // MK_KEY_ENCRYPTION_CPP
