#ifndef MK_PERMISSIONS_CPP
#define MK_PERMISSIONS_CPP

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <deque>
#include <memory>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <functional>

namespace MK_Security {

// ─── User Levels ────────────────────────────────────────────────────────────

enum class UserLevel {
    ROOT,
    SYSTEM,
    USER,
    PLUGIN,
    GUEST
};

// ─── Capabilities ───────────────────────────────────────────────────────────

enum class Capability {
    CAN_NETWORK,
    CAN_DISK_READ,
    CAN_DISK_WRITE,
    CAN_EXEC,
    CAN_MOUNT,
    CAN_KILL_PROCESS,
    CAN_MODIFY_USERS,
    CAN_INSTALL_PACKAGES,
    CAN_ACCESS_HARDWARE,
    CAN_MODIFY_FIREWALL,
    CAN_VIEW_AUDIT_LOG,
    CAN_ESCALATE
};


// ─── File Permission Flags ──────────────────────────────────────────────────

struct FileAccessFlags {
    bool read = false;
    bool write = false;
    bool execute = false;
};

struct FilePermissionSet {
    FileAccessFlags owner;
    FileAccessFlags group;
    FileAccessFlags other;
};

// ─── User/Group ─────────────────────────────────────────────────────────────

struct Group {
    std::string name;
    uint32_t gid;
    std::set<std::string> members;
};

struct User {
    std::string username;
    uint32_t uid;
    uint32_t primary_gid;
    UserLevel level;
    std::set<Capability> capabilities;
    std::set<std::string> groups;
    time_t created_at;
    bool locked = false;
};

// ─── Audit Log ──────────────────────────────────────────────────────────────

enum class AuditAction {
    FILE_READ,
    FILE_WRITE,
    FILE_EXEC,
    FILE_DELETE,
    PROCESS_START,
    PROCESS_KILL,
    NETWORK_CONNECT,
    PERMISSION_CHECK,
    PERMISSION_DENIED,
    PRIVILEGE_ESCALATE,
    PRIVILEGE_DROP,
    USER_CREATE,
    USER_DELETE,
    LOGIN,
    LOGOUT
};

struct AuditEntry {
    time_t timestamp;
    std::string username;
    uint32_t uid;
    AuditAction action;
    std::string resource;
    std::string details;
    bool success;
};

class AuditLog {
private:
    std::deque<AuditEntry> entries_;
    size_t max_entries_ = 50000;
    bool enabled_ = true;

public:
    void log(const std::string& username, uint32_t uid, AuditAction action,
             const std::string& resource, const std::string& details, bool success) {
        if (!enabled_) return;
        AuditEntry entry;
        entry.timestamp = std::time(nullptr);
        entry.username = username;
        entry.uid = uid;
        entry.action = action;
        entry.resource = resource;
        entry.details = details;
        entry.success = success;
        entries_.push_back(entry);
        if (entries_.size() > max_entries_) {
            entries_.pop_front();
        }
    }

    std::vector<AuditEntry> get_entries(size_t count = 100) const {
        std::vector<AuditEntry> result;
        size_t start = entries_.size() > count ? entries_.size() - count : 0;
        for (size_t i = start; i < entries_.size(); ++i) {
            result.push_back(entries_[i]);
        }
        return result;
    }

    std::vector<AuditEntry> get_entries_for_user(const std::string& username, size_t count = 100) const {
        std::vector<AuditEntry> result;
        for (auto it = entries_.rbegin(); it != entries_.rend() && result.size() < count; ++it) {
            if (it->username == username) result.push_back(*it);
        }
        std::reverse(result.begin(), result.end());
        return result;
    }

    std::vector<AuditEntry> get_denied_entries(size_t count = 100) const {
        std::vector<AuditEntry> result;
        for (auto it = entries_.rbegin(); it != entries_.rend() && result.size() < count; ++it) {
            if (!it->success) result.push_back(*it);
        }
        std::reverse(result.begin(), result.end());
        return result;
    }

    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }
    size_t size() const { return entries_.size(); }
    void clear() { entries_.clear(); }
};


// ─── Privilege Escalation ───────────────────────────────────────────────────

struct EscalationToken {
    uint64_t token_id;
    std::string username;
    UserLevel original_level;
    UserLevel elevated_level;
    std::set<Capability> granted_capabilities;
    time_t granted_at;
    uint32_t timeout_seconds;
    bool active = true;
};

// ─── Permission Manager ─────────────────────────────────────────────────────

class PermissionManager {
private:
    std::map<std::string, User> users_;
    std::map<std::string, Group> groups_;
    std::map<std::string, FilePermissionSet> file_permissions_; // path -> perms
    AuditLog audit_log_;
    std::vector<EscalationToken> escalation_tokens_;
    uint64_t next_token_id_ = 1;
    uint32_t next_uid_ = 1000;
    uint32_t next_gid_ = 1000;

