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
    UI::UI(network::NetworkManager& net) : net_(net), logger_(logging::LogManager::instance()) {}

    // Display welcome message at startup.
    void UI::showWelcome() {



        // Welcome message

            std::cout << "=====================================\n";
            std::cout << "  Welcome to P2P Messenger\n";
            std::cout << "=====================================\n\n";

            //Change: Display only public listening address for connection
            std::cout << "Your listening address: " << net_.getListeningAddress() << "\n";

            std::cout << "You can share the above address with other peers to connect.\n";
        }









        // Main UI loop handling user input.
        void UI::run() {
            //Change: Only call welcome and menu functions, no cout here
            showWelcome();

            while (true) {
                showMainMenu();

                int choice;
                std::cin >> choice;
                std::cin.ignore();

                switch (choice) {
                    case 1:
                        connectPeerMenu();
                    break;
                    case 2:
                        listPeersMenu();
                    break;
                    case 3:
                        sendMessageMenu();
                    break;
                    case 4:
                        inboxMenu();
                    break;
                    case 0:
                        return;
                    default:
                        invalidOptionMenu();
                    break;
                }
            }
        }

        //Change: New helper function to show invalid option message
        void UI::invalidOptionMenu() {
            std::cout << "Invalid option\n";
        }

    // File: source/ui/UI.cpp

    //CHANGE: Ask for single address input (ip:port) and split it
    void UI::connectPeerMenu() {
        std::string address;
        std::cout << "Enter peer address (ip:port): ";
        std::cin >> address;
        std::cin.ignore();

        auto [ip, portStr] = parseAddress(address); // Use NetworkManager parsing
        unsigned short port = static_cast<unsigned short>(std::stoi(portStr));

        net_.connectToPeer(ip, port);
        std::cout << "Attempted to connect to " << ip << ":" << port << "\n";
    }

    //CHANGE: Add const to implementation to match header.
    std::pair<std::string,std::string> UI::parseAddress(const std::string& addr) const {
        auto pos = addr.find(':');
        if (pos == std::string::npos) return {addr, "5555"}; // default port
        return {addr.substr(0, pos), addr.substr(pos + 1)};
    }






    // Display main menu options.
    void UI::showMainMenu() {
        std::cout << "\n-------------------\n";
        std::cout << "Main Menu:\n";
        std::cout << "1. Connect peer\n";
        std::cout << "2. List peers\n";
        std::cout << "3. Send message\n";
        std::cout << "4. Inbox\n";
        std::cout << "0. Exit\n";
        std::cout << "-------------------\n\n";
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

    //CHANGE: Encode topic and content in message before sending
    void UI::sendMessageMenu() {
        auto peers = net_.listPeerInfo();
        if (peers.empty()) {
            std::cout << "No connected peers available.\n";
            return;
        }

        std::cout << "Enter peer address: ";
        std::string peerAddr;
        std::getline(std::cin, peerAddr);

        std::cout << "Enter topic: ";
        std::string topic;
        std::getline(std::cin, topic);
        if (topic.empty()) topic = "(empty)";

        std::cout << "Enter message content: ";
        std::string content;
        std::getline(std::cin, content);

        //CHANGE: Encode topic and content together
        message::Message msg(peerAddr, topic, content, message::MessageType::SENT);
        logging::LogManager::instance().appendMessage(msg);

        net_.sendMessage(peerAddr, msg.encode());

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
