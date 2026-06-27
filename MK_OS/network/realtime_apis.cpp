#ifndef MK_REALTIME_APIS_CPP
#define MK_REALTIME_APIS_CPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <ctime>

#include "http.cpp"

// ===================================================================================
// MK REAL-TIME DATA APIs
// Provides live data from free, no-API-key-needed services:
//   - Current time in any timezone (worldtimeapi.org)
//   - Weather for any city (open-meteo.com geocoding + forecast)
//   - Currency exchange rates (exchangerate.host)
//   - Latest tech news headlines (HackerNews API)
//   - IP geolocation (ip-api.com)
// All requests use the existing MKHTTP class with libcurl.
// Designed for low-end hardware: minimal allocations, simple parsing.
// ===================================================================================

struct MKTimeData {
    std::string timezone;
    std::string datetime;
    std::string utcOffset;
    int dayOfWeek;     // 0=Sunday ... 6=Saturday
    int dayOfYear;
    int weekNumber;
    bool valid;
};

struct MKWeatherData {
    std::string city;
    double temperature;     // Celsius
    double windSpeed;       // km/h
    int humidity;           // percentage
    int weatherCode;        // WMO weather code
    std::string description;
    bool valid;
};

struct MKExchangeRate {
    std::string fromCurrency;
    std::string toCurrency;
    double rate;
    std::string date;
    bool valid;
};

struct MKNewsItem {
    std::string title;
    std::string url;
    int score;
    int id;
};

struct MKNewsData {
    std::vector<MKNewsItem> headlines;
    int totalFetched;
    bool valid;
};

struct MKGeoLocation {
    std::string ip;
    std::string country;
    std::string countryCode;
    std::string region;
    std::string city;
    double latitude;
    double longitude;
    std::string timezone;
    std::string isp;
    bool valid;
};

class MKRealtimeAPIs {
private:
    MKHTTP http;
    int totalRequests;
    int successfulRequests;
    int failedRequests;

    // Simple JSON string value extractor (reuse pattern from search_engine)
    std::string jsonGetString(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";

        pos += search.size();
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) pos++;
        if (pos >= json.size()) return "";

        if (json[pos] == '"') {
            pos++;
            std::string value;
            while (pos < json.size()) {
                if (json[pos] == '\\' && pos + 1 < json.size()) {
                    pos++;
                    if (json[pos] == '"') value += '"';
                    else if (json[pos] == 'n') value += '\n';
                    else if (json[pos] == '\\') value += '\\';
                    else value += json[pos];
                } else if (json[pos] == '"') {
                    break;
                } else {
                    value += json[pos];
                }
                pos++;
            }
            return value;
        }

