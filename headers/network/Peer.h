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

    // Constructor
    Peer(std::shared_ptr<tcp::socket> socket, const std::string& peerID):
        socket_(std::move(socket)),
        peerID_(peerID),
        lastActiveTime_(std::chrono::steady_clock::now()),
        messageHandler_(nullptr),
        disconnectHandler_(nullptr)
    {}

    // Messaging
    void sendMessage(const std::string& message);
    void startReceiving();

    // Identification:
    // IP:port and unique ID
    std::string getAddress() const;
    const std::string& getPeerID() const;

    // State
    bool isConnected() const {
        return socket_ && socket_->is_open();
    }
    std::chrono::steady_clock::time_point lastActive() const;
    
    // Callbacks 
    // set by NetworkManager
    void onMessage(std::function<void(const std::string&)>&& handler);
    void onDisconnect(std::function<void()>&& handler);

 
private:

    void handleReceive(const boost::system::error_code& error, std::size_t bytes_transferred);
    
    // Attributes
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
    std::array<char, 1024> buffer_;
    const std::string peerID_;
    std::chrono::steady_clock::time_point lastActiveTime_;

    // Event handlers
    std::function<void(const std::string&)> messageHandler_;
    std::function<void()> disconnectHandler_;
};

}

