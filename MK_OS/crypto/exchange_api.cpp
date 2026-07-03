// ============================================================
// MK OS - Crypto Exchange API
// Abstract exchange interface with Binance REST implementation
// HMAC-SHA256 request signing, API key management
// ============================================================
#ifndef MK_EXCHANGE_API_CPP
#define MK_EXCHANGE_API_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <cstring>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <curl/curl.h>

// ============================================================
// Data Structures
// ============================================================

enum class MKOrderSide {
    BUY,
    SELL
};

enum class MKOrderType {
    MARKET,
    LIMIT,
    STOP_LOSS,
    STOP_LOSS_LIMIT,
    TAKE_PROFIT,
    TAKE_PROFIT_LIMIT
};

enum class MKOrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    CANCELED,
    REJECTED,
    EXPIRED,
    UNKNOWN
};

struct MKOrder {
    std::string orderId;
    std::string symbol;
    MKOrderSide side;
    MKOrderType type;
    MKOrderStatus status;
    double quantity;
    double price;
    double filledQuantity;
    double avgFillPrice;
    long timestamp;
    bool valid;
};

struct MKBalance {
    std::string asset;
    double free;
    double locked;
    double total;
};

struct MKAccountInfo {
    std::vector<MKBalance> balances;
    double totalBTC;
    double totalUSD;
    bool valid;
};

struct MKExchangeConfig {
    std::string apiKey;
    std::string apiSecret;
    std::string baseUrl;
    bool testnet;
};

// ============================================================
// HMAC-SHA256 Implementation (minimal, no OpenSSL dependency)
// Uses a simple HMAC implementation for signing
// ============================================================

namespace MKCryptoAuth {

// SHA-256 constants
static const uint32_t k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
static uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
static uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
static uint32_t gam0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
static uint32_t gam1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

static std::vector<uint8_t> sha256(const std::vector<uint8_t>& msg) {
    uint32_t h0 = 0x6a09e667, h1 = 0xbb67ae85, h2 = 0x3c6ef372, h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f, h5 = 0x9b05688c, h6 = 0x1f83d9ab, h7 = 0x5be0cd19;

    // Pre-processing: padding
    size_t origLen = msg.size();
    size_t bitLen = origLen * 8;
    std::vector<uint8_t> padded = msg;
    padded.push_back(0x80);
    while ((padded.size() % 64) != 56) padded.push_back(0x00);
    for (int i = 7; i >= 0; i--) padded.push_back((bitLen >> (i * 8)) & 0xFF);

    // Process each 512-bit block
    for (size_t offset = 0; offset < padded.size(); offset += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            w[i] = ((uint32_t)padded[offset + i*4] << 24) |
                   ((uint32_t)padded[offset + i*4+1] << 16) |
                   ((uint32_t)padded[offset + i*4+2] << 8) |
                   ((uint32_t)padded[offset + i*4+3]);
        }
        for (int i = 16; i < 64; i++) {
            w[i] = gam1(w[i-2]) + w[i-7] + gam0(w[i-15]) + w[i-16];
        }

        uint32_t a = h0, b = h1, c = h2, d = h3;
        uint32_t e = h4, f = h5, g = h6, hh = h7;

        for (int i = 0; i < 64; i++) {
            uint32_t t1 = hh + sig1(e) + ch(e, f, g) + k[i] + w[i];
            uint32_t t2 = sig0(a) + maj(a, b, c);
            hh = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }

        h0 += a; h1 += b; h2 += c; h3 += d;
        h4 += e; h5 += f; h6 += g; h7 += hh;
    }

    std::vector<uint8_t> hash(32);
    for (int i = 0; i < 4; i++) {
        hash[i]    = (h0 >> (24 - i*8)) & 0xFF;
        hash[i+4]  = (h1 >> (24 - i*8)) & 0xFF;
        hash[i+8]  = (h2 >> (24 - i*8)) & 0xFF;
        hash[i+12] = (h3 >> (24 - i*8)) & 0xFF;
        hash[i+16] = (h4 >> (24 - i*8)) & 0xFF;
        hash[i+20] = (h5 >> (24 - i*8)) & 0xFF;
        hash[i+24] = (h6 >> (24 - i*8)) & 0xFF;
        hash[i+28] = (h7 >> (24 - i*8)) & 0xFF;
    }
    return hash;
}

