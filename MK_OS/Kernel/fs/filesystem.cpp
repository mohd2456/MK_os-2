#ifndef MK_FILESYSTEM_CPP
#define MK_FILESYSTEM_CPP

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <functional>
#include <deque>
#include <numeric>
#include <cstring>

namespace MK_FS {

// ─── Metadata & Permissions ─────────────────────────────────────────────────

struct FilePermissions {
    bool owner_read = true;
    bool owner_write = true;
    bool owner_exec = false;
    bool group_read = true;
    bool group_write = false;
    bool group_exec = false;
    bool other_read = true;
    bool other_write = false;
    bool other_exec = false;
};

struct FileMetadata {
    std::string name;
    size_t size = 0;
    FilePermissions permissions;
    time_t created_at = 0;
    time_t modified_at = 0;
    time_t accessed_at = 0;
    std::string owner = "root";
    std::string group = "system";
};

// ─── Journal (Write-Ahead Log) ──────────────────────────────────────────────

enum class JournalOpType {
    CREATE,
    WRITE,
    DELETE,
    RENAME,
    MKDIR,
    RMDIR
};

struct JournalEntry {
    uint64_t transaction_id;
    JournalOpType op;
    std::string path;
    std::string data;       // for writes
    std::string extra;      // for rename (new name)
    time_t timestamp;
    bool committed = false;
};

class Journal {
private:
    std::deque<JournalEntry> entries_;
    uint64_t next_tx_id_ = 1;
    size_t max_entries_ = 10000;

public:
    uint64_t begin_transaction(JournalOpType op, const std::string& path,
                               const std::string& data = "", const std::string& extra = "") {
        JournalEntry entry;
        entry.transaction_id = next_tx_id_++;
        entry.op = op;
        entry.path = path;
        entry.data = data;
        entry.extra = extra;
        entry.timestamp = std::time(nullptr);
        entry.committed = false;
        entries_.push_back(entry);
        if (entries_.size() > max_entries_) {
            entries_.pop_front();
        }
        return entry.transaction_id;
    }

    void commit(uint64_t tx_id) {
        for (auto& e : entries_) {
            if (e.transaction_id == tx_id) {
                e.committed = true;
                break;
            }
        }
    }

    void rollback(uint64_t tx_id) {
        entries_.erase(
            std::remove_if(entries_.begin(), entries_.end(),
                [tx_id](const JournalEntry& e) { return e.transaction_id == tx_id; }),
            entries_.end());
    }

    std::vector<JournalEntry> get_uncommitted() const {
        std::vector<JournalEntry> result;
        for (const auto& e : entries_) {
            if (!e.committed) result.push_back(e);
        }
        return result;
    }

    size_t size() const { return entries_.size(); }
};

// ─── File System Node ───────────────────────────────────────────────────────

enum class NodeType {
    FILE,
    DIRECTORY,
    SYMLINK,
    MOUNT_POINT
};

struct FSNode {
    NodeType type = NodeType::FILE;
    FileMetadata metadata;
    std::string content;                          // file content
    std::vector<std::shared_ptr<FSNode>> children; // directory children
    std::string symlink_target;                   // for symlinks
    std::string mount_source;                     // for mount points

    FSNode() {
        metadata.created_at = std::time(nullptr);
        metadata.modified_at = std::time(nullptr);
        metadata.accessed_at = std::time(nullptr);
    }

    std::shared_ptr<FSNode> find_child(const std::string& name) const {
        for (const auto& child : children) {
            if (child->metadata.name == name) return child;
        }
        return nullptr;
    }

    bool remove_child(const std::string& name) {
        auto it = std::remove_if(children.begin(), children.end(),
            [&name](const std::shared_ptr<FSNode>& n) { return n->metadata.name == name; });
        if (it != children.end()) {
            children.erase(it, children.end());
            return true;
        }
        return false;
    }
};

// ─── Mount Point ────────────────────────────────────────────────────────────

struct MountPoint {
    std::string path;
    std::string source;
    std::string fs_type;
    bool read_only = false;
};

// ─── File System Stats ──────────────────────────────────────────────────────

struct FSStats {
    size_t total_files = 0;
    size_t total_directories = 0;
    size_t used_space = 0;
    size_t total_space = 1024 * 1024 * 1024; // 1GB default
    size_t free_space = 1024 * 1024 * 1024;
    size_t journal_entries = 0;
    size_t mount_points = 0;
};

// ─── Search Result ──────────────────────────────────────────────────────────

struct SearchResult {
    std::string path;
    std::string name;
    NodeType type;
    size_t size;
    time_t modified_at;
};

// ─── Virtual File System ────────────────────────────────────────────────────

class VirtualFileSystem {
private:
    std::shared_ptr<FSNode> root_;
    std::string current_directory_;
    Journal journal_;
    std::vector<MountPoint> mount_points_;
    size_t total_space_;
    size_t used_space_;

