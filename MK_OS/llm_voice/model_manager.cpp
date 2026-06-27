#ifndef MK_MODEL_MANAGER_CPP
#define MK_MODEL_MANAGER_CPP

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <dirent.h>

// ============================================================================
// MKModelManager - Handles model files on disk for the local LLM
// Manages downloading, loading GGUF models, and memory budget enforcement
// ============================================================================

enum class QuantizationLevel {
    Q2_K,
    Q4_0,
    Q4_K_M,
    Q5_K_M,
    Q8_0,
    F16,
    UNKNOWN
};

struct ModelInfo {
    std::string name;
    std::string path;
    size_t size_bytes;
    size_t size_mb;
    QuantizationLevel quantization;
    std::string format;
    int parameter_count_millions;
    int context_length;
    bool is_loaded;
};

struct DownloadProgress {
    size_t bytes_downloaded;
    size_t total_bytes;
    float percentage;
    bool complete;
    bool error;
    std::string error_message;
};

class MKModelManager {
private:
    std::string models_directory_;
    size_t memory_budget_mb_;
    size_t memory_used_mb_;
    ModelInfo loaded_model_;
    std::vector<ModelInfo> available_models_;
    bool model_loaded_;

    size_t getFileSize(const std::string& path) const {
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            return static_cast<size_t>(st.st_size);
        }
        return 0;
    }

    bool fileExists(const std::string& path) const {
        struct stat st;
        return stat(path.c_str(), &st) == 0;
    }

    bool directoryExists(const std::string& path) const {
        struct stat st;
        return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
    }

    std::string getFileName(const std::string& path) const {
        size_t pos = path.find_last_of("/\\");
        if (pos != std::string::npos) return path.substr(pos + 1);
        return path;
    }

    std::string getExtension(const std::string& path) const {
        size_t pos = path.find_last_of('.');
        if (pos != std::string::npos) return path.substr(pos);
        return "";
    }

    QuantizationLevel detectQuantization(const std::string& filename) const {
        std::string lower = filename;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("q2_k") != std::string::npos) return QuantizationLevel::Q2_K;
        if (lower.find("q4_0") != std::string::npos) return QuantizationLevel::Q4_0;
        if (lower.find("q4_k_m") != std::string::npos) return QuantizationLevel::Q4_K_M;
        if (lower.find("q5_k_m") != std::string::npos) return QuantizationLevel::Q5_K_M;
        if (lower.find("q8_0") != std::string::npos) return QuantizationLevel::Q8_0;
        if (lower.find("f16") != std::string::npos) return QuantizationLevel::F16;
        return QuantizationLevel::UNKNOWN;
    }

    std::string quantizationToString(QuantizationLevel q) const {
        switch (q) {
            case QuantizationLevel::Q2_K: return "Q2_K";
            case QuantizationLevel::Q4_0: return "Q4_0";
            case QuantizationLevel::Q4_K_M: return "Q4_K_M";
            case QuantizationLevel::Q5_K_M: return "Q5_K_M";
            case QuantizationLevel::Q8_0: return "Q8_0";
            case QuantizationLevel::F16: return "F16";
            default: return "UNKNOWN";
        }
    }

    int estimateParameters(size_t size_mb, QuantizationLevel q) const {
        // Rough estimation based on size and quantization
        double bits_per_param = 4.0;
        switch (q) {
            case QuantizationLevel::Q2_K: bits_per_param = 2.5; break;
            case QuantizationLevel::Q4_0: bits_per_param = 4.0; break;
            case QuantizationLevel::Q4_K_M: bits_per_param = 4.5; break;
            case QuantizationLevel::Q5_K_M: bits_per_param = 5.5; break;
            case QuantizationLevel::Q8_0: bits_per_param = 8.0; break;
            case QuantizationLevel::F16: bits_per_param = 16.0; break;
            default: bits_per_param = 4.0; break;
        }
        double bytes_per_param = bits_per_param / 8.0;
        return static_cast<int>((size_mb * 1024.0 * 1024.0) / (bytes_per_param * 1000000.0));
    }

    ModelInfo buildModelInfo(const std::string& path) const {
        ModelInfo info;
        info.path = path;
        info.name = getFileName(path);
        info.size_bytes = getFileSize(path);
        info.size_mb = info.size_bytes / (1024 * 1024);
        info.format = getExtension(path);
        info.quantization = detectQuantization(info.name);
        info.parameter_count_millions = estimateParameters(info.size_mb, info.quantization);
        info.context_length = 2048;
        info.is_loaded = false;
        return info;
    }