        // Non-string value (number, bool, null)
        std::string value;
        while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != ']') {
            value += json[pos];
            pos++;
        }
        while (!value.empty() && (value.back() == ' ' || value.back() == '\n' || value.back() == '\r'))
            value.pop_back();
        return value;
    }

    // Parse a numeric value from JSON
    double jsonGetDouble(const std::string& json, const std::string& key) {
        std::string val = jsonGetString(json, key);
        if (val.empty() || val == "null") return 0.0;
        try {
            return std::stod(val);
        } catch (...) {
            return 0.0;
        }
    }

    int jsonGetInt(const std::string& json, const std::string& key) {
        std::string val = jsonGetString(json, key);
        if (val.empty() || val == "null") return 0;
        try {
            return std::stoi(val);
        } catch (...) {
            return 0;
        }
    }

    // URL-encode a string
    std::string urlEncode(const std::string& input) {
        std::string encoded;
        for (char c : input) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else if (c == ' ') {
                encoded += '+';
            } else {
                char hex[4];
                std::snprintf(hex, sizeof(hex), "%%%02X", (unsigned char)c);
                encoded += hex;
            }
        }
        return encoded;
    }

    // Convert WMO weather code to human description
    std::string weatherCodeToDescription(int code) {
        switch (code) {
            case 0: return "Clear sky";
            case 1: return "Mainly clear";
            case 2: return "Partly cloudy";
            case 3: return "Overcast";
            case 45: case 48: return "Foggy";
            case 51: case 53: case 55: return "Drizzle";
            case 56: case 57: return "Freezing drizzle";
            case 61: case 63: case 65: return "Rain";
            case 66: case 67: return "Freezing rain";
            case 71: case 73: case 75: return "Snowfall";
            case 77: return "Snow grains";
            case 80: case 81: case 82: return "Rain showers";
            case 85: case 86: return "Snow showers";
            case 95: return "Thunderstorm";
            case 96: case 99: return "Thunderstorm with hail";
            default: return "Unknown";
        }
    }

    // Extract array of integers from JSON array string
    std::vector<int> parseIntArray(const std::string& json, const std::string& key) {
        std::vector<int> result;
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return result;

        pos = json.find('[', pos);
        if (pos == std::string::npos) return result;
        pos++;

        std::string num;
        while (pos < json.size() && json[pos] != ']') {
            if (json[pos] == ',' || json[pos] == ' ' || json[pos] == '\n') {
                if (!num.empty()) {
                    try { result.push_back(std::stoi(num)); } catch (...) {}
                    num.clear();
                }
            } else {
                num += json[pos];
            }
            pos++;
        }
        if (!num.empty()) {
            try { result.push_back(std::stoi(num)); } catch (...) {}
        }
        return result;
    }