    // ── Path Utilities ──

    std::vector<std::string> split_path(const std::string& path) const {
        std::vector<std::string> parts;
        std::istringstream stream(path);
        std::string part;
        while (std::getline(stream, part, '/')) {
            if (!part.empty() && part != ".") {
                if (part == "..") {
                    if (!parts.empty()) parts.pop_back();
                } else {
                    parts.push_back(part);
                }
            }
        }
        return parts;
    }

    std::string resolve_path(const std::string& path) const {
        std::string resolved = path;
        if (resolved.empty()) resolved = "/";
        if (resolved[0] != '/') {
            resolved = current_directory_ + "/" + resolved;
        }
        // Normalize
        auto parts = split_path(resolved);
        std::string result = "/";
        for (size_t i = 0; i < parts.size(); ++i) {
            result += parts[i];
            if (i + 1 < parts.size()) result += "/";
        }
        return result;
    }

    std::shared_ptr<FSNode> navigate_to(const std::string& path) const {
        std::string resolved = resolve_path(path);
        if (resolved == "/") return root_;
        auto parts = split_path(resolved);
        auto current = root_;
        for (const auto& part : parts) {
            if (!current || current->type != NodeType::DIRECTORY) return nullptr;
            current = current->find_child(part);
        }
        return current;
    }

    std::shared_ptr<FSNode> navigate_to_parent(const std::string& path, std::string& child_name) const {
        std::string resolved = resolve_path(path);
        auto parts = split_path(resolved);
        if (parts.empty()) return nullptr;
        child_name = parts.back();
        parts.pop_back();
        auto current = root_;
        for (const auto& part : parts) {
            if (!current || current->type != NodeType::DIRECTORY) return nullptr;
            current = current->find_child(part);
        }
        return current;
    }

    void count_nodes(const std::shared_ptr<FSNode>& node, size_t& files, size_t& dirs, size_t& space) const {
        if (!node) return;
        if (node->type == NodeType::FILE) {
            files++;
            space += node->metadata.size;
        } else if (node->type == NodeType::DIRECTORY) {
            dirs++;
            for (const auto& child : node->children) {
                count_nodes(child, files, dirs, space);
            }
        }
    }

    void search_recursive(const std::shared_ptr<FSNode>& node, const std::string& current_path,
                          const std::function<bool(const FSNode&)>& predicate,
                          std::vector<SearchResult>& results) const {
        if (!node) return;
        if (predicate(*node)) {
            SearchResult sr;
            sr.path = current_path;
            sr.name = node->metadata.name;
            sr.type = node->type;
            sr.size = node->metadata.size;
            sr.modified_at = node->metadata.modified_at;
            results.push_back(sr);
        }
        if (node->type == NodeType::DIRECTORY) {
            for (const auto& child : node->children) {
                std::string child_path = current_path;
                if (child_path.back() != '/') child_path += "/";
                child_path += child->metadata.name;
                search_recursive(child, child_path, predicate, results);
            }
        }
    }

public:
    VirtualFileSystem(size_t total_space = 1024ULL * 1024 * 1024)
        : total_space_(total_space), used_space_(0) {
        root_ = std::make_shared<FSNode>();
        root_->type = NodeType::DIRECTORY;
        root_->metadata.name = "/";
        root_->metadata.permissions.owner_exec = true;
        root_->metadata.permissions.group_exec = true;
        root_->metadata.permissions.other_exec = true;
        current_directory_ = "/";
    }

    // ── Basic Operations ──

    bool create_file(const std::string& path, const std::string& content = "",
                     const std::string& owner = "root") {
        std::string child_name;
        auto parent = navigate_to_parent(path, child_name);
        if (!parent || parent->type != NodeType::DIRECTORY) return false;
        if (parent->find_child(child_name)) return false; // already exists

        uint64_t tx = journal_.begin_transaction(JournalOpType::CREATE, resolve_path(path), content);

        auto node = std::make_shared<FSNode>();
        node->type = NodeType::FILE;
        node->metadata.name = child_name;
        node->metadata.owner = owner;
        node->metadata.size = content.size();
        node->content = content;
        parent->children.push_back(node);
        used_space_ += content.size();

        journal_.commit(tx);
        return true;
    }

