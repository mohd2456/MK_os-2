#ifndef MK_NETWORK_STACK_CPP
#define MK_NETWORK_STACK_CPP

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <deque>
#include <memory>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <functional>
#include <cstdint>

namespace MK_Net {

// ─── Enumerations ───────────────────────────────────────────────────────────

enum class Protocol { TCP, UDP };
enum class SocketState { CLOSED, LISTENING, CONNECTING, CONNECTED, ERROR };
enum class FirewallAction { ALLOW, BLOCK };

// ─── DNS Cache ──────────────────────────────────────────────────────────────

struct DNSEntry {
    std::string domain;
    std::string ip_address;
    time_t cached_at;
    uint32_t ttl_seconds;
};

class DNSCache {
private:
    std::unordered_map<std::string, DNSEntry> cache_;
    size_t max_entries_ = 1024;

public:
    bool resolve(const std::string& domain, std::string& ip_out) {
        auto it = cache_.find(domain);
        if (it == cache_.end()) return false;
        time_t now = std::time(nullptr);
        if (now - it->second.cached_at > it->second.ttl_seconds) {
            cache_.erase(it);
            return false;
        }
        ip_out = it->second.ip_address;
        return true;
    }

    void store(const std::string& domain, const std::string& ip, uint32_t ttl = 300) {
        if (cache_.size() >= max_entries_) {
            // Evict oldest entry
            auto oldest = cache_.begin();
            for (auto it = cache_.begin(); it != cache_.end(); ++it) {
                if (it->second.cached_at < oldest->second.cached_at)
                    oldest = it;
            }
            cache_.erase(oldest);
        }
        DNSEntry entry;
        entry.domain = domain;
        entry.ip_address = ip;
        entry.cached_at = std::time(nullptr);
        entry.ttl_seconds = ttl;
        cache_[domain] = entry;
    }

    void flush() { cache_.clear(); }
    size_t size() const { return cache_.size(); }
};


// ─── Firewall ───────────────────────────────────────────────────────────────

struct FirewallRule {
    std::string description;
    std::string domain;       // empty = match all
    uint16_t port = 0;        // 0 = match all
    Protocol protocol = Protocol::TCP;
    FirewallAction action = FirewallAction::ALLOW;
    int priority = 0;         // higher = checked first
};

class Firewall {
private:
    std::vector<FirewallRule> rules_;

public:
    void add_rule(const FirewallRule& rule) {
        rules_.push_back(rule);
        std::sort(rules_.begin(), rules_.end(),
            [](const FirewallRule& a, const FirewallRule& b) {
                return a.priority > b.priority;
            });
    }

    bool remove_rule(const std::string& description) {
        auto it = std::remove_if(rules_.begin(), rules_.end(),
            [&description](const FirewallRule& r) { return r.description == description; });
        if (it == rules_.end()) return false;
        rules_.erase(it, rules_.end());
        return true;
    }

    FirewallAction check(const std::string& domain, uint16_t port, Protocol proto) const {
        for (const auto& rule : rules_) {
            bool domain_match = rule.domain.empty() || rule.domain == domain;
            bool port_match = rule.port == 0 || rule.port == port;
            bool proto_match = rule.protocol == proto;
            if (domain_match && port_match && proto_match) {
                return rule.action;
            }
        }
        return FirewallAction::ALLOW; // default allow
    }

    std::vector<FirewallRule> get_rules() const { return rules_; }
    void clear_rules() { rules_.clear(); }
};


// ─── Socket Abstraction ─────────────────────────────────────────────────────

struct Socket {
    uint32_t id;
    Protocol protocol;
    SocketState state = SocketState::CLOSED;
    std::string local_address = "0.0.0.0";
    uint16_t local_port = 0;
    std::string remote_address;
    uint16_t remote_port = 0;
    time_t created_at;
    time_t last_activity;
    size_t bytes_sent = 0;
    size_t bytes_received = 0;
    std::deque<std::string> send_buffer;
    std::deque<std::string> recv_buffer;
    int retries = 0;
    int max_retries = 3;
    bool auto_reconnect = true;
};

// ─── Connection Pool ────────────────────────────────────────────────────────

struct PooledConnection {
    std::shared_ptr<Socket> socket;
    std::string host;
    uint16_t port;
    time_t last_used;
    bool in_use = false;
};

class ConnectionPool {
private:
    std::vector<PooledConnection> pool_;
    size_t max_per_host_ = 6;
    size_t max_total_ = 64;
    uint32_t idle_timeout_ = 120; // seconds

public:
    std::shared_ptr<Socket> acquire(const std::string& host, uint16_t port) {
        // Clean up expired connections
        evict_idle();
        // Look for existing idle connection
        for (auto& conn : pool_) {
            if (conn.host == host && conn.port == port && !conn.in_use) {
                conn.in_use = true;
                conn.last_used = std::time(nullptr);
                return conn.socket;
            }
        }
        return nullptr; // caller needs to create new
    }

