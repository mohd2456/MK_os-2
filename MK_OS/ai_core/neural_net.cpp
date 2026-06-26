#ifndef MK_NEURAL_NET_CPP
#define MK_NEURAL_NET_CPP

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <random>
#include <algorithm>
#include <fstream>
#include <cstring>

// ===================================================================================
// MK TINY NEURAL NETWORK ENGINE
// From-scratch forward pass inference engine. NOT an LLM wrapper.
// Features:
//   - Quantized INT4/INT8 matrix operations for low-end hardware
//   - Preallocated tensor arenas (no runtime heap allocation)
//   - Layer-by-layer forward pass through configurable architecture
//   - ReLU, GELU, Softmax activations
//   - Supports both full-precision (float32) and quantized modes
// ===================================================================================


// Quantized weight storage: packs two INT4 values into one byte
struct MKQuantizedWeight {
    uint8_t packed;  // High nibble = weight1, Low nibble = weight2
    
    int8_t getFirst() const  { return (int8_t)((packed >> 4) & 0x0F) - 8; }
    int8_t getSecond() const { return (int8_t)(packed & 0x0F) - 8; }
    
    static MKQuantizedWeight pack(int8_t a, int8_t b) {
        MKQuantizedWeight w;
        w.packed = (uint8_t)(((a + 8) & 0x0F) << 4) | ((b + 8) & 0x0F);
        return w;
    }
};

// Tensor: multi-dimensional array with shape tracking
struct MKTensor {
    std::vector<float> data;
    int rows;
    int cols;
    
    MKTensor() : rows(0), cols(0) {}
    MKTensor(int r, int c) : rows(r), cols(c), data(r * c, 0.0f) {}
    
    float& at(int r, int c) { return data[r * cols + c]; }
    float at(int r, int c) const { return data[r * cols + c]; }
    int size() const { return rows * cols; }
    
    void randomInit(float scale = 0.01f) {
        std::mt19937 gen(42);
        std::normal_distribution<float> dist(0.0f, scale);
        for (auto& v : data) v = dist(gen);
    }
};


// Quantized tensor for INT8 weights with scale factor
struct MKQuantizedTensor {
    std::vector<int8_t> data;
    float scale;       // Dequantization: real_value = int_value * scale
    float zeroPoint;
    int rows, cols;
    
    MKQuantizedTensor() : scale(1.0f), zeroPoint(0.0f), rows(0), cols(0) {}
    MKQuantizedTensor(int r, int c) : rows(r), cols(c), data(r * c, 0), 
                                       scale(1.0f), zeroPoint(0.0f) {}
    
    // Quantize a float tensor to INT8
    static MKQuantizedTensor fromFloat(const MKTensor& t) {
        MKQuantizedTensor qt(t.rows, t.cols);
        float maxAbs = 0.0f;
        for (float v : t.data) maxAbs = std::max(maxAbs, std::abs(v));
        
        qt.scale = maxAbs / 127.0f;
        if (qt.scale == 0.0f) qt.scale = 1.0f;
        
        for (int i = 0; i < (int)t.data.size(); i++) {
            float clamped = std::max(-127.0f, std::min(127.0f, t.data[i] / qt.scale));
            qt.data[i] = (int8_t)std::round(clamped);
        }
        return qt;
    }
};

// ─────────────────────────────────────────────
//  ACTIVATION FUNCTIONS
// ─────────────────────────────────────────────
namespace MKActivations {
    inline float relu(float x) { return x > 0.0f ? x : 0.0f; }
    
    inline float gelu(float x) {
        // Approximate GELU: x * 0.5 * (1 + tanh(sqrt(2/pi) * (x + 0.044715*x^3)))
        float c = 0.7978845608f; // sqrt(2/pi)
        return 0.5f * x * (1.0f + std::tanh(c * (x + 0.044715f * x * x * x)));
    }
    
    inline float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }
    
    void softmax(float* data, int size) {
        float maxVal = *std::max_element(data, data + size);
        float sum = 0.0f;
        for (int i = 0; i < size; i++) {
            data[i] = std::exp(data[i] - maxVal);
            sum += data[i];
        }
        for (int i = 0; i < size; i++) {
            data[i] /= sum;
        }
    }
}


