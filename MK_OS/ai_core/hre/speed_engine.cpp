#ifndef MK_SPEED_ENGINE_CPP
#define MK_SPEED_ENGINE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cmath>
#include <fstream>
#include <sstream>
#include <deque>
#include <functional>

// ===================================================================================
// MK SPEED ENGINE — Making Graph Reasoning FAST
// ===================================================================================
// The #1 criticism of graph reasoning: "it's slow compared to neural networks."
// This engine DESTROYS that argument.
//
// How LLMs work: throw 175B parameters at every query. Slow, expensive.
// How MK works: smart caching + indexing + bloom filters = instant answers.
//
// KEY OPTIMIZATIONS:
//   1. Bloom Filter — O(1) "does this concept exist?" (no graph scan)
//   2. Hot Cache — Top 1000 facts in a flat array (cache-line friendly)
//   3. Pre-computed Answers — Common questions cached with TTL
//   4. Index Acceleration — By first letter, relation type, domain
//   5. Batch Processing — Multiple lookups prepared and executed together
//   6. Response Memoization — Same question → instant cached answer
//   7. Lazy Loading — Load knowledge domains on-demand
//   8. String Interning — Common strings stored once, referenced by ID
//
// Target: Sub-millisecond response for cached queries on a 2010 MacBook.
// ===================================================================================


// Forward declarations
class MKPatternGraph;

// ─────────────────────────────────────────────────────────────────────────────────
//  DATA STRUCTURES
// ─────────────────────────────────────────────────────────────────────────────────

// A cached fact in the hot cache (flat array for cache-line performance)
struct HotFact {
    char source[64];        // Fixed-size for array layout (no heap alloc)
    char relation[32];      // Relations are short strings
    char target[64];        // Target concept
    float weight;           // Confidence weight
    int accessCount;        // LRU tracking — how often accessed
    long lastAccessed;      // Timestamp of last access (for LRU eviction)
};

// A pre-computed answer cache entry
struct CachedAnswer {
    std::string question;       // The normalized query
    std::string answer;         // The full answer text
    float confidence;           // How confident the answer is
    long computedAt;            // When this was computed (for TTL)
    int ttlSeconds;             // Time-to-live before expiry
    int hitCount;               // How many times this was served
    bool valid;                 // Set to false when underlying facts change
};

// A memoized response (full response for repeated exact queries)
struct MemoizedResponse {
    std::string queryText;      // Exact user query
    std::string fullResponse;   // Complete response text
    long createdAt;             // Timestamp
    int ttlSeconds;             // Expiry
    int hits;                   // Times served
};


// Benchmark result for a single operation
struct BenchmarkResult {
    std::string operation;
    double microseconds;
    bool cacheHit;
    std::string details;
};

// Lazy-loaded domain info
struct DomainInfo {
    std::string domainName;     // e.g., "animals", "technology", "geography"
    std::string filename;       // .mk file for this domain
    bool loaded;                // Has this domain been loaded into graph?
    int factCount;              // How many facts in this domain
    long lastAccessed;          // When was this domain last queried
};

// Interned string entry
struct InternedString {
    int id;                     // Unique ID for this string
    std::string value;          // The actual string
    int refCount;               // How many facts reference this
};

// ─────────────────────────────────────────────────────────────────────────────────
//  THE SPEED ENGINE CLASS
// ─────────────────────────────────────────────────────────────────────────────────

class MKSpeedEngine {
private:
    // ═══════════════════════════════════════════════════════════════════
    //  BLOOM FILTER — O(1) existence check
    // ═══════════════════════════════════════════════════════════════════
    // 64KB = 524,288 bits. With 3 hash functions and 50K concepts,
    // false positive rate is ~1.5%. Acceptable for pre-filtering.
    
    static const int BLOOM_SIZE_BYTES = 65536;  // 64KB
    static const int BLOOM_SIZE_BITS = BLOOM_SIZE_BYTES * 8;
    static const int BLOOM_HASH_COUNT = 3;      // Number of hash functions
    
    unsigned char bloomFilter[BLOOM_SIZE_BYTES]; // The actual bit array
    int bloomInsertions;                          // How many items added


    // Hash functions for bloom filter (different seeds = different hashes)
    unsigned int bloomHash1(const std::string& key) {
        unsigned int hash = 5381;
        for (char c : key) hash = ((hash << 5) + hash) + c;
        return hash % BLOOM_SIZE_BITS;
    }
    
