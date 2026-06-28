#ifndef MK_FILE_READER_CPP
#define MK_FILE_READER_CPP

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <array>

// ============================================================
// MK File Reader
// Reads and summarizes text files of various formats.
// Supports: .txt, .md, .cpp, .py, .json, .csv, .log
// Can shell out to pdftotext for PDF reading.
// ============================================================

struct MKFileInfo {
    std::string path;
    std::string content;
    std::string extension;
    size_t fileSize;
    int lineCount;
    bool success;
    std::string error;
};

class MKFileReader {
private:
    static const size_t MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB limit

    std::string getExtension(const std::string& path) const {
        size_t dotPos = path.rfind('.');
        if (dotPos == std::string::npos) return "";
        std::string ext = path.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }

    bool isSupportedExtension(const std::string& ext) const {
        static const std::vector<std::string> supported = {
            "txt", "md", "cpp", "py", "json", "csv", "log",
            "h", "hpp", "c", "js", "ts", "html", "css",
            "yaml", "yml", "toml", "ini", "conf", "sh",
            "rs", "go", "java", "rb", "xml", "sql", "mk"
        };
        return std::find(supported.begin(), supported.end(), ext) != supported.end();
    }


public:
    MKFileReader() {}

    // Read a file and return its contents
    MKFileInfo readFile(const std::string& path) const {
        MKFileInfo info;
        info.path = path;
        info.success = false;
        info.fileSize = 0;
        info.lineCount = 0;

        std::string ext = getExtension(path);
        info.extension = ext;

        // Handle PDF specially
        if (ext == "pdf") {
            return readPDF(path);
        }

        // Check if supported
        if (!ext.empty() && !isSupportedExtension(ext)) {
            info.error = "Unsupported file type: ." + ext;
            return info;
        }

        // Open and read the file
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            info.error = "Cannot open file: " + path;
            return info;
        }

        info.fileSize = (size_t)file.tellg();
        if (info.fileSize > MAX_FILE_SIZE) {
            info.error = "File too large (" + std::to_string(info.fileSize / 1024 / 1024) + "MB). Max: 10MB";
            file.close();
            return info;
        }

        file.seekg(0, std::ios::beg);
        std::stringstream buffer;
        buffer << file.rdbuf();
        info.content = buffer.str();
        file.close();

        // Count lines
        info.lineCount = (int)std::count(info.content.begin(), info.content.end(), '\n');
        if (!info.content.empty() && info.content.back() != '\n') info.lineCount++;

        info.success = true;
        return info;
    }

    // Summarize content: show first N and last N lines
    std::string summarize(const std::string& content, int maxLines = 20) const {
        std::vector<std::string> lines;
        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line)) {
            lines.push_back(line);
        }

        if ((int)lines.size() <= maxLines) {
            return content; // Small enough to show entirely
        }

        int halfLines = maxLines / 2;
        std::string result;

        // First half
        for (int i = 0; i < halfLines; i++) {
            result += lines[i] + "\n";
        }

        result += "\n  ... (" + std::to_string((int)lines.size() - maxLines) + " lines omitted) ...\n\n";

        // Last half
        for (int i = (int)lines.size() - halfLines; i < (int)lines.size(); i++) {
            result += lines[i] + "\n";
        }

        return result;
    }

    // Read PDF by shelling out to pdftotext
    MKFileInfo readPDF(const std::string& path) const {
        MKFileInfo info;
        info.path = path;
        info.extension = "pdf";
        info.success = false;
        info.fileSize = 0;
        info.lineCount = 0;

        // Check if file exists
        std::ifstream check(path);
        if (!check.is_open()) {
            info.error = "Cannot open file: " + path;
            return info;
        }
        check.seekg(0, std::ios::end);
        info.fileSize = (size_t)check.tellg();
        check.close();

        // Shell out to pdftotext
        std::string cmd = "pdftotext \"" + path + "\" - 2>/dev/null";
        std::array<char, 4096> buffer;
        std::string result;

        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            info.error = "pdftotext not available. Install poppler-utils.";
            return info;
        }

        while (fgets(buffer.data(), (int)buffer.size(), pipe) != nullptr) {
            result += buffer.data();
            if (result.size() > MAX_FILE_SIZE) {
                result += "\n[TRUNCATED - file too large]";
                break;
            }
        }

        int exitCode = pclose(pipe);
        if (exitCode != 0 && result.empty()) {
            info.error = "pdftotext failed. Is the file a valid PDF?";
            return info;
        }

        info.content = result;
        info.lineCount = (int)std::count(result.begin(), result.end(), '\n');
        info.success = true;
        return info;
    }

    // Get a formatted display for a file reading result
    std::string formatResult(const MKFileInfo& info, int summaryLines = 30) const {
        if (!info.success) {
            return "Error: " + info.error;
        }

        std::string result;
        result += "File: " + info.path + "\n";
        result += "Type: ." + info.extension + " | Size: ";
        if (info.fileSize < 1024) result += std::to_string(info.fileSize) + " bytes";
        else if (info.fileSize < 1024 * 1024) result += std::to_string(info.fileSize / 1024) + " KB";
        else result += std::to_string(info.fileSize / 1024 / 1024) + " MB";
        result += " | Lines: " + std::to_string(info.lineCount) + "\n";
        result += std::string(50, '-') + "\n";
        result += summarize(info.content, summaryLines);

        return result;
    }
};

#endif // MK_FILE_READER_CPP
