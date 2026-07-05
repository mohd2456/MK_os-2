// ============================================================
// MK OS - Meme Coin Paper Trading Module
// Simulated trading with $20 fake wallet, realistic fees,
// CoinGecko real prices, and SQLite persistence.
// ============================================================
#ifndef MK_PAPER_TRADING_CPP
#define MK_PAPER_TRADING_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <curl/curl.h>
#include <sqlite3.h>

// ============================================================
// Paper Trade Record
// ============================================================
struct MKPaperTrade {
    int id;
    std::string symbol;
    std::string side;        // "buy" or "sell"
    double quantity;
    double price;
    double gasFee;
    double slippagePct;
    double dexFee;
    double totalCost;        // total USD spent (buy) or received (sell)
    long timestamp;
};

// ============================================================
// Paper Trading Result
// ============================================================
struct MKPaperTradeResult {
    bool success;
    std::string message;
    std::string error;
};

// ============================================================
// Portfolio Holding
// ============================================================
struct MKPaperHolding {
    std::string symbol;
    double quantity;
    double avgCost;
    double currentPrice;
    double unrealizedPnl;
};

// ============================================================
// Trading Stats
// ============================================================
struct MKPaperStats {
    double totalPnl;
    int totalTrades;
    int winningTrades;
    int losingTrades;
    double winRate;
    double bestTrade;
    double worstTrade;
    double currentBalance;
};

// ============================================================
// MKPaperTrading Class
// ============================================================
class MKPaperTrading {
private:
    static constexpr double STARTING_BALANCE = 20.00;
    static constexpr double GAS_FEE_MIN = 0.50;
    static constexpr double GAS_FEE_MAX = 2.00;
    static constexpr double SLIPPAGE_MIN = 0.01;  // 1%
    static constexpr double SLIPPAGE_MAX = 0.05;  // 5%
    static constexpr double DEX_FEE_RATE = 0.003; // 0.3%

    double cashBalance_;
    std::map<std::string, double> holdings_;      // symbol -> quantity
    std::map<std::string, double> avgCosts_;      // symbol -> avg cost per unit
    std::vector<MKPaperTrade> tradeHistory_;
    std::string dbPath_;
    sqlite3* db_;
    bool initialized_;
    unsigned int rngSeed_;

    // Curl write callback (static, scoped to this class)
    static size_t PaperCurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        if (!userp) return 0;
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    // Generate a random double in [min, max]
    double randomInRange(double min, double max) {
        double r = (double)rand_r(&rngSeed_) / (double)RAND_MAX;
        return min + r * (max - min);
    }

