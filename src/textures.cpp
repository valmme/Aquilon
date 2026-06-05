#include "textures.h"
#include "logger.h"
#include <cstdio>

static SDL_Texture* CreateFallbackTexture(SDL_Renderer* renderer) {
    constexpr int SIZE = 16;
    constexpr int HALF = SIZE / 2;

    SDL_Texture* tex = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STATIC,
        SIZE, SIZE);

    if (!tex) {
        Logger::Log("ASSETS", Logger::Level::Fatal,
                    "Failed to create fallback texture: %s", SDL_GetError());
        return nullptr;
    }

    Uint32 pixels[SIZE * SIZE];
    for (int y = 0; y < SIZE; ++y) {
        for (int x = 0; x < SIZE; ++x) {
            bool checker = ((x < HALF) ^ (y < HALF));
            pixels[y * SIZE + x] = checker ? 0xFF00FFFF : 0x000000FF;
        }
    }

    SDL_UpdateTexture(tex, nullptr, pixels, SIZE * sizeof(Uint32));
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    return tex;
}

static SDL_Texture* LoadTexture(SDL_Renderer* renderer, SDL_Texture* fallback,
                                const char* label, const char* file) {
    SDL_Texture* tex = IMG_LoadTexture(renderer, file);

    if (!tex) {
        Logger::Log("ASSETS", Logger::Level::Error,
                    "Failed to load texture '%s' from '%s': %s",
                    label, file, SDL_GetError());
        return fallback;
    }

    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    return tex;
}

Textures LoadTextures(SDL_Renderer* renderer) {
    Textures t{};

    t.none = CreateFallbackTexture(renderer);

    if (!t.none) {
        Logger::Log("ASSETS`", Logger::Level::Warn,
                    "Fallback texture 'none' could not be created.");
    }

    // tiles
    t.ice        = LoadTexture(renderer, t.none, "ice",        "resources/textures/ice.png");
    t.snow       = LoadTexture(renderer, t.none, "snow",       "resources/textures/snow.png");
    t.stone      = LoadTexture(renderer, t.none, "stone",      "resources/textures/stone.png");
    t.iron_ore   = LoadTexture(renderer, t.none, "iron_ore",   "resources/textures/iron_ore.png");
    t.iron_plate = LoadTexture(renderer, t.none, "iron_plate", "resources/textures/iron_plate.png");
    t.coal       = LoadTexture(renderer, t.none, "coal", "resources/textures/coal.png");
    t.furnace    = LoadTexture(renderer, t.none, "furnace",    "resources/textures/furnace.png");
    t.drill      = LoadTexture(renderer, t.none, "drill",      "resources/textures/drill.png");

    // ui
    t.slot          = LoadTexture(renderer, t.none, "slot",          "resources/textures/slot.png");
    t.crafting_slot = LoadTexture(renderer, t.none, "crafting_slot", "resources/textures/crafting_slot.png");

    Logger::Log("ASSETS", Logger::Level::Info, "Texture loading complete.");

    return t;
}

void FreeTextures(Textures& t) {
    if (t.ice) SDL_DestroyTexture(t.ice);
    if (t.snow) SDL_DestroyTexture(t.snow);
    if (t.stone) SDL_DestroyTexture(t.stone);
    if (t.iron_ore) SDL_DestroyTexture(t.iron_ore);
    if (t.iron_plate) SDL_DestroyTexture(t.iron_plate);
    if (t.furnace) SDL_DestroyTexture(t.furnace);
    if (t.slot) SDL_DestroyTexture(t.slot);
    if (t.none) SDL_DestroyTexture(t.none);

    t = {};
}
