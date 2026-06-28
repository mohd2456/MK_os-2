#ifndef MK_IMAGE_ANALYZER_CPP
#define MK_IMAGE_ANALYZER_CPP

#include <string>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>

// ============================================================
// MK Image Analyzer
// Reads image files, detects format from magic bytes,
// parses dimensions from PNG IHDR or JPEG SOF0 markers.
// Returns metadata: format, width, height, fileSize.
// ============================================================

struct MKImageInfo {
    std::string format;        // "PNG", "JPEG", "GIF", "BMP", "Unknown"
    int width;
    int height;
    size_t fileSize;
    std::string description;
    bool success;
    std::string error;
};

class MKImageAnalyzer {
private:
    // Read N bytes from file at offset
    std::vector<uint8_t> readBytes(std::ifstream& file, size_t count) const {
        std::vector<uint8_t> buffer(count);
        file.read(reinterpret_cast<char*>(buffer.data()), count);
        buffer.resize((size_t)file.gcount());
        return buffer;
    }

    // Read big-endian uint32
    uint32_t readBE32(const uint8_t* data) const {
        return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
               ((uint32_t)data[2] << 8) | (uint32_t)data[3];
    }

    // Read big-endian uint16
    uint16_t readBE16(const uint8_t* data) const {
        return ((uint16_t)data[0] << 8) | (uint16_t)data[1];
    }

    // Read little-endian uint32
    uint32_t readLE32(const uint8_t* data) const {
        return ((uint32_t)data[3] << 24) | ((uint32_t)data[2] << 16) |
               ((uint32_t)data[1] << 8) | (uint32_t)data[0];
    }

    // Read little-endian uint16
    uint16_t readLE16(const uint8_t* data) const {
        return ((uint16_t)data[1] << 8) | (uint16_t)data[0];
    }


    // Detect format from magic bytes
    std::string detectFormat(const std::vector<uint8_t>& header) const {
        if (header.size() < 8) return "Unknown";

        // PNG: 89 50 4E 47 0D 0A 1A 0A
        if (header[0] == 0x89 && header[1] == 0x50 && header[2] == 0x4E &&
            header[3] == 0x47 && header[4] == 0x0D && header[5] == 0x0A &&
            header[6] == 0x1A && header[7] == 0x0A) {
            return "PNG";
        }

        // JPEG: FF D8 FF
        if (header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF) {
            return "JPEG";
        }

        // GIF: GIF87a or GIF89a
        if (header[0] == 'G' && header[1] == 'I' && header[2] == 'F' &&
            header[3] == '8' && (header[4] == '7' || header[4] == '9') &&
            header[5] == 'a') {
            return "GIF";
        }

        // BMP: BM
        if (header[0] == 'B' && header[1] == 'M') {
            return "BMP";
        }

        return "Unknown";
    }

    // Parse PNG IHDR chunk for width/height (starts at offset 8)
    bool parsePNG(std::ifstream& file, MKImageInfo& info) const {
        file.seekg(8); // Skip 8-byte signature
        auto chunkHeader = readBytes(file, 8); // length(4) + type(4)
        if (chunkHeader.size() < 8) return false;

        // Verify IHDR chunk type
        if (chunkHeader[4] != 'I' || chunkHeader[5] != 'H' ||
            chunkHeader[6] != 'D' || chunkHeader[7] != 'R') {
            return false;
        }

        // Read IHDR data (at least 8 bytes for width+height)
        auto ihdrData = readBytes(file, 8);
        if (ihdrData.size() < 8) return false;

        info.width = (int)readBE32(ihdrData.data());
        info.height = (int)readBE32(ihdrData.data() + 4);
        return true;
    }

