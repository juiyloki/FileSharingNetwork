#include "network/NetworkManager.h"
#include "network/Peer.h"
#include <iostream>

namespace network {

NetworkManager& NetworkManager::instance() {
    static NetworkManager instance;
    return instance;
}

NetworkManager::NetworkManager()
    : acceptor_(ioContext_) {}

NetworkManager::~NetworkManager() {
    shutdown();
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

    doAccept();

    // Run in background thread
    std::thread([this]() { ioContext_.run(); }).detach();
}

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

std::vector<std::string> NetworkManager::listPeers() const {
    std::lock_guard<std::mutex> lock(peersMutex_);
    std::vector<std::string> ids;
    ids.reserve(peers_.size());
    for (auto& [id, _] : peers_) ids.push_back(id);
    return ids;
}

void NetworkManager::removePeer(const std::string& peerID) {
    std::lock_guard<std::mutex> lock(peersMutex_);
    peers_.erase(peerID);
    std::cout << "Peer removed: " << peerID << "\n";
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

} // namespace network
