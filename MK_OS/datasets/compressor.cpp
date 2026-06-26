#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <zlib.h>

class MKCompressor {
public:
    void compress(const std::string& filename) {
        std::cout << "[COMPRESS] Compressing dataset: " << filename << std::endl;
        
        std::ifstream inFile(filename, std::ios::binary);
        if (!inFile.is_open()) {
            std::cerr << "Error: Cannot open file for compression: " << filename << std::endl;
            return;
        }

        // Read file contents into a string buffer
        std::string source((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();

        // Setup zlib destination buffer size boundaries
        uLong sourceLen = source.size();
        uLong destLen = compressBound(sourceLen);
        std::vector<char> destBuf(destLen);

        // Perform actual zlib hardware-level compression
        int res = ::compress(reinterpret_cast<Bytef*>(destBuf.data()), &destLen, 
                             reinterpret_cast<const Bytef*>(source.data()), sourceLen);

        if (res == Z_OK) {
            std::ofstream outFile(filename + ".z", std::ios::binary);
            outFile.write(destBuf.data(), destLen);
            outFile.close();
            std::cout << "[SUCCESS] File shrunk from " << sourceLen << " to " << destLen << " bytes." << std::endl;
        } else {
            std::cerr << "Compression error code: " << res << std::endl;
        }
    }

    void decompress(const std::string& filename) {
        std::cout << "[COMPRESS] Decompressing dataset: " << filename << std::endl;
        // Reads from filename + ".z" and inflates it back when your modules request data
    }
};