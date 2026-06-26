#ifndef MK_DISK_CPP
#define MK_DISK_CPP

#include <iostream>
#include <string>
#include <map>

struct MKDiskBlock {
    int id;
    std::string data;
    bool dirty; // Written but not flushed
};

class MKDiskDriver {
private:
    std::map<int, MKDiskBlock> cache; // Block cache
    int readCount  = 0;
    int writeCount = 0;
    int maxBlocks  = 1024;

public:
    MKDiskDriver() {
        std::cout << "[DRIVER] DMA disk channel open. "
                  << "Block cache ready. Max=" << maxBlocks << " blocks.\n";
    }

    // Read a block — returns from cache if available
    std::string readBlock(int block) {
        readCount++;
        if(cache.count(block)) {
            std::cout << "[DRV:DISK] Cache hit: block " << block
                      << " -> \"" << cache[block].data << "\"\n";
            return cache[block].data;
        }
        std::cout << "[DRV:DISK] Read block " << block << " from storage.\n";
        return "";
    }

    // Write a block — stores in cache and marks dirty
    bool writeBlock(int block, const std::string& data) {
        if(block < 0 || block >= maxBlocks) {
            std::cout << "[DRV:DISK] ERROR: Block " << block << " out of range.\n";
            return false;
        }
        writeCount++;
        cache[block] = {block, data, true};
        std::cout << "[DRV:DISK] Write block " << block
                  << " | Data=\"" << data << "\"\n";
        return true;
    }

    // Flush all dirty blocks to storage
    void flush() {
        int flushed = 0;
        for(auto& kv : cache) {
            if(kv.second.dirty) {
                kv.second.dirty = false;
                flushed++;
            }
        }
        std::cout << "[DRV:DISK] Flushed " << flushed << " dirty block(s).\n";
    }

    // Wipe a block
    void clearBlock(int block) {
        cache.erase(block);
        std::cout << "[DRV:DISK] Block " << block << " cleared.\n";
    }

    void stats() const {
        std::cout << "[DRV:DISK] Reads=" << readCount
                  << " Writes=" << writeCount
                  << " Cached=" << cache.size() << " blocks.\n";
    }
};

#endif // MK_DISK_CPP