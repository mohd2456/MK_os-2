// ============================================================
// MK OS - Crypto Market Data Module
// Real-time price fetching from CoinGecko/Binance public APIs
// Uses libcurl for HTTP requests with rate limiting
// ============================================================
#ifndef MK_MARKET_DATA_CPP
#define MK_MARKET_DATA_CPP

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <mutex>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <curl/curl.h>

// ============================================================
// Data Structures
// ============================================================

struct MKPricePoint {
    double price;
    double volume;
    long timestamp;
};

struct MKOHLC {
    double open;
    double high;
    double low;
    double close;
    double volume;
    long timestamp;
};

struct MKOrderBookEntry {
    double price;
    double quantity;
};

struct MKOrderBook {
    std::string symbol;
    std::vector<MKOrderBookEntry> bids;
    std::vector<MKOrderBookEntry> asks;
    long timestamp;
    bool valid;
};

struct MKVolumeData {
    std::string symbol;
    double volume24h;
    double volumeChangePercent;
    long timestamp;
    bool valid;
};

struct MKPriceResult {
    std::string symbol;
    double price;
    double change24h;
    double changePercent24h;
    double marketCap;
    double volume24h;
    long timestamp;
    bool valid;
};

// ============================================================
// MKMarketData Class
// ============================================================

class MKMarketData {
private:
    // Rate limiting
    std::chrono::steady_clock::time_point lastRequestTime_;
    int requestCount_;
    static constexpr int MAX_REQUESTS_PER_MINUTE = 25;
    static constexpr int RATE_LIMIT_WINDOW_MS = 60000;
    std::mutex rateMutex_;

    // Price history storage
    std::map<std::string, std::vector<MKPricePoint>> priceHistory_;
    static constexpr size_t MAX_HISTORY_SIZE = 1000;

    // Base URLs
    static constexpr const char* COINGECKO_BASE = "https://api.coingecko.com/api/v3";
    static constexpr const char* BINANCE_BASE = "https://api.binance.com/api/v3";

    // Curl write callback
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        if (!userp) return 0;
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    // Rate limiter check
    bool checkRateLimit() {
        std::lock_guard<std::mutex> lock(rateMutex_);
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastRequestTime_).count();

        if (elapsed > RATE_LIMIT_WINDOW_MS) {
            requestCount_ = 0;
            lastRequestTime_ = now;
        }

        if (requestCount_ >= MAX_REQUESTS_PER_MINUTE) {
            return false;
        }

