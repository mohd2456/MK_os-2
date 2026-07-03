// ============================================================
// MK OS - Crypto Risk Manager
// Enforces position limits, stop-losses, drawdown protection
// Never risks more than safe thresholds allow
// ============================================================
#ifndef MK_RISK_MANAGER_CPP
#define MK_RISK_MANAGER_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <numeric>

// ============================================================
// Data Structures
// ============================================================

struct MKRiskCheck {
    bool approved;
    std::string reason;
    double adjustedSize;    // Risk-adjusted position size
    double riskScore;       // 0-100 (higher = riskier)
};

struct MKStopLoss {
    std::string symbol;
    double triggerPrice;
    double entryPrice;
    double percentBelow;    // How far below entry
    bool active;
};

struct MKDrawdownState {
    double peakValue;
    double currentValue;
    double drawdownPercent;
    bool tradingPaused;
    long pausedSince;
    std::string reason;
};

struct MKDailyLossTracker {
    double startOfDayValue;
    double currentValue;
    double dailyLossPercent;
    long dayStart;
    bool limitReached;
};

struct MKCorrelation {
    std::string symbolA;
    std::string symbolB;
    double correlation;     // -1 to +1
};

// ============================================================
// MKRiskManager Class
// ============================================================

class MKRiskManager {
private:
    // Risk parameters (configurable)
    double maxPositionPercent_;      // Max allocation to single coin (default 20%)
    double maxDailyLossPercent_;     // Max daily loss before pausing (default 5%)
    double maxDrawdownPercent_;      // Max drawdown before pausing (default 10%)
    double defaultStopLossPercent_;  // Default stop-loss (default 8%)
    double minPositionUSD_;          // Minimum position size
    double maxPositionUSD_;          // Maximum position size

    // State
    MKDrawdownState drawdownState_;
    MKDailyLossTracker dailyLoss_;
    std::map<std::string, MKStopLoss> stopLosses_;
    std::vector<MKCorrelation> correlations_;
    std::map<std::string, double> volatilities_;     // Symbol -> ATR-based volatility

    // Track recent losses for circuit breaker
    int consecutiveLosses_;
    static constexpr int CIRCUIT_BREAKER_LOSSES = 5;

    // Check if it's a new day
    bool isNewDay() {
        long now = std::time(nullptr);
        // Simple check: if more than 24h since day start
        return (now - dailyLoss_.dayStart) > 86400;
    }

    // Reset daily tracking
    void resetDailyTracking(double currentValue) {
        dailyLoss_.startOfDayValue = currentValue;
        dailyLoss_.currentValue = currentValue;
        dailyLoss_.dailyLossPercent = 0.0;
        dailyLoss_.dayStart = std::time(nullptr);
        dailyLoss_.limitReached = false;
    }

public:
    MKRiskManager()
        : maxPositionPercent_(20.0),
          maxDailyLossPercent_(5.0),
          maxDrawdownPercent_(10.0),
          defaultStopLossPercent_(8.0),
          minPositionUSD_(10.0),
          maxPositionUSD_(50000.0),
          consecutiveLosses_(0) {
        drawdownState_.peakValue = 0.0;
        drawdownState_.currentValue = 0.0;
        drawdownState_.drawdownPercent = 0.0;
        drawdownState_.tradingPaused = false;
        drawdownState_.pausedSince = 0;

        dailyLoss_.startOfDayValue = 0.0;
        dailyLoss_.currentValue = 0.0;
        dailyLoss_.dailyLossPercent = 0.0;
        dailyLoss_.dayStart = std::time(nullptr);
        dailyLoss_.limitReached = false;
    }

