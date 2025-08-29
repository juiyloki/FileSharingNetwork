#pragma once

#include <array>
#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace network {

    class Peer {
    public:
        using tcp = boost::asio::ip::tcp;

        // Constructs a Peer with a socket and listening address as its ID.
        Peer(std::shared_ptr<tcp::socket> socket, const std::string& listeningAddress);

        // Sends a message to this peer.
        void sendMessage(const std::string& message);

        // Starts asynchronous message receiving.
        void startReceiving();

        // Returns the peer's listening address (IP:port).
        std::string getAddress() const;

        // Returns the peer's unique identifier.
        const std::string& getPeerID() const;

        // Sets the peer's unique identifier.
        void setPeerID(const std::string& id);

        // Checks if the peer is currently connected.
        bool isConnected() const;

        // Returns the last time the peer was active.
        std::chrono::steady_clock::time_point lastActive() const;

        // Registers a callback for handling incoming messages.
        void onMessage(std::function<void(const std::string&)>&& handler);

        // Registers a callback for handling disconnection events.
        void onDisconnect(std::function<void()>&& handler);

        // Returns a string representation of the peer for UI display.
        std::string toString() const;

    private:
        // Handles received messages and updates state based on error and bytes transferred.
        void handleReceive(const boost::system::error_code& error, std::size_t bytes_transferred);

        // Socket for communication with this peer.
        std::shared_ptr<tcp::socket> socket_;

        // Buffer for receiving messages.
        std::array<char, 1024> buffer_;

        // Unique identifier for the peer (IP:port).
        std::string peerID_;

        // Timestamp of the last activity from this peer.
        std::chrono::steady_clock::time_point lastActiveTime_;

        // Callback for processing incoming messages.
        std::function<void(const std::string&)> messageHandler_;

        // Callback for handling disconnection events.
        std::function<void()> disconnectHandler_;
    };

}  // namespace network