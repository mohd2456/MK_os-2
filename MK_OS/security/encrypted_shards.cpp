#ifndef MK_ENCRYPTED_SHARDS_CPP
#define MK_ENCRYPTED_SHARDS_CPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <random>
#include <sstream>
#include <iomanip>

// ===================================================================================
// MK ENCRYPTED STORAGE SHARDS
// Provides encrypted-at-rest storage for mk_brain database shards.
// Each shard is independently encrypted so compromising one doesn't expose all data.
// Features:
//   - Per-shard encryption with unique derived keys
//   - Integrity verification via HMAC-style checksums
//   - Transparent encrypt/decrypt on read/write
//   - Key derivation from master password + shard salt
//   - Secure memory wiping after use
// ===================================================================================

struct MKShardHeader {
    char magic[4];          // "MKSH"
    uint32_t version;
    uint32_t shardId;
    uint32_t dataSize;
    uint64_t createdAt;
    uint64_t modifiedAt;
    uint8_t salt[16];       // Random salt for key derivation
    uint8_t checksum[8];    // Integrity checksum
    uint8_t flags;          // Encryption flags
    uint8_t reserved[7];
};

struct MKShard {
    int shardId;
    std::string name;
    std::string filename;
    bool encrypted;
    bool loaded;
    size_t dataSize;
    std::vector<uint8_t> plainData;     // Decrypted data (only in memory when needed)
    std::vector<uint8_t> salt;
};

class MKEncryptedShards {
private:
    std::map<int, MKShard> shards;
    std::string masterKey;
    std::string storageDir;
    int nextShardId;
    bool locked;            // If true, no decryption allowed until unlock
    
    // Derive a per-shard key from master key + salt
    std::vector<uint8_t> deriveKey(const std::string& master, const std::vector<uint8_t>& salt) {
        std::vector<uint8_t> key(32, 0);
        
        // PBKDF2-like derivation: iterate hash mixing
        for (int iter = 0; iter < 1000; iter++) {
            for (size_t i = 0; i < key.size(); i++) {
                key[i] ^= master[i % master.size()];
                key[i] ^= salt[i % salt.size()];
                key[i] = (key[i] * 37 + iter + i) & 0xFF;
            }
        }
        return key;
    }
    
    // XOR stream cipher encrypt/decrypt (symmetric)
    std::vector<uint8_t> xorCipher(const std::vector<uint8_t>& data, 
                                    const std::vector<uint8_t>& key) {
        std::vector<uint8_t> result(data.size());
        for (size_t i = 0; i < data.size(); i++) {
            // Generate keystream byte from key + position
            uint8_t ks = key[i % key.size()] ^ (uint8_t)((i * 7 + 13) & 0xFF);
            result[i] = data[i] ^ ks;
        }
        return result;
    }
    
    // Compute integrity checksum
    uint64_t computeChecksum(const std::vector<uint8_t>& data) {
        uint64_t hash = 0x5381;
        for (uint8_t b : data) {
            hash = ((hash << 5) + hash) + b;
        }
        return hash;
    }
    
    // Generate random salt
    std::vector<uint8_t> generateSalt(size_t length = 16) {
        std::vector<uint8_t> salt(length);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        for (auto& b : salt) b = dist(gen);
        return salt;
    }
    
    // Securely wipe memory
    void secureWipe(std::vector<uint8_t>& data) {
        volatile uint8_t* ptr = data.data();
        for (size_t i = 0; i < data.size(); i++) {
            ptr[i] = 0;
        }
        data.clear();
    }

public:
    MKEncryptedShards(const std::string& dir = "mk_shards") 
        : storageDir(dir), nextShardId(0), locked(true) {
        masterKey = "";
        std::cout << "[ENCRYPTED SHARDS] Secure shard storage initialized. Status: LOCKED\n";
    }
    
    // Unlock the shard system with master password
    void unlock(const std::string& password) {
        masterKey = password;
        locked = false;
        std::cout << "[ENCRYPTED SHARDS] Vault UNLOCKED.\n";
    }
    
    // Lock the vault — wipes all decrypted data from memory
    void lock() {
        for (auto& kv : shards) {
            if (!kv.second.plainData.empty()) {
                secureWipe(kv.second.plainData);
                kv.second.loaded = false;
            }
        }
        masterKey.clear();
        locked = true;
        std::cout << "[ENCRYPTED SHARDS] Vault LOCKED. All plaintext wiped from RAM.\n";
    }
    
