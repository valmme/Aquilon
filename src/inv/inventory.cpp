#include "inv/inventory.h"
#include "vmath.h"
#include <cstdio>
#include <cstring>

static constexpr float SLOT_SIZE      = 25.0f;
static constexpr float SLOT_SPACING   = 0.0f;
static constexpr int   INVENTORY_COLS = 10;
static constexpr int   INVENTORY_ROWS = 9;

static constexpr float INV_WIN_W = INVENTORY_COLS * SLOT_SIZE + (INVENTORY_COLS + 1) * SLOT_SPACING + SLOT_SPACING;
static constexpr float INV_WIN_H = INVENTORY_ROWS * SLOT_SIZE + (INVENTORY_ROWS + 1) * SLOT_SPACING + SLOT_SIZE;

Inventory::Inventory(GUIEngine& gui, Textures tex, TTF_Font* font) : gui(gui), font(font), tex(tex) {
    for (int j = 0; j < INVENTORY_ROWS; j++) {
        for (int i = 0; i < INVENTORY_COLS; i++) {
            float x = SLOT_SPACING + i * (SLOT_SIZE + SLOT_SPACING);
            float y = SLOT_SPACING + j * (SLOT_SIZE + SLOT_SPACING);
            slots.emplace_back(x, y);
        }
    }
}

Inventory::~Inventory() {
    delete cursor_item;
    for (Slot& slot : slots) delete slot.item;
}

void Inventory::open_window() {
    window = gui.create_inv_window(200, 200, INV_WIN_W, INV_WIN_H, "Inventory");

    window->set_content_draw_callback([this](SDL_Renderer* renderer, const SDL_FRect& content_rect) {
        SDL_Rect clip = {
            (int)content_rect.x,
            (int)content_rect.y,
            (int)content_rect.w,
            (int)content_rect.h
        };

        SDL_SetRenderClipRect(renderer, &clip);

        for (int i = 0; i < (int)slots.size(); i++) {
            int col = i % INVENTORY_COLS;
            int row = i / INVENTORY_COLS;

            slots[i].dest.x = content_rect.x + SLOT_SPACING + col * (SLOT_SIZE + SLOT_SPACING);
            slots[i].dest.y = content_rect.y + SLOT_SPACING + row * (SLOT_SIZE + SLOT_SPACING);
            slots[i].dest.w = SLOT_SIZE;
            slots[i].dest.h = SLOT_SIZE;
        }

        for (const Slot& slot : slots) {
            slot.draw(renderer, tex, font);
        }

        if (font) {
            for (const Slot& slot : slots) {
                if (slot.selected && slot.item) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "%s (%d)", slot.item->name.c_str(), slot.item->amount);

                    SDL_Surface* surf = TTF_RenderText_Blended(font, buf, strlen(buf), SDL_Color{255, 255, 255, 255});
                    if (surf) {
                        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);

                        if (tex) {
                            float bg_w = (float)surf->w + 8.0f;
                            float bg_h = (float)surf->h + 4.0f;
                            float bg_x = slot.dest.x + slot.dest.w + 6.0f;
                            float bg_y = slot.dest.y - bg_h - 4.0f;

                            if (bg_y < content_rect.y) {
                                bg_y = slot.dest.y + slot.dest.h + 4.0f;
                            }

                            float max_x = content_rect.x + content_rect.w - bg_w;
                            float max_y = content_rect.y + content_rect.h - bg_h;
                            if (bg_x < content_rect.x) bg_x = content_rect.x;
                            if (bg_y < content_rect.y) bg_y = content_rect.y;
                            if (bg_x > max_x) bg_x = max_x;
                            if (bg_y > max_y) bg_y = max_y;

                            SDL_FRect bg = {bg_x, bg_y, bg_w, bg_h};
                            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                            SDL_RenderFillRect(renderer, &bg);

                            SDL_FRect tdst = {bg.x + 4, bg.y + 2, (float)surf->w, (float)surf->h};
                            SDL_RenderTexture(renderer, tex, nullptr, &tdst);
                            SDL_DestroyTexture(tex);
                        }

                        SDL_DestroySurface(surf);
                    }
                }
            }
        }

        SDL_SetRenderClipRect(renderer, nullptr);
    });

    window->set_close_callback([this]() {
        open = false;
        window = nullptr;
    });
}

void Inventory::close_window() {
    gui.close_inv_window();
    open = false;
    window = nullptr;
}

void Inventory::handle_event(const SDL_Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
        if (open) close_window();
        return;
    }

    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_E) {
        open = !open;
        if (open) open_window();
        else close_window();
        return;
    }

    if (!open) return;

    for (Slot& slot : slots) slot.update(cursor_item, e);

    for (Slot& slot : slots) {
        if (slot.item && slot.item->amount <= 0) {
            delete slot.item;
            slot.item = nullptr;
        }
    }
}

void Inventory::update(float mx, float my) {
    if (cursor_item) {
        cursor_item->dest.w = 30.0f;
        cursor_item->dest.h = 30.0f;
        cursor_item->dest.x = mx - cursor_item->dest.w * 0.5f;
        cursor_item->dest.y = my - cursor_item->dest.h * 0.5f;
    }

    if (!open) return;

    for (Slot& slot : slots)
        slot.selected = point_in_rec(mx, my, slot.dest);
}

void Inventory::draw(SDL_Renderer* renderer, TTF_Font* font) const {
    if (cursor_item && font) {
        cursor_item->draw(renderer);

        char buf[16];
        snprintf(buf, sizeof(buf), "%d", cursor_item->amount);

        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface* surf = TTF_RenderText_Blended(font, buf, strlen(buf), white);
        if (!surf) return;

        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
        if (!tex) {
            SDL_DestroySurface(surf);
            return;
        }

        SDL_FRect bg = {
            cursor_item->dest.x + cursor_item->dest.w - surf->w - 4,
            cursor_item->dest.y + cursor_item->dest.h - surf->h - 4,
            (float)surf->w + 6,
            (float)surf->h + 4
        };

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_RenderFillRect(renderer, &bg);

        SDL_FRect tdst = {
            bg.x + 3,
            bg.y + 2,
            (float)surf->w,
            (float)surf->h
        };

        SDL_RenderTexture(renderer, tex, nullptr, &tdst);

        SDL_DestroyTexture(tex);
        SDL_DestroySurface(surf);
    }
}

void Inventory::pick(Item* item) {
    for (Slot& slot : slots) {
        if (slot.item && slot.item->type == item->type) {
            slot.item->amount += item->amount;
            delete item;
            return;
        }
    }

    for (Slot& slot : slots) {
        if (!slot.item) { slot.item = item; return; }
    }

    delete item;
}

void Inventory::remove(ItemType type, int amount) {
    for (Slot& slot : slots) {
        if (!slot.item || slot.item->type != type) continue;

        if (slot.item->amount >= amount) {
            slot.item->amount -= amount;
            if (slot.item->amount <= 0) { delete slot.item; slot.item = nullptr; }
            return;
        } 
        
        else {
            amount -= slot.item->amount;
            delete slot.item;
            slot.item = nullptr;
        }
    }
}

int Inventory::get_amount(ItemType type) const {
    int total = 0;
    for (const Slot& slot : slots)
        if (slot.item && slot.item->type == type) total += slot.item->amount;

    return total;
}
