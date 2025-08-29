#pragma once

#include "network/Peer.h"
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace network {

    class NetworkManager {
    public:
        using tcp = boost::asio::ip::tcp;

        // Returns the singleton instance of NetworkManager.
        static NetworkManager& instance();

        // Starts the server to listen for incoming connections on the specified port.
        void startServer(unsigned short port);

        // Connects to a peer at the given IP and port.
        void connectToPeer(const std::string& ip, unsigned short port);

        // Sends a message to a specific peer identified by peerID.
        void sendMessage(const std::string& peerID, const std::string& message);

        // Broadcasts a message to all connected peers.
        void broadcastMessage(const std::string& message);

        // Retrieves the current listening address (IP:port) of the server.
        std::string getListeningAddress() const;

        // Registers a callback to handle peer disconnection events.
        void onPeerDisconnected(std::function<void(const std::string&)> handler);

        // Returns a list of connected peers' information.
        std::vector<std::string> listPeerInfo() const;

        // Shuts down the network manager and closes all connections.
        void shutdown();

    private:
        // Private constructor to enforce singleton pattern.
        NetworkManager();

        // Destructor to clean up resources.
        ~NetworkManager();

        // Deleted copy constructor and assignment operator to prevent copying.
        NetworkManager(const NetworkManager&) = delete;
        NetworkManager& operator=(const NetworkManager&) = delete;

        // Accepts incoming connections asynchronously.
        void doAccept();

        // Removes a peer from the peers list upon disconnection.
        void removePeer(const std::string& peerID);

        // Boost.Asio I/O context for network operations.
        boost::asio::io_context ioContext_;

        // TCP acceptor for incoming connections.
        tcp::acceptor acceptor_;

        // Map of peer IDs to their respective Peer objects.
        std::unordered_map<std::string, std::shared_ptr<Peer>> peers_;

        // Mutex to ensure thread-safe access to peers_.
        mutable std::mutex peersMutex_;

        // Stores the server's listening address (IP:port).
        std::string ownAddress_;

        // Callback function for peer disconnection events.
        std::function<void(const std::string&)> peerDisconnectHandler_;
    };

}  // namespace network