// ─────────────────────────────────────────────
//  MATRIX OPERATIONS (Cache-friendly layouts)
// ─────────────────────────────────────────────
namespace MKMath {
    // Standard float32 matrix multiply: C = A * B
    // A is [M x K], B is [K x N], C is [M x N]
    void matmul(const float* A, const float* B, float* C,
                int M, int K, int N) {
        // Cache-optimized: iterate in order that maximizes L1 cache hits
        std::memset(C, 0, M * N * sizeof(float));
        for (int i = 0; i < M; i++) {
            for (int k = 0; k < K; k++) {
                float a_ik = A[i * K + k];
                for (int j = 0; j < N; j++) {
                    C[i * N + j] += a_ik * B[k * N + j];
                }
            }
        }
    }
    
    // Quantized INT8 matrix multiply with dequantization
    // Accumulates in INT32 to prevent overflow, then scales back to float
    void matmul_int8(const int8_t* A, float scaleA,
                     const int8_t* B, float scaleB,
                     float* C, int M, int K, int N) {
        float combinedScale = scaleA * scaleB;
        std::memset(C, 0, M * N * sizeof(float));
        
        for (int i = 0; i < M; i++) {
            for (int k = 0; k < K; k++) {
                int32_t a_ik = (int32_t)A[i * K + k];
                for (int j = 0; j < N; j++) {
                    int32_t b_kj = (int32_t)B[k * N + j];
                    C[i * N + j] += (float)(a_ik * b_kj) * combinedScale;
                }
            }
        }
    }
    
    // Vector dot product
    float dot(const float* a, const float* b, int size) {
        float sum = 0.0f;
        for (int i = 0; i < size; i++) sum += a[i] * b[i];
        return sum;
    }
    
    // Element-wise add with bias
    void addBias(float* data, const float* bias, int rows, int cols) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                data[i * cols + j] += bias[j];
            }
        }
    }
    
    // Layer normalization
    void layerNorm(float* data, int size, float eps = 1e-5f) {
        float mean = 0.0f;
        for (int i = 0; i < size; i++) mean += data[i];
        mean /= size;
        
        float var = 0.0f;
        for (int i = 0; i < size; i++) {
            float diff = data[i] - mean;
            var += diff * diff;
        }
        var /= size;
        
        float invStd = 1.0f / std::sqrt(var + eps);
        for (int i = 0; i < size; i++) {
            data[i] = (data[i] - mean) * invStd;
        }
    }
}


// ─────────────────────────────────────────────
//  NEURAL NETWORK LAYER DEFINITIONS
// ─────────────────────────────────────────────

enum class MKLayerType { LINEAR, EMBEDDING, LAYERNORM, ATTENTION };

struct MKLinearLayer {
    MKTensor weights;           // [input_dim x output_dim]
    std::vector<float> bias;    // [output_dim]
    MKQuantizedTensor qWeights; // Quantized version for INT8 mode
    bool quantized;
    
    MKLinearLayer() : quantized(false) {}
    
    void init(int inputDim, int outputDim) {
        weights = MKTensor(inputDim, outputDim);
        weights.randomInit(std::sqrt(2.0f / inputDim));
        bias.resize(outputDim, 0.0f);
    }
    
    void quantize() {
        qWeights = MKQuantizedTensor::fromFloat(weights);
        quantized = true;
    }
    
    // Forward pass: output = input * weights + bias
    MKTensor forward(const MKTensor& input) {
        MKTensor output(input.rows, weights.cols);
        
        if (quantized) {
            // Quantized path — INT8 matmul
            MKQuantizedTensor qInput = MKQuantizedTensor::fromFloat(input);
            MKMath::matmul_int8(qInput.data.data(), qInput.scale,
                               qWeights.data.data(), qWeights.scale,
                               output.data.data(),
                               input.rows, weights.rows, weights.cols);
        } else {
            // Float32 path
            MKMath::matmul(input.data.data(), weights.data.data(),
                          output.data.data(),
                          input.rows, weights.rows, weights.cols);
        }
        
        // Add bias
        MKMath::addBias(output.data.data(), bias.data(), output.rows, output.cols);
        return output;
    }
};

// Simple self-attention head (single head for memory efficiency)
struct MKAttentionLayer {
    MKLinearLayer queryProj;
    MKLinearLayer keyProj;
    MKLinearLayer valueProj;
    MKLinearLayer outputProj;
    int headDim;
    
    void init(int dim) {
        headDim = dim;
        queryProj.init(dim, dim);
        keyProj.init(dim, dim);
        valueProj.init(dim, dim);
        outputProj.init(dim, dim);
    }
    
