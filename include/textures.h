#ifndef AQUILON_TEXTURES_H
#define AQUILON_TEXTURES_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

struct Textures {
    SDL_Texture* none;

    // tiles
    SDL_Texture* ice;
    SDL_Texture* snow;
    SDL_Texture* stone;
    SDL_Texture* iron_ore;

    SDL_Texture* furnace;

    // ui
    SDL_Texture* slot;
    SDL_Texture* crafting_slot;
};

Textures LoadTextures(SDL_Renderer* renderer);
void FreeTextures(Textures& t);

#endif // AQUILON_TEXTURES_H
