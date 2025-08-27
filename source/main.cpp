#include "network/NetworkManager.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace network;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n"
                  << "  " << argv[0] << " server <port>\n"
                  << "  " << argv[0] << " client <host> <port>\n";
        return 1;
    }

    auto& net = NetworkManager::instance();

    if (std::string(argv[1]) == "server") {
        if (argc < 3) { std::cerr << "Port required.\n"; return 1; }
        unsigned short port = static_cast<unsigned short>(std::stoi(argv[2]));
        net.startServer(port);
        std::cout << "Server started on port " << port << "\n";

    } else if (std::string(argv[1]) == "client") {
        if (argc < 4) { std::cerr << "Host + port required.\n"; return 1; }
        std::string host = argv[2];
        unsigned short port = static_cast<unsigned short>(std::stoi(argv[3]));
        net.connectToPeer(host, port);
        std::cout << "Client connecting to " << host << ":" << port << "\n";

    } else {
        std::cerr << "Unknown mode: " << argv[1] << "\n";
        return 1;
    }

    // keep main alive
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