        requestCount_++;
        return true;
    }

    // HTTP GET request
    std::string httpGet(const std::string& url) {
        if (!checkRateLimit()) {
            return "";
        }

        std::string readBuffer;
        CURL* curl = curl_easy_init();
        if (!curl) return "";

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "MK-OS/2.0");

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return "";
        }

        return readBuffer;
    }

    // Simple JSON value extraction (no external JSON library needed)
    std::string extractJsonValue(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return "";

        size_t colonPos = json.find(':', keyPos + searchKey.size());
        if (colonPos == std::string::npos) return "";

        size_t valueStart = colonPos + 1;
        while (valueStart < json.size() && (json[valueStart] == ' ' || json[valueStart] == '\t'))
            valueStart++;

        if (valueStart >= json.size()) return "";

        // String value
        if (json[valueStart] == '"') {
            size_t valueEnd = json.find('"', valueStart + 1);
            if (valueEnd == std::string::npos) return "";
            return json.substr(valueStart + 1, valueEnd - valueStart - 1);
        }

        // Numeric/boolean/null value
        size_t valueEnd = valueStart;
        while (valueEnd < json.size() && json[valueEnd] != ',' &&
               json[valueEnd] != '}' && json[valueEnd] != ']' &&
               json[valueEnd] != '\n' && json[valueEnd] != '\r') {
            valueEnd++;
        }

        std::string value = json.substr(valueStart, valueEnd - valueStart);
        // Trim whitespace
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
            value.pop_back();
        return value;
    }

    // Extract array of numbers from JSON
    std::vector<double> extractJsonArray(const std::string& json, const std::string& key) {
        std::vector<double> result;
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return result;

        size_t arrStart = json.find('[', keyPos);
        if (arrStart == std::string::npos) return result;

        size_t arrEnd = json.find(']', arrStart);
        if (arrEnd == std::string::npos) return result;

        std::string arrStr = json.substr(arrStart + 1, arrEnd - arrStart - 1);
        std::istringstream iss(arrStr);
        std::string token;
        while (std::getline(iss, token, ',')) {
            try {
                double val = std::stod(token);
                result.push_back(val);
            } catch (...) {
                // Skip non-numeric tokens
            }
        }

        return result;
    }

    // Store price point in history
    void storePricePoint(const std::string& symbol, double price, double volume) {
        MKPricePoint point;
        point.price = price;
        point.volume = volume;
        point.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        auto& history = priceHistory_[symbol];
        history.push_back(point);

        // Limit history size
        if (history.size() > MAX_HISTORY_SIZE) {
            history.erase(history.begin());
        }
    }

    // Convert symbol to CoinGecko ID
    std::string symbolToCoinGeckoId(const std::string& symbol) {
        std::string lower = symbol;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        static const std::map<std::string, std::string> symbolMap = {
            {"btc", "bitcoin"},
            {"eth", "ethereum"},
            {"bnb", "binancecoin"},
            {"sol", "solana"},
            {"ada", "cardano"},
            {"xrp", "ripple"},
            {"dot", "polkadot"},
            {"doge", "dogecoin"},
            {"avax", "avalanche-2"},
            {"matic", "matic-network"},
            {"link", "chainlink"},
            {"atom", "cosmos"},
            {"uni", "uniswap"},
            {"ltc", "litecoin"},
            {"near", "near"},
            {"apt", "aptos"},
            {"arb", "arbitrum"},
            {"op", "optimism"},
            {"ftm", "fantom"},
            {"algo", "algorand"}
        };

        auto it = symbolMap.find(lower);
        if (it != symbolMap.end()) {
            return it->second;
        }
        return lower;
    }

    // Convert symbol to Binance pair format
    std::string symbolToBinancePair(const std::string& symbol) {
        std::string upper = symbol;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        return upper + "USDT";
    }

