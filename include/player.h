#pragma once
#include "SDL3/SDL.h"

class Player {
public:
    Player();

    void handle_input(const SDL_Event& e);
    void update(float delta_time);
    void render(SDL_Renderer* renderer);

private:
    float x, y;
    float speed;
};