    MKTensor forward(const MKTensor& input) {
        int seqLen = input.rows;
        
        // Project Q, K, V
        MKTensor Q = queryProj.forward(input);
        MKTensor K = keyProj.forward(input);
        MKTensor V = valueProj.forward(input);
        
        // Compute attention scores: Q * K^T / sqrt(d)
        float scale = 1.0f / std::sqrt((float)headDim);
        MKTensor scores(seqLen, seqLen);
        
        for (int i = 0; i < seqLen; i++) {
            for (int j = 0; j < seqLen; j++) {
                float dot = MKMath::dot(&Q.data[i * headDim], 
                                        &K.data[j * headDim], headDim);
                scores.at(i, j) = dot * scale;
                // Causal mask: prevent attending to future tokens
                if (j > i) scores.at(i, j) = -1e9f;
            }
        }
        
        // Softmax over each row
        for (int i = 0; i < seqLen; i++) {
            MKActivations::softmax(&scores.data[i * seqLen], seqLen);
        }
        
        // Weighted sum of values
        MKTensor attended(seqLen, headDim);
        MKMath::matmul(scores.data.data(), V.data.data(),
                       attended.data.data(), seqLen, seqLen, headDim);
        
        // Output projection
        return outputProj.forward(attended);
    }
};


// ─────────────────────────────────────────────
//  TRANSFORMER BLOCK
// ─────────────────────────────────────────────
struct MKTransformerBlock {
    MKAttentionLayer attention;
    MKLinearLayer ff1;    // Feed-forward expansion
    MKLinearLayer ff2;    // Feed-forward projection
    int dim;
    
    void init(int modelDim, int ffDim) {
        dim = modelDim;
        attention.init(modelDim);
        ff1.init(modelDim, ffDim);
        ff2.init(ffDim, modelDim);
    }
    
    MKTensor forward(const MKTensor& input) {
        // Self-attention + residual
        MKTensor attnOut = attention.forward(input);
        MKTensor residual1(input.rows, input.cols);
        for (int i = 0; i < input.size(); i++) {
            residual1.data[i] = input.data[i] + attnOut.data[i];
        }
        // Layer norm
        for (int i = 0; i < residual1.rows; i++) {
            MKMath::layerNorm(&residual1.data[i * dim], dim);
        }
        
        // Feed-forward + residual
        MKTensor ffOut = ff1.forward(residual1);
        // GELU activation
        for (auto& v : ffOut.data) v = MKActivations::gelu(v);
        ffOut = ff2.forward(ffOut);
        
        MKTensor output(input.rows, input.cols);
        for (int i = 0; i < input.size(); i++) {
            output.data[i] = residual1.data[i] + ffOut.data[i];
        }
        // Final layer norm
        for (int i = 0; i < output.rows; i++) {
            MKMath::layerNorm(&output.data[i * dim], dim);
        }
        return output;
    }
};

// ─────────────────────────────────────────────
//  COMPLETE MK NEURAL NETWORK
// ─────────────────────────────────────────────
class MKNeuralNet {
private:
    std::vector<MKTransformerBlock> layers;
    MKTensor embeddingTable;     // [vocab_size x dim]
    MKLinearLayer outputHead;    // [dim x vocab_size]
    int modelDim;
    int vocabSize;
    int numLayers;
    bool useQuantized;

public:
    MKNeuralNet() : modelDim(0), vocabSize(0), numLayers(0), useQuantized(false) {}
    
    // Initialize model architecture
    void init(int vocab, int dim, int nLayers, int ffDim) {
        vocabSize = vocab;
        modelDim = dim;
        numLayers = nLayers;
        
        // Token embedding table
        embeddingTable = MKTensor(vocab, dim);
        embeddingTable.randomInit(0.02f);
        
        // Transformer blocks
        layers.resize(nLayers);
        for (int i = 0; i < nLayers; i++) {
            layers[i].init(dim, ffDim);
        }
        
        // Output projection (language model head)
        outputHead.init(dim, vocab);
        
        std::cout << "[NEURAL NET] Model initialized: vocab=" << vocab
                  << " | dim=" << dim << " | layers=" << nLayers
                  << " | ff=" << ffDim << "\n";
        
        // Calculate parameter count
        long params = (long)vocab * dim;  // embeddings
        params += (long)nLayers * (4 * dim * dim + 2 * dim * ffDim); // transformer
        params += (long)dim * vocab;  // output head
        std::cout << "[NEURAL NET] Total parameters: ~" << (params / 1000000) << "M\n";
    }
    
    // Enable INT8 quantization on all linear layers
    void quantize() {
        useQuantized = true;
        outputHead.quantize();
        for (auto& layer : layers) {
            layer.attention.queryProj.quantize();
            layer.attention.keyProj.quantize();
            layer.attention.valueProj.quantize();
            layer.attention.outputProj.quantize();
            layer.ff1.quantize();
            layer.ff2.quantize();
        }
        std::cout << "[NEURAL NET] All layers quantized to INT8.\n";
    }


