// ============================================================================
// MK OS — Context Embedding Graph
// ============================================================================
// Creates a semantic SPACE where every concept has a position.
// Position determined by: what it IS, what it HAS, what it CAN do, where USED.
//
// Each dimension represents a property axis (living/nonliving, physical/abstract)
// Concepts close in space = semantically related.
// Can answer "what's similar to X?" for things NEVER explicitly linked.
//
// 64-dimensional space (256 bytes per concept)
// Property axes auto-discovered from graph patterns (not hard-coded)
//
// Pure C++ STL. No external dependencies.
// ============================================================================

#ifndef MK_CONTEXT_EMBEDDING_CPP
#define MK_CONTEXT_EMBEDDING_CPP

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

// ============================================================================
// Constants
// ============================================================================

static const int EMBEDDING_DIMENSIONS = 64;
static const float EMBEDDING_LEARNING_RATE = 0.1f;
static const int MAX_ITERATIONS = 50;
static const std::string EMBEDDING_CACHE = "knowledge_files/embeddings.dat";

// ============================================================================
// Structures
// ============================================================================

// A property axis discovered from the graph
// e.g., "living vs nonliving", "physical vs abstract"
struct PropertyAxis {
    int dimensionIndex;           // Which dimension this axis occupies
    std::string positivePole;     // Concepts at +1 end (e.g., "living")
    std::string negativePole;     // Concepts at -1 end (e.g., "nonliving")
    float importance;             // How much this axis differentiates concepts
    std::vector<std::string> positiveExemplars;  // Known positive examples
    std::vector<std::string> negativeExemplars;  // Known negative examples
};

// The embedding space containing all vectors and axes
struct EmbeddingSpace {
    int dimensions;
    std::unordered_map<std::string, std::vector<float>> vectors;
    std::vector<PropertyAxis> axes;
    int conceptCount;

    EmbeddingSpace() : dimensions(EMBEDDING_DIMENSIONS), conceptCount(0) {}
};

// Graph edge (consistent with other modules)
struct EmbGraphEdge {
    std::string source;
    std::string relation;
    std::string target;
    float weight;
};


// ============================================================================
// MKContextEmbedding — Main Class
// ============================================================================

class MKContextEmbedding {
private:
    EmbeddingSpace space;

    // Relations that define IS-A hierarchy (strong category signal)
    std::set<std::string> categoryRelations;

    // Relations that define properties (HAS, CAN)
    std::set<std::string> propertyRelations;

    // Relations that define usage/context
    std::set<std::string> contextRelations;

    // ========================================================================
    // Private: Initialization
    // ========================================================================

    void initRelationSets() {
        categoryRelations = {"is_a", "type_of", "kind_of", "instance_of", "category"};
        propertyRelations = {"has", "has_property", "can", "capable_of", "has_part"};
        contextRelations = {"used_in", "found_in", "part_of", "related_to", "domain"};
    }

    // ========================================================================
    // Private: Axis Discovery
    // ========================================================================

    // Auto-discover property axes from graph structure
    // Finds dimensions that naturally separate concepts
    void discoverAxes(const std::vector<EmbGraphEdge>& graph) {
        // Strategy: Find properties that split concepts into groups
        // If "living" things share many connections that "nonliving" don't,
        // that's a natural axis

        // Step 1: Find all unique property values
        std::map<std::string, std::set<std::string>> propertyToHolders;
        for (const auto& edge : graph) {
            if (propertyRelations.count(edge.relation) > 0) {
                propertyToHolders[edge.target].insert(edge.source);
            }
        }

        // Step 2: Properties with many holders become axes
        int axisIdx = 0;
        for (const auto& pair : propertyToHolders) {
            if (pair.second.size() >= 2 && axisIdx < EMBEDDING_DIMENSIONS) {
                PropertyAxis axis;
                axis.dimensionIndex = axisIdx;
                axis.positivePole = pair.first;
                axis.negativePole = "not_" + pair.first;
                axis.importance = (float)pair.second.size();
                for (const auto& holder : pair.second) {
                    axis.positiveExemplars.push_back(holder);
                }
                space.axes.push_back(axis);
                axisIdx++;
            }
        }

        // Step 3: Fill remaining dimensions with category-based axes
        std::map<std::string, std::set<std::string>> categoryMembers;
        for (const auto& edge : graph) {
            if (categoryRelations.count(edge.relation) > 0) {
                categoryMembers[edge.target].insert(edge.source);
            }
        }
        for (const auto& pair : categoryMembers) {
            if (axisIdx >= EMBEDDING_DIMENSIONS) break;
            if (pair.second.size() >= 2) {
                PropertyAxis axis;
                axis.dimensionIndex = axisIdx;
                axis.positivePole = pair.first;
                axis.negativePole = "not_" + pair.first;
                axis.importance = (float)pair.second.size() * 1.5f; // Categories weigh more
                for (const auto& member : pair.second) {
                    axis.positiveExemplars.push_back(member);
                }
                space.axes.push_back(axis);
                axisIdx++;
            }
        }

        std::cout << "[Embedding] Discovered " << space.axes.size()
                  << " property axes" << std::endl;
    }


