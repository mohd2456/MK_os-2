#ifndef MK_BRAIN_MEMORY_CPP
#define MK_BRAIN_MEMORY_CPP

#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <sqlite3.h>

struct DialogTurn {
    std::string role;
    std::string content;
};

// Long-term memory entry stored in SQLite
struct MKLongTermMemory {
    int id;
    std::string topic;
    std::string summary;
    std::time_t timestamp;
    float importance;
};

class MKBrainMemory {
private:
    std::vector<DialogTurn> shortTermContext;
    size_t maxContextTurns;

    // SQLite long-term memory
    sqlite3* ltmDb;
    std::string dbPath;
    bool dbInitialized;

    // Initialize the SQLite database and create tables
    bool initDatabase() {
        if (dbInitialized) return true;

        int rc = sqlite3_open(dbPath.c_str(), &ltmDb);
        if (rc != SQLITE_OK) {
            std::cerr << "[BRAIN MEMORY] Cannot open SQLite DB: " << sqlite3_errmsg(ltmDb) << "\n";
            ltmDb = nullptr;
            return false;
        }

        const char* createTable =
            "CREATE TABLE IF NOT EXISTS conversations ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  topic TEXT NOT NULL,"
            "  summary TEXT NOT NULL,"
            "  timestamp INTEGER NOT NULL,"
            "  importance REAL DEFAULT 0.5"
            ");";

        char* errMsg = nullptr;
        rc = sqlite3_exec(ltmDb, createTable, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "[BRAIN MEMORY] SQLite table creation failed: " << errMsg << "\n";
            sqlite3_free(errMsg);
            return false;
        }

        dbInitialized = true;
        return true;
    }

    // Extract a simple topic from dialog turns (first meaningful noun/phrase)
    std::string extractTopic(const std::vector<DialogTurn>& turns) {
        for (const auto& turn : turns) {
            if (turn.role == "user" && !turn.content.empty()) {
                // Use first user message as topic seed, truncate to 80 chars
                std::string topic = turn.content;
                if (topic.size() > 80) topic = topic.substr(0, 80);
                return topic;
            }
        }
        return "general conversation";
    }

    // Summarize a set of dialog turns into a compact string
    std::string summarizeTurns(const std::vector<DialogTurn>& turns) {
        std::string summary;
        for (const auto& turn : turns) {
            if (!summary.empty()) summary += " | ";
            std::string content = turn.content;
            if (content.size() > 100) content = content.substr(0, 100) + "...";
            summary += turn.role + ": " + content;
        }
        // Cap at 500 chars total
        if (summary.size() > 500) summary = summary.substr(0, 500);
        return summary;
    }

    // Score importance of dialog turns (higher = more worth remembering)
    float scoreImportance(const std::vector<DialogTurn>& turns) {
        float score = 0.3f; // base importance

        for (const auto& turn : turns) {
            std::string lower = turn.content;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            // Mentions of personal info boost importance
            if (lower.find("remember") != std::string::npos) score += 0.3f;
            if (lower.find("important") != std::string::npos) score += 0.2f;
            if (lower.find("my ") != std::string::npos) score += 0.1f;
            if (lower.find("don't forget") != std::string::npos) score += 0.3f;
            // Technical/project discussions are important
            if (lower.find("server") != std::string::npos) score += 0.1f;
            if (lower.find("deploy") != std::string::npos) score += 0.1f;
            if (lower.find("docker") != std::string::npos) score += 0.1f;
            if (lower.find("homelab") != std::string::npos) score += 0.1f;
        }

        if (score > 1.0f) score = 1.0f;
        return score;
    }

public:
    MKBrainMemory(size_t maxTurns = 6)
        : maxContextTurns(maxTurns), ltmDb(nullptr),
          dbPath("mk_brain_memory.db"), dbInitialized(false) {
        std::cout << "[BRAIN MEMORY] Initializing context tracking registers...\n";
    }

    ~MKBrainMemory() {
        if (ltmDb) {
            sqlite3_close(ltmDb);
            ltmDb = nullptr;
        }
    }

    // Appends a new interaction turn to the rotating context window
    void commitToShortTerm(const std::string& role, const std::string& text) {
        if (shortTermContext.size() >= maxContextTurns) {
            // Before evicting, archive oldest turns to long-term memory
            archiveToLongTerm();
        }
        shortTermContext.push_back({role, text});
    }