    // ========================================================
    // Check if an order passes risk checks
    // ========================================================
    MKRiskCheck checkOrder(const std::string& symbol, double orderSizeUSD,
                           double currentPortfolioValue, double currentAllocationPercent,
                           double currentVolatility = 0.0) {
        MKRiskCheck result;
        result.approved = true;
        result.adjustedSize = orderSizeUSD;
        result.riskScore = 0.0;

        // Check 1: Trading paused due to drawdown
        if (drawdownState_.tradingPaused) {
            result.approved = false;
            result.reason = "Trading paused: portfolio drawdown exceeded " +
                            std::to_string(maxDrawdownPercent_).substr(0, 4) + "% threshold";
            result.riskScore = 100.0;
            return result;
        }

        // Check 2: Daily loss limit
        if (dailyLoss_.limitReached) {
            result.approved = false;
            result.reason = "Daily loss limit reached: " +
                            std::to_string(maxDailyLossPercent_).substr(0, 4) + "% max daily loss";
            result.riskScore = 95.0;
            return result;
        }

        // Check 3: Circuit breaker (consecutive losses)
        if (consecutiveLosses_ >= CIRCUIT_BREAKER_LOSSES) {
            result.approved = false;
            result.reason = "Circuit breaker: " + std::to_string(consecutiveLosses_) +
                            " consecutive losses - cool down required";
            result.riskScore = 90.0;
            return result;
        }

        // Check 4: Position size limit (never >20% in one coin)
        double newAllocation = currentAllocationPercent +
            (orderSizeUSD / std::max(1.0, currentPortfolioValue)) * 100.0;

        if (newAllocation > maxPositionPercent_) {
            double maxBuy = (maxPositionPercent_ - currentAllocationPercent) / 100.0 * currentPortfolioValue;
            if (maxBuy <= 0) {
                result.approved = false;
                result.reason = "Position limit: " + symbol + " already at " +
                                std::to_string(currentAllocationPercent).substr(0, 5) +
                                "% (max " + std::to_string(maxPositionPercent_).substr(0, 4) + "%)";
                result.riskScore = 80.0;
                return result;
            }
            // Reduce order to fit within limits
            result.adjustedSize = maxBuy;
            result.reason = "Order reduced from $" + std::to_string(orderSizeUSD).substr(0, 8) +
                            " to $" + std::to_string(maxBuy).substr(0, 8) +
                            " (position limit)";
            result.riskScore = 60.0;
        }

        // Check 5: Minimum and maximum position size
        if (result.adjustedSize < minPositionUSD_) {
            result.approved = false;
            result.reason = "Order too small: $" + std::to_string(result.adjustedSize).substr(0, 8) +
                            " (minimum $" + std::to_string(minPositionUSD_).substr(0, 6) + ")";
            result.riskScore = 20.0;
            return result;
        }

        if (result.adjustedSize > maxPositionUSD_) {
            result.adjustedSize = maxPositionUSD_;
            result.reason = "Order capped at maximum: $" +
                            std::to_string(maxPositionUSD_).substr(0, 8);
            result.riskScore = 50.0;
        }

        // Check 6: Volatility-adjusted sizing
        if (currentVolatility > 0.0) {
            // Higher volatility = smaller position
            double volatilityMultiplier = 1.0;
            if (currentVolatility > 5.0) volatilityMultiplier = 0.5;       // Very volatile
            else if (currentVolatility > 3.0) volatilityMultiplier = 0.7;  // High
            else if (currentVolatility > 2.0) volatilityMultiplier = 0.85; // Moderate

            double adjustedForVol = result.adjustedSize * volatilityMultiplier;
            if (adjustedForVol < result.adjustedSize) {
                result.adjustedSize = adjustedForVol;
                if (result.reason.empty()) {
                    result.reason = "Volatility-adjusted: reduced by " +
                                    std::to_string((1.0 - volatilityMultiplier) * 100).substr(0, 4) + "%";
                }
                result.riskScore += 15.0;
            }
        }

        // Check 7: Correlation-based diversification
        double correlationRisk = getCorrelationRisk(symbol);
        if (correlationRisk > 0.7) {
            result.adjustedSize *= 0.7;
            if (result.reason.empty()) {
                result.reason = "High correlation with existing positions - size reduced";
            }
            result.riskScore += 20.0;
        }

        // Final risk score clamping
        result.riskScore = std::min(100.0, result.riskScore);

        if (result.reason.empty()) {
            result.reason = "Approved - risk score: " + std::to_string(result.riskScore).substr(0, 5);
        }

        return result;
    }

    // ========================================================
    // Update portfolio value and check drawdown
    // ========================================================
    void updatePortfolioValue(double currentValue) {
        // Update peak
        if (currentValue > drawdownState_.peakValue) {
            drawdownState_.peakValue = currentValue;
        }

        drawdownState_.currentValue = currentValue;

        // Calculate drawdown
        if (drawdownState_.peakValue > 0) {
            drawdownState_.drawdownPercent =
                ((drawdownState_.peakValue - currentValue) / drawdownState_.peakValue) * 100.0;
        }

        // Check drawdown threshold
        if (drawdownState_.drawdownPercent >= maxDrawdownPercent_ && !drawdownState_.tradingPaused) {
            drawdownState_.tradingPaused = true;
            drawdownState_.pausedSince = std::time(nullptr);
            drawdownState_.reason = "Portfolio dropped " +
                std::to_string(drawdownState_.drawdownPercent).substr(0, 5) +
                "% from peak - trading paused";
        }

        // Daily loss tracking
        if (isNewDay()) {
            resetDailyTracking(currentValue);
        } else {
            dailyLoss_.currentValue = currentValue;
            if (dailyLoss_.startOfDayValue > 0) {
                dailyLoss_.dailyLossPercent =
                    ((dailyLoss_.startOfDayValue - currentValue) / dailyLoss_.startOfDayValue) * 100.0;
                if (dailyLoss_.dailyLossPercent >= maxDailyLossPercent_) {
                    dailyLoss_.limitReached = true;
                }
            }
        }
    }

