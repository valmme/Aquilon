#include "inv/inventory.h"
#include "inv/crafting.h"
#include "textrenderer.h"
#include "vmath.h"
#include "localization.h"
#include <cstdio>

static constexpr float SLOT_SIZE      = 35.0f;
static constexpr float SLOT_SPACING   = 0.0f;
static constexpr int   INVENTORY_COLS = 10;
static constexpr int   INVENTORY_ROWS = 9;

static constexpr float CRAFT_PANEL_W = 260.0f;

static constexpr float INV_WIN_W = INVENTORY_COLS * SLOT_SIZE + (INVENTORY_COLS + 1) * SLOT_SPACING + SLOT_SPACING;
static constexpr float INV_WIN_H = INVENTORY_ROWS * SLOT_SIZE + (INVENTORY_ROWS + 1) * SLOT_SPACING + SLOT_SIZE;

Inventory::Inventory(GUIEngine& gui, Textures tex, TTF_Font* font, const InputConfig& input) : gui(gui), font(font), tex(tex), input(input) {
    for (int j = 0; j < INVENTORY_ROWS; j++) {
        for (int i = 0; i < INVENTORY_COLS; i++) {
            float x = SLOT_SPACING + i * (SLOT_SIZE + SLOT_SPACING);
            float y = SLOT_SPACING + j * (SLOT_SIZE + SLOT_SPACING);
            slots.emplace_back(Vec2{x, y});
        }
    }
}

Inventory::~Inventory() {
    delete cursor_item;
    for (Slot& slot : slots) delete slot.item;
}

void Inventory::open_window() {
    window = gui.CreateInvWindow(SDL_FRect{100, 150, INV_WIN_W + CRAFT_PANEL_W, INV_WIN_H}, Localize("Inventory"));

    window->SetContentDrawCallback([this](SDL_Renderer* renderer, const SDL_FRect& content_rect) {
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

                    float bg_w = 140.0f;
                    float bg_h = (float)TTF_GetFontHeight(font) + 4.0f;
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
                    SDL_SetRenderDrawColor(renderer, 8, 9, 11, 210);
                    SDL_RenderFillRect(renderer, &bg);

                    TextRenderer::DrawText(renderer, font, bg.x + 4.0f, bg.y + 2.0f, buf, SDL_Color{235, 237, 241, 255});
                }
            }
        }

        craft_rect = {
            content_rect.x + INV_WIN_W,
            content_rect.y,
            CRAFT_PANEL_W,
            content_rect.h
        };

        if (crafting) {
            crafting->draw_panel(renderer, craft_rect, *this);
        }

        SDL_SetRenderClipRect(renderer, nullptr);
    });

    window->SetCloseCallback([this]() {
        open = false;
        window = nullptr;
    });
}

void Inventory::close_window() {
    gui.CloseInvWindow();
    open = false;
    window = nullptr;
}

void Inventory::handle_event(const SDL_Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
        if (open && KeyBindMatches(input.inventory_close, e.key.key)) {
            close_window();
            return;
        }

        if (KeyBindMatches(input.inventory_toggle, e.key.key)) {
            open = !open;
            if (open) open_window();
            else close_window();
            return;
        }
    }

    if (!open) return;

    if (e.type == SDL_EVENT_KEY_DOWN && e.key.repeat) {
        return;
    }

    for (Slot& slot : slots) slot.update(cursor_item, e);

    for (Slot& slot : slots) {
        if (slot.item && slot.item->amount <= 0) {
            delete slot.item;
            slot.item = nullptr;
        }
    }

    if (crafting) {
        crafting->handle_event(e, *this, craft_rect);
    }
}

void Inventory::update(float mx, float my) {
    if (cursor_item) {
        cursor_item->dest.w = SLOT_SIZE;
        cursor_item->dest.h = SLOT_SIZE;
        cursor_item->dest.x = mx - cursor_item->dest.w * 0.5f;
        cursor_item->dest.y = my - cursor_item->dest.h * 0.5f;
    }

    if (!open) return;

    for (Slot& slot : slots)
        slot.selected = PointInRec(mx, my, slot.dest);

    if (crafting) {
        crafting->select_by_mouse(mx, my, craft_rect);
    }
}

void Inventory::draw(SDL_Renderer* renderer, TTF_Font* font) const {
    if (cursor_item && font) {
        cursor_item->draw(renderer);

        char buf[16];
        snprintf(buf, sizeof(buf), "%d", cursor_item->amount);

        SDL_Color white = {236, 238, 242, 255};
        SDL_FRect bg = {
            cursor_item->dest.x + cursor_item->dest.w - 18.0f,
            cursor_item->dest.y + cursor_item->dest.h - (float)TTF_GetFontHeight(font) - 4.0f,
            18.0f,
            (float)TTF_GetFontHeight(font) + 4.0f
        };

        SDL_SetRenderDrawColor(renderer, 8, 9, 11, 210);
        SDL_RenderFillRect(renderer, &bg);

        TextRenderer::DrawText(renderer, font, bg.x + 3.0f, bg.y + 2.0f, buf, white);
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

bool Inventory::is_dragging_placeable() const {
    return cursor_item && cursor_item->can_place;
}


const Item* Inventory::get_dragged_item() const {
    return cursor_item;
}

Item* Inventory::release_cursor_item() {
    Item* out = cursor_item;
    cursor_item = nullptr;
    return out;
}

void Inventory::set_cursor_item(Item* item) {
    delete cursor_item;
    cursor_item = item;
}
