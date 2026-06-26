#ifndef MK_DAILY_BRIEFING_CPP
#define MK_DAILY_BRIEFING_CPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <fstream>

// ===================================================================================
// MK DAILY BRIEFING GENERATOR
// Compiles a personalized morning report covering:
//   - System health (thermal, battery, disk)
//   - Overnight build progress (from self-correction engine)
//   - Knowledge learned since last briefing
//   - Pending tasks and schedule
//   - Weather/time info (if network available)
// Output is formatted for Telegram push notification.
// ===================================================================================

struct MKBriefingSection {
    std::string title;
    std::string emoji;
    std::vector<std::string> items;
};

struct MKSystemSnapshot {
    float cpuTempC;
    float batteryPercent;
    bool onAC;
    float diskUsedPercent;
    long freeStorageMB;
    int activeProcesses;
    long uptimeHours;
};

struct MKBuildProgress {
    int totalBuilds;
    int successfulBuilds;
    std::string lastBuildFile;
    std::string lastBuildStatus;
    std::time_t lastBuildTime;
};

struct MKLearningProgress {
    int factsLearnedToday;
    int totalFacts;
    int uncertainFacts;
    std::vector<std::string> recentTopics;
};

class MKDailyBriefing {
private:
    std::vector<MKBriefingSection> sections;
    std::string lastBriefing;
    std::time_t lastBriefingTime;
    std::string briefingLogFile;
    
    std::string getTimeGreeting() {
        std::time_t now = std::time(nullptr);
        struct tm* t = std::localtime(&now);
        int hour = t->tm_hour;
        
        if (hour < 6) return "Late night";
        if (hour < 12) return "Good morning";
        if (hour < 17) return "Good afternoon";
        if (hour < 21) return "Good evening";
        return "Late night";
    }
    
    std::string formatDate() {
        std::time_t now = std::time(nullptr);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%A, %B %d %Y", std::localtime(&now));
        return std::string(buf);
    }
    
    std::string formatTime() {
        std::time_t now = std::time(nullptr);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%H:%M", std::localtime(&now));
        return std::string(buf);
    }

public:
    MKDailyBriefing() : lastBriefingTime(0), briefingLogFile("mk_briefings.log") {
        std::cout << "[BRIEFING] Daily report generator ready.\n";
    }
    
    // Generate full briefing from all subsystem data
    std::string generate(const std::string& userName,
                         const MKSystemSnapshot& system,
                         const MKBuildProgress& builds,
                         const MKLearningProgress& learning) {
        sections.clear();
        std::stringstream brief;
        
        // Header
        brief << "━━━━━━━━━━━━━━━━━━━━━━━━\n";
        brief << "  MK DAILY BRIEFING\n";
        brief << "━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        brief << getTimeGreeting() << ", " << userName << "!\n";
        brief << formatDate() << " | " << formatTime() << "\n\n";
        
        // System Health Section
        brief << "--- SYSTEM HEALTH ---\n";
        brief << "  CPU: " << system.cpuTempC << "C";
        if (system.cpuTempC > 70) brief << " [HOT]";
        else if (system.cpuTempC > 50) brief << " [WARM]";
        else brief << " [COOL]";
        brief << "\n";
        
        brief << "  Battery: " << system.batteryPercent << "% " 
              << (system.onAC ? "[CHARGING]" : "[BATTERY]") << "\n";
        brief << "  Storage: " << system.freeStorageMB << "MB free ("
              << system.diskUsedPercent << "% used)\n";
        brief << "  Uptime: " << system.uptimeHours << " hours\n";
        brief << "  Processes: " << system.activeProcesses << " active\n\n";
        
        // Build Progress
        brief << "--- BUILD PROGRESS ---\n";
        if (builds.totalBuilds > 0) {
            brief << "  Attempts: " << builds.totalBuilds << " | Success: " << builds.successfulBuilds << "\n";
            brief << "  Last: " << builds.lastBuildFile << " -> " << builds.lastBuildStatus << "\n";
            
            float rate = (builds.totalBuilds > 0) ? 
                (float)builds.successfulBuilds / builds.totalBuilds * 100.0f : 0.0f;
            brief << "  Success Rate: " << (int)rate << "%\n";
        } else {
            brief << "  No builds since last briefing.\n";
        }
        brief << "\n";
        
        // Learning Progress
        brief << "--- KNOWLEDGE ---\n";
        brief << "  Facts learned today: " << learning.factsLearnedToday << "\n";
        brief << "  Total knowledge base: " << learning.totalFacts << " facts\n";
        brief << "  Uncertain (needs review): " << learning.uncertainFacts << "\n";
        
        if (!learning.recentTopics.empty()) {
            brief << "  Recent topics: ";
            for (size_t i = 0; i < learning.recentTopics.size() && i < 3; i++) {
                brief << learning.recentTopics[i];
                if (i < learning.recentTopics.size() - 1 && i < 2) brief << ", ";
            }
            brief << "\n";
        }
        brief << "\n";
        
        // Recommendations
        brief << "--- RECOMMENDATIONS ---\n";
        if (system.cpuTempC > 70) {
            brief << "  [!] High CPU temp. Consider closing background tasks.\n";
        }
        if (!system.onAC && system.batteryPercent < 30) {
            brief << "  [!] Low battery. Heavy tasks deferred until AC connected.\n";
        }
        if (system.freeStorageMB < 5000) {
            brief << "  [!] Disk space low. Consider compacting vector indexes.\n";
        }
        if (learning.uncertainFacts > 10) {
            brief << "  [i] " << learning.uncertainFacts << " facts need verification.\n";
        }
        if (system.onAC && system.cpuTempC < 50) {
            brief << "  [+] Good time for background indexing/model optimization.\n";
        }
        
        brief << "\n━━━━━━━━━━━━━━━━━━━━━━━━\n";
        brief << "  MK-OS v2026 | Local AI\n";
        brief << "━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        lastBriefing = brief.str();
        lastBriefingTime = std::time(nullptr);
        
        // Save to log
        std::ofstream log(briefingLogFile, std::ios::app);
        if (log.is_open()) {
            log << "\n" << lastBriefing << "\n";
            log.close();
        }
        
        return lastBriefing;
    }
    
    // Quick status one-liner (for Telegram /status command)
    std::string quickStatus(const MKSystemSnapshot& system) {
        std::stringstream ss;
        ss << "MK " << formatTime() << " | ";
        ss << system.cpuTempC << "C | ";
        ss << system.batteryPercent << "%" << (system.onAC ? " AC" : " BAT") << " | ";
        ss << system.freeStorageMB << "MB free";
        return ss.str();
    }
    
    // Check if it's time for a new briefing (once per day)
    bool shouldGenerate() {
        if (lastBriefingTime == 0) return true;
        
        std::time_t now = std::time(nullptr);
        double hours = difftime(now, lastBriefingTime) / 3600.0;
        return hours >= 24.0;
    }
    
    std::string getLastBriefing() const { return lastBriefing; }
};

#endif // MK_DAILY_BRIEFING_CPP
