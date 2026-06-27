// ============================================================
// MK OS - Knowledge Sync Tool
// Syncs the MK knowledge base with a GitHub repository.
// This allows MK to:
//   1. PULL latest knowledge from GitHub (get updates from cloud)
//   2. PUSH learned facts back to GitHub (share what MK learned)
//   3. Auto-sync on a schedule (stay up to date)
//
// Use case: MK runs on iPhone (UTM SE) and syncs knowledge
// with the GitHub repo so you can update knowledge from anywhere.
//
// Compiles standalone:
//   clang++ -std=c++17 tools/knowledge_sync.cpp -o mk_sync -lcurl
//
// Usage:
//   ./mk_sync pull                  # Download latest knowledge from GitHub
//   ./mk_sync push                  # Upload local learned facts to GitHub
//   ./mk_sync status                # Check sync status
//   ./mk_sync auto                  # Auto-sync loop (every 30 min)
//
// Environment variables:
//   MK_GITHUB_TOKEN   - GitHub Personal Access Token (PAT)
//   MK_GITHUB_REPO    - Repository (default: mohd2456/MK_os-2)
//   MK_KNOWLEDGE_PATH - Local knowledge directory path
// ============================================================

#ifndef MK_KNOWLEDGE_SYNC_CPP
#define MK_KNOWLEDGE_SYNC_CPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>
#include <algorithm>
#include <map>
#include <curl/curl.h>

// ============================================================
// CURL Helper
// ============================================================
static size_t SyncWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ============================================================
// GitHub API Client for Knowledge Sync
// ============================================================
class MKKnowledgeSync {
private:
    std::string githubToken;
    std::string repoOwner;
    std::string repoName;
    std::string branch;
    std::string knowledgePath;       // Local path to knowledge files
    std::string remoteBasePath;      // Path in repo (MK_OS/ai_core/hre/knowledge_files)
    
    // Sync state
    std::string lastSyncTime;
    int totalPulled;
    int totalPushed;
    int syncErrors;

    // ── GitHub API Request ──
    std::string githubAPI(const std::string& endpoint, const std::string& method = "GET",
                          const std::string& body = "") {
        CURL* curl = curl_easy_init();
        if (!curl) return "";
        
        std::string response;
        std::string url = "https://api.github.com" + endpoint;
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + githubToken).c_str());
        headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");
        headers = curl_slist_append(headers, "User-Agent: MK-OS-Knowledge-Sync/1.0");
        if (!body.empty()) {
            headers = curl_slist_append(headers, "Content-Type: application/json");
        }
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, SyncWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        if (method == "PUT") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            if (!body.empty()) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            }
        } else if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            if (!body.empty()) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            }
        }
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "[SYNC ERROR] " << curl_easy_strerror(res) << "\n";
            syncErrors++;
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return response;
    }

    // ── Simple JSON field extractor ──
    std::string extractField(const std::string& json, const std::string& field) {
        std::string key = "\"" + field + "\"";
        size_t pos = json.find(key);
        if (pos == std::string::npos) return "";
        
        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        pos++;
        
        // Skip whitespace
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        
        if (json[pos] == '"') {
            // String value
            pos++;
            size_t end = json.find('"', pos);
            if (end == std::string::npos) return "";
            return json.substr(pos, end - pos);
        } else {
            // Number or other
            size_t end = pos;
            while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != '\n') end++;
            return json.substr(pos, end - pos);
        }
    }

    // ── Base64 encode (for GitHub file upload API) ──
    std::string base64Encode(const std::string& input) {
        static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string encoded;
        int val = 0, bits = -6;
        const unsigned int b63 = 0x3F;
        
        for (unsigned char c : input) {
            val = (val << 8) + c;
            bits += 8;
            while (bits >= 0) {
                encoded.push_back(chars[(val >> bits) & b63]);
                bits -= 6;
            }
        }
        if (bits > -6) encoded.push_back(chars[((val << 8) >> (bits + 8)) & b63]);
        while (encoded.size() % 4) encoded.push_back('=');
        return encoded;
    }

    // ── Base64 decode ──
    std::string base64Decode(const std::string& input) {
        static const int lookup[] = {
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
            -1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
            -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
        };
        
        std::string decoded;
        int val = 0, bits = -8;
        for (char c : input) {
            if (c == '=' || c == '\n' || c == '\r') continue;
            if ((unsigned char)c >= 128) continue;
            int v = lookup[(unsigned char)c];
            if (v == -1) continue;
            val = (val << 6) + v;
            bits += 6;
            if (bits >= 0) {
                decoded.push_back((char)((val >> bits) & 0xFF));
                bits -= 8;
            }
        }
        return decoded;
    }

    // ── Read local file ──
    std::string readLocalFile(const std::string& filename) {
        std::string path = knowledgePath + "/" + filename;
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        return content;
    }

    // ── Write local file ──
    bool writeLocalFile(const std::string& filename, const std::string& content) {
        std::string path = knowledgePath + "/" + filename;
        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << content;
        file.close();
        return true;
    }

    // ── Get current timestamp ──
    std::string getTimestamp() {
        time_t now = std::time(nullptr);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return std::string(buf);
    }

    // ── JSON escape ──
    std::string jsonEscape(const std::string& s) {
        std::string result;
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        return result;
    }

