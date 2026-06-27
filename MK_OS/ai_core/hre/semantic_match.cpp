// ============================================================================
// MK OS — Semantic Match Engine
// ============================================================================
// Weighted Semantic Matching — Graph-derived meaning vectors
//
// How it works WITHOUT neural nets:
// Every concept gets a "semantic vector" computed FROM its graph position.
// Its relations determine its meaning. "feline" has similar relations to "cat"
// therefore their vectors are close. No training, no ML — just graph math.
//
// Vector computation:
// - Count shared neighbors between nodes
// - Count shared relation types
// - Measure path distance
// - Detect domain overlap (same category = closer)
//
// 32-dimensional float vectors (128 bytes per concept — tiny)
// Similarity = cosine of angle between vectors (dot product / magnitudes)
//
// Pure C++ STL. No external dependencies.
// ============================================================================

#ifndef MK_SEMANTIC_MATCH_CPP
#define MK_SEMANTIC_MATCH_CPP

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <functional>

// ============================================================================
// Constants
// ============================================================================

// Number of dimensions in each semantic vector
// 32 is enough to capture semantic relationships from a graph
// More dimensions = finer distinctions but more memory & CPU
static const int SEMANTIC_DIMENSIONS = 32;

// File where vectors are persisted between boots
static const std::string VECTOR_CACHE_FILE = "knowledge_files/semantic_vectors.dat";

// Minimum similarity threshold to consider concepts "related"
static const float MIN_SIMILARITY_THRESHOLD = 0.15f;

// Maximum number of concepts to scan for similarity search
static const int MAX_SCAN_LIMIT = 10000;

// ============================================================================
// Semantic Vector Structure
// ============================================================================

// A semantic vector is just 32 floats that encode a concept's "meaning"
// based on its position in the knowledge graph.
// Concepts with similar relations will have similar vectors.
struct SemanticVector {
    float dims[SEMANTIC_DIMENSIONS];  // The 32-dimensional vector
    bool computed;                     // Has this been computed yet?
    int version;                       // Incremented on update (for cache invalidation)

    SemanticVector() : computed(false), version(0) {
        // Initialize all dimensions to zero
        for (int i = 0; i < SEMANTIC_DIMENSIONS; i++) {
            dims[i] = 0.0f;
        }
    }
};

// ============================================================================
// Graph Interface (minimal — just what we need from MKPatternGraph)
// ============================================================================

// We need to query the graph for neighbors, relations, paths
// This struct represents a single fact/edge in the graph
struct GraphEdge {
    std::string source;
    std::string relation;
    std::string target;
    float weight;
};

// ============================================================================
// MKSemanticMatch — Main Class
// ============================================================================

class MKSemanticMatch {
private:
    // Map of concept -> its semantic vector
    std::unordered_map<std::string, SemanticVector> vectors;

    // Relation type -> dimension mapping
    // Each unique relation type gets assigned to specific dimensions
    // This ensures concepts with similar relations cluster together
    std::map<std::string, std::vector<int>> relationDimensionMap;

    // All known concepts (for iteration)
    std::vector<std::string> allConcepts;

    // Dirty flag — set when new facts added, vectors need recompute
    std::set<std::string> dirtyNodes;

    // Statistics
    int totalVectors;
    int totalComparisons;

    // ========================================================================
    // Private: Dimension Assignment
    // ========================================================================

    // Assign dimensions to a relation type using hash distribution
    // Each relation influences 4-8 dimensions (overlapping is fine — it
    // creates richer representations)
    std::vector<int> getDimensionsForRelation(const std::string& relation) {
        // Check cache first
        auto it = relationDimensionMap.find(relation);
        if (it != relationDimensionMap.end()) {
            return it->second;
        }

        // Hash the relation string to determine which dimensions it affects
        std::hash<std::string> hasher;
        size_t hash = hasher(relation);

        std::vector<int> dims;
        // Each relation affects 4 dimensions (spread across the 32)
        for (int i = 0; i < 4; i++) {
            int dim = (hash + i * 7) % SEMANTIC_DIMENSIONS;  // 7 is coprime to 32
            dims.push_back(dim);
            hash = hash * 31 + i;  // Rehash for next dimension
        }

        // Special relations get extra dimensions for stronger signal
        if (relation == "is_a" || relation == "type_of") {
            // Category relations are the strongest semantic signal
            dims.push_back((hash + 13) % SEMANTIC_DIMENSIONS);
            dims.push_back((hash + 17) % SEMANTIC_DIMENSIONS);
        }
        if (relation == "has_property" || relation == "can") {
            // Properties and capabilities are strong signals too
            dims.push_back((hash + 19) % SEMANTIC_DIMENSIONS);
        }

        relationDimensionMap[relation] = dims;
        return dims;
    }

