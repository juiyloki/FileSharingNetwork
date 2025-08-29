#pragma once

#include <boost/asio.hpp>
#include <array>
#include <memory>
#include <string>
#include <functional>
#include <chrono>

namespace network {

    class Peer {
    public:
        using tcp = boost::asio::ip::tcp;

        // Constructor using listening address as peer ID
        Peer(std::shared_ptr<tcp::socket> socket, const std::string& listeningAddress);

        // Messaging
        void sendMessage(const std::string& message);
        void startReceiving();

        // Identification
        std::string getAddress() const;
        const std::string& getPeerID() const;
        void setPeerID(const std::string& id); // Added setter for peerID

        // State
        bool isConnected() const;
        std::chrono::steady_clock::time_point lastActive() const;

        // Callbacks
        void onMessage(std::function<void(const std::string&)>&& handler);
        void onDisconnect(std::function<void()>&& handler);

        // UI display
        std::string toString() const;

    private:
        void handleReceive(const boost::system::error_code& error, std::size_t bytes_transferred);

        // Attributes
        std::shared_ptr<tcp::socket> socket_;
        std::array<char, 1024> buffer_;
        std::string peerID_; // Now stores listening address (IP:port)
        std::chrono::steady_clock::time_point lastActiveTime_;
        std::function<void(const std::string&)> messageHandler_;
        std::function<void()> disconnectHandler_;
    };

}