    // ========================================================================
    // Private: Vector Computation
    // ========================================================================

    // Compute embedding for a single concept based on its graph relations
    std::vector<float> computeEmbedding(const std::string& concept,
                                         const std::vector<EmbGraphEdge>& graph) {
        std::vector<float> vec(EMBEDDING_DIMENSIONS, 0.0f);

        // For each axis, determine where this concept falls
        for (const auto& axis : space.axes) {
            int dim = axis.dimensionIndex;
            if (dim >= EMBEDDING_DIMENSIONS) continue;

            // Check if concept is a positive exemplar
            bool isPositive = false;
            for (const auto& ex : axis.positiveExemplars) {
                if (ex == concept) { isPositive = true; break; }
            }
            if (isPositive) {
                vec[dim] = 1.0f * axis.importance;
                continue;
            }

            // Check if concept is a negative exemplar
            bool isNegative = false;
            for (const auto& ex : axis.negativeExemplars) {
                if (ex == concept) { isNegative = true; break; }
            }
            if (isNegative) {
                vec[dim] = -1.0f * axis.importance;
                continue;
            }

            // Neither — check if concept is connected to positive exemplars
            float score = 0.0f;
            int connections = 0;
            for (const auto& edge : graph) {
                if (edge.source == concept || edge.target == concept) {
                    std::string neighbor = (edge.source == concept) ?
                                           edge.target : edge.source;
                    // Is neighbor a positive exemplar?
                    for (const auto& ex : axis.positiveExemplars) {
                        if (ex == neighbor) {
                            score += edge.weight * 0.5f;
                            connections++;
                        }
                    }
                    // Is neighbor a negative exemplar?
                    for (const auto& ex : axis.negativeExemplars) {
                        if (ex == neighbor) {
                            score -= edge.weight * 0.5f;
                            connections++;
                        }
                    }
                }
            }

            if (connections > 0) {
                vec[dim] = score / (float)connections;
            }
        }

        // Additional: encode connection density in unused dimensions
        int connectionCount = 0;
        std::set<std::string> neighborSet;
        for (const auto& edge : graph) {
            if (edge.source == concept) {
                connectionCount++;
                neighborSet.insert(edge.target);
            } else if (edge.target == concept) {
                connectionCount++;
                neighborSet.insert(edge.source);
            }
        }

        // Use last few dimensions for structural properties
        if (EMBEDDING_DIMENSIONS > 4) {
            vec[EMBEDDING_DIMENSIONS - 1] = (float)connectionCount * 0.1f;
            vec[EMBEDDING_DIMENSIONS - 2] = (float)neighborSet.size() * 0.1f;
        }

        // Normalize to unit vector
        float mag = 0.0f;
        for (int i = 0; i < EMBEDDING_DIMENSIONS; i++) {
            mag += vec[i] * vec[i];
        }
        mag = std::sqrt(mag);
        if (mag > 0.001f) {
            for (int i = 0; i < EMBEDDING_DIMENSIONS; i++) {
                vec[i] /= mag;
            }
        }

        return vec;
    }

    // ========================================================================
    // Private: Distance Calculation
    // ========================================================================

    float euclideanDistance(const std::vector<float>& a, const std::vector<float>& b) {
        float sum = 0.0f;
        int dims = std::min((int)a.size(), (int)b.size());
        for (int i = 0; i < dims; i++) {
            float diff = a[i] - b[i];
            sum += diff * diff;
        }
        return std::sqrt(sum);
    }

    float cosineDistance(const std::vector<float>& a, const std::vector<float>& b) {
        float dot = 0.0f, magA = 0.0f, magB = 0.0f;
        int dims = std::min((int)a.size(), (int)b.size());
        for (int i = 0; i < dims; i++) {
            dot += a[i] * b[i];
            magA += a[i] * a[i];
            magB += b[i] * b[i];
        }
        magA = std::sqrt(magA);
        magB = std::sqrt(magB);
        if (magA < 0.001f || magB < 0.001f) return 1.0f; // Max distance
        return 1.0f - (dot / (magA * magB)); // Convert similarity to distance
    }


public:
    // ========================================================================
    // Constructor
    // ========================================================================