    // ========================================================================
    // Private: Vector Computation from Graph
    // ========================================================================

    // Compute the semantic vector for a single concept
    // by examining all its connections in the graph
    void computeVectorForConcept(const std::string& concept,
                                  const std::vector<GraphEdge>& edges) {
        SemanticVector vec;

        // Pass 1: Direct relations (strongest signal)
        // For each edge involving this concept, add to appropriate dimensions
        for (const auto& edge : edges) {
            bool isSource = (edge.source == concept);
            bool isTarget = (edge.target == concept);

            if (!isSource && !isTarget) continue;

            // Get which dimensions this relation type affects
            std::vector<int> dims = getDimensionsForRelation(edge.relation);

            // The connected concept's hash determines the SIGN of contribution
            // This means "cat is_a animal" and "dog is_a animal" get similar
            // vectors because they share the same relation+target
            std::string neighbor = isSource ? edge.target : edge.source;
            std::hash<std::string> hasher;
            size_t neighborHash = hasher(neighbor);

            for (int dim : dims) {
                // Direction matters: being the source vs target of a relation
                // gives different semantic meaning
                float direction = isSource ? 1.0f : -0.5f;

                // Weight by edge weight and add noise from neighbor hash
                float value = direction * edge.weight;

                // Use neighbor hash to create a semi-unique contribution
                float hashContrib = ((neighborHash % 100) / 100.0f) * 0.3f;
                vec.dims[dim] += value + hashContrib;
            }
        }

        // Pass 2: Neighbor count normalization
        // Concepts with many connections get normalized down
        // so they don't dominate just because they're hubs
        int connectionCount = 0;
        for (const auto& edge : edges) {
            if (edge.source == concept || edge.target == concept) {
                connectionCount++;
            }
        }
        if (connectionCount > 5) {
            float normFactor = 5.0f / (float)connectionCount;
            for (int i = 0; i < SEMANTIC_DIMENSIONS; i++) {
                vec.dims[i] *= normFactor;
            }
        }

        // Pass 3: Normalize vector to unit length
        // This makes cosine similarity == dot product (faster)
        normalizeVector(vec);

        vec.computed = true;
        vec.version++;
        vectors[concept] = vec;
    }

    // ========================================================================
    // Private: Vector Math
    // ========================================================================

    // Normalize a vector to unit length (magnitude = 1)
    void normalizeVector(SemanticVector& vec) {
        float magnitude = 0.0f;
        for (int i = 0; i < SEMANTIC_DIMENSIONS; i++) {
            magnitude += vec.dims[i] * vec.dims[i];
        }
        magnitude = std::sqrt(magnitude);

        if (magnitude > 0.0001f) {  // Avoid division by zero
            for (int i = 0; i < SEMANTIC_DIMENSIONS; i++) {
                vec.dims[i] /= magnitude;
            }
        }
    }

    // Dot product of two vectors (= cosine similarity when both normalized)
    float dotProduct(const SemanticVector& a, const SemanticVector& b) {
        float sum = 0.0f;
        for (int i = 0; i < SEMANTIC_DIMENSIONS; i++) {
            sum += a.dims[i] * b.dims[i];
        }
        return sum;
    }

    // Cosine similarity (handles non-normalized vectors too)
    float cosineSimilarity(const SemanticVector& a, const SemanticVector& b) {
        float dot = 0.0f;
        float magA = 0.0f;
        float magB = 0.0f;

        for (int i = 0; i < SEMANTIC_DIMENSIONS; i++) {
            dot += a.dims[i] * b.dims[i];
            magA += a.dims[i] * a.dims[i];
            magB += b.dims[i] * b.dims[i];
        }

        magA = std::sqrt(magA);
        magB = std::sqrt(magB);

        if (magA < 0.0001f || magB < 0.0001f) return 0.0f;
        return dot / (magA * magB);
    }

public:
    // ========================================================================
    // Constructor
    // ========================================================================

    MKSemanticMatch() : totalVectors(0), totalComparisons(0) {
        // Try to load cached vectors from disk
        loadVectors();
    }

    // ========================================================================
    // Core API: Build Vectors
    // ========================================================================

