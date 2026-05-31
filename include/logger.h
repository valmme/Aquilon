#ifndef AQUILON_LOGGER_H
#define AQUILON_LOGGER_H

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <string>

class Logger {
public:
    enum class Level {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Fatal
    };

    static void SetLogFile(const std::string& path);
    static void SetConsoleOutput(bool enabled);
    static void SetLogLevel(Level level);
    static void Log(const char* type, Level level, const char* format, ...);

private:
    static const char* LevelToString(Level level);
    static std::string FormatTime();

    static std::ofstream log_file_;
    static bool console_output_;
    static Level min_level_;
    static std::mutex mutex_;
};

#endif // AQUILON_LOGGER_H