public:
    MKRealtimeAPIs() : totalRequests(0), successfulRequests(0), failedRequests(0) {
        std::cout << "[REALTIME] Real-time data API module initialized.\n";
    }

    // ─── Current Time in Any Timezone ──────────────────────────────────────────
    // Uses worldtimeapi.org/api/timezone/{area}/{location}
    MKTimeData getCurrentTime(const std::string& timezone) {
        totalRequests++;
        MKTimeData data;
        data.valid = false;
        data.timezone = timezone;
        data.dayOfWeek = 0;
        data.dayOfYear = 0;
        data.weekNumber = 0;

        std::string url = "http://worldtimeapi.org/api/timezone/" + timezone;
        std::string response = http.get(url);

        if (response.empty()) {
            failedRequests++;
            std::cout << "[REALTIME] Time request failed for timezone: " << timezone << "\n";
            return data;
        }

        data.datetime = jsonGetString(response, "datetime");
        data.utcOffset = jsonGetString(response, "utc_offset");
        data.dayOfWeek = jsonGetInt(response, "day_of_week");
        data.dayOfYear = jsonGetInt(response, "day_of_year");
        data.weekNumber = jsonGetInt(response, "week_number");

        if (!data.datetime.empty()) {
            data.valid = true;
            successfulRequests++;
            std::cout << "[REALTIME] Time in " << timezone << ": " << data.datetime << "\n";
        } else {
            failedRequests++;
        }

        return data;
    }

    // ─── Weather for Any City ──────────────────────────────────────────────────
    // Step 1: Geocode city name using open-meteo geocoding API
    // Step 2: Fetch weather using latitude/longitude
    MKWeatherData getWeather(const std::string& city) {
        totalRequests++;
        MKWeatherData data;
        data.valid = false;
        data.city = city;
        data.temperature = 0.0;
        data.windSpeed = 0.0;
        data.humidity = 0;
        data.weatherCode = 0;

        // Step 1: Geocode the city
        std::string geoUrl = "https://geocoding-api.open-meteo.com/v1/search?name="
                           + urlEncode(city) + "&count=1&language=en&format=json";
        std::string geoResponse = http.get(geoUrl);

        if (geoResponse.empty()) {
            failedRequests++;
            std::cout << "[REALTIME] Weather geocoding failed for: " << city << "\n";
            return data;
        }

        // Extract latitude and longitude from geocoding results
        double lat = jsonGetDouble(geoResponse, "latitude");
        double lon = jsonGetDouble(geoResponse, "longitude");

        if (lat == 0.0 && lon == 0.0) {
            failedRequests++;
            std::cout << "[REALTIME] Could not geocode city: " << city << "\n";
            return data;
        }

        // Step 2: Fetch current weather
        std::string weatherUrl = "https://api.open-meteo.com/v1/forecast?latitude="
                               + std::to_string(lat) + "&longitude=" + std::to_string(lon)
                               + "&current_weather=true";
        std::string weatherResponse = http.get(weatherUrl);

        if (weatherResponse.empty()) {
            failedRequests++;
            std::cout << "[REALTIME] Weather fetch failed for: " << city << "\n";
            return data;
        }

        // Parse current_weather object
        size_t cwPos = weatherResponse.find("\"current_weather\"");
        if (cwPos != std::string::npos) {
            std::string cwSection = weatherResponse.substr(cwPos);
            data.temperature = jsonGetDouble(cwSection, "temperature");
            data.windSpeed = jsonGetDouble(cwSection, "windspeed");
            data.weatherCode = jsonGetInt(cwSection, "weathercode");
            data.description = weatherCodeToDescription(data.weatherCode);
            data.valid = true;
            successfulRequests++;
            std::cout << "[REALTIME] Weather in " << city << ": "
                      << data.temperature << "C, " << data.description << "\n";
        } else {
            failedRequests++;
        }

        return data;
    }

    // ─── Currency Exchange Rates ───────────────────────────────────────────────
    // Uses exchangerate.host free API (no key required)
    MKExchangeRate getExchangeRate(const std::string& fromCurrency, const std::string& toCurrency) {
        totalRequests++;
        MKExchangeRate data;
        data.valid = false;
        data.fromCurrency = fromCurrency;
        data.toCurrency = toCurrency;
        data.rate = 0.0;

        std::string url = "https://api.exchangerate.host/latest?base="
                        + urlEncode(fromCurrency) + "&symbols=" + urlEncode(toCurrency);
        std::string response = http.get(url);

        if (response.empty()) {
            failedRequests++;
            std::cout << "[REALTIME] Exchange rate request failed: "
                      << fromCurrency << " -> " << toCurrency << "\n";
            return data;
        }

        data.date = jsonGetString(response, "date");
        data.rate = jsonGetDouble(response, toCurrency);

        if (data.rate > 0.0) {
            data.valid = true;
            successfulRequests++;
            std::cout << "[REALTIME] Exchange: 1 " << fromCurrency << " = "
                      << data.rate << " " << toCurrency << "\n";
        } else {
            // Try alternative parsing - rates might be nested
            size_t ratesPos = response.find("\"rates\"");
            if (ratesPos != std::string::npos) {
                std::string ratesSection = response.substr(ratesPos);
                data.rate = jsonGetDouble(ratesSection, toCurrency);
                if (data.rate > 0.0) {
                    data.valid = true;
                    successfulRequests++;
                    std::cout << "[REALTIME] Exchange: 1 " << fromCurrency << " = "
                              << data.rate << " " << toCurrency << "\n";
                } else {
                    failedRequests++;
                }
            } else {
                failedRequests++;
            }
        }

        return data;
    }

    // ─── Tech News Headlines (HackerNews API) ─────────────────────────────────
    // Fetches top story IDs, then fetches details for top N stories
    MKNewsData getTechNews(int maxHeadlines = 10) {
        totalRequests++;
        MKNewsData data;
        data.valid = false;
        data.totalFetched = 0;

        // Get top story IDs
        std::string topUrl = "https://hacker-news.firebaseio.com/v0/topstories.json";
        std::string topResponse = http.get(topUrl);

        if (topResponse.empty()) {
            failedRequests++;
            std::cout << "[REALTIME] HackerNews top stories request failed.\n";
            return data;
        }

        // Parse the array of story IDs (just the first N)
        std::vector<int> storyIds = parseIntArray(topResponse, "");
        if (storyIds.empty()) {
            // The response is a raw array like [123,456,...], parse directly
            std::string cleaned = topResponse;
            // Remove brackets
            if (!cleaned.empty() && cleaned[0] == '[') cleaned = cleaned.substr(1);
            if (!cleaned.empty() && cleaned.back() == ']') cleaned.pop_back();

            std::stringstream ss(cleaned);
            std::string token;
            while (std::getline(ss, token, ',') && (int)storyIds.size() < maxHeadlines) {
                // Trim whitespace
                while (!token.empty() && token[0] == ' ') token = token.substr(1);
                if (!token.empty()) {
                    try { storyIds.push_back(std::stoi(token)); } catch (...) {}
                }
            }
        }

        // Limit to maxHeadlines
        int fetchCount = std::min(maxHeadlines, (int)storyIds.size());

        // Fetch details for each story
        for (int i = 0; i < fetchCount; i++) {
            std::string itemUrl = "https://hacker-news.firebaseio.com/v0/item/"
                                + std::to_string(storyIds[i]) + ".json";
            std::string itemResponse = http.get(itemUrl);

            if (!itemResponse.empty()) {
                MKNewsItem item;
                item.title = jsonGetString(itemResponse, "title");
                item.url = jsonGetString(itemResponse, "url");
                item.score = jsonGetInt(itemResponse, "score");
                item.id = storyIds[i];

                if (!item.title.empty()) {
                    data.headlines.push_back(item);
                    data.totalFetched++;
                }
            }
        }

        if (data.totalFetched > 0) {
            data.valid = true;
            successfulRequests++;
            std::cout << "[REALTIME] Fetched " << data.totalFetched
                      << " tech news headlines from HackerNews.\n";
        } else {
            failedRequests++;
        }

        return data;
    }

    // ─── IP Geolocation ────────────────────────────────────────────────────────
    // Uses ip-api.com (free, no key, JSON format)
    MKGeoLocation getIPGeolocation(const std::string& ip = "") {
        totalRequests++;
        MKGeoLocation data;
        data.valid = false;
        data.latitude = 0.0;
        data.longitude = 0.0;

        // If no IP provided, use "json" endpoint for current IP
        std::string target = ip.empty() ? "json" : ("json/" + ip);
        std::string url = "http://ip-api.com/" + target;
        std::string response = http.get(url);

        if (response.empty()) {
            failedRequests++;
            std::cout << "[REALTIME] IP geolocation request failed.\n";
            return data;
        }

        std::string status = jsonGetString(response, "status");
        if (status != "success") {
            failedRequests++;
            std::cout << "[REALTIME] IP geolocation failed: " << jsonGetString(response, "message") << "\n";
            return data;
        }

        data.ip = jsonGetString(response, "query");
        data.country = jsonGetString(response, "country");
        data.countryCode = jsonGetString(response, "countryCode");
        data.region = jsonGetString(response, "regionName");
        data.city = jsonGetString(response, "city");
        data.latitude = jsonGetDouble(response, "lat");
        data.longitude = jsonGetDouble(response, "lon");
        data.timezone = jsonGetString(response, "timezone");
        data.isp = jsonGetString(response, "isp");
        data.valid = true;
        successfulRequests++;

        std::cout << "[REALTIME] Geolocation: " << data.ip << " -> "
                  << data.city << ", " << data.country << "\n";

        return data;
    }

    // ─── Statistics ────────────────────────────────────────────────────────────

    int getTotalRequests() const { return totalRequests; }
    int getSuccessfulRequests() const { return successfulRequests; }
    int getFailedRequests() const { return failedRequests; }

    void printStats() const {
        std::cout << "[REALTIME STATS] Total requests: " << totalRequests
                  << " | Successful: " << successfulRequests
                  << " | Failed: " << failedRequests << "\n";
    }
};

#endif // MK_REALTIME_APIS_CPP
