#include "ui/UI.h"
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <algorithm>

namespace ui {

    UI::UI(network::NetworkManager& net) : net_(net), logger_(logging::LogManager::instance()) {}

    void UI::showWelcome() {
        std::cout << "\n=====================================\n";
        std::cout << "  Welcome to P2P Messenger\n";
        std::cout << "=====================================\n";
        std::cout << "Your listening address: " << net_.getListeningAddress() << "\n";
        std::cout << "You can share the above address with other peers to connect.\n";
    }

    void UI::run() {
        showWelcome();
        while (true) {
            showMainMenu();
            int choice;
            std::cin >> choice;
            std::cin.ignore();
            switch (choice) {
                case 1: connectPeerMenu(); break;
                case 2: listPeersMenu(); break;
                case 3: sendMessageMenu(); break;
                case 4: broadcastMessageMenu(); break; // New option
                case 5: inboxMenu(); break;
                case 0: return;
                default: invalidOptionMenu(); break;
            }
        }
    }

    void UI::invalidOptionMenu() {
        std::cout << "Invalid option\n";
    }

    std::pair<std::string, std::string> UI::parseAddress(const std::string& addr) const {
        auto pos = addr.find(':');
        if (pos == std::string::npos) return {addr, "5555"};
        return {addr.substr(0, pos), addr.substr(pos + 1)};
    }

    void UI::connectPeerMenu() {
        std::string address;
        std::cout << "\n-------------------\n";
        std::cout << "Enter peer address (ip:port): ";
        std::getline(std::cin, address);
        auto [ip, portStr] = parseAddress(address);
        try {
            unsigned short port = static_cast<unsigned short>(std::stoi(portStr));
            net_.connectToPeer(ip, port);
            std::cout << "Attempted to connect to " << ip << ":" << port << "\n";
        } catch (...) {
            std::cout << "Invalid port number\n";
        }
        std::cout << "-------------------\n";
    }

    void UI::showMainMenu() {
        std::cout << "\n-------------------\n";
        std::cout << "Main Menu:\n";
        std::cout << "1. Connect peer\n";
        std::cout << "2. List peers\n";
        std::cout << "3. Send message\n";
        std::cout << "4. Broadcast message\n"; // New option
        std::cout << "5. Inbox\n";
        std::cout << "0. Exit\n";
        std::cout << "-------------------\n";
    }

    void UI::listPeersMenu() {
        auto peers = net_.listPeerInfo();
        std::cout << "\n-------------------\n";
        if (peers.empty()) {
            std::cout << "No peers connected.\n";
            return;
        }
        std::cout << "Connected peers:\n";
        for (size_t i = 0; i < peers.size(); ++i) {
            std::cout << i + 1 << ". " << peers[i] << "\n";
        }
        std::cout << "-------------------\n";
    }

    void UI::sendMessageMenu() {
        auto peers = net_.listPeerInfo();
        std::cout << "\n-------------------\n";
        if (peers.empty()) {
            std::cout << "No connected peers available.\n";
            return;
        }
        std::cout << "Enter peer address: ";
        std::string peerAddr;
        std::getline(std::cin, peerAddr);

        // Check if peerAddr is in connected peers
        bool peerFound = false;
        for (const auto& peer : peers) {
            if (peer.find("Address: " + peerAddr) != std::string::npos) {
                peerFound = true;
                break;
            }
        }
        if (!peerFound) {
            std::cout << "Error: Peer " << peerAddr << " is not connected.\n";
            return;
        }

        std::cout << "Enter topic: ";
        std::string topic;
        std::getline(std::cin, topic);
        if (topic.empty()) topic = "(empty)";
        std::cout << "Enter message content: ";
        std::string content;
        std::getline(std::cin, content);
        message::Message msg(net_.getListeningAddress(), topic, content, message::MessageType::SENT);
        logging::LogManager::instance().appendMessage(msg);
        net_.sendMessage(peerAddr, msg.encode());
        std::cout << "Message sent and logged.\n";
        std::cout << "-------------------\n";
    }

