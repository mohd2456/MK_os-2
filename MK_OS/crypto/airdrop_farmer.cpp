// ============================================================
// MK OS - Airdrop Farmer
// Tracks upcoming airdrops, requirements, completion status
// Maintains protocol list with airdrop criteria
// ============================================================
#ifndef MK_AIRDROP_FARMER_CPP
#define MK_AIRDROP_FARMER_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>

// ============================================================
// Data Structures
// ============================================================

enum class MKAirdropStatus {
    UPCOMING,
    ACTIVE,
    COMPLETED,
    CLAIMED,
    EXPIRED,
    UNKNOWN
};

struct MKAirdropTask {
    std::string description;
    bool completed;
    std::string category;   // "bridge", "swap", "stake", "governance", "social"
    int priority;           // 1-5 (5 = most important)
};

struct MKAirdrop {
    std::string protocol;
    std::string chain;
    std::string description;
    MKAirdropStatus status;
    std::vector<MKAirdropTask> tasks;
    double estimatedValue;      // Estimated USD value
    double confidence;          // 0-1 how likely is the airdrop
    long deadline;              // Deadline timestamp (0 if unknown)
    long addedAt;
    std::string tier;           // "S", "A", "B", "C" (quality tier)
    std::string notes;
};

struct MKFarmingStats {
    int totalProtocols;
    int activeProtocols;
    int completedAirdrops;
    double totalEstimatedValue;
    double totalClaimed;
    int tasksCompleted;
    int tasksRemaining;
};

// ============================================================
// MKAirdropFarmer Class
// ============================================================

class MKAirdropFarmer {
private:
    std::vector<MKAirdrop> airdrops_;

    // Pre-populated list of known airdrop opportunities
    void initializeKnownAirdrops() {
        // These are example protocols that may have future airdrops
        // Based on common airdrop farming strategies

        addAirdrop("LayerZero", "Multi-chain", "Cross-chain messaging protocol",
            {{"Bridge assets using Stargate", false, "bridge", 5},
             {"Use LayerZero OFT transfers", false, "bridge", 4},
             {"Bridge on 5+ chains", false, "bridge", 4},
             {"Make 10+ cross-chain txns", false, "bridge", 3},
             {"Use on consecutive months", false, "activity", 3}},
            2000.0, 0.7, 0, "A");

        addAirdrop("zkSync", "zkSync Era", "ZK rollup L2 scaling solution",
            {{"Bridge ETH to zkSync Era", false, "bridge", 5},
             {"Swap on SyncSwap or Mute", false, "swap", 4},
             {"Provide liquidity", false, "stake", 3},
             {"Use 5+ dApps on zkSync", false, "activity", 4},
             {"Transact in different months", false, "activity", 3},
             {"Hold NFTs on zkSync", false, "activity", 2}},
            3000.0, 0.6, 0, "S");

        addAirdrop("Scroll", "Scroll", "ZK rollup L2",
            {{"Bridge ETH to Scroll", false, "bridge", 5},
             {"Swap on Ambient or SkyDrome", false, "swap", 4},
             {"Provide liquidity on Scroll DEX", false, "stake", 3},
             {"Deploy a contract on Scroll", false, "activity", 3},
             {"Use lending protocols", false, "activity", 4}},
            1500.0, 0.65, 0, "A");

        addAirdrop("Linea", "Linea", "Consensys L2 zkEVM",
            {{"Bridge ETH to Linea", false, "bridge", 5},
             {"Swap on Linea DEXes", false, "swap", 4},
             {"Complete Linea Voyage tasks", false, "activity", 5},
             {"Provide liquidity", false, "stake", 3},
             {"Use 3+ protocols", false, "activity", 3}},
            1200.0, 0.6, 0, "A");

        addAirdrop("Base", "Base", "Coinbase L2",
            {{"Bridge to Base", false, "bridge", 5},
             {"Swap on Aerodrome", false, "swap", 4},
             {"Mint NFTs on Base", false, "activity", 3},
             {"Use Friend.tech or Farcaster", false, "social", 3},
             {"Provide liquidity on Base", false, "stake", 3}},
            800.0, 0.5, 0, "B");

        addAirdrop("Eigenlayer", "Ethereum", "Restaking protocol",
            {{"Restake ETH via Eigenlayer", false, "stake", 5},
             {"Use liquid restaking (ezETH, rETH)", false, "stake", 5},
             {"Delegate to operators", false, "governance", 4},
             {"Restake multiple LSTs", false, "stake", 3}},
            5000.0, 0.75, 0, "S");

        addAirdrop("Blast", "Blast", "Yield-bearing L2",
            {{"Bridge ETH to Blast", false, "bridge", 5},
             {"Earn Blast Points", false, "activity", 5},
             {"Use Blast dApps", false, "activity", 4},
             {"Invite friends for Gold", false, "social", 3},
             {"Hold USDB on Blast", false, "stake", 3}},
            2500.0, 0.8, 0, "S");

        addAirdrop("Monad", "Monad", "High-performance EVM L1",
            {{"Join Monad Discord", false, "social", 3},
             {"Complete testnet tasks", false, "activity", 5},
             {"Follow on Twitter", false, "social", 2},
             {"Participate in community", false, "social", 3}},
            3000.0, 0.4, 0, "A");

        addAirdrop("Berachain", "Berachain", "Proof of Liquidity L1",
            {{"Use Berachain testnet", false, "activity", 5},
             {"Provide liquidity on testnet", false, "stake", 4},
             {"Participate in governance", false, "governance", 3},
             {"Complete Galxe quests", false, "social", 3}},
            2000.0, 0.5, 0, "A");

        addAirdrop("Hyperliquid", "Hyperliquid", "Perp DEX",
            {{"Trade on Hyperliquid", false, "swap", 5},
             {"Provide liquidity", false, "stake", 4},
             {"Reach trading volume milestones", false, "activity", 4},
             {"Use limit orders", false, "swap", 3}},
            4000.0, 0.7, 0, "S");
    }

public:
    MKAirdropFarmer() {
        initializeKnownAirdrops();
    }