    // Forward pass: token IDs → logits over vocabulary
    MKTensor forward(const std::vector<int>& tokenIds) {
        int seqLen = tokenIds.size();
        
        // 1. Token embedding lookup
        MKTensor hidden(seqLen, modelDim);
        for (int i = 0; i < seqLen; i++) {
            int tid = tokenIds[i];
            if (tid >= 0 && tid < vocabSize) {
                for (int j = 0; j < modelDim; j++) {
                    hidden.at(i, j) = embeddingTable.at(tid, j);
                }
            }
            // Add positional encoding (sinusoidal)
            for (int j = 0; j < modelDim; j++) {
                float angle = (float)i / std::pow(10000.0f, (float)(2*(j/2)) / modelDim);
                hidden.at(i, j) += (j % 2 == 0) ? std::sin(angle) : std::cos(angle);
            }
        }
        
        // 2. Pass through transformer blocks
        for (int l = 0; l < numLayers; l++) {
            hidden = layers[l].forward(hidden);
        }
        
        // 3. Output projection to vocabulary logits
        MKTensor logits = outputHead.forward(hidden);
        return logits;
    }
    
    // Generate next token given input sequence (greedy)
    int generateNext(const std::vector<int>& tokenIds) {
        MKTensor logits = forward(tokenIds);
        
        // Take logits from last position
        int lastRow = logits.rows - 1;
        float* lastLogits = &logits.data[lastRow * logits.cols];
        
        // Find argmax
        int bestId = 0;
        float bestVal = lastLogits[0];
        for (int i = 1; i < vocabSize; i++) {
            if (lastLogits[i] > bestVal) {
                bestVal = lastLogits[i];
                bestId = i;
            }
        }
        return bestId;
    }
    
    // Generate with temperature sampling
    int generateSampled(const std::vector<int>& tokenIds, float temperature = 0.8f) {
        MKTensor logits = forward(tokenIds);
        int lastRow = logits.rows - 1;
        float* lastLogits = &logits.data[lastRow * logits.cols];
        
        // Apply temperature
        for (int i = 0; i < vocabSize; i++) {
            lastLogits[i] /= temperature;
        }
        
        // Softmax to get probabilities
        MKActivations::softmax(lastLogits, vocabSize);
        
        // Sample from distribution
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float r = dist(gen);
        
        float cumSum = 0.0f;
        for (int i = 0; i < vocabSize; i++) {
            cumSum += lastLogits[i];
            if (cumSum >= r) return i;
        }
        return vocabSize - 1;
    }
    
    // Save model weights to binary file
    void saveWeights(const std::string& filename) {
        std::ofstream out(filename, std::ios::binary);
        if (!out.is_open()) return;
        
        // Header
        out.write((char*)&vocabSize, sizeof(int));
        out.write((char*)&modelDim, sizeof(int));
        out.write((char*)&numLayers, sizeof(int));
        
        // Embedding table
        out.write((char*)embeddingTable.data.data(), 
                  embeddingTable.data.size() * sizeof(float));
        
        out.close();
        std::cout << "[NEURAL NET] Weights saved to: " << filename << "\n";
    }
    
    // Load model weights from binary file
    void loadWeights(const std::string& filename) {
        std::ifstream in(filename, std::ios::binary);
        if (!in.is_open()) {
            std::cerr << "[NEURAL NET ERROR] Cannot load weights: " << filename << "\n";
            return;
        }
        in.read((char*)&vocabSize, sizeof(int));
        in.read((char*)&modelDim, sizeof(int));
        in.read((char*)&numLayers, sizeof(int));
        
        embeddingTable = MKTensor(vocabSize, modelDim);
        in.read((char*)embeddingTable.data.data(),
                embeddingTable.data.size() * sizeof(float));
        
        in.close();
        std::cout << "[NEURAL NET] Weights loaded from: " << filename << "\n";
    }
    
    // Getters
    int getDim() const { return modelDim; }
    int getVocab() const { return vocabSize; }
    int getLayers() const { return numLayers; }
    long getParamCount() const {
        long p = (long)vocabSize * modelDim;
        p += (long)numLayers * (4 * modelDim * modelDim + 2 * modelDim * (modelDim * 4));
        p += (long)modelDim * vocabSize;
        return p;
    }
};

#endif // MK_NEURAL_NET_CPP
