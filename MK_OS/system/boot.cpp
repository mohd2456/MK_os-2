#ifndef MK_BOOT_CPP
#define MK_BOOT_CPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <unistd.h>

// ===================================================================================
// MK BOOT SUBSYSTEM
// Handles hardware detection, partition verification, environment variable setup,
// and sequential initialization of all kernel-level primitives before handing off
// to the ai_core execution daemon.
// ===================================================================================

struct MKHardwareProfile {
    std::string osName;
    std::string kernelVersion;
    std::string architecture;
    std::string hostname;
    long totalRAM_MB;
    long freeStorage_MB;
    int cpuCores;
    bool hasSSE2;
    bool hasNEON;
    bool isACPowered;
};

class MKBoot {
private:
    MKHardwareProfile hwProfile;
    std::vector<std::string> bootLog;
    std::string datasetMountPath;
    bool bootSuccess;
    std::time_t bootTimestamp;

    void logEvent(const std::string& msg) {
        std::time_t now = std::time(nullptr);
        char timeStr[64];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", std::localtime(&now));
        std::string entry = "[BOOT " + std::string(timeStr) + "] " + msg;
        bootLog.push_back(entry);
        std::cout << entry << "\n";
    }

    // Detect host OS and kernel version via uname syscall
    void detectOS() {
        struct utsname sysInfo;
        if (uname(&sysInfo) == 0) {
            hwProfile.osName = sysInfo.sysname;
            hwProfile.kernelVersion = sysInfo.release;
            hwProfile.architecture = sysInfo.machine;
            hwProfile.hostname = sysInfo.nodename;
        } else {
            hwProfile.osName = "Unknown";
            hwProfile.kernelVersion = "Unknown";
            hwProfile.architecture = "Unknown";
            hwProfile.hostname = "mk-node";
        }
        logEvent("OS Detected: " + hwProfile.osName + " " + hwProfile.kernelVersion + " (" + hwProfile.architecture + ")");
    }

    // Detect available CPU cores for thread pool sizing
    void detectCPU() {
        hwProfile.cpuCores = sysconf(_SC_NPROCESSORS_ONLN);
        if (hwProfile.cpuCores <= 0) hwProfile.cpuCores = 1;
        logEvent("CPU Cores Available: " + std::to_string(hwProfile.cpuCores));

        // Detect SIMD capabilities via architecture string
        #if defined(__SSE2__)
            hwProfile.hasSSE2 = true;
            logEvent("SIMD: SSE2 instruction set DETECTED — HAL acceleration enabled.");
        #else
            hwProfile.hasSSE2 = false;
            logEvent("SIMD: SSE2 not available — falling back to integer math kernels.");
        #endif

        #if defined(__ARM_NEON) || defined(__ARM_NEON__)
            hwProfile.hasNEON = true;
            logEvent("SIMD: ARM NEON vector units DETECTED.");
        #else
            hwProfile.hasNEON = false;
        #endif
    }

    // Detect available RAM from /proc/meminfo (Linux) or sysctl (macOS)
    void detectMemory() {
        long pages = sysconf(_SC_PHYS_PAGES);
        long pageSize = sysconf(_SC_PAGE_SIZE);
        if (pages > 0 && pageSize > 0) {
            hwProfile.totalRAM_MB = (pages * pageSize) / (1024 * 1024);
        } else {
            hwProfile.totalRAM_MB = 0;
        }
        logEvent("Physical RAM: " + std::to_string(hwProfile.totalRAM_MB) + " MB");
    }

    // Check disk space for dataset partition
    void detectStorage() {
        struct statvfs stat;
        std::string checkPath = datasetMountPath.empty() ? "/" : datasetMountPath;
        if (statvfs(checkPath.c_str(), &stat) == 0) {
            hwProfile.freeStorage_MB = (stat.f_bavail * stat.f_frsize) / (1024 * 1024);
        } else {
            hwProfile.freeStorage_MB = 0;
        }
        logEvent("Free Storage at '" + checkPath + "': " + std::to_string(hwProfile.freeStorage_MB) + " MB");
    }

    // Check if running on AC power (Linux battery check)
    void detectPowerSource() {
        hwProfile.isACPowered = true; // Default to AC
        std::ifstream powerFile("/sys/class/power_supply/AC/online");
        if (powerFile.is_open()) {
            int val = 0;
            powerFile >> val;
            hwProfile.isACPowered = (val == 1);
            powerFile.close();
        }
        logEvent(hwProfile.isACPowered ? "Power: AC connected — full performance mode." 
                                       : "Power: BATTERY — activating power saver governor.");
    }

