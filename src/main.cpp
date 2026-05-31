#include <SDL3/SDL.h>
#include <cstdio>
#include "include/player.h"

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

    SDL_SetRenderVSync(renderer, true);

    Player player;

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

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        player.render(renderer);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}