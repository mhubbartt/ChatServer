
// Standard C++ Headers
#include <chrono>             // For std::chrono
#include <ctime>              // For time utilities
#include <string>             // For std::string
#include <vector>             // For std::vector
#include <sstream>            // For std::ostringstream
#include <iomanip>            // For std::put_time
#include <functional>         // For std::function

// Third-Party Library Headers
#include <imgui.h>                             // ImGui core
#include <spdlog/spdlog.h>                     // Spdlog for logging
#include <nlohmann/json.hpp>                   // JSON for Modern C++

// Project Specific Headers
#include "ChatServer.h"         // ChatServer class
#include "ConsoleUI.h"          // ConsoleUI class




void ConsoleUI::RenderMessageLogPanel(ChatServer* server,
                                      std::function<void(const std::string&)> broadcastCallback,
                                      std::function<void(const std::string&, const std::string&)> privateMessageCallback,
                                      float halfWidth) 
{
    std::vector<std::string> messages;
    if (server) {
        messages = server->GetMessages(); // Fetch all messages
    }

    ImGui::Begin("Message Window");

    // Display Messages
    ImGui::BeginChild("MessageArea", ImVec2(halfWidth, 200), true);
    ImGui::Text("Messages:");
    ImGui::Separator();

    for (const auto& jsonMessage : messages) {
        try {
            // Parse the JSON message
            auto messageObject = nlohmann::json::parse(jsonMessage);

            // Extract fields for display
            std::string timestamp = messageObject.value("timestamp", "Unknown Time");
            std::string sender = messageObject.value("sender", "Unknown Sender");
            std::string content = messageObject.value("content", "Unknown Message");
            std::string ipAddress = messageObject.value("ip", "Unknown IP");

            // Format the message for display
            std::string formattedMessage = "[" + timestamp + "] " + sender + " (" + ipAddress + "): " + content;
            ImGui::TextWrapped("%s", formattedMessage.c_str());
        } catch (const std::exception& e) {
            // Display parsing errors
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error parsing message: %s", e.what());
        }
    }
    ImGui::EndChild();

    // Broadcast Input
static char broadcastInputBuffer[256] = "";
ImGui::Text("Broadcast Message:");
if (ImGui::InputText("##BroadcastInput", broadcastInputBuffer, sizeof(broadcastInputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
    // Construct the JSON object for broadcasting
    try {
        nlohmann::json broadcastMessage;
        
        // Generate timestamp
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTime;
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&localTime, &currentTime);
#else
        localtime_r(&currentTime, &localTime);
#endif
        std::ostringstream timestampStream;
        timestampStream << std::put_time(&localTime, "%m/%d/%Y %I:%M:%S");
        
        broadcastMessage["timestamp"] = timestampStream.str();
        broadcastMessage["sender"] = "Server"; // Hardcoded or dynamic sender
        broadcastMessage["content"] = std::string(broadcastInputBuffer);

        // Execute the callback with the correctly formatted message
        broadcastCallback(broadcastMessage.dump());
    } catch (const std::exception& e) {
        spdlog::error("Failed to construct or send broadcast message: {}", e.what());
    }

    broadcastInputBuffer[0] = '\0'; // Clear the buffer
}

if (ImGui::Button("Send Broadcast")) {
    // Construct the JSON object for broadcasting
    try {
        nlohmann::json broadcastMessage;
        
        // Generate timestamp
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTime;
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&localTime, &currentTime);
#else
        localtime_r(&currentTime, &localTime);
#endif
        std::ostringstream timestampStream;
        timestampStream << std::put_time(&localTime, "%m/%d/%Y %I:%M:%S");
        
        broadcastMessage["timestamp"] = timestampStream.str();
        broadcastMessage["sender"] = "Server"; // Hardcoded or dynamic sender
        broadcastMessage["content"] = std::string(broadcastInputBuffer);

        // Execute the callback with the correctly formatted message
        broadcastCallback(broadcastMessage.dump());
    } catch (const std::exception& e) {
        spdlog::error("Failed to construct or send broadcast message: {}", e.what());
    }

    broadcastInputBuffer[0] = '\0'; // Clear the buffer
}

    // Private Message Input
    static char privateRecipientBuffer[128] = "";
    static char privateMessageBuffer[256] = "";

    ImGui::Text("Private Message:");
    ImGui::InputText("Recipient", privateRecipientBuffer, sizeof(privateRecipientBuffer));
    ImGui::InputText("Message", privateMessageBuffer, sizeof(privateMessageBuffer));
    if (ImGui::Button("Send Private")) {
        std::string recipient(privateRecipientBuffer);
        std::string message(privateMessageBuffer);
        privateMessageCallback(recipient, message);
        privateRecipientBuffer[0] = '\0'; // Clear the recipient buffer
        privateMessageBuffer[0] = '\0';  // Clear the message buffer
    }

    ImGui::End();
}
