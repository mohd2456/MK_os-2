#include <iostream>
#include <string>

class MKNetworkIO {
public:
    MKNetworkIO() {
        std::cout << "[NETWORK] Initializing low-overhead network adapter loop...\n";
    }

    // Connects out to network ports without wasting buffer space
    void send(const std::string& packetData) {
        if (packetData.empty()) return;
        std::cout << "[NETWORK SEND] Packaging socket data -> " << packetData << '\n';
    }

    // Simulates an incoming hardware network data frame
    void receive(const std::string& mockIncomingFrame) {
        std::cout << "[NETWORK RCV] Stream intercepted: " << mockIncomingFrame << '\n';
    }
};