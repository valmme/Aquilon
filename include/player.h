#pragma once
#include "SDL3/SDL.h"
#include "math.h"

class Player {
public:
    Player();

    void handle_input(const SDL_Event& e);
    void update(float delta_time);
    void render(SDL_Renderer* renderer);

private:
    SDL_FRect player;
    float speed;

    bool up, down, left, right;

    int anim_frame;
    float anim_timer;
    float anim_speed;

    void update_animation(float delta_time);
};