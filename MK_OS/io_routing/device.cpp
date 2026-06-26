#include <iostream>
#include <string>
#include <vector>

class MKDeviceIO {
public:
    void connectDevice(const std::string& device) {
        std::cout << "[DEVICE IO] Initializing hardware bridge to: " << device << '\n';
    }

    // Pillar 1 Optimization: Passes raw hardware data arrays via a pointer 
    // to a shared memory block, avoiding expensive string copying.
    void streamSensorDataZeroCopy(const std::string& device, const uint8_t* sharedBuffer, size_t bufferSize) {
        if (!sharedBuffer || bufferSize == 0) {
            std::cerr << "[DEVICE IO ERROR] Invalid shared memory address map." << '\n';
            return;
        }

        std::cout << "[DEVICE IO] Direct streaming from " << device 
                  << " | Processing " << bufferSize << " bytes via zero-copy pipeline.\n";
        
        // The AI Core or driver can now read 'sharedBuffer' instantly without a CPU memory transfer!
    }

    void sendToDevice(const std::string& device, const std::string& data) {
        std::cout << "[DEVICE IO] Sending command packets to " << device << ": " << data << '\n';
    }
};