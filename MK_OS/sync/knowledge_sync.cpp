#ifndef MK_KNOWLEDGE_SYNC_CPP
#define MK_KNOWLEDGE_SYNC_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <queue>

// ===================================================================================
// MK KNOWLEDGE SYNC
// Synchronizes learned knowledge across all MK instances.
// When MK learns something on phone (via Telegram), it queues the fact for sync.
// When main PC comes online, facts sync via HTTP/SSH.
// Implements eventual consistency with timestamps and conflict resolution
// (latest wins, user corrections always win).
// ===================================================================================

enum class MKSyncPriority {
    LOW,        // Background knowledge
    NORMAL,     // Standard facts
    HIGH,       // Important learned facts
    CRITICAL    // User corrections (always win)
};

enum class MKSyncStatus {
    QUEUED,
    SYNCING,
    SYNCED,
    CONFLICT,
    FAILED
};

struct MKSyncFact {
    std::string source;
    std::string relation;
    std::string target;
    float weight;
    std::time_t learned_at;
    std::time_t synced_at;
    std::string origin_device;
    MKSyncPriority priority;
    MKSyncStatus status;
    bool user_correction;   // User corrections always win conflicts
    int sync_attempts;
    std::string fact_id;    // Unique identifier for dedup
};

struct MKSyncConflict {
    MKSyncFact local_fact;
    MKSyncFact remote_fact;
    std::string resolution;  // "local_wins", "remote_wins", "user_correction"
    std::time_t resolved_at;
};

struct MKSyncState {
    std::string device_id;
    std::time_t last_sync;
    int facts_sent;
    int facts_received;
    int conflicts_resolved;
    bool connected;
};

class MKKnowledgeSync {
private:
    std::vector<MKSyncFact> sync_queue;
    std::vector<MKSyncFact> synced_facts;
    std::vector<MKSyncConflict> conflicts;
    std::map<std::string, MKSyncState> peer_states;
    std::string local_device_id;
    int max_queue_size;
    int total_synced;
    int total_conflicts;
    std::string persistence_path;

    // Generate a unique fact ID
    std::string generateFactId(const std::string& source, const std::string& relation,
                              const std::string& target) const {
        return source + "|" + relation + "|" + target;
    }

    // Resolve a conflict between local and remote facts
    std::string resolveConflict(const MKSyncFact& local, const MKSyncFact& remote) {
        // Rule 1: User corrections ALWAYS win
        if (local.user_correction && !remote.user_correction) return "local_wins";
        if (remote.user_correction && !local.user_correction) return "remote_wins";
        if (local.user_correction && remote.user_correction) {
            // Both are user corrections - latest wins
            return (local.learned_at >= remote.learned_at) ? "local_wins" : "remote_wins";
        }

        // Rule 2: Higher priority wins
        if (local.priority > remote.priority) return "local_wins";
        if (remote.priority > local.priority) return "remote_wins";

        // Rule 3: Latest timestamp wins (eventual consistency)
        return (local.learned_at >= remote.learned_at) ? "local_wins" : "remote_wins";
    }

public:
    MKKnowledgeSync() : local_device_id("local_mk"), max_queue_size(10000),
                        total_synced(0), total_conflicts(0),
                        persistence_path("mk_sync_queue.dat") {}

    // Set the local device identifier
    void setDeviceId(const std::string& id) { local_device_id = id; }

