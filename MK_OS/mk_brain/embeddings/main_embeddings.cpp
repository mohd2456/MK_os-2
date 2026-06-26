#include <iostream>
#include <vector>
#include "embeddings_eng.cpp"

int main() {
    std::cout << "[NEXUS CORE] Running Embeddings Engine Validation Rig...\n\n";

    MKEmbeddingsEngine engine;

    // 1. Fetch vector footprints for matching tokens
    std::vector<float> nexusVec = engine.getEmbedding("nexus");
    std::vector<float> coreVec = engine.getEmbedding("core");
    std::vector<float> bootVec = engine.getEmbedding("boot");

    // 2. Perform distance verification math
    float nexusToCore = engine.calculateSimilarity(nexusVec, coreVec);
    float nexusToBoot = engine.calculateSimilarity(nexusVec, bootVec);

    std::cout << "\n[MATH MATRIX] Semantic Similarity Computations:\n";
    std::cout << "-> Similarity (nexus <-> core): " << nexusToCore << " (Expected High ContextMatch)\n";
    std::cout << "-> Similarity (nexus <-> boot): " << nexusToBoot << " (Expected Low ContextMatch)\n";

    std::cout << "\n[NEXUS CORE] Embeddings pipeline verification complete.\n";
    return 0;
}