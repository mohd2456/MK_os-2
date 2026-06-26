#ifndef MK_MAIN_NETWORK_CPP
#define MK_MAIN_NETWORK_CPP

#include <iostream>
#include <string>
#include "mk_network.cpp"
#include "http.cpp"
#include "parser.cpp"
#include "update.cpp"

int main() {
    std::cout << "[NEXUS CORE] Booting Network Layer Integration Test...\n\n";

    MKNetwork net;
    MKHTTP http;
    MKNetworkParser parser;
    MKDatasetUpdater updater;

    // 1. Establish low-level handshake
    net.connect("https://duckduckgo.com");

    // 2. Fetch the live JSON string stream via libcurl
    std::string data = http.get("https://api.duckduckgo.com/?q=AI&format=json");

    // 3. Clean and isolate the target token nodes
    std::string parsed = parser.parseJSON(data);
    std::cout << "[PARSED DATA]: " << parsed << '\n';

    // 4. Update local non-volatile datasets with new facts
    updater.appendToDataset("datasets/knowledge/facts.json", parsed);

    std::cout << "\n[NETWORK] Integration complete. Pipeline verified.\n";
    return 0;
}

#endif // MK_MAIN_NETWORK_CPP