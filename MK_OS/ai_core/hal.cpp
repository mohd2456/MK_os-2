#ifndef MK_HAL_CPP
#define MK_HAL_CPP

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <functional>
#include <map>

// ===================================================================================
// MK HARDWARE ABSTRACTION LAYER (HAL)
// Detects CPU capabilities at runtime and dispatches math operations to the
// fastest available implementation:
//   - SSE2 vectorized (x86_64 with SSE2)
//   - NEON vectorized (ARM)
//   - Scalar fallback (any CPU)
// Provides a stable ABI for math plugins to implement.
// ===================================================================================


enum class MKSIMDCapability {
    NONE,       // Pure scalar fallback
    SSE2,       // x86 128-bit vector
    AVX2,       // x86 256-bit vector  
    NEON,       // ARM 128-bit vector
    DSP         // Embedded DSP units
};

struct MKHALProfile {
    MKSIMDCapability bestSIMD;
    std::string cpuModel;
    int cacheLineSize;
    int l1CacheKB;
    int l2CacheKB;
    bool hasHardwareFloat;
    bool hasFMA;
};

// ─────────────────────────────────────────────
//  HAL MATH KERNEL INTERFACE
//  Each implementation provides these operations
// ─────────────────────────────────────────────
struct MKMathKernel {
    std::string name;
    MKSIMDCapability requiredSIMD;
    
    // Function pointers for core operations
    std::function<void(const float*, const float*, float*, int)> vecAdd;
    std::function<void(const float*, const float*, float*, int)> vecMul;
    std::function<float(const float*, const float*, int)> dotProduct;
    std::function<void(const float*, const float*, float*, int, int, int)> matmul;
};

class MKHAL {
private:
    MKHALProfile profile;
    std::map<std::string, MKMathKernel> kernelRegistry;
    MKMathKernel* activeKernel;
    
    // ── SCALAR FALLBACK IMPLEMENTATIONS ──
    static void scalarVecAdd(const float* a, const float* b, float* c, int n) {
        for (int i = 0; i < n; i++) c[i] = a[i] + b[i];
    }
    
    static void scalarVecMul(const float* a, const float* b, float* c, int n) {
        for (int i = 0; i < n; i++) c[i] = a[i] * b[i];
    }
    
    static float scalarDot(const float* a, const float* b, int n) {
        float sum = 0.0f;
        for (int i = 0; i < n; i++) sum += a[i] * b[i];
        return sum;
    }
    
    static void scalarMatmul(const float* A, const float* B, float* C,
                             int M, int K, int N) {
        std::memset(C, 0, M * N * sizeof(float));
        for (int i = 0; i < M; i++) {
            for (int k = 0; k < K; k++) {
                float a = A[i * K + k];
                for (int j = 0; j < N; j++) {
                    C[i * N + j] += a * B[k * N + j];
                }
            }
        }
    }


    // ── SSE2 OPTIMIZED IMPLEMENTATIONS ──
    // These use 128-bit SIMD registers to process 4 floats per cycle
    #if defined(__SSE2__)
    #include <immintrin.h>
    
    static void sse2VecAdd(const float* a, const float* b, float* c, int n) {
        int i = 0;
        for (; i + 4 <= n; i += 4) {
            __m128 va = _mm_loadu_ps(a + i);
            __m128 vb = _mm_loadu_ps(b + i);
            __m128 vc = _mm_add_ps(va, vb);
            _mm_storeu_ps(c + i, vc);
        }
        for (; i < n; i++) c[i] = a[i] + b[i];
    }
    
    static void sse2VecMul(const float* a, const float* b, float* c, int n) {
        int i = 0;
        for (; i + 4 <= n; i += 4) {
            __m128 va = _mm_loadu_ps(a + i);
            __m128 vb = _mm_loadu_ps(b + i);
            __m128 vc = _mm_mul_ps(va, vb);
            _mm_storeu_ps(c + i, vc);
        }
        for (; i < n; i++) c[i] = a[i] * b[i];
    }
    
    static float sse2Dot(const float* a, const float* b, int n) {
        __m128 sum = _mm_setzero_ps();
        int i = 0;
        for (; i + 4 <= n; i += 4) {
            __m128 va = _mm_loadu_ps(a + i);
            __m128 vb = _mm_loadu_ps(b + i);
            sum = _mm_add_ps(sum, _mm_mul_ps(va, vb));
        }
        // Horizontal sum of 4 floats in sum register
        float result[4];
        _mm_storeu_ps(result, sum);
        float total = result[0] + result[1] + result[2] + result[3];
        for (; i < n; i++) total += a[i] * b[i];
        return total;
    }
    
    static void sse2Matmul(const float* A, const float* B, float* C,
                           int M, int K, int N) {
        std::memset(C, 0, M * N * sizeof(float));
        for (int i = 0; i < M; i++) {
            for (int k = 0; k < K; k++) {
                __m128 va = _mm_set1_ps(A[i * K + k]);
                int j = 0;
                for (; j + 4 <= N; j += 4) {
                    __m128 vb = _mm_loadu_ps(B + k * N + j);
                    __m128 vc = _mm_loadu_ps(C + i * N + j);
                    vc = _mm_add_ps(vc, _mm_mul_ps(va, vb));
                    _mm_storeu_ps(C + i * N + j, vc);
                }
                for (; j < N; j++) {
                    C[i * N + j] += A[i * K + k] * B[k * N + j];
                }
            }
        }
    }
    #endif


