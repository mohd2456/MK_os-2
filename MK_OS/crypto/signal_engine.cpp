// ============================================================
// MK OS - Crypto Signal Engine
// Combines technical indicators into buy/sell/hold signals
// Multi-timeframe analysis, signal strength scoring, history
// ============================================================
#ifndef MK_SIGNAL_ENGINE_CPP
#define MK_SIGNAL_ENGINE_CPP

#include <string>
#include <vector>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>

// ============================================================
// Data Structures
// ============================================================

enum class MKSignalType {
    STRONG_BUY,
    BUY,
    HOLD,
    SELL,
    STRONG_SELL
};

struct MKTradingSignal {
    MKSignalType type;
    std::string symbol;
    int strength;           // 0-100
    double confidence;      // 0.0-1.0
    std::string reasoning;
    std::string timeframe;
    long timestamp;
    std::vector<std::string> indicators;  // Which indicators triggered
};

struct MKBacktestResult {
    int totalTrades;
    int winningTrades;
    int losingTrades;
    double winRate;
    double profitFactor;
    double maxDrawdown;
    double totalReturn;
    double sharpeRatio;
    bool valid;
};

struct MKIndicatorScore {
    std::string name;
    double score;   // -1.0 (very bearish) to +1.0 (very bullish)
    double weight;
};

// ============================================================
// MKSignalEngine Class
// ============================================================

class MKSignalEngine {
private:
    std::vector<MKTradingSignal> signalHistory_;
    static constexpr size_t MAX_SIGNAL_HISTORY = 500;
    int consecutiveFalseSignals_;
    static constexpr int FALSE_SIGNAL_THRESHOLD = 3;

    // Score individual indicators
    MKIndicatorScore scoreRSI(double rsi) {
        MKIndicatorScore score;
        score.name = "RSI";
        score.weight = 0.2;

        if (rsi <= 20.0) score.score = 1.0;        // Extremely oversold - strong buy
        else if (rsi <= 30.0) score.score = 0.7;    // Oversold - buy
        else if (rsi <= 40.0) score.score = 0.3;    // Mildly oversold - lean buy
        else if (rsi <= 60.0) score.score = 0.0;    // Neutral
        else if (rsi <= 70.0) score.score = -0.3;   // Mildly overbought - lean sell
        else if (rsi <= 80.0) score.score = -0.7;   // Overbought - sell
        else score.score = -1.0;                    // Extremely overbought - strong sell

        return score;
    }

    MKIndicatorScore scoreMACDHistogram(double histogram, double prevHistogram) {
        MKIndicatorScore score;
        score.name = "MACD";
        score.weight = 0.25;

        if (histogram > 0 && prevHistogram <= 0) {
            score.score = 0.8;  // Bullish crossover
        } else if (histogram < 0 && prevHistogram >= 0) {
            score.score = -0.8; // Bearish crossover
        } else if (histogram > 0) {
            score.score = std::min(0.5, histogram * 10.0); // Positive momentum
        } else {
            score.score = std::max(-0.5, histogram * 10.0); // Negative momentum
        }

        return score;
    }

    MKIndicatorScore scoreBollingerPosition(double price, double upper, double lower, double middle) {
        MKIndicatorScore score;
        score.name = "Bollinger";
        score.weight = 0.15;

        double range = upper - lower;
        if (range <= 0) {
            score.score = 0.0;
            return score;
        }

        double position = (price - lower) / range; // 0 = at lower, 1 = at upper

        if (position <= 0.0) score.score = 0.9;       // Below lower band - oversold
        else if (position <= 0.2) score.score = 0.6;  // Near lower band
        else if (position <= 0.4) score.score = 0.2;
        else if (position <= 0.6) score.score = 0.0;  // Middle
        else if (position <= 0.8) score.score = -0.2;
        else if (position < 1.0) score.score = -0.6;  // Near upper band
        else score.score = -0.9;                       // Above upper band - overbought

        return score;
    }

    MKIndicatorScore scoreVWAP(double price, double vwap) {
        MKIndicatorScore score;
        score.name = "VWAP";
        score.weight = 0.1;

        if (vwap <= 0.0) {
            score.score = 0.0;
            return score;
        }

        double deviation = (price - vwap) / vwap;

        if (deviation < -0.03) score.score = 0.7;       // Well below VWAP - buy
        else if (deviation < -0.01) score.score = 0.3;  // Below VWAP
        else if (deviation < 0.01) score.score = 0.0;   // At VWAP
        else if (deviation < 0.03) score.score = -0.3;  // Above VWAP
        else score.score = -0.7;                        // Well above VWAP - sell

        return score;
    }

