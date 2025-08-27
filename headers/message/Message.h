#pragma once

#include <string>
#include <chrono>

namespace message {

    // Divide messages into two types.
    enum class MessageType { SENT, RECEIVED };

    class Message {

    public:

        // Constructor.
        Message(const std::string& peerID,
                const std::string& topic,
                const std::string& content,
                MessageType type);

        // Accessors.
        const std::string& getPeerID() const;
        const std::string& getTopic() const;
        const std::string& getContent() const;
        MessageType getType() const;
        std::chrono::system_clock::time_point getTimestamp() const;
        bool isRead() const;

        // Mutators.
        void markRead();

        // Logging.
        std::string encode() const;
        static Message decode(const std::string& line);

        // UI.
        std::string toString() const;

    private:

        // Attributes
        std::string peerID_;
        std::string topic_;
        std::string content_;
        MessageType type_;
        bool read_;
        std::chrono::system_clock::time_point timestamp_;

    };

}
