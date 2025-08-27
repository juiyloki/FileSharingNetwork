#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "message/Message.h"

namespace log {

    class LogManager {

    public:

        // Singleton instance
        static LogManager& instance();

        // Append/delete message from log
        void appendMessage(const message::Message& msg);
        void deleteMessage(size_t index, bool sent);

        // Read log into
        std::vector<message::Message> readAll();

        // Return all messages in one vector, ready for UI.
        std::vector<std::string> getMessagesReadable(bool sent) const;

    private:

        // Private constructor
        LogManager();
        ~LogManager();

        // Singleton prequations.
        LogManager(const LogManager&) = delete;
        LogManager& operator=(const LogManager&) = delete;

        // In memory storage of messages.
        std::vector<message::Message> sentMessages_;
        std::vector<message::Message> receivedMessages_;

        // For thread safety.
        std::mutex fileMutex_;

        // Define log files.
        const std::string sentLogFile_ = "logs/messages_sent.log";
        const std::string receivedLogFile_ = "logs/messages_received.log";

        // Ensure log folder is created.
        void ensureLogFolderExists();

        // Save to file
        void saveSentToFile();
        void saveReceivedToFile();

        // Observer for UI notification of new message.
        std::function<void(const message::Message&)> observer_;
        void notifyObserver(const message::Message& msg);
    };

}
