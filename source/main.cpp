#include "network/NetworkManager.h"
#include "ui/UI.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    using namespace network;

    // Parse port from command line or use default
    unsigned short port = 5555;
    if (argc > 1) {
        try {
            port = static_cast<unsigned short>(std::stoi(argv[1]));
        } catch (...) {
            std::cerr << "Invalid port number. Using default 5555.\n";
        }
    }

    // Start the server
    NetworkManager& net = NetworkManager::instance();
    net.startServer(port);

    // Print startup info
    std::cout << "P2P Node started on port " << port << std::endl;
    std::cout << "Use another terminal to connect peers.\n";

    // Run terminal UI
    ui::UI ui(net);
    ui.run();

    // Shutdown cleanly on exit
    net.shutdown();

    return 0;
}
