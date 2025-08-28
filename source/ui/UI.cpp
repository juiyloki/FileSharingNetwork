#include "ui/UI.h"
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>

#include <thread>
#include <cstddef>

namespace ui {

    // Constructor stores reference to NetworkManager and LogManager.
    UI::UI(network::NetworkManager& net) : net_(net), logger_(log::LogManager::instance()) {}

    // Display welcome message at startup.
    void UI::showWelcome() {
        std::cout << "=====================================\n";
        std::cout << "  Welcome to Encrypted P2P Messenger\n";
        std::cout << "=====================================\n\n";
    }

    // Main UI loop handling user input.
    void UI::run() {
        showWelcome();
        bool running = true;
        while (running) {
            showMainMenu();
            int choice;
            std::cin >> choice;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            switch(choice) {
                case 1: listPeersMenu(); break;
                case 2: sendMessageMenu(); break;
                case 3: inboxMenu(); break;
                case 0: running = false; break;
                default: std::cout << "Invalid option.\n"; break;
            }
        }
    }

    // Display main menu options.
    void UI::showMainMenu() {
        std::cout << "\nMain Menu:\n";
        std::cout << "1. List peers\n";
        std::cout << "2. Send message\n";
        std::cout << "3. Inbox\n";
        std::cout << "0. Exit\n";
        std::cout << "Choice: ";
    }

    // List connected peers using NetworkManager info.
    void UI::listPeersMenu() {
        std::vector<std::string> peers = net_.listPeerInfo(); // vector of strings from Peer::toString
        if (peers.empty()) {
            std::cout << "No peers connected.\n";
            return;
        }
        std::cout << "\nConnected peers:\n";
        for (size_t i = 0; i < peers.size(); ++i) {
            std::cout << i+1 << ". " << peers[i] << "\n";
        }
    }

    // Prompt user to send a message to a connected peer.
    void UI::sendMessageMenu() {
        auto peers = net_.listPeerInfo();
        if (peers.empty()) {
            std::cout << "No connected peers available.\n";
            return;
        }

        std::cout << "Enter Peer ID to send message: ";
        std::string peerID;
        std::getline(std::cin, peerID);

        auto it = std::find_if(peers.begin(), peers.end(),
                    [&](const std::string& s){ return s.find(peerID) != std::string::npos; });
        if (it == peers.end()) {
            std::cout << "Peer not connected.\n";
            return;
        }

        std::cout << "Enter topic (cannot be empty): ";
        std::string topic;
        std::getline(std::cin, topic);
        if (topic.empty()) topic = "(empty)";

        std::cout << "Enter message content: ";
        std::string content;
        std::getline(std::cin, content);

        // Send via NetworkManager
        net_.sendMessage(peerID, content);

        // Log sent message
        message::Message msg(peerID, topic, content, message::MessageType::SENT);
        logger_.appendMessage(msg);

        std::cout << "Message sent and logged.\n";
    }

    // Inbox menu to list sent and received messages.
    void UI::inboxMenu() {
        std::cout << "\nInbox Menu:\n";
        std::cout << "1. View Sent\n";
        std::cout << "2. View Received\n";
        std::cout << "0. Back\n";
        std::cout << "Choice: ";
        int choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch(choice) {
            case 1: viewSent(); break;
            case 2: viewReceived(); break;
            case 0: return;
            default: std::cout << "Invalid option.\n"; break;
        }
    }

    // Display sent messages and allow selecting to open or delete.
    void UI::viewSent() {
        auto messages = logger_.getSentStrings();
        if (messages.empty()) { std::cout << "No sent messages.\n"; return; }
        for (size_t i = 0; i < messages.size(); ++i)
            std::cout << i+1 << ". " << messages[i] << "\n";

        std::cout << "Enter message number to open, 0 to back: ";
        size_t choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (choice == 0 || choice > messages.size()) return;

        // Show content (decode from message vector)
        auto allMsgs = logger_.readAll();
        size_t count = 0;
        for (auto& msg : allMsgs) {
            if (msg.getType() == message::MessageType::SENT) {
                ++count;
                if (count == choice) {
                    std::cout << "Topic: " << msg.getTopic() << "\n";
                    std::cout << "Content: " << msg.getContent() << "\n";
                    std::cout << "Delete this message? (y/n): ";
                    char del; std::cin >> del; std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    if (del == 'y' || del == 'Y') logger_.deleteMessage(choice-1,true);
                    break;
                }
            }
        }
    }

    // Display received messages and allow selecting to open or delete.
    void UI::viewReceived() {
        auto messages = logger_.getReceivedStrings();
        if (messages.empty()) { std::cout << "No received messages.\n"; return; }
        for (size_t i = 0; i < messages.size(); ++i)
            std::cout << i+1 << ". " << messages[i] << "\n";

        std::cout << "Enter message number to open, 0 to back: ";
        size_t choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (choice == 0 || choice > messages.size()) return;

        // Show content (decode from message vector)
        auto allMsgs = logger_.readAll();
        size_t count = 0;
        for (auto& msg : allMsgs) {
            if (msg.getType() == message::MessageType::RECEIVED) {
                ++count;
                if (count == choice) {
                    std::cout << "Topic: " << msg.getTopic() << "\n";
                    std::cout << "Content: " << msg.getContent() << "\n";
                    std::cout << "Delete this message? (y/n): ";
                    char del; std::cin >> del; std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    if (del == 'y' || del == 'Y') logger_.deleteMessage(choice-1,false);
                    break;
                }
            }
        }
    }

}
