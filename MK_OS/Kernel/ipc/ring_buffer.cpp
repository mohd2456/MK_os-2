#ifndef MK_RING_BUFFER_CPP
#define MK_RING_BUFFER_CPP

#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <atomic>
#include <chrono>

// ===================================================================================
// MK ZERO-COPY IPC RING BUFFER
// Lock-free shared memory communication between kernel and ai_core.
// Features:
//   - 64-byte structured headers (cache-line aligned)
//   - Sequence IDs + monotonic timestamps for packet integrity
//   - CRC32 checksums to detect corruption
//   - Single-producer single-consumer (SPSC) lock-free design
//   - Direct memory pointers bypass serialization overhead
// ===================================================================================

// Header fits exactly in one 64-byte cache line
struct MKRingHeader {
    std::atomic<uint32_t> writePos;     // 4 bytes
    std::atomic<uint32_t> readPos;      // 4 bytes
    uint32_t capacity;                   // 4 bytes
    uint32_t slotSize;                   // 4 bytes
    uint64_t sequenceCounter;            // 8 bytes
    uint64_t createdTimestamp;           // 8 bytes
    uint32_t producerPid;                // 4 bytes
    uint32_t consumerPid;                // 4 bytes
    uint32_t totalWritten;               // 4 bytes
    uint32_t totalRead;                  // 4 bytes
    uint32_t overflows;                  // 4 bytes
    uint32_t checksumErrors;             // 4 bytes
    char name[8];                        // 8 bytes
    // Total: 64 bytes exactly
};

// Each message in the ring has this packet wrapper
struct MKIPCPacket {
    uint32_t sequenceId;
    uint64_t timestamp;         // Monotonic nanoseconds
    uint32_t payloadSize;
    uint32_t crc32;
    uint8_t  type;              // Message type tag
    uint8_t  priority;          // 0=realtime, 255=idle
    uint8_t  flags;             // Bit flags for special handling
    uint8_t  reserved;
    // Payload follows immediately after this header
};

enum class MKIPCMessageType : uint8_t {
    DATA            = 0x01,     // Raw data transfer
    COMMAND         = 0x02,     // Control command
    RESPONSE        = 0x03,     // Response to a command
    WEIGHT_REQUEST  = 0x10,     // ai_core requests a weight layer
    WEIGHT_READY    = 0x11,     // Kernel signals weight is loaded
    INFERENCE_START = 0x20,     // ai_core starting inference burst
    INFERENCE_DONE  = 0x21,     // ai_core finished inference
    HEARTBEAT       = 0xFE,     // Keep-alive ping
    SHUTDOWN        = 0xFF      // Graceful shutdown signal
};

class MKRingBuffer {
private:
    MKRingHeader* header;
    char* buffer;
    size_t totalSize;
    bool ownsMemory;

    // Simple CRC32 for integrity verification
    uint32_t computeCRC32(const void* data, size_t length) {
        const uint8_t* bytes = (const uint8_t*)data;
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < length; i++) {
            crc ^= bytes[i];
            for (int j = 0; j < 8; j++) {
                crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
            }
        }
        return crc ^ 0xFFFFFFFF;
    }

    uint64_t getMonotonicNs() {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();
    }

    // Get pointer to slot N in the ring
    char* slotPtr(uint32_t index) {
        return buffer + sizeof(MKRingHeader) + (index * header->slotSize);
    }

