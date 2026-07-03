// ============================================================
// MK OS - Crypto Technical Analysis Module
// Pure mathematical calculations on price arrays
// Implements: RSI, MACD, Bollinger Bands, EMA, SMA, VWAP,
// Support/Resistance, MA Crossover, Fibonacci Retracement
// ============================================================
#ifndef MK_TECHNICAL_ANALYSIS_CPP
#define MK_TECHNICAL_ANALYSIS_CPP

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>

// ============================================================
// Data Structures
// ============================================================

struct MKMACDResult {
    std::vector<double> macdLine;
    std::vector<double> signalLine;
    std::vector<double> histogram;
    bool valid;
};

struct MKBollingerBands {
    std::vector<double> upper;
    std::vector<double> middle;
    std::vector<double> lower;
    std::vector<double> bandwidth;
    bool valid;
};

struct MKFibonacciLevels {
    double high;
    double low;
    double level_236;    // 23.6%
    double level_382;    // 38.2%
    double level_500;    // 50.0%
    double level_618;    // 61.8%
    double level_786;    // 78.6%
    bool valid;
};

struct MKSupportResistance {
    std::vector<double> supports;
    std::vector<double> resistances;
    bool valid;
};

struct MKCrossoverSignal {
    enum Type { GOLDEN_CROSS, DEATH_CROSS, NONE };
    Type type;
    int index;       // Where the crossover occurred
    double strength; // 0-1
};

// ============================================================
// MKTechnicalAnalysis Class
// ============================================================

class MKTechnicalAnalysis {
public:
    MKTechnicalAnalysis() = default;

    // ========================================================
    // Simple Moving Average (SMA)
    // ========================================================
    std::vector<double> SMA(const std::vector<double>& prices, int period) {
        std::vector<double> result;
        if ((int)prices.size() < period || period <= 0) return result;

        result.reserve(prices.size() - period + 1);
        double sum = 0.0;
        for (int i = 0; i < period; i++) {
            sum += prices[i];
        }
        result.push_back(sum / period);

        for (size_t i = period; i < prices.size(); i++) {
            sum += prices[i] - prices[i - period];
            result.push_back(sum / period);
        }

        return result;
    }

    // ========================================================
    // Exponential Moving Average (EMA)
    // ========================================================
    std::vector<double> EMA(const std::vector<double>& prices, int period) {
        std::vector<double> result;
        if ((int)prices.size() < period || period <= 0) return result;

        double multiplier = 2.0 / (period + 1);

        // Start with SMA for first value
        double sum = 0.0;
        for (int i = 0; i < period; i++) {
            sum += prices[i];
        }
        double ema = sum / period;
        result.push_back(ema);

        for (size_t i = period; i < prices.size(); i++) {
            ema = (prices[i] - ema) * multiplier + ema;
            result.push_back(ema);
        }

        return result;
    }

    // ========================================================
    // Relative Strength Index (RSI) - 14 period default
    // ========================================================
    std::vector<double> RSI(const std::vector<double>& prices, int period = 14) {
        std::vector<double> result;
        if ((int)prices.size() < period + 1 || period <= 0) return result;

        // Calculate price changes
        std::vector<double> gains, losses;
        for (size_t i = 1; i < prices.size(); i++) {
            double change = prices[i] - prices[i - 1];
            gains.push_back(change > 0 ? change : 0.0);
            losses.push_back(change < 0 ? -change : 0.0);
        }

        if ((int)gains.size() < period) return result;

        // First average gain and loss
        double avgGain = 0.0, avgLoss = 0.0;
        for (int i = 0; i < period; i++) {
            avgGain += gains[i];
            avgLoss += losses[i];
        }
        avgGain /= period;
        avgLoss /= period;

        // Calculate RSI using smoothed method
        if (avgLoss == 0.0) {
            result.push_back(100.0);
        } else {
            double rs = avgGain / avgLoss;
            result.push_back(100.0 - (100.0 / (1.0 + rs)));
        }

        for (size_t i = period; i < gains.size(); i++) {
            avgGain = (avgGain * (period - 1) + gains[i]) / period;
            avgLoss = (avgLoss * (period - 1) + losses[i]) / period;

            if (avgLoss == 0.0) {
                result.push_back(100.0);
            } else {
                double rs = avgGain / avgLoss;
                result.push_back(100.0 - (100.0 / (1.0 + rs)));
            }
        }

        return result;
    }

