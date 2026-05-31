#pragma once
#include "SDL3/SDL.h"
#include "vmath.h"

struct Camera {
    float x = 0;
    float y = 0;

    float deadzone_w = 120.0f;
    float deadzone_h = 90.0f;

    void update(SDL_FRect player) {
        float screen_px = player.x + player.w * 0.5f - x;
        float screen_py = player.y + player.h * 0.5f - y;

        float box_left   = 400.0f - deadzone_w * 0.5f;
        float box_right  = 400.0f + deadzone_w * 0.5f;
        float box_top    = 300.0f - deadzone_h * 0.5f;
        float box_bottom = 300.0f + deadzone_h * 0.5f;

        if (screen_px < box_left)   x -= (box_left   - screen_px);
        if (screen_px > box_right)  x += (screen_px  - box_right);
        if (screen_py < box_top)    y -= (box_top     - screen_py);
        if (screen_py > box_bottom) y += (screen_py   - box_bottom);
    }
};

class Player {
public:
    Player();

    void handle_input(const SDL_Event& e);
    void update(float delta_time);
    void render(SDL_Renderer* renderer, const Camera& cam);

    SDL_FRect player;

private:
    float speed;

    bool up, down, left, right;

    int anim_frame;
    float anim_timer;
    float anim_speed;

    void update_animation(float delta_time);
};