    void UI::broadcastMessageMenu() {
        std::cout << "\n-------------------\n";
        auto peers = net_.listPeerInfo();
        if (peers.empty()) {
            std::cout << "No connected peers available to broadcast.\n";
            return;
        }

        std::cout << "Enter topic: ";
        std::string topic;
        std::getline(std::cin, topic);
        if (topic.empty()) topic = "(empty)";
        std::cout << "Enter message content: ";
        std::string content;
        std::getline(std::cin, content);
        message::Message msg(net_.getListeningAddress(), topic, content, message::MessageType::SENT);
        logging::LogManager::instance().appendMessage(msg);
        net_.broadcastMessage(msg.encode());
        std::cout << "Message broadcasted to all peers and logged.\n";
        std::cout << "-------------------\n";
    }

    void UI::inboxMenu() {
        std::cout << "\n-------------------\n";
        std::cout << "Inbox Menu:\n";
        std::cout << "1. View Sent\n";
        std::cout << "2. View Received\n";
        std::cout << "0. Back\n";
        std::cout << "-------------------\n";
        int choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        switch (choice) {
            case 1: viewSent(); break;
            case 2: viewReceived(); break;
            case 0: return;
            default: std::cout << "Invalid option.\n"; break;
        }
    }

    void UI::viewSent() {
        auto messages = logger_.getSentStrings();
        std::cout << "\n-------------------\n";
        if (messages.empty()) {
            std::cout << "No sent messages.\n";
            std::cout << "-------------------\n";
            return;
        }
        for (size_t i = 0; i < messages.size(); ++i) {
            std::cout << i + 1 << ". " << messages[i] << "\n";
        }

        std::cout << "Enter message number to open, 0 to back: ";
        size_t choice;
        std::cin >> choice;
        std::cout << "\n-------------------\n";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (choice == 0 || choice > messages.size()) return;

        // Access sent messages directly from LogManager
        auto sentMessages = logger_.readAll();
        std::vector<message::Message> sentOnly;
        for (const auto& msg : sentMessages) {
            if (msg.getType() == message::MessageType::SENT) {
                sentOnly.push_back(msg);
            }
        }
        if (choice <= sentOnly.size()) {
            const auto& msg = sentOnly[choice - 1];
            std::cout << "Topic: " << msg.getTopic() << "\n";
            std::cout << "Content: " << msg.getContent() << "\n";
            std::cout << "Delete this message? (y/n): ";
            char del;
            std::cin >> del;
            std::cout << "\n-------------------\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (del == 'y' || del == 'Y') {
                logger_.deleteMessage(choice - 1, true);
            }
        }
    }

    void UI::viewReceived() {
        auto messages = logger_.getReceivedStrings();
        std::cout << "\n-------------------\n";
        if (messages.empty()) {
            std::cout << "No received messages.\n";
            return;
        }
        for (size_t i = 0; i < messages.size(); ++i) {
            std::cout << i + 1 << ". " << messages[i] << "\n";
        }

        std::cout << "Enter message number to open, 0 to back: ";
        size_t choice;
        std::cin >> choice;
        std::cout << "\n-------------------\n";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (choice == 0 || choice > messages.size()) return;

        // Access received messages directly from LogManager
        auto receivedMessages = logger_.readAll();
        std::vector<message::Message> receivedOnly;
        for (const auto& msg : receivedMessages) {
            if (msg.getType() == message::MessageType::RECEIVED) {
                receivedOnly.push_back(msg);
            }
        }
        if (choice <= receivedOnly.size()) {
            const auto& msg = receivedOnly[choice - 1];
            std::cout << "Topic: " << msg.getTopic() << "\n";
            std::cout << "Content: " << msg.getContent() << "\n";
            std::cout << "Delete this message? (y/n): ";
            char del;
            std::cin >> del;
            std::cout << "\n-------------------\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (del == 'y' || del == 'Y') {
                logger_.deleteMessage(choice - 1, false);
            }
        }
        std::cout << "-------------------\n";
    }

    void UI::onMessageReceived(const message::Message& msg) {
        std::cout << "New message from " << msg.getPeerID() << " | Topic: " << msg.getTopic()
                  << " | Content: " << msg.getContent() << "\n";
    }

}