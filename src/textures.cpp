#include "textures.h"
#include "logger.h"
#include <cstdio>

static SDL_Texture* find_texture(SDL_Renderer* renderer, SDL_Texture* fallback, const char* label, const char* file) {
    SDL_Texture* tex = IMG_LoadTexture(renderer, file);

    if (!tex) {
        Logger::Log("APPLICATION", Logger::Level::Error,
                    "Failed to load texture '%s' from '%s': %s",
                    label, file, SDL_GetError());
        return fallback;
    }

    return tex;
}

Textures load_textures(SDL_Renderer* renderer) {
    Textures t{};

    t.none = IMG_LoadTexture(renderer, "resources/textures/none.png");
    if (!t.none) {
        Logger::Log("APPLICATION", Logger::Level::Warn,
                    "Fallback texture 'none' could not be loaded from 'resources/textures/none.png': %s",
                    SDL_GetError());
    }

    t.ice  = find_texture(renderer, t.none, "ice",  "resources/textures/ice.png");
    t.snow = find_texture(renderer, t.none, "snow", "resources/textures/snow.png");
    t.rock = find_texture(renderer, t.none, "rock", "resources/textures/rock.png");
    t.ore  = find_texture(renderer, t.none, "ore",  "resources/textures/rock.png");

    Logger::Log("APPLICATION", Logger::Level::Info, "Texture loading complete.");

    return t;
}

void free_textures(Textures& t) {
    SDL_Texture* destroyed[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    int destroyed_count = 0;

    auto destroy_unique = [&](SDL_Texture* texture) {
        if (!texture) {
            return;
        }

        for (int i = 0; i < destroyed_count; ++i) {
            if (destroyed[i] == texture) {
                return;
            }
        }

        SDL_DestroyTexture(texture);
        destroyed[destroyed_count++] = texture;
    };

    destroy_unique(t.ice);
    destroy_unique(t.snow);
    destroy_unique(t.rock);
    destroy_unique(t.ore);
    destroy_unique(t.none);

    t = {};
}
