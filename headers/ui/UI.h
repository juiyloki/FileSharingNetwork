#pragma once

#include "log/LogManager.h"
#include "message/Message.h"
#include "network/NetworkManager.h"
#include <string>
#include <vector>

namespace ui {

    class UI {
    public:
        // Constructs the UI with a reference to the NetworkManager.
        explicit UI(network::NetworkManager& net);

        // Starts the main UI loop to handle user interactions.
        void run();

        // Callback for handling received messages.
        void onMessageReceived(const message::Message& msg);

    private:
        // Displays the welcome message.
        void showWelcome();

        // Displays the main menu options.
        void showMainMenu();

        // Displays the list of connected peers.
        void listPeersMenu();

        // Handles the menu for sending a message to a specific peer.
        void sendMessageMenu();

        // Handles the menu for broadcasting a message to all peers.
        void broadcastMessageMenu();

        // Displays the inbox with options to view sent or received messages.
        void inboxMenu();

        // Displays the list of sent messages.
        void viewSent();

        // Displays the list of received messages.
        void viewReceived();

        // Handles the menu for connecting to a peer.
        void connectPeerMenu();

        // Displays an error message for a invalid menu options.
        void invalidOptionMenu();

        // Parses an address string (ip:port) into IP and port components.
        std::pair<std::string, std::string> parseAddress(const std::string& addr) const;

        // Reference to the NetworkManager for network operations.
        network::NetworkManager& net_;

        // Reference to the LogManager singleton for logging operations.
        logging::LogManager& logger_ = logging::LogManager::instance();
    };

}  // namespace ui