    // ========================================================
    // Add a new airdrop to track
    // ========================================================
    void addAirdrop(const std::string& protocol, const std::string& chain,
                    const std::string& description,
                    const std::vector<MKAirdropTask>& tasks,
                    double estimatedValue, double confidence,
                    long deadline, const std::string& tier) {
        MKAirdrop airdrop;
        airdrop.protocol = protocol;
        airdrop.chain = chain;
        airdrop.description = description;
        airdrop.status = MKAirdropStatus::ACTIVE;
        airdrop.tasks = tasks;
        airdrop.estimatedValue = estimatedValue;
        airdrop.confidence = confidence;
        airdrop.deadline = deadline;
        airdrop.addedAt = std::time(nullptr);
        airdrop.tier = tier;
        airdrops_.push_back(airdrop);
    }

    // ========================================================
    // Complete a task
    // ========================================================
    bool completeTask(const std::string& protocol, int taskIndex) {
        for (auto& airdrop : airdrops_) {
            if (airdrop.protocol == protocol) {
                if (taskIndex >= 0 && taskIndex < (int)airdrop.tasks.size()) {
                    airdrop.tasks[taskIndex].completed = true;
                    // Check if all tasks are done
                    bool allDone = true;
                    for (const auto& task : airdrop.tasks) {
                        if (!task.completed) { allDone = false; break; }
                    }
                    if (allDone) {
                        airdrop.status = MKAirdropStatus::COMPLETED;
                    }
                    return true;
                }
            }
        }
        return false;
    }

    // ========================================================
    // Get farming statistics
    // ========================================================
    MKFarmingStats getStats() const {
        MKFarmingStats stats;
        stats.totalProtocols = static_cast<int>(airdrops_.size());
        stats.activeProtocols = 0;
        stats.completedAirdrops = 0;
        stats.totalEstimatedValue = 0.0;
        stats.totalClaimed = 0.0;
        stats.tasksCompleted = 0;
        stats.tasksRemaining = 0;

        for (const auto& airdrop : airdrops_) {
            stats.totalEstimatedValue += airdrop.estimatedValue * airdrop.confidence;

            if (airdrop.status == MKAirdropStatus::ACTIVE) stats.activeProtocols++;
            if (airdrop.status == MKAirdropStatus::COMPLETED ||
                airdrop.status == MKAirdropStatus::CLAIMED) stats.completedAirdrops++;

            for (const auto& task : airdrop.tasks) {
                if (task.completed) stats.tasksCompleted++;
                else stats.tasksRemaining++;
            }
        }

        return stats;
    }

    // ========================================================
    // Get airdrops by tier
    // ========================================================
    std::vector<MKAirdrop> getByTier(const std::string& tier) const {
        std::vector<MKAirdrop> result;
        for (const auto& airdrop : airdrops_) {
            if (airdrop.tier == tier) result.push_back(airdrop);
        }
        return result;
    }

    // ========================================================
    // Get priority tasks (highest priority uncompleted tasks)
    // ========================================================
    std::vector<std::pair<std::string, MKAirdropTask>> getPriorityTasks(int limit = 10) const {
        std::vector<std::pair<std::string, MKAirdropTask>> tasks;

        for (const auto& airdrop : airdrops_) {
            if (airdrop.status != MKAirdropStatus::ACTIVE) continue;
            for (const auto& task : airdrop.tasks) {
                if (!task.completed) {
                    tasks.push_back({airdrop.protocol, task});
                }
            }
        }

        // Sort by priority (descending)
        std::sort(tasks.begin(), tasks.end(),
            [](const auto& a, const auto& b) {
                return a.second.priority > b.second.priority;
            });

        if ((int)tasks.size() > limit) tasks.resize(limit);
        return tasks;
    }

    // ========================================================
    // Getters
    // ========================================================
    const std::vector<MKAirdrop>& getAllAirdrops() const { return airdrops_; }

    int protocolCount() const { return static_cast<int>(airdrops_.size()); }

    double getTotalEstimatedValue() const {
        double total = 0.0;
        for (const auto& a : airdrops_) {
            total += a.estimatedValue * a.confidence;
        }
        return total;
    }

    std::string getSummary() const {
        auto stats = getStats();
        std::string summary;
        summary += "Airdrop Farmer Status:\n";
        summary += "  Tracking: " + std::to_string(stats.totalProtocols) + " protocols\n";
        summary += "  Active: " + std::to_string(stats.activeProtocols) + "\n";
        summary += "  Completed: " + std::to_string(stats.completedAirdrops) + "\n";
        summary += "  Tasks done: " + std::to_string(stats.tasksCompleted) + "/" +
                   std::to_string(stats.tasksCompleted + stats.tasksRemaining) + "\n";
        summary += "  Est. value: $" + std::to_string((int)stats.totalEstimatedValue) + "\n";
        return summary;
    }
};

#endif // MK_AIRDROP_FARMER_CPP