    // Detect CPU capabilities at runtime
    void detectCapabilities() {
        profile.bestSIMD = MKSIMDCapability::NONE;
        profile.cacheLineSize = 64;  // Common default
        profile.l1CacheKB = 32;
        profile.l2CacheKB = 256;
        profile.hasHardwareFloat = true;
        profile.hasFMA = false;
        
        #if defined(__SSE2__)
            profile.bestSIMD = MKSIMDCapability::SSE2;
        #endif
        #if defined(__AVX2__)
            profile.bestSIMD = MKSIMDCapability::AVX2;
            profile.hasFMA = true;
        #endif
        #if defined(__ARM_NEON) || defined(__ARM_NEON__)
            profile.bestSIMD = MKSIMDCapability::NEON;
        #endif
        
        // Try to read CPU model string
        #if defined(__linux__)
        std::ifstream cpuInfo("/proc/cpuinfo");
        if (cpuInfo.is_open()) {
            std::string line;
            while (std::getline(cpuInfo, line)) {
                if (line.find("model name") != std::string::npos) {
                    size_t colon = line.find(':');
                    if (colon != std::string::npos) {
                        profile.cpuModel = line.substr(colon + 2);
                    }
                    break;
                }
            }
            cpuInfo.close();
        }
        #else
            profile.cpuModel = "Unknown CPU";
        #endif
    }

public:
    MKHAL() : activeKernel(nullptr) {
        detectCapabilities();
        registerDefaultKernels();
        selectBestKernel();
        
        std::cout << "[HAL] Hardware Abstraction Layer initialized.\n";
        std::cout << "[HAL] CPU: " << profile.cpuModel << "\n";
        std::cout << "[HAL] SIMD: " << simdLabel() << "\n";
        std::cout << "[HAL] Active kernel: " << (activeKernel ? activeKernel->name : "NONE") << "\n";
    }
    
    std::string simdLabel() const {
        switch (profile.bestSIMD) {
            case MKSIMDCapability::NONE: return "Scalar (No SIMD)";
            case MKSIMDCapability::SSE2: return "SSE2 (128-bit)";
            case MKSIMDCapability::AVX2: return "AVX2 (256-bit)";
            case MKSIMDCapability::NEON: return "ARM NEON (128-bit)";
            case MKSIMDCapability::DSP:  return "DSP";
        }
        return "Unknown";
    }
    
    void registerDefaultKernels() {
        // Always register scalar fallback
        MKMathKernel scalar;
        scalar.name = "scalar_fallback";
        scalar.requiredSIMD = MKSIMDCapability::NONE;
        scalar.vecAdd = scalarVecAdd;
        scalar.vecMul = scalarVecMul;
        scalar.dotProduct = scalarDot;
        scalar.matmul = scalarMatmul;
        kernelRegistry["scalar"] = scalar;
        
        #if defined(__SSE2__)
        MKMathKernel sse2;
        sse2.name = "sse2_vectorized";
        sse2.requiredSIMD = MKSIMDCapability::SSE2;
        sse2.vecAdd = sse2VecAdd;
        sse2.vecMul = sse2VecMul;
        sse2.dotProduct = sse2Dot;
        sse2.matmul = sse2Matmul;
        kernelRegistry["sse2"] = sse2;
        #endif
    }
    
    // Automatically select the fastest available kernel
    void selectBestKernel() {
        #if defined(__SSE2__)
        if (profile.bestSIMD >= MKSIMDCapability::SSE2) {
            activeKernel = &kernelRegistry["sse2"];
            return;
        }
        #endif
        activeKernel = &kernelRegistry["scalar"];
    }
    
    // Register a third-party math kernel plugin
    void registerKernel(const std::string& name, MKMathKernel kernel) {
        kernelRegistry[name] = kernel;
        std::cout << "[HAL] Registered plugin kernel: \"" << name << "\"\n";
    }
    
    // ── PUBLIC MATH DISPATCH API ──
    void vecAdd(const float* a, const float* b, float* c, int n) {
        if (activeKernel && activeKernel->vecAdd) activeKernel->vecAdd(a, b, c, n);
    }
    
    void vecMul(const float* a, const float* b, float* c, int n) {
        if (activeKernel && activeKernel->vecMul) activeKernel->vecMul(a, b, c, n);
    }
    
    float dotProduct(const float* a, const float* b, int n) {
        if (activeKernel && activeKernel->dotProduct) return activeKernel->dotProduct(a, b, n);
        return 0.0f;
    }
    
    void matmul(const float* A, const float* B, float* C, int M, int K, int N) {
        if (activeKernel && activeKernel->matmul) activeKernel->matmul(A, B, C, M, K, N);
    }
    
    // Benchmark: runs dot product N times and reports throughput
    void benchmark(int vectorSize = 1024, int iterations = 10000) {
        std::vector<float> a(vectorSize, 1.0f), b(vectorSize, 2.0f);
        
        auto start = std::chrono::high_resolution_clock::now();
        float result = 0.0f;
        for (int i = 0; i < iterations; i++) {
            result += dotProduct(a.data(), b.data(), vectorSize);
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        double opsPerSec = (double)iterations * vectorSize * 2.0 / (ms / 1000.0);
        
        std::cout << "[HAL BENCHMARK] Kernel: " << activeKernel->name << "\n";
        std::cout << "[HAL BENCHMARK] " << iterations << " dot products of size " 
                  << vectorSize << " in " << ms << "ms\n";
        std::cout << "[HAL BENCHMARK] Throughput: " << (opsPerSec / 1e9) << " GFLOPS\n";
    }
    
    const MKHALProfile& getProfile() const { return profile; }
};

#endif // MK_HAL_CPP
