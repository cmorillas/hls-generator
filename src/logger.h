#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    LOG_ERROR  // Renamed from ERROR to avoid conflict with Windows wingdi.h
};

class Logger {
public:
    static void setLevel(LogLevel level);

    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);

private:
    static LogLevel currentLevel;
    static void log(LogLevel level, const std::string& message);
    static std::string levelToString(LogLevel level);
};

#endif // LOGGER_H
