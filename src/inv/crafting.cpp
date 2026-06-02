#include "inv/crafting.h"
#include "inv/inventory.h"
#include <cstdio>
#include <cstring>

static constexpr float PANEL_PAD = 8.0f;
static constexpr float LIST_ROW_H = 34.0f;
static constexpr float ICON_BOX = 24.0f;

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
        ItemType::FURNACE,
        {
            { ItemType::STONE, 5 }
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
    if (!can_craft(inv, r)) return false;

    for (const auto& ing : r.ingredients) {
        inv.remove(ing.type, ing.amount);
    }

    inv.pick(new Item(r.result_type, r.name, r.result_amount, r.result_texture, false, {1, 1}));
    return true;
}

void CraftingSystem::select_by_mouse(float mx, float my, const SDL_FRect& panel_rect) {
    hovered = -1;
    float list_x = panel_rect.x + PANEL_PAD;
    float list_y = panel_rect.y + PANEL_PAD + 18.0f;

    for (int i = 0; i < (int)recipes.size(); ++i) {
        SDL_FRect row = {
            list_x,
            list_y + i * LIST_ROW_H,
            panel_rect.w * 0.42f,
            LIST_ROW_H - 3.0f
        };
        if (mx >= row.x && mx <= row.x + row.w && my >= row.y && my <= row.y + row.h) {
            hovered = i;
            return;
        }
    }
}

void CraftingSystem::update(float, float) {}

void CraftingSystem::handle_event(const SDL_Event& e) {
    (void)e;
}

static void draw_text(SDL_Renderer* renderer, TTF_Font* font, float x, float y, const char* text, SDL_Color color) {
    if (!renderer || !font || !text || !*text) return;

    SDL_Surface* surf = TTF_RenderText_Blended(font, text, std::strlen(text), color);
    if (!surf) return;

    SDL_Texture* tx = SDL_CreateTextureFromSurface(renderer, surf);
    if (!tx) {
        SDL_DestroySurface(surf);
        return;
    }

    SDL_FRect dst = { x, y, (float)surf->w, (float)surf->h };
    SDL_RenderTexture(renderer, tx, nullptr, &dst);

    SDL_DestroyTexture(tx);
    SDL_DestroySurface(surf);
}

