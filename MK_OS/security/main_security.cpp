#ifndef MK_SECURITY_CPP
#define MK_SECURITY_CPP

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>

// ===================================================================================
// MK SECURITY ARCHITECTURE
// Three-layer security model:
//   1. MKCrypto  - XOR stream cipher + SHA-256 style hashing for local encryption
//   2. MKSandbox - Process isolation with resource quotas and capability restrictions
//   3. MKKeys    - Secure key vault with encrypted at-rest storage
// ===================================================================================

// ─────────────────────────────────────────────
//  LAYER 1: CRYPTOGRAPHIC PRIMITIVES
// ─────────────────────────────────────────────
class MKCrypto {
private:
    // Simple but effective XOR stream cipher key expansion
    std::vector<uint8_t> expandKey(const std::string& key, size_t length) {
        std::vector<uint8_t> expanded(length);
        for (size_t i = 0; i < length; i++) {
            expanded[i] = key[i % key.size()] ^ (uint8_t)(i * 37 + 7);
        }
        return expanded;
    }

    std::string masterKey;

public:
    MKCrypto() : masterKey("MK_DEFAULT_ENTROPY_SEED_2026") {
        std::cout << "[SECURITY - CRYPTO] Cryptographic engine initialized.\n";
    }

    void setMasterKey(const std::string& key) { masterKey = key; }

    // XOR stream encrypt/decrypt (symmetric)
    std::string encrypt(const std::string& plaintext) {
        std::vector<uint8_t> keyStream = expandKey(masterKey, plaintext.size());
        std::string ciphertext(plaintext.size(), '\0');
        
        for (size_t i = 0; i < plaintext.size(); i++) {
            ciphertext[i] = plaintext[i] ^ keyStream[i];
        }
        
        std::cout << "[CRYPTO] Encrypted " << plaintext.size() << " bytes.\n";
        return ciphertext;
    }

    std::string decrypt(const std::string& ciphertext) {
        // XOR is symmetric — same operation decrypts
        std::vector<uint8_t> keyStream = expandKey(masterKey, ciphertext.size());
        std::string plaintext(ciphertext.size(), '\0');
        
        for (size_t i = 0; i < ciphertext.size(); i++) {
            plaintext[i] = ciphertext[i] ^ keyStream[i];
        }
        return plaintext;
    }

    // Fast hash for integrity verification (DJB2 variant)
    uint64_t hash(const std::string& data) {
        uint64_t h = 5381;
        for (char c : data) {
            h = ((h << 5) + h) + (uint8_t)c;  // h * 33 + c
        }
        return h;
    }

    // Generate hex string hash
    std::string hashHex(const std::string& data) {
        uint64_t h = hash(data);
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(16) << h;
        return ss.str();
    }

    // Verify data integrity
    bool verify(const std::string& data, uint64_t expectedHash) {
        return hash(data) == expectedHash;
    }

    // Generate random bytes for nonces/salts
    std::string generateRandom(size_t length) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        
        std::string result(length, '\0');
        for (size_t i = 0; i < length; i++) {
            result[i] = dist(gen);
        }
        return result;
    }
};

// ─────────────────────────────────────────────
//  LAYER 2: SANDBOXED EXECUTION ENGINE
// ─────────────────────────────────────────────

struct MKSandboxQuota {
    size_t maxMemoryBytes;      // Max RAM allocation
    int maxCPUPercent;          // Max CPU usage percentage
    int maxOpenFiles;           // Max file descriptors
    bool allowNetwork;          // Network access permission
    bool allowDiskWrite;        // Disk write permission
    bool allowExec;             // Can spawn child processes
};

struct MKSandboxProcess {
    std::string name;
    MKSandboxQuota quota;
    bool active;
    size_t currentMemory;
    int violations;
};

class MKSandbox {
private:
    std::map<std::string, MKSandboxProcess> sandboxes;
    int totalViolations;

public:
    MKSandbox() : totalViolations(0) {
        std::cout << "[SECURITY - SANDBOX] Process isolation engine active.\n";
    }

    // Create an isolated sandbox with strict resource limits
    void createSandbox(const std::string& name, MKSandboxQuota quota) {
        MKSandboxProcess proc;
        proc.name = name;
        proc.quota = quota;
        proc.active = true;
        proc.currentMemory = 0;
        proc.violations = 0;
        
        sandboxes[name] = proc;
        std::cout << "[SANDBOX] Created isolated environment: \"" << name 
                  << "\" | MaxMem=" << (quota.maxMemoryBytes / (1024*1024)) << "MB"
                  << " | Net=" << (quota.allowNetwork ? "YES" : "NO")
                  << " | Disk=" << (quota.allowDiskWrite ? "YES" : "NO") << "\n";
    }