    bool create_directory(const std::string& path, const std::string& owner = "root") {
        std::string child_name;
        auto parent = navigate_to_parent(path, child_name);
        if (!parent || parent->type != NodeType::DIRECTORY) return false;
        if (parent->find_child(child_name)) return false;

        uint64_t tx = journal_.begin_transaction(JournalOpType::MKDIR, resolve_path(path));

        auto node = std::make_shared<FSNode>();
        node->type = NodeType::DIRECTORY;
        node->metadata.name = child_name;
        node->metadata.owner = owner;
        node->metadata.permissions.owner_exec = true;
        node->metadata.permissions.group_exec = true;
        node->metadata.permissions.other_exec = true;
        parent->children.push_back(node);

        journal_.commit(tx);
        return true;
    }

    std::string read_file(const std::string& path) {
        auto node = navigate_to(path);
        if (!node || node->type != NodeType::FILE) return "";
        node->metadata.accessed_at = std::time(nullptr);
        return node->content;
    }

    bool write_file(const std::string& path, const std::string& content) {
        auto node = navigate_to(path);
        if (!node || node->type != NodeType::FILE) return false;

        uint64_t tx = journal_.begin_transaction(JournalOpType::WRITE, resolve_path(path), content);

        used_space_ -= node->metadata.size;
        node->content = content;
        node->metadata.size = content.size();
        node->metadata.modified_at = std::time(nullptr);
        used_space_ += content.size();

        journal_.commit(tx);
        return true;
    }

    bool append_file(const std::string& path, const std::string& content) {
        auto node = navigate_to(path);
        if (!node || node->type != NodeType::FILE) return false;

        uint64_t tx = journal_.begin_transaction(JournalOpType::WRITE, resolve_path(path), content);

        node->content += content;
        node->metadata.size = node->content.size();
        node->metadata.modified_at = std::time(nullptr);
        used_space_ += content.size();

        journal_.commit(tx);
        return true;
    }

    bool delete_file(const std::string& path) {
        std::string child_name;
        auto parent = navigate_to_parent(path, child_name);
        if (!parent) return false;
        auto node = parent->find_child(child_name);
        if (!node || node->type != NodeType::FILE) return false;

        uint64_t tx = journal_.begin_transaction(JournalOpType::DELETE, resolve_path(path));

        used_space_ -= node->metadata.size;
        parent->remove_child(child_name);

        journal_.commit(tx);
        return true;
    }

    bool delete_directory(const std::string& path, bool recursive = false) {
        std::string child_name;
        auto parent = navigate_to_parent(path, child_name);
        if (!parent) return false;
        auto node = parent->find_child(child_name);
        if (!node || node->type != NodeType::DIRECTORY) return false;
        if (!recursive && !node->children.empty()) return false;

        uint64_t tx = journal_.begin_transaction(JournalOpType::RMDIR, resolve_path(path));

        if (recursive) {
            size_t files = 0, dirs = 0, space = 0;
            count_nodes(node, files, dirs, space);
            used_space_ -= space;
        }
        parent->remove_child(child_name);

        journal_.commit(tx);
        return true;
    }

    bool rename(const std::string& old_path, const std::string& new_path) {
        std::string old_name, new_name;
        auto old_parent = navigate_to_parent(old_path, old_name);
        auto new_parent = navigate_to_parent(new_path, new_name);
        if (!old_parent || !new_parent) return false;
        auto node = old_parent->find_child(old_name);
        if (!node) return false;
        if (new_parent->find_child(new_name)) return false;

        uint64_t tx = journal_.begin_transaction(JournalOpType::RENAME, resolve_path(old_path), "", resolve_path(new_path));

        old_parent->remove_child(old_name);
        node->metadata.name = new_name;
        node->metadata.modified_at = std::time(nullptr);
        new_parent->children.push_back(node);

        journal_.commit(tx);
        return true;
    }

    // ── Directory Navigation ──

    bool change_directory(const std::string& path) {
        auto node = navigate_to(path);
        if (!node || node->type != NodeType::DIRECTORY) return false;
        current_directory_ = resolve_path(path);
        return true;
    }

    std::string get_current_directory() const {
        return current_directory_;
    }

