#pragma once

#ifndef CONSOLE_UI_H
#define CONSOLE_UI_H


// Third-Party Library Headers
#include <GLFW/glfw3.h>       // GLFW library headers
#include <imgui.h>            // Dear ImGui headers

// Standard C++ Headers
#include <vector>             // For std::vector
#include <string>             // For std::string
#include <functional>         // For std::function
#include <mutex>              // For std::mutex

class ChatServer;

class ConsoleUI {
public:
    ConsoleUI();   
    ~ConsoleUI();

    bool Initialize();
    void RenderChatServerPanel(std::vector<std::string>& logs, const std::vector<std::string>& connections,
                               std::function<void()> startServerCallback, std::function<void()> stopServerCallback,
                               std::function<void(const std::string&)> processCommandCallback, float cpuUsage,
                               size_t memoryUsage, const std::string& formattedUptime, int totalConnections,
                               int messagesSent, int messagesReceived, size_t dataSent, size_t dataReceived,
                               float& halfWidth);
    void RenderMessageLogPanel(ChatServer* server,
                               std::function<void(const std::string&)> broadcastCallback, std::function<void(const std::string&, const std::string&)>
                               privateMessageCallback, float halfWidth);
    
    void Render(
        std::vector<std::string>& logs,
        ChatServer* server,
        const std::vector<std::string>& connections,
        std::function<void()> startServerCallback,
        std::function<void()> stopServerCallback,
        std::function<void(const std::string&)> processCommandCallback,
        std::function<void(const std::string&, const std::string&)> privateMessageCallback,
        std::function<void(const std::string&, int)> updateSettingsCallback,
        float cpuUsage,
        size_t memoryUsage,
        const std::string& formattedUptime,
        int totalConnections,
        int messagesSent,
        // *** Total messages sent
        int messagesReceived,
        // *** Total messages received
        size_t dataSent,
        // *** Total data sent (bytes)
        size_t dataReceived          // *** Total data received (bytes)
    );
    
    bool ShouldClose();
    void Shutdown();    
    void SaveSetting(const std::string& key, const std::string& value);
    void RenderSettingsPanel(std::function<void(const std::string&, const std::string&)> saveSettingCallback);
    

private:
      
    GLFWwindow* window;
    const char* glslVersion = "#version 130";
    bool isServerRunning;
    char commandInputBuffer[256] = "";
    char MessageInputBuffer[256] = "";
    std::vector<std::string> commandHistory;
    bool shouldScrollToBottom = false; // Flag for auto-scrolling logs

    ImFont* MainFont = nullptr;
};

#endif // CONSOLE_UI_H