    // Verify dataset partition is mounted and accessible
    bool verifyDatasetPartition() {
        if (datasetMountPath.empty()) {
            datasetMountPath = "/mnt/datasets";
        }
        // Check if path exists and is readable
        if (access(datasetMountPath.c_str(), R_OK) == 0) {
            logEvent("Dataset partition verified at: " + datasetMountPath);
            return true;
        }
        logEvent("[WARN] Dataset partition NOT found at: " + datasetMountPath + " — using fallback root storage.");
        datasetMountPath = "./datasets";
        return false;
    }

    // Set environment variables for child processes
    void setupEnvironment() {
        // Cache budget: 300MB default, reduced to 150MB on battery
        int cacheBudgetMB = hwProfile.isACPowered ? 300 : 150;
        setenv("MK_MODE", "boot", 1);
        setenv("MK_DATASET_PATH", datasetMountPath.c_str(), 1);
        setenv("MK_CORE_CACHE_BUDGET", std::to_string(cacheBudgetMB * 1024 * 1024).c_str(), 1);
        setenv("MK_CPU_CORES", std::to_string(hwProfile.cpuCores).c_str(), 1);
        setenv("MK_HAS_SSE2", hwProfile.hasSSE2 ? "1" : "0", 1);
        setenv("MK_HAS_NEON", hwProfile.hasNEON ? "1" : "0", 1);
        setenv("MK_RAM_MB", std::to_string(hwProfile.totalRAM_MB).c_str(), 1);

        logEvent("Environment configured: Cache=" + std::to_string(cacheBudgetMB) + "MB | Cores=" + 
                 std::to_string(hwProfile.cpuCores) + " | SIMD=" + (hwProfile.hasSSE2 ? "SSE2" : (hwProfile.hasNEON ? "NEON" : "NONE")));
    }

public:
    MKBoot() : bootSuccess(false), datasetMountPath("/mnt/datasets") {
        bootTimestamp = std::time(nullptr);
    }

    // Master boot sequence — call this to initialize everything
    void start() {
        std::cout << "\n";
        logEvent("=== MK HYBRID OS BOOT SEQUENCE INITIATED ===");
        logEvent("Boot timestamp: " + std::string(std::ctime(&bootTimestamp)));

        // Phase 1: Hardware Detection
        logEvent("--- Phase 1: Hardware Detection ---");
        detectOS();
        detectCPU();
        detectMemory();
        detectStorage();
        detectPowerSource();

        // Phase 2: Partition Verification
        logEvent("--- Phase 2: Storage Partition Check ---");
        verifyDatasetPartition();

        // Phase 3: Environment Setup
        logEvent("--- Phase 3: Environment Configuration ---");
        setupEnvironment();

        // Phase 4: Readiness Assessment
        logEvent("--- Phase 4: Boot Validation ---");
        if (hwProfile.totalRAM_MB < 512) {
            logEvent("[WARN] Low RAM detected (" + std::to_string(hwProfile.totalRAM_MB) + "MB). Ultra-low precision mode recommended.");
        }
        if (hwProfile.freeStorage_MB < 1024) {
            logEvent("[WARN] Low disk space. Dataset streaming may be constrained.");
        }

        bootSuccess = true;
        logEvent("=== MK BOOT SEQUENCE COMPLETE — ALL SYSTEMS GO ===\n");
    }

    // Accessors
    const MKHardwareProfile& getHardwareProfile() const { return hwProfile; }
    bool isBootSuccessful() const { return bootSuccess; }
    bool isOnACPower() const { return hwProfile.isACPowered; }
    int getCPUCores() const { return hwProfile.cpuCores; }
    long getRAM_MB() const { return hwProfile.totalRAM_MB; }
    const std::string& getDatasetPath() const { return datasetMountPath; }

    // Dump full boot log to file for postmortem analysis
    void saveBootLog(const std::string& filename = "mk_boot_log.txt") {
        std::ofstream out(filename);
        if (out.is_open()) {
            for (const auto& line : bootLog) {
                out << line << "\n";
            }
            out.close();
        }
    }
};

#endif // MK_BOOT_CPP
