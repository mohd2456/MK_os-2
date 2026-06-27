#ifndef MK_MODEL_CONFIG_CPP
#define MK_MODEL_CONFIG_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <algorithm>

// ===================================================================================
// MK NEURAL GRAPH FUSION — MODEL CONFIGURATION
// ===================================================================================
// Configuration and weight management for the LLM voice module.
// Defines model architectures, quantization formats, memory requirements,
// and layer configurations for supported models.
//
// Supported architectures:
//   - Pythia 1.4B (EleutherAI) - primary voice model
//   - Pythia 410M (EleutherAI) - lightweight fallback
//
// Quantization formats:
//   - Q4_0: 4-bit uniform quantization (smallest, fastest)
//   - Q4_1: 4-bit with per-block scale+min (better quality)
//   - Q8_0: 8-bit quantization (highest quality, 2x size)
//   - F16:  16-bit float (reference quality, no quantization)
//
// This file defines the constants and structures. Actual weights are loaded
// from GGUF files at runtime by llm_voice.cpp.
// ===================================================================================

// Quantization format identifiers
enum class MKQuantFormat : int {
    Q4_0 = 0,      // 4-bit, block size 32, no zero-point
    Q4_1 = 1,      // 4-bit, block size 32, with min value
    Q8_0 = 2,      // 8-bit, block size 32
    F16  = 3,      // 16-bit float (no quantization)
    F32  = 4       // 32-bit float (full precision, debug only)
};

// Activation function types
enum class MKActivationType : int {
    GELU        = 0,
    RELU        = 1,
    SILU        = 2,    // SiLU/Swish
    GELU_NEW    = 3     // GELU approximation used by GPT-NeoX
};

// Attention type
enum class MKAttentionType : int {
    MULTI_HEAD      = 0,    // Standard multi-head attention
    GROUPED_QUERY   = 1,    // Grouped-query attention (GQA)
    MULTI_QUERY     = 2     // Multi-query attention (MQA)
};

// Positional encoding type
enum class MKPositionEncoding : int {
    ROTARY      = 0,    // RoPE (Rotary Position Embedding) - used by Pythia
    ABSOLUTE    = 1,    // Learned absolute positions
    ALIBI       = 2,    // ALiBi (Attention with Linear Biases)
    NONE        = 3     // No positional encoding
};

// Configuration for a single transformer layer
struct MKLayerConfig {
    int hidden_size;            // Dimension of hidden states
    int intermediate_size;      // FFN intermediate dimension (usually 4x hidden)
    int num_attention_heads;    // Number of attention heads
    int num_kv_heads;           // Number of KV heads (for GQA; equals num_heads for MHA)
    int head_dim;               // Dimension per head (hidden_size / num_heads)
    MKActivationType activation;
    bool use_bias;              // Whether linear layers have bias terms
    bool parallel_attn;         // Parallel attention + FFN (GPT-NeoX style)
    float layer_norm_eps;       // Layer norm epsilon
};

// Full model architecture definition
struct MKModelArchitecture {
    std::string name;
    std::string family;                 // "pythia", "llama", "gpt-neox"
    std::string version;
    
    // Core dimensions
    int vocab_size;
    int max_seq_length;
    int hidden_size;
    int num_layers;
    int num_attention_heads;
    int num_kv_heads;
    int intermediate_size;
    int head_dim;
    
    // Architecture details
    MKAttentionType attention_type;
    MKPositionEncoding position_encoding;
    MKActivationType activation;
    float layer_norm_eps;
    bool use_parallel_residual;         // GPT-NeoX style parallel attn+ffn
    bool use_bias;
    int rotary_dim;                     // Dimension for RoPE (if used)
    float rotary_base;                  // Base for RoPE frequency
    
    // Memory estimates (in MB)
    size_t params_count;                // Total parameter count
    size_t memory_f32_mb;               // Memory at full precision
    size_t memory_f16_mb;               // Memory at FP16
    size_t memory_q8_mb;                // Memory at Q8_0
    size_t memory_q4_mb;                // Memory at Q4_0
    
