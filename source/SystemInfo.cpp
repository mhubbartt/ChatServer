// Prevent Windows.h from including unnecessary headers
#ifdef _WIN32
#include <Windows.h>          // Windows-specific headers
#include <psapi.h>            // Process memory info for Windows
#undef APIENTRY               // Avoid APIENTRY redefinition issues
#else
#include <sys/resource.h>     // For resource usage (Linux/Unix)
#include <unistd.h>           // For POSIX APIs
#endif

// Standard C++ Headers
#include <chrono>             // Time measurements
#include <iomanip>            // std::put_time
#include <sstream>            // std::ostringstream
#include <string>             // std::string

// Project-Specific Headers
#include "ChatServer.h"       // ChatServer class





float ChatServer::GetCPUUsage() {
    static auto lastUpdateTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count() < 1000) {
        return 0.0f; // Return 0 if the update interval hasn't passed
    }
    lastUpdateTime = now;

#ifdef _WIN32
    // Windows-specific CPU usage calculation
    FILETIME idleTime, kernelTime, userTime;
    static ULARGE_INTEGER prevIdle = {}, prevKernel = {}, prevUser = {};
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER currIdle, currKernel, currUser;
        currIdle.LowPart = idleTime.dwLowDateTime;
        currIdle.HighPart = idleTime.dwHighDateTime;
        currKernel.LowPart = kernelTime.dwLowDateTime;
        currKernel.HighPart = kernelTime.dwHighDateTime;
        currUser.LowPart = userTime.dwLowDateTime;
        currUser.HighPart = userTime.dwHighDateTime;

        ULONGLONG deltaIdle = currIdle.QuadPart - prevIdle.QuadPart;
        ULONGLONG deltaKernel = currKernel.QuadPart - prevKernel.QuadPart;
        ULONGLONG deltaUser = currUser.QuadPart - prevUser.QuadPart;

        prevIdle = currIdle;
        prevKernel = currKernel;
        prevUser = currUser;

        ULONGLONG total = deltaKernel + deltaUser;
        return total > 0 ? 100.0f * (total - deltaIdle) / total : 0.0f;
    }
    return 0.0f;
#else
    // Linux-specific CPU usage calculation
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    double userTime = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    double sysTime = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
    return userTime + sysTime;
#endif
}


size_t ChatServer::GetMemoryUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS memCounters;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &memCounters, sizeof(memCounters))) {
        return memCounters.WorkingSetSize / 1024; // Convert bytes to kilobytes
    }
    return 0;
#else
    struct rusage usageData;
    getrusage(RUSAGE_SELF, &usageData);
    return usageData.ru_maxrss; // Max resident set size in kilobytes
#endif
}

float ChatServer::GetUptime() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<float>(now - ServerStartTime).count(); // *** Calculate uptime
}

int ChatServer::GetTotalConnections() const {
    return TotalConnections.load(); // *** Return total connections
}

std::string ChatServer::GetFormattedUptime() const {
    auto now = std::chrono::steady_clock::now();
    auto uptimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - ServerStartTime).count();

    // Explicit cast to int with range check to prevent potential data loss
    int hours = static_cast<int>(uptimeSeconds / 3600);
    int minutes = static_cast<int>((uptimeSeconds % 3600) / 60);
    int seconds = static_cast<int>(uptimeSeconds % 60);

    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << hours << ":"
        << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << seconds;

    return oss.str();
}

void ChatServer::IncrementMessagesSent() {
    TotalMessagesSent++; // *** Increment total messages sent
}

void ChatServer::IncrementMessagesReceived() {
    TotalMessagesReceived++; // *** Increment total messages received
}

void ChatServer::AddDataSent(size_t bytes) {
    TotalDataSent += bytes; // *** Add bytes to total data sent
}

void ChatServer::AddDataReceived(size_t bytes) {
    TotalDataReceived += bytes; // *** Add bytes to total data received
}

int ChatServer::GetMessagesSent() const {
    return TotalMessagesSent.load(); // *** Return sent messages
}

int ChatServer::GetMessagesReceived() const {
    return TotalMessagesReceived.load(); // *** Return received messages
}

size_t ChatServer::GetDataSent() const {
    return TotalDataSent.load(); // *** Return total data sent
}

size_t ChatServer::GetDataReceived() const {
    return TotalDataReceived.load(); // *** Return total data received
}