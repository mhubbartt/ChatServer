#pragma once

#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

// Standard C++ Headers
#include <memory>
#include <mutex>
#include <chrono>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>

// Boost Headers
#include <boost/asio.hpp>
#include <boost/beast.hpp>


class ConsoleUI;
class ChatSession;

class ChatServer {
public:
    // Constructor to initialize the ChatServer
    ChatServer(boost::asio::io_context& IoContext, const boost::asio::ip::tcp::endpoint& Endpoint, int TimeoutSeconds);

    // *** Connection Management ***
    void AcceptConnection(); // Asynchronously accept new client connections
    std::vector<std::string> GetActiveConnections(); // Retrieve a list of active client addresses
    int GetTotalConnections() const; // Get the total number of connections handled
    void StartSessionCleaner(boost::asio::io_context& IoContext); // Start periodic session cleanup
    void CleanTimedOutSessions(); // Remove disconnected or timed-out sessions

    // *** System Information ***
    float GetCPUUsage();  // Retrieve the current CPU usage of the server
    size_t GetMemoryUsage(); // Retrieve the current memory usage of the server
    float GetUptime() const; // Get the uptime of the server in seconds
    std::string GetFormattedUptime() const; // Get uptime in the format hh:mm:ss
    void IncrementMessagesSent(); // Increment the count of messages sent
    void IncrementMessagesReceived(); // Increment the count of messages received
    void AddDataSent(size_t bytes); // Add to the total data sent (in bytes)
    void AddDataReceived(size_t bytes); // Add to the total data received (in bytes)
    int GetMessagesSent() const; // Get the total number of sent messages
    int GetMessagesReceived() const; // Get the total number of received messages
    size_t GetDataSent() const; // Get the total amount of data sent
    size_t GetDataReceived() const; // Get the total amount of data received

    // *** Administrative Functions ***
    void BroadcastAllSystemMessage(const std::string& rawMessage); // Broadcast a system message to all clients
    void SendMessageToClient(const std::string& client, const std::string& message); // Send a private message to a specific client
    static std::string FormatChatMessage(const std::string& sender, const std::string& content); // Format a chat message as JSON

    // *** Thread-Safe Operations ***
    std::vector<std::string> GetMessages(); // Retrieve all logged messages
    void AddToMessageLog(const std::string& message); // Add a message to the server log
    std::string GetSessionIP(const std::string& sender); // Retrieve the IP address of a specific session

private:
    // *** Consolidated Mutexes ***
    std::mutex MessageMutex; // Protects MessageLog and UI-related access
    std::mutex SessionMutex; // Protects SessionSet and SessionMap

    // *** Resources ***
    std::unordered_map<std::string, std::shared_ptr<ChatSession>> SessionMap; // Maps sender names to their sessions
    std::unordered_set<std::shared_ptr<ChatSession>> SessionSet; // Tracks active sessions
    std::vector<std::string> MessageLog; // Stores all messages for logging and display

    boost::asio::ip::tcp::acceptor Acceptor; // Handles incoming client connections
    std::shared_ptr<boost::asio::steady_timer> Timer; // Timer for periodic operations
    std::chrono::seconds TimeoutDuration; // Duration for session timeout
    std::chrono::steady_clock::time_point ServerStartTime; // Tracks the server's start time

    // *** Atomic Statistics ***
    std::atomic<int> TotalConnections{0}; // Total number of connections handled
    std::atomic<int> TotalMessagesSent{0}; // Total messages sent
    std::atomic<int> TotalMessagesReceived{0}; // Total messages received
    std::atomic<size_t> TotalDataSent{0}; // Total data sent in bytes
    std::atomic<size_t> TotalDataReceived{0}; // Total data received in bytes
};

#endif // CHAT_SERVER_H
