#ifndef AQUILON_PLAYER_H
#define AQUILON_PLAYER_H

#include "SDL3/SDL.h"
#include "vmath.h"

struct Camera {
    float x = 0;
    float y = 0;

    float zoom = 1.0f;

    float deadzone_w = 120.0f;
    float deadzone_h = 90.0f;
    
    float follow_lerp_moving = 0.08f;
    float follow_lerp_stopped = 0.22f;
    float last_player_cx = 0.0f;
    float last_player_cy = 0.0f;

    void update(SDL_FRect player, float screen_cx, float screen_cy) {
        float player_cx = player.x + player.w * 0.5f;
        float player_cy = player.y + player.h * 0.5f;

        float desired_x = player_cx - (screen_cx) / zoom;
        float desired_y = player_cy - (screen_cy) / zoom;

        float dx = player_cx - last_player_cx;
        float dy = player_cy - last_player_cy;
        float move_sq = dx*dx + dy*dy;
        bool moving = move_sq > 1e-4f;

        float lerp = moving ? follow_lerp_moving : follow_lerp_stopped;

        x += (desired_x - x) * lerp;
        y += (desired_y - y) * lerp;

        last_player_cx = player_cx;
        last_player_cy = player_cy;
    }

    SDL_FPoint WorldToScreen(float world_x, float world_y) const {
        SDL_FPoint p;
        p.x = (world_x - x) * zoom;
        p.y = (world_y - y) * zoom;
        return p;
    }

    SDL_FRect WorldToScreenRect(float world_x, float world_y, float w, float h) const {
        SDL_FPoint p = WorldToScreen(world_x, world_y);
        SDL_FRect r;
        r.x = p.x;
        r.y = p.y;
        r.w = w * zoom;
        r.h = h * zoom;
        return r;
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

#endif // AQUILON_PLAYER_H