    // ========================================================
    // MACD (12, 26, 9) - Moving Average Convergence Divergence
    // ========================================================
    MKMACDResult MACD(const std::vector<double>& prices, int fastPeriod = 12,
                      int slowPeriod = 26, int signalPeriod = 9) {
        MKMACDResult result;
        result.valid = false;

        if ((int)prices.size() < slowPeriod + signalPeriod) return result;

        auto fastEMA = EMA(prices, fastPeriod);
        auto slowEMA = EMA(prices, slowPeriod);

        if (fastEMA.empty() || slowEMA.empty()) return result;

        // Align EMAs - slow EMA starts later
        int offset = slowPeriod - fastPeriod;
        for (size_t i = 0; i < slowEMA.size(); i++) {
            if ((int)i + offset < (int)fastEMA.size()) {
                result.macdLine.push_back(fastEMA[i + offset] - slowEMA[i]);
            }
        }

        if ((int)result.macdLine.size() < signalPeriod) return result;

        // Signal line is EMA of MACD line
        result.signalLine = EMA(result.macdLine, signalPeriod);

        // Histogram = MACD - Signal
        int signalOffset = signalPeriod - 1;
        for (size_t i = 0; i < result.signalLine.size(); i++) {
            if ((int)i + signalOffset < (int)result.macdLine.size()) {
                result.histogram.push_back(
                    result.macdLine[i + signalOffset] - result.signalLine[i]);
            }
        }

        result.valid = true;
        return result;
    }

    // ========================================================
    // Bollinger Bands (20, 2) - SMA +/- 2 standard deviations
    // ========================================================
    MKBollingerBands BollingerBands(const std::vector<double>& prices,
                                     int period = 20, double numStdDev = 2.0) {
        MKBollingerBands result;
        result.valid = false;

        if ((int)prices.size() < period || period <= 0) return result;

        result.middle = SMA(prices, period);
        if (result.middle.empty()) return result;

        result.upper.reserve(result.middle.size());
        result.lower.reserve(result.middle.size());
        result.bandwidth.reserve(result.middle.size());

        for (size_t i = 0; i < result.middle.size(); i++) {
            // Calculate standard deviation for this window
            double sum = 0.0;
            int startIdx = static_cast<int>(i);
            for (int j = startIdx; j < startIdx + period; j++) {
                double diff = prices[j] - result.middle[i];
                sum += diff * diff;
            }
            double stdDev = std::sqrt(sum / period);

            result.upper.push_back(result.middle[i] + numStdDev * stdDev);
            result.lower.push_back(result.middle[i] - numStdDev * stdDev);

            double bw = (result.middle[i] > 0) ?
                (result.upper.back() - result.lower.back()) / result.middle[i] : 0.0;
            result.bandwidth.push_back(bw);
        }

        result.valid = true;
        return result;
    }

    // ========================================================
    // Volume Weighted Average Price (VWAP)
    // ========================================================
    std::vector<double> VWAP(const std::vector<double>& prices,
                             const std::vector<double>& volumes) {
        std::vector<double> result;
        if (prices.empty() || volumes.empty() || prices.size() != volumes.size()) return result;

        double cumulativePV = 0.0;
        double cumulativeV = 0.0;

        result.reserve(prices.size());
        for (size_t i = 0; i < prices.size(); i++) {
            cumulativePV += prices[i] * volumes[i];
            cumulativeV += volumes[i];
            if (cumulativeV > 0.0) {
                result.push_back(cumulativePV / cumulativeV);
            } else {
                result.push_back(prices[i]);
            }
        }

        return result;
    }

