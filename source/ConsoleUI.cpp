
// Add GLAD loader or include ImGui's OpenGL loader if required
#ifdef IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <glad/glad.h>
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>
#else
#include <imgui_impl_opengl3_loader.h>
#endif

// Standard C++ Headers
#include <string>
#include <vector>
#include <functional>

// ImGui Headers
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// Project-Specific Headers
#include "ConsoleUI.h"
#include <imgui_impl_opengl3_loader.h>
#include <iostream>

ConsoleUI::ConsoleUI() :  window(nullptr), isServerRunning(false)
{
}

ConsoleUI::~ConsoleUI() {
    Shutdown();
}

bool ConsoleUI::Initialize() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    window = glfwCreateWindow(1280, 720, "Chat Server Console", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    } 
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    io.Fonts->AddFontDefault();
    MainFont = io.Fonts->AddFontFromFileTTF("Fonts/IBMPlexSans-Medium.ttf", 18.0f);
    IM_ASSERT(MainFont != nullptr);   
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    return true;
}
// Function to save a key-value pair to config.ini



void ConsoleUI::Render(
    std::vector<std::string>& logs,
    ChatServer* server,
    const std::vector<std::string>& connections,
    std::function<void()> startServerCallback,
    std::function<void()> stopServerCallback,
    std::function<void(const std::string&)> processCommandCallback,
    std::function<void(const std::string&, const std::string&)> privateMessageCallback,
    std::function<void(const std::string&, int)> updateSettingsCallback, // New callback for settings
    float cpuUsage,
    size_t memoryUsage,
    const std::string& formattedUptime,
    int totalConnections, int messagesSent, int messagesReceived, size_t dataSent, size_t dataReceived
) {


  //   ShowWindow(GetConsoleWindow(), SW_HIDE);
   
    
    // Render UI elements (Logs, Buttons, Commands, Statistics)
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    
    ImGui::PushFont(MainFont);  // Set Global Font

    //   ****  Render chat server panel  ****
    float halfWidth;
    RenderChatServerPanel(logs, connections, startServerCallback, stopServerCallback, processCommandCallback, cpuUsage,
                          memoryUsage, formattedUptime, totalConnections, messagesSent, messagesReceived, dataSent,
                          dataReceived, halfWidth);

    //   ****  Render message log panel  ****
    RenderMessageLogPanel(server,  processCommandCallback, privateMessageCallback, halfWidth);

    //   ****  Render settings panel  ****
    RenderSettingsPanel([this](const std::string& key, const std::string& value) { SaveSetting(key, value);});


    ImGui::PopFont(); // Reset Global Font

    
    // Render the ImGui frame
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

bool ConsoleUI::ShouldClose() {
    return glfwWindowShouldClose(window);
}

void ConsoleUI::Shutdown() {
    static bool isShutdown = false; // Track if Shutdown has already been called
    if (isShutdown) {
        return; // Avoid running Shutdown multiple times
    }

    isShutdown = true; // Mark as shutdown to prevent re-entry

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();
}



