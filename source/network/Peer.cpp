#include "network/Peer.h"
#include <iostream>
#include <sstream>
#include <boost/asio.hpp>

namespace network {

    // Constructor
    Peer::Peer(std::shared_ptr<tcp::socket> socket, const std::string& listeningAddress)
        : socket_(std::move(socket)), peerID_(listeningAddress), lastActiveTime_(std::chrono::steady_clock::now()),
          messageHandler_(nullptr), disconnectHandler_(nullptr) {}

    // Check if the peer is connected
    bool Peer::isConnected() const {
        return socket_ && socket_->is_open();
    }

    // Get the peer ID (listening address)
    const std::string& Peer::getPeerID() const {
        return peerID_;
    }

    // Set the peer ID
    void Peer::setPeerID(const std::string& id) {
        peerID_ = id;
    }

    // Get the IP:port string of the connected peer
    std::string Peer::getAddress() const {
        if (!isConnected()) return "Peer disconnected.";
        boost::system::error_code ec;
        auto endpoint = socket_->remote_endpoint(ec);
        if (ec) return "Unknown (error: " + ec.message() + ")";
        return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    }

    // Assign callbacks for message arrival and disconnection events
    void Peer::onMessage(std::function<void(const std::string&)>&& handler) {
        messageHandler_ = std::move(handler);
    }

    void Peer::onDisconnect(std::function<void()>&& handler) {
        disconnectHandler_ = std::move(handler);
    }

    // Send message with error handling
    void Peer::sendMessage(const std::string& message) {
        if (!isConnected()) return;
        auto msg = std::make_shared<std::string>(message);
        boost::asio::async_write(*socket_, boost::asio::buffer(*msg),
            [this, msg](const boost::system::error_code& ec, std::size_t) {
                if (ec) {
                    std::cerr << "Error sending message to " << peerID_ << ": " << ec.message() << "\n";
                    if (disconnectHandler_) disconnectHandler_();
                }
            });
    }

    // Start receiving messages
    void Peer::startReceiving() {
        if (!isConnected()) return;
        socket_->async_read_some(
            boost::asio::buffer(buffer_.data(), buffer_.size()),
            [this](const boost::system::error_code& ec, std::size_t bytes) {
                handleReceive(ec, bytes);
            });
    }

    // Handle incoming messages and disconnects
    void Peer::handleReceive(const boost::system::error_code& error, std::size_t bytes_transferred) {
        if (error) {
            if (error != boost::asio::error::operation_aborted) {
                std::cerr << "Receive error from " << peerID_ << ": " << error.message() << "\n";
            }
            boost::system::error_code ignore;
            if (socket_ && socket_->is_open()) socket_->close(ignore);
            if (disconnectHandler_) disconnectHandler_();
            return;
        }

        lastActiveTime_ = std::chrono::steady_clock::now();
        std::string msg(buffer_.data(), bytes_transferred);
        if (messageHandler_) messageHandler_(msg);

        socket_->async_read_some(
            boost::asio::buffer(buffer_.data(), buffer_.size()),
            [this](const boost::system::error_code& ec, std::size_t bytes) {
                handleReceive(ec, bytes);
            });
    }

    // Return peer information for UI
    std::string Peer::toString() const {
        std::ostringstream oss;
        oss << "Address: " << peerID_; // Use listening address
        auto now = std::chrono::steady_clock::now();
        auto secondsSinceActive = std::chrono::duration_cast<std::chrono::seconds>(
            now - lastActiveTime_).count();
        oss << " | Last active: " << secondsSinceActive << " seconds ago";
        return oss.str();
    }

}