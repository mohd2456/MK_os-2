// ============================================================
// MK OS - Crypto Trading Bot
// Orchestrates the full trading loop:
// Scan -> Analyze -> Signal -> Risk Check -> Execute -> Monitor
// Paper trading and live trading modes
// ============================================================
#ifndef MK_TRADING_BOT_CPP
#define MK_TRADING_BOT_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <chrono>
#include <thread>
#include <atomic>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iomanip>

// ============================================================
// Data Structures
// ============================================================

enum class MKBotMode {
    PAPER,      // Simulated trading - no real money
    LIVE,       // Real trading via exchange API
    ANALYSIS    // Only generates signals, no execution
};

struct MKTradeEntry {
    std::string symbol;
    std::string side;       // "buy" or "sell"
    double quantity;
    double price;
    double totalUSD;
    std::string reason;
    long timestamp;
    bool isPaper;
};

struct MKBotStats {
    int totalTrades;
    int winningTrades;
    int losingTrades;
    double winRate;
    double totalPnl;
    double totalPnlPercent;
    double bestTrade;
    double worstTrade;
    double avgTradeReturn;
    long uptimeSeconds;
    int signalsGenerated;
    int signalsExecuted;
    int signalsRejected;
};

struct MKBotConfig {
    MKBotMode mode;
    std::vector<std::string> watchlist;
    double paperBalance;
    int scanIntervalSeconds;
    bool autoTrade;
    double minSignalStrength;
    double minConfidence;
};

// ============================================================
// MKTradingBot Class
// ============================================================

class MKTradingBot {
private:
    MKBotConfig config_;
    MKBotStats stats_;
    std::vector<MKTradeEntry> tradeJournal_;
    std::map<std::string, double> paperBalances_;   // Symbol -> amount held
    double paperCashUSD_;
    std::atomic<bool> running_;
    long startTime_;
    std::string journalPath_;

    // Dependencies (references to other crypto modules)
    MKMarketData& marketData_;
    MKTechnicalAnalysis& technicalAnalysis_;
    MKSignalEngine& signalEngine_;
    MKPortfolioManager& portfolioManager_;
    MKRiskManager& riskManager_;
    MKExchangeAPI& exchangeApi_;

    // Record a trade in the journal
    void recordTrade(const MKTradeEntry& trade) {
        tradeJournal_.push_back(trade);
        stats_.totalTrades++;

        // Save to journal file
        std::ofstream journal(journalPath_, std::ios::app);
        if (journal.is_open()) {
            journal << trade.timestamp << "|"
                    << trade.symbol << "|"
                    << trade.side << "|"
                    << std::fixed << std::setprecision(8) << trade.quantity << "|"
                    << std::fixed << std::setprecision(2) << trade.price << "|"
                    << std::fixed << std::setprecision(2) << trade.totalUSD << "|"
                    << trade.reason << "|"
                    << (trade.isPaper ? "PAPER" : "LIVE") << "\n";
            journal.close();
        }
    }

    // Execute a paper trade
    bool executePaperTrade(const std::string& symbol, const std::string& side,
                           double quantity, double price, const std::string& reason) {
        MKTradeEntry trade;
        trade.symbol = symbol;
        trade.side = side;
        trade.quantity = quantity;
        trade.price = price;
        trade.totalUSD = quantity * price;
        trade.reason = reason;
        trade.timestamp = std::time(nullptr);
        trade.isPaper = true;

        if (side == "buy") {
            if (paperCashUSD_ < trade.totalUSD) {
                // Adjust to available cash
                trade.totalUSD = paperCashUSD_;
                trade.quantity = trade.totalUSD / price;
                if (trade.quantity <= 0) return false;
            }
            paperCashUSD_ -= trade.totalUSD;
            paperBalances_[symbol] += trade.quantity;
            portfolioManager_.recordBuy(symbol, trade.quantity, price);
        } else {
            double available = paperBalances_[symbol];
            if (available <= 0) return false;
            trade.quantity = std::min(quantity, available);
            trade.totalUSD = trade.quantity * price;
            paperBalances_[symbol] -= trade.quantity;
            paperCashUSD_ += trade.totalUSD;
            portfolioManager_.recordSell(symbol, trade.quantity, price);
        }

        recordTrade(trade);
        stats_.signalsExecuted++;
        return true;
    }