    // ========================================================
    // Stop-loss management
    // ========================================================
    void setStopLoss(const std::string& symbol, double entryPrice, double stopPercent = 0.0) {
        double pct = (stopPercent > 0.0) ? stopPercent : defaultStopLossPercent_;
        MKStopLoss sl;
        sl.symbol = symbol;
        sl.entryPrice = entryPrice;
        sl.triggerPrice = entryPrice * (1.0 - pct / 100.0);
        sl.percentBelow = pct;
        sl.active = true;
        stopLosses_[symbol] = sl;
    }

    bool isStopLossTriggered(const std::string& symbol, double currentPrice) {
        auto it = stopLosses_.find(symbol);
        if (it == stopLosses_.end() || !it->second.active) return false;
        return currentPrice <= it->second.triggerPrice;
    }

    void removeStopLoss(const std::string& symbol) {
        stopLosses_.erase(symbol);
    }

    // ========================================================
    // Trailing stop-loss (updates stop as price moves up)
    // ========================================================
    void updateTrailingStop(const std::string& symbol, double currentPrice) {
        auto it = stopLosses_.find(symbol);
        if (it == stopLosses_.end() || !it->second.active) return;

        // Only trail upward
        double newStop = currentPrice * (1.0 - it->second.percentBelow / 100.0);
        if (newStop > it->second.triggerPrice) {
            it->second.triggerPrice = newStop;
        }
    }

    // ========================================================
    // Correlation tracking
    // ========================================================
    void setCorrelation(const std::string& symbolA, const std::string& symbolB, double corr) {
        MKCorrelation c;
        c.symbolA = symbolA;
        c.symbolB = symbolB;
        c.correlation = corr;
        correlations_.push_back(c);
    }

    double getCorrelationRisk(const std::string& symbol) {
        double maxCorr = 0.0;
        for (const auto& c : correlations_) {
            if (c.symbolA == symbol || c.symbolB == symbol) {
                maxCorr = std::max(maxCorr, std::abs(c.correlation));
            }
        }
        return maxCorr;
    }

    // ========================================================
    // Volatility tracking
    // ========================================================
    void updateVolatility(const std::string& symbol, double atrPercent) {
        volatilities_[symbol] = atrPercent;
    }

    double getVolatility(const std::string& symbol) const {
        auto it = volatilities_.find(symbol);
        return (it != volatilities_.end()) ? it->second : 0.0;
    }

    // ========================================================
    // Report trade outcomes (for circuit breaker)
    // ========================================================
    void reportWin() {
        consecutiveLosses_ = 0;
    }

    void reportLoss() {
        consecutiveLosses_++;
    }

    // ========================================================
    // Resume trading (manually reset pauses)
    // ========================================================
    void resumeTrading() {
        drawdownState_.tradingPaused = false;
        drawdownState_.peakValue = drawdownState_.currentValue; // Reset peak
        dailyLoss_.limitReached = false;
        consecutiveLosses_ = 0;
    }

    // ========================================================
    // Configuration
    // ========================================================
    void setMaxPositionPercent(double pct) { maxPositionPercent_ = pct; }
    void setMaxDailyLossPercent(double pct) { maxDailyLossPercent_ = pct; }
    void setMaxDrawdownPercent(double pct) { maxDrawdownPercent_ = pct; }
    void setDefaultStopLossPercent(double pct) { defaultStopLossPercent_ = pct; }
    void setMinPositionUSD(double amt) { minPositionUSD_ = amt; }
    void setMaxPositionUSD(double amt) { maxPositionUSD_ = amt; }

    // ========================================================
    // Getters
    // ========================================================
    MKDrawdownState getDrawdownState() const { return drawdownState_; }
    MKDailyLossTracker getDailyLossState() const { return dailyLoss_; }
    bool isTradingPaused() const { return drawdownState_.tradingPaused || dailyLoss_.limitReached; }
    int getConsecutiveLosses() const { return consecutiveLosses_; }
    double getMaxPositionPercent() const { return maxPositionPercent_; }

    std::string getRiskSummary() const {
        std::string summary;
        summary += "Risk Manager Status:\n";
        summary += "  Max position: " + std::to_string(maxPositionPercent_).substr(0, 5) + "%\n";
        summary += "  Drawdown: " + std::to_string(drawdownState_.drawdownPercent).substr(0, 5) + "%";
        summary += " (max " + std::to_string(maxDrawdownPercent_).substr(0, 5) + "%)\n";
        summary += "  Daily loss: " + std::to_string(dailyLoss_.dailyLossPercent).substr(0, 5) + "%";
        summary += " (max " + std::to_string(maxDailyLossPercent_).substr(0, 5) + "%)\n";
        summary += "  Consecutive losses: " + std::to_string(consecutiveLosses_) + "\n";
        summary += "  Trading: " + std::string(isTradingPaused() ? "PAUSED" : "ACTIVE") + "\n";
        return summary;
    }
};

#endif // MK_RISK_MANAGER_CPP