    // ========================================================
    // Support and Resistance Detection
    // ========================================================
    MKSupportResistance detectSupportResistance(const std::vector<double>& prices,
                                                 int lookback = 5) {
        MKSupportResistance result;
        result.valid = false;

        if ((int)prices.size() < lookback * 2 + 1) return result;

        // Find local minima (support) and maxima (resistance)
        for (int i = lookback; i < (int)prices.size() - lookback; i++) {
            bool isMin = true, isMax = true;

            for (int j = 1; j <= lookback; j++) {
                if (prices[i] >= prices[i - j] || prices[i] >= prices[i + j]) {
                    isMin = false;
                }
                if (prices[i] <= prices[i - j] || prices[i] <= prices[i + j]) {
                    isMax = false;
                }
            }

            if (isMin) {
                // Check if this level is not too close to an existing support
                bool tooClose = false;
                for (double s : result.supports) {
                    if (std::abs(prices[i] - s) / s < 0.005) { // Within 0.5%
                        tooClose = true;
                        break;
                    }
                }
                if (!tooClose) result.supports.push_back(prices[i]);
            }

            if (isMax) {
                bool tooClose = false;
                for (double r : result.resistances) {
                    if (std::abs(prices[i] - r) / r < 0.005) {
                        tooClose = true;
                        break;
                    }
                }
                if (!tooClose) result.resistances.push_back(prices[i]);
            }
        }

        std::sort(result.supports.begin(), result.supports.end());
        std::sort(result.resistances.begin(), result.resistances.end());

        result.valid = (!result.supports.empty() || !result.resistances.empty());
        return result;
    }

    // ========================================================
    // Moving Average Crossover Detection
    // ========================================================
    MKCrossoverSignal detectMACrossover(const std::vector<double>& prices,
                                         int shortPeriod = 50, int longPeriod = 200) {
        MKCrossoverSignal signal;
        signal.type = MKCrossoverSignal::NONE;
        signal.index = -1;
        signal.strength = 0.0;

        auto shortMA = SMA(prices, shortPeriod);
        auto longMA = SMA(prices, longPeriod);

        if (shortMA.size() < 2 || longMA.size() < 2) return signal;

        // Align MAs
        int offset = longPeriod - shortPeriod;
        if (offset < 0 || (int)shortMA.size() <= offset + 1) return signal;

        // Check last two points for crossover
        size_t longIdx = longMA.size() - 1;
        size_t shortIdx = longIdx + offset;

        if (shortIdx >= shortMA.size() || shortIdx < 1) return signal;

        double shortNow = shortMA[shortIdx];
        double shortPrev = shortMA[shortIdx - 1];
        double longNow = longMA[longIdx];
        double longPrev = longMA[longIdx - 1];

        // Golden cross: short MA crosses above long MA
        if (shortPrev <= longPrev && shortNow > longNow) {
            signal.type = MKCrossoverSignal::GOLDEN_CROSS;
            signal.index = static_cast<int>(prices.size() - 1);
            signal.strength = std::min(1.0, std::abs(shortNow - longNow) / longNow * 100.0);
        }
        // Death cross: short MA crosses below long MA
        else if (shortPrev >= longPrev && shortNow < longNow) {
            signal.type = MKCrossoverSignal::DEATH_CROSS;
            signal.index = static_cast<int>(prices.size() - 1);
            signal.strength = std::min(1.0, std::abs(shortNow - longNow) / longNow * 100.0);
        }

        return signal;
    }

    // ========================================================
    // Fibonacci Retracement Levels
    // ========================================================
    MKFibonacciLevels FibonacciRetracement(const std::vector<double>& prices) {
        MKFibonacciLevels result;
        result.valid = false;

        if (prices.empty()) return result;

        // Find swing high and low
        result.high = *std::max_element(prices.begin(), prices.end());
        result.low = *std::min_element(prices.begin(), prices.end());

        if (result.high == result.low) return result;

        double diff = result.high - result.low;

        // Fibonacci levels (measured from the high)
        result.level_236 = result.high - diff * 0.236;
        result.level_382 = result.high - diff * 0.382;
        result.level_500 = result.high - diff * 0.500;
        result.level_618 = result.high - diff * 0.618;
        result.level_786 = result.high - diff * 0.786;

        result.valid = true;
        return result;
    }

