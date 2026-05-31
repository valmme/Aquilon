#include "textures.h"
#include <cstdio>

Textures load_textures(SDL_Renderer* renderer) {
    Textures t;

    t.ice  = IMG_LoadTexture(renderer, "resources/textures/ice.png");
    t.snow = IMG_LoadTexture(renderer, "resources/textures/snow.png");
    t.rock = IMG_LoadTexture(renderer, "resources/textures/rock.png");
    t.ore  = IMG_LoadTexture(renderer, "resources/textures/ore.png");

    return t;
}

void free_textures(Textures& t) {
    if (t.ice)  SDL_DestroyTexture(t.ice);
    if (t.snow) SDL_DestroyTexture(t.snow);
    if (t.rock) SDL_DestroyTexture(t.rock);
    if (t.ore)  SDL_DestroyTexture(t.ore);

    t = {};
}