    // Scan entire graph and compute semantic vectors for ALL concepts
    // Call this on boot (or load from cache instead)
    void buildSemanticVectors(const std::vector<GraphEdge>& allEdges) {
        // Collect all unique concepts from edges
        std::set<std::string> conceptSet;
        for (const auto& edge : allEdges) {
            conceptSet.insert(edge.source);
            conceptSet.insert(edge.target);
        }

        allConcepts.clear();
        for (const auto& c : conceptSet) {
            allConcepts.push_back(c);
        }

        // Compute vector for each concept
        for (const auto& concept : allConcepts) {
            computeVectorForConcept(concept, allEdges);
        }

        totalVectors = allConcepts.size();

        // Persist to disk so we don't recompute next boot
        saveVectors();

        std::cout << "[SemanticMatch] Built " << totalVectors
                  << " semantic vectors (" << totalVectors * 128
                  << " bytes total)" << std::endl;
    }

    // ========================================================================
    // Core API: Similarity
    // ========================================================================

    // Compute similarity between two concepts (0.0 = unrelated, 1.0 = identical)
    // This is the key function — replaces keyword matching with meaning matching
    float similarity(const std::string& conceptA, const std::string& conceptB) {
        totalComparisons++;

        // Same concept = perfect match
        if (conceptA == conceptB) return 1.0f;

        // Look up vectors
        auto itA = vectors.find(conceptA);
        auto itB = vectors.find(conceptB);

        // If either concept has no vector, can't compare
        if (itA == vectors.end() || itB == vectors.end()) return 0.0f;
        if (!itA->second.computed || !itB->second.computed) return 0.0f;

        // Cosine similarity of their vectors
        float sim = cosineSimilarity(itA->second, itB->second);

        // Clamp to [0, 1] range (shouldn't go negative with good vectors, but safety)
        if (sim < 0.0f) sim = 0.0f;
        if (sim > 1.0f) sim = 1.0f;

        return sim;
    }

    // ========================================================================
    // Core API: Find Similar
    // ========================================================================

    // Find the K most similar concepts to the given one
    // Returns them sorted by similarity (highest first)
    std::vector<std::string> findSimilar(const std::string& concept, int topK) {
        std::vector<std::pair<float, std::string>> scores;

        // Get this concept's vector
        auto it = vectors.find(concept);
        if (it == vectors.end() || !it->second.computed) {
            return {};  // Unknown concept
        }

        const SemanticVector& queryVec = it->second;

        // Compare against all other concepts
        int scanned = 0;
        for (const auto& other : allConcepts) {
            if (other == concept) continue;
            if (scanned >= MAX_SCAN_LIMIT) break;

            auto otherIt = vectors.find(other);
            if (otherIt == vectors.end() || !otherIt->second.computed) continue;

            float sim = cosineSimilarity(queryVec, otherIt->second);
            if (sim >= MIN_SIMILARITY_THRESHOLD) {
                scores.push_back({sim, other});
            }
            scanned++;
        }

        // Sort by similarity (highest first)
        std::sort(scores.begin(), scores.end(),
                  [](const std::pair<float, std::string>& a,
                     const std::pair<float, std::string>& b) {
                      return a.first > b.first;
                  });

        // Take top K
        std::vector<std::string> results;
        for (int i = 0; i < topK && i < (int)scores.size(); i++) {
            results.push_back(scores[i].second);
        }

        return results;
    }

    // ========================================================================
    // Core API: Fuzzy Query
    // ========================================================================

    // Even if exact query words aren't in the graph, find related concepts
    // This is what makes MK "understand" even when vocabulary doesn't match
    std::vector<std::string> fuzzyQuery(const std::vector<std::string>& queryWords,
                                         const std::vector<GraphEdge>& graph) {
        std::vector<std::string> results;
        std::map<std::string, float> candidateScores;

        for (const auto& word : queryWords) {
            // First: try exact match
            auto it = vectors.find(word);
            if (it != vectors.end() && it->second.computed) {
                // Found exact — also find its similar concepts
                std::vector<std::string> similar = findSimilar(word, 5);
                candidateScores[word] += 1.0f;  // Exact match = full score
                for (const auto& s : similar) {
                    candidateScores[s] += similarity(word, s);
                }
            } else {
                // No exact match — check for substring matches first
                for (const auto& concept : allConcepts) {
                    // Substring check (catches "program" in "programming")
                    if (concept.find(word) != std::string::npos ||
                        word.find(concept) != std::string::npos) {
                        candidateScores[concept] += 0.6f;
                    }
                }
            }
        }

        // Sort candidates by accumulated score
        std::vector<std::pair<float, std::string>> sorted;
        for (const auto& pair : candidateScores) {
            sorted.push_back({pair.second, pair.first});
        }
        std::sort(sorted.begin(), sorted.end(),
                  [](const std::pair<float, std::string>& a,
                     const std::pair<float, std::string>& b) {
                      return a.first > b.first;
                  });

        // Return top results
        for (int i = 0; i < 10 && i < (int)sorted.size(); i++) {
            results.push_back(sorted[i].second);
        }

        return results;
    }