    std::set<Capability> default_capabilities(UserLevel level) const {
        std::set<Capability> caps;
        switch (level) {
            case UserLevel::ROOT:
                caps = {Capability::CAN_NETWORK, Capability::CAN_DISK_READ,
                        Capability::CAN_DISK_WRITE, Capability::CAN_EXEC,
                        Capability::CAN_MOUNT, Capability::CAN_KILL_PROCESS,
                        Capability::CAN_MODIFY_USERS, Capability::CAN_INSTALL_PACKAGES,
                        Capability::CAN_ACCESS_HARDWARE, Capability::CAN_MODIFY_FIREWALL,
                        Capability::CAN_VIEW_AUDIT_LOG, Capability::CAN_ESCALATE};
                break;
            case UserLevel::SYSTEM:
                caps = {Capability::CAN_NETWORK, Capability::CAN_DISK_READ,
                        Capability::CAN_DISK_WRITE, Capability::CAN_EXEC,
                        Capability::CAN_KILL_PROCESS, Capability::CAN_ACCESS_HARDWARE};
                break;
            case UserLevel::USER:
                caps = {Capability::CAN_NETWORK, Capability::CAN_DISK_READ,
                        Capability::CAN_DISK_WRITE, Capability::CAN_EXEC};
                break;
            case UserLevel::PLUGIN:
                caps = {Capability::CAN_DISK_READ, Capability::CAN_EXEC};
                break;
            case UserLevel::GUEST:
                caps = {Capability::CAN_DISK_READ};
                break;
        }
        return caps;
    }

public:
    PermissionManager() {
        // Create root user
        create_user("root", UserLevel::ROOT);
        create_user("system", UserLevel::SYSTEM);
        create_group("root");
        create_group("system");
        create_group("users");
        create_group("plugins");
    }

    // ── User Management ──

    bool create_user(const std::string& username, UserLevel level) {
        if (users_.find(username) != users_.end()) return false;
        User user;
        user.username = username;
        user.uid = next_uid_++;
        user.level = level;
        user.capabilities = default_capabilities(level);
        user.created_at = std::time(nullptr);
        // Assign primary group based on level
        switch (level) {
            case UserLevel::ROOT: user.primary_gid = 0; user.groups.insert("root"); break;
            case UserLevel::SYSTEM: user.primary_gid = 1; user.groups.insert("system"); break;
            case UserLevel::PLUGIN: user.primary_gid = 3; user.groups.insert("plugins"); break;
            default: user.primary_gid = 2; user.groups.insert("users"); break;
        }
        users_[username] = user;
        audit_log_.log("system", 0, AuditAction::USER_CREATE, username, "Created user", true);
        return true;
    }

    bool delete_user(const std::string& username) {
        if (username == "root") return false;
        auto it = users_.find(username);
        if (it == users_.end()) return false;
        users_.erase(it);
        audit_log_.log("system", 0, AuditAction::USER_DELETE, username, "Deleted user", true);
        return true;
    }

    User* get_user(const std::string& username) {
        auto it = users_.find(username);
        return (it != users_.end()) ? &it->second : nullptr;
    }

    std::vector<User> list_users() const {
        std::vector<User> result;
        for (const auto& p : users_) result.push_back(p.second);
        return result;
    }

    // ── Group Management ──

    bool create_group(const std::string& name) {
        if (groups_.find(name) != groups_.end()) return false;
        Group group;
        group.name = name;
        group.gid = next_gid_++;
        groups_[name] = group;
        return true;
    }

    bool add_to_group(const std::string& username, const std::string& group_name) {
        auto uit = users_.find(username);
        auto git = groups_.find(group_name);
        if (uit == users_.end() || git == groups_.end()) return false;
        uit->second.groups.insert(group_name);
        git->second.members.insert(username);
        return true;
    }

    bool remove_from_group(const std::string& username, const std::string& group_name) {
        auto uit = users_.find(username);
        auto git = groups_.find(group_name);
        if (uit == users_.end() || git == groups_.end()) return false;
        uit->second.groups.erase(group_name);
        git->second.members.erase(username);
        return true;
    }


    // ── Capability Checks ──

    bool has_capability(const std::string& username, Capability cap) {
        auto user = get_user(username);
        if (!user) return false;
        if (user->locked) return false;
        // Check escalation tokens
        for (const auto& token : escalation_tokens_) {
            if (token.active && token.username == username) {
                time_t now = std::time(nullptr);
                if (now - token.granted_at < token.timeout_seconds) {
                    if (token.granted_capabilities.count(cap)) return true;
                }
            }
        }
        return user->capabilities.count(cap) > 0;
    }

