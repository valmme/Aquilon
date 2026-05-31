#include "logger.h"

#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
#ifdef _WIN32
WORD ConsoleColorForLevel(Logger::Level level, WORD default_attributes) {
    switch (level) {
        case Logger::Level::Trace: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        case Logger::Level::Debug: return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case Logger::Level::Info:  return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case Logger::Level::Warn:  return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case Logger::Level::Error: return FOREGROUND_RED | FOREGROUND_INTENSITY;
        case Logger::Level::Fatal: return FOREGROUND_RED | FOREGROUND_INTENSITY;
        default:                   return default_attributes;
    }
}

void WriteConsoleChunk(HANDLE handle, const std::string& chunk) {
    if (chunk.empty()) {
        return;
    }

    DWORD written = 0;
    WriteConsoleA(handle, chunk.c_str(), (DWORD)chunk.size(), &written, nullptr);
}
#else
const char* ConsoleColorPrefix(Logger::Level level) {
    switch (level) {
        case Logger::Level::Trace: return "\x1b[37m";
        case Logger::Level::Debug: return "\x1b[36m";
        case Logger::Level::Info:  return "\x1b[32m";
        case Logger::Level::Warn:  return "\x1b[33m";
        case Logger::Level::Error: return "\x1b[31m";
        case Logger::Level::Fatal: return "\x1b[1;31m";
        default:                   return "\x1b[0m";
    }
}
#endif
}

std::ofstream Logger::log_file_;
bool Logger::console_output_ = true;
Logger::Level Logger::min_level_ = Logger::Level::Trace;
std::mutex Logger::mutex_;

void Logger::SetLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_file_.is_open()) {
        log_file_.close();
    }
    log_file_.open(path, std::ios::app);
}

void Logger::SetConsoleOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    console_output_ = enabled;
}

void Logger::SetLogLevel(Level level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

const char* Logger::LevelToString(Level level) {
    switch (level) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
        case Level::Fatal: return "FATAL";
        default:           return "UNKNOWN";
    }
}

std::string Logger::FormatTime() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto millis = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t tt = system_clock::to_time_t(now);
    std::tm local_tm;
#ifdef _WIN32
    localtime_s(&local_tm, &tt);
#else
    localtime_r(&tt, &local_tm);
#endif

    std::ostringstream ss;
    ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S")
       << '.' << std::setw(3) << std::setfill('0') << millis.count();
    return ss.str();
}

void Logger::Log(const char* type, Level level, const char* format, ...) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (static_cast<int>(level) < static_cast<int>(min_level_)) {
        return;
    }

    std::string time = FormatTime();

    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    std::ostringstream output;
    output << '[' << time << ']' 
           << '[' << type << ']' 
           << '[' << LevelToString(level) << "] "
           << message << '\n';

    const std::string& text = output.str();
    if (console_output_) {
#ifdef _WIN32
        HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (handle != INVALID_HANDLE_VALUE) {
            CONSOLE_SCREEN_BUFFER_INFO info{};
            WORD default_attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            if (GetConsoleScreenBufferInfo(handle, &info)) {
                default_attributes = info.wAttributes;
            }

            const WORD level_attributes = ConsoleColorForLevel(level, default_attributes);
            const std::string prefix = "[" + time + "][" + std::string(type) + "][";
            const std::string level_text = LevelToString(level);
            const std::string suffix = "] " + std::string(message) + "\n";

            SetConsoleTextAttribute(handle, default_attributes);
            WriteConsoleChunk(handle, prefix);
            SetConsoleTextAttribute(handle, level_attributes);
            WriteConsoleChunk(handle, level_text);
            SetConsoleTextAttribute(handle, default_attributes);
            WriteConsoleChunk(handle, suffix);
        } else {
            std::fwrite(text.c_str(), 1, text.size(), stdout);
        }
#else
        const char* prefix = ConsoleColorPrefix(level);
        std::fwrite(prefix, 1, std::strlen(prefix), stdout);
        std::fwrite(text.c_str(), 1, text.size() - 1, stdout);
        std::fwrite("\x1b[0m\n", 1, 5, stdout);
#endif
    }
    if (log_file_.is_open()) {
        log_file_ << text;
        log_file_.flush();
    }
}
