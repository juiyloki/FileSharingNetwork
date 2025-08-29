#include "network/NetworkManager.h"
#include "message/Message.h"
#include "ui/UI.h"
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

    //Change: Return private PeerID
    std::string NetworkManager::getOwnPeerID() const {
        return ownPeerID_;
    }

    //Change: Return public listening address
    std::string NetworkManager::getListeningAddress() const {
        return ownAddress_;
    }



    //CHANGE: Compute local machine IP dynamically instead of showing 127.0.0.1.
    //CHANGE: This allows the user to share an IP reachable by other machines in the LAN.
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

        // Change: Generate unique PeerID is no longer needed, we rely on IP:port
        // ownPeerID_ = "peer_" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));

        // Change: Determine the actual machine IP for display
        try {
            boost::asio::ip::tcp::socket tempSocket(ioContext_);
            tempSocket.connect(tcp::endpoint(boost::asio::ip::address::from_string("8.8.8.8"), 53));
            ownAddress_ = tempSocket.local_endpoint().address().to_string() + ":" + std::to_string(port);
            tempSocket.close();
        } catch (...) {
            ownAddress_ = "unknown:" + std::to_string(port);
        }

        doAccept();

        // Run in background thread
        std::thread([this]() { ioContext_.run(); }).detach();
    }

    // Helper to split "ip:port" string
std::pair<std::string, unsigned short> NetworkManager::splitAddress(const std::string& addr) const {
    auto pos = addr.find(':');
    if (pos == std::string::npos) {
        // No port provided, use default 5555
        return {addr, 5555};
    }
    std::string ip = addr.substr(0, pos);
    unsigned short port = static_cast<unsigned short>(std::stoi(addr.substr(pos + 1)));
    return {ip, port};
}




// Accept new connections
void NetworkManager::doAccept() {
    auto socket = std::make_shared<tcp::socket>(ioContext_);
    acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
        if (!ec) {
            std::string peerKey = socket->remote_endpoint().address().to_string() + ":" +
                                  std::to_string(socket->remote_endpoint().port());

            auto peer = std::make_shared<Peer>(socket, peerKey);

            {
                std::lock_guard<std::mutex> lock(peersMutex_);
                peers_[peerKey] = peer;
            }

            peer->onMessage([this, peer](const std::string& msg) {
                if (msg.rfind("HANDSHAKE:", 0) == 0) {
                    std::string otherAddr = msg.substr(10);

                    std::lock_guard<std::mutex> lock(peersMutex_);
                    if (peers_.find(otherAddr) == peers_.end()) {
                        auto [ip, port] = splitAddress(otherAddr);
                        connectToPeer(ip, port); // Bi-directional: connect back
                    }
                    return;
                }

                // Normal message
                try {
                    message::Message m = message::Message::decode(msg);
                    logging::LogManager::instance().appendMessage(m);
                    std::cout << "Received from " << peer->getPeerID()
                              << " | Topic: " << m.getTopic()
                              << " | Content: " << m.getContent() << "\n";
                } catch (...) {}
            });

            peer->onDisconnect([this, peer]() {
                std::lock_guard<std::mutex> lock(peersMutex_);
                for (auto it = peers_.begin(); it != peers_.end(); ++it) {
                    if (it->second == peer) {
                        peers_.erase(it);
                        break;
                    }
                }
                std::cout << "Peer disconnected\n";
            });

            peer->startReceiving();

            peer->sendMessage("HANDSHAKE:" + getListeningAddress());

            std::cout << "Accepted connection from " << peerKey << "\n";
        }

        doAccept(); // continue accepting
    });
}

// Connect to a peer
void NetworkManager::connectToPeer(const std::string& ip, unsigned short port) {
    std::string peerAddr = ip + ":" + std::to_string(port);

    {
        std::lock_guard<std::mutex> lock(peersMutex_);
        if (peers_.count(peerAddr)) return; // already connected
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
            if (msg.rfind("HANDSHAKE:", 0) == 0) {
                std::string otherAddr = msg.substr(10);

                std::lock_guard<std::mutex> lock(peersMutex_);
                if (peers_.find(otherAddr) == peers_.end()) {
                    auto [ip, port] = splitAddress(otherAddr);
                    connectToPeer(ip, port); // connect back if not already
                }
                return;
            }

            try {
                message::Message m = message::Message::decode(msg);
                logging::LogManager::instance().appendMessage(m);
                std::cout << "Received from " << peer->getPeerID()
                          << " | Topic: " << m.getTopic()
                          << " | Content: " << m.getContent() << "\n";
            } catch (...) {}
        });

        peer->onDisconnect([this, peer]() {
            std::lock_guard<std::mutex> lock(peersMutex_);
            for (auto it = peers_.begin(); it != peers_.end(); ++it) {
                if (it->second == peer) {
                    peers_.erase(it);
                    break;
                }
            }
            std::cout << "Peer disconnected\n";
        });

        peer->startReceiving();

        peer->sendMessage("HANDSHAKE:" + getListeningAddress());

        std::cout << "Connected to peer: " << peerAddr << "\n";
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

    // Remove peer safely from the map.
    void NetworkManager::removePeer(const std::string& peerID) {

        //Change: Added lock for thread safety.
        std::lock_guard<std::mutex> lock(peersMutex_);

        auto it = peers_.find(peerID);

        //Change: Only erase if exists.
        if (it != peers_.end()) {
            //Change: Close socket before removal.
            if (it->second->isConnected()) {
                boost::system::error_code ignore;
                it->second->sendMessage("disconnecting");
            }

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