    // Fetch current price from CoinGecko
    // Returns -1.0 on failure
    double fetchPrice(const std::string& coinId) {
        std::string url = "https://api.coingecko.com/api/v3/simple/price?ids=" 
                          + coinId + "&vs_currencies=usd";

        std::string readBuffer;
        CURL* curl = curl_easy_init();
        if (!curl) return -1.0;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, PaperCurlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "MK-OS/2.0");

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) return -1.0;
        if (readBuffer.empty()) return -1.0;

        // Parse: {"coinId":{"usd":0.001234}}
        // Find "usd" value
        size_t usdPos = readBuffer.find("\"usd\"");
        if (usdPos == std::string::npos) return -1.0;

        size_t colonPos = readBuffer.find(':', usdPos + 5);
        if (colonPos == std::string::npos) return -1.0;

        size_t valStart = colonPos + 1;
        while (valStart < readBuffer.size() && 
               (readBuffer[valStart] == ' ' || readBuffer[valStart] == '\t'))
            valStart++;

        // Extract numeric value
        size_t valEnd = valStart;
        while (valEnd < readBuffer.size() && 
               (readBuffer[valEnd] == '.' || readBuffer[valEnd] == '-' || 
                readBuffer[valEnd] == 'e' || readBuffer[valEnd] == 'E' ||
                readBuffer[valEnd] == '+' ||
                (readBuffer[valEnd] >= '0' && readBuffer[valEnd] <= '9')))
            valEnd++;

        if (valEnd == valStart) return -1.0;

        std::string valStr = readBuffer.substr(valStart, valEnd - valStart);
        try {
            double price = std::stod(valStr);
            if (price <= 0.0) return -1.0;
            return price;
        } catch (...) {
            return -1.0;
        }
    }

    // Initialize SQLite database
    bool initDB() {
        if (dbPath_.empty()) return false;

        int rc = sqlite3_open(dbPath_.c_str(), &db_);
        if (rc != SQLITE_OK) {
            db_ = nullptr;
            return false;
        }

        // Create tables
        const char* createTradesSQL = 
            "CREATE TABLE IF NOT EXISTS paper_trades ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  symbol TEXT NOT NULL,"
            "  side TEXT NOT NULL,"
            "  quantity REAL NOT NULL,"
            "  price REAL NOT NULL,"
            "  gas_fee REAL NOT NULL,"
            "  slippage_pct REAL NOT NULL,"
            "  dex_fee REAL NOT NULL,"
            "  total_cost REAL NOT NULL,"
            "  timestamp INTEGER NOT NULL"
            ");";

        const char* createPortfolioSQL = 
            "CREATE TABLE IF NOT EXISTS paper_portfolio ("
            "  symbol TEXT PRIMARY KEY,"
            "  quantity REAL NOT NULL,"
            "  avg_cost REAL NOT NULL"
            ");";

        const char* createBalanceSQL = 
            "CREATE TABLE IF NOT EXISTS paper_balance ("
            "  id INTEGER PRIMARY KEY CHECK (id = 1),"
            "  cash_usd REAL NOT NULL"
            ");";

        char* errMsg = nullptr;
        sqlite3_exec(db_, createTradesSQL, nullptr, nullptr, &errMsg);
        if (errMsg) { sqlite3_free(errMsg); errMsg = nullptr; }
        sqlite3_exec(db_, createPortfolioSQL, nullptr, nullptr, &errMsg);
        if (errMsg) { sqlite3_free(errMsg); errMsg = nullptr; }
        sqlite3_exec(db_, createBalanceSQL, nullptr, nullptr, &errMsg);
        if (errMsg) { sqlite3_free(errMsg); errMsg = nullptr; }

        return true;
    }

    // Load state from SQLite
    void loadState() {
        if (!db_) return;

        // Load balance
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, "SELECT cash_usd FROM paper_balance WHERE id = 1;", -1, &stmt, nullptr);
        if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
            cashBalance_ = sqlite3_column_double(stmt, 0);
        }
        if (stmt) sqlite3_finalize(stmt);

        // Load portfolio
        stmt = nullptr;
        rc = sqlite3_prepare_v2(db_, "SELECT symbol, quantity, avg_cost FROM paper_portfolio;", -1, &stmt, nullptr);
        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                std::string sym = (const char*)sqlite3_column_text(stmt, 0);
                double qty = sqlite3_column_double(stmt, 1);
                double avg = sqlite3_column_double(stmt, 2);
                holdings_[sym] = qty;
                avgCosts_[sym] = avg;
            }
        }
        if (stmt) sqlite3_finalize(stmt);

        // Load trade history
        stmt = nullptr;
        rc = sqlite3_prepare_v2(db_, 
            "SELECT id, symbol, side, quantity, price, gas_fee, slippage_pct, dex_fee, total_cost, timestamp "
            "FROM paper_trades ORDER BY id ASC;", -1, &stmt, nullptr);
        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                MKPaperTrade t;
                t.id = sqlite3_column_int(stmt, 0);
                t.symbol = (const char*)sqlite3_column_text(stmt, 1);
                t.side = (const char*)sqlite3_column_text(stmt, 2);
                t.quantity = sqlite3_column_double(stmt, 3);
                t.price = sqlite3_column_double(stmt, 4);
                t.gasFee = sqlite3_column_double(stmt, 5);
                t.slippagePct = sqlite3_column_double(stmt, 6);
                t.dexFee = sqlite3_column_double(stmt, 7);
                t.totalCost = sqlite3_column_double(stmt, 8);
                t.timestamp = (long)sqlite3_column_int64(stmt, 9);
                tradeHistory_.push_back(t);
            }
        }
        if (stmt) sqlite3_finalize(stmt);
    }

    // Save balance to DB
    void saveBalance() {
        if (!db_) return;
        char* errMsg = nullptr;
        std::string sql = "INSERT OR REPLACE INTO paper_balance (id, cash_usd) VALUES (1, " 
                          + std::to_string(cashBalance_) + ");";
        sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
        if (errMsg) sqlite3_free(errMsg);
    }

    // Save a portfolio entry
    void saveHolding(const std::string& symbol) {
        if (!db_) return;

        auto hIt = holdings_.find(symbol);
        if (hIt == holdings_.end() || hIt->second <= 0.0) {
            // Remove from portfolio
            std::string sql = "DELETE FROM paper_portfolio WHERE symbol = '" + symbol + "';";
            sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, nullptr);
        } else {
            double avg = avgCosts_.count(symbol) ? avgCosts_[symbol] : 0.0;
            std::string sql = "INSERT OR REPLACE INTO paper_portfolio (symbol, quantity, avg_cost) VALUES ('" 
                              + symbol + "', " + std::to_string(hIt->second) + ", " + std::to_string(avg) + ");";
            sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, nullptr);
        }
    }

    // Save a trade record
    void saveTrade(const MKPaperTrade& trade) {
        if (!db_) return;
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "INSERT INTO paper_trades (symbol, side, quantity, price, gas_fee, slippage_pct, dex_fee, total_cost, timestamp) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, trade.symbol.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, trade.side.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 3, trade.quantity);
            sqlite3_bind_double(stmt, 4, trade.price);
            sqlite3_bind_double(stmt, 5, trade.gasFee);
            sqlite3_bind_double(stmt, 6, trade.slippagePct);
            sqlite3_bind_double(stmt, 7, trade.dexFee);
            sqlite3_bind_double(stmt, 8, trade.totalCost);
            sqlite3_bind_int64(stmt, 9, trade.timestamp);
            sqlite3_step(stmt);
        }
        if (stmt) sqlite3_finalize(stmt);
    }

