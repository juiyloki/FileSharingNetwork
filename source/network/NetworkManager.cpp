#include "network/NetworkManager.h"
#include "log/LogManager.h"
#include "message/Message.h"
#include <iostream>
#include <thread>

namespace network {

    // Returns the singleton instance of NetworkManager.
    NetworkManager& NetworkManager::instance() {
        static NetworkManager instance;
        return instance;
    }

    // Constructs NetworkManager with initialized acceptor and I/O context.
    NetworkManager::NetworkManager() : acceptor_(ioContext_) {}

    // Cleans up by shutting down all connections.
    NetworkManager::~NetworkManager() {
        shutdown();
    }

    // Starts the server to listen for incoming connections on the specified port.
    // Determines the machine's IP by connecting to Google DNS (8.8.8.8).
    void NetworkManager::startServer(unsigned short port) {
        // Set up acceptor.
        boost::system::error_code ec;
        tcp::endpoint endpoint(tcp::v4(), port);
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            std::cerr << "Acceptor open failed: " << ec.message() << "\n";
            return;
        }
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        acceptor_.bind(endpoint, ec);
        if (ec) {
            std::cerr << "Bind failed: " << ec.message() << "\n";
            return;
        }
        acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec) {
            std::cerr << "Listen failed: " << ec.message() << "\n";
            return;
        }

        // Determine local IP address by connecting to Google DNS (8.8.8.8:53).
        // Fallback to "unknown:<port>" if connection fails.
        try {
            boost::asio::ip::tcp::socket tempSocket(ioContext_);
            tempSocket.connect(tcp::endpoint(boost::asio::ip::address::from_string("8.8.8.8"), 53));
            ownAddress_ = tempSocket.local_endpoint().address().to_string() + ":" + std::to_string(port);
            tempSocket.close();
        } catch (...) {
            ownAddress_ = "unknown:" + std::to_string(port);
        }

        // Start accepting connections and run I/O context in a detached thread.
        doAccept();
        std::thread([this]() { ioContext_.run(); }).detach();
    }

    // Connects to a peer at the given IP and port.
    // Skips if already connected to the peer.
    void NetworkManager::connectToPeer(const std::string& ip, unsigned short port) {
        std::string peerAddr = ip + ":" + std::to_string(port);
        {
            std::lock_guard<std::mutex> lock(peersMutex_);
            if (peers_.count(peerAddr)) {
                return;
            }
        }

        auto socket = std::make_shared<tcp::socket>(ioContext_);
        tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);

        socket->async_connect(endpoint, [this, socket, peerAddr](const boost::system::error_code& ec) {
            if (ec) {
                std::cerr << "Failed to connect to " << peerAddr << ": " << ec.message() << "\n";
                return;
            }

            // Create and register peer.
            auto peer = std::make_shared<Peer>(socket, peerAddr);
            {
                std::lock_guard<std::mutex> lock(peersMutex_);
                peers_[peerAddr] = peer;
            }

            // Set up message handler.
            peer->onMessage([this, peer](const std::string& msg) {
                try {
                    message::Message m = message::Message::decode(msg);
                    // Override type to RECEIVED for all incoming messages.
                    // This ensures consistency regardless of sender's encoding.
                    m = message::Message(m.getPeerID(), m.getTopic(), m.getContent(), message::MessageType::RECEIVED);

                    // Update peerID if the sender's listening address differs.
                    {
                        std::lock_guard<std::mutex> lock(peersMutex_);
                        if (m.getPeerID() != peer->getPeerID()) {
                            peers_.erase(peer->getPeerID());
                            peers_[m.getPeerID()] = peer;
                            peer->setPeerID(m.getPeerID());
                        }
                    }
                    logging::LogManager::instance().appendMessage(m);
                    std::cout << "Received from " << m.getPeerID()
                              << " | Topic: " << m.getTopic()
                              << " | Content: " << m.getContent() << "\n";
                } catch (...) {
                    // Ignore parsing errors to prevent crashes from malformed messages.
                }
            });

            // Set up disconnect handler.
            peer->onDisconnect([this, peer]() {
                removePeer(peer->getPeerID());
                std::cout << "Peer disconnected\n";
            });

            peer->startReceiving();
            std::cout << "Connected to peer: " << peerAddr << "\n";
        });
    }

    // Sends a message to a specific peer identified by peerID.
    void NetworkManager::sendMessage(const std::string& peerID, const std::string& message) {
        std::lock_guard<std::mutex> lock(peersMutex_);
        auto it = peers_.find(peerID);
        if (it != peers_.end()) {
            it->second->sendMessage(message);
        }
    }

    // Broadcasts a message to all connected peers.
    void NetworkManager::broadcastMessage(const std::string& message) {
        std::lock_guard<std::mutex> lock(peersMutex_);
        for (auto& [id, peer] : peers_) {
            peer->sendMessage(message);
        }
    }

    // Retrieves the current listening address (IP:port) of the server.
    std::string NetworkManager::getListeningAddress() const {
        return ownAddress_;
    }

    // Registers a callback to handle peer disconnection events.
    void NetworkManager::onPeerDisconnected(std::function<void(const std::string&)> handler) {
        peerDisconnectHandler_ = std::move(handler);
    }

    // Returns a list of connected peers' information.
    std::vector<std::string> NetworkManager::listPeerInfo() const {
        std::vector<std::string> result;
        std::lock_guard<std::mutex> lock(peersMutex_);
        for (const auto& [id, peer] : peers_) {
            if (peer) {
                result.push_back(peer->toString());
            }
        }
        return result;
    }

    // Shuts down the network manager and closes all connections.
    // Sends "disconnecting" message to peers before closing.
    void NetworkManager::shutdown() {
        // Close acceptor, ignoring errors for simplicity.
        boost::system::error_code ec;
        acceptor_.close(ec);

        // Notify and close all peers.
        {
            std::lock_guard<std::mutex> lock(peersMutex_);
            for (auto& [id, peer] : peers_) {
                if (peer->isConnected()) {
                    peer->sendMessage("disconnecting");
                }
            }
            peers_.clear();
        }
        ioContext_.stop();
    }

    // Accepts incoming connections asynchronously.
    // Registers new peers and sets up their message and disconnect handlers.
    void NetworkManager::doAccept() {
        auto socket = std::make_shared<tcp::socket>(ioContext_);
        acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
            if (!ec) {
                // Use temporary key; will be updated by received message.
                std::string tempPeerKey = socket->remote_endpoint().address().to_string() + ":" +
                                          std::to_string(socket->remote_endpoint().port());
                auto peer = std::make_shared<Peer>(socket, tempPeerKey);
                {
                    std::lock_guard<std::mutex> lock(peersMutex_);
                    peers_[tempPeerKey] = peer;
                }

                // Set up message handler.
                peer->onMessage([this, peer](const std::string& msg) {
                    try {
                        message::Message m = message::Message::decode(msg);
                        // Override type to RECEIVED for all incoming messages.
                        // This ensures consistency regardless of sender's encoding.
                        m = message::Message(m.getPeerID(), m.getTopic(), m.getContent(), message::MessageType::RECEIVED);

                        // Update peerID to the sender's listening address.
                        {
                            std::lock_guard<std::mutex> lock(peersMutex_);
                            if (m.getPeerID() != peer->getPeerID()) {
                                peers_.erase(peer->getPeerID());
                                peers_[m.getPeerID()] = peer;
                                peer->setPeerID(m.getPeerID());
                            }
                        }
                        logging::LogManager::instance().appendMessage(m);
                        std::cout << "Received from " << m.getPeerID()
                                  << " | Topic: " << m.getTopic()
                                  << " | Content: " << m.getContent() << "\n";
                    } catch (...) {
                        // Ignore parsing errors to prevent crashes from malformed messages.
                    }
                });

                // Set up disconnect handler.
                peer->onDisconnect([this, peer]() {
                    removePeer(peer->getPeerID());
                    std::cout << "Peer disconnected\n";
                });

                peer->startReceiving();
                std::cout << "Accepted connection from " << tempPeerKey << "\n";
            }
            // Continue accepting connections.
            doAccept();
        });
    }

    // Removes a peer from the peers list upon disconnection.
    // Sends a "disconnecting" message if the peer is still connected.
    void NetworkManager::removePeer(const std::string& peerID) {
        std::lock_guard<std::mutex> lock(peersMutex_);
        auto it = peers_.find(peerID);
        if (it != peers_.end()) {
            // Check connection status before sending message (best-effort).
            if (it->second->isConnected()) {
                boost::system::error_code ignore;
                it->second->sendMessage("disconnecting");
            }
            peers_.erase(it);
            std::cout << "Peer removed: " << peerID << "\n";
            if (peerDisconnectHandler_) {
                peerDisconnectHandler_(peerID);
            }
        }
    }

}  // namespace network