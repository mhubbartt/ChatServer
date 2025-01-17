#pragma once

#ifndef CUSTOM_LOG_SINK_H
#define CUSTOM_LOG_SINK_H



// Third-Party Library Headers
#include <spdlog/sinks/base_sink.h>    // spdlog base sink for logging

// Standard C++ Headers
#include <vector>                      // For std::vector
#include <string>                      // For std::string
#include <mutex>                       // For std::mutex

class CustomLogSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit CustomLogSink(std::vector<std::string>& logs) : logs(logs) {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        logs.emplace_back(fmt::to_string(formatted));

        // Manage log size to avoid excessive memory use
        if (logs.size() > 1000) {
            logs.erase(logs.begin(), logs.begin() + 100);
        }
    }

    void flush_() override {}

private:
    std::vector<std::string>& logs;
};

#endif // CUSTOM_LOG_SINK_H
