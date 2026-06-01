#include "inv/inventory.h"
#include "vmath.h"
#include <cstdio>

Inventory::Inventory() {
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 5; i++) {
            slots.emplace_back(i * 55 + 5, j * 55 + 5);
        }
    }
}

Inventory::~Inventory() {
    delete cursor_item;
    for (Slot& slot : slots) delete slot.item;
}

void Inventory::handle_event(const SDL_Event& e) {
    for (Slot& slot : slots) slot.update(cursor_item, e);

    for (Slot& slot : slots) {
        if (slot.item && slot.item->amount <= 0) {
            delete slot.item;
            slot.item = nullptr;
        }
    }
}

void Inventory::update(float mx, float my) {
    for (Slot& slot : slots) {
        slot.selected = point_in_rec(mx, my, slot.dest);
    }

    if (cursor_item) {
        cursor_item->dest.x = mx - 15;
        cursor_item->dest.y = my - 15;
    }
}

void Inventory::draw(SDL_Renderer* renderer, TTF_Font* font) const {
    for (const auto& slot : slots) slot.draw(renderer, font);

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

    if (cursor_item) cursor_item->draw(renderer);
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
        if (!slot.item) {
            slot.item = item;
            return;
        }
    }

    delete item;    
}

void Inventory::remove(ItemType type, int amount) {
    for (Slot& slot : slots) {
        if (!slot.item || slot.item->type != type) continue;
        if (slot.item->amount >= amount) {
            slot.item->amount -= amount;

            if (slot.item->amount <= 0) {
                delete slot.item;
                slot.item = nullptr;
            }

            else {
                amount -= slot.item->amount;
                delete slot.item;
                slot.item = nullptr;
            }
        }
    }
}

int Inventory::get_amount(ItemType type) const {
    int total = 0;
    for (const Slot& slot: slots) {
        if (slot.item && slot.item->type == type) total += slot.item->amount;
    }

    return total;
}