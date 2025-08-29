#include "network/NetworkManager.h"
#include "ui/UI.h"
#include <iostream>
#include <string>

// Entry point for the P2P messaging application.
// Starts the server, runs the UI, and shuts down cleanly.
int main(int argc, char* argv[]) {
    using namespace network;

    // Parse port from command-line arguments, default to 5555.
    // Relies on std::stoi exceptions for validation.
    unsigned short port = 5555;
    if (argc > 1) {
        try {
            port = static_cast<unsigned short>(std::stoi(argv[1]));
        } catch (...) {
            std::cerr << "Invalid port number. Using default 5555.\n";
        }
    }

    // Initialize and start the server.
    NetworkManager& net = NetworkManager::instance();
    net.startServer(port);

    // Run the terminal UI.
    ui::UI ui(net);
    ui.run();

    // Clean up network resources.
    net.shutdown();

    return 0;
}