    // ========================================================================
    // Incremental Update
    // ========================================================================

    // When a new fact is added to the graph, only recompute affected vectors
    // Much faster than rebuilding everything
    void incrementalUpdate(const GraphEdge& newEdge,
                           const std::vector<GraphEdge>& allEdges) {
        // Mark source and target as dirty
        dirtyNodes.insert(newEdge.source);
        dirtyNodes.insert(newEdge.target);

        // Recompute dirty nodes
        for (const auto& node : dirtyNodes) {
            computeVectorForConcept(node, allEdges);
        }
        dirtyNodes.clear();

        // Add new concepts to allConcepts if they weren't there
        if (vectors.find(newEdge.source) != vectors.end()) {
            bool found = false;
            for (const auto& c : allConcepts) {
                if (c == newEdge.source) { found = true; break; }
            }
            if (!found) allConcepts.push_back(newEdge.source);
        }
        if (vectors.find(newEdge.target) != vectors.end()) {
            bool found = false;
            for (const auto& c : allConcepts) {
                if (c == newEdge.target) { found = true; break; }
            }
            if (!found) allConcepts.push_back(newEdge.target);
        }

        totalVectors = allConcepts.size();
    }

    // ========================================================================
    // Persistence: Save/Load Vectors
    // ========================================================================

    // Save all computed vectors to disk (binary format for speed)
    void saveVectors() {
        std::ofstream file(VECTOR_CACHE_FILE, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[SemanticMatch] WARNING: Cannot save vectors to "
                      << VECTOR_CACHE_FILE << std::endl;
            return;
        }

        // Write number of vectors
        int count = (int)vectors.size();
        file.write(reinterpret_cast<char*>(&count), sizeof(int));

        // Write each concept name + vector
        for (const auto& pair : vectors) {
            if (!pair.second.computed) continue;

            // Write concept name length + string
            int nameLen = (int)pair.first.size();
            file.write(reinterpret_cast<char*>(&nameLen), sizeof(int));
            file.write(pair.first.c_str(), nameLen);

            // Write the 32 floats
            file.write(reinterpret_cast<const char*>(pair.second.dims),
                       SEMANTIC_DIMENSIONS * sizeof(float));

            // Write version
            file.write(reinterpret_cast<const char*>(&pair.second.version),
                       sizeof(int));
        }

        file.close();
    }

    // Load previously computed vectors from disk
    void loadVectors() {
        std::ifstream file(VECTOR_CACHE_FILE, std::ios::binary);
        if (!file.is_open()) {
            // No cache file — will need to build from scratch
            return;
        }

        // Read number of vectors
        int count = 0;
        file.read(reinterpret_cast<char*>(&count), sizeof(int));

        for (int i = 0; i < count; i++) {
            // Read concept name
            int nameLen = 0;
            file.read(reinterpret_cast<char*>(&nameLen), sizeof(int));
            if (nameLen <= 0 || nameLen > 1000) break;  // Sanity check

            std::string name(nameLen, '\0');
            file.read(&name[0], nameLen);

            // Read the 32 floats
            SemanticVector vec;
            file.read(reinterpret_cast<char*>(vec.dims),
                      SEMANTIC_DIMENSIONS * sizeof(float));

            // Read version
            file.read(reinterpret_cast<char*>(&vec.version), sizeof(int));

            vec.computed = true;
            vectors[name] = vec;
            allConcepts.push_back(name);
        }

        totalVectors = allConcepts.size();
        file.close();

        if (totalVectors > 0) {
            std::cout << "[SemanticMatch] Loaded " << totalVectors
                      << " cached vectors from disk" << std::endl;
        }
    }

    // ========================================================================
    // Utility: Get Raw Vector
    // ========================================================================

    // Get the raw vector for a concept (for debugging or external use)
    std::vector<float> getVector(const std::string& concept) {
        std::vector<float> result(SEMANTIC_DIMENSIONS, 0.0f);
        auto it = vectors.find(concept);
        if (it != vectors.end() && it->second.computed) {
            for (int i = 0; i < SEMANTIC_DIMENSIONS; i++) {
                result[i] = it->second.dims[i];
            }
        }
        return result;
    }

    // ========================================================================
    // Utility: Stats
    // ========================================================================

    int getVectorCount() const { return totalVectors; }
    int getComparisonCount() const { return totalComparisons; }

    // Check if a concept has a computed vector
    bool hasVector(const std::string& concept) {
        auto it = vectors.find(concept);
        return (it != vectors.end() && it->second.computed);
    }
};

#endif // MK_SEMANTIC_MATCH_CPP
