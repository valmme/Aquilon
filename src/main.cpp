#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cstdio>
#include "player.h"
#include "textures.h"
#include "gen/world.h"

const int TILE_SIZE = 32;
const int MAP_W = 32;
const int MAP_H = 32;

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;

    SDL_Window* window = SDL_CreateWindow("Aquilon", 800, 600, 0);
    if (!window) {
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Textures tex = load_textures(renderer);

    World world;
    Player player;
    Camera cam;

    Uint64 last_counter = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    float delta_time = 0.0f;

    bool running = true;
    SDL_Event e;

    while (running) {
        Uint64 current_counter = SDL_GetPerformanceCounter();
        delta_time = (float)(current_counter - last_counter) / frequency;
        last_counter = current_counter;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;

            player.handle_input(e);
        }

        player.update(delta_time);
        world.update((int)player.player.x, (int)player.player.y);
        cam.update(player.player);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (int y = 0; y < MAP_H; y++) {
            for (int x = 0; x < MAP_W; x++) {

                Tile t = world.get_tile(x, y);

                SDL_Texture* current = tex.ice;

                if (t.type == ROCK) current = tex.rock;
                else if (t.type == ORE) current = tex.ore;

                SDL_FRect dst = {
                    x * TILE_SIZE - cam.x,
                    y * TILE_SIZE - cam.y,
                    TILE_SIZE,
                    TILE_SIZE
                };

                SDL_RenderTexture(renderer, current, NULL, &dst);
            }
        }

        player.render(renderer, cam);
        SDL_RenderPresent(renderer);
    }

    free_textures(tex);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}