    // Execute a live trade
    bool executeLiveTrade(const std::string& symbol, const std::string& side,
                          double quantity, double price, const std::string& reason) {
        if (!exchangeApi_.isInitialized()) return false;

        MKOrderSide orderSide = (side == "buy") ? MKOrderSide::BUY : MKOrderSide::SELL;
        auto order = exchangeApi_.placeOrder(orderSide, symbol, quantity, MKOrderType::MARKET);

        if (!order.valid || order.status == MKOrderStatus::REJECTED) {
            return false;
        }

        MKTradeEntry trade;
        trade.symbol = symbol;
        trade.side = side;
        trade.quantity = quantity;
        trade.price = price;
        trade.totalUSD = quantity * price;
        trade.reason = reason;
        trade.timestamp = std::time(nullptr);
        trade.isPaper = false;

        if (side == "buy") {
            portfolioManager_.recordBuy(symbol, quantity, price);
            riskManager_.setStopLoss(symbol, price);
        } else {
            portfolioManager_.recordSell(symbol, quantity, price);
        }

        recordTrade(trade);
        stats_.signalsExecuted++;
        return true;
    }

    // Analyze a single symbol
    void analyzeSymbol(const std::string& symbol) {
        // Fetch market data
        auto priceResult = marketData_.fetchPrice(symbol);
        if (!priceResult.valid) return;

        // Get OHLC for technical analysis
        auto ohlcData = marketData_.fetchOHLC(symbol, "1h");
        if (ohlcData.size() < 30) return; // Need enough data

        // Extract price and volume arrays
        std::vector<double> closes, volumes, highs, lows;
        for (const auto& candle : ohlcData) {
            closes.push_back(candle.close);
            volumes.push_back(candle.volume);
            highs.push_back(candle.high);
            lows.push_back(candle.low);
        }

        // Calculate indicators
        double latestRSI = technicalAnalysis_.getLatestRSI(closes, 14);
        auto macdResult = technicalAnalysis_.MACD(closes, 12, 26, 9);
        auto bbands = technicalAnalysis_.BollingerBands(closes, 20, 2.0);
        auto vwapValues = technicalAnalysis_.VWAP(closes, volumes);

        // Get MACD histogram values
        double macdHist = 0.0, prevMacdHist = 0.0;
        if (macdResult.valid && macdResult.histogram.size() >= 2) {
            macdHist = macdResult.histogram.back();
            prevMacdHist = macdResult.histogram[macdResult.histogram.size() - 2];
        }

        // Get Bollinger values
        double bbUpper = 0.0, bbLower = 0.0, bbMiddle = 0.0;
        if (bbands.valid && !bbands.upper.empty()) {
            bbUpper = bbands.upper.back();
            bbLower = bbands.lower.back();
            bbMiddle = bbands.middle.back();
        }

        // Get VWAP
        double vwap = vwapValues.empty() ? priceResult.price : vwapValues.back();

        // Generate signal
        auto signal = signalEngine_.generateSignal(
            symbol, closes, volumes, latestRSI,
            macdHist, prevMacdHist,
            bbUpper, bbLower, bbMiddle, vwap, "1h");

        stats_.signalsGenerated++;

        // Update volatility for risk manager
        auto atrValues = technicalAnalysis_.ATR(highs, lows, closes, 14);
        if (!atrValues.empty() && priceResult.price > 0) {
            double atrPercent = (atrValues.back() / priceResult.price) * 100.0;
            riskManager_.updateVolatility(symbol, atrPercent);
        }

        // Update portfolio price
        portfolioManager_.updatePrice(symbol, priceResult.price);

        // Check stop-losses
        if (riskManager_.isStopLossTriggered(symbol, priceResult.price)) {
            // Execute stop-loss sell
            if (portfolioManager_.hasPosition(symbol)) {
                auto holding = portfolioManager_.getHolding(symbol);
                std::string reason = "STOP-LOSS triggered at $" +
                    std::to_string(priceResult.price).substr(0, 10);

                if (config_.mode == MKBotMode::PAPER) {
                    executePaperTrade(symbol, "sell", holding.amount, priceResult.price, reason);
                } else if (config_.mode == MKBotMode::LIVE) {
                    executeLiveTrade(symbol, "sell", holding.amount, priceResult.price, reason);
                }
                riskManager_.reportLoss();
                riskManager_.removeStopLoss(symbol);
            }
            return; // Don't generate new signals after stop-loss
        }

        // Act on signal if auto-trading is enabled
        if (!config_.autoTrade || config_.mode == MKBotMode::ANALYSIS) return;

        // Filter by minimum signal strength and confidence
        if (signal.strength < (int)config_.minSignalStrength ||
            signal.confidence < config_.minConfidence) {
            return;
        }

        // Only act on buy/sell signals
        if (signal.type == MKSignalType::HOLD) return;

        // Determine order size
        double portfolioValue = portfolioManager_.getTotalValue() + paperCashUSD_;
        double currentAlloc = 0.0;
        if (portfolioManager_.hasPosition(symbol) && portfolioValue > 0) {
            auto holding = portfolioManager_.getHolding(symbol);
            currentAlloc = (holding.currentValue / portfolioValue) * 100.0;
        }

        // Handle buy signal
        if (signal.type == MKSignalType::BUY || signal.type == MKSignalType::STRONG_BUY) {
            double orderSize = portfolioValue * 0.05; // 5% of portfolio per trade
            if (signal.type == MKSignalType::STRONG_BUY) orderSize *= 1.5;

            // Risk check
            double volatility = riskManager_.getVolatility(symbol);
            auto riskCheck = riskManager_.checkOrder(
                symbol, orderSize, portfolioValue, currentAlloc, volatility);

            if (!riskCheck.approved) {
                stats_.signalsRejected++;
                return;
            }

            double quantity = riskCheck.adjustedSize / priceResult.price;
            std::string reason = signalEngine_.signalTypeToString(signal.type) +
                " (strength:" + std::to_string(signal.strength) +
                " conf:" + std::to_string(signal.confidence).substr(0, 4) + ")";

            if (config_.mode == MKBotMode::PAPER) {
                executePaperTrade(symbol, "buy", quantity, priceResult.price, reason);
            } else if (config_.mode == MKBotMode::LIVE) {
                executeLiveTrade(symbol, "buy", quantity, priceResult.price, reason);
            }
        }
        // Handle sell signal
        else if ((signal.type == MKSignalType::SELL || signal.type == MKSignalType::STRONG_SELL)
                 && portfolioManager_.hasPosition(symbol)) {
            auto holding = portfolioManager_.getHolding(symbol);
            double sellAmount = holding.amount;
            if (signal.type == MKSignalType::SELL) sellAmount *= 0.5; // Partial sell

            std::string reason = signalEngine_.signalTypeToString(signal.type) +
                " (strength:" + std::to_string(signal.strength) +
                " conf:" + std::to_string(signal.confidence).substr(0, 4) + ")";

            if (config_.mode == MKBotMode::PAPER) {
                executePaperTrade(symbol, "sell", sellAmount, priceResult.price, reason);
            } else if (config_.mode == MKBotMode::LIVE) {
                executeLiveTrade(symbol, "sell", sellAmount, priceResult.price, reason);
            }

            // Track win/loss
            if (holding.pnl > 0) riskManager_.reportWin();
            else riskManager_.reportLoss();
        }
    }

public:
    MKTradingBot(MKMarketData& md, MKTechnicalAnalysis& ta, MKSignalEngine& se,
                 MKPortfolioManager& pm, MKRiskManager& rm, MKExchangeAPI& ea)
        : running_(false),
          startTime_(0),
          journalPath_("crypto_trade_journal.log"),
          marketData_(md),
          technicalAnalysis_(ta),
          signalEngine_(se),
          portfolioManager_(pm),
          riskManager_(rm),
          exchangeApi_(ea) {
        // Default config
        config_.mode = MKBotMode::PAPER;
        config_.paperBalance = 10000.0;
        config_.scanIntervalSeconds = 300; // 5 minutes
        config_.autoTrade = true;
        config_.minSignalStrength = 50;
        config_.minConfidence = 0.5;
        config_.watchlist = {"BTC", "ETH", "SOL", "BNB", "ADA"};

        paperCashUSD_ = config_.paperBalance;

        // Initialize stats
        stats_.totalTrades = 0;
        stats_.winningTrades = 0;
        stats_.losingTrades = 0;
        stats_.winRate = 0.0;
        stats_.totalPnl = 0.0;
        stats_.totalPnlPercent = 0.0;
        stats_.bestTrade = 0.0;
        stats_.worstTrade = 0.0;
        stats_.avgTradeReturn = 0.0;
        stats_.uptimeSeconds = 0;
        stats_.signalsGenerated = 0;
        stats_.signalsExecuted = 0;
        stats_.signalsRejected = 0;
    }

