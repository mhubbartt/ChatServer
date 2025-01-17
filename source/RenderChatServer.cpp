// Platform-Specific Headers
#ifdef _WIN32
#include <Windows.h>          // Windows-specific headers
#undef APIENTRY               // Prevent redefinition of APIENTRY
#else
#include <unistd.h>           // POSIX header for Linux/Unix
#endif

// Standard C++ Headers
#include <string>             // For std::string
#include <vector>             // For std::vector
#include <functional>         // For std::function

// Third-Party Library Headers
#include <imgui.h>                             // ImGui core

// Project-Specific Headers
#include "ConsoleUI.h"          // ConsoleUI class



void ConsoleUI::RenderChatServerPanel(std::vector<std::string>& logs, const std::vector<std::string>& connections, std::function<void()> startServerCallback, std::function<void()> stopServerCallback, std::function<void(const std::string&)> processCommandCallback, float cpuUsage, size_t memoryUsage, const std::string& formattedUptime, int totalConnections, int messagesSent, int messagesReceived, size_t dataSent, size_t dataReceived, float& halfWidth)
{
    ImGui::SetNextWindowSize(ImVec2(800, 600));
    
    ImGui::Begin( "Game Chat Server Console", NULL,ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);   

    // Start/Stop Buttons
    if (!isServerRunning) {
        if (ImGui::Button("Start Server")) {
            startServerCallback();
            isServerRunning = true;
        }
    } else {
        if (ImGui::Button("Stop Server")) {
            stopServerCallback();
            isServerRunning = false;
        }
    }
    
    ImGui::SameLine();

    
    // Uptime with Conditional Coloring
    if (isServerRunning) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255)); // Green
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255)); // Red
    }
    ImGui::Bullet();
    ImGui::PopStyleColor(); // Revert to default color
    ImGui::Text("Uptime: %s", formattedUptime.c_str());

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0,175,255, 255)); // blue
    ImGui::Bullet();   
    ImGui::PopStyleColor(); // Revert to default color
    ImGui::Text("Active Connections: %d", (int)connections.size());

    
    ImGui::Separator();
    ImGui::Spacing();
    halfWidth = ImGui::GetContentRegionAvail().x * 0.5f;

    // Logs Section
    ImGui::BeginChild("LogArea", ImVec2(halfWidth, 200), true);
    ImGui::Text("Logs:");
    ImGui::Separator();
    for (const auto& message : logs) {
        ImGui::TextWrapped("%s", message.c_str());
    }
    ImGui::EndChild();

    // Move to the same line for the next section
    ImGui::SameLine();

    // Connected Clients Section
    ImGui::BeginChild("ConnectionArea", ImVec2(halfWidth, 200), true);
    ImGui::Text("Connected Clients:");
    ImGui::Separator();
    for (const auto& client : connections) {
        ImGui::Text("%s", client.c_str());
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Spacing();

    // Server Statistics
    ImGui::BeginChild("ServerStatistics", ImVec2(halfWidth, 200), true);
    ImGui::Text("Server Statistics:");
    ImGui::Separator();
    ImGui::BulletText("CPU Usage: %.2f%%", cpuUsage);
    ImGui::BulletText("Memory Usage: %zu KB", memoryUsage);
  
    ImGui::BulletText("Total Session Connections : %d", totalConnections);
   
    ImGui::EndChild();      
    
    ImGui::SameLine();
    
    // Real-Time Metrics
    ImGui::BeginChild("RealTimeMetrics", ImVec2(halfWidth, 200), true);
    ImGui::Text("Real-Time Metrics :");
    ImGui::Separator();
    ImGui::BulletText("Messages Sent: %d", messagesSent);
    ImGui::BulletText("Messages Received : %d", messagesReceived);
    ImGui::BulletText("Data Sent : %.2f KB", dataSent / 1024.0);
    ImGui::BulletText("Data Received : %.2f KB", dataReceived / 1024.0);
    
    ImGui::EndChild();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    // Command Input
    ImGui::Text("Command Input:");
    if (ImGui::InputText("##CommandInput", commandInputBuffer, sizeof(commandInputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        std::string command(commandInputBuffer);
        processCommandCallback(command);
        commandInputBuffer[0] = '\0'; // Clear the buffer
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Submit Command")) {
        std::string command(commandInputBuffer);
        processCommandCallback(command);
        commandInputBuffer[0] = '\0'; // Clear the buffer
    }

    ImGui::End();
}
