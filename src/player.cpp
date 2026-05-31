#include "include/player.h"

Player::Player() {
    player = { 400, 300, 32, 32 };
    speed = 200.0f;

    up = down = left = right = false;

    anim_frame = 0;
    anim_timer = 0;
    anim_speed = 0.1f;
}

void Player::handle_input(const SDL_Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN) {
        if (e.key.key == SDLK_W) up = true;
        if (e.key.key == SDLK_S) down = true;
        if (e.key.key == SDLK_A) left = true;
        if (e.key.key == SDLK_D) right = true;
    }

    if (e.type == SDL_EVENT_KEY_UP) {
        if (e.key.key == SDLK_W) up = false;
        if (e.key.key == SDLK_S) down = false;
        if (e.key.key == SDLK_A) left = false;
        if (e.key.key == SDLK_D) right = false;
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


void Player::render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &player);
}