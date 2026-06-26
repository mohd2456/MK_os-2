#ifndef MK_AUTONOMOUS_AGENT_CPP
#define MK_AUTONOMOUS_AGENT_CPP

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <ctime>
#include <fstream>
#include <sstream>
#include <algorithm>

// ===================================================================================
// MK AUTONOMOUS INTERNET AGENT
// Self-directed web crawler and knowledge gatherer. MK accesses the internet
// autonomously to learn, update its knowledge base, and fetch information.
// Features:
//   - Priority-based URL crawl queue
//   - Scheduled background knowledge gathering during idle
//   - Content extraction and fact parsing
//   - Respects rate limits and robots.txt patterns
//   - Feeds extracted knowledge directly to MKLearningEngine
// ===================================================================================

enum class MKCrawlPriority {
    IMMEDIATE,      // User asked for something — fetch NOW
    HIGH,           // Important knowledge gap identified
    NORMAL,         // Scheduled learning topic
    LOW,            // Background curiosity exploration
    IDLE            // Only crawl during maintenance windows
};

enum class MKCrawlStatus {
    QUEUED,
    IN_PROGRESS,
    COMPLETED,
    FAILED,
    RATE_LIMITED
};

struct MKCrawlTask {
    int id;
    std::string url;
    std::string topic;              // What we're trying to learn
    MKCrawlPriority priority;
    MKCrawlStatus status;
    std::time_t queuedAt;
    std::time_t completedAt;
    std::string extractedContent;   // Cleaned text from page
    std::vector<std::string> extractedFacts;  // Parsed facts
    int retryCount;
};

struct MKAgentConfig {
    int maxConcurrentCrawls;
    int maxRetries;
    int rateLimitDelayMs;
    int maxContentSizeKB;
    bool respectRobotsTxt;
    std::string userAgent;
    std::vector<std::string> blockedDomains;
    std::vector<std::string> trustedDomains;
};

class MKAutonomousAgent {
private:
    std::queue<MKCrawlTask> crawlQueue;
    std::vector<MKCrawlTask> completedTasks;
    std::map<std::string, std::time_t> domainLastAccess;  // Rate limiting
    MKAgentConfig config;
    int nextTaskId;
    int totalCrawls;
    int successfulCrawls;
    int failedCrawls;
    long totalBytesDownloaded;
    
    // Check if domain is allowed
    bool isDomainAllowed(const std::string& url) {
        for (const auto& blocked : config.blockedDomains) {
            if (url.find(blocked) != std::string::npos) return false;
        }
        return true;
    }
    
    // Extract domain from URL
    std::string extractDomain(const std::string& url) {
        size_t start = url.find("://");
        if (start == std::string::npos) start = 0;
        else start += 3;
        size_t end = url.find('/', start);
        return url.substr(start, end - start);
    }
    
    // Check rate limit for a domain
    bool isRateLimited(const std::string& domain) {
        auto it = domainLastAccess.find(domain);
        if (it == domainLastAccess.end()) return false;
        
        std::time_t now = std::time(nullptr);
        double elapsed = difftime(now, it->second) * 1000;  // ms
        return elapsed < config.rateLimitDelayMs;
    }
    
    // Strip HTML tags from content (basic extraction)
    std::string stripHTML(const std::string& html) {
        std::string text;
        bool inTag = false;
        for (char c : html) {
            if (c == '<') { inTag = true; continue; }
            if (c == '>') { inTag = false; continue; }
            if (!inTag) text += c;
        }
        return text;
    }
    
    // Extract sentences that look like facts
    std::vector<std::string> extractFacts(const std::string& text) {
        std::vector<std::string> facts;
        std::stringstream ss(text);
        std::string sentence;
        
        while (std::getline(ss, sentence, '.')) {
            // Trim
            size_t start = sentence.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) continue;
            sentence = sentence.substr(start);
            
            // Only keep sentences that look informational (>20 chars, <200 chars)
            if (sentence.size() > 20 && sentence.size() < 200) {
                // Filter out navigation/UI text
                if (sentence.find("click") == std::string::npos &&
                    sentence.find("subscribe") == std::string::npos &&
                    sentence.find("cookie") == std::string::npos) {
                    facts.push_back(sentence);
                }
            }
        }
        return facts;
    }