    bool grant_capability(const std::string& username, Capability cap) {
        auto user = get_user(username);
        if (!user) return false;
        user->capabilities.insert(cap);
        return true;
    }

    bool revoke_capability(const std::string& username, Capability cap) {
        auto user = get_user(username);
        if (!user) return false;
        user->capabilities.erase(cap);
        return true;
    }

    // ── File Permission Checks ──

    void set_file_permissions(const std::string& path, const FilePermissionSet& perms) {
        file_permissions_[path] = perms;
    }

    bool check_file_access(const std::string& username, const std::string& path,
                           bool read, bool write, bool execute) {
        auto user = get_user(username);
        if (!user) {
            audit_log_.log(username, 0, AuditAction::PERMISSION_DENIED, path, "User not found", false);
            return false;
        }
        // Root can do anything
        if (user->level == UserLevel::ROOT) {
            audit_log_.log(username, user->uid, AuditAction::PERMISSION_CHECK, path, "Root access", true);
            return true;
        }
        // Check capability
        if (read && !has_capability(username, Capability::CAN_DISK_READ)) {
            audit_log_.log(username, user->uid, AuditAction::PERMISSION_DENIED, path, "No disk read cap", false);
            return false;
        }
        if (write && !has_capability(username, Capability::CAN_DISK_WRITE)) {
            audit_log_.log(username, user->uid, AuditAction::PERMISSION_DENIED, path, "No disk write cap", false);
            return false;
        }
        if (execute && !has_capability(username, Capability::CAN_EXEC)) {
            audit_log_.log(username, user->uid, AuditAction::PERMISSION_DENIED, path, "No exec cap", false);
            return false;
        }
        // Check file-level permissions
        auto pit = file_permissions_.find(path);
        if (pit == file_permissions_.end()) {
            // No explicit permissions: allow for owner-level
            audit_log_.log(username, user->uid, AuditAction::PERMISSION_CHECK, path, "No perms set, allowed", true);
            return true;
        }
        const auto& perms = pit->second;
        // Simplified: check 'other' permissions for non-root
        const auto& flags = perms.other;
        bool allowed = true;
        if (read && !flags.read) allowed = false;
        if (write && !flags.write) allowed = false;
        if (execute && !flags.execute) allowed = false;

        if (!allowed) {
            audit_log_.log(username, user->uid, AuditAction::PERMISSION_DENIED, path, "Permission denied", false);
        } else {
            audit_log_.log(username, user->uid, AuditAction::PERMISSION_CHECK, path, "Access granted", true);
        }
        return allowed;
    }

    // ── Privilege Escalation ──

    uint64_t escalate(const std::string& username, UserLevel target_level, uint32_t timeout_seconds = 300) {
        auto user = get_user(username);
        if (!user) return 0;
        if (!has_capability(username, Capability::CAN_ESCALATE) && user->level != UserLevel::ROOT) return 0;

        EscalationToken token;
        token.token_id = next_token_id_++;
        token.username = username;
        token.original_level = user->level;
        token.elevated_level = target_level;
        token.granted_capabilities = default_capabilities(target_level);
        token.granted_at = std::time(nullptr);
        token.timeout_seconds = timeout_seconds;
        token.active = true;
        escalation_tokens_.push_back(token);

        audit_log_.log(username, user->uid, AuditAction::PRIVILEGE_ESCALATE, "",
                      "Escalated to level " + std::to_string(static_cast<int>(target_level)), true);
        return token.token_id;
    }

    bool drop_escalation(uint64_t token_id) {
        for (auto& token : escalation_tokens_) {
            if (token.token_id == token_id && token.active) {
                token.active = false;
                audit_log_.log(token.username, 0, AuditAction::PRIVILEGE_DROP, "",
                              "Dropped escalation", true);
                return true;
            }
        }
        return false;
    }

    void expire_escalations() {
        time_t now = std::time(nullptr);
        for (auto& token : escalation_tokens_) {
            if (token.active && (now - token.granted_at >= token.timeout_seconds)) {
                token.active = false;
            }
        }
    }

    // ── Audit ──

    AuditLog& get_audit_log() { return audit_log_; }
    const AuditLog& get_audit_log() const { return audit_log_; }

    // ── Lock/Unlock ──

    bool lock_user(const std::string& username) {
        auto user = get_user(username);
        if (!user || username == "root") return false;
        user->locked = true;
        return true;
    }

    bool unlock_user(const std::string& username) {
        auto user = get_user(username);
        if (!user) return false;
        user->locked = false;
        return true;
    }
};

} // namespace MK_Security

#endif // MK_PERMISSIONS_CPP
