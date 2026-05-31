#include "textures.h"
#include <cstdio>

SDL_Texture* find_texture(SDL_Renderer* renderer, SDL_Texture* none, const char* file) {
    SDL_Texture* tex = IMG_LoadTexture(renderer, file);

    if (!tex) return none;
    return tex;
}

Textures load_textures(SDL_Renderer* renderer) {
    Textures t;

    t.none = IMG_LoadTexture(renderer, "resources/textures/none.png");

    t.ice  = find_texture(renderer, t.none, "resources/textures/ice.png");
    t.snow = find_texture(renderer, t.none, "resources/textures/snow.png");
    t.rock = find_texture(renderer, t.none, "resources/textures/rock.png");
    t.ore  = find_texture(renderer, t.none, "resources/textures/rock.png");

    return t;
}

void free_textures(Textures& t) {
    if (t.ice)  SDL_DestroyTexture(t.ice);
    if (t.snow) SDL_DestroyTexture(t.snow);
    if (t.rock) SDL_DestroyTexture(t.rock);
    if (t.ore)  SDL_DestroyTexture(t.ore);

    t = {};
}