public:
    MKModelManager() 
        : models_directory_("./models"), memory_budget_mb_(2048), 
          memory_used_mb_(0), model_loaded_(false) {
        loaded_model_.is_loaded = false;
    }

    explicit MKModelManager(const std::string& models_dir)
        : models_directory_(models_dir), memory_budget_mb_(2048),
          memory_used_mb_(0), model_loaded_(false) {
        loaded_model_.is_loaded = false;
    }

    DownloadProgress downloadModel(const std::string& url, const std::string& dest_path) {
        DownloadProgress progress;
        progress.complete = false;
        progress.error = false;
        progress.bytes_downloaded = 0;
        progress.total_bytes = 0;
        progress.percentage = 0.0f;

        // Build curl command
        std::string cmd = "curl -L -o \"" + dest_path + "\" \"" + url + "\" 2>&1";
        
        std::cout << "[MKModelManager] Downloading model from: " << url << std::endl;
        std::cout << "[MKModelManager] Saving to: " << dest_path << std::endl;

        int ret = system(cmd.c_str());
        if (ret != 0) {
            progress.error = true;
            progress.error_message = "Download failed with exit code: " + std::to_string(ret);
            return progress;
        }

        if (!fileExists(dest_path)) {
            progress.error = true;
            progress.error_message = "File not found after download";
            return progress;
        }

        size_t file_size = getFileSize(dest_path);
        progress.bytes_downloaded = file_size;
        progress.total_bytes = file_size;
        progress.percentage = 100.0f;
        progress.complete = true;

        // Refresh available models
        scanModelsDirectory();
        
        std::cout << "[MKModelManager] Download complete: " << (file_size / 1024 / 1024) 
                  << " MB" << std::endl;
        return progress;
    }

    bool loadGGUF(const std::string& path) {
        if (!fileExists(path)) {
            std::cerr << "[MKModelManager] Model file not found: " << path << std::endl;
            return false;
        }

        ModelInfo info = buildModelInfo(path);

        // Check memory budget
        if (memory_used_mb_ + info.size_mb > memory_budget_mb_) {
            std::cerr << "[MKModelManager] Cannot load model: exceeds memory budget" << std::endl;
            std::cerr << "  Required: " << info.size_mb << " MB" << std::endl;
            std::cerr << "  Available: " << (memory_budget_mb_ - memory_used_mb_) << " MB" << std::endl;
            std::cerr << "  Budget: " << memory_budget_mb_ << " MB" << std::endl;
            return false;
        }

        // Unload current model if one is loaded
        if (model_loaded_) {
            unloadCurrentModel();
        }

        // Simulate loading the model into memory
        info.is_loaded = true;
        loaded_model_ = info;
        model_loaded_ = true;
        memory_used_mb_ += info.size_mb;

        std::cout << "[MKModelManager] Loaded: " << info.name << std::endl;
        std::cout << "  Size: " << info.size_mb << " MB" << std::endl;
        std::cout << "  Quantization: " << quantizationToString(info.quantization) << std::endl;
        std::cout << "  Est. Parameters: " << info.parameter_count_millions << "M" << std::endl;
        return true;
    }

    void unloadCurrentModel() {
        if (model_loaded_) {
            memory_used_mb_ -= loaded_model_.size_mb;
            std::cout << "[MKModelManager] Unloaded: " << loaded_model_.name << std::endl;
            loaded_model_ = ModelInfo();
            loaded_model_.is_loaded = false;
            model_loaded_ = false;
        }
    }

    ModelInfo getModelInfo() const {
        return loaded_model_;
    }

    void setMemoryBudget(size_t mb) {
        memory_budget_mb_ = mb;
        std::cout << "[MKModelManager] Memory budget set to: " << mb << " MB" << std::endl;
    }

    size_t getMemoryBudget() const { return memory_budget_mb_; }
    size_t getMemoryUsed() const { return memory_used_mb_; }
    size_t getMemoryAvailable() const { return memory_budget_mb_ - memory_used_mb_; }

    std::vector<ModelInfo> listAvailableModels() {
        scanModelsDirectory();
        return available_models_;
    }

    void scanModelsDirectory() {
        available_models_.clear();
        if (!directoryExists(models_directory_)) return;

        DIR* dir = opendir(models_directory_.c_str());
        if (!dir) return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            
            std::string full_path = models_directory_ + "/" + name;
            std::string ext = getExtension(name);
            
            if (ext == ".gguf" || ext == ".bin" || ext == ".ggml") {
                ModelInfo info = buildModelInfo(full_path);
                available_models_.push_back(info);
            }
        }
        closedir(dir);
    }

    void setModelsDirectory(const std::string& path) {
        models_directory_ = path;
    }

    std::string getModelsDirectory() const { return models_directory_; }
    bool isModelLoaded() const { return model_loaded_; }

    bool canLoadModel(const std::string& path) const {
        size_t size = getFileSize(path) / (1024 * 1024);
        return (memory_used_mb_ + size) <= memory_budget_mb_;
    }

    bool deleteModel(const std::string& path) {
        if (!fileExists(path)) return false;
        if (model_loaded_ && loaded_model_.path == path) {
            unloadCurrentModel();
        }
        int ret = std::remove(path.c_str());
        if (ret == 0) {
            scanModelsDirectory();
            return true;
        }
        return false;
    }
};

#endif // MK_MODEL_MANAGER_CPP
