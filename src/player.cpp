#include "player.h"

Player::Player(const InputConfig& input) : input(input) {
    player = { 0.0f, 0.0f, 32, 32 };
    speed = 250.0f;

    up = down = left = right = false;

    anim_frame = 0;
    anim_timer = 0;
    anim_speed = 0.1f;
}

void Player::handle_input(const SDL_Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN) {
        if (KeyBindMatches(input.move_up, e.key.key)) up = true;
        if (KeyBindMatches(input.move_down, e.key.key)) down = true;
        if (KeyBindMatches(input.move_left, e.key.key)) left = true;
        if (KeyBindMatches(input.move_right, e.key.key)) right = true;
    }

    if (e.type == SDL_EVENT_KEY_UP) {
        if (KeyBindMatches(input.move_up, e.key.key)) up = false;
        if (KeyBindMatches(input.move_down, e.key.key)) down = false;
        if (KeyBindMatches(input.move_left, e.key.key)) left = false;
        if (KeyBindMatches(input.move_right, e.key.key)) right = false;
    }
}

void Player::update(float delta_time) {
    if (up)    player.y -= speed * delta_time;
    if (down)  player.y += speed * delta_time;
    if (left)  player.x -= speed * delta_time;
    if (right) player.x += speed * delta_time;

    update_animation(delta_time);
}

void Player::update_animation(float delta_time) {
    if (up || down || left || right) {
        anim_timer += delta_time;

        if (anim_timer >= anim_speed) {
            anim_timer = 0.0f;
            anim_frame = (anim_frame + 1) % 4;
        }

    } 
    
    else {
        anim_frame = 0;
    }
}


void Player::render(SDL_Renderer* renderer, const Camera& cam) {
    SDL_FRect screen_rect = cam.WorldToScreenRect(player.x, player.y, player.w, player.h);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &screen_rect);
}