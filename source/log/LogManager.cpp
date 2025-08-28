#include "log/LogManager.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <cstddef>
#include <iostream>

namespace logging {

    // Singleton instance.
    LogManager& LogManager::instance() {
        static LogManager instance;
        return instance;
    }

    // Private constructor for single instance.
    LogManager::LogManager() {
        ensureLogFolderExists();
        // Load existing messages if files exist
        std::ifstream sentFile(sentLogFile_);
        std::string line;
        while (std::getline(sentFile, line)) {
            try { sentMessages_.emplace_back(message::Message::decode(line)); }
            catch(...) {}
        }
        std::ifstream recvFile(receivedLogFile_);
        while (std::getline(recvFile, line)) {
            try { receivedMessages_.emplace_back(message::Message::decode(line)); }
            catch(...) {}
        }
    }

    // Saves current vectors to files.
    LogManager::~LogManager() {
        saveSentToFile();
        saveReceivedToFile();
    }

    // Append a message to proper vector and save to file.
    void LogManager::appendMessage(const message::Message& msg) {
        std::lock_guard<std::mutex> lock(fileMutex_);
        if (msg.getType() == message::MessageType::SENT)
            sentMessages_.push_back(msg);
        else
            receivedMessages_.push_back(msg);
        saveSentToFile();
        saveReceivedToFile();
    }

    // Delete a message from vector by its index and type (sent/received), then update file.
    void LogManager::deleteMessage(size_t index, bool sent) {
        std::lock_guard<std::mutex> lock(fileMutex_);
        auto& vec = sent ? sentMessages_ : receivedMessages_;
        if (index >= vec.size()) return;  // safety check
        vec.erase(vec.begin() + index);
        if (sent) saveSentToFile();
        else saveReceivedToFile();
    }


    // Return all messages in memory as a single vector.
    std::vector<message::Message> LogManager::readAll() {
        std::lock_guard<std::mutex> lock(fileMutex_);
        std::vector<message::Message> all = sentMessages_;
        all.insert(all.end(), receivedMessages_.begin(), receivedMessages_.end());
        return all;
    }

    // Returns vector of readable strings for sent messages.
    std::vector<std::string> LogManager::getSentStrings() {
        std::lock_guard<std::mutex> lock(fileMutex_);
        std::vector<std::string> result;
        for (auto& msg : sentMessages_) result.push_back(msg.toString());
        return result;
    }

    // Returns vector of readable strings for received messages.
    std::vector<std::string> LogManager::getReceivedStrings() {
        std::lock_guard<std::mutex> lock(fileMutex_);
        std::vector<std::string> result;
        for (auto& msg : receivedMessages_) result.push_back(msg.toString());
        return result;
    }

    // Create logs directory if it does not exist.
    void LogManager::ensureLogFolderExists() {
        std::filesystem::create_directories("logs");
    }

    // Save sent messages vector to file.
    void LogManager::saveSentToFile() {
        std::ofstream file(sentLogFile_, std::ios::trunc);
        for (auto& msg : sentMessages_) file << msg.encode() << "\n";
    }

    // Save received messages vector to file.
    void LogManager::saveReceivedToFile() {
        std::ofstream file(receivedLogFile_, std::ios::trunc);
        for (auto& msg : receivedMessages_) file << msg.encode() << "\n";
    }

}