    // ========================================================
    // Configuration
    // ========================================================
    void setMode(MKBotMode mode) { config_.mode = mode; }
    void setWatchlist(const std::vector<std::string>& symbols) { config_.watchlist = symbols; }
    void setScanInterval(int seconds) { config_.scanIntervalSeconds = seconds; }
    void setAutoTrade(bool enabled) { config_.autoTrade = enabled; }
    void setMinSignalStrength(double strength) { config_.minSignalStrength = strength; }
    void setMinConfidence(double conf) { config_.minConfidence = conf; }
    void setPaperBalance(double balance) {
        config_.paperBalance = balance;
        paperCashUSD_ = balance;
    }

    MKBotMode getMode() const { return config_.mode; }
    bool isRunning() const { return running_.load(); }

    // ========================================================
    // Run a single scan cycle (non-blocking)
    // ========================================================
    void runScanCycle() {
        // Update drawdown state
        double totalValue = portfolioManager_.getTotalValue() + paperCashUSD_;
        riskManager_.updatePortfolioValue(totalValue);

        // Check if trading is paused
        if (riskManager_.isTradingPaused()) return;

        // Scan each symbol in watchlist
        for (const auto& symbol : config_.watchlist) {
            analyzeSymbol(symbol);
        }

        // Check DCA schedules
        auto dcaDue = portfolioManager_.getDCASchedulesDue();
        for (const auto& dca : dcaDue) {
            if (!dca.active) continue;
            auto priceResult = marketData_.fetchPrice(dca.symbol);
            if (!priceResult.valid) continue;

            double quantity = dca.amountUSD / priceResult.price;
            std::string reason = "DCA schedule ($" + std::to_string(dca.amountUSD).substr(0, 6) +
                                 " " + dca.interval + ")";

            if (config_.mode == MKBotMode::PAPER) {
                executePaperTrade(dca.symbol, "buy", quantity, priceResult.price, reason);
            } else if (config_.mode == MKBotMode::LIVE) {
                executeLiveTrade(dca.symbol, "buy", quantity, priceResult.price, reason);
            }
        }

        // Periodic portfolio snapshot
        portfolioManager_.saveSnapshot();
    }

