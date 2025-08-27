#pragma once

#include "network/Peer.h"
#include <boost/asio.hpp>
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>

namespace network {

class NetworkManager {

public:

    using tcp = boost::asio::ip::tcp;

    // Singleton accessor
    static NetworkManager& instance();

    // Start listening for incoming connections
    void startServer(unsigned short port);

    // Connect to a remote peer
    void connectToPeer(const std::string& host, unsigned short port);

    // Send message to specific peer
    void sendMessage(const std::string& peerID, const std::string& message);

    // Broadcast to all peers
    void broadcastMessage(const std::string& message);

    // Stop and close everything
    void shutdown();

    // Get current peers
    std::vector<std::string> listPeers() const;

private:

    // Private constructor
    NetworkManager();
    ~NetworkManager();

    // No copy/move
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    // Accept new connection
    void doAccept();

    // Handle peer disconnect
    void removePeer(const std::string& peerID);

    // Attributes
    boost::asio::io_context ioContext_;
    tcp::acceptor acceptor_;
    std::unordered_map<std::string, std::shared_ptr<Peer>> peers_;
    mutable std::mutex peersMutex_;
};

}
