#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cstdio>
#include "player.h"
#include "textures.h"
#include "gen/world.h"

const int TILE_SIZE = 32;

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

    SDL_SetRenderVSync(renderer, 1);

    Textures tex = load_textures(renderer);

    World world;
    Player player;
    Camera cam;

    Uint64 last_counter = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    float delta_time = 0.0f;
    float fps = 0.0f;

    bool running = true;
    SDL_Event e;

    while (running) {
        Uint64 current_counter = SDL_GetPerformanceCounter();
        delta_time = (float)(current_counter - last_counter) / frequency;
        last_counter = current_counter;
        fps = 1.0f / delta_time;

        char title[128];
        snprintf(title, sizeof(title), "Aquilon - FPS: %.1f", fps);
        SDL_SetWindowTitle(window, title);
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;

            player.handle_input(e);
        }

        player.update(delta_time);

        int player_tile_x = (int)player.player.x / TILE_SIZE;
        int player_tile_y = (int)player.player.y / TILE_SIZE;

        world.update(player_tile_x, player_tile_y);
        cam.update(player.player);

        SDL_SetRenderDrawColor(renderer, 50, 130, 230, 255);
        SDL_RenderClear(renderer);

        for (auto& [key, chunk] : world.get_chunks()) {
            for (int ty = 0; ty < CHUNK_SIZE; ty++) {
                for (int tx = 0; tx < CHUNK_SIZE; tx++) {
                    int world_x = chunk.pos.x * CHUNK_SIZE + tx;
                    int world_y = chunk.pos.y * CHUNK_SIZE + ty;

                    Tile t = chunk.tiles[tx][ty];

                    SDL_Texture* current = tex.ice;
                    if (t.type == ROCK) current = tex.rock;
                    else if (t.type == SNOW) current = tex.snow;
                    else if (t.type == ORE)  current = tex.ore;

                    SDL_FRect dst = {
                        world_x * TILE_SIZE - cam.x,
                        world_y * TILE_SIZE - cam.y,
                        (float)TILE_SIZE,
                        (float)TILE_SIZE
                    };

                    SDL_RenderTexture(renderer, current, NULL, &dst);
                }
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