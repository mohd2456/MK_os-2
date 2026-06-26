#include <iostream>
#include <string>

class MKNetworkIO {
public:
    MKNetworkIO() {
        std::cout << "[NETWORK] Initializing low-overhead network adapter loop...\n";
    }

    // Sends packet data over simulated sockets
    void send(const std::string& packetData) {
        if (packetData.empty()) return;
        std::cout << "[NETWORK SEND] Packaging socket data -> " << packetData << '\n';
    }

    // Receives incoming data stream packets
    void receive(const std::string& mockIncomingFrame) {
        if (mockIncomingFrame.empty()) return;
        std::cout << "[NETWORK RCV] Stream intercepted: " << mockIncomingFrame << '\n';
    }
};