    // Parse JPEG SOF0 marker for dimensions
    bool parseJPEG(std::ifstream& file, MKImageInfo& info) const {
        file.seekg(2); // Skip FF D8

        while (file.good()) {
            auto marker = readBytes(file, 2);
            if (marker.size() < 2) break;

            if (marker[0] != 0xFF) break;

            // SOF0 marker: FF C0
            if (marker[1] == 0xC0 || marker[1] == 0xC2) {
                auto lengthBytes = readBytes(file, 2);
                if (lengthBytes.size() < 2) break;

                auto sofData = readBytes(file, 5); // precision(1) + height(2) + width(2)
                if (sofData.size() < 5) break;

                info.height = (int)readBE16(sofData.data() + 1);
                info.width = (int)readBE16(sofData.data() + 3);
                return true;
            }

            // Skip other markers
            if (marker[1] != 0xD8 && marker[1] != 0xD9 && marker[1] != 0x00) {
                auto lengthBytes = readBytes(file, 2);
                if (lengthBytes.size() < 2) break;
                uint16_t length = readBE16(lengthBytes.data());
                if (length < 2) break;
                file.seekg((int)length - 2, std::ios::cur);
            }
        }
        return false;
    }

    // Parse GIF dimensions (at offset 6-9, little-endian)
    bool parseGIF(std::ifstream& file, MKImageInfo& info) const {
        file.seekg(6);
        auto dims = readBytes(file, 4);
        if (dims.size() < 4) return false;
        info.width = (int)readLE16(dims.data());
        info.height = (int)readLE16(dims.data() + 2);
        return true;
    }

    // Parse BMP dimensions (at offset 18-25, little-endian)
    bool parseBMP(std::ifstream& file, MKImageInfo& info) const {
        file.seekg(18);
        auto dims = readBytes(file, 8);
        if (dims.size() < 8) return false;
        info.width = (int)readLE32(dims.data());
        info.height = (int)readLE32(dims.data() + 4);
        if (info.height < 0) info.height = -info.height; // BMP can have negative height
        return true;
    }

public:
    MKImageAnalyzer() {}

    MKImageInfo analyze(const std::string& filepath) const {
        MKImageInfo info;
        info.width = 0;
        info.height = 0;
        info.fileSize = 0;
        info.success = false;

        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            info.error = "Cannot open file: " + filepath;
            return info;
        }

        // Get file size
        file.seekg(0, std::ios::end);
        info.fileSize = (size_t)file.tellg();
        file.seekg(0, std::ios::beg);

        // Read magic bytes
        auto header = readBytes(file, 16);
        info.format = detectFormat(header);

        // Parse dimensions based on format
        if (info.format == "PNG") {
            file.seekg(0);
            parsePNG(file, info);
        } else if (info.format == "JPEG") {
            file.seekg(0);
            parseJPEG(file, info);
        } else if (info.format == "GIF") {
            file.seekg(0);
            parseGIF(file, info);
        } else if (info.format == "BMP") {
            file.seekg(0);
            parseBMP(file, info);
        }

        file.close();

        // Generate description
        if (info.format != "Unknown") {
            info.description = info.format + " image, " +
                              std::to_string(info.width) + "x" + std::to_string(info.height) +
                              " pixels, ";
            if (info.fileSize < 1024) info.description += std::to_string(info.fileSize) + " bytes";
            else if (info.fileSize < 1024 * 1024)
                info.description += std::to_string(info.fileSize / 1024) + " KB";
            else
                info.description += std::to_string(info.fileSize / 1024 / 1024) + " MB";
            info.success = true;
        } else {
            info.error = "Unknown image format (not PNG/JPEG/GIF/BMP)";
        }

        return info;
    }

    // Format result for display
    std::string formatResult(const MKImageInfo& info) const {
        if (!info.success) return "Error: " + info.error;

        std::string result;
        result += "Format:     " + info.format + "\n";
        result += "Dimensions: " + std::to_string(info.width) + " x " +
                  std::to_string(info.height) + " px\n";
        result += "File Size:  ";
        if (info.fileSize < 1024) result += std::to_string(info.fileSize) + " bytes\n";
        else if (info.fileSize < 1024 * 1024)
            result += std::to_string(info.fileSize / 1024) + " KB\n";
        else result += std::to_string(info.fileSize / 1024 / 1024) + " MB\n";
        result += "Summary:    " + info.description;
        return result;
    }
};

#endif // MK_IMAGE_ANALYZER_CPP
