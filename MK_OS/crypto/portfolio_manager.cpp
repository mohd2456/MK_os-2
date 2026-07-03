// ============================================================
// MK OS - Crypto Portfolio Manager
// Tracks holdings, PnL, allocation, rebalancing, DCA
// Persists to SQLite for crash-safe storage
// ============================================================
#ifndef MK_PORTFOLIO_MANAGER_CPP
#define MK_PORTFOLIO_MANAGER_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <sqlite3.h>

// ============================================================
// Data Structures
// ============================================================

struct MKHolding {
    std::string symbol;
    double amount;
    double avgBuyPrice;
    double currentPrice;
    double totalInvested;
    double currentValue;
    double pnl;
    double pnlPercent;
    double allocationPercent;
    long lastUpdated;
};

struct MKDCASchedule {
    std::string symbol;
    double amountUSD;       // USD to invest per interval
    std::string interval;   // "daily", "weekly", "monthly"
    long nextExecution;
    bool active;
    int executionCount;
    double totalInvested;
};

struct MKRebalanceAction {
    std::string symbol;
    std::string action;     // "buy" or "sell"
    double amount;
    double reason_percent;  // How far off target
    std::string reasoning;
};

struct MKPortfolioSnapshot {
    double totalValue;
    double totalInvested;
    double totalPnl;
    double totalPnlPercent;
    int positionCount;
    std::vector<MKHolding> holdings;
    long timestamp;
};

// ============================================================
// MKPortfolioManager Class
// ============================================================

class MKPortfolioManager {
private:
    sqlite3* db_;
    std::map<std::string, MKHolding> holdings_;
    std::vector<MKDCASchedule> dcaSchedules_;
    std::map<std::string, double> targetAllocations_;
    std::string dbPath_;

    // Initialize SQLite database
    bool initDatabase() {
        int rc = sqlite3_open(dbPath_.c_str(), &db_);
        if (rc != SQLITE_OK) {
            db_ = nullptr;
            return false;
        }

        const char* createHoldingsTable =
            "CREATE TABLE IF NOT EXISTS holdings ("
            "  symbol TEXT PRIMARY KEY,"
            "  amount REAL NOT NULL DEFAULT 0,"
            "  avg_buy_price REAL NOT NULL DEFAULT 0,"
            "  total_invested REAL NOT NULL DEFAULT 0,"
            "  last_updated INTEGER NOT NULL DEFAULT 0"
            ");";

        const char* createTradesTable =
            "CREATE TABLE IF NOT EXISTS trades ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  symbol TEXT NOT NULL,"
            "  side TEXT NOT NULL,"
            "  amount REAL NOT NULL,"
            "  price REAL NOT NULL,"
            "  total_usd REAL NOT NULL,"
            "  timestamp INTEGER NOT NULL"
            ");";

        const char* createDCATable =
            "CREATE TABLE IF NOT EXISTS dca_schedules ("
            "  symbol TEXT PRIMARY KEY,"
            "  amount_usd REAL NOT NULL,"
            "  interval TEXT NOT NULL,"
            "  next_execution INTEGER NOT NULL,"
            "  active INTEGER NOT NULL DEFAULT 1,"
            "  execution_count INTEGER NOT NULL DEFAULT 0,"
            "  total_invested REAL NOT NULL DEFAULT 0"
            ");";

        const char* createSnapshotsTable =
            "CREATE TABLE IF NOT EXISTS portfolio_snapshots ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  total_value REAL NOT NULL,"
            "  total_invested REAL NOT NULL,"
            "  total_pnl REAL NOT NULL,"
            "  position_count INTEGER NOT NULL,"
            "  timestamp INTEGER NOT NULL"
            ");";

        char* errMsg = nullptr;
        sqlite3_exec(db_, createHoldingsTable, nullptr, nullptr, &errMsg);
        if (errMsg) { sqlite3_free(errMsg); errMsg = nullptr; }
        sqlite3_exec(db_, createTradesTable, nullptr, nullptr, &errMsg);
        if (errMsg) { sqlite3_free(errMsg); errMsg = nullptr; }
        sqlite3_exec(db_, createDCATable, nullptr, nullptr, &errMsg);
        if (errMsg) { sqlite3_free(errMsg); errMsg = nullptr; }
        sqlite3_exec(db_, createSnapshotsTable, nullptr, nullptr, &errMsg);
        if (errMsg) { sqlite3_free(errMsg); errMsg = nullptr; }

        return true;
    }