    MKIndicatorScore scoreTrend(const std::vector<double>& prices, int lookback = 20) {
        MKIndicatorScore score;
        score.name = "Trend";
        score.weight = 0.15;

        if ((int)prices.size() < lookback) {
            score.score = 0.0;
            return score;
        }

        // Simple linear regression slope
        int n = lookback;
        int startIdx = static_cast<int>(prices.size()) - n;
        double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;

        for (int i = 0; i < n; i++) {
            double x = static_cast<double>(i);
            double y = prices[startIdx + i];
            sumX += x;
            sumY += y;
            sumXY += x * y;
            sumX2 += x * x;
        }

        double denom = n * sumX2 - sumX * sumX;
        if (denom == 0.0) {
            score.score = 0.0;
            return score;
        }

        double slope = (n * sumXY - sumX * sumY) / denom;
        double avgPrice = sumY / n;

        // Normalize slope as percentage of average price
        double normalizedSlope = (slope / avgPrice) * 100.0;

        score.score = std::max(-1.0, std::min(1.0, normalizedSlope));
        return score;
    }

    MKIndicatorScore scoreVolume(double currentVolume, double avgVolume) {
        MKIndicatorScore score;
        score.name = "Volume";
        score.weight = 0.15;

        if (avgVolume <= 0.0) {
            score.score = 0.0;
            return score;
        }

        double volumeRatio = currentVolume / avgVolume;

        // High volume confirms trends, low volume suggests weakness
        if (volumeRatio > 2.0) score.score = 0.4;      // Very high volume - trend confirmation
        else if (volumeRatio > 1.5) score.score = 0.2;
        else if (volumeRatio > 0.8) score.score = 0.0;  // Normal
        else if (volumeRatio > 0.5) score.score = -0.1; // Low volume
        else score.score = -0.3;                         // Very low volume - weak

        return score;
    }

    // Filter out false signals based on recent history
    bool isFalseSignal(const MKTradingSignal& signal) {
        if (signalHistory_.size() < 3) return false;

        // Check if we have been flip-flopping signals
        int recentBuys = 0, recentSells = 0;
        int checkCount = std::min((int)signalHistory_.size(), 5);
        for (int i = 0; i < checkCount; i++) {
            const auto& hist = signalHistory_[signalHistory_.size() - 1 - i];
            if (hist.symbol != signal.symbol) continue;
            if (hist.type == MKSignalType::BUY || hist.type == MKSignalType::STRONG_BUY) recentBuys++;
            if (hist.type == MKSignalType::SELL || hist.type == MKSignalType::STRONG_SELL) recentSells++;
        }

        // If signals are oscillating rapidly, filter out weak ones
        if (recentBuys > 0 && recentSells > 0 && signal.strength < 60) {
            return true;
        }

        // Filter weak signals after consecutive false signals
        if (consecutiveFalseSignals_ >= FALSE_SIGNAL_THRESHOLD && signal.strength < 70) {
            return true;
        }

        return false;
    }

public:
    MKSignalEngine() : consecutiveFalseSignals_(0) {}

