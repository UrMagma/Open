#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <format>

#define LOGGER_NAME "FortniteGameServerLogger"

// Logging macros matching Raider's style
#define LOG_DEBUG(...) \
    if (spdlog::get(LOGGER_NAME) != nullptr) { \
        spdlog::get(LOGGER_NAME)->debug(std::format(__VA_ARGS__)); \
    }

#define LOG_INFO(...) \
    if (spdlog::get(LOGGER_NAME) != nullptr) { \
        spdlog::get(LOGGER_NAME)->info(std::format(__VA_ARGS__)); \
    }

#define LOG_WARN(...) \
    if (spdlog::get(LOGGER_NAME) != nullptr) { \
        spdlog::get(LOGGER_NAME)->warn(std::format(__VA_ARGS__)); \
    }

#define LOG_ERROR(...) \
    if (spdlog::get(LOGGER_NAME) != nullptr) { \
        spdlog::get(LOGGER_NAME)->error(std::format(__VA_ARGS__)); \
    }

#define LOG_CRITICAL(...) \
    if (spdlog::get(LOGGER_NAME) != nullptr) { \
        spdlog::get(LOGGER_NAME)->critical(std::format(__VA_ARGS__)); \
    }

#define LOG_FATAL(...) LOG_CRITICAL(__VA_ARGS__)

namespace FortniteGS::Utils {
    
    class Logger {
    public:
        static void Initialize() {
            // Create console sink with color support
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_pattern("%^[%H:%M:%S.%e] [%l] %v%$");
            
            // Create file sink
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("FortniteGameServer.log", true);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
            
            // Combine sinks
            std::vector<spdlog::sink_ptr> sinks = { console_sink, file_sink };
            auto logger = std::make_shared<spdlog::logger>(LOGGER_NAME, sinks.begin(), sinks.end());
            
            // Set log level and flush policy
            logger->set_level(spdlog::level::level_enum::info);
            logger->flush_on(spdlog::level::level_enum::warn);
            
            // Register the logger
            spdlog::register_logger(logger);
            
            LOG_INFO("Logger initialized successfully");
        }
        
        static void Shutdown() {
            LOG_INFO("Shutting down logger");
            spdlog::shutdown();
        }
        
        static void SetLogLevel(spdlog::level::level_enum level) {
            if (auto logger = spdlog::get(LOGGER_NAME)) {
                logger->set_level(level);
                LOG_INFO("Log level set to: {}", spdlog::level::to_string_view(level));
            }
        }
        
        static void FlushLogs() {
            if (auto logger = spdlog::get(LOGGER_NAME)) {
                logger->flush();
            }
        }
    };
}
