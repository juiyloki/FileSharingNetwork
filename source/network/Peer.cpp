#include "network/Peer.h"
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <sstream>

namespace network {

    // Constructs a Peer with a socket and listening address as its ID.
    // Initializes handlers as null (std::function default) and sets last active time.
    Peer::Peer(std::shared_ptr<tcp::socket> socket, const std::string& listeningAddress)
        : socket_(std::move(socket)),
          peerID_(listeningAddress),
          lastActiveTime_(std::chrono::steady_clock::now()) {}

    // Sends a message to this peer asynchronously.
    // The message is shared to ensure it persists during async write.
    void Peer::sendMessage(const std::string& message) {
        if (!isConnected()) {
            return;
        }
        auto msg = std::make_shared<std::string>(message);
        boost::asio::async_write(*socket_, boost::asio::buffer(*msg),
            [this, msg](const boost::system::error_code& ec, std::size_t) {
                if (ec) {
                    // Log error and trigger disconnect handler if set.
                    std::cerr << "Error sending message to " << peerID_ << ": " << ec.message() << "\n";
                    if (disconnectHandler_) {
                        disconnectHandler_();
                    }
                }
            });
    }

    // Starts asynchronous message receiving loop.
    // Uses a fixed-size buffer (1024 bytes); larger messages may be truncated.
    void Peer::startReceiving() {
        if (!isConnected()) {
            return;
        }
        socket_->async_read_some(
            boost::asio::buffer(buffer_.data(), buffer_.size()),
            [this](const boost::system::error_code& ec, std::size_t bytes) {
                handleReceive(ec, bytes);
            });
    }

    // Returns the peer's listening address (IP:port).
    std::string Peer::getAddress() const {
        if (!isConnected()) {
            return "Peer disconnected.";
        }
        boost::system::error_code ec;
        auto endpoint = socket_->remote_endpoint(ec);
        if (ec) {
            return "Unknown (error: " + ec.message() + ")";
        }
        return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    }

    // Returns the peer's unique identifier (listening address).
    const std::string& Peer::getPeerID() const {
        return peerID_;
    }

    // Sets the peer's unique identifier.
    void Peer::setPeerID(const std::string& id) {
        peerID_ = id;
    }

    // Checks if the peer's socket is open and connected.
    bool Peer::isConnected() const {
        return socket_ && socket_->is_open();
    }

    // Returns the last time the peer was active (received a message).
    std::chrono::steady_clock::time_point Peer::lastActive() const {
        return lastActiveTime_;
    }

    // Registers a callback for handling incoming messages.
    void Peer::onMessage(std::function<void(const std::string&)>&& handler) {
        messageHandler_ = std::move(handler);
    }

    // Registers a callback for handling disconnection events.
    void Peer::onDisconnect(std::function<void()>&& handler) {
        disconnectHandler_ = std::move(handler);
    }

    // Handles received messages and errors, continuing the async read loop.
    // Closes socket on any error (except operation_aborted) and triggers disconnect handler.
    void Peer::handleReceive(const boost::system::error_code& error, std::size_t bytes_transferred) {
        // Handle errors and disconnection.
        if (error) {
            if (error != boost::asio::error::operation_aborted) {
                std::cerr << "Receive error from " << peerID_ << ": " << error.message() << "\n";
            }
            boost::system::error_code ignore;
            if (socket_ && socket_->is_open()) {
                socket_->close(ignore);
            }
            if (disconnectHandler_) {
                disconnectHandler_();
            }
            return;
        }

        // Process received message and continue reading.
        lastActiveTime_ = std::chrono::steady_clock::now();
        std::string msg(buffer_.data(), bytes_transferred);
        if (messageHandler_) {
            messageHandler_(msg);
        }

        // Restart async read loop.
        socket_->async_read_some(
            boost::asio::buffer(buffer_.data(), buffer_.size()),
            [this](const boost::system::error_code& ec, std::size_t bytes) {
                handleReceive(ec, bytes);
            });
    }

    // Returns a string representation of the peer for UI display.
    // Shows the listening address and time since last activity.
    std::string Peer::toString() const {
        std::ostringstream oss;
        oss << "Address: " << peerID_;
        auto now = std::chrono::steady_clock::now();
        auto secondsSinceActive = std::chrono::duration_cast<std::chrono::seconds>(
            now - lastActiveTime_).count();
        oss << " | Last active: " << secondsSinceActive << " seconds ago";
        return oss.str();
    }

}  // namespace network