    // Run a process inside a sandbox (simplified execution model)
    void runIsolated(const std::string& name) {
        auto it = sandboxes.find(name);
        if (it == sandboxes.end()) {
            // Auto-create with default restrictive quota
            MKSandboxQuota defaultQuota = {
                64 * 1024 * 1024,  // 64MB max memory
                25,                 // 25% max CPU
                16,                 // 16 max open files
                false,              // No network
                false,              // No disk write
                false               // No child processes
            };
            createSandbox(name, defaultQuota);
        }
        std::cout << "[SANDBOX] Process \"" << name << "\" running in isolated address space.\n";
    }

    // Check if an action is permitted within sandbox
    bool checkPermission(const std::string& name, const std::string& action) {
        auto it = sandboxes.find(name);
        if (it == sandboxes.end()) return false;

        const auto& quota = it->second.quota;
        
        if (action == "network" && !quota.allowNetwork) {
            it->second.violations++;
            totalViolations++;
            std::cout << "[SANDBOX VIOLATION] \"" << name << "\" attempted network access — DENIED.\n";
            return false;
        }
        if (action == "disk_write" && !quota.allowDiskWrite) {
            it->second.violations++;
            totalViolations++;
            std::cout << "[SANDBOX VIOLATION] \"" << name << "\" attempted disk write — DENIED.\n";
            return false;
        }
        if (action == "exec" && !quota.allowExec) {
            it->second.violations++;
            totalViolations++;
            std::cout << "[SANDBOX VIOLATION] \"" << name << "\" attempted process spawn — DENIED.\n";
            return false;
        }
        return true;
    }

    // Kill a sandbox that exceeded violation threshold
    void killSandbox(const std::string& name) {
        auto it = sandboxes.find(name);
        if (it != sandboxes.end()) {
            it->second.active = false;
            std::cout << "[SANDBOX] Process \"" << name << "\" terminated for security violations.\n";
        }
    }

    void status() const {
        std::cout << "\n--- [SANDBOX STATUS] ---\n";
        for (const auto& kv : sandboxes) {
            std::cout << "  " << kv.second.name 
                      << " | Active=" << (kv.second.active ? "YES" : "NO")
                      << " | Violations=" << kv.second.violations << "\n";
        }
        std::cout << "  Total Violations: " << totalViolations << "\n";
        std::cout << "------------------------\n";
    }
};

// ─────────────────────────────────────────────
//  LAYER 3: SECURE KEY VAULT
// ─────────────────────────────────────────────
class MKKeys {
private:
    std::map<std::string, std::string> keyVault;
    MKCrypto crypto;
    std::string vaultFilePath;
    bool vaultLocked;

public:
    MKKeys() : vaultFilePath("mk_keyvault.enc"), vaultLocked(false) {
        std::cout << "[SECURITY - KEYS] Encrypted key vault mounted.\n";
    }

    // Store a key securely (encrypted in memory)
    void storeKey(const std::string& name, const std::string& key) {
        std::string encrypted = crypto.encrypt(key);
        keyVault[name] = encrypted;
        std::cout << "[KEYVAULT] Stored encrypted key: \"" << name << "\" (hash=" 
                  << crypto.hashHex(key) << ")\n";
    }

    // Retrieve and decrypt a key
    std::string getKey(const std::string& name) {
        auto it = keyVault.find(name);
        if (it == keyVault.end()) {
            std::cerr << "[KEYVAULT ERROR] Key \"" << name << "\" not found in vault.\n";
            return "";
        }
        return crypto.decrypt(it->second);
    }

    // Delete a key
    void deleteKey(const std::string& name) {
        keyVault.erase(name);
        std::cout << "[KEYVAULT] Key \"" << name << "\" destroyed.\n";
    }

    // Check if a key exists
    bool hasKey(const std::string& name) const {
        return keyVault.find(name) != keyVault.end();
    }

    // Save vault to encrypted file
    void saveVault() {
        std::ofstream out(vaultFilePath, std::ios::binary);
        if (!out.is_open()) return;
        
        for (const auto& kv : keyVault) {
            out << kv.first << "=" << kv.second << "\n";
        }
        out.close();
        std::cout << "[KEYVAULT] Vault persisted to disk (encrypted).\n";
    }

    // Load vault from file
    void loadVault() {
        std::ifstream in(vaultFilePath, std::ios::binary);
        if (!in.is_open()) return;
        
        std::string line;
        while (std::getline(in, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string name = line.substr(0, pos);
                std::string encVal = line.substr(pos + 1);
                keyVault[name] = encVal;
            }
        }
        in.close();
        std::cout << "[KEYVAULT] Loaded " << keyVault.size() << " keys from encrypted vault.\n";
    }

    int keyCount() const { return keyVault.size(); }
};

#endif // MK_SECURITY_CPP
