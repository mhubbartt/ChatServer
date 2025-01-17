

// Standard C++ Headers
#include <fstream>            // For file input/output
#include <string>             // For std::string
#include <regex>              // For std::regex

// Third-Party Library Headers
#include <spdlog/spdlog.h>                     // Spdlog for logging
#include <imgui.h>                             // ImGui core

// Project-Specific Headers
#include "ConsoleUI.h"          // ConsoleUI class



void ConsoleUI::SaveSetting(const std::string& key, const std::string& value) {
    std::ifstream file("config.ini");
    std::string config((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Update or insert the key-value pair
    std::regex regex(key + "\\s*=\\s*.*");
    std::string newEntry = key + " = " + value;
    if (std::regex_search(config, regex)) {
        config = std::regex_replace(config, regex, newEntry);
    } else {
        config += "\n" + newEntry;
    }

    // Write back to the file
    std::ofstream outFile("config.ini");
    outFile << config;
    outFile.close();
    spdlog::info("Saved setting: {} = {}", key, value);
}


void ConsoleUI::RenderSettingsPanel(std::function<void(const std::string&, const std::string&)> saveSettingCallback) {
    static char serverPort[6] = "8080";
    static int maxClients = 100;
    static bool enableLogging = true;
    static int timeoutSeconds = 30;
    static bool darkTheme = true;
    static int cleanerInterval = 60;
    static int maxMessageSize = 1024;
    static char logFilePath[256] = "logs/server.log";
    static char rawConfig[4096] = "";

    ImGui::Begin("Settings");

    if (ImGui::CollapsingHeader("Server Settings")) {
        if (ImGui::InputText("Server Port", serverPort, sizeof(serverPort))) {
            saveSettingCallback("ServerPort", serverPort);
        }
        if (ImGui::SliderInt("Max Clients", &maxClients, 1, 1000)) {
            saveSettingCallback("MaxClients", std::to_string(maxClients));
        }
        if (ImGui::SliderInt("Timeout (seconds)", &timeoutSeconds, 1, 300)) {
            saveSettingCallback("TimeoutSeconds", std::to_string(timeoutSeconds));
        }
        if (ImGui::SliderInt("Cleaner Interval (seconds)", &cleanerInterval, 10, 3600)) {
            saveSettingCallback("CleanerInterval", std::to_string(cleanerInterval));
        }
        if (ImGui::SliderInt("Max Message Size (bytes)", &maxMessageSize, 256, 8192)) {
            saveSettingCallback("MaxMessageSize", std::to_string(maxMessageSize));
        }
    }

    if (ImGui::CollapsingHeader("Logging Settings")) {
        if (ImGui::InputText("Log File Path", logFilePath, sizeof(logFilePath))) {
            saveSettingCallback("LogFilePath", logFilePath);
        }
        if (ImGui::Checkbox("Enable Logging", &enableLogging)) {
            saveSettingCallback("EnableLogging", enableLogging ? "true" : "false");
        }
    }

    if (ImGui::CollapsingHeader("Theme Settings")) {
        if (ImGui::Button("Dark Theme")) {
            ImGui::StyleColorsDark();
            darkTheme = true;
            saveSettingCallback("Theme", "dark");
        }
        ImGui::SameLine();
        if (ImGui::Button("Light Theme")) {
            ImGui::StyleColorsLight();
            darkTheme = false;
            saveSettingCallback("Theme", "light");
        }
    }

    if (ImGui::CollapsingHeader("Advanced Configuration")) {
        if (rawConfig[0] == '\0') {
            std::ifstream file("config.ini");
            std::string config((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
#ifdef _WIN32
            strncpy_s(rawConfig, config.c_str(), sizeof(rawConfig) - 1);
#else
            strncpy(rawConfig, config.c_str(), sizeof(rawConfig) - 1);
            rawConfig[sizeof(rawConfig) - 1] = '\0'; // Ensure null termination
#endif
        }

        ImGui::InputTextMultiline("Raw Config", rawConfig, sizeof(rawConfig), ImVec2(-1, 200));
        if (ImGui::Button("Save Changes")) {
            std::ofstream outFile("config.ini");
            outFile << rawConfig;
            outFile.close();
            spdlog::info("Configuration saved.");
        }
    }

    ImGui::End();
}