static std::vector<uint8_t> hmacSha256(const std::string& key, const std::string& msg) {
    std::vector<uint8_t> keyBytes(key.begin(), key.end());

    // If key > 64 bytes, hash it
    if (keyBytes.size() > 64) {
        keyBytes = sha256(keyBytes);
    }

    // Pad key to 64 bytes
    keyBytes.resize(64, 0x00);

    // Inner and outer pads
    std::vector<uint8_t> ipad(64), opad(64);
    for (int i = 0; i < 64; i++) {
        ipad[i] = keyBytes[i] ^ 0x36;
        opad[i] = keyBytes[i] ^ 0x5c;
    }

    // Inner hash: SHA256(ipad || message)
    std::vector<uint8_t> innerInput = ipad;
    innerInput.insert(innerInput.end(), msg.begin(), msg.end());
    std::vector<uint8_t> innerHash = sha256(innerInput);

    // Outer hash: SHA256(opad || inner_hash)
    std::vector<uint8_t> outerInput = opad;
    outerInput.insert(outerInput.end(), innerHash.begin(), innerHash.end());

    return sha256(outerInput);
}

static std::string toHex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    for (uint8_t byte : data) {
        oss << std::hex << std::setfill('0') << std::setw(2) << (int)byte;
    }
    return oss.str();
}

} // namespace MKCryptoAuth

// ============================================================
// MKExchangeAPI Class
// ============================================================

class MKExchangeAPI {
private:
    MKExchangeConfig config_;
    bool initialized_;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        if (!userp) return 0;
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    // Generate signature for Binance API
    std::string sign(const std::string& queryString) {
        auto hmac = MKCryptoAuth::hmacSha256(config_.apiSecret, queryString);
        return MKCryptoAuth::toHex(hmac);
    }

    // Get current timestamp in milliseconds
    long getTimestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // Make a signed GET request
    std::string signedGet(const std::string& endpoint, const std::string& params = "") {
        std::string queryString = params;
        if (!queryString.empty()) queryString += "&";
        queryString += "timestamp=" + std::to_string(getTimestamp());

        std::string signature = sign(queryString);
        queryString += "&signature=" + signature;

        std::string url = config_.baseUrl + endpoint + "?" + queryString;

        std::string readBuffer;
        CURL* curl = curl_easy_init();
        if (!curl) return "";

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + config_.apiKey).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) return "";
        return readBuffer;
    }

    // Make a signed POST request
    std::string signedPost(const std::string& endpoint, const std::string& params) {
        std::string queryString = params;
        if (!queryString.empty()) queryString += "&";
        queryString += "timestamp=" + std::to_string(getTimestamp());

        std::string signature = sign(queryString);
        queryString += "&signature=" + signature;

        std::string url = config_.baseUrl + endpoint;

        std::string readBuffer;
        CURL* curl = curl_easy_init();
        if (!curl) return "";

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + config_.apiKey).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, queryString.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) return "";
        return readBuffer;
    }

    // Make a signed DELETE request
    std::string signedDelete(const std::string& endpoint, const std::string& params) {
        std::string queryString = params;
        if (!queryString.empty()) queryString += "&";
        queryString += "timestamp=" + std::to_string(getTimestamp());

        std::string signature = sign(queryString);
        queryString += "&signature=" + signature;

        std::string url = config_.baseUrl + endpoint + "?" + queryString;

        std::string readBuffer;
        CURL* curl = curl_easy_init();
        if (!curl) return "";

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + config_.apiKey).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) return "";
        return readBuffer;
    }

    // Simple JSON value extraction
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

        if (json[valueStart] == '"') {
            size_t valueEnd = json.find('"', valueStart + 1);
            if (valueEnd == std::string::npos) return "";
            return json.substr(valueStart + 1, valueEnd - valueStart - 1);
        }

        size_t valueEnd = valueStart;
        while (valueEnd < json.size() && json[valueEnd] != ',' &&
               json[valueEnd] != '}' && json[valueEnd] != ']') {
            valueEnd++;
        }
        std::string val = json.substr(valueStart, valueEnd - valueStart);
        while (!val.empty() && (val.back() == ' ' || val.back() == '\t')) val.pop_back();
        return val;
    }

    // URL encode
    std::string urlEncode(const std::string& str) {
        CURL* curl = curl_easy_init();
        if (!curl) return str;
        char* output = curl_easy_escape(curl, str.c_str(), static_cast<int>(str.length()));
        std::string encoded(output);
        curl_free(output);
        curl_easy_cleanup(curl);
        return encoded;
    }