    unsigned int bloomHash2(const std::string& key) {
        unsigned int hash = 0;
        for (char c : key) hash = hash * 31 + c;
        return hash % BLOOM_SIZE_BITS;
    }
    
    unsigned int bloomHash3(const std::string& key) {
        unsigned int hash = 2166136261u;
        for (char c : key) { hash ^= c; hash *= 16777619u; }
        return hash % BLOOM_SIZE_BITS;
    }
    
    // Set a bit in the bloom filter
    void bloomSetBit(unsigned int pos) {
        bloomFilter[pos / 8] |= (1 << (pos % 8));
    }
    
    // Check a bit in the bloom filter
    bool bloomCheckBit(unsigned int pos) {
        return (bloomFilter[pos / 8] & (1 << (pos % 8))) != 0;
    }

    // ═══════════════════════════════════════════════════════════════════
    //  HOT CACHE — Top 1000 most accessed facts in flat array
    // ═══════════════════════════════════════════════════════════════════
    
    static const int HOT_CACHE_SIZE = 1000;
    HotFact hotCache[HOT_CACHE_SIZE];
    int hotCacheCount;          // Current number of items in cache
    int hotCacheEvictions;      // How many times we evicted


    // ═══════════════════════════════════════════════════════════════════
    //  PRE-COMPUTED ANSWER CACHE
    // ═══════════════════════════════════════════════════════════════════
    
    std::unordered_map<std::string, CachedAnswer> answerCache;
    int answerCacheHits;
    int answerCacheMisses;
    int defaultTTL;             // Default time-to-live in seconds (300 = 5 min)
    
    // ═══════════════════════════════════════════════════════════════════
    //  INDEX ACCELERATION
    // ═══════════════════════════════════════════════════════════════════
    
    // Index by first letter of source concept → list of source concepts
    std::unordered_map<char, std::vector<std::string>> letterIndex;
    
    // Index by relation type → list of source concepts that have this relation
    std::unordered_map<std::string, std::vector<std::string>> relationTypeIndex;
    
    // Index by domain → list of concepts in that domain
    std::unordered_map<std::string, std::vector<std::string>> domainIndex;
    
    // ═══════════════════════════════════════════════════════════════════
    //  RESPONSE MEMOIZATION
    // ═══════════════════════════════════════════════════════════════════
    
    std::unordered_map<std::string, MemoizedResponse> memoCache;
    int memoCacheHits;
    int memoCacheMisses;
    int memoTTL;                // Memo TTL in seconds (600 = 10 min)
    static const int MAX_MEMO_SIZE = 500;  // Max memoized responses


    // ═══════════════════════════════════════════════════════════════════
    //  LAZY LOADING
    // ═══════════════════════════════════════════════════════════════════
    
    std::unordered_map<std::string, DomainInfo> domains;
    int domainsLoaded;
    
    // ═══════════════════════════════════════════════════════════════════
    //  STRING INTERNING — Store common strings once
    // ═══════════════════════════════════════════════════════════════════
    
    std::unordered_map<std::string, int> stringToId;    // string → intern ID
    std::vector<std::string> idToString;                 // intern ID → string
    int nextInternId;
    
    // ═══════════════════════════════════════════════════════════════════
    //  BENCHMARKING
    // ═══════════════════════════════════════════════════════════════════
    
    std::deque<BenchmarkResult> recentBenchmarks;
    int maxBenchmarkHistory;
    double totalQueryTimeUs;    // Total microseconds spent on queries
    int totalQueries;
    double avgQueryTimeUs;      // Running average

    // ═══════════════════════════════════════════════════════════════════
    //  INTERNAL HELPERS
    // ═══════════════════════════════════════════════════════════════════

    // Get current time in microseconds (for benchmarking)
    long getCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    
    double getMicroseconds() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }


    // Normalize string for cache keys
    std::string normalize(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        size_t start = result.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = result.find_last_not_of(" \t\r\n");
        return result.substr(start, end - start + 1);
    }

    // Find the LRU (least recently used) slot in hot cache for eviction
    int findLRUSlot() {
        int lruIdx = 0;
        long oldestAccess = hotCache[0].lastAccessed;
        for (int i = 1; i < hotCacheCount; i++) {
            if (hotCache[i].lastAccessed < oldestAccess) {
                oldestAccess = hotCache[i].lastAccessed;
                lruIdx = i;
            }
        }
        return lruIdx;
    }

    // Record a benchmark measurement
    void recordBenchmark(const std::string& op, double microseconds, bool cacheHit,
                         const std::string& details = "") {
        BenchmarkResult br;
        br.operation = op;
        br.microseconds = microseconds;
        br.cacheHit = cacheHit;
        br.details = details;
        recentBenchmarks.push_back(br);
        if ((int)recentBenchmarks.size() > maxBenchmarkHistory) {
            recentBenchmarks.pop_front();
        }
        
        totalQueryTimeUs += microseconds;
        totalQueries++;
        avgQueryTimeUs = totalQueryTimeUs / totalQueries;
    }

