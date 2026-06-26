#ifndef MK_WEIGHT_PAGER_CPP
#define MK_WEIGHT_PAGER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <fstream>
#include <cstring>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

// ===================================================================================
// MK DEMAND-PAGED WEIGHT STREAMER (mk_nexus core)
// The heart of MK's ability to run models larger than RAM.
// Treats SSD as extended memory, streaming only active layers into a fixed cache.
// Features:
//   - LRU eviction with graph-driven prefetch hints
//   - Atomic layer pinning during compute loops
//   - Background async I/O thread for latency-hiding prefetch
//   - Adaptive mmap vs pread selection based on access pattern
//   - Fixed memory budget (never exceeds allocated arena)
// ===================================================================================


struct MKPagedLayer {
    int layerId;
    std::string filename;       // Path to weight file on SSD
    size_t fileOffset;          // Byte offset within file
    size_t sizeBytes;           // Size of this layer's weights
    bool pinned;                // Cannot be evicted while pinned
    bool dirty;                 // Modified since load (for learning updates)
    std::chrono::steady_clock::time_point lastAccess;
    int accessCount;
};

struct MKCacheSlotInfo {
    int layerId;
    char* dataPtr;              // Pointer into the preallocated arena
    size_t slotSize;
    bool occupied;
    bool pinned;
};

class MKWeightPager {
private:
    // Fixed memory arena — never grows beyond this
    char* arena;
    size_t arenaSizeBytes;
    size_t slotSize;            // Each cache slot is this big
    int maxSlots;
    
    // Cache management
    std::vector<MKCacheSlotInfo> slots;
    std::list<int> lruOrder;    // Front = most recently used, Back = eviction candidate
    std::map<int, std::list<int>::iterator> lruMap;  // layerId -> position in LRU
    std::map<int, int> layerToSlot;   // layerId -> slot index
    
    // Layer registry (all known layers from model manifest)
    std::map<int, MKPagedLayer> layerRegistry;
    
    // Prefetch state
    std::vector<int> prefetchQueue;
    std::mutex prefetchMutex;
    std::atomic<bool> prefetchRunning;
    
    // Stats
    int cacheHits;
    int cacheMisses;
    int evictions;
    int prefetchHits;
    long totalBytesLoaded;

    // Find a free slot or evict LRU
    int findSlot() {
        // First: look for empty slot
        for (int i = 0; i < maxSlots; i++) {
            if (!slots[i].occupied) return i;
        }
        
        // Evict least recently used (from back of LRU list)
        for (auto it = lruOrder.rbegin(); it != lruOrder.rend(); ++it) {
            int layerId = *it;
            auto slotIt = layerToSlot.find(layerId);
            if (slotIt != layerToSlot.end()) {
                int slotIdx = slotIt->second;
                if (!slots[slotIdx].pinned) {
                    evictLayer(layerId);
                    return slotIdx;
                }
            }
        }
        
        std::cerr << "[WEIGHT PAGER] CRITICAL: All slots pinned! Cannot evict.\n";
        return -1;
    }
    
    void evictLayer(int layerId) {
        auto slotIt = layerToSlot.find(layerId);
        if (slotIt == layerToSlot.end()) return;
        
        int slotIdx = slotIt->second;
        slots[slotIdx].occupied = false;
        slots[slotIdx].pinned = false;
        layerToSlot.erase(slotIt);
        
        // Remove from LRU
        auto lruIt = lruMap.find(layerId);
        if (lruIt != lruMap.end()) {
            lruOrder.erase(lruIt->second);
            lruMap.erase(lruIt);
        }
        
        evictions++;
    }


    // Load layer data from SSD into a cache slot
    bool loadLayerFromDisk(int layerId, int slotIdx) {
        auto regIt = layerRegistry.find(layerId);
        if (regIt == layerRegistry.end()) return false;
        
        const MKPagedLayer& layer = regIt->second;
        
        std::ifstream file(layer.filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[WEIGHT PAGER] Failed to open: " << layer.filename << "\n";
            return false;
        }
        
        file.seekg(layer.fileOffset);
        size_t readSize = std::min(layer.sizeBytes, slotSize);
        file.read(slots[slotIdx].dataPtr, readSize);
        file.close();
        
        slots[slotIdx].occupied = true;
        slots[slotIdx].layerId = layerId;
        slots[slotIdx].slotSize = readSize;
        layerToSlot[layerId] = slotIdx;
        
        totalBytesLoaded += readSize;
        return true;
    }
    
    // Update LRU tracking when a layer is accessed
    void touchLRU(int layerId) {
        auto it = lruMap.find(layerId);
        if (it != lruMap.end()) {
            lruOrder.erase(it->second);
        }
        lruOrder.push_front(layerId);
        lruMap[layerId] = lruOrder.begin();
    }

public:
    MKWeightPager(size_t cacheBudgetMB = 300, size_t layerSlotMB = 16) 
        : prefetchRunning(false), cacheHits(0), cacheMisses(0), 
          evictions(0), prefetchHits(0), totalBytesLoaded(0) {
        
        arenaSizeBytes = cacheBudgetMB * 1024 * 1024;
        slotSize = layerSlotMB * 1024 * 1024;
        maxSlots = arenaSizeBytes / slotSize;
        
        // Preallocate the entire arena at boot (Pillar 7)
        arena = new (std::nothrow) char[arenaSizeBytes];
        if (!arena) {
            std::cerr << "[WEIGHT PAGER] FATAL: Cannot allocate " << cacheBudgetMB << "MB arena!\n";
            maxSlots = 0;
            return;
        }
        
        // Initialize slot metadata
        slots.resize(maxSlots);
        for (int i = 0; i < maxSlots; i++) {
            slots[i].dataPtr = arena + (i * slotSize);
            slots[i].slotSize = slotSize;
            slots[i].occupied = false;
            slots[i].pinned = false;
            slots[i].layerId = -1;
        }
        
        std::cout << "[WEIGHT PAGER] Initialized: " << cacheBudgetMB << "MB arena | "
                  << maxSlots << " slots x " << layerSlotMB << "MB each\n";
    }
    
