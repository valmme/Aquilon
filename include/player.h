#ifndef AQUILON_PLAYER_H
#define AQUILON_PLAYER_H

#include "SDL3/SDL.h"
#include "vmath.h"
#include "config.h"
#include "gen/tile.h"

class World;
class Inventory;
struct Textures;
struct Item;

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

    SDL_FPoint world_to_screen(float world_x, float world_y) const {
        SDL_FPoint p;
        p.x = (world_x - x) * zoom;
        p.y = (world_y - y) * zoom;
        return p;
    }

    SDL_FRect world_to_screen_rect(float world_x, float world_y, float w, float h) const {
        SDL_FPoint p = world_to_screen(world_x, world_y);
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
    struct MiningState {
        bool active = false;
        int tile_x = 0;
        int tile_y = 0;
        TileType tile_type = EMPTY;
        float duration = 0.0f;
        float progress = 0.0f;
    };

    explicit Player(const InputConfig& input = InputConfig{});

    void handle_input(const SDL_Event& e);
    void update(float delta_time);
    void render(SDL_Renderer* renderer, const Camera& cam);
    bool is_mining() const;
    void start_mining(int tile_x, int tile_y, TileType tile_type);
    void stop_mining();
    void update_mining(float delta_time, World& world, Inventory& inventory, const Textures& textures);
    void draw_mining_progress_bar(SDL_Renderer* renderer, int win_w, int win_h) const;

    SDL_FRect player;

private:
    InputConfig input;
    float speed;

    bool up, down, left, right;

    int anim_frame;
    float anim_timer;
    float anim_speed;
    MiningState mining;

    void update_animation(float delta_time);
    static float mining_duration_for(TileType type);
    static bool is_mineable(TileType type);
    static Item* make_drop_for_tile(const Tile& tile, const Textures& textures);
};

#endif // AQUILON_PLAYER_H
