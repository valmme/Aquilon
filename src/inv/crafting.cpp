#include "inv/crafting.h"
#include "inv/crafting_queue.h"
#include "inv/inventory.h"
#include "textrenderer.h"
#include <cstdio>
#include <climits>
#include <algorithm>

static constexpr float PANEL_PAD  = 8.0f;
static constexpr float SLOT_W     = 42.0f;
static constexpr float SLOT_H     = 42.0f;
static constexpr float SLOT_GAP   = 4.0f;
static constexpr int   GRID_COLS  = 4;

static int craft_count(const Inventory& inv, const Recipe& r) {
    int count = INT_MAX;
    
    for (const auto& ing : r.ingredients) {
        int have = inv.get_amount(ing.type);
        count = std::min(count, have / ing.amount);
    }

    return (count == INT_MAX) ? 0 : count;
}

static SDL_FRect slot_rect(const SDL_FRect& panel, int i) {
    int col = i % GRID_COLS;
    int row = i / GRID_COLS;

    return {
        panel.x + PANEL_PAD + col * (SLOT_W + SLOT_GAP),
        panel.y + PANEL_PAD + 18.0f + row * (SLOT_H + SLOT_GAP),
        SLOT_W,
        SLOT_H
    };
}

CraftingSystem::CraftingSystem(Textures& tex, TTF_Font* font)
    : tex(tex), font(font) {}

const char* CraftingSystem::item_type_name(ItemType type) {
    switch (type) {
        case ItemType::STONE: return "Stone";
        case ItemType::IRON_ORE: return "Iron Ore";
        case ItemType::FURNACE: return "Furnace";
        default: return "Unknown";
    }
}

void CraftingSystem::initialize_recipes(Textures* tex) {
    add_recipe(Recipe{
        "Furnace",
        1,
        tex->furnace,
        2.5f,
        ItemType::FURNACE,
        {
            { ItemType::STONE, 5 }
        }
    });

    add_recipe(Recipe{
        "Drill",
        1,
        tex->drill,
        3.0f,
        ItemType::DRILL,
        {
            { ItemType::IRON_PLATE, 3 }, { ItemType::FURNACE, 1 }
        }
    });
}

void CraftingSystem::add_recipe(const Recipe& recipe) {
    recipes.push_back(recipe);
    if (selected < 0) selected = 0;
}

bool CraftingSystem::can_craft(const Inventory& inv, const Recipe& recipe) const {
    for (const auto& ing : recipe.ingredients) {
        if (inv.get_amount(ing.type) < ing.amount) return false;
    }
    return true;
}

bool CraftingSystem::craft_selected(Inventory& inv) {
    if (selected < 0 || selected >= (int)recipes.size()) return false;

    const Recipe& r = recipes[selected];
    return queue.enqueue(r, inv, recipes);
}

void CraftingSystem::update(float delta_time, Inventory& inv) {
    queue.update(delta_time, inv);
}

bool CraftingSystem::has_queue() const {
    return !queue.empty();
}

void CraftingSystem::select_by_mouse(float mx, float my, const SDL_FRect& panel_rect) {
    hovered = -1;
    for (int i = 0; i < (int)recipes.size(); ++i) {
        SDL_FRect r = slot_rect(panel_rect, i);
        if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) {
            hovered = i;
            return;
        }
    }
}

void CraftingSystem::handle_event(const SDL_Event& e, Inventory& inv, const SDL_FRect& panel_rect) {
    if (e.type != SDL_EVENT_MOUSE_BUTTON_DOWN || e.button.button != SDL_BUTTON_LEFT) return;
    float mx = (float)e.button.x;
    float my = (float)e.button.y;

    for (int i = 0; i < (int)recipes.size(); ++i) {
        SDL_FRect r = slot_rect(panel_rect, i);
        if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) {
            selected = i;
            craft_selected(inv);
            return;
        }
    }
}