    // Queue a fact for synchronization
    void queueFact(const std::string& source, const std::string& relation,
                   const std::string& target, float weight = 1.0f,
                   MKSyncPriority priority = MKSyncPriority::NORMAL,
                   bool user_correction = false) {
        MKSyncFact fact;
        fact.source = source;
        fact.relation = relation;
        fact.target = target;
        fact.weight = weight;
        fact.learned_at = std::time(nullptr);
        fact.synced_at = 0;
        fact.origin_device = local_device_id;
        fact.priority = priority;
        fact.status = MKSyncStatus::QUEUED;
        fact.user_correction = user_correction;
        fact.sync_attempts = 0;
        fact.fact_id = generateFactId(source, relation, target);

        // Check for duplicates in queue
        for (const auto& existing : sync_queue) {
            if (existing.fact_id == fact.fact_id) return;  // Already queued
        }

        sync_queue.push_back(fact);

        // Trim queue if too large (remove oldest non-critical items)
        if ((int)sync_queue.size() > max_queue_size) {
            auto it = std::remove_if(sync_queue.begin(), sync_queue.end(),
                [](const MKSyncFact& f) {
                    return f.priority == MKSyncPriority::LOW && f.sync_attempts > 3;
                });
            sync_queue.erase(it, sync_queue.end());
        }
    }

    // Receive a fact from a remote device
    bool receiveFact(const MKSyncFact& remote_fact) {
        // Check for conflicts with existing synced facts
        for (const auto& local : synced_facts) {
            if (local.fact_id == remote_fact.fact_id) {
                // Conflict detected
                std::string resolution = resolveConflict(local, remote_fact);

                MKSyncConflict conflict;
                conflict.local_fact = local;
                conflict.remote_fact = remote_fact;
                conflict.resolution = resolution;
                conflict.resolved_at = std::time(nullptr);
                conflicts.push_back(conflict);
                total_conflicts++;

                if (resolution == "remote_wins") {
                    // Replace local fact with remote
                    for (auto& f : synced_facts) {
                        if (f.fact_id == remote_fact.fact_id) {
                            f = remote_fact;
                            f.status = MKSyncStatus::SYNCED;
                            f.synced_at = std::time(nullptr);
                            break;
                        }
                    }
                }
                return true;
            }
        }

        // No conflict - just add it
        MKSyncFact fact = remote_fact;
        fact.status = MKSyncStatus::SYNCED;
        fact.synced_at = std::time(nullptr);
        synced_facts.push_back(fact);
        total_synced++;
        return true;
    }

    // Get facts that need to be sent to a peer
    std::vector<MKSyncFact> getFactsForPeer(const std::string& peer_id) const {
        std::vector<MKSyncFact> to_send;

        auto it = peer_states.find(peer_id);
        std::time_t last_sync = 0;
        if (it != peer_states.end()) {
            last_sync = it->second.last_sync;
        }

        for (const auto& fact : sync_queue) {
            if (fact.status == MKSyncStatus::QUEUED && fact.learned_at > last_sync) {
                to_send.push_back(fact);
            }
        }

        // Sort by priority (critical first)
        std::sort(to_send.begin(), to_send.end(),
                  [](const MKSyncFact& a, const MKSyncFact& b) {
                      return static_cast<int>(a.priority) > static_cast<int>(b.priority);
                  });

        return to_send;
    }

    // Mark facts as synced to a peer
    void markSynced(const std::string& peer_id, const std::vector<std::string>& fact_ids) {
        for (auto& fact : sync_queue) {
            for (const auto& id : fact_ids) {
                if (fact.fact_id == id) {
                    fact.status = MKSyncStatus::SYNCED;
                    fact.synced_at = std::time(nullptr);
                }
            }
        }

        // Update peer state
        peer_states[peer_id].last_sync = std::time(nullptr);
        peer_states[peer_id].facts_sent += static_cast<int>(fact_ids.size());
    }

    // Register a peer device
    void registerPeer(const std::string& peer_id) {
        if (peer_states.find(peer_id) == peer_states.end()) {
            MKSyncState state;
            state.device_id = peer_id;
            state.last_sync = 0;
            state.facts_sent = 0;
            state.facts_received = 0;
            state.conflicts_resolved = 0;
            state.connected = false;
            peer_states[peer_id] = state;
        }
    }

