#include "SDL3/SDL.h"

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Aquilon", 800, 600, 1);

    SDL_Event e;
    bool running = true;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
}