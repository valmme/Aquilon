#include "player.h"

#include "inv/inventory.h"
#include "inv/item.h"
#include "textures.h"
#include "gen/world.h"
#include <cmath>

Player::Player(const InputConfig& input) : input(input) {
    player = { 0.0f, 0.0f, 32, 32 };
    speed = 250.0f;

    up = down = left = right = false;

    anim_frame = 0;
    anim_timer = 0;
    anim_speed = 0.1f;
    placement_rotation = 0.0f;
}

void Player::handle_input(const SDL_Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN) {
        if (KeyBindMatches(input.move_up, e.key.key)) up = true;
        if (KeyBindMatches(input.move_down, e.key.key)) down = true;
        if (KeyBindMatches(input.move_left, e.key.key)) left = true;
        if (KeyBindMatches(input.move_right, e.key.key)) right = true;

        if (KeyBindMatches(input.rotate_placement, e.key.key)) {
            placement_rotation += 90.0f;
            if (placement_rotation >= 360.0f) {
                placement_rotation = 0.0f;
            }
        }
    }

    if (e.type == SDL_EVENT_KEY_UP) {
        if (KeyBindMatches(input.move_up, e.key.key)) up = false;
        if (KeyBindMatches(input.move_down, e.key.key)) down = false;
        if (KeyBindMatches(input.move_left, e.key.key)) left = false;
        if (KeyBindMatches(input.move_right, e.key.key)) right = false;
    }
}

void Player::handle_drop(Inventory& inv, float mw_x, float mw_y) {
    if (!inv.cursor_item || !drop_system) return;

    Item* single = inv.cursor_item->copy();
    single->amount = 1;

    drop_system->spawn(single, mw_x, mw_y);
    inv.consume_cursor_item_one();
}

void Player::handle_pickup(Inventory& inv) {
    if (!drop_system) return;

    float cx = player.x + player.w * 0.5f;
    float cy = player.y + player.h * 0.5f;

    for (Item* i : drop_system->pickup_near(cx, cy, PICKUP_RADIUS)) {
        inv.pick(i);
    }
}

bool Player::can_place_at(const std::vector<PlacedObject>& placed_objects, int x, int y, vec2 size) {
    for (const PlacedObject& obj : placed_objects) {
        int ax1 = obj.x;
        int ay1 = obj.y;
        int ax2 = obj.x + (int)obj.size.x - 1;
        int ay2 = obj.y + (int)obj.size.y - 1;

        int bx1 = x;
        int by1 = y;
        int bx2 = x + (int)size.x - 1;
        int by2 = y + (int)size.y - 1;

        bool overlap = !(bx2 < ax1 || bx1 > ax2 || by2 < ay1 || by1 > ay2);
        if (overlap) return false;
    }
    return true;
}

bool Player::can_place_item_at(ItemType type, World& world, int x, int y, vec2 size, const std::vector<PlacedObject>& placed_objects) {
    if (!can_place_at(placed_objects, x, y, size)) {
        return false;
    }

    if (type == ItemType::DRILL) {
        for (int dy = 0; dy < (int)size.y; ++dy) {
            for (int dx = 0; dx < (int)size.x; ++dx) {
                Tile tile = world.get_tile(x + dx, y + dy);
                if (tile.type != IRON_ORE || tile.yield <= 0) {
                    return false;
                }
            }
        }
        return true;
    }

    return true;
}

void Player::update_conveyors(DroppedItemSystem& drop_system, float delta_time) {
    for (auto& dropped : drop_system.get_items()) {
        int tx = (int)std::floor(dropped.x / 32.0f);
        int ty = (int)std::floor(dropped.y / 32.0f);
        for (const auto& obj : placed_objects) {
            if (obj.type == ItemType::CONVEYOR && obj.x == tx && obj.y == ty) {
                float move_speed = 64.0f;
                float rad = (obj.rotation - 90.0f) * (3.14159f / 180.0f);
                dropped.x += std::cos(rad) * move_speed * delta_time;
                dropped.y += std::sin(rad) * move_speed * delta_time;
            }
        }
    }
}