    // Layer configuration
    MKLayerConfig layer_config;
};

// Model file metadata (from GGUF header)
struct MKModelFileInfo {
    std::string file_path;
    std::string model_name;
    MKQuantFormat quant_format;
    size_t file_size_bytes;
    std::string sha256_checksum;
    std::string download_url;
    bool verified;                      // SHA256 has been checked
};

// Weight tensor metadata
struct MKWeightTensor {
    std::string name;                   // e.g., "layers.0.attention.wq.weight"
    std::vector<int> shape;             // Dimensions
    MKQuantFormat format;
    size_t offset_in_file;              // Byte offset in GGUF file
    size_t size_bytes;                  // Size of this tensor's data
    int layer_index;                    // Which layer (-1 for embeddings)
};

class MKModelConfig {
private:
    std::unordered_map<std::string, MKModelArchitecture> architectures;
    std::vector<MKModelFileInfo> known_models;
    std::string active_model;
    MKQuantFormat preferred_quant;

    // Initialize Pythia 1.4B architecture
    MKModelArchitecture createPythia14B() {
        MKModelArchitecture arch;
        arch.name = "pythia-1.4b";
        arch.family = "pythia";
        arch.version = "v0";
        
        arch.vocab_size = 50304;
        arch.max_seq_length = 2048;
        arch.hidden_size = 2048;
        arch.num_layers = 24;
        arch.num_attention_heads = 16;
        arch.num_kv_heads = 16;
        arch.intermediate_size = 8192;
        arch.head_dim = 128;                // 2048 / 16
        
        arch.attention_type = MKAttentionType::MULTI_HEAD;
        arch.position_encoding = MKPositionEncoding::ROTARY;
        arch.activation = MKActivationType::GELU_NEW;
        arch.layer_norm_eps = 1e-5f;
        arch.use_parallel_residual = true;
        arch.use_bias = true;
        arch.rotary_dim = 32;               // 25% of head_dim
        arch.rotary_base = 10000.0f;
        
        arch.params_count = 1414647808;     // ~1.4B
        arch.memory_f32_mb = 5376;
        arch.memory_f16_mb = 2688;
        arch.memory_q8_mb = 1344;
        arch.memory_q4_mb = 672;
        
        arch.layer_config.hidden_size = 2048;
        arch.layer_config.intermediate_size = 8192;
        arch.layer_config.num_attention_heads = 16;
        arch.layer_config.num_kv_heads = 16;
        arch.layer_config.head_dim = 128;
        arch.layer_config.activation = MKActivationType::GELU_NEW;
        arch.layer_config.use_bias = true;
        arch.layer_config.parallel_attn = true;
        arch.layer_config.layer_norm_eps = 1e-5f;
        
        return arch;
    }

    // Initialize Pythia 410M architecture
    MKModelArchitecture createPythia410M() {
        MKModelArchitecture arch;
        arch.name = "pythia-410m";
        arch.family = "pythia";
        arch.version = "v0";
        
        arch.vocab_size = 50304;
        arch.max_seq_length = 2048;
        arch.hidden_size = 1024;
        arch.num_layers = 24;
        arch.num_attention_heads = 16;
        arch.num_kv_heads = 16;
        arch.intermediate_size = 4096;
        arch.head_dim = 64;                 // 1024 / 16
        
        arch.attention_type = MKAttentionType::MULTI_HEAD;
        arch.position_encoding = MKPositionEncoding::ROTARY;
        arch.activation = MKActivationType::GELU_NEW;
        arch.layer_norm_eps = 1e-5f;
        arch.use_parallel_residual = true;
        arch.use_bias = true;
        arch.rotary_dim = 16;               // 25% of head_dim
        arch.rotary_base = 10000.0f;
        
        arch.params_count = 405334016;      // ~410M
        arch.memory_f32_mb = 1545;
        arch.memory_f16_mb = 773;
        arch.memory_q8_mb = 386;
        arch.memory_q4_mb = 193;
        
        arch.layer_config.hidden_size = 1024;
        arch.layer_config.intermediate_size = 4096;
        arch.layer_config.num_attention_heads = 16;
        arch.layer_config.num_kv_heads = 16;
        arch.layer_config.head_dim = 64;
        arch.layer_config.activation = MKActivationType::GELU_NEW;
        arch.layer_config.use_bias = true;
        arch.layer_config.parallel_attn = true;
        arch.layer_config.layer_norm_eps = 1e-5f;
        
        return arch;
    }

