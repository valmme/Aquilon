#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

struct Textures {
    SDL_Texture* none;

    SDL_Texture* ice;
    SDL_Texture* snow;
    SDL_Texture* rock;
    SDL_Texture* ore;
};

Textures load_textures(SDL_Renderer* renderer);
void free_textures(Textures& t);