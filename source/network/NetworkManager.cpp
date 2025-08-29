#include "network/NetworkManager.h"
#include "message/Message.h"
#include <iostream>
#include <thread>
#include "log/LogManager.h"

namespace network {

    NetworkManager& NetworkManager::instance() {
        static NetworkManager instance;
        return instance;
    }

    NetworkManager::NetworkManager() : acceptor_(ioContext_) {}

    NetworkManager::~NetworkManager() {
        shutdown();
    }

    std::string NetworkManager::getListeningAddress() const {
        return ownAddress_;
    }

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

        // Determine the actual machine IP
        try {
            boost::asio::ip::tcp::socket tempSocket(ioContext_);
            tempSocket.connect(tcp::endpoint(boost::asio::ip::address::from_string("8.8.8.8"), 53));
            ownAddress_ = tempSocket.local_endpoint().address().to_string() + ":" + std::to_string(port);
            tempSocket.close();
        } catch (...) {
            ownAddress_ = "unknown:" + std::to_string(port);
        }

        doAccept();
        std::thread([this]() { ioContext_.run(); }).detach();
    }

    void NetworkManager::doAccept() {
        auto socket = std::make_shared<tcp::socket>(ioContext_);
        acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
            if (!ec) {
                // Use temporary key; will be updated by received message
                std::string tempPeerKey = socket->remote_endpoint().address().to_string() + ":" +
                                          std::to_string(socket->remote_endpoint().port());
                auto peer = std::make_shared<Peer>(socket, tempPeerKey);
                {
                    std::lock_guard<std::mutex> lock(peersMutex_);
                    peers_[tempPeerKey] = peer;
                }

                peer->onMessage([this, peer](const std::string& msg) {
                    try {
                        message::Message m = message::Message::decode(msg);
                        // Update peerID to the sender's listening address
                        {
                            std::lock_guard<std::mutex> lock(peersMutex_);
                            if (m.getPeerID() != peer->getPeerID()) {
                                peers_.erase(peer->getPeerID());
                                peers_[m.getPeerID()] = peer;
                                peer->setPeerID(m.getPeerID()); // Use setter
                            }
                        }
                        logging::LogManager::instance().appendMessage(m);
                        std::cout << "Received from " << m.getPeerID()
                                  << " | Topic: " << m.getTopic()
                                  << " | Content: " << m.getContent() << "\n";
                    } catch (...) {}
                });

                peer->onDisconnect([this, peer]() {
                    removePeer(peer->getPeerID());
                    std::cout << "Peer disconnected\n";
                });

                peer->startReceiving();
                std::cout << "Accepted connection from " << tempPeerKey << "\n";
            }
            doAccept();
        });
    }

    void NetworkManager::connectToPeer(const std::string& ip, unsigned short port) {
        std::string peerAddr = ip + ":" + std::to_string(port);
        {
            std::lock_guard<std::mutex> lock(peersMutex_);
            if (peers_.count(peerAddr)) return;
        }

        auto socket = std::make_shared<tcp::socket>(ioContext_);
        tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);

        socket->async_connect(endpoint, [this, socket, peerAddr](const boost::system::error_code& ec) {
            if (ec) {
                std::cerr << "Failed to connect to " << peerAddr << ": " << ec.message() << "\n";
                return;
            }

            auto peer = std::make_shared<Peer>(socket, peerAddr);
            {
                std::lock_guard<std::mutex> lock(peersMutex_);
                peers_[peerAddr] = peer;
            }

            peer->onMessage([this, peer](const std::string& msg) {
                try {
                    message::Message m = message::Message::decode(msg);
                    // Update peerID if needed
                    {
                        std::lock_guard<std::mutex> lock(peersMutex_);
                        if (m.getPeerID() != peer->getPeerID()) {
                            peers_.erase(peer->getPeerID());
                            peers_[m.getPeerID()] = peer;
                            peer->setPeerID(m.getPeerID()); // Use setter
                        }
                    }
                    logging::LogManager::instance().appendMessage(m);
                    std::cout << "Received from " << m.getPeerID()
                              << " | Topic: " << m.getTopic()
                              << " | Content: " << m.getContent() << "\n";
                } catch (...) {}
            });

            peer->onDisconnect([this, peer]() {
                removePeer(peer->getPeerID());
                std::cout << "Peer disconnected\n";
            });

            peer->startReceiving();
            std::cout << "Connected to peer: " << peerAddr << "\n";
        });
    }

    void NetworkManager::sendMessage(const std::string& peerID, const std::string& message) {
        std::lock_guard<std::mutex> lock(peersMutex_);
        auto it = peers_.find(peerID);
        if (it != peers_.end()) it->second->sendMessage(message);
    }

    void NetworkManager::broadcastMessage(const std::string& message) {
        std::lock_guard<std::mutex> lock(peersMutex_);
        for (auto& [id, peer] : peers_) {
            peer->sendMessage(message);
        }
    }

    void NetworkManager::onPeerDisconnected(std::function<void(const std::string&)> handler) {
        peerDisconnectHandler_ = std::move(handler);
    }

    void NetworkManager::removePeer(const std::string& peerID) {
        std::lock_guard<std::mutex> lock(peersMutex_);
        auto it = peers_.find(peerID);
        if (it != peers_.end()) {
            if (it->second->isConnected()) {
                boost::system::error_code ignore;
                it->second->sendMessage("disconnecting");
            }
            peers_.erase(it);
            std::cout << "Peer removed: " << peerID << "\n";
            if (peerDisconnectHandler_) peerDisconnectHandler_(peerID);
        }
    }

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

    std::vector<std::string> NetworkManager::listPeerInfo() const {
        std::vector<std::string> result;
        std::lock_guard<std::mutex> lock(peersMutex_);
        for (const auto& [id, peer] : peers_) {
            if (peer) result.push_back(peer->toString());
        }
        return result;
    }

}