    void release(const std::shared_ptr<Socket>& socket) {
        for (auto& conn : pool_) {
            if (conn.socket == socket) {
                conn.in_use = false;
                conn.last_used = std::time(nullptr);
                return;
            }
        }
    }

    bool add(const std::string& host, uint16_t port, std::shared_ptr<Socket> socket) {
        if (pool_.size() >= max_total_) evict_idle();
        if (pool_.size() >= max_total_) return false;
        // Check per-host limit
        size_t host_count = 0;
        for (const auto& c : pool_) {
            if (c.host == host && c.port == port) host_count++;
        }
        if (host_count >= max_per_host_) return false;

        PooledConnection conn;
        conn.socket = socket;
        conn.host = host;
        conn.port = port;
        conn.last_used = std::time(nullptr);
        conn.in_use = true;
        pool_.push_back(conn);
        return true;
    }

    void evict_idle() {
        time_t now = std::time(nullptr);
        pool_.erase(std::remove_if(pool_.begin(), pool_.end(),
            [this, now](const PooledConnection& c) {
                return !c.in_use && (now - c.last_used > idle_timeout_);
            }), pool_.end());
    }

    size_t active_connections() const {
        size_t count = 0;
        for (const auto& c : pool_) { if (c.in_use) count++; }
        return count;
    }

    size_t total_connections() const { return pool_.size(); }
};


// ─── Bandwidth Monitor ──────────────────────────────────────────────────────

struct BandwidthSample {
    time_t timestamp;
    size_t bytes_in;
    size_t bytes_out;
};

class BandwidthMonitor {
private:
    std::deque<BandwidthSample> samples_;
    size_t total_bytes_in_ = 0;
    size_t total_bytes_out_ = 0;
    size_t window_seconds_ = 60;

public:
    void record_incoming(size_t bytes) {
        total_bytes_in_ += bytes;
        add_sample(bytes, 0);
    }

    void record_outgoing(size_t bytes) {
        total_bytes_out_ += bytes;
        add_sample(0, bytes);
    }

    double get_incoming_rate() const { // bytes per second
        return calculate_rate(true);
    }

    double get_outgoing_rate() const {
        return calculate_rate(false);
    }

    size_t total_incoming() const { return total_bytes_in_; }
    size_t total_outgoing() const { return total_bytes_out_; }

private:
    void add_sample(size_t in, size_t out) {
        BandwidthSample s;
        s.timestamp = std::time(nullptr);
        s.bytes_in = in;
        s.bytes_out = out;
        samples_.push_back(s);
        prune();
    }

    void prune() {
        time_t cutoff = std::time(nullptr) - window_seconds_;
        while (!samples_.empty() && samples_.front().timestamp < cutoff) {
            samples_.pop_front();
        }
    }

    double calculate_rate(bool incoming) const {
        if (samples_.size() < 2) return 0.0;
        time_t span = samples_.back().timestamp - samples_.front().timestamp;
        if (span <= 0) return 0.0;
        size_t total = 0;
        for (const auto& s : samples_) {
            total += incoming ? s.bytes_in : s.bytes_out;
        }
        return static_cast<double>(total) / span;
    }
};

// ─── Network Stats ──────────────────────────────────────────────────────────

struct NetworkStats {
    size_t total_bytes_sent = 0;
    size_t total_bytes_received = 0;
    size_t active_connections = 0;
    size_t total_connections_made = 0;
    size_t failed_connections = 0;
    size_t dns_cache_hits = 0;
    size_t dns_cache_misses = 0;
    size_t firewall_blocks = 0;
    double current_upload_rate = 0.0;
    double current_download_rate = 0.0;
};


// ─── Network Stack ──────────────────────────────────────────────────────────

class NetworkStack {
private:
    DNSCache dns_cache_;
    Firewall firewall_;
    ConnectionPool conn_pool_;
    BandwidthMonitor bandwidth_;
    std::map<uint32_t, std::shared_ptr<Socket>> sockets_;
    uint32_t next_socket_id_ = 1;
    NetworkStats stats_;
    bool network_available_ = true;

public:
    NetworkStack() = default;

    // ── Socket Operations ──

    uint32_t create_socket(Protocol proto) {
        auto sock = std::make_shared<Socket>();
        sock->id = next_socket_id_++;
        sock->protocol = proto;
        sock->state = SocketState::CLOSED;
        sock->created_at = std::time(nullptr);
        sock->last_activity = std::time(nullptr);
        sockets_[sock->id] = sock;
        return sock->id;
    }

