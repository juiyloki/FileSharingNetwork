#include "message/Message.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace message {

    // Constructs a Message with peer ID, topic, content, and type.
    // Initializes read_ as false (unused, reserved for future).
    Message::Message(const std::string& peerID,
                    const std::string& topic,
                    const std::string& content,
                    MessageType type)
        : peerID_(peerID),
          topic_(topic),
          content_(content),
          type_(type),
          read_(false),
          timestamp_(std::chrono::system_clock::now()) {}

    // Returns the peer ID associated with the message.
    const std::string& Message::getPeerID() const {
        return peerID_;
    }

    // Returns the topic of the message.
    const std::string& Message::getTopic() const {
        return topic_;
    }

    // Returns the content of the message.
    const std::string& Message::getContent() const {
        return content_;
    }

    // Returns the type of the message (SENT or RECEIVED).
    MessageType Message::getType() const {
        return type_;
    }

    // Returns the timestamp when the message was created.
    std::chrono::system_clock::time_point Message::getTimestamp() const {
        return timestamp_;
    }

    // Checks if the message has been read (unused, reserved for future).
    bool Message::isRead() const {
        return read_;
    }

    // Marks the message as read (unused, reserved for future).
    void Message::markRead() {
        read_ = true;
    }

    // Encodes the message into a single line for logging.
    std::string Message::encode() const {
        std::ostringstream oss;
        auto timeT = std::chrono::system_clock::to_time_t(timestamp_);
        oss << peerID_ << "|" << static_cast<int>(type_) << "|"
            << read_ << "|" << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S")
            << "|" << topic_ << "|" << content_;
        return oss.str();
    }

    // Decodes a string into a Message object.
    // Assumes well-formed input with minimal validation.
    Message Message::decode(const std::string& line) {
        std::istringstream iss(line);
        std::string part;
        std::vector<std::string> tokens;
        while (std::getline(iss, part, '|')) {
            tokens.push_back(part);
        }
        if (tokens.size() < 6) {
            throw std::runtime_error("Malformed message log line");
        }

        MessageType type = static_cast<MessageType>(std::stoi(tokens[1]));
        bool read = (tokens[2] == "1" || tokens[2] == "true");

        // Parse timestamp using local time (may lose precision).
        std::tm tm = {};
        std::istringstream ssTime(tokens[3]);
        ssTime >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        auto timePoint = std::chrono::system_clock::from_time_t(std::mktime(&tm));

        Message msg(tokens[0], tokens[4], tokens[5], type);
        msg.read_ = read;
        msg.timestamp_ = timePoint;
        return msg;
    }

    // Returns a single-line summary of the message for UI display.
    std::string Message::toString() const {
        auto timeT = std::chrono::system_clock::to_time_t(timestamp_);
        std::ostringstream oss;
        oss << "[" << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S") << "] "
            << "Topic: " << topic_ << " | PeerID: " << peerID_;
        return oss.str();
    }

}  // namespace message