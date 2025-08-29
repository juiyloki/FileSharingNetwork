#pragma once

#include <chrono>
#include <string>

namespace message {

    enum class MessageType { SENT, RECEIVED };

    class Message {
    public:
        // Constructs a Message with peer ID, topic, content, and type.
        Message(const std::string& peerID,
            const std::string& topic,
            const std::string& content,
            MessageType type);

        // Returns the peer ID associated with the message.
        const std::string& getPeerID() const;

        // Returns the topic of the message.
        const std::string& getTopic() const;

        // Returns the content of the message.
        const std::string& getContent() const;

        // Returns the type of the message (SENT or RECEIVED).
        MessageType getType() const;

        // Returns the timestamp when the message was created.
        std::chrono::system_clock::time_point getTimestamp() const;

        // Checks if the message has been read.
        bool isRead() const;

        // Marks the message as read.
        void markRead();

        // Encodes the message into a string for logging.
        std::string encode() const;

        // Decodes a string into a Message object.
        static Message decode(const std::string& line);

        // Returns a string representation of the message for UI display.
        std::string toString() const;

    private:
        // Unique identifier of the peer who sent or received the message.
        std::string peerID_;

        // Topic or subject of the message.
        std::string topic_;

        // Content of the message.
        std::string content_;

        // Type of the message (SENT or RECEIVED).
        MessageType type_;

        // Indicates whether the message has been read.
        bool read_;

        // Timestamp of when the message was created.
        std::chrono::system_clock::time_point timestamp_;
    };

}  // namespace message