public:
    MKKnowledgeSync() : totalPulled(0), totalPushed(0), syncErrors(0) {
        // Load configuration from environment
        const char* token = std::getenv("MK_GITHUB_TOKEN");
        const char* repo = std::getenv("MK_GITHUB_REPO");
        const char* path = std::getenv("MK_KNOWLEDGE_PATH");
        
        githubToken = token ? token : "";
        
        // Parse owner/repo
        std::string fullRepo = repo ? repo : "mohd2456/MK_os-2";
        size_t slash = fullRepo.find('/');
        if (slash != std::string::npos) {
            repoOwner = fullRepo.substr(0, slash);
            repoName = fullRepo.substr(slash + 1);
        }
        
        knowledgePath = path ? path : "ai_core/hre/knowledge_files";
        remoteBasePath = "MK_OS/ai_core/hre/knowledge_files";
        branch = "main";
        
        std::cout << "[SYNC] MK Knowledge Sync initialized.\n";
        std::cout << "[SYNC] Repo: " << repoOwner << "/" << repoName << "\n";
        std::cout << "[SYNC] Local: " << knowledgePath << "\n";
        std::cout << "[SYNC] Remote: " << remoteBasePath << "\n";
    }

    // ════════════════════════════════════════════════════════════════
    //  PULL — Download latest knowledge from GitHub
    // ════════════════════════════════════════════════════════════════
    
    int pull() {
        if (githubToken.empty()) {
            std::cerr << "[SYNC ERROR] No GitHub token. Set MK_GITHUB_TOKEN env variable.\n";
            return -1;
        }
        
        std::cout << "[SYNC] Pulling latest knowledge from GitHub...\n";
        
        // Knowledge files to sync
        std::vector<std::string> files = {
            "core_facts.mk",
            "coding_knowledge.mk",
            "system_knowledge.mk",
            "world_knowledge.mk",
            "learned_facts.mk",
            "personal_facts.mk",
            "rules.mk",
            "lexicon.mk",
            "phrases.mk"
        };
        
        int pulled = 0;
        for (const auto& filename : files) {
            std::string remotePath = remoteBasePath + "/" + filename;
            std::string endpoint = "/repos/" + repoOwner + "/" + repoName + 
                                   "/contents/" + remotePath + "?ref=" + branch;
            
            std::string response = githubAPI(endpoint);
            if (response.empty()) {
                std::cout << "[SYNC] Skip (not found): " << filename << "\n";
                continue;
            }
            
            // Extract base64 content
            std::string content64 = extractField(response, "content");
            if (content64.empty()) continue;
            
            // Remove newlines from base64
            content64.erase(std::remove(content64.begin(), content64.end(), '\n'), content64.end());
            content64.erase(std::remove(content64.begin(), content64.end(), '\\'), content64.end());
            // Handle escaped 'n' from JSON newlines
            std::string cleanBase64;
            for (size_t i = 0; i < content64.size(); i++) {
                if (content64[i] == 'n' && i > 0 && content64[i-1] == '\\') continue;
                cleanBase64 += content64[i];
            }
            
            std::string decoded = base64Decode(content64);
            if (decoded.empty()) {
                std::cout << "[SYNC] Skip (empty content): " << filename << "\n";
                continue;
            }
            
            // Write to local file
            if (writeLocalFile(filename, decoded)) {
                // Count new facts
                int factCount = 0;
                std::istringstream ss(decoded);
                std::string line;
                while (std::getline(ss, line)) {
                    if (!line.empty() && line[0] != '#' && line.find('|') != std::string::npos) {
                        factCount++;
                    }
                }
                std::cout << "[SYNC] Updated: " << filename << " (" << factCount << " facts)\n";
                pulled++;
                totalPulled++;
            }
        }
        
        lastSyncTime = getTimestamp();
        std::cout << "[SYNC] Pull complete. " << pulled << " files updated.\n";
        return pulled;
    }

    // ════════════════════════════════════════════════════════════════
    //  PUSH — Upload local learned facts back to GitHub
    // ════════════════════════════════════════════════════════════════
    
    int push() {
        if (githubToken.empty()) {
            std::cerr << "[SYNC ERROR] No GitHub token. Set MK_GITHUB_TOKEN env variable.\n";
            return -1;
        }
        
        std::cout << "[SYNC] Pushing local knowledge to GitHub...\n";
        
        // Only push files that MK modifies at runtime
        std::vector<std::string> pushFiles = {
            "learned_facts.mk",
            "personal_facts.mk"
        };
        
        int pushed = 0;
        for (const auto& filename : pushFiles) {
            std::string content = readLocalFile(filename);
            if (content.empty()) continue;
            
            std::string remotePath = remoteBasePath + "/" + filename;
            
            // First, get the current SHA (needed for updates)
            std::string getEndpoint = "/repos/" + repoOwner + "/" + repoName + 
                                      "/contents/" + remotePath + "?ref=" + branch;
            std::string getResponse = githubAPI(getEndpoint);
            std::string sha = extractField(getResponse, "sha");
            
            // Encode content
            std::string encoded = base64Encode(content);
            
            // Build PUT request body
            std::string commitMsg = "sync: MK auto-push learned knowledge (" + getTimestamp() + ")";
            std::string body = "{"
                "\"message\":\"" + jsonEscape(commitMsg) + "\","
                "\"content\":\"" + encoded + "\","
                "\"branch\":\"" + branch + "\"";
            if (!sha.empty()) {
                body += ",\"sha\":\"" + sha + "\"";
            }
            body += "}";
            
            // Push to GitHub
            std::string putEndpoint = "/repos/" + repoOwner + "/" + repoName + 
                                      "/contents/" + remotePath;
            std::string putResponse = githubAPI(putEndpoint, "PUT", body);
            
            if (putResponse.find("\"content\"") != std::string::npos) {
                std::cout << "[SYNC] Pushed: " << filename << "\n";
                pushed++;
                totalPushed++;
            } else {
                std::cerr << "[SYNC ERROR] Failed to push: " << filename << "\n";
                syncErrors++;
            }
        }
        
        lastSyncTime = getTimestamp();
        std::cout << "[SYNC] Push complete. " << pushed << " files uploaded.\n";
        return pushed;
    }

    // ════════════════════════════════════════════════════════════════
    //  MERGE — Smart merge (don't overwrite, append new facts only)
    // ════════════════════════════════════════════════════════════════
    
    int smartMerge(const std::string& filename) {
        // Read local version
        std::string local = readLocalFile(filename);
        
        // Get remote version
        std::string remotePath = remoteBasePath + "/" + filename;
        std::string endpoint = "/repos/" + repoOwner + "/" + repoName + 
                               "/contents/" + remotePath + "?ref=" + branch;
        std::string response = githubAPI(endpoint);
        std::string content64 = extractField(response, "content");
        content64.erase(std::remove(content64.begin(), content64.end(), '\n'), content64.end());
        std::string remote = base64Decode(content64);
        
        if (remote.empty()) return 0;
        
        // Find facts in remote that are NOT in local
        std::map<std::string, bool> localFacts;
        std::istringstream localStream(local);
        std::string line;
        while (std::getline(localStream, line)) {
            if (!line.empty() && line[0] != '#' && line.find('|') != std::string::npos) {
                localFacts[line] = true;
            }
        }
        
        // Append new remote facts to local
        int newFacts = 0;
        std::ofstream out(knowledgePath + "/" + filename, std::ios::app);
        if (!out.is_open()) return 0;
        
        std::istringstream remoteStream(remote);
        while (std::getline(remoteStream, line)) {
            if (!line.empty() && line[0] != '#' && line.find('|') != std::string::npos) {
                if (localFacts.find(line) == localFacts.end()) {
                    out << line << "\n";
                    newFacts++;
                }
            }
        }
        out.close();
        
        if (newFacts > 0) {
            std::cout << "[SYNC MERGE] " << filename << ": +" << newFacts << " new facts merged.\n";
        }
        return newFacts;
    }

    // ════════════════════════════════════════════════════════════════
    //  STATUS — Check sync status
    // ════════════════════════════════════════════════════════════════
    
    void status() {
        std::cout << "\n  ═══ MK Knowledge Sync Status ═══\n";
        std::cout << "  Repo:        " << repoOwner << "/" << repoName << "\n";
        std::cout << "  Branch:      " << branch << "\n";
        std::cout << "  Local path:  " << knowledgePath << "\n";
        std::cout << "  Token:       " << (githubToken.empty() ? "NOT SET" : "configured") << "\n";
        std::cout << "  Last sync:   " << (lastSyncTime.empty() ? "never" : lastSyncTime) << "\n";
        std::cout << "  Pulled:      " << totalPulled << " files\n";
        std::cout << "  Pushed:      " << totalPushed << " files\n";
        std::cout << "  Errors:      " << syncErrors << "\n";
        
        // Count local facts
        std::vector<std::string> files = {"core_facts.mk", "coding_knowledge.mk", 
            "system_knowledge.mk", "world_knowledge.mk", "learned_facts.mk",
            "personal_facts.mk", "rules.mk"};
        int totalFacts = 0;
        for (const auto& f : files) {
            std::string content = readLocalFile(f);
            std::istringstream ss(content);
            std::string line;
            while (std::getline(ss, line)) {
                if (!line.empty() && line[0] != '#' && line.find('|') != std::string::npos) {
                    totalFacts++;
                }
            }
        }
        std::cout << "  Local facts: " << totalFacts << "\n";
        std::cout << "  ════════════════════════════════\n\n";
    }

    // ════════════════════════════════════════════════════════════════
    //  AUTO — Periodic sync loop
    // ════════════════════════════════════════════════════════════════
    
    void autoSync(int intervalMinutes = 30) {
        std::cout << "[SYNC] Auto-sync started. Interval: " << intervalMinutes << " minutes.\n";
        std::cout << "[SYNC] Press Ctrl+C to stop.\n";
        
        while (true) {
            // Pull first (get latest knowledge)
            pull();
            
            // Smart merge all files (don't overwrite, just add new facts)
            std::vector<std::string> mergeFiles = {
                "core_facts.mk", "coding_knowledge.mk", "system_knowledge.mk",
                "world_knowledge.mk"
            };
            for (const auto& f : mergeFiles) {
                smartMerge(f);
            }
            
            // Push local learned facts back
            push();
            
            std::cout << "[SYNC] Next sync in " << intervalMinutes << " minutes...\n";
            std::this_thread::sleep_for(std::chrono::minutes(intervalMinutes));
        }
    }

    // ════════════════════════════════════════════════════════════════
    //  QUICK PUSH — Push a single new fact immediately
    // ════════════════════════════════════════════════════════════════
    
    bool pushFact(const std::string& source, const std::string& relation, 
                  const std::string& target, float weight = 1.0f) {
        // Append locally
        std::string factLine = source + "|" + relation + "|" + target + "|" + 
                               std::to_string(weight).substr(0, 4);
        
        std::ofstream out(knowledgePath + "/learned_facts.mk", std::ios::app);
        if (out.is_open()) {
            out << factLine << "\n";
            out.close();
        }
        
        // Push to GitHub
        return push() > 0;
    }
};

