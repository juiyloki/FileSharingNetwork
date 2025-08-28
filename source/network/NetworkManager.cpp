#include "network/NetworkManager.h"
#include "network/Peer.h"
#include <iostream>
#include <thread>


namespace network {

    // Singleton instance.
    NetworkManager& NetworkManager::instance() {
        static NetworkManager instance;
        return instance;
    }

    // No copy/move.
    NetworkManager::NetworkManager()
        : acceptor_(ioContext_) {}

    NetworkManager::~NetworkManager() {
        shutdown();
    }

    // Start a local server.
    void NetworkManager::startServer(unsigned short port) {
        boost::system::error_code ec;
        tcp::endpoint endpoint(tcp::v4(), port);
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) { std::cerr << "Acceptor open failed: " << ec.message() << "\n"; return; }
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        acceptor_.bind(endpoint, ec);
        if (ec) { std::cerr << "Bind failed: " << ec.message() << "\n"; return; }
        acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec) { std::cerr << "Listen failed: " << ec.message() << "\n"; return; }

        doAccept();

        // Run in background thread
        std::thread([this]() { ioContext_.run(); }).detach();
    }

    // Accept new connection.
    void NetworkManager::doAccept() {
        auto socket = std::make_shared<tcp::socket>(ioContext_);
        acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
            if (!ec) {
                auto peerID = socket->remote_endpoint().address().to_string() + ":" +
                              std::to_string(socket->remote_endpoint().port());
                auto peer = std::make_shared<Peer>(socket, peerID);
                {
                    std::lock_guard<std::mutex> lock(peersMutex_);
                    peers_[peerID] = peer;
                }
                peer->onDisconnect([this, peerID]() { removePeer(peerID); });
                peer->startReceiving();
                std::cout << "New peer connected: " << peerID << "\n";
            }
            doAccept(); // keep accepting
        });
    }

    // Connect to peer by ID.
    void NetworkManager::connectToPeer(const std::string& host, unsigned short port) {
        auto socket = std::make_shared<tcp::socket>(ioContext_);
        tcp::resolver resolver(ioContext_);
        auto endpoints = resolver.resolve(host, std::to_string(port));
        boost::asio::async_connect(*socket, endpoints,
            [this, socket](const boost::system::error_code& ec, const tcp::endpoint& ep) {
                if (!ec) {
                    std::string peerID = ep.address().to_string() + ":" + std::to_string(ep.port());
                    auto peer = std::make_shared<Peer>(socket, peerID);
                    {
                        std::lock_guard<std::mutex> lock(peersMutex_);
                        peers_[peerID] = peer;
                    }
                    peer->onDisconnect([this, peerID]() { removePeer(peerID); });
                    peer->startReceiving();
                    std::cout << "Connected to peer: " << peerID << "\n";
                } else {
                    std::cerr << "Connect failed: " << ec.message() << "\n";
                }
            });
    }

    // Message sending.
    void NetworkManager::sendMessage(const std::string& peerID, const std::string& message) {
        std::lock_guard<std::mutex> lock(peersMutex_);
        auto it = peers_.find(peerID);
        if (it != peers_.end()) it->second->sendMessage(message);
    }

    // Message broadcasting.
    void NetworkManager::broadcastMessage(const std::string& message) {
        std::lock_guard<std::mutex> lock(peersMutex_);
        for (auto& [id, peer] : peers_) {
            peer->sendMessage(message);
        }
    }

    // Peer disconnect handler.
    void NetworkManager::onPeerDisconnected(std::function<void(const std::string&)> handler) {
        peerDisconnectHandler_ = std::move(handler);
    }

    // Safe peer removal.
    void NetworkManager::removePeer(const std::string& peerID) {
        std::lock_guard<std::mutex> lock(peersMutex_);
        auto it = peers_.find(peerID);
        if (it != peers_.end()) {
            peers_.erase(it);
            std::cout << "Peer removed: " << peerID << std::endl;

            if (peerDisconnectHandler_) {
                peerDisconnectHandler_(peerID);
            }
        }
    }

    // Safe shutdown.
    void NetworkManager::shutdown() {
        boost::system::error_code ec;
        acceptor_.close(ec);
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

    // Create a vector of all peers' string code for UI.
    std::vector<std::string> NetworkManager::listPeerInfo() const {
        std::vector<std::string> result;
        for (const auto& [id, peer] : peers_) {
            if (peer) {
                result.push_back(peer->toString());
            }
        }
        return result;
    }

}
