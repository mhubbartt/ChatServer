#pragma once

#ifndef CHAT_SESSION_H
#define CHAT_SESSION_H


// Boost Headers
#include <boost/asio.hpp>
#include <boost/beast.hpp>

// Standard C++ Headers
#include <memory>
#include <string>
#include <mutex>
#include <unordered_set>
#include <queue>

class ChatServer;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;

/**
 * @brief Represents a single WebSocket session.
 */
class ChatSession : public std::enable_shared_from_this<ChatSession> {
public:
    ChatSession(tcp::socket Socket,ChatServer* Server, std::unordered_set<std::shared_ptr<ChatSession>>& SessionSet, std::mutex& SessionMutex);
    
    /**
     * @brief Starts the WebSocket session.
     */
    void Start();

    /**
     * @brief Checks if the session has timed out.
     * @param CurrentTime The current time point.
     * @param TimeoutDuration The timeout duration.
     * @return True if the session is timed out; otherwise, false.
     */
    bool IsTimedOut(std::chrono::steady_clock::time_point CurrentTime, std::chrono::seconds TimeoutDuration) const;

    /**
     * @brief Retrieves the client's IP address as a string.
     * @return The client's IP address.
     */
    std::string GetClientAddress() const;

    /**
     * @brief Disconnects the session.
     */
    void Disconnect();

    bool IsConnected() const;
    void Cleanup();
    std::string GetClientID() const;
    std::string GetIPAddress();
    void AddMessage(const std::string& Message);

private:
    void ReadMessage();

    void ReceiveMessage(const std::string& message);
    void DoWrite();
    
    boost::asio::ip::tcp::endpoint RemoteEndpoint; // Stores the remote connection endpoint
    ChatServer* Server;
    websocket::stream<tcp::socket> WebSocketStream;
    beast::multi_buffer Buffer;
    std::unordered_set<std::shared_ptr<ChatSession>>& SessionSet;
    std::mutex& SessionMutex;
    std::queue<std::string> WriteQueue;
    std::chrono::steady_clock::time_point LastActivity;
    bool IsWriting = false;
};

#endif // CHAT_SESSION_H
