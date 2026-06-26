#include <iostream>
#include <vector>
#include "cache_mgr.cpp"

int main() {
    std::cout << "[NEXUS CORE] Running Brain Cache Validation Rig...\n\n";

    // Initialize the manager with a tight, fixed 2-slot window to force evictions quickly
    MKCacheManager cache(2);

    // Create dummy 4-bit quantized byte streams for three model layers
    std::vector<uint8_t> layer1Data = {0x1A, 0x2B, 0x3C, 0x4D};
    std::vector<uint8_t> layer2Data = {0x5E, 0x6F, 0x7A, 0x8B};
    std::vector<uint8_t> layer3Data = {0x9C, 0xAD, 0xBE, 0xCF};

    // 1. Fill the cache slots
    cache.cacheHotLayer("transformer_layer_0", layer1Data);
    cache.cacheHotLayer("transformer_layer_1", layer2Data);

    // 2. Mark a layer as inactive (cold) so it can be recycled
    std::cout << "\n[SYSTEM] Execution moving past Layer 0. Setting it to cold...\n";
    cache.setLayerCold("transformer_layer_0");

    // 3. Load a new layer to trigger an eviction of the cold layer
    std::cout << "\n[SYSTEM] Loading Layer 2 into pipeline...\n";
    cache.cacheHotLayer("transformer_layer_2", layer3Data);

    std::cout << "\n[NEXUS CORE] Cache cycle validation complete.\n";
    return 0;
}