    MKContextEmbedding() {
        initRelationSets();
        loadEmbeddings();
    }

    // ========================================================================
    // Core API: Build Space
    // ========================================================================

    // Compute positions for all concepts in the graph
    void buildSpace(const std::vector<EmbGraphEdge>& graph) {
        // Step 1: Discover axes from graph structure
        discoverAxes(graph);

        // Step 2: Collect all concepts
        std::set<std::string> conceptSet;
        for (const auto& edge : graph) {
            conceptSet.insert(edge.source);
            conceptSet.insert(edge.target);
        }

        // Step 3: Compute embedding for each concept
        for (const auto& concept : conceptSet) {
            space.vectors[concept] = computeEmbedding(concept, graph);
        }

        space.conceptCount = (int)conceptSet.size();

        // Step 4: Iterative refinement — pull connected concepts closer
        for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
            float totalDelta = 0.0f;

            for (const auto& edge : graph) {
                auto srcIt = space.vectors.find(edge.source);
                auto tgtIt = space.vectors.find(edge.target);
                if (srcIt == space.vectors.end() || tgtIt == space.vectors.end()) continue;

                // Connected concepts should be closer
                // Pull them together by a small amount
                float pullStrength = EMBEDDING_LEARNING_RATE * edge.weight;

                for (int d = 0; d < EMBEDDING_DIMENSIONS; d++) {
                    float diff = tgtIt->second[d] - srcIt->second[d];
                    float delta = diff * pullStrength;
                    srcIt->second[d] += delta;
                    tgtIt->second[d] -= delta * 0.5f;
                    totalDelta += std::abs(delta);
                }
            }

            // Convergence check
            if (totalDelta < 0.001f) break;
        }

        // Re-normalize all vectors after refinement
        for (auto& pair : space.vectors) {
            float mag = 0.0f;
            for (int i = 0; i < EMBEDDING_DIMENSIONS; i++) {
                mag += pair.second[i] * pair.second[i];
            }
            mag = std::sqrt(mag);
            if (mag > 0.001f) {
                for (int i = 0; i < EMBEDDING_DIMENSIONS; i++) {
                    pair.second[i] /= mag;
                }
            }
        }

        saveEmbeddings();

