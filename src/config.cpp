#include "config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#include <SDL3/SDL_filesystem.h>

namespace {
std::string Trim(const std::string& value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });

    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();

    if (begin >= end) {
        return {};
    }

    return std::string(begin, end);
}

std::string ToLower(std::string value) {
    for (char& ch : value) {
        ch = (char)std::tolower((unsigned char)ch);
    }
    return value;
}

bool ParseLogLevel(const std::string& value, Logger::Level& level) {
    const std::string lower = ToLower(value);
    if (lower == "trace") { level = Logger::Level::Trace; return true; }
    if (lower == "debug") { level = Logger::Level::Debug; return true; }
    if (lower == "info")  { level = Logger::Level::Info;  return true; }
    if (lower == "warn")  { level = Logger::Level::Warn;  return true; }
    if (lower == "error") { level = Logger::Level::Error; return true; }
    if (lower == "fatal") { level = Logger::Level::Fatal; return true; }
    return false;
}

bool ParseBool(const std::string& value, bool& enabled) {
    const std::string lower = ToLower(value);
    if (lower == "on" || lower == "true" || lower == "1" || lower == "yes") {
        enabled = true;
        return true;
    }
    if (lower == "off" || lower == "false" || lower == "0" || lower == "no") {
        enabled = false;
        return true;
    }
    return false;
}

std::string AppConfigToText(const AppConfig& config) {
    std::ostringstream out;
    out << "# Aquilon auto-generated configuration\n\n";
    out << "# Renderer backend options: auto, software, gpu, vulkan, opengl, direct3d11, direct3d12, metal\n";
    out << "renderer = " << (config.renderer_backend.empty() ? "auto" : config.renderer_backend) << "\n\n";
    out << "# Logging level: trace, debug, info, warn, error, fatal\n";

    switch (config.log_level) {
        case Logger::Level::Trace: out << "log_level = trace\n\n"; break;
        case Logger::Level::Debug: out << "log_level = debug\n\n"; break;
        case Logger::Level::Info:  out << "log_level = info\n\n"; break;
        case Logger::Level::Warn:  out << "log_level = warn\n\n"; break;
        case Logger::Level::Error: out << "log_level = error\n\n"; break;
        case Logger::Level::Fatal: out << "log_level = fatal\n\n"; break;
    }

    out << "# VSync: on / off\n";
    out << "vsync = " << (config.vsync_enabled ? "on" : "off") << "\n";
    return out.str();
}

bool WriteDefaultConfig(const std::string& path) {
    AppConfig defaults;
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file << AppConfigToText(defaults);
    return static_cast<bool>(file);
}
}

bool LoadAppConfig(const std::string& path, AppConfig& config) {
    std::ifstream file(path);
    if (!file.is_open()) {
        if (WriteDefaultConfig(path)) {
            Logger::Log("SYSTEM", Logger::Level::Info,
                        "Config file '%s' did not exist; created default config.",
                        path.c_str());
        } else {
            Logger::Log("SYSTEM", Logger::Level::Warn,
                        "Config file '%s' not found and could not be created; using built-in defaults.",
                        path.c_str());
        }
        return false;
    }

    std::string line;
    int line_number = 0;
    while (std::getline(file, line)) {
        ++line_number;

        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        const std::size_t separator = trimmed.find('=');
        if (separator == std::string::npos) {
            Logger::Log("SYSTEM", Logger::Level::Warn,
                        "Ignoring malformed config line %d in '%s'.",
                        line_number, path.c_str());
            continue;
        }

        const std::string key = ToLower(Trim(trimmed.substr(0, separator)));
        const std::string value = Trim(trimmed.substr(separator + 1));

        if (key == "renderer" || key == "renderer_backend" || key == "backend") {
            const std::string lowered = ToLower(value);
            if (lowered.empty() || lowered == "auto" || lowered == "default" || lowered == "none") {
                config.renderer_backend.clear();
            } else {
                config.renderer_backend = value;
            }
            continue;
        }

        if (key == "log_level" || key == "log") {
            if (!ParseLogLevel(value, config.log_level)) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown log level '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            }
            continue;
        }

        if (key == "vsync") {
            if (!ParseBool(value, config.vsync_enabled)) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown vsync value '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            }
            continue;
        }

        Logger::Log("SYSTEM", Logger::Level::Warn,
                    "Ignoring unknown config key '%s' at line %d in '%s'.",
                    key.c_str(), line_number, path.c_str());
    }

    Logger::Log("SYSTEM", Logger::Level::Info, "Loaded config from '%s'.", path.c_str());
    return true;
}