public:
    // Create a new ring buffer with given capacity
    MKRingBuffer(const std::string& name, uint32_t numSlots = 64, 
                 uint32_t slotBytes = 4096) : ownsMemory(true) {
        
        totalSize = sizeof(MKRingHeader) + (numSlots * slotBytes);
        buffer = new (std::nothrow) char[totalSize];
        
        if (!buffer) {
            std::cerr << "[IPC] FATAL: Cannot allocate ring buffer!\n";
            header = nullptr;
            return;
        }
        std::memset(buffer, 0, totalSize);
        
        // Initialize header
        header = reinterpret_cast<MKRingHeader*>(buffer);
        header->writePos.store(0);
        header->readPos.store(0);
        header->capacity = numSlots;
        header->slotSize = slotBytes;
        header->sequenceCounter = 0;
        header->createdTimestamp = getMonotonicNs();
        header->producerPid = 0;
        header->consumerPid = 0;
        header->totalWritten = 0;
        header->totalRead = 0;
        header->overflows = 0;
        header->checksumErrors = 0;
        std::strncpy(header->name, name.c_str(), 7);
        header->name[7] = '\0';
        
        std::cout << "[IPC] Ring buffer \"" << name << "\" created: " 
                  << numSlots << " slots x " << slotBytes << " bytes = "
                  << (totalSize / 1024) << "KB\n";
    }
    
    // Attach to existing shared memory region
    MKRingBuffer(char* sharedMemory) : ownsMemory(false) {
        buffer = sharedMemory;
        header = reinterpret_cast<MKRingHeader*>(buffer);
        totalSize = sizeof(MKRingHeader) + (header->capacity * header->slotSize);
    }
    
    ~MKRingBuffer() {
        if (ownsMemory && buffer) delete[] buffer;
    }


    // Write a message into the ring (producer side)
    bool write(const void* payload, uint32_t payloadSize, 
               MKIPCMessageType type = MKIPCMessageType::DATA, uint8_t priority = 128) {
        if (!header || !payload) return false;
        
        uint32_t packetTotalSize = sizeof(MKIPCPacket) + payloadSize;
        if (packetTotalSize > header->slotSize) {
            std::cerr << "[IPC] ERROR: Payload too large for slot (" 
                      << payloadSize << " > " << header->slotSize << ")\n";
            return false;
        }
        
        // Check if ring is full
        uint32_t wp = header->writePos.load(std::memory_order_relaxed);
        uint32_t rp = header->readPos.load(std::memory_order_acquire);
        uint32_t nextWp = (wp + 1) % header->capacity;
        
        if (nextWp == rp) {
            header->overflows++;
            return false;  // Ring full — would overwrite unread data
        }
        
        // Build packet header
        char* slot = slotPtr(wp);
        MKIPCPacket* pkt = reinterpret_cast<MKIPCPacket*>(slot);
        pkt->sequenceId = (uint32_t)(++header->sequenceCounter);
        pkt->timestamp = getMonotonicNs();
        pkt->payloadSize = payloadSize;
        pkt->type = (uint8_t)type;
        pkt->priority = priority;
        pkt->flags = 0;
        pkt->reserved = 0;
        
        // Copy payload directly after packet header (zero serialization)
        std::memcpy(slot + sizeof(MKIPCPacket), payload, payloadSize);
        
        // Compute CRC over payload
        pkt->crc32 = computeCRC32(payload, payloadSize);
        
        // Advance write position (release semantics: ensures all writes above are visible)
        header->writePos.store(nextWp, std::memory_order_release);
        header->totalWritten++;
        
        return true;
    }
    
    // Read a message from the ring (consumer side)
    bool read(void* outPayload, uint32_t& outSize, MKIPCMessageType& outType) {
        if (!header || !outPayload) return false;
        
        uint32_t rp = header->readPos.load(std::memory_order_relaxed);
        uint32_t wp = header->writePos.load(std::memory_order_acquire);
        
        if (rp == wp) return false;  // Ring empty
        
        // Read packet
        char* slot = slotPtr(rp);
        MKIPCPacket* pkt = reinterpret_cast<MKIPCPacket*>(slot);
        
        // Verify CRC integrity
        uint32_t computedCRC = computeCRC32(slot + sizeof(MKIPCPacket), pkt->payloadSize);
        if (computedCRC != pkt->crc32) {
            header->checksumErrors++;
            std::cerr << "[IPC] CRC MISMATCH on packet #" << pkt->sequenceId << "!\n";
            // Still advance read pos to skip corrupted packet
            header->readPos.store((rp + 1) % header->capacity, std::memory_order_release);
            return false;
        }
        
        // Copy payload to output buffer
        outSize = pkt->payloadSize;
        outType = (MKIPCMessageType)pkt->type;
        std::memcpy(outPayload, slot + sizeof(MKIPCPacket), pkt->payloadSize);
        
        // Advance read position
        header->readPos.store((rp + 1) % header->capacity, std::memory_order_release);
        header->totalRead++;
        
        return true;
    }
    
    // Write convenience: string message
    bool writeString(const std::string& msg, MKIPCMessageType type = MKIPCMessageType::DATA) {
        return write(msg.c_str(), msg.size(), type);
    }
    
    // Read convenience: string message
    std::string readString() {
        char buf[4096];
        uint32_t size = 0;
        MKIPCMessageType type;
        if (read(buf, size, type)) {
            return std::string(buf, size);
        }
        return "";
    }
    
    // Check if ring has unread messages
    bool hasData() const {
        if (!header) return false;
        return header->writePos.load(std::memory_order_acquire) != 
               header->readPos.load(std::memory_order_acquire);
    }
    
    // Get number of pending unread messages
    uint32_t pending() const {
        if (!header) return 0;
        uint32_t wp = header->writePos.load(std::memory_order_acquire);
        uint32_t rp = header->readPos.load(std::memory_order_acquire);
        return (wp >= rp) ? (wp - rp) : (header->capacity - rp + wp);
    }
    
    bool isFull() const {
        if (!header) return true;
        uint32_t nextWp = (header->writePos.load() + 1) % header->capacity;
        return nextWp == header->readPos.load();
    }
    
    // Get raw pointer to ring memory (for sharing via mmap/shm)
    char* getRawMemory() { return buffer; }
    size_t getTotalSize() const { return totalSize; }
    
    void printStats() const {
        if (!header) return;
        std::cout << "\n--- [IPC RING: " << header->name << "] ---\n";
        std::cout << "  Capacity: " << header->capacity << " slots x " << header->slotSize << " bytes\n";
        std::cout << "  Written: " << header->totalWritten << " | Read: " << header->totalRead << "\n";
        std::cout << "  Pending: " << pending() << " | Overflows: " << header->overflows << "\n";
        std::cout << "  CRC Errors: " << header->checksumErrors << "\n";
        std::cout << "-------------------------------\n";
    }
};

#endif // MK_RING_BUFFER_CPP
