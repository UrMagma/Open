#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <mutex>

// Simple logging system for DLL compatibility
class SimpleLogger {
public:
    enum Level {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3,
        CRITICAL = 4
    };
    
static SimpleLogger& Get() {
    static SimpleLogger instance;
    return instance;
}
    
    void Log(Level level, const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        const char* levelStr = "";
        switch (level) {
            case DEBUG: levelStr = "DEBUG"; break;
            case INFO: levelStr = "INFO"; break;
            case WARN: levelStr = "WARN"; break;
            case ERROR: levelStr = "ERROR"; break;
            case CRITICAL: levelStr = "CRITICAL"; break;
        }
        
        std::cout << "[" << std::put_time(&tm, "%H:%M:%S") << "] [" 
                  << levelStr << "] " << message << std::endl;
    }
    
private:
    std::mutex logMutex;
};

// Logging macros
#define LOG_DEBUG(msg) SimpleLogger::Get().Log(SimpleLogger::DEBUG, msg)
#define LOG_INFO(msg) SimpleLogger::Get().Log(SimpleLogger::INFO, msg)
#define LOG_WARN(msg) SimpleLogger::Get().Log(SimpleLogger::WARN, msg)
#define LOG_ERROR(msg) SimpleLogger::Get().Log(SimpleLogger::ERROR, msg)
#define LOG_CRITICAL(msg) SimpleLogger::Get().Log(SimpleLogger::CRITICAL, msg)
#define LOG_FATAL(msg) LOG_CRITICAL(msg)

namespace FortniteGS::Utils {
    class Logger {
    public:
        static void Initialize() {
            LOG_INFO("Logger initialized successfully");
        }
        
        static void Shutdown() {
            LOG_INFO("Shutting down logger");
        }
        
        static void FlushLogs() {
            // No-op for simple logger
        }
    };
}
