#include <SDL3/SDL.h>
#include "include/player.h"

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;

    SDL_Window* window = SDL_CreateWindow("Aquilon", 800, 600, 0);
    if (!window) {
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    Player player;

    Uint64 last = SDL_GetTicks();

    bool running = true;
    SDL_Event e;

    while (running) {
        Uint64 now = SDL_GetTicks();
        float dt = (now - last) / 1000.0f;
        last = now;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;

            player.handle_input(e);
        }

        player.update(dt);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        player.render(renderer);

        SDL_RenderPresent(renderer);

        SDL_Delay(1);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}