// Standard C++ Headers
#include <mutex>              // For std::mutex
#include <string>             // For std::string
#include <unordered_set>      // For std::unordered_set
#include <memory>             // For std::shared_ptr
#include <queue>              // For std::queue
#include <chrono>             // For std::chrono

// Third-Party Library Headers
#include <spdlog/spdlog.h>                          // Spdlog for logging
#include <boost/asio.hpp>                           // Boost Asio
#include <boost/beast.hpp>                          // Boost Beast

// Project-Specific Headers
#include "ChatSession.h"        // ChatSession class definition
#include "ChatServer.h"         // ChatServer class definition

namespace beast = boost::beast;

ChatSession::ChatSession(tcp::socket Socket, ChatServer* Server, std::unordered_set<std::shared_ptr<ChatSession>>& SessionSet, std::mutex& SessionMutex)
    : Server(Server), WebSocketStream(std::move(Socket)), SessionSet(SessionSet), SessionMutex(SessionMutex),
      LastActivity(std::chrono::steady_clock::now()) {}


void ChatSession::Start() {
    spdlog::info("Starting WebSocket session...");
    WebSocketStream.async_accept([Self = shared_from_this()](beast::error_code ErrorCode) {
        if (!ErrorCode) {
            spdlog::info("WebSocket session accepted.");
            Self->LastActivity = std::chrono::steady_clock::now();
            Self->ReadMessage();
            {
                std::lock_guard<std::mutex> Lock(Self->SessionMutex);
                Self->SessionSet.insert(Self);
                spdlog::info("Session added. Total sessions: {}", Self->SessionSet.size());
            }
        } else {
            spdlog::error("WebSocket handshake failed: {}", ErrorCode.message());
        }
    });
}

bool ChatSession::IsTimedOut(std::chrono::steady_clock::time_point CurrentTime, std::chrono::seconds TimeoutDuration) const {
    return std::chrono::duration_cast<std::chrono::seconds>(CurrentTime - LastActivity) > TimeoutDuration;
}

std::string ChatSession::GetClientAddress() const {
    try {
        return WebSocketStream.next_layer().remote_endpoint().address().to_string();
    } catch (const std::exception& e) {
        spdlog::error("Failed to retrieve client address: {}", e.what());
        return "Unknown";
    }
}

void ChatSession::Disconnect() {
    {
        std::lock_guard<std::mutex> Lock(SessionMutex);
        spdlog::info("Removing session.");
        SessionSet.erase(shared_from_this());
    }

    beast::error_code ErrorCode;
    WebSocketStream.close(websocket::close_code::normal, ErrorCode);
    if (ErrorCode) {
        spdlog::error("Error during disconnect: {}", ErrorCode.message());
    } else {
        spdlog::info("Session disconnected cleanly.");
    }
}

void ChatSession::ReadMessage() {
    WebSocketStream.async_read(this->Buffer, [this, Self = shared_from_this()](beast::error_code ErrorCode, std::size_t BytesTransferred) {
        (void)BytesTransferred; // Mark BytesTransferred as unused

        if (!ErrorCode) {
            std::string Message = beast::buffers_to_string(this->Buffer.data());
            spdlog::info("Received message: {}", Message);
            this->Buffer.consume(this->Buffer.size());
            Server->AddToMessageLog(Message);

            // Broadcast to all sessions
            for (const auto& Session : SessionSet) {
                Session->AddMessage(Message);
            }
            ReadMessage();
        }
        else {
            spdlog::error("Read error: {}", ErrorCode.message());
            Disconnect();
        }
        });
}

void ChatSession::AddMessage(const std::string& message) {
    if (!Server) {
        spdlog::error("Server is null in SendMessage!");
        return;
    }

    auto bytes = message.size();
    Server->AddDataSent(bytes);
    Server->IncrementMessagesSent();
    spdlog::info("Sending message: {}", message);

    auto LocalBuffer = std::make_shared<std::string>(message); // Avoid hiding the member Buffer
    auto self = shared_from_this();
    WebSocketStream.async_write(
        boost::asio::buffer(*LocalBuffer),
        [this, self, LocalBuffer](boost::system::error_code ec, std::size_t bytesTransferred) {
            if (!ec) {
                spdlog::info("Sent message: {} ({} bytes)", *LocalBuffer, bytesTransferred);
            }
            else {
                spdlog::error("Error sending message: {}", ec.message());
            }
        }
    );
}

void ChatSession::ReceiveMessage(const std::string& message) {
    auto bytes = message.size();
    Server->AddDataReceived(bytes);
    Server->IncrementMessagesReceived();    
    spdlog::info("Metrics Update: Received {} bytes, Total Messages Received: {}", bytes, Server->GetMessagesReceived());

    spdlog::info("Received message: {}", message);
}

void ChatSession::DoWrite() {
    if (WriteQueue.empty()) {
        IsWriting = false;
        return;
    }

    auto LocalBuffer = std::make_shared<std::string>(WriteQueue.front());
    WriteQueue.pop();

    WebSocketStream.async_write(boost::asio::buffer(*LocalBuffer), [this, Self = shared_from_this(), LocalBuffer](beast::error_code ErrorCode, std::size_t) {
        if (ErrorCode) {
            spdlog::error("Write error: {}", ErrorCode.message());
            Disconnect();
        }
        else {
            DoWrite();
        }
        });
}
bool ChatSession::IsConnected() const {
    return WebSocketStream.is_open();
}

void ChatSession::Cleanup() {
    if (WebSocketStream.is_open()) {
        beast::error_code ec;
        WebSocketStream.close(websocket::close_code::normal, ec);
        if (ec) {
            spdlog::warn("Error closing WebSocket for session: {}", ec.message());
        }
    }
    spdlog::info("Session cleaned up for: {}", GetClientAddress());
}

std::string ChatSession::GetClientID() const {
    return WebSocketStream.next_layer().remote_endpoint().address().to_string();
}

std::string ChatSession::GetIPAddress() {
    return RemoteEndpoint.address().to_string(); // Convert IP address to string
}