    // Update peer connection status
    void setPeerConnected(const std::string& peer_id, bool connected) {
        auto it = peer_states.find(peer_id);
        if (it != peer_states.end()) {
            it->second.connected = connected;
        }
    }

    // Get queue size
    int queueSize() const { return static_cast<int>(sync_queue.size()); }

    // Get synced count
    int syncedCount() const { return total_synced; }

    // Get conflict count
    int conflictCount() const { return total_conflicts; }

    // Get peer count
    int peerCount() const { return static_cast<int>(peer_states.size()); }

    // Get connected peers
    std::vector<std::string> getConnectedPeers() const {
        std::vector<std::string> connected;
        for (const auto& kv : peer_states) {
            if (kv.second.connected) connected.push_back(kv.first);
        }
        return connected;
    }

    // Serialize queue for transport (simple pipe-delimited format)
    std::string serializeQueue() const {
        std::ostringstream ss;
        for (const auto& fact : sync_queue) {
            if (fact.status != MKSyncStatus::QUEUED) continue;
            ss << fact.source << "|" << fact.relation << "|" << fact.target << "|"
               << fact.weight << "|" << fact.learned_at << "|" << fact.origin_device << "|"
               << static_cast<int>(fact.priority) << "|"
               << (fact.user_correction ? "1" : "0") << "\n";
        }
        return ss.str();
    }

    // Parse serialized facts from a remote device
    std::vector<MKSyncFact> deserializeFacts(const std::string& data) const {
        std::vector<MKSyncFact> facts;
        std::istringstream iss(data);
        std::string line;

        while (std::getline(iss, line)) {
            if (line.empty()) continue;
            std::istringstream fields(line);
            MKSyncFact fact;
            std::string val;

            if (!std::getline(fields, fact.source, '|')) continue;
            if (!std::getline(fields, fact.relation, '|')) continue;
            if (!std::getline(fields, fact.target, '|')) continue;
            if (std::getline(fields, val, '|')) { try { fact.weight = std::stof(val); } catch (...) { fact.weight = 1.0f; } }
            if (std::getline(fields, val, '|')) { try { fact.learned_at = static_cast<std::time_t>(std::stol(val)); } catch (...) { fact.learned_at = std::time(nullptr); } }
            if (std::getline(fields, fact.origin_device, '|')) {}
            if (std::getline(fields, val, '|')) { try { fact.priority = static_cast<MKSyncPriority>(std::stoi(val)); } catch (...) { fact.priority = MKSyncPriority::NORMAL; } }
            if (std::getline(fields, val, '|')) { fact.user_correction = (val == "1"); }

            fact.fact_id = generateFactId(fact.source, fact.relation, fact.target);
            fact.status = MKSyncStatus::QUEUED;
            fact.sync_attempts = 0;
            fact.synced_at = 0;
            facts.push_back(fact);
        }
        return facts;
    }

    // Save queue to disk
    void save() const {
        std::ofstream out(persistence_path);
        if (!out.is_open()) return;
        out << serializeQueue();
        out.close();
    }

    // Load queue from disk
    void load() {
        std::ifstream in(persistence_path);
        if (!in.is_open()) return;
        std::string content((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
        in.close();

        auto facts = deserializeFacts(content);
        for (auto& fact : facts) {
            sync_queue.push_back(fact);
        }
    }

    // Get summary
    std::string getSummary() const {
        std::ostringstream ss;
        ss << "=== Knowledge Sync ===\n";
        ss << "  Device: " << local_device_id << "\n";
        ss << "  Queue: " << sync_queue.size() << " facts waiting\n";
        ss << "  Synced: " << total_synced << " facts\n";
        ss << "  Conflicts: " << total_conflicts << " resolved\n";
        ss << "  Peers: " << peer_states.size() << "\n";
        auto connected = getConnectedPeers();
        ss << "  Connected peers: " << connected.size() << "\n";
        return ss.str();
    }
};

#endif // MK_KNOWLEDGE_SYNC_CPP