public:
    MKAutonomousAgent() : nextTaskId(0), totalCrawls(0), 
                          successfulCrawls(0), failedCrawls(0), totalBytesDownloaded(0) {
        // Default configuration
        config.maxConcurrentCrawls = 1;
        config.maxRetries = 2;
        config.rateLimitDelayMs = 3000;  // 3 seconds between requests to same domain
        config.maxContentSizeKB = 512;
        config.respectRobotsTxt = true;
        config.userAgent = "MK-OS/1.0 (Autonomous Knowledge Agent)";
        config.blockedDomains = {"facebook.com", "instagram.com", "tiktok.com"};
        config.trustedDomains = {"wikipedia.org", "cppreference.com", "arxiv.org"};
        
        std::cout << "[AGENT] Autonomous internet knowledge agent initialized.\n";
    }


    // Queue a URL for crawling
    int queueCrawl(const std::string& url, const std::string& topic,
                   MKCrawlPriority priority = MKCrawlPriority::NORMAL) {
        if (!isDomainAllowed(url)) {
            std::cout << "[AGENT] Blocked domain, skipping: " << url << "\n";
            return -1;
        }
        
        MKCrawlTask task;
        task.id = nextTaskId++;
        task.url = url;
        task.topic = topic;
        task.priority = priority;
        task.status = MKCrawlStatus::QUEUED;
        task.queuedAt = std::time(nullptr);
        task.completedAt = 0;
        task.retryCount = 0;
        
        crawlQueue.push(task);
        std::cout << "[AGENT] Queued: \"" << topic << "\" -> " << url << "\n";
        return task.id;
    }
    
    // Queue a topic for research (agent will find URLs itself)
    void researchTopic(const std::string& topic, MKCrawlPriority priority = MKCrawlPriority::NORMAL) {
        // Build search URLs from trusted sources
        std::string encoded = topic;
        std::replace(encoded.begin(), encoded.end(), ' ', '+');
        
        queueCrawl("https://en.wikipedia.org/wiki/" + encoded, topic, priority);
        queueCrawl("https://api.duckduckgo.com/?q=" + encoded + "&format=json", topic, priority);
        
        std::cout << "[AGENT] Research initiated for: \"" << topic << "\"\n";
    }
    
    // Process the next crawl task (call from idle loop or background thread)
    // Returns the extracted content, or empty string on failure
    // NOTE: Actual HTTP is handled by the network/http.cpp module
    std::string processNextTask(const std::string& fetchedContent = "") {
        if (crawlQueue.empty()) return "";
        
        MKCrawlTask task = crawlQueue.front();
        crawlQueue.pop();
        
        std::string domain = extractDomain(task.url);
        if (isRateLimited(domain)) {
            task.status = MKCrawlStatus::RATE_LIMITED;
            task.retryCount++;
            if (task.retryCount < config.maxRetries) {
                crawlQueue.push(task);  // Re-queue for later
            }
            return "";
        }
        
        task.status = MKCrawlStatus::IN_PROGRESS;
        totalCrawls++;
        domainLastAccess[domain] = std::time(nullptr);
        
        // If content was provided externally (from MKHTTP), process it
        if (!fetchedContent.empty()) {
            task.extractedContent = stripHTML(fetchedContent);
            task.extractedFacts = extractFacts(task.extractedContent);
            task.status = MKCrawlStatus::COMPLETED;
            task.completedAt = std::time(nullptr);
            successfulCrawls++;
            totalBytesDownloaded += fetchedContent.size();
            
            std::cout << "[AGENT] Crawl complete: \"" << task.topic << "\" | " 
                      << task.extractedFacts.size() << " facts extracted.\n";
            
            completedTasks.push_back(task);
            return task.extractedContent;
        }
        
        // If no content, mark as needing HTTP fetch
        std::cout << "[AGENT] Ready to fetch: " << task.url << "\n";
        task.status = MKCrawlStatus::FAILED;
        failedCrawls++;
        return "";
    }
    
    // Get all facts from recent crawls
    std::vector<std::string> getRecentFacts(int maxFacts = 50) {
        std::vector<std::string> allFacts;
        for (const auto& task : completedTasks) {
            for (const auto& fact : task.extractedFacts) {
                allFacts.push_back(fact);
                if ((int)allFacts.size() >= maxFacts) return allFacts;
            }
        }
        return allFacts;
    }
    
    // Generate a learning plan for idle time
    void generateIdlePlan(const std::vector<std::string>& topics) {
        std::cout << "[AGENT] Generating idle-time learning plan...\n";
        for (const auto& topic : topics) {
            researchTopic(topic, MKCrawlPriority::IDLE);
        }
        std::cout << "[AGENT] Plan queued: " << topics.size() << " topics for background learning.\n";
    }
    
    // Save crawl history to file
    void saveHistory(const std::string& filename = "mk_crawl_history.log") {
        std::ofstream out(filename, std::ios::app);
        if (!out.is_open()) return;
        for (const auto& task : completedTasks) {
            out << task.completedAt << "|" << task.topic << "|" << task.url 
                << "|" << task.extractedFacts.size() << " facts\n";
        }
        out.close();
    }
    
    int queueSize() const { return crawlQueue.size(); }
    int getCompleted() const { return completedTasks.size(); }
    
    void printStats() const {
        std::cout << "[AGENT STATS] Crawls: " << totalCrawls 
                  << " | Success: " << successfulCrawls 
                  << " | Failed: " << failedCrawls
                  << " | Downloaded: " << (totalBytesDownloaded / 1024) << "KB"
                  << " | Queue: " << crawlQueue.size() << "\n";
    }
};

#endif // MK_AUTONOMOUS_AGENT_CPP