    // ========================================================
    // Average True Range (ATR) - volatility indicator
    // ========================================================
    std::vector<double> ATR(const std::vector<double>& highs,
                            const std::vector<double>& lows,
                            const std::vector<double>& closes,
                            int period = 14) {
        std::vector<double> result;
        if (highs.size() < 2 || highs.size() != lows.size() || highs.size() != closes.size()) {
            return result;
        }

        // Calculate True Range
        std::vector<double> trValues;
        trValues.push_back(highs[0] - lows[0]); // First TR is just high-low

        for (size_t i = 1; i < highs.size(); i++) {
            double hl = highs[i] - lows[i];
            double hc = std::abs(highs[i] - closes[i - 1]);
            double lc = std::abs(lows[i] - closes[i - 1]);
            trValues.push_back(std::max({hl, hc, lc}));
        }

        if ((int)trValues.size() < period) return result;

        // First ATR is simple average
        double sum = 0.0;
        for (int i = 0; i < period; i++) {
            sum += trValues[i];
        }
        double atr = sum / period;
        result.push_back(atr);

        // Subsequent ATRs use smoothing
        for (size_t i = period; i < trValues.size(); i++) {
            atr = (atr * (period - 1) + trValues[i]) / period;
            result.push_back(atr);
        }

        return result;
    }

    // ========================================================
    // Stochastic Oscillator
    // ========================================================
    std::vector<double> Stochastic(const std::vector<double>& highs,
                                    const std::vector<double>& lows,
                                    const std::vector<double>& closes,
                                    int period = 14) {
        std::vector<double> result;
        if ((int)closes.size() < period || highs.size() != closes.size() ||
            lows.size() != closes.size()) {
            return result;
        }

        for (size_t i = period - 1; i < closes.size(); i++) {
            double highestHigh = *std::max_element(
                highs.begin() + (i - period + 1), highs.begin() + i + 1);
            double lowestLow = *std::min_element(
                lows.begin() + (i - period + 1), lows.begin() + i + 1);

            double range = highestHigh - lowestLow;
            if (range > 0.0) {
                result.push_back(((closes[i] - lowestLow) / range) * 100.0);
            } else {
                result.push_back(50.0);
            }
        }

        return result;
    }

    // ========================================================
    // On-Balance Volume (OBV)
    // ========================================================
    std::vector<double> OBV(const std::vector<double>& prices,
                            const std::vector<double>& volumes) {
        std::vector<double> result;
        if (prices.size() < 2 || prices.size() != volumes.size()) return result;

        double obv = volumes[0];
        result.push_back(obv);

        for (size_t i = 1; i < prices.size(); i++) {
            if (prices[i] > prices[i - 1]) {
                obv += volumes[i];
            } else if (prices[i] < prices[i - 1]) {
                obv -= volumes[i];
            }
            result.push_back(obv);
        }

        return result;
    }

    // ========================================================
    // Utility: Get latest RSI value
    // ========================================================
    double getLatestRSI(const std::vector<double>& prices, int period = 14) {
        auto rsiValues = RSI(prices, period);
        if (rsiValues.empty()) return 50.0; // Neutral default
        return rsiValues.back();
    }

    // Utility: Check if RSI is overbought (>70) or oversold (<30)
    std::string getRSICondition(double rsi) {
        if (rsi >= 80.0) return "extremely_overbought";
        if (rsi >= 70.0) return "overbought";
        if (rsi <= 20.0) return "extremely_oversold";
        if (rsi <= 30.0) return "oversold";
        if (rsi >= 60.0) return "bullish";
        if (rsi <= 40.0) return "bearish";
        return "neutral";
    }
};

#endif // MK_TECHNICAL_ANALYSIS_CPP