    // ========================================================
    // Generate trading signal from multiple indicators
    // ========================================================
    MKTradingSignal generateSignal(const std::string& symbol,
                                    const std::vector<double>& prices,
                                    const std::vector<double>& volumes,
                                    double currentRSI,
                                    double macdHistogram,
                                    double prevMacdHistogram,
                                    double bbUpper,
                                    double bbLower,
                                    double bbMiddle,
                                    double vwapValue,
                                    const std::string& timeframe = "1h") {
        MKTradingSignal signal;
        signal.symbol = symbol;
        signal.timeframe = timeframe;
        signal.timestamp = std::time(nullptr);

        if (prices.empty()) {
            signal.type = MKSignalType::HOLD;
            signal.strength = 0;
            signal.confidence = 0.0;
            signal.reasoning = "Insufficient data";
            return signal;
        }

        double currentPrice = prices.back();
        double avgVolume = volumes.empty() ? 0.0 :
            std::accumulate(volumes.begin(), volumes.end(), 0.0) / volumes.size();
        double currentVolume = volumes.empty() ? 0.0 : volumes.back();

        // Score each indicator
        std::vector<MKIndicatorScore> scores;
        scores.push_back(scoreRSI(currentRSI));
        scores.push_back(scoreMACDHistogram(macdHistogram, prevMacdHistogram));
        scores.push_back(scoreBollingerPosition(currentPrice, bbUpper, bbLower, bbMiddle));
        scores.push_back(scoreVWAP(currentPrice, vwapValue));
        scores.push_back(scoreTrend(prices));
        scores.push_back(scoreVolume(currentVolume, avgVolume));

        // Calculate weighted composite score
        double totalWeight = 0.0;
        double weightedScore = 0.0;
        for (const auto& s : scores) {
            weightedScore += s.score * s.weight;
            totalWeight += s.weight;
        }

        double compositeScore = (totalWeight > 0.0) ? weightedScore / totalWeight : 0.0;

        // Convert composite score to signal
        if (compositeScore >= 0.6) {
            signal.type = MKSignalType::STRONG_BUY;
            signal.strength = std::min(100, (int)(compositeScore * 100));
        } else if (compositeScore >= 0.25) {
            signal.type = MKSignalType::BUY;
            signal.strength = std::min(80, (int)(compositeScore * 100) + 20);
        } else if (compositeScore <= -0.6) {
            signal.type = MKSignalType::STRONG_SELL;
            signal.strength = std::min(100, (int)(std::abs(compositeScore) * 100));
        } else if (compositeScore <= -0.25) {
            signal.type = MKSignalType::SELL;
            signal.strength = std::min(80, (int)(std::abs(compositeScore) * 100) + 20);
        } else {
            signal.type = MKSignalType::HOLD;
            signal.strength = (int)((1.0 - std::abs(compositeScore)) * 50);
        }

        // Calculate confidence based on indicator agreement
        int agreeing = 0;
        for (const auto& s : scores) {
            if ((compositeScore > 0 && s.score > 0) || (compositeScore < 0 && s.score < 0)) {
                agreeing++;
            }
        }
        signal.confidence = static_cast<double>(agreeing) / scores.size();

        // Build reasoning string
        signal.reasoning = "Composite score: " + std::to_string(compositeScore).substr(0, 5);
        for (const auto& s : scores) {
            if (std::abs(s.score) >= 0.5) {
                signal.indicators.push_back(s.name + "=" + std::to_string(s.score).substr(0, 5));
            }
        }

        // Apply false signal filter
        if (isFalseSignal(signal)) {
            signal.type = MKSignalType::HOLD;
            signal.strength = 0;
            signal.reasoning += " [FILTERED - potential false signal]";
        }

        // Store in history
        signalHistory_.push_back(signal);
        if (signalHistory_.size() > MAX_SIGNAL_HISTORY) {
            signalHistory_.erase(signalHistory_.begin());
        }

        return signal;
    }

    // ========================================================
    // Multi-timeframe analysis
    // ========================================================
    MKTradingSignal multiTimeframeSignal(const std::string& symbol,
                                          const MKTradingSignal& shortTerm,
                                          const MKTradingSignal& mediumTerm,
                                          const MKTradingSignal& longTerm) {
        MKTradingSignal result;
        result.symbol = symbol;
        result.timeframe = "multi";
        result.timestamp = std::time(nullptr);

        // Convert signals to scores
        auto signalToScore = [](const MKTradingSignal& s) -> double {
            switch (s.type) {
                case MKSignalType::STRONG_BUY: return 1.0;
                case MKSignalType::BUY: return 0.5;
                case MKSignalType::HOLD: return 0.0;
                case MKSignalType::SELL: return -0.5;
                case MKSignalType::STRONG_SELL: return -1.0;
            }
            return 0.0;
        };

        // Weight: long-term trend > medium > short
        double score = signalToScore(shortTerm) * 0.2 +
                       signalToScore(mediumTerm) * 0.3 +
                       signalToScore(longTerm) * 0.5;

        if (score >= 0.5) result.type = MKSignalType::STRONG_BUY;
        else if (score >= 0.2) result.type = MKSignalType::BUY;
        else if (score <= -0.5) result.type = MKSignalType::STRONG_SELL;
        else if (score <= -0.2) result.type = MKSignalType::SELL;
        else result.type = MKSignalType::HOLD;

        result.strength = std::min(100, (int)(std::abs(score) * 100));

        // Confidence is higher when all timeframes agree
        bool allAgree = (signalToScore(shortTerm) > 0 && signalToScore(mediumTerm) > 0 &&
                         signalToScore(longTerm) > 0) ||
                        (signalToScore(shortTerm) < 0 && signalToScore(mediumTerm) < 0 &&
                         signalToScore(longTerm) < 0);
        result.confidence = allAgree ? 0.9 : 0.5;

        result.reasoning = "Multi-TF: short=" + std::to_string(signalToScore(shortTerm)).substr(0, 4) +
                           " med=" + std::to_string(signalToScore(mediumTerm)).substr(0, 4) +
                           " long=" + std::to_string(signalToScore(longTerm)).substr(0, 4);

        return result;
    }