// ============================================================
// Standalone main() — Command-line interface
// ============================================================
int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════╗\n";
    std::cout << "  ║   MK Knowledge Sync Tool v1.0       ║\n";
    std::cout << "  ╚══════════════════════════════════════╝\n\n";
    
    if (argc < 2) {
        std::cout << "  Usage: mk_sync <command>\n\n";
        std::cout << "  Commands:\n";
        std::cout << "    pull     Download latest knowledge from GitHub\n";
        std::cout << "    push     Upload local learned facts to GitHub\n";
        std::cout << "    merge    Smart merge (append only new facts)\n";
        std::cout << "    status   Show sync status\n";
        std::cout << "    auto     Auto-sync loop (every 30 min)\n";
        std::cout << "\n  Environment variables:\n";
        std::cout << "    MK_GITHUB_TOKEN    Your GitHub Personal Access Token\n";
        std::cout << "    MK_GITHUB_REPO     Repository (default: mohd2456/MK_os-2)\n";
        std::cout << "    MK_KNOWLEDGE_PATH  Local knowledge dir (default: ai_core/hre/knowledge_files)\n\n";
        return 1;
    }
    
    MKKnowledgeSync sync;
    std::string command = argv[1];
    
    if (command == "pull") {
        sync.pull();
    } else if (command == "push") {
        sync.push();
    } else if (command == "merge") {
        std::vector<std::string> files = {"core_facts.mk", "coding_knowledge.mk",
            "system_knowledge.mk", "world_knowledge.mk", "learned_facts.mk"};
        for (const auto& f : files) {
            sync.smartMerge(f);
        }
    } else if (command == "status") {
        sync.status();
    } else if (command == "auto") {
        int interval = (argc >= 3) ? std::atoi(argv[2]) : 30;
        sync.autoSync(interval);
    } else {
        std::cerr << "  Unknown command: " << command << "\n";
        return 1;
    }
    
    return 0;
}

#endif // MK_KNOWLEDGE_SYNC_CPP