    // Load holdings from database
    void loadHoldings() {
        if (!db_) return;

        const char* sql = "SELECT symbol, amount, avg_buy_price, total_invested, last_updated FROM holdings;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            MKHolding h;
            h.symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            h.amount = sqlite3_column_double(stmt, 1);
            h.avgBuyPrice = sqlite3_column_double(stmt, 2);
            h.totalInvested = sqlite3_column_double(stmt, 3);
            h.lastUpdated = sqlite3_column_int64(stmt, 4);
            h.currentPrice = 0.0;
            h.currentValue = 0.0;
            h.pnl = 0.0;
            h.pnlPercent = 0.0;
            h.allocationPercent = 0.0;
            holdings_[h.symbol] = h;
        }

        sqlite3_finalize(stmt);
    }

    // Load DCA schedules
    void loadDCASchedules() {
        if (!db_) return;

        const char* sql = "SELECT symbol, amount_usd, interval, next_execution, active, "
                          "execution_count, total_invested FROM dca_schedules;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            MKDCASchedule dca;
            dca.symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            dca.amountUSD = sqlite3_column_double(stmt, 1);
            dca.interval = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            dca.nextExecution = sqlite3_column_int64(stmt, 3);
            dca.active = sqlite3_column_int(stmt, 4) != 0;
            dca.executionCount = sqlite3_column_int(stmt, 5);
            dca.totalInvested = sqlite3_column_double(stmt, 6);
            dcaSchedules_.push_back(dca);
        }

        sqlite3_finalize(stmt);
    }

    // Save a holding to database
    void saveHolding(const MKHolding& h) {
        if (!db_) return;

        const char* sql = "INSERT OR REPLACE INTO holdings (symbol, amount, avg_buy_price, "
                          "total_invested, last_updated) VALUES (?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

        sqlite3_bind_text(stmt, 1, h.symbol.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 2, h.amount);
        sqlite3_bind_double(stmt, 3, h.avgBuyPrice);
        sqlite3_bind_double(stmt, 4, h.totalInvested);
        sqlite3_bind_int64(stmt, 5, h.lastUpdated);

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // Record a trade
    void recordTrade(const std::string& symbol, const std::string& side,
                     double amount, double price) {
        if (!db_) return;

        const char* sql = "INSERT INTO trades (symbol, side, amount, price, total_usd, timestamp) "
                          "VALUES (?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

        sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, side.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 3, amount);
        sqlite3_bind_double(stmt, 4, price);
        sqlite3_bind_double(stmt, 5, amount * price);
        sqlite3_bind_int64(stmt, 6, std::time(nullptr));

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

public:
    MKPortfolioManager(const std::string& dbPath = "crypto_portfolio.db")
        : db_(nullptr), dbPath_(dbPath) {
        initDatabase();
        loadHoldings();
        loadDCASchedules();
    }

    ~MKPortfolioManager() {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

    // ========================================================
    // Record a buy trade
    // ========================================================
    void recordBuy(const std::string& symbol, double amount, double price) {
        auto& h = holdings_[symbol];
        h.symbol = symbol;

        // Update average buy price
        double newTotalInvested = h.totalInvested + (amount * price);
        double newAmount = h.amount + amount;
        if (newAmount > 0) {
            h.avgBuyPrice = newTotalInvested / newAmount;
        }
        h.amount = newAmount;
        h.totalInvested = newTotalInvested;
        h.lastUpdated = std::time(nullptr);

        saveHolding(h);
        recordTrade(symbol, "buy", amount, price);
    }

    // ========================================================
    // Record a sell trade
    // ========================================================
    void recordSell(const std::string& symbol, double amount, double price) {
        auto it = holdings_.find(symbol);
        if (it == holdings_.end()) return;

        auto& h = it->second;
        double sellAmount = std::min(amount, h.amount);

        // Reduce position proportionally
        double proportion = sellAmount / h.amount;
        h.totalInvested -= h.totalInvested * proportion;
        h.amount -= sellAmount;
        h.lastUpdated = std::time(nullptr);

        if (h.amount <= 0.0) {
            h.amount = 0.0;
            h.totalInvested = 0.0;
            h.avgBuyPrice = 0.0;
        }

        saveHolding(h);
        recordTrade(symbol, "sell", sellAmount, price);
    }

    // ========================================================
    // Update current prices
    // ========================================================
    void updatePrice(const std::string& symbol, double currentPrice) {
        auto it = holdings_.find(symbol);
        if (it == holdings_.end()) return;

        auto& h = it->second;
        h.currentPrice = currentPrice;
        h.currentValue = h.amount * currentPrice;
        h.pnl = h.currentValue - h.totalInvested;
        h.pnlPercent = (h.totalInvested > 0) ? (h.pnl / h.totalInvested * 100.0) : 0.0;
        h.lastUpdated = std::time(nullptr);
    }

    // ========================================================
    // Get portfolio snapshot
    // ========================================================
    MKPortfolioSnapshot getSnapshot() {
        MKPortfolioSnapshot snap;
        snap.totalValue = 0.0;
        snap.totalInvested = 0.0;
        snap.positionCount = 0;
        snap.timestamp = std::time(nullptr);

        for (auto& pair : holdings_) {
            auto& h = pair.second;
            if (h.amount > 0) {
                snap.totalValue += h.currentValue;
                snap.totalInvested += h.totalInvested;
                snap.positionCount++;
            }
        }

        // Calculate allocation percentages
        for (auto& pair : holdings_) {
            auto& h = pair.second;
            if (h.amount > 0 && snap.totalValue > 0) {
                h.allocationPercent = (h.currentValue / snap.totalValue) * 100.0;
                snap.holdings.push_back(h);
            }
        }

        snap.totalPnl = snap.totalValue - snap.totalInvested;
        snap.totalPnlPercent = (snap.totalInvested > 0) ?
            (snap.totalPnl / snap.totalInvested * 100.0) : 0.0;

        return snap;
    }

    // ========================================================
    // Rebalancing suggestions
    // ========================================================
    void setTargetAllocation(const std::string& symbol, double targetPercent) {
        targetAllocations_[symbol] = targetPercent;
    }

    std::vector<MKRebalanceAction> getRebalanceSuggestions() {
        std::vector<MKRebalanceAction> actions;
        auto snap = getSnapshot();

        if (snap.totalValue <= 0 || targetAllocations_.empty()) return actions;

        for (const auto& target : targetAllocations_) {
            auto it = holdings_.find(target.first);
            double currentAlloc = 0.0;

            if (it != holdings_.end() && it->second.amount > 0) {
                currentAlloc = (it->second.currentValue / snap.totalValue) * 100.0;
            }

            double diff = target.second - currentAlloc;

            // Only suggest rebalancing if off by more than 2%
            if (std::abs(diff) > 2.0) {
                MKRebalanceAction action;
                action.symbol = target.first;
                action.reason_percent = diff;

                if (diff > 0) {
                    action.action = "buy";
                    action.amount = (diff / 100.0) * snap.totalValue;
                    action.reasoning = target.first + " is underweight by " +
                                       std::to_string(diff).substr(0, 5) + "% - buy $" +
                                       std::to_string(action.amount).substr(0, 8);
                } else {
                    action.action = "sell";
                    action.amount = (std::abs(diff) / 100.0) * snap.totalValue;
                    action.reasoning = target.first + " is overweight by " +
                                       std::to_string(std::abs(diff)).substr(0, 5) + "% - sell $" +
                                       std::to_string(action.amount).substr(0, 8);
                }

                actions.push_back(action);
            }
        }

        return actions;
    }

    // ========================================================
    // DCA Schedule Management
    // ========================================================
    void addDCASchedule(const std::string& symbol, double amountUSD,
                        const std::string& interval) {
        MKDCASchedule dca;
        dca.symbol = symbol;
        dca.amountUSD = amountUSD;
        dca.interval = interval;
        dca.active = true;
        dca.executionCount = 0;
        dca.totalInvested = 0.0;

        // Calculate next execution
        long now = std::time(nullptr);
        if (interval == "daily") dca.nextExecution = now + 86400;
        else if (interval == "weekly") dca.nextExecution = now + 604800;
        else if (interval == "monthly") dca.nextExecution = now + 2592000;
        else dca.nextExecution = now + 86400;

        dcaSchedules_.push_back(dca);

        // Persist to database
        if (db_) {
            const char* sql = "INSERT OR REPLACE INTO dca_schedules "
                              "(symbol, amount_usd, interval, next_execution, active, "
                              "execution_count, total_invested) VALUES (?, ?, ?, ?, ?, ?, ?);";
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, dca.symbol.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt, 2, dca.amountUSD);
                sqlite3_bind_text(stmt, 3, dca.interval.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int64(stmt, 4, dca.nextExecution);
                sqlite3_bind_int(stmt, 5, 1);
                sqlite3_bind_int(stmt, 6, 0);
                sqlite3_bind_double(stmt, 7, 0.0);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
        }
    }

    std::vector<MKDCASchedule> getDCASchedulesDue() {
        std::vector<MKDCASchedule> due;
        long now = std::time(nullptr);
        for (auto& dca : dcaSchedules_) {
            if (dca.active && dca.nextExecution <= now) {
                due.push_back(dca);
            }
        }
        return due;
    }

    const std::vector<MKDCASchedule>& getDCASchedules() const {
        return dcaSchedules_;
    }

    // ========================================================
    // Portfolio Snapshot History (for tracking over time)
    // ========================================================
    void saveSnapshot() {
        if (!db_) return;
        auto snap = getSnapshot();

        const char* sql = "INSERT INTO portfolio_snapshots "
                          "(total_value, total_invested, total_pnl, position_count, timestamp) "
                          "VALUES (?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

        sqlite3_bind_double(stmt, 1, snap.totalValue);
        sqlite3_bind_double(stmt, 2, snap.totalInvested);
        sqlite3_bind_double(stmt, 3, snap.totalPnl);
        sqlite3_bind_int(stmt, 4, snap.positionCount);
        sqlite3_bind_int64(stmt, 5, snap.timestamp);

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // ========================================================
    // Getters
    // ========================================================
    int positionCount() const {
        int count = 0;
        for (const auto& pair : holdings_) {
            if (pair.second.amount > 0) count++;
        }
        return count;
    }

    double getTotalValue() {
        double total = 0.0;
        for (const auto& pair : holdings_) {
            total += pair.second.currentValue;
        }
        return total;
    }

    double getTotalInvested() const {
        double total = 0.0;
        for (const auto& pair : holdings_) {
            total += pair.second.totalInvested;
        }
        return total;
    }

    MKHolding getHolding(const std::string& symbol) const {
        auto it = holdings_.find(symbol);
        if (it != holdings_.end()) return it->second;
        MKHolding empty;
        empty.symbol = symbol;
        empty.amount = 0.0;
        empty.avgBuyPrice = 0.0;
        empty.currentPrice = 0.0;
        empty.currentValue = 0.0;
        empty.totalInvested = 0.0;
        empty.pnl = 0.0;
        empty.pnlPercent = 0.0;
        empty.allocationPercent = 0.0;
        empty.lastUpdated = 0;
        return empty;
    }

    bool hasPosition(const std::string& symbol) const {
        auto it = holdings_.find(symbol);
        return it != holdings_.end() && it->second.amount > 0.0;
    }
};

#endif // MK_PORTFOLIO_MANAGER_CPP