    std::vector<FileMetadata> list_directory(const std::string& path = ".") {
        auto node = navigate_to(path);
        std::vector<FileMetadata> result;
        if (!node || node->type != NodeType::DIRECTORY) return result;
        node->metadata.accessed_at = std::time(nullptr);
        for (const auto& child : node->children) {
            result.push_back(child->metadata);
        }
        return result;
    }

    // ── Mount Points ──

    bool mount(const std::string& path, const std::string& source,
               const std::string& fs_type = "mkfs", bool read_only = false) {
        // Create mount point directory if needed
        auto node = navigate_to(path);
        if (!node) {
            if (!create_directory(path)) return false;
            node = navigate_to(path);
        }
        if (!node || node->type != NodeType::DIRECTORY) return false;

        node->type = NodeType::MOUNT_POINT;
        node->mount_source = source;

        MountPoint mp;
        mp.path = resolve_path(path);
        mp.source = source;
        mp.fs_type = fs_type;
        mp.read_only = read_only;
        mount_points_.push_back(mp);
        return true;
    }

    bool unmount(const std::string& path) {
        std::string resolved = resolve_path(path);
        auto it = std::remove_if(mount_points_.begin(), mount_points_.end(),
            [&resolved](const MountPoint& mp) { return mp.path == resolved; });
        if (it == mount_points_.end()) return false;
        mount_points_.erase(it, mount_points_.end());

        auto node = navigate_to(path);
        if (node) node->type = NodeType::DIRECTORY;
        return true;
    }

    std::vector<MountPoint> get_mount_points() const {
        return mount_points_;
    }

    // ── Search ──

    std::vector<SearchResult> search_by_name(const std::string& name_pattern) const {
        std::vector<SearchResult> results;
        search_recursive(root_, "/", [&name_pattern](const FSNode& node) {
            return node.metadata.name.find(name_pattern) != std::string::npos;
        }, results);
        return results;
    }

    std::vector<SearchResult> search_by_extension(const std::string& extension) const {
        std::vector<SearchResult> results;
        std::string ext = extension;
        if (!ext.empty() && ext[0] != '.') ext = "." + ext;
        search_recursive(root_, "/", [&ext](const FSNode& node) {
            if (node.type != NodeType::FILE) return false;
            const std::string& name = node.metadata.name;
            if (name.size() < ext.size()) return false;
            return name.compare(name.size() - ext.size(), ext.size(), ext) == 0;
        }, results);
        return results;
    }

    std::vector<SearchResult> search_by_content(const std::string& query) const {
        std::vector<SearchResult> results;
        search_recursive(root_, "/", [&query](const FSNode& node) {
            if (node.type != NodeType::FILE) return false;
            return node.content.find(query) != std::string::npos;
        }, results);
        return results;
    }

    // ── Stats ──

    FSStats get_stats() const {
        FSStats stats;
        size_t space = 0;
        count_nodes(root_, stats.total_files, stats.total_directories, space);
        stats.used_space = space;
        stats.total_space = total_space_;
        stats.free_space = (total_space_ > space) ? (total_space_ - space) : 0;
        stats.journal_entries = journal_.size();
        stats.mount_points = mount_points_.size();
        return stats;
    }

    // ── File Info ──

    FileMetadata* get_metadata(const std::string& path) {
        auto node = navigate_to(path);
        if (!node) return nullptr;
        return &node->metadata;
    }

    bool set_permissions(const std::string& path, const FilePermissions& perms) {
        auto node = navigate_to(path);
        if (!node) return false;
        node->metadata.permissions = perms;
        return true;
    }

    bool set_owner(const std::string& path, const std::string& owner, const std::string& group) {
        auto node = navigate_to(path);
        if (!node) return false;
        node->metadata.owner = owner;
        node->metadata.group = group;
        return true;
    }

    bool exists(const std::string& path) const {
        return navigate_to(path) != nullptr;
    }

    bool is_directory(const std::string& path) const {
        auto node = navigate_to(path);
        return node && node->type == NodeType::DIRECTORY;
    }

    bool is_file(const std::string& path) const {
        auto node = navigate_to(path);
        return node && node->type == NodeType::FILE;
    }

    size_t file_size(const std::string& path) const {
        auto node = navigate_to(path);
        if (!node || node->type != NodeType::FILE) return 0;
        return node->metadata.size;
    }
};

} // namespace MK_FS

#endif // MK_FILESYSTEM_CPP
