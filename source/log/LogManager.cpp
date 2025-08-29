#include "log/LogManager.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace logging {

    // Returns the singleton instance of LogManager.
    LogManager& LogManager::instance() {
        static LogManager instance;
        return instance;
    }

    // Constructs LogManager and loads existing messages from log files.
    // Ignores parsing errors to ensure startup robustness.
    LogManager::LogManager() {
        ensureLogFolderExists();
        // Load sent messages.
        std::ifstream sentFile(sentLogFile_);
        std::string line;
        while (std::getline(sentFile, line)) {
            try {
                sentMessages_.emplace_back(message::Message::decode(line));
            } catch (...) {
                // Ignore errors to handle malformed log entries.
            }
        }
        // Load received messages.
        std::ifstream recvFile(receivedLogFile_);
        while (std::getline(recvFile, line)) {
            try {
                receivedMessages_.emplace_back(message::Message::decode(line));
            } catch (...) {
                // Ignore errors to handle malformed log entries.
            }
        }
    }

    // Saves all messages to log files on destruction.
    LogManager::~LogManager() {
        saveSentToFile();
        saveReceivedToFile();
    }

    // Appends a message to the appropriate log (sent or received) and saves to file.
    // Saves both log files for simplicity, even if only one is modified.
    void LogManager::appendMessage(const message::Message& msg) {
        std::lock_guard<std::mutex> lock(fileMutex_);
        if (msg.getType() == message::MessageType::SENT) {
            sentMessages_.push_back(msg);
        } else {
            receivedMessages_.push_back(msg);
        }
        saveSentToFile();
        saveReceivedToFile();
    }

    // Deletes a message at the specified index from either sent or received log.
    // Updates the corresponding log file.
    void LogManager::deleteMessage(size_t index, bool sent) {
        std::lock_guard<std::mutex> lock(fileMutex_);
        auto& vec = sent ? sentMessages_ : receivedMessages_;
        if (index >= vec.size()) {
            return;
        }
        vec.erase(vec.begin() + index);
        if (sent) {
            saveSentToFile();
        } else {
            saveReceivedToFile();
        }
    }

    // Retrieves all messages (sent and received) as a single vector.
    std::vector<message::Message> LogManager::readAll() {
        std::lock_guard<std::mutex> lock(fileMutex_);
        std::vector<message::Message> all = sentMessages_;
        all.insert(all.end(), receivedMessages_.begin(), receivedMessages_.end());
        return all;
    }

    // Returns a vector of strings representing sent messages.
    std::vector<std::string> LogManager::getSentStrings() {
        std::lock_guard<std::mutex> lock(fileMutex_);
        std::vector<std::string> result;
        for (auto& msg : sentMessages_) {
            result.push_back(msg.toString());
        }
        return result;
    }

    // Returns a vector of strings representing received messages.
    std::vector<std::string> LogManager::getReceivedStrings() {
        std::lock_guard<std::mutex> lock(fileMutex_);
        std::vector<std::string> result;
        for (auto& msg : receivedMessages_) {
            result.push_back(msg.toString());
        }
        return result;
    }

    // Ensures the log directory exists before file operations.
    void LogManager::ensureLogFolderExists() {
        std::filesystem::create_directories("logs");
    }

    // Saves sent messages to the log file, overwriting existing content.
    void LogManager::saveSentToFile() {
        std::ofstream file(sentLogFile_, std::ios::trunc);
        for (auto& msg : sentMessages_) {
            file << msg.encode() << "\n";
        }
    }

    // Saves received messages to the log file, overwriting existing content.
    void LogManager::saveReceivedToFile() {
        std::ofstream file(receivedLogFile_, std::ios::trunc);
        for (auto& msg : receivedMessages_) {
            file << msg.encode() << "\n";
        }
    }

    // Notifies the observer of a new message.
    // Currently unused, reserved for future UI integration.
    void LogManager::notifyObserver(const message::Message& msg) {
        if (observer_) {
            observer_(msg);
        }
    }

}  // namespace logging