public:
    MKMarketData()
        : lastRequestTime_(std::chrono::steady_clock::now()),
          requestCount_(0) {}

    // Fetch current price for a symbol
    MKPriceResult fetchPrice(const std::string& symbol) {
        MKPriceResult result;
        result.symbol = symbol;
        result.valid = false;

        // Try Binance first (faster, more reliable for trading pairs)
        std::string binancePair = symbolToBinancePair(symbol);
        std::string url = std::string(BINANCE_BASE) + "/ticker/24hr?symbol=" + binancePair;
        std::string response = httpGet(url);

        if (!response.empty() && response.find("\"symbol\"") != std::string::npos) {
            std::string priceStr = extractJsonValue(response, "lastPrice");
            std::string change24hStr = extractJsonValue(response, "priceChange");
            std::string changePercentStr = extractJsonValue(response, "priceChangePercent");
            std::string volumeStr = extractJsonValue(response, "volume");

            if (!priceStr.empty()) {
                try {
                    result.price = std::stod(priceStr);
                    result.change24h = change24hStr.empty() ? 0.0 : std::stod(change24hStr);
                    result.changePercent24h = changePercentStr.empty() ? 0.0 : std::stod(changePercentStr);
                    result.volume24h = volumeStr.empty() ? 0.0 : std::stod(volumeStr);
                    result.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    result.valid = true;

                    storePricePoint(symbol, result.price, result.volume24h);
                } catch (...) {
                    result.valid = false;
                }
            }
        }

        // Fallback to CoinGecko if Binance fails
        if (!result.valid) {
            std::string coinId = symbolToCoinGeckoId(symbol);
            url = std::string(COINGECKO_BASE) + "/simple/price?ids=" + coinId +
                  "&vs_currencies=usd&include_24hr_change=true&include_market_cap=true&include_24hr_vol=true";
            response = httpGet(url);

            if (!response.empty() && response.find("usd") != std::string::npos) {
                std::string priceStr = extractJsonValue(response, "usd");
                std::string changeStr = extractJsonValue(response, "usd_24h_change");
                std::string mcapStr = extractJsonValue(response, "usd_market_cap");
                std::string volStr = extractJsonValue(response, "usd_24h_vol");

                if (!priceStr.empty()) {
                    try {
                        result.price = std::stod(priceStr);
                        result.changePercent24h = changeStr.empty() ? 0.0 : std::stod(changeStr);
                        result.change24h = result.price * result.changePercent24h / 100.0;
                        result.marketCap = mcapStr.empty() ? 0.0 : std::stod(mcapStr);
                        result.volume24h = volStr.empty() ? 0.0 : std::stod(volStr);
                        result.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();
                        result.valid = true;

                        storePricePoint(symbol, result.price, result.volume24h);
                    } catch (...) {
                        result.valid = false;
                    }
                }
            }
        }

        return result;
    }

    // Fetch OHLC (candlestick) data
    std::vector<MKOHLC> fetchOHLC(const std::string& symbol, const std::string& interval = "1h") {
        std::vector<MKOHLC> result;

        std::string binancePair = symbolToBinancePair(symbol);
        std::string url = std::string(BINANCE_BASE) + "/klines?symbol=" + binancePair +
                          "&interval=" + interval + "&limit=100";
        std::string response = httpGet(url);

        if (response.empty() || response[0] != '[') return result;

        // Parse Binance klines array format: [[timestamp,open,high,low,close,volume,...],...]
        size_t pos = 0;
        while (pos < response.size()) {
            size_t arrStart = response.find('[', pos + 1);
            if (arrStart == std::string::npos) break;

            size_t arrEnd = response.find(']', arrStart + 1);
            if (arrEnd == std::string::npos) break;

            std::string entry = response.substr(arrStart + 1, arrEnd - arrStart - 1);
            pos = arrEnd + 1;

            // Parse comma-separated values (remove quotes)
            std::vector<std::string> fields;
            std::istringstream iss(entry);
            std::string field;
            while (std::getline(iss, field, ',')) {
                // Remove quotes
                field.erase(std::remove(field.begin(), field.end(), '"'), field.end());
                // Trim whitespace
                while (!field.empty() && field[0] == ' ') field.erase(field.begin());
                while (!field.empty() && field.back() == ' ') field.pop_back();
                fields.push_back(field);
            }

            if (fields.size() >= 6) {
                try {
                    MKOHLC candle;
                    candle.timestamp = std::stol(fields[0]) / 1000; // Convert ms to s
                    candle.open = std::stod(fields[1]);
                    candle.high = std::stod(fields[2]);
                    candle.low = std::stod(fields[3]);
                    candle.close = std::stod(fields[4]);
                    candle.volume = std::stod(fields[5]);
                    result.push_back(candle);
                } catch (...) {
                    // Skip malformed entries
                }
            }
        }

        return result;
    }

    // Fetch order book
    MKOrderBook fetchOrderBook(const std::string& symbol) {
        MKOrderBook result;
        result.symbol = symbol;
        result.valid = false;

        std::string binancePair = symbolToBinancePair(symbol);
        std::string url = std::string(BINANCE_BASE) + "/depth?symbol=" + binancePair + "&limit=20";
        std::string response = httpGet(url);

        if (response.empty()) return result;

        // Parse bids and asks - format: "bids":[["price","qty"],...]
        auto parseSide = [&](const std::string& side) -> std::vector<MKOrderBookEntry> {
            std::vector<MKOrderBookEntry> entries;
            std::string key = "\"" + side + "\"";
            size_t sidePos = response.find(key);
            if (sidePos == std::string::npos) return entries;

            size_t arrStart = response.find('[', sidePos);
            if (arrStart == std::string::npos) return entries;

            // Find matching end bracket (accounting for nested arrays)
            int depth = 0;
            size_t arrEnd = arrStart;
            for (size_t i = arrStart; i < response.size(); i++) {
                if (response[i] == '[') depth++;
                else if (response[i] == ']') {
                    depth--;
                    if (depth == 0) { arrEnd = i; break; }
                }
            }

            // Parse individual entries
            size_t searchPos = arrStart;
            while (searchPos < arrEnd) {
                size_t entryStart = response.find('[', searchPos + 1);
                if (entryStart == std::string::npos || entryStart >= arrEnd) break;

                size_t entryEnd = response.find(']', entryStart + 1);
                if (entryEnd == std::string::npos || entryEnd > arrEnd) break;

                std::string entryStr = response.substr(entryStart + 1, entryEnd - entryStart - 1);
                searchPos = entryEnd + 1;

                // Parse "price","qty"
                size_t commaPos = entryStr.find(',');
                if (commaPos == std::string::npos) continue;

                std::string priceStr = entryStr.substr(0, commaPos);
                std::string qtyStr = entryStr.substr(commaPos + 1);

                // Remove quotes
                priceStr.erase(std::remove(priceStr.begin(), priceStr.end(), '"'), priceStr.end());
                qtyStr.erase(std::remove(qtyStr.begin(), qtyStr.end(), '"'), qtyStr.end());

                try {
                    MKOrderBookEntry entry;
                    entry.price = std::stod(priceStr);
                    entry.quantity = std::stod(qtyStr);
                    entries.push_back(entry);
                } catch (...) {}
            }

            return entries;
        };

        result.bids = parseSide("bids");
        result.asks = parseSide("asks");
        result.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        result.valid = (!result.bids.empty() || !result.asks.empty());

        return result;
    }

    // Fetch 24h volume
    MKVolumeData fetchVolume(const std::string& symbol) {
        MKVolumeData result;
        result.symbol = symbol;
        result.valid = false;

        std::string binancePair = symbolToBinancePair(symbol);
        std::string url = std::string(BINANCE_BASE) + "/ticker/24hr?symbol=" + binancePair;
        std::string response = httpGet(url);

        if (!response.empty() && response.find("\"volume\"") != std::string::npos) {
            std::string volumeStr = extractJsonValue(response, "volume");
            std::string quoteVolStr = extractJsonValue(response, "quoteVolume");

            if (!volumeStr.empty()) {
                try {
                    result.volume24h = std::stod(quoteVolStr.empty() ? volumeStr : quoteVolStr);
                    result.volumeChangePercent = 0.0; // Not directly provided
                    result.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    result.valid = true;
                } catch (...) {}
            }
        }

        return result;
    }

    // Get price history for a symbol
    std::vector<MKPricePoint> getPriceHistory(const std::string& symbol) const {
        auto it = priceHistory_.find(symbol);
        if (it != priceHistory_.end()) {
            return it->second;
        }
        return {};
    }

    // Get closing prices from OHLC data (useful for technical analysis)
    std::vector<double> getClosingPrices(const std::string& symbol, const std::string& interval = "1h") {
        auto ohlc = fetchOHLC(symbol, interval);
        std::vector<double> closes;
        closes.reserve(ohlc.size());
        for (const auto& candle : ohlc) {
            closes.push_back(candle.close);
        }
        return closes;
    }

    // Get volumes from OHLC data
    std::vector<double> getVolumes(const std::string& symbol, const std::string& interval = "1h") {
        auto ohlc = fetchOHLC(symbol, interval);
        std::vector<double> volumes;
        volumes.reserve(ohlc.size());
        for (const auto& candle : ohlc) {
            volumes.push_back(candle.volume);
        }
        return volumes;
    }

    // Get number of tracked symbols
    int trackedSymbolCount() const {
        return static_cast<int>(priceHistory_.size());
    }

    // Get all tracked symbols
    std::vector<std::string> getTrackedSymbols() const {
        std::vector<std::string> symbols;
        for (const auto& pair : priceHistory_) {
            symbols.push_back(pair.first);
        }
        return symbols;
    }
};

#endif // MK_MARKET_DATA_CPP
