#include "config.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

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

bool ParseInt(const std::string& value, int& out_value) {
    if (value.empty()) {
        return false;
    }

    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0') {
        return false;
    }

    out_value = static_cast<int>(parsed);
    return true;
}

std::string KeyName(SDL_Keycode key) {
    const char* name = SDL_GetKeyName(key);
    if (!name || !*name) {
        return "Unknown";
    }
    return name;
}

bool ParseKeyBindValue(const std::string& value, KeyBind& bind) {
    std::vector<SDL_Keycode> parsed_keys;

    std::stringstream stream(value);
    std::string token;
    while (std::getline(stream, token, ',')) {
        const std::string trimmed = Trim(token);
        if (trimmed.empty()) {
            continue;
        }

        const std::string lowered = ToLower(trimmed);
        if (lowered == "none" || lowered == "off" || lowered == "disable" || lowered == "disabled") {
            bind.keys.clear();
            return true;
        }

        const SDL_Keycode key = SDL_GetKeyFromName(trimmed.c_str());
        if (key == SDLK_UNKNOWN) {
            return false;
        }

        parsed_keys.push_back(key);
    }

    bind.keys = std::move(parsed_keys);
    return true;
}

std::string KeyBindToLine(const KeyBind& bind) {
    if (bind.keys.empty()) {
        return "none";
    }

    std::ostringstream out;
    for (std::size_t i = 0; i < bind.keys.size(); ++i) {
        if (i != 0) {
            out << ", ";
        }
        out << KeyName(bind.keys[i]);
    }
    return out.str();
}

void WriteKeyBind(std::ostringstream& out, const char* label, const KeyBind& bind) {
    out << "# " << label << "\n";
    out << label << " = " << KeyBindToLine(bind) << "\n\n";
}

std::string AppConfigToText(const AppConfig& config) {
    std::ostringstream out;
    out << "# Aquilon auto-generated configuration\n\n";
    out << "# Renderer backend options: auto, software, gpu, vulkan, opengl, direct3d11, direct3d12, metal\n";
    out << "renderer = " << (config.renderer_backend.empty() ? "auto" : config.renderer_backend) << "\n\n";
    out << "# Language code used for resources/localization/<code>.loc\n";
    out << "language = " << (config.language.empty() ? "en" : config.language) << "\n\n";
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
    out << "\n# Window size\n";
    out << "window_width = " << config.window_width << "\n";
    out << "window_height = " << config.window_height << "\n\n";
    out << "# Chunk streaming distance in chunks around the player\n";
    out << "chunk_distance = " << config.chunk_distance << "\n\n";
    out << "# Input bindings use SDL key names separated by commas.\n";
    WriteKeyBind(out, "move_up", config.input.move_up);
    WriteKeyBind(out, "move_down", config.input.move_down);
    WriteKeyBind(out, "move_left", config.input.move_left);
    WriteKeyBind(out, "move_right", config.input.move_right);
    WriteKeyBind(out, "inventory_toggle", config.input.inventory_toggle);
    WriteKeyBind(out, "inventory_close", config.input.inventory_close);
    WriteKeyBind(out, "zoom_in", config.input.zoom_in);
    WriteKeyBind(out, "zoom_out", config.input.zoom_out);
    WriteKeyBind(out, "drop_item", config.input.drop_item);
    WriteKeyBind(out, "pick_item", config.input.pick_item);
    WriteKeyBind(out, "rotate_placement", config.input.rotate_placement);
    return out.str();
}

bool WriteDefaultConfig(const std::string& path) {
    AppConfig defaults;
    defaults.language = "en";
    defaults.window_width = 800;
    defaults.window_height = 600;
    defaults.chunk_distance = 4;
    defaults.input.move_up.keys = { SDLK_W, SDLK_UP };
    defaults.input.move_down.keys = { SDLK_S, SDLK_DOWN };
    defaults.input.move_left.keys = { SDLK_A, SDLK_LEFT };
    defaults.input.move_right.keys = { SDLK_D, SDLK_RIGHT };
    defaults.input.inventory_toggle.keys = { SDLK_E };
    defaults.input.inventory_close.keys = { SDLK_ESCAPE };
    defaults.input.zoom_in.keys = { SDLK_EQUALS, SDLK_KP_PLUS };
    defaults.input.zoom_out.keys = { SDLK_MINUS, SDLK_KP_MINUS };
    defaults.input.drop_item.keys = { SDLK_Z };
    defaults.input.pick_item.keys = { SDLK_F };
    defaults.input.rotate_placement.keys = { SDLK_R };

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file << AppConfigToText(defaults);
    return static_cast<bool>(file);
}
} // namespace

