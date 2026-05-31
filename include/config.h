#ifndef AQUILON_CONFIG_H
#define AQUILON_CONFIG_H

#include <string>

#include "logger.h"

struct AppConfig {
    std::string renderer_backend;
    Logger::Level log_level = Logger::Level::Trace;
    bool vsync_enabled = true;
};

bool LoadAppConfig(const std::string& path, AppConfig& config);

#endif // AQUILON_CONFIG_H
