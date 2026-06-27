#ifndef MK_COW_SNAPSHOTS_CPP
#define MK_COW_SNAPSHOTS_CPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <ctime>
#include <algorithm>

// ===================================================================================
// MK COPY-ON-WRITE (CoW) MEMORY SNAPSHOTS
// Enables instant state rollbacks and parallel sandboxing during experiments.
// Instead of copying entire memory regions, tracks only pages that change.
// Features:
//   - O(1) snapshot creation (just save page table state)
//   - Only copies pages when they're written to (lazy copy)
//   - Multiple named snapshots for A/B testing
//   - Rollback to any previous snapshot instantly
//   - Memory-efficient: unchanged pages shared across snapshots
// ===================================================================================

struct MKMemoryPage {
    int pageId;
    char* data;
    size_t size;
    bool modified;          // Has this page been written since last snapshot?
    int refCount;           // How many snapshots reference this page?
};

struct MKSnapshot {
    int snapshotId;
    std::string name;
    std::time_t createdAt;
    std::map<int, MKMemoryPage*> pageTable;  // pageId -> page data at snapshot time
    size_t totalSize;
    std::string description;
};

class MKCoWSnapshots {
private:
    // Live page table (current state)
    std::map<int, MKMemoryPage*> livePages;
    
    // Snapshot history
    std::vector<MKSnapshot> snapshots;
    int nextSnapshotId;
    int nextPageId;
    size_t pageSize;        // Size of each page (default 4KB)
    size_t maxMemory;       // Total memory budget
    size_t usedMemory;
    
    // Allocate a new page
    MKMemoryPage* allocatePage(size_t size) {
        if (usedMemory + size > maxMemory) {
            std::cerr << "[COW] WARNING: Memory budget exceeded!\n";
            return nullptr;
        }
        
        MKMemoryPage* page = new MKMemoryPage();
        page->pageId = nextPageId++;
        page->data = new char[size];
        page->size = size;
        page->modified = false;
        page->refCount = 1;
        std::memset(page->data, 0, size);
        
        usedMemory += size;
        return page;
    }
    
    // Clone a page (actual data copy — only happens on write)
    MKMemoryPage* clonePage(MKMemoryPage* original) {
        MKMemoryPage* copy = allocatePage(original->size);
        if (!copy) return nullptr;
        std::memcpy(copy->data, original->data, original->size);
        copy->modified = false;
        return copy;
    }
    
    // Release a page reference
    void releasePage(MKMemoryPage* page) {
        if (!page) return;
        page->refCount--;
        if (page->refCount <= 0) {
            usedMemory -= page->size;
            delete[] page->data;
            delete page;
        }
    }

public:
    MKCoWSnapshots(size_t pageSz = 4096, size_t maxMem = 64 * 1024 * 1024)
        : nextSnapshotId(0), nextPageId(0), pageSize(pageSz), 
          maxMemory(maxMem), usedMemory(0) {
        std::cout << "[COW] Copy-on-Write snapshot engine ready. Page size: " << pageSz
                  << " | Budget: " << (maxMem / (1024*1024)) << "MB\n";
    }
    
    ~MKCoWSnapshots() {
        // Decrement refCount for each reference (live + snapshots), then free
        // only pages whose refCount drops to 0.  Using a set ensures each
        // unique page pointer is freed at most once, preventing double-free
        // when a page is shared between livePages and snapshot pageTables.
        std::set<MKMemoryPage*> allPages;

        // Decrement refCount for each live reference
        for (auto& kv : livePages) {
            if (kv.second) {
                kv.second->refCount--;
                allPages.insert(kv.second);
            }
        }

        // Decrement refCount for each snapshot reference
        for (auto& snap : snapshots) {
            for (auto& kv : snap.pageTable) {
                if (kv.second) {
                    kv.second->refCount--;
                    allPages.insert(kv.second);
                }
            }
        }

        // Free only pages whose refCount dropped to 0 or below
        for (auto* page : allPages) {
            if (page && page->refCount <= 0) {
                delete[] page->data;
                delete page;
            }
        }
    }
    
    // Allocate a new tracked memory region
    int createRegion(size_t size, const std::string& label = "") {
        MKMemoryPage* page = allocatePage(size);
        if (!page) return -1;
        livePages[page->pageId] = page;
        std::cout << "[COW] Region #" << page->pageId << " created: " << size << " bytes"
                  << (label.empty() ? "" : " (\"" + label + "\")") << "\n";
        return page->pageId;
    }
    
