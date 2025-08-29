#pragma once

#include "message/Message.h"
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace logging {

    class LogManager {
    public:
        // Returns the singleton instance of LogManager.
        static LogManager& instance();

        // Appends a message to the appropriate log (sent or received).
        void appendMessage(const message::Message& msg);

        // Deletes a message at the specified index from either sent or received log.
        void deleteMessage(size_t index, bool sent);

        // Retrieves all messages (sent and received).
        std::vector<message::Message> readAll();

        // Returns a vector of strings representing sent messages.
        std::vector<std::string> getSentStrings();

        // Returns a vector of strings representing received messages.
        std::vector<std::string> getReceivedStrings();

    private:
        // Private constructor to enforce singleton pattern.
        LogManager();

        // Destructor to clean up resources.
        ~LogManager();

        // Deleted copy constructor and assignment operator to prevent copying.
        LogManager(const LogManager&) = delete;
        LogManager& operator=(const LogManager&) = delete;

        // Ensures the log directory exists.
        void ensureLogFolderExists();

        // Saves sent messages to the log file.
        void saveSentToFile();

        // Saves received messages to the log file.
        void saveReceivedToFile();

        // Notifies the observer of a new message.
        void notifyObserver(const message::Message& msg);

        // In-memory storage for sent messages.
        std::vector<message::Message> sentMessages_;

        // In-memory storage for received messages.
        std::vector<message::Message> receivedMessages_;

        // Mutex for thread-safe file operations.
        std::mutex fileMutex_;

        // File paths for sent and received message logs.
        const std::string sentLogFile_ = "logs/messages_sent.log";
        const std::string receivedLogFile_ = "logs/messages_received.log";

        // Callback for notifying UI of new messages.
        std::function<void(const message::Message&)> observer_;
    };

}  // namespace logging