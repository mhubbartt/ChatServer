
// Standard C++ Headers
#include <vector>             // For std::vector
#include <string>             // For std::string
#include <mutex>              // For std::mutex
#include <chrono>             // For std::chrono
#include <memory>             // For std::shared_ptr

// Third-Party Library Headers
#include <spdlog/spdlog.h>                          // Spdlog logging
#include <boost/asio.hpp>                           // Boost Asio
#include <boost/beast.hpp>                          // Boost Beast
#include <json/include/nlohmann/json.hpp>           // JSON for Modern C++

// Project-Specific Headers
#include "ChatServer.h"         // ChatServer class
#include "ChatSession.h"        // ChatSession class

using tcp = boost::asio::ip::tcp;

// Constructor
ChatServer::ChatServer(boost::asio::io_context& IoContext, const boost::asio::ip::tcp::endpoint& Endpoint,
    int TimeoutSeconds)
    : Acceptor(IoContext, Endpoint), TimeoutDuration(TimeoutSeconds) {
    ServerStartTime = std::chrono::steady_clock::now(); // Initialize server start time
    spdlog::info("Chat server initialized.");
    AcceptConnection(); // Begin accepting client connections
    StartSessionCleaner(IoContext); // Start session cleanup timer
}

// *** Connection Management ***

std::vector<std::string> ChatServer::GetActiveConnections() {
    // Thread-safe access to session data
    std::lock_guard<std::mutex> lock(SessionMutex);
    std::vector<std::string> connections;
    for (const auto& session : SessionSet) {
        try {
            connections.push_back(session->GetClientAddress());
        } catch (const std::exception& e) {
            spdlog::warn("Failed to retrieve client details: {}", e.what());
        }
    }
    return connections;
}

void ChatServer::AcceptConnection() {
    spdlog::info("Waiting for new connection...");
    Acceptor.async_accept([this](boost::beast::error_code errorCode, tcp::socket socket) {
        if (!errorCode) {
            spdlog::info("New client connected: {}", socket.remote_endpoint().address().to_string());
            TotalConnections++; // Increment total connections count

            // Create and start a new session
            auto session = std::make_shared<ChatSession>(std::move(socket), this, SessionSet, SessionMutex);
            session->Start();
        } else {
            spdlog::error("Failed to accept connection: {}", errorCode.message());
        }
        AcceptConnection(); // Continue accepting connections
    });
}

void ChatServer::StartSessionCleaner(boost::asio::io_context& IoContext) {
    Timer = std::make_shared<boost::asio::steady_timer>(IoContext, std::chrono::seconds(30));
    Timer->async_wait([this, &IoContext](boost::beast::error_code) {
        spdlog::info("Cleaning up timed-out sessions...");
        CleanTimedOutSessions();
        StartSessionCleaner(IoContext); // Restart the timer
    });
}

void ChatServer::CleanTimedOutSessions() {
    std::vector<std::shared_ptr<ChatSession>> disconnectedSessions;

    {
        // Lock the session mutex to identify disconnected sessions
        std::lock_guard<std::mutex> lock(SessionMutex);
        for (auto it = SessionSet.begin(); it != SessionSet.end();) {
            if (!(*it)->IsConnected()) {
                disconnectedSessions.push_back(*it); // Collect sessions to remove
                it = SessionSet.erase(it); // Remove from active sessions
            } else {
                ++it;
            }
        }
    }

    // Cleanup disconnected sessions outside the lock
    for (const auto& session : disconnectedSessions) {
        try {
            session->Disconnect();
            spdlog::info("Cleaned up disconnected session.");
        } catch (const std::exception& e) {
            spdlog::error("Error during session cleanup: {}", e.what());
        }
    }
}

// *** Administrative Functions ***

void ChatServer::BroadcastAllSystemMessage(const std::string& rawMessage) {
    // Parse the incoming JSON message
    nlohmann::json parsedMessage;
    try {
        parsedMessage = nlohmann::json::parse(rawMessage);
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse incoming message: {}", e.what());
        return;
    }

    // Extract fields for broadcasting
    std::string timestamp = parsedMessage.value("timestamp", "Unknown Time");
    std::string sender = parsedMessage.value("sender", "Unknown Sender");
    std::string content = parsedMessage.value("content", "Unknown Content");

    // Log the message with IP address
    nlohmann::json logMessage = {
        {"timestamp", timestamp},
        {"sender", sender},
        {"content", content},
        {"ip", GetSessionIP(sender)}
    };

    {
        std::lock_guard<std::mutex> lock(MessageMutex); // Consolidated mutex for messages
        MessageLog.push_back(logMessage.dump());
    }

    // Prepare JSON for broadcasting (excluding IP)
    nlohmann::json broadcastMessage = {
        {"timestamp", timestamp},
        {"sender", sender},
        {"content", content}
    };

    // Broadcast to all sessions
    std::lock_guard<std::mutex> lock(SessionMutex);
    for (const auto& session : SessionSet) {
        session->AddMessage(broadcastMessage.dump());
    }

    spdlog::info("Broadcasted message: {}", broadcastMessage.dump());
}

// *** Thread-Safe Operations ***

std::vector<std::string> ChatServer::GetMessages() {
    std::lock_guard<std::mutex> lock(MessageMutex); // Consolidated mutex
    return MessageLog; // Return a copy of the log
}

void ChatServer::AddToMessageLog(const std::string& message) {
    nlohmann::json parsedMessage;
    try {
        parsedMessage = nlohmann::json::parse(message);
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse message for logging: {}", e.what());
        return;
    }

    std::string sender = parsedMessage.value("sender", "Unknown Sender");
    parsedMessage["ip"] = GetSessionIP(sender); // Dynamically fetch and add IP address

    std::string logMessage = parsedMessage.dump();

    {
        std::lock_guard<std::mutex> lock(MessageMutex); // Consolidated mutex
        MessageLog.push_back(logMessage);
    }

    spdlog::info("Added message to log: {}", logMessage);
}

std::string ChatServer::GetSessionIP(const std::string& sender) {
    std::lock_guard<std::mutex> lock(SessionMutex); // Consolidated mutex for sessions
    auto it = SessionMap.find(sender);
    if (it != SessionMap.end() && it->second) {
        return it->second->GetIPAddress(); // Retrieve IP from session
    }
    return "Unknown IP";
}
std::string ChatServer::FormatChatMessage(const std::string& sender, const std::string& content) {
    nlohmann::json messageJson; // Using nlohmann::json library
    messageJson["sender"] = sender;
    messageJson["content"] = content;
    messageJson["timestamp"] = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Add timestamp
    return messageJson.dump(); // Serialize JSON to string
}
void ChatServer::SendMessageToClient(const std::string& client, const std::string& message) {
    std::lock_guard<std::mutex> lock(SessionMutex); // Ensure thread-safe access to sessions
    std::string formattedMessage = FormatChatMessage("Server", message); // Use the helper to format as JSON

    for (const auto& session : SessionSet) {
        if (session->GetClientID() == client) { // Check if the client matches
            session->AddMessage(formattedMessage); // Add the message to the session's queue
            spdlog::info("Sent message to {}: {}", client, formattedMessage);
            return;
        }
    }
    spdlog::warn("Client {} not found for private message", client);
}