void Player::update_placed_drills(float delta_time, World& world, Inventory& inventory, const Textures& textures) {
    for (PlacedObject& obj : placed_objects) {
        if (obj.type != ItemType::DRILL) {
            continue;
        }

        bool has_ore = false;
        for (int dy = 0; dy < (int)obj.size.y && !has_ore; ++dy) {
            for (int dx = 0; dx < (int)obj.size.x; ++dx) {
                Tile tile = world.get_tile(obj.x + dx, obj.y + dy);
                if (tile.type == IRON_ORE && tile.yield > 0) {
                    has_ore = true;
                    break;
                }
            }
        }

        bool has_fuel = obj.fuel_remaining > 0.0f || obj.fuel_amount > 0;
        if (!has_ore || !has_fuel) {
            obj.mining_progress = 0.0f;
            continue;
        }

        if (obj.fuel_remaining <= 0.0f) {
            obj.fuel_remaining = 1.0f;
            obj.fuel_amount--;
            if (obj.fuel_amount < 0) obj.fuel_amount = 0;
        }

        obj.fuel_remaining -= delta_time * 0.2f;
        if (obj.fuel_remaining < 0.0f) obj.fuel_remaining = 0.0f;

        if (obj.fuel_remaining <= 0.0f) {
            obj.mining_progress = 0.0f;
            continue;
        }

        obj.mining_progress += delta_time;
        float duration = mining_duration_for(IRON_ORE);
        if (obj.mining_progress < duration) {
            continue;
        }

        obj.mining_progress -= duration;

        bool mined = false;
        for (int dy = 0; dy < (int)obj.size.y && !mined; ++dy) {
            for (int dx = 0; dx < (int)obj.size.x; ++dx) {
                int tx = obj.x + dx;
                int ty = obj.y + dy;
                Tile tile = world.get_tile(tx, ty);
                if (tile.type != IRON_ORE || tile.yield <= 0) {
                    continue;
                }

                if (Item* drop = make_drop_for_tile(tile, textures)) {
                    inventory.pick(drop);
                }

                tile.yield -= 1;
                if (tile.yield <= 0) {
                    tile = Tile{EMPTY, false, 0};
                }

                world.set_tile(tx, ty, tile);
                mined = true;
                break;
            }
        }

        if (!mined) {
            obj.mining_progress = 0.0f;
        }
    }
}

void Player::draw_placement_preview_texture(SDL_Renderer* renderer, const Camera& cam, const Item* item, float mouse_x, float mouse_y, bool can_place_here) const {
    if (!renderer || !item || !item->can_place || !item->texture) return;

    int tile_x = (int)std::floor((cam.x + mouse_x / cam.zoom) / 32.0f);
    int tile_y = (int)std::floor((cam.y + mouse_y / cam.zoom) / 32.0f);

    float world_x = tile_x * 32.0f;
    float world_y = tile_y * 32.0f;
    float world_w = item->size.x * 32.0f;
    float world_h = item->size.y * 32.0f;

    SDL_FRect dst = cam.world_to_screen_rect(world_x, world_y, world_w, world_h);

    SDL_SetTextureBlendMode(item->texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(item->texture, can_place_here ? 120 : 80);
    SDL_SetTextureColorMod(item->texture, can_place_here ? 90 : 255, can_place_here ? 255 : 80, can_place_here ? 120 : 80);
    SDL_RenderTextureRotated(renderer, item->texture, nullptr, &dst, (double)placement_rotation, nullptr, SDL_FLIP_NONE);

    SDL_SetTextureAlphaMod(item->texture, 255);
    SDL_SetTextureColorMod(item->texture, 255, 255, 255);
}

void Player::handle_item_placement(const SDL_Event& e, const Camera& cam, Inventory& inventory, World& world, bool allow_world_interaction) {
    if (!allow_world_interaction || inventory.open) return;
    if (e.type != SDL_EVENT_MOUSE_BUTTON_DOWN || e.button.button != SDL_BUTTON_LEFT) return;

    const int click_tile_x = (int)std::floor((cam.x + (float)e.button.x / cam.zoom) / 32.0f);
    const int click_tile_y = (int)std::floor((cam.y + (float)e.button.y / cam.zoom) / 32.0f);

    if (on_object_clicked) {
        for (PlacedObject& obj : placed_objects) {
            if (click_tile_x >= obj.x && click_tile_x < obj.x + (int)obj.size.x &&
                click_tile_y >= obj.y && click_tile_y < obj.y + (int)obj.size.y) {
                on_object_clicked(obj);
                return;
            }
        }
    }

    Item* dragged = inventory.cursor_item;
    if (!dragged || !dragged->can_place) {
        return;
    }

    if (!can_place_item_at(dragged->type, world, click_tile_x, click_tile_y, dragged->size, placed_objects)) {
        return;
    }

    PlacedObject obj;
    obj.type = dragged->type;
    obj.texture = dragged->texture;
    obj.x = click_tile_x;
    obj.y = click_tile_y;
    obj.size = dragged->size;
    obj.opens_inv = dragged->opens_inv;
    obj.rotation = placement_rotation;

    placed_objects.push_back(obj);
    inventory.consume_cursor_item_one();
}

void Player::update(Inventory& inv, Camera& cam, float delta_time, float mx, float my) {
    if (up)    player.y -= speed * delta_time;
    if (down)  player.y += speed * delta_time;
    if (left)  player.x -= speed * delta_time;
    if (right) player.x += speed * delta_time;

    update_animation(delta_time);
}

void Player::render_placed_objects(SDL_Renderer* renderer, const Camera& cam,
                                   int visible_min_tile_x, int visible_min_tile_y,
                                   int visible_max_tile_x, int visible_max_tile_y) const {
    for (const PlacedObject& obj : placed_objects) {
        const int obj_x1 = obj.x;
        const int obj_y1 = obj.y;
        const int obj_x2 = obj.x + (int)obj.size.x - 1;
        const int obj_y2 = obj.y + (int)obj.size.y - 1;
        if (obj_x2 < visible_min_tile_x || obj_x1 > visible_max_tile_x ||
            obj_y2 < visible_min_tile_y || obj_y1 > visible_max_tile_y) {
            continue;
        }

        if (!obj.texture) continue;

        SDL_FRect dst = cam.world_to_screen_rect(
            obj.x * 32.0f,
            obj.y * 32.0f,
            obj.size.x * 32.0f,
            obj.size.y * 32.0f
        );

        SDL_RenderTextureRotated(renderer, obj.texture, nullptr, &dst, (double)obj.rotation, nullptr, SDL_FLIP_NONE);
    }
}

void Player::draw_item_placement_preview(SDL_Renderer* renderer, const Camera& cam,
                                         const Inventory& inventory, World& world, float mouse_x, float mouse_y) const {
    const Item* dragged = inventory.cursor_item;
    if (!dragged || !dragged->can_place) {
        return;
    }

    int place_tile_x = (int)std::floor((cam.x + mouse_x / cam.zoom) / 32.0f);
    int place_tile_y = (int)std::floor((cam.y + mouse_y / cam.zoom) / 32.0f);

    bool can_place_here = can_place_item_at(dragged->type, world, place_tile_x, place_tile_y, dragged->size, placed_objects);
    draw_placement_preview_texture(renderer, cam, dragged, mouse_x, mouse_y, can_place_here);
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
    SDL_FRect screen_rect = cam.world_to_screen_rect(player.x, player.y, player.w, player.h);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &screen_rect);
}