    // Write to a tracked region (triggers CoW if page is shared with snapshot)
    bool writeRegion(int pageId, const void* data, size_t offset, size_t length) {
        auto it = livePages.find(pageId);
        if (it == livePages.end()) return false;
        
        MKMemoryPage* page = it->second;
        
        // If this page is shared with any snapshot, we need to copy it first
        if (page->refCount > 1) {
            // Copy-on-Write triggered!
            MKMemoryPage* newPage = clonePage(page);
            if (!newPage) return false;
            
            page->refCount--;  // Original still referenced by snapshots
            livePages[pageId] = newPage;
            page = newPage;
            
            std::cout << "[COW] Copy-on-Write triggered for page #" << pageId << "\n";
        }
        
        // Now safe to write
        if (offset + length > page->size) return false;
        std::memcpy(page->data + offset, data, length);
        page->modified = true;
        return true;
    }
    
    // Read from a tracked region
    bool readRegion(int pageId, void* output, size_t offset, size_t length) {
        auto it = livePages.find(pageId);
        if (it == livePages.end()) return false;
        
        MKMemoryPage* page = it->second;
        if (offset + length > page->size) return false;
        std::memcpy(output, page->data + offset, length);
        return true;
    }
    
    // Take a snapshot of current state (O(1) — just increments ref counts)
    int takeSnapshot(const std::string& name, const std::string& description = "") {
        MKSnapshot snap;
        snap.snapshotId = nextSnapshotId++;
        snap.name = name;
        snap.createdAt = std::time(nullptr);
        snap.description = description;
        snap.totalSize = 0;
        
        // Share all current pages with the snapshot (increment ref counts)
        for (auto& kv : livePages) {
            kv.second->refCount++;
            kv.second->modified = false;
            snap.pageTable[kv.first] = kv.second;
            snap.totalSize += kv.second->size;
        }
        
        snapshots.push_back(snap);
        std::cout << "[COW] Snapshot #" << snap.snapshotId << " \"" << name 
                  << "\" captured: " << snap.pageTable.size() << " pages ("
                  << (snap.totalSize / 1024) << "KB)\n";
        return snap.snapshotId;
    }
    
    // Rollback to a named snapshot (restores all pages to that state)
    bool rollback(const std::string& name) {
        for (auto& snap : snapshots) {
            if (snap.name == name) {
                // Release current live pages
                for (auto& kv : livePages) {
                    kv.second->refCount--;
                    if (kv.second->refCount <= 0) {
                        usedMemory -= kv.second->size;
                        delete[] kv.second->data;
                        delete kv.second;
                    }
                }
                
                // Restore from snapshot (share pages again)
                livePages.clear();
                for (auto& kv : snap.pageTable) {
                    kv.second->refCount++;
                    livePages[kv.first] = kv.second;
                }
                
                std::cout << "[COW] Rolled back to snapshot \"" << name << "\"\n";
                return true;
            }
        }
        std::cerr << "[COW] Snapshot \"" << name << "\" not found!\n";
        return false;
    }
    
    // Rollback by ID
    bool rollbackById(int snapshotId) {
        for (auto& snap : snapshots) {
            if (snap.snapshotId == snapshotId) return rollback(snap.name);
        }
        return false;
    }
    
    // Delete a snapshot (frees shared pages if no longer referenced)
    void deleteSnapshot(const std::string& name) {
        for (auto it = snapshots.begin(); it != snapshots.end(); ++it) {
            if (it->name == name) {
                for (auto& kv : it->pageTable) releasePage(kv.second);
                snapshots.erase(it);
                std::cout << "[COW] Snapshot \"" << name << "\" deleted.\n";
                return;
            }
        }
    }
    
    // List all snapshots
    void listSnapshots() const {
        std::cout << "\n--- [COW SNAPSHOTS] ---\n";
        for (const auto& snap : snapshots) {
            std::cout << "  #" << snap.snapshotId << " \"" << snap.name << "\" | "
                      << snap.pageTable.size() << " pages | " 
                      << (snap.totalSize / 1024) << "KB | "
                      << std::ctime(&snap.createdAt);
        }
        std::cout << "  Memory used: " << (usedMemory / 1024) << "KB / "
                  << (maxMemory / (1024*1024)) << "MB\n";
        std::cout << "-----------------------\n";
    }
    
    size_t getUsedMemory() const { return usedMemory; }
    int getSnapshotCount() const { return snapshots.size(); }
};

#endif // MK_COW_SNAPSHOTS_CPP