    // Archives the oldest turns from short-term to SQLite long-term memory
    void archiveToLongTerm() {
        if (shortTermContext.empty()) return;

        // Archive the oldest 2 turns (one exchange) before eviction
        size_t archiveCount = std::min((size_t)2, shortTermContext.size());
        std::vector<DialogTurn> toArchive(shortTermContext.begin(),
                                          shortTermContext.begin() + archiveCount);

        std::string topic = extractTopic(toArchive);
        std::string summary = summarizeTurns(toArchive);
        float importance = scoreImportance(toArchive);

        // Save to SQLite
        saveToDisk(topic, summary, importance);

        // Evict the archived turns
        shortTermContext.erase(shortTermContext.begin(),
                               shortTermContext.begin() + archiveCount);
    }

    // Persist a conversation summary to SQLite long-term memory
    bool saveToDisk(const std::string& topic, const std::string& summary, float importance) {
        if (!initDatabase()) return false;

        const char* insertSql =
            "INSERT INTO conversations (topic, summary, timestamp, importance) VALUES (?, ?, ?, ?);";

        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(ltmDb, insertSql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) return false;

        std::time_t now = std::time(nullptr);
        sqlite3_bind_text(stmt, 1, topic.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, summary.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, (sqlite3_int64)now);
        sqlite3_bind_double(stmt, 4, (double)importance);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE);
    }

    // Load recent memories from SQLite (most recent N entries)
    std::vector<MKLongTermMemory> loadFromDisk(int maxEntries = 20) {
        std::vector<MKLongTermMemory> memories;
        if (!initDatabase()) return memories;

        const char* selectSql =
            "SELECT id, topic, summary, timestamp, importance "
            "FROM conversations ORDER BY timestamp DESC LIMIT ?;";

        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(ltmDb, selectSql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) return memories;

        sqlite3_bind_int(stmt, 1, maxEntries);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            MKLongTermMemory mem;
            mem.id = sqlite3_column_int(stmt, 0);
            const char* topicVal = (const char*)sqlite3_column_text(stmt, 1);
            const char* summaryVal = (const char*)sqlite3_column_text(stmt, 2);
            mem.topic = topicVal ? topicVal : "";
            mem.summary = summaryVal ? summaryVal : "";
            mem.timestamp = (std::time_t)sqlite3_column_int64(stmt, 3);
            mem.importance = (float)sqlite3_column_double(stmt, 4);
            memories.push_back(mem);
        }

        sqlite3_finalize(stmt);
        return memories;
    }

    // Query long-term memories by keyword relevance
    std::vector<MKLongTermMemory> queryRelevantMemories(const std::string& query, int maxResults = 5) {
        std::vector<MKLongTermMemory> results;
        if (!initDatabase()) return results;
        if (query.empty()) return results;

        // Use SQLite LIKE for basic keyword matching
        // Search both topic and summary columns
        const char* searchSql =
            "SELECT id, topic, summary, timestamp, importance "
            "FROM conversations "
            "WHERE topic LIKE ? OR summary LIKE ? "
            "ORDER BY importance DESC, timestamp DESC LIMIT ?;";

        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(ltmDb, searchSql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) return results;

        std::string pattern = "%" + query + "%";
        sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, maxResults);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            MKLongTermMemory mem;
            mem.id = sqlite3_column_int(stmt, 0);
            const char* topicVal = (const char*)sqlite3_column_text(stmt, 1);
            const char* summaryVal = (const char*)sqlite3_column_text(stmt, 2);
            mem.topic = topicVal ? topicVal : "";
            mem.summary = summaryVal ? summaryVal : "";
            mem.timestamp = (std::time_t)sqlite3_column_int64(stmt, 3);
            mem.importance = (float)sqlite3_column_double(stmt, 4);
            results.push_back(mem);
        }

        sqlite3_finalize(stmt);
        return results;
    }

    // Get number of long-term memories stored
    int longTermCount() {
        if (!initDatabase()) return 0;

        const char* countSql = "SELECT COUNT(*) FROM conversations;";
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(ltmDb, countSql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) return 0;

        int count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        return count;
    }

    void dumpContext() const {
        std::cout << "--- ACTIVE CONTEXT BUFFER ---\n";
        for (const auto& turn : shortTermContext) {
            std::cout << " [" << turn.role << "]: " << turn.content << '\n';
        }
        std::cout << "-----------------------------\n";
    }

    // Returns conversation history as a formatted string for LLM context
    std::string getContextString() const {
        std::string result;
        for (const auto& turn : shortTermContext) {
            result += turn.role + ": " + turn.content + "\n";
        }
        return result;
    }

    // Get current short-term buffer size
    size_t shortTermSize() const { return shortTermContext.size(); }

    // Set the database path (for testing)
    void setDbPath(const std::string& path) {
        dbPath = path;
        // Reset state so next operation re-opens with new path
        if (ltmDb) {
            sqlite3_close(ltmDb);
            ltmDb = nullptr;
        }
        dbInitialized = false;
    }
};

#endif // MK_BRAIN_MEMORY_CPP