    // Initialize known model files
    void initKnownModels() {
        // Pythia 1.4B Q4_0
        MKModelFileInfo pythia14b_q4;
        pythia14b_q4.file_path = "models/pythia-1.4b-q4_0.gguf";
        pythia14b_q4.model_name = "pythia-1.4b";
        pythia14b_q4.quant_format = MKQuantFormat::Q4_0;
        pythia14b_q4.file_size_bytes = 704643072;       // ~672 MB
        pythia14b_q4.sha256_checksum = "a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2";
        pythia14b_q4.download_url = "https://huggingface.co/EleutherAI/pythia-1.4b-gguf/resolve/main/pythia-1.4b-q4_0.gguf";
        pythia14b_q4.verified = false;
        known_models.push_back(pythia14b_q4);

        // Pythia 1.4B Q8_0
        MKModelFileInfo pythia14b_q8;
        pythia14b_q8.file_path = "models/pythia-1.4b-q8_0.gguf";
        pythia14b_q8.model_name = "pythia-1.4b";
        pythia14b_q8.quant_format = MKQuantFormat::Q8_0;
        pythia14b_q8.file_size_bytes = 1409286144;      // ~1344 MB
        pythia14b_q8.sha256_checksum = "b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3";
        pythia14b_q8.download_url = "https://huggingface.co/EleutherAI/pythia-1.4b-gguf/resolve/main/pythia-1.4b-q8_0.gguf";
        pythia14b_q8.verified = false;
        known_models.push_back(pythia14b_q8);

        // Pythia 410M Q4_0
        MKModelFileInfo pythia410m_q4;
        pythia410m_q4.file_path = "models/pythia-410m-q4_0.gguf";
        pythia410m_q4.model_name = "pythia-410m";
        pythia410m_q4.quant_format = MKQuantFormat::Q4_0;
        pythia410m_q4.file_size_bytes = 202375168;      // ~193 MB
        pythia410m_q4.sha256_checksum = "c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4";
        pythia410m_q4.download_url = "https://huggingface.co/EleutherAI/pythia-410m-gguf/resolve/main/pythia-410m-q4_0.gguf";
        pythia410m_q4.verified = false;
        known_models.push_back(pythia410m_q4);

        // Pythia 410M Q4_1
        MKModelFileInfo pythia410m_q41;
        pythia410m_q41.file_path = "models/pythia-410m-q4_1.gguf";
        pythia410m_q41.model_name = "pythia-410m";
        pythia410m_q41.quant_format = MKQuantFormat::Q4_1;
        pythia410m_q41.file_size_bytes = 225443840;     // ~215 MB
        pythia410m_q41.sha256_checksum = "d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5";
        pythia410m_q41.download_url = "https://huggingface.co/EleutherAI/pythia-410m-gguf/resolve/main/pythia-410m-q4_1.gguf";
        pythia410m_q41.verified = false;
        known_models.push_back(pythia410m_q41);
    }

public:
    MKModelConfig() : active_model("pythia-1.4b"), preferred_quant(MKQuantFormat::Q4_0) {
        // Register architectures
        architectures["pythia-1.4b"] = createPythia14B();
        architectures["pythia-410m"] = createPythia410M();
        initKnownModels();
        
        std::cout << "[MODEL CONFIG] Model configuration manager initialized.\n";
        std::cout << "  Architectures: " << architectures.size() << "\n";
        std::cout << "  Known model files: " << known_models.size() << "\n";
        std::cout << "  Active model: " << active_model << "\n";
    }