bool Player::is_mining() const {
    return mining.active;
}

float Player::mining_duration_for(TileType type) {
    switch (type) {
        case STONE: return 0.80f;
        case IRON_ORE: return 1.10f;
        default: return 0.0f;
    }
}

bool Player::is_mineable(TileType type) {
    return type == STONE || type == IRON_ORE;
}

Item* Player::make_drop_for_tile(const Tile& tile, const Textures& textures) {
    switch (tile.type) {
        case STONE: return Item::IRON_ORE.copy();
        case IRON_ORE: return Item::IRON_ORE.copy();
        default:
            return nullptr;
    }
}

void Player::start_mining(int tile_x, int tile_y, TileType tile_type) {
    mining.active = true;
    mining.tile_x = tile_x;
    mining.tile_y = tile_y;
    mining.tile_type = tile_type;
    mining.duration = mining_duration_for(tile_type);
    mining.progress = 0.0f;
}

void Player::stop_mining() {
    mining.active = false;
    mining.progress = 0.0f;
}

void Player::update_mining(float delta_time, World& world, Inventory& inventory, const Textures& textures) {
    if (!mining.active) {
        return;
    }

    Tile mined_tile = world.get_tile(mining.tile_x, mining.tile_y);
    if (mined_tile.type != mining.tile_type || !is_mineable(mined_tile.type)) {
        stop_mining();
        return;
    }

    mining.progress += delta_time;
    if (mining.progress < mining.duration) {
        return;
    }

    Tile current = world.get_tile(mining.tile_x, mining.tile_y);
    if (current.yield > 0) {
        current.yield -= 1;

        if (Item* drop = make_drop_for_tile(mined_tile, textures)) {
            inventory.pick(drop);
        }

        if (current.yield <= 0) {
            current = Tile{EMPTY, false, 0};
        }

        world.set_tile(mining.tile_x, mining.tile_y, current);
    }

    stop_mining();
}

void Player::draw_mining_progress_bar(SDL_Renderer* renderer, int win_w, int win_h) const {
    if (!renderer || !mining.active || mining.duration <= 0.0f) {
        return;
    }

    float progress = mining.progress / mining.duration;
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    float bar_w = (float)(win_w - 48);
    if (bar_w > 420.0f) bar_w = 420.0f;
    if (bar_w < 0.0f) bar_w = 0.0f;
    const float bar_x = ((float)win_w - bar_w) * 0.5f;
    const float bar_y = (float)win_h - 20.0f;
    const float bar_h = 6.0f;

    SDL_SetRenderDrawColor(renderer, 12, 15, 18, 220);
    SDL_FRect bg = {bar_x, bar_y, bar_w, bar_h};
    SDL_RenderFillRect(renderer, &bg);

    SDL_SetRenderDrawColor(renderer, 58, 66, 76, 220);
    SDL_FRect top = {bar_x, bar_y, bar_w, 1.0f};
    SDL_FRect bottom = {bar_x, bar_y + bar_h - 1.0f, bar_w, 1.0f};
    SDL_RenderFillRect(renderer, &top);
    SDL_RenderFillRect(renderer, &bottom);

    const float fill_w = (bar_w - 2.0f) * progress;
    if (fill_w > 0.0f) {
        SDL_SetRenderDrawColor(renderer, 216, 176, 80, 255);
        SDL_FRect fill = {bar_x + 1.0f, bar_y + 1.0f, fill_w, bar_h - 2.0f};
        SDL_RenderFillRect(renderer, &fill);
    }
}
