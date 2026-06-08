#ifndef AQUILON_CONFIG_H
#define AQUILON_CONFIG_H

#include <vector>
#include <string>

#include "SDL3/SDL.h"
#include "logger.h"

struct KeyBind {
    std::vector<SDL_Keycode> keys;
};

struct InputConfig {
    KeyBind move_up;
    KeyBind move_down;
    KeyBind move_left;
    KeyBind move_right;
    KeyBind inventory_toggle;
    KeyBind inventory_close;
    KeyBind zoom_in;
    KeyBind zoom_out;
};

struct AppConfig {
    std::string renderer_backend;
    std::string language = "en";
    Logger::Level log_level = Logger::Level::Trace;
    bool vsync_enabled = true;
    int window_width = 800;
    int window_height = 600;
    int chunk_distance = 4;
    InputConfig input = [] {
        InputConfig input;
        input.move_up.keys = { SDLK_W, SDLK_UP };
        input.move_down.keys = { SDLK_S, SDLK_DOWN };
        input.move_left.keys = { SDLK_A, SDLK_LEFT };
        input.move_right.keys = { SDLK_D, SDLK_RIGHT };
        input.inventory_toggle.keys = { SDLK_E };
        input.inventory_close.keys = { SDLK_ESCAPE };
        input.zoom_in.keys = { SDLK_EQUALS, SDLK_KP_PLUS };
        input.zoom_out.keys = { SDLK_MINUS, SDLK_KP_MINUS };
        return input;
    }();
};

std::string KeyBindToText(const KeyBind& bind);
bool KeyBindMatches(const KeyBind& bind, SDL_Keycode key);
bool LoadAppConfig(const std::string& path, AppConfig& config);
bool SaveAppConfig(const std::string& path, const AppConfig& config);

#endif // AQUILON_CONFIG_H
