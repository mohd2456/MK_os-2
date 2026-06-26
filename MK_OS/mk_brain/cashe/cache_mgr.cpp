#ifndef MK_CACHE_MGR_CPP
#define MK_CACHE_MGR_CPP

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

// Lightweight structure representing a quantized model weight layer in cache
struct CacheSlot {
    std::string layerId;
    std::vector<uint8_t> quantizedWeights; // Pillar 3: Raw 4-bit compressed weight bytes
    bool isHot;                            // Pillar 2: Marks if this layer is actively processing
};

class MKCacheManager {
private:
    std::unordered_map<std::string, CacheSlot> centralCache;
    size_t maxCacheCapacitySlots;

public:
    MKCacheManager(size_t maxSlots = 4) : maxCacheCapacitySlots(maxSlots) {
        std::cout << "[BRAIN CACHE] Initialized Fixed-Slot Hot Weight Cache Window.\n";
    }

    // Loads a target layer directly into a hot RAM slot
    void cacheHotLayer(const std::string& layerId, const std::vector<uint8_t>& rawData) {
        // If cache is full, evict the oldest or coldest layer to keep RAM completely flat
        if (centralCache.size() >= maxCacheCapacitySlots) {
            for (auto it = centralCache.begin(); it != centralCache.end(); ++it) {
                if (!it->second.isHot) {
                    std::cout << "[BRAIN CACHE] Evicting cold layer: " << it->first << " from RAM.\n";
                    centralCache.erase(it);
                    break;
                }
            }
        }

        CacheSlot slot{layerId, rawData, true};
        centralCache[layerId] = slot;
        std::cout << "[BRAIN CACHE] Layer " << layerId << " loaded into active execution cache (" 
                  << rawData.size() << " bytes, quantized).\n";
    }

    // Sets a layer back to cold so it can be safely swapped out later
    void setLayerCold(const std::string& layerId) {
        if (centralCache.find(layerId) != centralCache.end()) {
            centralCache[layerId].isHot = false;
        }
    }

    bool isLayerCached(const std::string& layerId) {
        return centralCache.find(layerId) != centralCache.end();
    }
};

#endif // MK_CACHE_MGR_CPP