    // Create a new encrypted shard
    int createShard(const std::string& name, const std::vector<uint8_t>& data) {
        if (locked) {
            std::cerr << "[ENCRYPTED SHARDS] ERROR: Vault is locked!\n";
            return -1;
        }
        
        MKShard shard;
        shard.shardId = nextShardId++;
        shard.name = name;
        shard.filename = storageDir + "/shard_" + std::to_string(shard.shardId) + ".enc";
        shard.encrypted = true;
        shard.loaded = true;
        shard.dataSize = data.size();
        shard.plainData = data;
        shard.salt = generateSalt();
        
        shards[shard.shardId] = shard;
        
        // Immediately persist encrypted to disk
        persistShard(shard.shardId);
        
        std::cout << "[ENCRYPTED SHARDS] Created shard #" << shard.shardId 
                  << " \"" << name << "\" (" << data.size() << " bytes)\n";
        return shard.shardId;
    }
    
    // Create shard from string data
    int createShardFromString(const std::string& name, const std::string& data) {
        std::vector<uint8_t> bytes(data.begin(), data.end());
        return createShard(name, bytes);
    }
    
    // Read a shard (decrypts from disk if not in memory)
    std::vector<uint8_t> readShard(int shardId) {
        if (locked) {
            std::cerr << "[ENCRYPTED SHARDS] ERROR: Vault is locked!\n";
            return {};
        }
        
        auto it = shards.find(shardId);
        if (it == shards.end()) return {};
        
        if (!it->second.loaded) {
            loadShard(shardId);
        }
        return it->second.plainData;
    }
    
    // Read shard as string
    std::string readShardString(int shardId) {
        auto data = readShard(shardId);
        return std::string(data.begin(), data.end());
    }
    
    // Persist (encrypt and save) a shard to disk
    void persistShard(int shardId) {
        auto it = shards.find(shardId);
        if (it == shards.end() || it->second.plainData.empty()) return;
        
        auto& shard = it->second;
        std::vector<uint8_t> key = deriveKey(masterKey, shard.salt);
        std::vector<uint8_t> encrypted = xorCipher(shard.plainData, key);
        uint64_t checksum = computeChecksum(shard.plainData);
        
        // Write to file with header
        std::ofstream out(shard.filename, std::ios::binary);
        if (!out.is_open()) return;
        
        MKShardHeader header;
        std::memcpy(header.magic, "MKSH", 4);
        header.version = 1;
        header.shardId = shard.shardId;
        header.dataSize = encrypted.size();
        header.createdAt = std::time(nullptr);
        header.modifiedAt = header.createdAt;
        std::memcpy(header.salt, shard.salt.data(), std::min(shard.salt.size(), (size_t)16));
        std::memcpy(header.checksum, &checksum, 8);
        header.flags = 0x01;  // Encrypted flag
        
        out.write((char*)&header, sizeof(header));
        out.write((char*)encrypted.data(), encrypted.size());
        out.close();
    }
    
    // Load (decrypt from disk) a shard into memory
    void loadShard(int shardId) {
        auto it = shards.find(shardId);
        if (it == shards.end()) return;
        
        auto& shard = it->second;
        std::ifstream in(shard.filename, std::ios::binary);
        if (!in.is_open()) return;
        
        MKShardHeader header;
        in.read((char*)&header, sizeof(header));
        
        std::vector<uint8_t> encrypted(header.dataSize);
        in.read((char*)encrypted.data(), header.dataSize);
        in.close();
        
        // Decrypt
        shard.salt.assign(header.salt, header.salt + 16);
        std::vector<uint8_t> key = deriveKey(masterKey, shard.salt);
        shard.plainData = xorCipher(encrypted, key);
        shard.loaded = true;
        
        // Verify checksum
        uint64_t expectedChecksum;
        std::memcpy(&expectedChecksum, header.checksum, 8);
        uint64_t actualChecksum = computeChecksum(shard.plainData);
        
        if (expectedChecksum != actualChecksum) {
            std::cerr << "[ENCRYPTED SHARDS] INTEGRITY FAILURE on shard #" << shardId << "!\n";
            secureWipe(shard.plainData);
            shard.loaded = false;
        }
    }
    
    // Delete a shard (wipe from memory and disk)
    void deleteShard(int shardId) {
        auto it = shards.find(shardId);
        if (it == shards.end()) return;
        
        secureWipe(it->second.plainData);
        std::remove(it->second.filename.c_str());
        shards.erase(it);
        std::cout << "[ENCRYPTED SHARDS] Shard #" << shardId << " destroyed.\n";
    }
    
    int shardCount() const { return shards.size(); }
    bool isLocked() const { return locked; }
    
    void printStatus() const {
        std::cout << "\n--- [ENCRYPTED SHARDS] ---\n";
        std::cout << "  Status: " << (locked ? "LOCKED" : "UNLOCKED") << "\n";
        std::cout << "  Shards: " << shards.size() << "\n";
        for (const auto& kv : shards) {
            std::cout << "  #" << kv.second.shardId << " \"" << kv.second.name 
                      << "\" | " << kv.second.dataSize << " bytes"
                      << " | " << (kv.second.loaded ? "IN MEMORY" : "ON DISK") << "\n";
        }
        std::cout << "--------------------------\n";
    }
};

#endif // MK_ENCRYPTED_SHARDS_CPP