public:
    // ─────────────────────────────────────────────────────────────────────────────
    //  CONSTRUCTOR
    // ─────────────────────────────────────────────────────────────────────────────

    MKSpeedEngine() 
        : bloomInsertions(0), hotCacheCount(0), hotCacheEvictions(0),
          answerCacheHits(0), answerCacheMisses(0), defaultTTL(300),
          memoCacheHits(0), memoCacheMisses(0), memoTTL(600),
          domainsLoaded(0), nextInternId(0), maxBenchmarkHistory(100),
          totalQueryTimeUs(0.0), totalQueries(0), avgQueryTimeUs(0.0) {
        
        // Zero out the bloom filter
        std::memset(bloomFilter, 0, BLOOM_SIZE_BYTES);
        // Zero out the hot cache
        std::memset(hotCache, 0, sizeof(hotCache));
        
        // Pre-intern common relation names (stored ONCE, referenced by ID)
        internString("is_a");
        internString("has");
        internString("can");
        internString("lives_in");
        internString("part_of");
        internString("type_of");
        internString("created");
        internString("uses");
        internString("needs");
        internString("likes");
        internString("opposite_of");
        internString("located_in");
        
        std::cout << "[SPEED ENGINE] Performance optimization layer initialized.\n";
        std::cout << "[SPEED ENGINE] Bloom filter: " << BLOOM_SIZE_BYTES 
                  << " bytes, Hot cache: " << HOT_CACHE_SIZE << " slots\n";
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  BLOOM FILTER API — O(1) concept existence check
    // ─────────────────────────────────────────────────────────────────────────────

    // Add a concept to the bloom filter (call when adding to graph)
    void bloomInsert(const std::string& concept) {
        std::string key = normalize(concept);
        bloomSetBit(bloomHash1(key));
        bloomSetBit(bloomHash2(key));
        bloomSetBit(bloomHash3(key));
        bloomInsertions++;
    }

    // Check if a concept MIGHT exist (false = definitely not, true = probably yes)
    // False positive rate ~1.5% with 50K items in 64KB filter
    bool bloomMightExist(const std::string& concept) {
        std::string key = normalize(concept);
        return bloomCheckBit(bloomHash1(key)) &&
               bloomCheckBit(bloomHash2(key)) &&
               bloomCheckBit(bloomHash3(key));
    }

    // Quick check before expensive graph traversal
    // Returns false if concept DEFINITELY doesn't exist (saves a graph scan)
    bool quickExistenceCheck(const std::string& concept) {
        double start = getMicroseconds();
        bool result = bloomMightExist(concept);
        double elapsed = getMicroseconds() - start;
        recordBenchmark("bloom_check", elapsed, false, concept);
        return result;
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  HOT CACHE API — Fast array of most-used facts
    // ─────────────────────────────────────────────────────────────────────────────

    // Add a fact to the hot cache (called when a fact is accessed)
    void hotCacheInsert(const std::string& source, const std::string& relation,
                        const std::string& target, float weight) {
        // Check if already in cache — just update access count
        for (int i = 0; i < hotCacheCount; i++) {
            if (std::strncmp(hotCache[i].source, source.c_str(), 63) == 0 &&
                std::strncmp(hotCache[i].relation, relation.c_str(), 31) == 0 &&
                std::strncmp(hotCache[i].target, target.c_str(), 63) == 0) {
                hotCache[i].accessCount++;
                hotCache[i].lastAccessed = getCurrentTimestamp();
                return;
            }
        }
        
        // Find a slot (or evict LRU)
        int slot;
        if (hotCacheCount < HOT_CACHE_SIZE) {
            slot = hotCacheCount++;
        } else {
            slot = findLRUSlot();
            hotCacheEvictions++;
        }
        
        // Copy into fixed-size buffers (safe truncation)
        std::strncpy(hotCache[slot].source, source.c_str(), 63);
        hotCache[slot].source[63] = '\0';
        std::strncpy(hotCache[slot].relation, relation.c_str(), 31);
        hotCache[slot].relation[31] = '\0';
        std::strncpy(hotCache[slot].target, target.c_str(), 63);
        hotCache[slot].target[63] = '\0';
        hotCache[slot].weight = weight;
        hotCache[slot].accessCount = 1;
        hotCache[slot].lastAccessed = getCurrentTimestamp();
    }

    // Query the hot cache — returns target if found, empty string if miss
    std::string hotCacheLookup(const std::string& source, const std::string& relation) {
        double start = getMicroseconds();
        std::string src = normalize(source);
        std::string rel = normalize(relation);
        
        for (int i = 0; i < hotCacheCount; i++) {
            if (std::strncmp(hotCache[i].source, src.c_str(), 63) == 0 &&
                std::strncmp(hotCache[i].relation, rel.c_str(), 31) == 0) {
                hotCache[i].accessCount++;
                hotCache[i].lastAccessed = getCurrentTimestamp();
                double elapsed = getMicroseconds() - start;
                recordBenchmark("hot_cache_hit", elapsed, true, src + " " + rel);
                return std::string(hotCache[i].target);
            }
        }
        
        double elapsed = getMicroseconds() - start;
        recordBenchmark("hot_cache_miss", elapsed, false, src + " " + rel);
        return "";  // Cache miss
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  PRE-COMPUTED ANSWER CACHE
    // ─────────────────────────────────────────────────────────────────────────────

    // Store a pre-computed answer for a common question
    void cacheAnswer(const std::string& question, const std::string& answer,
                     float confidence, int ttl = 0) {
        std::string key = normalize(question);
        CachedAnswer ca;
        ca.question = key;
        ca.answer = answer;
        ca.confidence = confidence;
        ca.computedAt = getCurrentTimestamp();
        ca.ttlSeconds = (ttl > 0) ? ttl : defaultTTL;
        ca.hitCount = 0;
        ca.valid = true;
        answerCache[key] = ca;
    }

    // Look up a pre-computed answer (returns empty string if miss or expired)
    std::string lookupAnswer(const std::string& question) {
        double start = getMicroseconds();
        std::string key = normalize(question);
        
        auto it = answerCache.find(key);
        if (it != answerCache.end() && it->second.valid) {
            // Check TTL
            long now = getCurrentTimestamp();
            if (now - it->second.computedAt < it->second.ttlSeconds) {
                it->second.hitCount++;
                answerCacheHits++;
                double elapsed = getMicroseconds() - start;
                recordBenchmark("answer_cache_hit", elapsed, true, key);
                return it->second.answer;
            } else {
                // Expired — invalidate
                it->second.valid = false;
            }
        }
        
        answerCacheMisses++;
        double elapsed = getMicroseconds() - start;
        recordBenchmark("answer_cache_miss", elapsed, false, key);
        return "";
    }

    // Invalidate cached answers that depend on a concept (call when facts change)
    void invalidateAnswersFor(const std::string& concept) {
        std::string key = normalize(concept);
        for (auto& pair : answerCache) {
            if (pair.second.question.find(key) != std::string::npos) {
                pair.second.valid = false;
            }
        }
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  RESPONSE MEMOIZATION — Exact query → exact response
    // ─────────────────────────────────────────────────────────────────────────────

    // Memoize a full response for an exact query
    void memoizeResponse(const std::string& queryText, const std::string& fullResponse) {
        if ((int)memoCache.size() >= MAX_MEMO_SIZE) {
            // Evict oldest entry
            long oldest = getCurrentTimestamp();
            std::string oldestKey;
            for (auto& pair : memoCache) {
                if (pair.second.createdAt < oldest) {
                    oldest = pair.second.createdAt;
                    oldestKey = pair.first;
                }
            }
            if (!oldestKey.empty()) memoCache.erase(oldestKey);
        }
        
        std::string key = normalize(queryText);
        MemoizedResponse mr;
        mr.queryText = key;
        mr.fullResponse = fullResponse;
        mr.createdAt = getCurrentTimestamp();
        mr.ttlSeconds = memoTTL;
        mr.hits = 0;
        memoCache[key] = mr;
    }

    // Check if we have a memoized response for this exact query
    std::string getMemoizedResponse(const std::string& queryText) {
        double start = getMicroseconds();
        std::string key = normalize(queryText);
        
        auto it = memoCache.find(key);
        if (it != memoCache.end()) {
            long now = getCurrentTimestamp();
            if (now - it->second.createdAt < it->second.ttlSeconds) {
                it->second.hits++;
                memoCacheHits++;
                double elapsed = getMicroseconds() - start;
                recordBenchmark("memo_hit", elapsed, true, key);
                return it->second.fullResponse;
            } else {
                memoCache.erase(it);  // Expired
            }
        }
        
        memoCacheMisses++;
        double elapsed = getMicroseconds() - start;
        recordBenchmark("memo_miss", elapsed, false, key);
        return "";
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  INDEX ACCELERATION
    // ─────────────────────────────────────────────────────────────────────────────

    // Register a concept in all relevant indexes
    void indexConcept(const std::string& concept, const std::string& relation,
                      const std::string& domain = "general") {
        std::string norm = normalize(concept);
        if (norm.empty()) return;
        
        // Letter index
        letterIndex[norm[0]].push_back(norm);
        
        // Relation type index
        std::string rel = normalize(relation);
        relationTypeIndex[rel].push_back(norm);
        
        // Domain index
        std::string dom = normalize(domain);
        domainIndex[dom].push_back(norm);
    }

    // Fast lookup by first letter (narrows search space before graph query)
    std::vector<std::string> getConceptsByLetter(char letter) {
        char lower = std::tolower(letter);
        auto it = letterIndex.find(lower);
        if (it != letterIndex.end()) return it->second;
        return {};
    }

    // Get all concepts that have a specific relation
    std::vector<std::string> getConceptsByRelation(const std::string& relation) {
        std::string rel = normalize(relation);
        auto it = relationTypeIndex.find(rel);
        if (it != relationTypeIndex.end()) return it->second;
        return {};
    }

    // Get all concepts in a domain
    std::vector<std::string> getConceptsByDomain(const std::string& domain) {
        std::string dom = normalize(domain);
        auto it = domainIndex.find(dom);
        if (it != domainIndex.end()) return it->second;
        return {};
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  BATCH PROCESSING — Prepare multiple lookups, execute together
    // ─────────────────────────────────────────────────────────────────────────────

    struct BatchQuery {
        std::string source;
        std::string relation;
        std::string result;     // Filled after execution
        bool resolved;
    };

    // Execute a batch of queries — check caches first, then graph for remainder
    void executeBatch(std::vector<BatchQuery>& queries) {
        double start = getMicroseconds();
        int cacheHits = 0;
        
        for (auto& q : queries) {
            q.resolved = false;
            
            // Try hot cache first
            std::string cached = hotCacheLookup(q.source, q.relation);
            if (!cached.empty()) {
                q.result = cached;
                q.resolved = true;
                cacheHits++;
            }
        }
        
        double elapsed = getMicroseconds() - start;
        recordBenchmark("batch_execute", elapsed, cacheHits > 0,
                       "batch_size=" + std::to_string(queries.size()) + 
                       " cache_hits=" + std::to_string(cacheHits));
        
        // Unresolved queries need to go to the full graph
        // (caller handles this — we just pre-filtered with caches)
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  LAZY LOADING — Load knowledge domains on-demand
    // ─────────────────────────────────────────────────────────────────────────────

    // Register a domain (call at startup — just records existence, doesn't load)
    void registerDomain(const std::string& domainName, const std::string& filename,
                        int estimatedFacts) {
        DomainInfo di;
        di.domainName = normalize(domainName);
        di.filename = filename;
        di.loaded = false;
        di.factCount = estimatedFacts;
        di.lastAccessed = 0;
        domains[di.domainName] = di;
    }

    // Check if a domain needs to be loaded (returns filename if yes, empty if already loaded)
    std::string checkDomainNeeded(const std::string& concept) {
        // Look through domains to see if this concept belongs to an unloaded domain
        for (auto& pair : domains) {
            if (!pair.second.loaded) {
                // Heuristic: check if concept name suggests this domain
                // (in practice, you'd have a concept-to-domain index)
                if (concept.find(pair.first) != std::string::npos ||
                    pair.first.find(concept) != std::string::npos) {
                    return pair.second.filename;
                }
            }
        }
        return "";
    }

    // Mark a domain as loaded
    void markDomainLoaded(const std::string& domainName) {
        std::string name = normalize(domainName);
        auto it = domains.find(name);
        if (it != domains.end()) {
            it->second.loaded = true;
            it->second.lastAccessed = getCurrentTimestamp();
            domainsLoaded++;
        }
    }

    // Check if a domain is loaded
    bool isDomainLoaded(const std::string& domainName) {
        std::string name = normalize(domainName);
        auto it = domains.find(name);
        return (it != domains.end() && it->second.loaded);
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  STRING INTERNING — Store common strings once, reference by ID
    // ─────────────────────────────────────────────────────────────────────────────

    // Intern a string (returns its unique ID, stores only one copy)
    int internString(const std::string& s) {
        auto it = stringToId.find(s);
        if (it != stringToId.end()) return it->second;
        
        int id = nextInternId++;
        stringToId[s] = id;
        idToString.push_back(s);
        return id;
    }

    // Get a string by its intern ID
    std::string getInternedString(int id) {
        if (id >= 0 && id < (int)idToString.size()) return idToString[id];
        return "";
    }

    // Get the intern ID for a string (returns -1 if not interned)
    int getInternId(const std::string& s) {
        auto it = stringToId.find(s);
        return (it != stringToId.end()) ? it->second : -1;
    }

    // Get memory saved by interning
    size_t getInterningSavedBytes() {
        size_t saved = 0;
        for (const auto& pair : stringToId) {
            // Each reference beyond the first saves the string length
            // (In practice: relation names appear thousands of times)
            saved += pair.first.size() * 100;  // Estimate 100 uses per interned string
        }
        return saved;
    }


    // ─────────────────────────────────────────────────────────────────────────────
    //  BENCHMARKING & STATS
    // ─────────────────────────────────────────────────────────────────────────────

    // Get average query latency in microseconds
    double getAvgLatencyUs() const { return avgQueryTimeUs; }
    
    // Get total queries processed
    int getTotalQueries() const { return totalQueries; }
    
    // Get cache hit rates
    float getAnswerCacheHitRate() const {
        int total = answerCacheHits + answerCacheMisses;
        return (total > 0) ? (float)answerCacheHits / total : 0.0f;
    }
    
    float getMemoCacheHitRate() const {
        int total = memoCacheHits + memoCacheMisses;
        return (total > 0) ? (float)memoCacheHits / total : 0.0f;
    }

    // Full performance report
    void printStats() const {
        std::cout << "\n--- [SPEED ENGINE] ---\n";
        std::cout << "  Bloom filter: " << bloomInsertions << " insertions, " 
                  << BLOOM_SIZE_BYTES << " bytes\n";
        std::cout << "  Hot cache: " << hotCacheCount << "/" << HOT_CACHE_SIZE 
                  << " slots, " << hotCacheEvictions << " evictions\n";
        std::cout << "  Answer cache: " << answerCache.size() << " entries, "
                  << answerCacheHits << " hits / " << answerCacheMisses << " misses ("
                  << (int)(getAnswerCacheHitRate() * 100) << "% hit rate)\n";
        std::cout << "  Memo cache: " << memoCache.size() << " entries, "
                  << memoCacheHits << " hits / " << memoCacheMisses << " misses ("
                  << (int)(getMemoCacheHitRate() * 100) << "% hit rate)\n";
        std::cout << "  Interned strings: " << stringToId.size() << " (saving ~" 
                  << getInterningSavedBytes() / 1024 << " KB)\n";
        std::cout << "  Domains: " << domainsLoaded << "/" << domains.size() << " loaded\n";
        std::cout << "  Total queries: " << totalQueries << "\n";
        std::cout << "  Avg latency: " << avgQueryTimeUs << " μs\n";
        std::cout << "───────────────────────\n";
    }

    // Print recent benchmark history
    void printBenchmarks(int count = 10) const {
        std::cout << "\n=== RECENT BENCHMARKS ===\n";
        int start = std::max(0, (int)recentBenchmarks.size() - count);
        for (int i = start; i < (int)recentBenchmarks.size(); i++) {
            const auto& b = recentBenchmarks[i];
            std::cout << "  " << b.operation << ": " << b.microseconds << " μs"
                      << (b.cacheHit ? " [HIT]" : " [MISS]")
                      << " — " << b.details << "\n";
        }
        std::cout << "=========================\n";
    }
};

#endif // MK_SPEED_ENGINE_CPP
