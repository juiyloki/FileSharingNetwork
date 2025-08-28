#include "message/Message.h"
#include <sstream>
#include <vector>
#include <iomanip>
#include <ctime>
#include <stdexcept>


namespace message {

    // Constructor
    Message::Message(const std::string& peerID,
                     const std::string& topic,
                     const std::string& content,
                     MessageType type):
        peerID_(peerID), topic_(topic), content_(content),
        type_(type), read_(false),
        timestamp_(std::chrono::system_clock::now()) {}

    // Accessors
    const std::string& Message::getPeerID() const { return peerID_; }
    const std::string& Message::getTopic() const { return topic_; }
    const std::string& Message::getContent() const { return content_; }
    MessageType Message::getType() const { return type_; }
    std::chrono::system_clock::time_point Message::getTimestamp() const { return timestamp_; }
    bool Message::isRead() const { return read_; }

    // Mark as read
    void Message::markRead() { read_ = true; }

    // Encode for storage (single line, compressed)
    std::string Message::encode() const {
        std::ostringstream oss;
        auto timeT = std::chrono::system_clock::to_time_t(timestamp_);
        oss << peerID_ << "|" << static_cast<int>(type_) << "|"
            << read_ << "|" << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S")
            << "|" << topic_ << "|" << content_;
        return oss.str();
    }

    // Decode from log line
    Message Message::decode(const std::string& line) {
        std::istringstream iss(line);
        std::string part;
        std::vector<std::string> tokens;
        while (std::getline(iss, part, '|')) tokens.push_back(part);
        if (tokens.size() < 6) throw std::runtime_error("Malformed message log line");

        MessageType type = static_cast<MessageType>(std::stoi(tokens[1]));
        bool read = (tokens[2] == "1" || tokens[2] == "true");

        std::tm tm = {};
        std::istringstream ssTime(tokens[3]);
        ssTime >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        auto timePoint = std::chrono::system_clock::from_time_t(std::mktime(&tm));

        Message msg(tokens[0], tokens[4], tokens[5], type);
        msg.read_ = read;
        msg.timestamp_ = timePoint;
        return msg;
    }

    // Returns single-line summary for UI
    std::string Message::toString() const {
        auto timeT = std::chrono::system_clock::to_time_t(timestamp_);
        std::ostringstream oss;
        oss << "[" << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S") << "] "
            << "Topic: " << topic_ << " | PeerID: " << peerID_;
        return oss.str();
    }

}