void CraftingSystem::draw_panel(SDL_Renderer* renderer, const SDL_FRect& panel_rect, const Inventory& inv) const {
    if (!renderer) return;

    SDL_SetRenderDrawColor(renderer, 28, 32, 38, 255);
    SDL_RenderFillRect(renderer, &panel_rect);
    SDL_SetRenderDrawColor(renderer, 60, 72, 84, 255);
    SDL_RenderRect(renderer, &panel_rect);

    SDL_Color white = {240, 243, 247, 255};
    SDL_Color muted = {180, 188, 196, 255};
    SDL_Color good = {120, 220, 140, 255};
    SDL_Color bad = {230, 100, 100, 255};
    SDL_Color yellow = {220, 200, 90, 255};

    draw_text(renderer, font, panel_rect.x + PANEL_PAD, panel_rect.y + 4.0f, "Crafting", white);

    if (recipes.empty()) {
        draw_text(renderer, font, panel_rect.x + PANEL_PAD, panel_rect.y + 28.0f, "No recipes", muted);
        return;
    }

    float list_x = panel_rect.x + PANEL_PAD;
    float list_y = panel_rect.y + PANEL_PAD + 18.0f;
    float list_w = panel_rect.w * 0.42f;
    float detail_x = panel_rect.x + list_w + PANEL_PAD * 2.0f;
    float detail_w = panel_rect.w - (detail_x - panel_rect.x) - PANEL_PAD;

    for (int i = 0; i < (int)recipes.size(); ++i) {
        const Recipe& r = recipes[i];
        bool can = can_craft(inv, r);
        bool sel = (i == selected);
        bool hov = (i == hovered);

        SDL_FRect row = {
            list_x,
            list_y + i * LIST_ROW_H,
            list_w,
            LIST_ROW_H - 3.0f
        };

        if (sel) SDL_SetRenderDrawColor(renderer, 60, 92, 122, 255);
        else if (hov) SDL_SetRenderDrawColor(renderer, 50, 58, 68, 255);
        else SDL_SetRenderDrawColor(renderer, 38, 44, 52, 255);

        SDL_RenderFillRect(renderer, &row);
        SDL_SetRenderDrawColor(renderer, 16, 18, 22, 255);
        SDL_RenderRect(renderer, &row);

        SDL_FRect icon = { row.x + 4.0f, row.y + 5.0f, ICON_BOX, ICON_BOX };
        SDL_SetRenderDrawColor(renderer, 10, 12, 14, 255);
        SDL_RenderFillRect(renderer, &icon);

        if (r.result_texture) {
            SDL_FRect dst = { icon.x + 2.0f, icon.y + 2.0f, icon.w - 4.0f, icon.h - 4.0f };
            SDL_RenderTexture(renderer, r.result_texture, nullptr, &dst);
        }

        draw_text(renderer, font, row.x + 34.0f, row.y + 4.0f, r.name.c_str(), can ? white : muted);

        char line[64];
        snprintf(line, sizeof(line), "x%d", r.result_amount);
        draw_text(renderer, font, row.x + 34.0f, row.y + 18.0f, line, muted);
    }

    if (selected < 0 || selected >= (int)recipes.size()) return;

    const Recipe& r = recipes[selected];
    bool craftable = can_craft(inv, r);

    SDL_FRect detail = {
        detail_x,
        panel_rect.y + PANEL_PAD + 18.0f,
        detail_w,
        panel_rect.h - PANEL_PAD * 2.0f - 18.0f
    };

    SDL_SetRenderDrawColor(renderer, 22, 26, 31, 255);
    SDL_RenderFillRect(renderer, &detail);
    SDL_SetRenderDrawColor(renderer, 72, 84, 96, 255);
    SDL_RenderRect(renderer, &detail);

    draw_text(renderer, font, detail.x + 8.0f, detail.y + 8.0f, r.name.c_str(), white);

    char buf[128];
    snprintf(buf, sizeof(buf), "Result: %s x%d", r.name.c_str(), r.result_amount);
    draw_text(renderer, font, detail.x + 8.0f, detail.y + 28.0f, buf, muted);
    draw_text(renderer, font, detail.x + 8.0f, detail.y + 48.0f, craftable ? "Status: can craft" : "Status: missing items", craftable ? good : bad);
    draw_text(renderer, font, detail.x + 8.0f, detail.y + 72.0f, "Ingredients:", yellow);

    float y = detail.y + 92.0f;
    for (const auto& ing : r.ingredients) {
        bool enough = inv.get_amount(ing.type) >= ing.amount;

        SDL_FRect icon = { detail.x + 8.0f, y + 1.0f, 14.0f, 14.0f };
        SDL_SetRenderDrawColor(renderer, 10, 12, 14, 255);
        SDL_RenderFillRect(renderer, &icon);

        SDL_Texture* icon_tex = nullptr;
        switch (ing.type) {
            case ItemType::STONE: icon_tex = tex.stone; break;
            case ItemType::IRON_ORE: icon_tex = tex.iron_ore; break;
            case ItemType::FURNACE: icon_tex = tex.furnace; break;
            default: break;
        }

        if (icon_tex) {
            SDL_FRect dst = { icon.x + 1.0f, icon.y + 1.0f, 12.0f, 12.0f };
            SDL_RenderTexture(renderer, icon_tex, nullptr, &dst);
        }

        snprintf(buf, sizeof(buf), "%s  %d", item_type_name(ing.type), ing.amount);
        draw_text(renderer, font, detail.x + 28.0f, y - 1.0f, buf, enough ? white : bad);
        y += 18.0f;
    }

    SDL_FRect button = { detail.x + 8.0f, detail.y + detail.h - 34.0f, 92.0f, 24.0f };
    SDL_SetRenderDrawColor(renderer, craftable ? 40 : 55, craftable ? 120 : 60, craftable ? 55 : 60, 255);
    SDL_RenderFillRect(renderer, &button);
    SDL_SetRenderDrawColor(renderer, 16, 16, 16, 255);
    SDL_RenderRect(renderer, &button);
    draw_text(renderer, font, button.x + 17.0f, button.y + 4.0f, "Craft", white);
}