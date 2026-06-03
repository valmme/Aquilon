#include "textures.h"
#include "logger.h"
#include <cstdio>

static SDL_Texture* LoadTexture(SDL_Renderer* renderer, const char* label, const char* file) {
    SDL_Texture* tex = IMG_LoadTexture(renderer, file);

    if (!tex) {
        Logger::Log("APPLICATION", Logger::Level::Error,
                    "Failed to load texture '%s' from '%s': %s",
                    label, file, SDL_GetError());
        return nullptr;
    }

    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    if (tex) {
        return tex;
    }

    SDL_Texture* fallback = LoadTexture(renderer, "none", "resources/textures/none.png");
    if (!fallback) {
        Logger::Log("APPLICATION", Logger::Level::Fatal,
                    "Fallback texture could not be created for '%s'.",
                    label);
    }
    return fallback;
}

Textures LoadTextures(SDL_Renderer* renderer) {
    Textures t{};

    t.none = IMG_LoadTexture(renderer, "resources/textures/none.png");

    if (!t.none) {
        Logger::Log("APPLICATION", Logger::Level::Warn,
                    "Fallback texture 'none' could not be loaded from 'resources/textures/none.png': %s",
                    SDL_GetError());
    }

    // tiles
    t.ice      = LoadTexture(renderer, "ice", "resources/textures/ice.png");
    t.snow     = LoadTexture(renderer, "snow", "resources/textures/snow.png");
    t.stone    = LoadTexture(renderer, "stone", "resources/textures/stone.png");
    t.iron_ore = LoadTexture(renderer, "iron_ore", "resources/textures/iron_ore.png");
    t.furnace  = LoadTexture(renderer, "furnace", "resources/textures/furnace.png");

    // ui
    t.slot          = LoadTexture(renderer, "slot", "resources/textures/slot.png");
    t.crafting_slot = LoadTexture(renderer, "crafting_slot", "resources/textures/crafting_slot.png");

    Logger::Log("APPLICATION", Logger::Level::Info, "Texture loading complete.");

    return t;
}

void FreeTextures(Textures& t) {
    if (t.ice) SDL_DestroyTexture(t.ice);
    if (t.snow) SDL_DestroyTexture(t.snow);
    if (t.stone) SDL_DestroyTexture(t.stone);
    if (t.iron_ore) SDL_DestroyTexture(t.iron_ore);
    if (t.furnace) SDL_DestroyTexture(t.furnace);
    if (t.slot) SDL_DestroyTexture(t.slot);
    if (t.none) SDL_DestroyTexture(t.none);

    t = {};
}