void CraftingSystem::draw_panel(SDL_Renderer* renderer, const SDL_FRect& panel_rect, const Inventory& inv) const {
    if (!renderer) return;

    SDL_SetRenderDrawColor(renderer, 14, 15, 18, 255);
    SDL_RenderFillRect(renderer, &panel_rect);
    SDL_SetRenderDrawColor(renderer, 40, 44, 52, 255);
    SDL_RenderRect(renderer, &panel_rect);

    SDL_Color white = {238, 240, 243, 255};
    SDL_Color muted = {120, 126, 134, 255};

    TextRenderer::DrawText(renderer, font, panel_rect.x + PANEL_PAD, panel_rect.y + 4.0f, "Crafting", white);

    if (recipes.empty()) {
        TextRenderer::DrawText(renderer, font, panel_rect.x + PANEL_PAD, panel_rect.y + 28.0f, "No recipes", muted);
        return;
    }

    for (int i = 0; i < (int)recipes.size(); ++i) {
        const Recipe& r = recipes[i];
        SDL_FRect slot = slot_rect(panel_rect, i);
        int cnt = craft_count(inv, r);
        bool can = (cnt > 0);
        bool sel = (i == selected);
        bool hov = (i == hovered);

        if (tex.crafting_slot) {
            SDL_RenderTexture(renderer, tex.crafting_slot, nullptr, &slot);
        } 
        
        else {
            SDL_SetRenderDrawColor(renderer, 22, 24, 29, 255);
            SDL_RenderFillRect(renderer, &slot);
        }

        if (!can) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 8, 8, 10, 180);
            SDL_RenderFillRect(renderer, &slot);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        } 
        
        else if (sel || hov) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, sel ? 30 : 15);
            SDL_RenderFillRect(renderer, &slot);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }

        if (r.result_texture) {
            float pad = 6.0f;
            SDL_FRect icon = { slot.x + pad, slot.y + pad, slot.w - pad * 2, slot.h - pad * 2 };
            SDL_SetTextureScaleMode(r.result_texture, SDL_SCALEMODE_NEAREST);

            if (!can) SDL_SetTextureAlphaMod(r.result_texture, 80);
            SDL_RenderTexture(renderer, r.result_texture, nullptr, &icon);
            if (!can) SDL_SetTextureAlphaMod(r.result_texture, 255);
        }

        if (font) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", cnt);

            float fh = (float)TTF_GetFontHeight(font);

            float bw = (cnt >= 10) ? 20.0f : 13.0f;
            SDL_FRect nb = {
                slot.x + slot.w - bw - 2.0f,
                slot.y + slot.h - fh - 2.0f,
                bw, fh
            };
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 5, 5, 7, 180);
            SDL_RenderFillRect(renderer, &nb);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            SDL_Color cnt_col = can
                ? SDL_Color{220, 196, 134, 255}
                : SDL_Color{100, 104, 110, 255};

            TextRenderer::DrawText(renderer, font, nb.x + 2.0f, nb.y, buf, cnt_col);
        }

        if (sel) {
            SDL_SetRenderDrawColor(renderer, 220, 196, 134, 200);
            SDL_RenderRect(renderer, &slot);
        }
    }

    if (hovered >= 0 && hovered < (int)recipes.size() && font) {
        const Recipe& r = recipes[hovered];
        SDL_FRect slot = slot_rect(panel_rect, hovered);

        const char* name = r.name.c_str();
        int tw = 0, th = 0;
        TTF_GetStringSize(font, name, 0, &tw, &th);

        char time_buf[64];
        snprintf(time_buf, sizeof(time_buf), "Time: %.1fs", r.craft_duration);
        int ttw = 0, tth = 0;
        TTF_GetStringSize(font, time_buf, 0, &ttw, &tth);

        float pad = 4.0f;
        float tip_w = std::max((float)tw, (float)ttw) + pad * 2;
        float tip_h = (float)th + (float)tth + pad * 3;
        float tip_x = slot.x + slot.w * 0.5f - tip_w * 0.5f;
        float tip_y = slot.y - tip_h - 4.0f;

        if (tip_x < panel_rect.x + PANEL_PAD) tip_x = panel_rect.x + PANEL_PAD;
        if (tip_x + tip_w > panel_rect.x + panel_rect.w - PANEL_PAD)
            tip_x = panel_rect.x + panel_rect.w - PANEL_PAD - tip_w;

        SDL_FRect bg = { tip_x, tip_y, tip_w, tip_h };
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 10, 11, 14, 220);
        SDL_RenderFillRect(renderer, &bg);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 60, 65, 75, 255);
        SDL_RenderRect(renderer, &bg);

        TextRenderer::DrawText(renderer, font, tip_x + pad, tip_y + pad, name, {238, 240, 243, 255});
        TextRenderer::DrawText(renderer, font, tip_x + pad, tip_y + pad + (float)th + pad, time_buf, {170, 176, 184, 255});
    }
}

void CraftingSystem::draw_queue(SDL_Renderer* renderer, TTF_Font* font, const SDL_FRect& screen_rect) const {
    queue.draw(renderer, font, screen_rect);
}