public:
    MKExchangeAPI() : initialized_(false) {
        config_.baseUrl = "https://api.binance.com";
        config_.testnet = false;
    }

    // Initialize with config file or environment variables
    bool initialize(const std::string& configPath = "") {
        // Try config file first
        if (!configPath.empty()) {
            std::ifstream file(configPath);
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    size_t eq = line.find('=');
                    if (eq == std::string::npos) continue;
                    std::string key = line.substr(0, eq);
                    std::string value = line.substr(eq + 1);
                    // Trim
                    while (!key.empty() && key.back() == ' ') key.pop_back();
                    while (!value.empty() && value[0] == ' ') value.erase(value.begin());

                    if (key == "BINANCE_API_KEY") config_.apiKey = value;
                    else if (key == "BINANCE_API_SECRET") config_.apiSecret = value;
                    else if (key == "BINANCE_TESTNET" && value == "true") {
                        config_.testnet = true;
                        config_.baseUrl = "https://testnet.binance.vision";
                    }
                }
                file.close();
            }
        }

        // Fall back to environment variables
        if (config_.apiKey.empty()) {
            const char* key = std::getenv("BINANCE_API_KEY");
            if (key) config_.apiKey = key;
        }
        if (config_.apiSecret.empty()) {
            const char* secret = std::getenv("BINANCE_API_SECRET");
            if (secret) config_.apiSecret = secret;
        }

        initialized_ = !config_.apiKey.empty() && !config_.apiSecret.empty();
        return initialized_;
    }

    // Enable testnet mode
    void enableTestnet() {
        config_.testnet = true;
        config_.baseUrl = "https://testnet.binance.vision";
    }

    // ========================================================
    // Place an order
    // ========================================================
    MKOrder placeOrder(MKOrderSide side, const std::string& symbol,
                       double quantity, MKOrderType type, double price = 0.0) {
        MKOrder order;
        order.symbol = symbol;
        order.side = side;
        order.type = type;
        order.quantity = quantity;
        order.price = price;
        order.valid = false;

        if (!initialized_) {
            order.status = MKOrderStatus::REJECTED;
            return order;
        }

        std::string upperSymbol = symbol;
        std::transform(upperSymbol.begin(), upperSymbol.end(), upperSymbol.begin(), ::toupper);
        if (upperSymbol.find("USDT") == std::string::npos) {
            upperSymbol += "USDT";
        }

        std::string params = "symbol=" + upperSymbol;
        params += "&side=" + std::string(side == MKOrderSide::BUY ? "BUY" : "SELL");

        std::string orderTypeStr;
        switch (type) {
            case MKOrderType::MARKET: orderTypeStr = "MARKET"; break;
            case MKOrderType::LIMIT: orderTypeStr = "LIMIT"; break;
            case MKOrderType::STOP_LOSS: orderTypeStr = "STOP_LOSS"; break;
            case MKOrderType::STOP_LOSS_LIMIT: orderTypeStr = "STOP_LOSS_LIMIT"; break;
            case MKOrderType::TAKE_PROFIT: orderTypeStr = "TAKE_PROFIT"; break;
            case MKOrderType::TAKE_PROFIT_LIMIT: orderTypeStr = "TAKE_PROFIT_LIMIT"; break;
        }
        params += "&type=" + orderTypeStr;

        // Format quantity to avoid scientific notation
        std::ostringstream qtyStr;
        qtyStr << std::fixed << std::setprecision(8) << quantity;
        params += "&quantity=" + qtyStr.str();

        if (type == MKOrderType::LIMIT && price > 0) {
            std::ostringstream priceStr;
            priceStr << std::fixed << std::setprecision(8) << price;
            params += "&price=" + priceStr.str();
            params += "&timeInForce=GTC";
        }

        std::string response = signedPost("/api/v3/order", params);

        if (!response.empty()) {
            order.orderId = extractJsonValue(response, "orderId");
            std::string statusStr = extractJsonValue(response, "status");

            if (statusStr == "NEW") order.status = MKOrderStatus::NEW;
            else if (statusStr == "FILLED") order.status = MKOrderStatus::FILLED;
            else if (statusStr == "PARTIALLY_FILLED") order.status = MKOrderStatus::PARTIALLY_FILLED;
            else if (statusStr == "CANCELED") order.status = MKOrderStatus::CANCELED;
            else if (statusStr == "REJECTED") order.status = MKOrderStatus::REJECTED;
            else order.status = MKOrderStatus::UNKNOWN;

            std::string filledQty = extractJsonValue(response, "executedQty");
            if (!filledQty.empty()) {
                try { order.filledQuantity = std::stod(filledQty); } catch (...) {}
            }

            order.timestamp = std::time(nullptr);
            order.valid = !order.orderId.empty();
        }

        return order;
    }

    // ========================================================
    // Cancel an order
    // ========================================================
    bool cancelOrder(const std::string& symbol, const std::string& orderId) {
        if (!initialized_) return false;

        std::string upperSymbol = symbol;
        std::transform(upperSymbol.begin(), upperSymbol.end(), upperSymbol.begin(), ::toupper);
        if (upperSymbol.find("USDT") == std::string::npos) {
            upperSymbol += "USDT";
        }

        std::string params = "symbol=" + upperSymbol + "&orderId=" + orderId;
        std::string response = signedDelete("/api/v3/order", params);

        return !response.empty() && response.find("\"status\"") != std::string::npos;
    }

    // ========================================================
    // Get account balance
    // ========================================================
    MKAccountInfo getBalance() {
        MKAccountInfo info;
        info.valid = false;
        info.totalBTC = 0.0;
        info.totalUSD = 0.0;

        if (!initialized_) return info;

        std::string response = signedGet("/api/v3/account");
        if (response.empty()) return info;

        // Parse balances from response
        size_t searchPos = 0;
        while (true) {
            size_t assetPos = response.find("\"asset\"", searchPos);
            if (assetPos == std::string::npos) break;

            std::string asset = extractJsonValue(
                response.substr(assetPos - 1), "asset");

            size_t freePos = response.find("\"free\"", assetPos);
            size_t lockedPos = response.find("\"locked\"", assetPos);

            if (freePos == std::string::npos) break;

            std::string freeStr = extractJsonValue(
                response.substr(freePos - 1), "free");
            std::string lockedStr = (lockedPos != std::string::npos) ?
                extractJsonValue(response.substr(lockedPos - 1), "locked") : "0";

            double freeVal = 0.0, lockedVal = 0.0;
            try {
                freeVal = std::stod(freeStr);
                lockedVal = std::stod(lockedStr);
            } catch (...) {}

            if (freeVal > 0 || lockedVal > 0) {
                MKBalance bal;
                bal.asset = asset;
                bal.free = freeVal;
                bal.locked = lockedVal;
                bal.total = freeVal + lockedVal;
                info.balances.push_back(bal);
            }

            searchPos = (lockedPos != std::string::npos) ? lockedPos + 10 : freePos + 10;
        }

        info.valid = true;
        return info;
    }

    // ========================================================
    // Get open orders
    // ========================================================
    std::vector<MKOrder> getOpenOrders(const std::string& symbol = "") {
        std::vector<MKOrder> orders;
        if (!initialized_) return orders;

        std::string params;
        if (!symbol.empty()) {
            std::string upperSymbol = symbol;
            std::transform(upperSymbol.begin(), upperSymbol.end(), upperSymbol.begin(), ::toupper);
            if (upperSymbol.find("USDT") == std::string::npos) {
                upperSymbol += "USDT";
            }
            params = "symbol=" + upperSymbol;
        }

        std::string response = signedGet("/api/v3/openOrders", params);
        if (response.empty()) return orders;

        // Simple parsing - count order entries
        size_t pos = 0;
        while (true) {
            size_t orderStart = response.find("\"orderId\"", pos);
            if (orderStart == std::string::npos) break;

            MKOrder order;
            order.orderId = extractJsonValue(response.substr(orderStart - 1), "orderId");
            order.symbol = extractJsonValue(response.substr(orderStart - 1), "symbol");

            std::string sideStr = extractJsonValue(response.substr(orderStart - 1), "side");
            order.side = (sideStr == "BUY") ? MKOrderSide::BUY : MKOrderSide::SELL;

            order.status = MKOrderStatus::NEW;
            order.valid = !order.orderId.empty();

            std::string priceStr = extractJsonValue(response.substr(orderStart - 1), "price");
            std::string qtyStr = extractJsonValue(response.substr(orderStart - 1), "origQty");
            try {
                if (!priceStr.empty()) order.price = std::stod(priceStr);
                if (!qtyStr.empty()) order.quantity = std::stod(qtyStr);
            } catch (...) {}

            if (order.valid) orders.push_back(order);

            pos = orderStart + 10;
        }

        return orders;
    }

    // ========================================================
    // Getters
    // ========================================================
    bool isInitialized() const { return initialized_; }
    bool isTestnet() const { return config_.testnet; }
    std::string getBaseUrl() const { return config_.baseUrl; }
};

#endif // MK_EXCHANGE_API_CPP