        std::cout << "[Embedding] Built " << space.conceptCount
                  << " embeddings in " << EMBEDDING_DIMENSIONS << "D space ("
                  << space.conceptCount * EMBEDDING_DIMENSIONS * 4
                  << " bytes)" << std::endl;
    }

    // ========================================================================
    // Core API: Get Embedding
    // ========================================================================

    // Get the position vector for a concept
    std::vector<float> getEmbedding(const std::string& concept) {
        auto it = space.vectors.find(concept);
        if (it != space.vectors.end()) {
            return it->second;
        }
        return std::vector<float>(EMBEDDING_DIMENSIONS, 0.0f);
    }

    // ========================================================================
    // Core API: Distance
    // ========================================================================

    // How far apart in meaning (0.0 = same, 2.0 = maximally different)
    float distance(const std::string& a, const std::string& b) {
        if (a == b) return 0.0f;
        auto itA = space.vectors.find(a);
        auto itB = space.vectors.find(b);
        if (itA == space.vectors.end() || itB == space.vectors.end()) return 2.0f;
        return cosineDistance(itA->second, itB->second);
    }

    // ========================================================================
    // Core API: Neighborhood
    // ========================================================================

    // Find everything within semantic radius of a concept
    std::vector<std::string> neighborhood(const std::string& concept, float radius) {
        std::vector<std::string> results;
        auto it = space.vectors.find(concept);
        if (it == space.vectors.end()) return results;

        const std::vector<float>& center = it->second;

        for (const auto& pair : space.vectors) {
            if (pair.first == concept) continue;
            float dist = cosineDistance(center, pair.second);
            if (dist <= radius) {
                results.push_back(pair.first);
            }
        }

        // Sort by distance (closest first)
        std::sort(results.begin(), results.end(),
                  [this, &concept](const std::string& a, const std::string& b) {
                      return distance(concept, a) < distance(concept, b);
                  });

        return results;
    }


    // ========================================================================
    // Core API: Infer Category
    // ========================================================================

    // Guess what category an unknown concept belongs to
    // by finding its nearest neighbors and their categories
    std::string inferCategory(const std::string& unknownConcept,
                              const std::vector<EmbGraphEdge>& graph) {
        // Find nearest neighbors in embedding space
        std::vector<std::string> neighbors = neighborhood(unknownConcept, 0.5f);
        if (neighbors.empty()) {
            // Try wider radius
            neighbors = neighborhood(unknownConcept, 1.0f);
        }
        if (neighbors.empty()) return "unknown";

        // Count categories of neighbors (vote-based inference)
        std::map<std::string, int> categoryVotes;
        for (const auto& neighbor : neighbors) {
            // Find what category this neighbor belongs to
            for (const auto& edge : graph) {
                if (edge.source == neighbor && categoryRelations.count(edge.relation) > 0) {
                    categoryVotes[edge.target]++;
                }
            }
        }

        // Return the category with most votes
        std::string bestCategory = "unknown";
        int bestVotes = 0;
        for (const auto& pair : categoryVotes) {
            if (pair.second > bestVotes) {
                bestVotes = pair.second;
                bestCategory = pair.first;
            }
        }

        return bestCategory;
    }

    // ========================================================================
    // Core API: Analogy Completion
    // ========================================================================

    // "a is to b as c is to ?" — vector arithmetic on embeddings
    // king - man + woman = queen (classic analogy test)
    std::vector<std::pair<std::string, float>> analogyComplete(
            const std::string& a, const std::string& b, const std::string& c,
            int topK = 5) {
        std::vector<std::pair<std::string, float>> results;

        auto itA = space.vectors.find(a);
        auto itB = space.vectors.find(b);
        auto itC = space.vectors.find(c);

        if (itA == space.vectors.end() ||
            itB == space.vectors.end() ||
            itC == space.vectors.end()) {
            return results;
        }

        // Compute target vector: b - a + c
        // "The relationship from a to b, applied to c"
        std::vector<float> target(EMBEDDING_DIMENSIONS);
        for (int i = 0; i < EMBEDDING_DIMENSIONS; i++) {
            target[i] = itB->second[i] - itA->second[i] + itC->second[i];
        }

        // Normalize target
        float mag = 0.0f;
        for (int i = 0; i < EMBEDDING_DIMENSIONS; i++) {
            mag += target[i] * target[i];
        }
        mag = std::sqrt(mag);
        if (mag > 0.001f) {
            for (int i = 0; i < EMBEDDING_DIMENSIONS; i++) {
                target[i] /= mag;
            }
        }

        // Find concepts closest to target (excluding a, b, c)
        std::vector<std::pair<float, std::string>> scored;
        for (const auto& pair : space.vectors) {
            if (pair.first == a || pair.first == b || pair.first == c) continue;
            float dist = cosineDistance(target, pair.second);
            scored.push_back({dist, pair.first});
        }

        std::sort(scored.begin(), scored.end());

        for (int i = 0; i < topK && i < (int)scored.size(); i++) {
            float similarity = 1.0f - scored[i].first;
            results.push_back({scored[i].second, similarity});
        }

        return results;
    }

    // ========================================================================
    // Persistence
    // ========================================================================

    void saveEmbeddings() {
        std::ofstream file(EMBEDDING_CACHE, std::ios::binary);
        if (!file.is_open()) return;

        int count = (int)space.vectors.size();
        file.write(reinterpret_cast<char*>(&count), sizeof(int));

        for (const auto& pair : space.vectors) {
            int nameLen = (int)pair.first.size();
            file.write(reinterpret_cast<char*>(&nameLen), sizeof(int));
            file.write(pair.first.c_str(), nameLen);
            file.write(reinterpret_cast<const char*>(pair.second.data()),
                       EMBEDDING_DIMENSIONS * sizeof(float));
        }
        file.close();
    }

    void loadEmbeddings() {
        std::ifstream file(EMBEDDING_CACHE, std::ios::binary);
        if (!file.is_open()) return;

        int count = 0;
        file.read(reinterpret_cast<char*>(&count), sizeof(int));

        for (int i = 0; i < count; i++) {
            int nameLen = 0;
            file.read(reinterpret_cast<char*>(&nameLen), sizeof(int));
            if (nameLen <= 0 || nameLen > 1000) break;

            std::string name(nameLen, '\0');
            file.read(&name[0], nameLen);

            std::vector<float> vec(EMBEDDING_DIMENSIONS);
            file.read(reinterpret_cast<char*>(vec.data()),
                      EMBEDDING_DIMENSIONS * sizeof(float));

            space.vectors[name] = vec;
        }

        space.conceptCount = (int)space.vectors.size();
        file.close();

        if (space.conceptCount > 0) {
            std::cout << "[Embedding] Loaded " << space.conceptCount
                      << " cached embeddings" << std::endl;
        }
    }

    // ========================================================================
    // Utility
    // ========================================================================

    int getConceptCount() const { return space.conceptCount; }
    int getDimensions() const { return space.dimensions; }
    int getAxisCount() const { return (int)space.axes.size(); }
};

#endif // MK_CONTEXT_EMBEDDING_CPP