    // ========================================================
    // Start the bot (runs in a loop until stopped)
    // ========================================================
    void start() {
        running_ = true;
        startTime_ = std::time(nullptr);
    }

    void stop() {
        running_ = false;
    }

    // ========================================================
    // Performance report
    // ========================================================
    std::string getPerformanceReport() {
        std::ostringstream report;
        report << std::fixed << std::setprecision(2);

        // Calculate stats
        double totalValue = portfolioManager_.getTotalValue() + paperCashUSD_;
        double totalPnl = totalValue - config_.paperBalance;
        double pnlPercent = (config_.paperBalance > 0) ?
            (totalPnl / config_.paperBalance * 100.0) : 0.0;

        stats_.totalPnl = totalPnl;
        stats_.totalPnlPercent = pnlPercent;
        if (stats_.totalTrades > 0) {
            stats_.avgTradeReturn = totalPnl / stats_.totalTrades;
        }
        stats_.uptimeSeconds = std::time(nullptr) - startTime_;

        report << "=== MK Trading Bot Performance ===\n";
        report << "Mode: " << (config_.mode == MKBotMode::PAPER ? "PAPER" :
                               config_.mode == MKBotMode::LIVE ? "LIVE" : "ANALYSIS") << "\n";
        report << "Uptime: " << (stats_.uptimeSeconds / 3600) << "h "
               << ((stats_.uptimeSeconds % 3600) / 60) << "m\n";
        report << "\n";
        report << "Portfolio:\n";
        report << "  Starting:  $" << config_.paperBalance << "\n";
        report << "  Current:   $" << totalValue << "\n";
        report << "  PnL:       $" << totalPnl << " (" << pnlPercent << "%)\n";
        report << "  Cash:      $" << paperCashUSD_ << "\n";
        report << "\n";
        report << "Trading:\n";
        report << "  Total trades:    " << stats_.totalTrades << "\n";
        report << "  Signals gen:     " << stats_.signalsGenerated << "\n";
        report << "  Signals exec:    " << stats_.signalsExecuted << "\n";
        report << "  Signals rejected:" << stats_.signalsRejected << "\n";
        report << "\n";
        report << "Risk:\n";
        report << "  " << riskManager_.getRiskSummary();

        return report.str();
    }

    // ========================================================
    // Trade journal access
    // ========================================================
    const std::vector<MKTradeEntry>& getTradeJournal() const {
        return tradeJournal_;
    }

    MKBotStats getStats() const { return stats_; }
    MKBotConfig getConfig() const { return config_; }

    double getPaperCash() const { return paperCashUSD_; }
    double getPaperPortfolioValue() const {
        double total = paperCashUSD_;
        for (const auto& pair : paperBalances_) {
            auto holding = portfolioManager_.getHolding(pair.first);
            total += holding.currentValue;
        }
        return total;
    }

    // ========================================================
    // Status string for display
    // ========================================================
    std::string getStatusString() const {
        std::string mode;
        switch (config_.mode) {
            case MKBotMode::PAPER: mode = "PAPER"; break;
            case MKBotMode::LIVE: mode = "LIVE"; break;
            case MKBotMode::ANALYSIS: mode = "ANALYSIS"; break;
        }
        return "CryptoBot[" + mode + "] trades:" + std::to_string(stats_.totalTrades) +
               " signals:" + std::to_string(stats_.signalsGenerated) +
               " watching:" + std::to_string(config_.watchlist.size()) + " coins";
    }
};

#endif // MK_TRADING_BOT_CPP