    // ─────────────────────────────────────────
    //  ARCHITECTURE QUERIES
    // ─────────────────────────────────────────

    // Get architecture details for a model
    const MKModelArchitecture* getArchitecture(const std::string& model_name) const {
        auto it = architectures.find(model_name);
        if (it != architectures.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // Get the active model architecture
    const MKModelArchitecture* getActiveArchitecture() const {
        return getArchitecture(active_model);
    }

    // List available architectures
    std::vector<std::string> listArchitectures() const {
        std::vector<std::string> names;
        for (const auto& pair : architectures) {
            names.push_back(pair.first);
        }
        return names;
    }

    // Set active model
    bool setActiveModel(const std::string& model_name) {
        if (architectures.find(model_name) != architectures.end()) {
            active_model = model_name;
            return true;
        }
        return false;
    }

    // ─────────────────────────────────────────
    //  QUANTIZATION HELPERS
    // ─────────────────────────────────────────

    // Get bytes per element for a quantization format
    static float getBytesPerElement(MKQuantFormat format) {
        switch (format) {
            case MKQuantFormat::Q4_0: return 0.5f;      // 4 bits
            case MKQuantFormat::Q4_1: return 0.5625f;   // 4 bits + overhead
            case MKQuantFormat::Q8_0: return 1.0f;      // 8 bits
            case MKQuantFormat::F16:  return 2.0f;      // 16 bits
            case MKQuantFormat::F32:  return 4.0f;      // 32 bits
            default: return 4.0f;
        }
    }

    // Get human-readable name for quantization format
    static std::string getQuantName(MKQuantFormat format) {
        switch (format) {
            case MKQuantFormat::Q4_0: return "Q4_0";
            case MKQuantFormat::Q4_1: return "Q4_1";
            case MKQuantFormat::Q8_0: return "Q8_0";
            case MKQuantFormat::F16:  return "F16";
            case MKQuantFormat::F32:  return "F32";
            default: return "Unknown";
        }
    }

    // Estimate memory for a model at a given quantization
    size_t estimateMemoryMB(const std::string& model_name, MKQuantFormat format) const {
        auto arch = getArchitecture(model_name);
        if (!arch) return 0;
        float bytes_per_param = getBytesPerElement(format);
        return static_cast<size_t>((arch->params_count * bytes_per_param) / (1024 * 1024));
    }

    // Set preferred quantization format
    void setPreferredQuant(MKQuantFormat format) { preferred_quant = format; }
    MKQuantFormat getPreferredQuant() const { return preferred_quant; }

    // ─────────────────────────────────────────
    //  MODEL FILE MANAGEMENT
    // ─────────────────────────────────────────

    // Get model file info for the active model with preferred quantization
    const MKModelFileInfo* getModelFile() const {
        for (const auto& info : known_models) {
            if (info.model_name == active_model && info.quant_format == preferred_quant) {
                return &info;
            }
        }
        // Fallback: any file for the active model
        for (const auto& info : known_models) {
            if (info.model_name == active_model) {
                return &info;
            }
        }
        return nullptr;
    }

    // Get all known model files
    const std::vector<MKModelFileInfo>& getKnownModels() const { return known_models; }

    // Generate expected weight tensor names for a model
    std::vector<MKWeightTensor> getExpectedTensors(const std::string& model_name) const {
        std::vector<MKWeightTensor> tensors;
        auto arch = getArchitecture(model_name);
        if (!arch) return tensors;

        // Embedding layer
        MKWeightTensor embed;
        embed.name = "token_embedding.weight";
        embed.shape = {arch->vocab_size, arch->hidden_size};
        embed.format = preferred_quant;
        embed.offset_in_file = 0;
        embed.size_bytes = static_cast<size_t>(arch->vocab_size * arch->hidden_size * getBytesPerElement(preferred_quant));
        embed.layer_index = -1;
        tensors.push_back(embed);

        // Per-layer tensors
        for (int l = 0; l < arch->num_layers; l++) {
            std::string prefix = "layers." + std::to_string(l) + ".";

            // Attention weights
            MKWeightTensor wq, wk, wv, wo;
            wq.name = prefix + "attention.wq.weight";
            wq.shape = {arch->hidden_size, arch->hidden_size};
            wq.format = preferred_quant;
            wq.layer_index = l;
            tensors.push_back(wq);

            wk.name = prefix + "attention.wk.weight";
            wk.shape = {arch->hidden_size, arch->hidden_size};
            wk.format = preferred_quant;
            wk.layer_index = l;
            tensors.push_back(wk);

            wv.name = prefix + "attention.wv.weight";
            wv.shape = {arch->hidden_size, arch->hidden_size};
            wv.format = preferred_quant;
            wv.layer_index = l;
            tensors.push_back(wv);

            wo.name = prefix + "attention.wo.weight";
            wo.shape = {arch->hidden_size, arch->hidden_size};
            wo.format = preferred_quant;
            wo.layer_index = l;
            tensors.push_back(wo);

            // FFN weights
            MKWeightTensor w1, w2;
            w1.name = prefix + "ffn.w1.weight";
            w1.shape = {arch->hidden_size, arch->intermediate_size};
            w1.format = preferred_quant;
            w1.layer_index = l;
            tensors.push_back(w1);

            w2.name = prefix + "ffn.w2.weight";
            w2.shape = {arch->intermediate_size, arch->hidden_size};
            w2.format = preferred_quant;
            w2.layer_index = l;
            tensors.push_back(w2);

            // Layer norms
            MKWeightTensor ln1, ln2;
            ln1.name = prefix + "attention_norm.weight";
            ln1.shape = {arch->hidden_size};
            ln1.format = MKQuantFormat::F32;  // Norms always in FP32
            ln1.layer_index = l;
            tensors.push_back(ln1);

            ln2.name = prefix + "ffn_norm.weight";
            ln2.shape = {arch->hidden_size};
            ln2.format = MKQuantFormat::F32;
            ln2.layer_index = l;
            tensors.push_back(ln2);
        }

        // Final layer norm + output head
        MKWeightTensor final_norm, output_head;
        final_norm.name = "final_norm.weight";
        final_norm.shape = {arch->hidden_size};
        final_norm.format = MKQuantFormat::F32;
        final_norm.layer_index = -1;
        tensors.push_back(final_norm);

        output_head.name = "output.weight";
        output_head.shape = {arch->hidden_size, arch->vocab_size};
        output_head.format = preferred_quant;
        output_head.layer_index = -1;
        tensors.push_back(output_head);

        return tensors;
    }

    // ─────────────────────────────────────────
    //  STATS
    // ─────────────────────────────────────────

    std::string getActiveModel() const { return active_model; }

    void printStats() const {
        std::cout << "\n--- [MODEL CONFIG] ---\n";
        std::cout << "  Active model: " << active_model << "\n";
        std::cout << "  Preferred quant: " << getQuantName(preferred_quant) << "\n";
        std::cout << "  Known architectures: " << architectures.size() << "\n";
        std::cout << "  Known model files: " << known_models.size() << "\n";
        
        auto arch = getActiveArchitecture();
        if (arch) {
            std::cout << "  --- " << arch->name << " ---\n";
            std::cout << "    Params: " << arch->params_count << "\n";
            std::cout << "    Layers: " << arch->num_layers << "\n";
            std::cout << "    Hidden: " << arch->hidden_size << "\n";
            std::cout << "    Heads: " << arch->num_attention_heads << "\n";
            std::cout << "    Vocab: " << arch->vocab_size << "\n";
            std::cout << "    Context: " << arch->max_seq_length << "\n";
            std::cout << "    Memory (Q4_0): " << arch->memory_q4_mb << " MB\n";
            std::cout << "    Memory (Q8_0): " << arch->memory_q8_mb << " MB\n";
            std::cout << "    Memory (F16):  " << arch->memory_f16_mb << " MB\n";
        }
        std::cout << "----------------------\n";
    }
};

#endif // MK_MODEL_CONFIG_CPP