    // ========================================================
    // Simple backtesting on historical data
    // ========================================================
    MKBacktestResult backtest(const std::vector<double>& prices,
                              const std::vector<MKTradingSignal>& signals,
                              double startingCapital = 10000.0) {
        MKBacktestResult result;
        result.valid = false;
        result.totalTrades = 0;
        result.winningTrades = 0;
        result.losingTrades = 0;

        if (prices.empty() || signals.empty()) return result;

        double capital = startingCapital;
        double peakCapital = startingCapital;
        double maxDrawdown = 0.0;
        double position = 0.0; // Number of units held
        double entryPrice = 0.0;

        for (const auto& signal : signals) {
            // Find the price at signal timestamp (simplified - use signal index)
            size_t priceIdx = std::min(prices.size() - 1,
                                        static_cast<size_t>(result.totalTrades));
            double price = prices[priceIdx];

            if ((signal.type == MKSignalType::BUY || signal.type == MKSignalType::STRONG_BUY)
                && position == 0.0) {
                // Enter long position
                position = capital / price;
                entryPrice = price;
                result.totalTrades++;
            } else if ((signal.type == MKSignalType::SELL || signal.type == MKSignalType::STRONG_SELL)
                       && position > 0.0) {
                // Exit position
                double exitValue = position * price;
                if (exitValue > capital) result.winningTrades++;
                else result.losingTrades++;
                capital = exitValue;
                position = 0.0;

                // Track drawdown
                if (capital > peakCapital) peakCapital = capital;
                double drawdown = (peakCapital - capital) / peakCapital;
                if (drawdown > maxDrawdown) maxDrawdown = drawdown;
            }
        }

        // Close any open position at last price
        if (position > 0.0 && !prices.empty()) {
            capital = position * prices.back();
        }

        result.totalReturn = (capital - startingCapital) / startingCapital * 100.0;
        result.maxDrawdown = maxDrawdown * 100.0;
        result.winRate = result.totalTrades > 0 ?
            static_cast<double>(result.winningTrades) / result.totalTrades * 100.0 : 0.0;
        result.profitFactor = result.losingTrades > 0 ?
            static_cast<double>(result.winningTrades) / result.losingTrades : 0.0;
        result.sharpeRatio = 0.0; // Simplified - would need daily returns
        result.valid = result.totalTrades > 0;

        return result;
    }

    // ========================================================
    // Report false signal (for adaptive filtering)
    // ========================================================
    void reportFalseSignal() {
        consecutiveFalseSignals_++;
    }

    void reportGoodSignal() {
        consecutiveFalseSignals_ = std::max(0, consecutiveFalseSignals_ - 1);
    }

    // ========================================================
    // Getters
    // ========================================================
    const std::vector<MKTradingSignal>& getSignalHistory() const {
        return signalHistory_;
    }

    int signalCount() const {
        return static_cast<int>(signalHistory_.size());
    }

    MKTradingSignal getLastSignal(const std::string& symbol) const {
        for (auto it = signalHistory_.rbegin(); it != signalHistory_.rend(); ++it) {
            if (it->symbol == symbol) return *it;
        }
        MKTradingSignal empty;
        empty.type = MKSignalType::HOLD;
        empty.strength = 0;
        empty.confidence = 0.0;
        empty.symbol = symbol;
        return empty;
    }

    std::string signalTypeToString(MKSignalType type) const {
        switch (type) {
            case MKSignalType::STRONG_BUY: return "STRONG_BUY";
            case MKSignalType::BUY: return "BUY";
            case MKSignalType::HOLD: return "HOLD";
            case MKSignalType::SELL: return "SELL";
            case MKSignalType::STRONG_SELL: return "STRONG_SELL";
        }
        return "UNKNOWN";
    }
};

#endif // MK_SIGNAL_ENGINE_CPP