    bool connect(uint32_t socket_id, const std::string& host, uint16_t port) {
        auto it = sockets_.find(socket_id);
        if (it == sockets_.end()) return false;
        auto& sock = it->second;

        // Firewall check
        if (firewall_.check(host, port, sock->protocol) == FirewallAction::BLOCK) {
            stats_.firewall_blocks++;
            sock->state = SocketState::ERROR;
            return false;
        }

        // DNS resolution
        std::string ip;
        if (!dns_cache_.resolve(host, ip)) {
            stats_.dns_cache_misses++;
            ip = simulate_dns_resolve(host);
            dns_cache_.store(host, ip);
        } else {
            stats_.dns_cache_hits++;
        }

        // Try pooled connection
        auto pooled = conn_pool_.acquire(host, port);
        if (pooled) {
            sockets_[socket_id] = pooled;
            pooled->id = socket_id;
            return true;
        }

        // Connect
        sock->remote_address = ip;
        sock->remote_port = port;
        sock->state = SocketState::CONNECTING;
        sock->last_activity = std::time(nullptr);

        // Simulate connection success
        sock->state = SocketState::CONNECTED;
        stats_.total_connections_made++;

        // Add to pool
        conn_pool_.add(host, port, sock);
        return true;
    }

    bool listen(uint32_t socket_id, uint16_t port) {
        auto it = sockets_.find(socket_id);
        if (it == sockets_.end()) return false;
        it->second->local_port = port;
        it->second->state = SocketState::LISTENING;
        return true;
    }

    ssize_t send(uint32_t socket_id, const std::string& data) {
        auto it = sockets_.find(socket_id);
        if (it == sockets_.end()) return -1;
        auto& sock = it->second;
        if (sock->state != SocketState::CONNECTED) return -1;

        sock->bytes_sent += data.size();
        sock->last_activity = std::time(nullptr);
        stats_.total_bytes_sent += data.size();
        bandwidth_.record_outgoing(data.size());
        return static_cast<ssize_t>(data.size());
    }

    std::string receive(uint32_t socket_id, size_t max_bytes = 4096) {
        auto it = sockets_.find(socket_id);
        if (it == sockets_.end()) return "";
        auto& sock = it->second;
        if (sock->state != SocketState::CONNECTED) return "";

        // Simulate receiving from buffer
        if (sock->recv_buffer.empty()) return "";
        std::string data = sock->recv_buffer.front();
        sock->recv_buffer.pop_front();
        if (data.size() > max_bytes) data.resize(max_bytes);

        sock->bytes_received += data.size();
        sock->last_activity = std::time(nullptr);
        stats_.total_bytes_received += data.size();
        bandwidth_.record_incoming(data.size());
        return data;
    }

    bool close_socket(uint32_t socket_id) {
        auto it = sockets_.find(socket_id);
        if (it == sockets_.end()) return false;
        auto& sock = it->second;
        conn_pool_.release(sock);
        sock->state = SocketState::CLOSED;
        sockets_.erase(it);
        return true;
    }

    // ── Auto-Reconnect ──

    bool reconnect(uint32_t socket_id) {
        auto it = sockets_.find(socket_id);
        if (it == sockets_.end()) return false;
        auto& sock = it->second;
        if (!sock->auto_reconnect) return false;
        if (sock->retries >= sock->max_retries) {
            sock->state = SocketState::ERROR;
            stats_.failed_connections++;
            return false;
        }
        sock->retries++;
        sock->state = SocketState::CONNECTING;
        // Simulate reconnection
        sock->state = SocketState::CONNECTED;
        sock->last_activity = std::time(nullptr);
        return true;
    }

    // ── DNS ──

    std::string resolve_dns(const std::string& domain) {
        std::string ip;
        if (dns_cache_.resolve(domain, ip)) {
            stats_.dns_cache_hits++;
            return ip;
        }
        stats_.dns_cache_misses++;
        ip = simulate_dns_resolve(domain);
        dns_cache_.store(domain, ip);
        return ip;
    }

    // ── Firewall ──

    void add_firewall_rule(const FirewallRule& rule) {
        firewall_.add_rule(rule);
    }

    bool remove_firewall_rule(const std::string& description) {
        return firewall_.remove_rule(description);
    }

    std::vector<FirewallRule> get_firewall_rules() const {
        return firewall_.get_rules();
    }

    // ── Stats ──

    NetworkStats get_stats() {
        stats_.active_connections = conn_pool_.active_connections();
        stats_.current_upload_rate = bandwidth_.get_outgoing_rate();
        stats_.current_download_rate = bandwidth_.get_incoming_rate();
        return stats_;
    }

    // ── Network Availability ──

    void set_network_available(bool available) { network_available_ = available; }
    bool is_network_available() const { return network_available_; }

private:
    std::string simulate_dns_resolve(const std::string& domain) {
        // Simple hash-based fake IP for simulation
        uint32_t hash = 0;
        for (char c : domain) hash = hash * 31 + c;
        return std::to_string((hash >> 24) & 0xFF) + "." +
               std::to_string((hash >> 16) & 0xFF) + "." +
               std::to_string((hash >> 8) & 0xFF) + "." +
               std::to_string(hash & 0xFF);
    }
};

} // namespace MK_Net

#endif // MK_NETWORK_STACK_CPP
