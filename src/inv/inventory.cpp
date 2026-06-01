#include "inv/inventory.h"
#include "vmath.h"
#include <cstdio>

static constexpr float SLOT_SIZE      = 25.0f;
static constexpr float SLOT_SPACING   = 5.0f;
static constexpr int   INVENTORY_COLS = 10;
static constexpr int   INVENTORY_ROWS = 9;

static constexpr float INV_WIN_W = INVENTORY_COLS * SLOT_SIZE + (INVENTORY_COLS + 1) * SLOT_SPACING + SLOT_SPACING;
static constexpr float INV_WIN_H = INVENTORY_ROWS * SLOT_SIZE + (INVENTORY_ROWS + 1) * SLOT_SPACING + SLOT_SIZE;

Inventory::Inventory(GUIEngine& gui, TTF_Font* font) : gui(gui), font(font) {
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

        for (const Slot& slot : slots) slot.draw(renderer, font);

        if (font) {
            for (const Slot& slot : slots) {
                if (slot.selected && slot.item) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "%s (%d)", slot.item->name.c_str(), slot.item->amount);

                    SDL_Surface* surf = TTF_RenderText_Blended(font, buf, strlen(buf), SDL_Color{255, 255, 255, 255});
                    if (surf) {
                        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);

                        if (tex) {
                            SDL_FRect bg = {slot.dest.x + 20, slot.dest.y - 22, (float)surf->w + 8, (float)surf->h + 4};
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
        
        draw(renderer, font);
        SDL_SetRenderClipRect(renderer, nullptr);
    });

    window->set_close_callback([this]() {
        open = false;
        window = nullptr;
    });
}

void Inventory::close_window() {
    gui.close_inv_window();
    window = nullptr;
}

void Inventory::handle_event(const SDL_Event& e) {
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
    if (!open) return;

    for (Slot& slot : slots)
        slot.selected = point_in_rec(mx, my, slot.dest);

    if (cursor_item) {
        cursor_item->dest.x = mx - 15;
        cursor_item->dest.y = my - 15;
    }
}

void Inventory::draw(SDL_Renderer* renderer, TTF_Font* font) const {
    if (!open) return;

    for (const Slot& slot : slots)
        slot.draw(renderer, font);

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