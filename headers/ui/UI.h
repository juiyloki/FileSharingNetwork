#pragma once
#include <vector>
#include <string>
#include "network/NetworkManager.h"
#include "message/Message.h"
#include "log/LogManager.h"
#include <cstddef>

namespace ui {

    class UI {

    public:

        void invalidOptionMenu();

        // New helper function for connecting to peer
        void connectPeerMenu();

        // Explicit constructor.
        explicit UI(network::NetworkManager& net);

        // Start UI loop.
        void run();

        // Observer callback
        void onMessageReceived(const message::Message& msg);

    private:

        //CHANGE: Add helper to parse single address string "ip:port"
        std::pair<std::string, std::string> parseAddress(const std::string& addr) const;


        // Managers' references.
        network::NetworkManager& net_;
        logging::LogManager& logger_ = logging::LogManager::instance();

        // Message data
        void showWelcome();
        void showMainMenu();
        void listPeersMenu();
        void sendMessageMenu();
        void inboxMenu();
        void viewSent();
        void viewReceived();

    };

}
