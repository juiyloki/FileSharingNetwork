#include "network/Peer.h"
#include <iostream>
#include <sstream>
#include <boost/asio.hpp>

namespace network {

    // Check if the peer is connected.
    bool Peer::isConnected() const {
        return socket_ && socket_->is_open();
    }   

    // Get the unique ID.
    const std::string& Peer::getPeerID() const {
        return peerID_;
    }

    // Safely get the IP:port string of the connected peer.
    std::string Peer::getAddress() const {
        if (!isConnected()) return "Peer disconnected.";

        boost::system::error_code ec;
        auto endpoint = socket_->remote_endpoint(ec);
        if (ec) return "Unknown (error: " + ec.message() + ")";
        return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());

    }

    // Assign callbacks for message arrival and disconnection events.
    void Peer::onMessage(std::function<void(const std::string&)>&& handler) {
        messageHandler_ = std::move(handler);
    }

    void Peer::onDisconnect(std::function<void()>&& handler) {
        disconnectHandler_ = std::move(handler);
    }  

    // Copy message safely, send message and set a lambda for error handling.
    void Peer::sendMessage(const std::string& message) {

        if (!isConnected()) return;

        auto msg = std::make_shared<std::string>(message);
        boost::asio::async_write(*socket_, boost::asio::buffer(*msg),
            [this, msg](const boost::system::error_code& ec, std::size_t) {
                if (ec) {
                    std::cerr << "Error sending message to " << peerID_ << ": " << ec.message() << "\n";
                    if (disconnectHandler_) disconnectHandler_();
                }
            }
        );

    }

    // Prepare for message arrival, on event or error call handleReceive().
    void Peer::startReceiving() {
        if (!isConnected()) return;

        socket_->async_read_some(
            boost::asio::buffer(buffer_.data(), buffer_.size()),
            [this](const boost::system::error_code& ec, std::size_t bytes) {
                handleReceive(ec, bytes);
            }
        );
    }


    // Behaviour on message recieve.
    void Peer::handleReceive(const boost::system::error_code& error,
                         std::size_t bytes_transferred) {
        // Error handling
        if (error) {
            if (error != boost::asio::error::operation_aborted) {
                std::cerr << "Receive error from " << peerID_
                      << ": " << error.message() << "\n";
            }
            boost::system::error_code ignore;
            if (socket_) socket_->close(ignore);
            if (disconnectHandler_) disconnectHandler_();
            return;
        }

        // Update last active timestamp.
        lastActiveTime_ = std::chrono::steady_clock::now();
        std::string msg(buffer_.data(), bytes_transferred);
        if (messageHandler_) {
            messageHandler_(msg);
        }

        // Go back to active waiting for message arival.
        socket_->async_read_some(
            boost::asio::buffer(buffer_.data(), buffer_.size()),
            [this](const boost::system::error_code& ec, std::size_t bytes) {
                handleReceive(ec, bytes);
            }
        );
    }

    // Turn Peer info to string for UI.
    std::string Peer::toString() const {
        std::ostringstream oss;

        oss << "PeerID: " << peerID_
            << " | Address: " << getAddress();

        auto now = std::chrono::steady_clock::now();
        auto secondsSinceActive = std::chrono::duration_cast<std::chrono::seconds>(
            now - lastActiveTime_
        ).count();

        oss << " | Last active: " << secondsSinceActive << " seconds ago";

        return oss.str();
    }

}