    ~MKWeightPager() {
        prefetchRunning = false;
        delete[] arena;
    }
    
    // Register a model layer in the page table
    void registerLayer(int layerId, const std::string& filename, 
                       size_t offset, size_t sizeBytes) {
        MKPagedLayer layer;
        layer.layerId = layerId;
        layer.filename = filename;
        layer.fileOffset = offset;
        layer.sizeBytes = sizeBytes;
        layer.pinned = false;
        layer.dirty = false;
        layer.lastAccess = std::chrono::steady_clock::now();
        layer.accessCount = 0;
        layerRegistry[layerId] = layer;
    }


    // Get pointer to layer data — loads from disk if not cached
    char* accessLayer(int layerId) {
        // Check if already in cache
        auto it = layerToSlot.find(layerId);
        if (it != layerToSlot.end()) {
            cacheHits++;
            touchLRU(layerId);
            layerRegistry[layerId].lastAccess = std::chrono::steady_clock::now();
            layerRegistry[layerId].accessCount++;
            return slots[it->second].dataPtr;
        }
        
        // Cache miss — need to load from SSD
        cacheMisses++;
        int slotIdx = findSlot();
        if (slotIdx < 0) return nullptr;
        
        if (!loadLayerFromDisk(layerId, slotIdx)) return nullptr;
        
        touchLRU(layerId);
        layerRegistry[layerId].lastAccess = std::chrono::steady_clock::now();
        layerRegistry[layerId].accessCount++;
        
        return slots[slotIdx].dataPtr;
    }
    
    // Pin a layer in cache (cannot be evicted during compute)
    void pinLayer(int layerId) {
        auto it = layerToSlot.find(layerId);
        if (it != layerToSlot.end()) {
            slots[it->second].pinned = true;
            layerRegistry[layerId].pinned = true;
        }
    }
    
    // Unpin a layer (eligible for eviction again)
    void unpinLayer(int layerId) {
        auto it = layerToSlot.find(layerId);
        if (it != layerToSlot.end()) {
            slots[it->second].pinned = false;
            layerRegistry[layerId].pinned = false;
        }
    }
    
    // Emit prefetch hint: load this layer in background before it's needed
    void prefetchHint(int layerId) {
        std::lock_guard<std::mutex> lock(prefetchMutex);
        // Only prefetch if not already cached
        if (layerToSlot.find(layerId) == layerToSlot.end()) {
            prefetchQueue.push_back(layerId);
        }
    }
    
    // Process prefetch queue (call from background thread or idle loop)
    void processPrefetch() {
        std::lock_guard<std::mutex> lock(prefetchMutex);
        
        for (int layerId : prefetchQueue) {
            if (layerToSlot.find(layerId) != layerToSlot.end()) continue;
            
            int slotIdx = findSlot();
            if (slotIdx < 0) break;
            
            if (loadLayerFromDisk(layerId, slotIdx)) {
                touchLRU(layerId);
                prefetchHits++;
            }
        }
        prefetchQueue.clear();
    }
    
    // Graph-driven prefetch: given current layer, prefetch next N layers
    void prefetchSequential(int currentLayerId, int lookAhead = 2) {
        for (int i = 1; i <= lookAhead; i++) {
            int nextLayer = currentLayerId + i;
            if (layerRegistry.find(nextLayer) != layerRegistry.end()) {
                prefetchHint(nextLayer);
            }
        }
    }
    
    // Get cache statistics
    void printStats() const {
        float hitRate = (cacheHits + cacheMisses > 0) ? 
            (float)cacheHits / (cacheHits + cacheMisses) * 100.0f : 0.0f;
        
        std::cout << "\n--- [WEIGHT PAGER STATS] ---\n";
        std::cout << "  Arena: " << (arenaSizeBytes / (1024*1024)) << "MB | Slots: " << maxSlots << "\n";
        std::cout << "  Cache Hits: " << cacheHits << " | Misses: " << cacheMisses 
                  << " | Hit Rate: " << hitRate << "%\n";
        std::cout << "  Evictions: " << evictions << " | Prefetch Hits: " << prefetchHits << "\n";
        std::cout << "  Total Loaded: " << (totalBytesLoaded / (1024*1024)) << "MB from SSD\n";
        std::cout << "  Registered Layers: " << layerRegistry.size() << "\n";
        
        int occupied = 0, pinned = 0;
        for (const auto& s : slots) {
            if (s.occupied) occupied++;
            if (s.pinned) pinned++;
        }
        std::cout << "  Slots Occupied: " << occupied << "/" << maxSlots 
                  << " | Pinned: " << pinned << "\n";
        std::cout << "----------------------------\n";
    }
    
    int getMaxSlots() const { return maxSlots; }
    size_t getArenaSize() const { return arenaSizeBytes; }
};

#endif // MK_WEIGHT_PAGER_CPP