public:
    // Constructor: path to SQLite DB file for persistence
    MKPaperTrading(const std::string& dbPath = "paper_trading.db") 
        : cashBalance_(STARTING_BALANCE),
          dbPath_(dbPath),
          db_(nullptr),
          initialized_(false),
          rngSeed_((unsigned int)time(nullptr))
    {
        if (initDB()) {
            // Check if balance exists in DB
            sqlite3_stmt* stmt = nullptr;
            int rc = sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM paper_balance;", -1, &stmt, nullptr);
            bool hasState = false;
            if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
                hasState = sqlite3_column_int(stmt, 0) > 0;
            }
            if (stmt) sqlite3_finalize(stmt);

            if (hasState) {
                loadState();
            } else {
                saveBalance();
            }
            initialized_ = true;
        }
    }

    ~MKPaperTrading() {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

    // Set RNG seed (for deterministic testing)
    void setSeed(unsigned int seed) { rngSeed_ = seed; }

    // Get current cash balance
    double getBalance() const { return cashBalance_; }

    // Get number of trades
    int getTradeCount() const { return (int)tradeHistory_.size(); }

    // Check if initialized
    bool isInitialized() const { return initialized_; }

    // ============================================================
    // BUY: Purchase a meme coin with USD
    // ============================================================
    MKPaperTradeResult buy(const std::string& symbol, double amountUsd) {
        if (symbol.empty()) {
            return {false, "", "Symbol cannot be empty"};
        }
        if (amountUsd <= 0.0) {
            return {false, "", "Amount must be positive"};
        }

        // Check sufficient funds
        if (amountUsd > cashBalance_) {
            return {false, "", "Insufficient funds. Balance: $" + 
                    formatMoney(cashBalance_) + ", requested: $" + formatMoney(amountUsd)};
        }

        // Fetch real price from CoinGecko
        double price = fetchPrice(symbol);
        if (price < 0.0) {
            return {false, "", "Failed to fetch price for '" + symbol + 
                    "' from CoinGecko. Check the coin ID is valid."};
        }

        // Calculate fees
        double gasFee = randomInRange(GAS_FEE_MIN, GAS_FEE_MAX);
        double slippagePct = randomInRange(SLIPPAGE_MIN, SLIPPAGE_MAX);
        double dexFee = amountUsd * DEX_FEE_RATE;

        // Total fees
        double totalFees = gasFee + dexFee;
        if (totalFees >= amountUsd) {
            return {false, "", "Fees ($" + formatMoney(totalFees) + 
                    ") exceed trade amount ($" + formatMoney(amountUsd) + ")"};
        }

        // Amount available for purchase after fees
        double netAmount = amountUsd - totalFees;

        // Apply slippage to price (price goes UP for buyer)
        double effectivePrice = price * (1.0 + slippagePct);

        // Quantity purchased
        double quantity = netAmount / effectivePrice;

        // Update state
        cashBalance_ -= amountUsd;

        // Update average cost
        double prevQty = holdings_.count(symbol) ? holdings_[symbol] : 0.0;
        double prevAvg = avgCosts_.count(symbol) ? avgCosts_[symbol] : 0.0;
        double totalQty = prevQty + quantity;
        double newAvg = (totalQty > 0.0) ? ((prevAvg * prevQty) + (effectivePrice * quantity)) / totalQty : effectivePrice;
        holdings_[symbol] = totalQty;
        avgCosts_[symbol] = newAvg;

        // Record trade
        MKPaperTrade trade;
        trade.id = (int)tradeHistory_.size() + 1;
        trade.symbol = symbol;
        trade.side = "buy";
        trade.quantity = quantity;
        trade.price = effectivePrice;
        trade.gasFee = gasFee;
        trade.slippagePct = slippagePct;
        trade.dexFee = dexFee;
        trade.totalCost = amountUsd;
        trade.timestamp = (long)time(nullptr);
        tradeHistory_.push_back(trade);

        // Persist
        saveBalance();
        saveHolding(symbol);
        saveTrade(trade);

        // Build result message
        std::ostringstream msg;
        msg << std::fixed << std::setprecision(6);
        msg << "Bought " << quantity << " " << symbol << " @ $" << effectivePrice;
        msg << std::setprecision(2);
        msg << " | Spent: $" << amountUsd;
        msg << " | Fees: gas=$" << gasFee << " dex=$" << dexFee;
        msg << " | Slippage: " << std::setprecision(1) << (slippagePct * 100.0) << "%";
        msg << " | Balance: $" << std::setprecision(2) << cashBalance_;

        return {true, msg.str(), ""};
    }

    // ============================================================
    // SELL: Sell a held coin (amount=quantity, or 0 for all)
    // ============================================================
    MKPaperTradeResult sell(const std::string& symbol, double amount = 0.0) {
        if (symbol.empty()) {
            return {false, "", "Symbol cannot be empty"};
        }

        // Check if we hold this coin
        if (holdings_.find(symbol) == holdings_.end() || holdings_[symbol] <= 0.0) {
            return {false, "", "You don't hold any " + symbol};
        }

        double heldQty = holdings_[symbol];
        double sellQty = (amount <= 0.0 || amount >= heldQty) ? heldQty : amount;

        // Fetch real price from CoinGecko
        double price = fetchPrice(symbol);
        if (price < 0.0) {
            return {false, "", "Failed to fetch price for '" + symbol + 
                    "' from CoinGecko. Check the coin ID is valid."};
        }

        // Calculate fees
        double gasFee = randomInRange(GAS_FEE_MIN, GAS_FEE_MAX);
        double slippagePct = randomInRange(SLIPPAGE_MIN, SLIPPAGE_MAX);

        // Apply slippage to price (price goes DOWN for seller)
        double effectivePrice = price * (1.0 - slippagePct);
        double grossProceeds = sellQty * effectivePrice;
        double dexFee = grossProceeds * DEX_FEE_RATE;
        double netProceeds = grossProceeds - gasFee - dexFee;

        if (netProceeds < 0.0) netProceeds = 0.0;

        // Update state
        cashBalance_ += netProceeds;
        holdings_[symbol] -= sellQty;
        if (holdings_[symbol] <= 1e-12) {
            holdings_.erase(symbol);
            avgCosts_.erase(symbol);
        }

        // Record trade
        MKPaperTrade trade;
        trade.id = (int)tradeHistory_.size() + 1;
        trade.symbol = symbol;
        trade.side = "sell";
        trade.quantity = sellQty;
        trade.price = effectivePrice;
        trade.gasFee = gasFee;
        trade.slippagePct = slippagePct;
        trade.dexFee = dexFee;
        trade.totalCost = netProceeds;
        trade.timestamp = (long)time(nullptr);
        tradeHistory_.push_back(trade);

        // Persist
        saveBalance();
        saveHolding(symbol);
        saveTrade(trade);

        // Build result message
        std::ostringstream msg;
        msg << std::fixed << std::setprecision(6);
        msg << "Sold " << sellQty << " " << symbol << " @ $" << effectivePrice;
        msg << std::setprecision(2);
        msg << " | Received: $" << netProceeds;
        msg << " | Fees: gas=$" << gasFee << " dex=$" << dexFee;
        msg << " | Slippage: " << std::setprecision(1) << (slippagePct * 100.0) << "%";
        msg << " | Balance: $" << std::setprecision(2) << cashBalance_;

        return {true, msg.str(), ""};
    }

    // ============================================================
    // GET PORTFOLIO: Current holdings with unrealized P/L
    // ============================================================
    std::string getPortfolio() {
        if (holdings_.empty()) {
            std::ostringstream msg;
            msg << std::fixed << std::setprecision(2);
            msg << "Portfolio empty. Cash balance: $" << cashBalance_;
            return msg.str();
        }

        std::ostringstream msg;
        msg << std::fixed << std::setprecision(2);
        msg << "Paper Trading Portfolio:\n";
        msg << "Cash: $" << cashBalance_ << "\n";
        msg << "---\n";

        double totalValue = cashBalance_;
        for (const auto& kv : holdings_) {
            const std::string& sym = kv.first;
            double qty = kv.second;
            double avg = avgCosts_.count(sym) ? avgCosts_[sym] : 0.0;

            // Try to fetch current price
            double currentPrice = fetchPrice(sym);
            double holdingValue = 0.0;
            double pnl = 0.0;

            if (currentPrice > 0.0) {
                holdingValue = qty * currentPrice;
                pnl = holdingValue - (qty * avg);
                totalValue += holdingValue;
                msg << std::setprecision(6) << sym << ": " << qty << " units";
                msg << std::setprecision(8) << " @ avg $" << avg;
                msg << std::setprecision(2) << " | Value: $" << holdingValue;
                msg << " | P/L: " << (pnl >= 0 ? "+" : "") << "$" << pnl << "\n";
            } else {
                holdingValue = qty * avg;
                totalValue += holdingValue;
                msg << std::setprecision(6) << sym << ": " << qty << " units";
                msg << " @ avg $" << std::setprecision(8) << avg;
                msg << " (price fetch failed)\n";
            }
        }

        msg << "---\n";
        msg << std::setprecision(2) << "Total portfolio value: $" << totalValue;
        msg << " | Starting: $" << STARTING_BALANCE;
        msg << " | Overall P/L: " << (totalValue - STARTING_BALANCE >= 0 ? "+" : "") 
            << "$" << (totalValue - STARTING_BALANCE);

        return msg.str();
    }

    // ============================================================
    // GET TRADE HISTORY: Last N trades
    // ============================================================
    std::string getTradeHistory(int n = 10) {
        if (tradeHistory_.empty()) {
            return "No trades yet.";
        }

        int start = (int)tradeHistory_.size() - n;
        if (start < 0) start = 0;

        std::ostringstream msg;
        msg << std::fixed;
        msg << "Last " << std::min(n, (int)tradeHistory_.size()) << " trades:\n";

        for (int i = start; i < (int)tradeHistory_.size(); i++) {
            const auto& t = tradeHistory_[i];
            msg << std::setprecision(6);
            msg << "#" << t.id << " " << t.side << " " << t.quantity << " " << t.symbol;
            msg << std::setprecision(8) << " @ $" << t.price;
            msg << std::setprecision(2) << " | gas=$" << t.gasFee << " dex=$" << t.dexFee;
            msg << " | slippage=" << std::setprecision(1) << (t.slippagePct * 100.0) << "%";
            msg << " | total=$" << std::setprecision(2) << t.totalCost << "\n";
        }

        return msg.str();
    }

    // ============================================================
    // GET STATS: Trading performance
    // ============================================================
    std::string getStats() {
        std::ostringstream msg;
        msg << std::fixed << std::setprecision(2);

        if (tradeHistory_.empty()) {
            msg << "No trades yet. Balance: $" << cashBalance_;
            return msg.str();
        }

        // Calculate stats from trade history
        double totalSpent = 0.0;
        double totalReceived = 0.0;
        int buys = 0, sells = 0;
        double bestTrade = -999999.0;
        double worstTrade = 999999.0;
        int winCount = 0;

        for (const auto& t : tradeHistory_) {
            if (t.side == "buy") {
                totalSpent += t.totalCost;
                buys++;
            } else {
                totalReceived += t.totalCost;
                sells++;
                // For sell trades, P/L = received - (qty * avg_cost_at_time)
                // Approximate: compare effective sell price to avg cost
                double avgCostForSym = avgCosts_.count(t.symbol) ? avgCosts_[t.symbol] : t.price;
                double tradePnl = (t.price - avgCostForSym) * t.quantity - t.gasFee - t.dexFee;
                if (tradePnl > bestTrade) bestTrade = tradePnl;
                if (tradePnl < worstTrade) worstTrade = tradePnl;
                if (tradePnl > 0) winCount++;
            }
        }

        // Total portfolio value approximation
        double holdingsValue = 0.0;
        for (const auto& kv : holdings_) {
            double avg = avgCosts_.count(kv.first) ? avgCosts_[kv.first] : 0.0;
            holdingsValue += kv.second * avg;
        }

        double totalPnl = (cashBalance_ + holdingsValue) - STARTING_BALANCE;
        double winRate = (sells > 0) ? ((double)winCount / (double)sells) * 100.0 : 0.0;

        msg << "Paper Trading Stats:\n";
        msg << "Balance: $" << cashBalance_ << "\n";
        msg << "Total trades: " << (int)tradeHistory_.size() << " (" << buys << " buys, " << sells << " sells)\n";
        msg << "Total spent: $" << totalSpent << " | Total received: $" << totalReceived << "\n";
        msg << "Total P/L: " << (totalPnl >= 0 ? "+" : "") << "$" << totalPnl << "\n";
        if (sells > 0) {
            msg << "Win rate: " << std::setprecision(1) << winRate << "%\n";
            if (bestTrade > -999999.0) msg << "Best trade: " << (bestTrade >= 0 ? "+" : "") << "$" << std::setprecision(2) << bestTrade << "\n";
            if (worstTrade < 999999.0) msg << "Worst trade: " << (worstTrade >= 0 ? "+" : "") << "$" << std::setprecision(2) << worstTrade << "\n";
        }

        return msg.str();
    }

    // ============================================================
    // GET HOLDINGS MAP (for testing)
    // ============================================================
    const std::map<std::string, double>& getHoldings() const { return holdings_; }

private:
    std::string formatMoney(double amount) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << amount;
        return ss.str();
    }
};

#endif // MK_PAPER_TRADING_CPP
