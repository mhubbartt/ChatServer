

// Third-Party Library Headers
#include <boost/asio.hpp>     // Boost.Asio headers
#include <boost/property_tree/ini_parser.hpp>  // Boost property tree
#include <boost/property_tree/ptree.hpp>       // Boost property tree
#include <spdlog/spdlog.h>                     // Spdlog logger
#include <spdlog/sinks/rotating_file_sink.h>   // Spdlog rotating file sink

// Standard C++ Headers
#include <csignal>            // Signal handling
#include <vector>             // For std::vector
#include <string>             // For std::string

// Project-Specific Headers
#include "ChatServer.h"       // ChatServer class
#include "ChatSession.h"      // ChatSession class
#include "ConsoleUI.h"        // ConsoleUI class
#include "CustomLogSink.h"    // CustomLogSink class

volatile std::sig_atomic_t SignalStatus = 0;

void SignalHandler(int Signal) {
    SignalStatus = Signal;
    spdlog::info("Received signal {}. Shutting down gracefully.", Signal);
}

int main(int argc, char* argv[]) {

    (void)argc; // Mark argc as unused
    (void)argv; // Mark argv as unused
    try {
        int Port = 8080;
        int TimeoutSeconds = 60;
        int maxClients = 1000;

        boost::property_tree::ptree Config;
        boost::asio::io_context IoContext;

        try {
            boost::property_tree::ini_parser::read_ini("config.ini", Config);
            Port = Config.get<int>("server.port", Port);
            TimeoutSeconds = Config.get<int>("server.timeout", TimeoutSeconds);
            maxClients = Config.get<int>("server.maxClients", maxClients);
        } catch (const std::exception& e) {
            spdlog::warn("Failed to load config.ini, using defaults: {}", e.what());
        }

        auto rotatingLogger = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/chatserver.log", 1048576 * 5, 3);
        std::vector<std::string> Logs;        
        auto customSink = std::make_shared<CustomLogSink>(Logs);
        auto multiSinkLogger = std::make_shared<spdlog::logger>("ChatServer", spdlog::sinks_init_list({rotatingLogger, customSink}));
        spdlog::set_default_logger(multiSinkLogger);
        spdlog::set_level(spdlog::level::info);

        ChatServer* server = nullptr;
        ConsoleUI consoleUI;

        if (!consoleUI.Initialize()) {
            spdlog::critical("Failed to initialize ConsoleUI");
            return -1;
        }

        std::thread serverThread; // Dedicated thread for io_context
        bool isServerRunning = false;

        while (!consoleUI.ShouldClose()) {
            auto connections = server ? server->GetActiveConnections() : std::vector<std::string>();

            consoleUI.Render(
                Logs,
                server,
                connections,
                [&]() { // Start Server Callback
                    if (!isServerRunning) {
                        spdlog::info("Starting server...");
                        server = new ChatServer(IoContext, tcp::endpoint(tcp::v4(), static_cast<boost::asio::ip::port_type>(Port)), TimeoutSeconds);

                        serverThread = std::thread([&]() { IoContext.run(); });
                        isServerRunning = true;
                    }
                },
                [&]() { // Stop Server Callback
                    if (isServerRunning) {
                        spdlog::info("Stopping server...");
                        IoContext.stop();
                        if (serverThread.joinable()) serverThread.join();
                        delete server;
                        server = nullptr;
                        isServerRunning = false;
                        IoContext.restart(); // Reset io_context for reuse
                    }
                },
                [&](const std::string& command) { // Command Callback
                    spdlog::info("Processing command: {}", command);
                    // Handle commands (log/broadcast/private message)
                    if (command.rfind("/msg", 0) == 0) { // Check if the command starts with "/msg"
                        size_t spaceIndex = command.find(' ');
                        if (spaceIndex != std::string::npos) {
                            std::string target = command.substr(5, spaceIndex - 5);
                            std::string message = command.substr(spaceIndex + 1);
                            spdlog::info("Private message to {}: {}", target, message);
                            // Implement private message logic
                            if (server) {
                                server->SendMessageToClient(target, message);
                            }
                        }
                    }
                    else {
                        if (server) {
                            server->BroadcastAllSystemMessage(command);
                        }
                    }
                },
                [&](const std::string& key, const std::string& value) { // Private Message Callback
                    spdlog::info("Private message: Key = {}, Value = {}", key, value);
                },
                [&](const std::string& newPort, int newMaxClients) { // Settings Callback
                    try {
                        Port = std::stoi(newPort);
                        maxClients = newMaxClients;
                        spdlog::info("Updated settings: Port {}, Max Clients {}", Port, maxClients);
                        if (server) {
                            spdlog::warn("Restart the server to apply new settings.");
                        }
                    } catch (const std::exception& e) {
                        spdlog::error("Failed to update settings: {}", e.what());
                    }
                },
                server ? server->GetCPUUsage() : 0.0f,
                server ? server->GetMemoryUsage() : 0,
                server ? server->GetFormattedUptime() : "00:00:00",
                server ? server->GetTotalConnections() : 0,
                server ? server->GetMessagesSent() : 0,
                server ? server->GetMessagesReceived() : 0,
                server ? server->GetDataSent() : 0, server ? server->GetDataReceived() : 0
            );
        }

        consoleUI.Shutdown();
        if (isServerRunning && serverThread.joinable()) serverThread.join();
    } catch (const std::exception& e) {
        spdlog::critical("Exception in main: {}", e.what());
    }

    return 0;
}