bool SaveAppConfig(const std::string& path, const AppConfig& config) {
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        Logger::Log("SYSTEM", Logger::Level::Error,
                    "Could not open config file '%s' for writing.",
                    path.c_str());
        return false;
    }

    file << AppConfigToText(config);
    if (file.fail()) {
        Logger::Log("SYSTEM", Logger::Level::Error,
                    "Failed to write config to '%s'.",
                    path.c_str());
        return false;
    }

    Logger::Log("SYSTEM", Logger::Level::Info, "Saved config to '%s'.", path.c_str());
    return true;
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

        if (key == "language" || key == "locale" || key == "lang") {
            if (value.empty()) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Empty language value at line %d in '%s'.",
                            line_number, path.c_str());
            } else {
                config.language = ToLower(value);
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

        if (key == "window_width" || key == "window_w" || key == "width") {
            int parsed = config.window_width;
            if (!ParseInt(value, parsed) || parsed <= 0) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown window width '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            } else {
                config.window_width = parsed;
            }
            continue;
        }

        if (key == "window_height" || key == "window_h" || key == "height") {
            int parsed = config.window_height;
            if (!ParseInt(value, parsed) || parsed <= 0) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown window height '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            } else {
                config.window_height = parsed;
            }
            continue;
        }

        if (key == "chunk_distance" || key == "chunk_radius" || key == "load_radius") {
            int parsed = config.chunk_distance;
            if (!ParseInt(value, parsed) || parsed < 0) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown chunk distance '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            } else {
                config.chunk_distance = parsed;
            }
            continue;
        }

        if (key == "move_up") {
            if (!ParseKeyBindValue(value, config.input.move_up)) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown key bind '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            }
            continue;
        }

        if (key == "move_down") {
            if (!ParseKeyBindValue(value, config.input.move_down)) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown key bind '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            }
            continue;
        }

        if (key == "move_left") {
            if (!ParseKeyBindValue(value, config.input.move_left)) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown key bind '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            }
            continue;
        }

        if (key == "move_right") {
            if (!ParseKeyBindValue(value, config.input.move_right)) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown key bind '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            }
            continue;
        }

        if (key == "inventory_toggle") {
            if (!ParseKeyBindValue(value, config.input.inventory_toggle)) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown key bind '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            }
            continue;
        }

        if (key == "inventory_close") {
            if (!ParseKeyBindValue(value, config.input.inventory_close)) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown key bind '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            }
            continue;
        }

        if (key == "zoom_in") {
            if (!ParseKeyBindValue(value, config.input.zoom_in)) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown key bind '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            }
            continue;
        }

        if (key == "zoom_out") {
            if (!ParseKeyBindValue(value, config.input.zoom_out)) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown key bind '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            }
            continue;
        }

        if (key == "drop_item") {
            if (!ParseKeyBindValue(value, config.input.drop_item)) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown key bind '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            }
            continue;
        }

        if (key == "pick_item") {
            if (!ParseKeyBindValue(value, config.input.pick_item)) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown key bind '%s' at line %d in '%s'.",
                            value.c_str(), line_number, path.c_str());
            }
            continue;
        }

        if (key == "rotate_placement") {
            if (!ParseKeyBindValue(value, config.input.rotate_placement)) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Unknown key bind '%s' at line %d in '%s'.",
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

std::string KeyBindToText(const KeyBind& bind) {
    return KeyBindToLine(bind);
}

bool KeyBindMatches(const KeyBind& bind, SDL_Keycode key) {
    for (SDL_Keycode candidate : bind.keys) {
        if (candidate == key) {
            return true;
        }
    }
    return false;
}
