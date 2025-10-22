#include "logger.h"
#include <ctime>
#include <iomanip>
#include <sstream>

LogLevel Logger::currentLevel = LogLevel::INFO;

void Logger::setLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::LOG_ERROR, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < currentLevel) {
        return;
    }

    // Timestamp (thread-safe version)
    auto now = std::time(nullptr);
    std::tm tm{};

#ifdef _WIN32
    // Windows: use localtime_s (thread-safe)
    localtime_s(&tm, &now);
#else
    // Linux/POSIX: use localtime_r (thread-safe)
    localtime_r(&now, &tm);
#endif

    std::ostringstream oss;
    oss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ";
    oss << "[" << levelToString(level) << "] ";
    oss << message;

    if (level == LogLevel::LOG_ERROR) {
        std::cerr << oss.str() << std::endl;
    } else {
        std::cout << oss.str() << std::endl;
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
