#include "inv/crafting.h"
#include "inv/crafting_queue.h"
#include "inv/inventory.h"
#include "textrenderer.h"
#include "localization.h"
#include <cstdio>
#include <climits>
#include <algorithm>

static constexpr float PANEL_PAD  = 8.0f;
static constexpr float SLOT_W     = 42.0f;
static constexpr float SLOT_H     = 42.0f;
static constexpr float SLOT_GAP   = 4.0f;
static constexpr int   GRID_COLS  = 4;

static int craft_count(const Inventory& inv, const Recipe& r, const std::unordered_map<ItemType, int>& available) {
    int count = INT_MAX;
    
    for (const RecipeIngredient& ing : r.ingredients) {
        int have = available.count(ing.type) ? available.at(ing.type) : 0;

        have = std::max(0, have);
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

static SDL_Texture* ingredient_texture(const Textures& tex, ItemType type) {
    switch (type) {
        case ItemType::STONE: return tex.stone;
        case ItemType::IRON_ORE: return tex.iron_ore;
        case ItemType::IRON_PLATE: return tex.iron_plate;
        case ItemType::COAL: return tex.coal;
        case ItemType::FURNACE: return tex.furnace;
        case ItemType::DRILL: return tex.drill;
        default: return nullptr;
    }
}

const char* CraftingSystem::item_type_name(ItemType type) {
    switch (type) {
        case ItemType::STONE: return "Stone";
        case ItemType::IRON_ORE: return "Iron Ore";
        case ItemType::IRON_PLATE: return "Iron Plate";
        case ItemType::COPPER_ORE: return "Copper Ore";
        case ItemType::COAL: return "Coal";
        case ItemType::FURNACE: return "Furnace";
        case ItemType::DRILL: return "Drill";
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
    int count = 1;

    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) count = 1;
    else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_RIGHT) count = 5;
    else return;

    float mx = (float)e.button.x;
    float my = (float)e.button.y;

    for (int i = 0; i < (int)recipes.size(); ++i) {
        SDL_FRect r = slot_rect(panel_rect, i);
        if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) {
            selected = i;

            std::unordered_map<ItemType, int> available = queue.compute_available(inv);
            int max_craftable = craft_count(inv, recipes[selected], available);
            count = std::min(count, max_craftable);

            for (int j = 0; j < count; j++) craft_selected(inv);
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
        std::unordered_map<ItemType, int> available = queue.compute_available(inv);
        int cnt = craft_count(inv, r, available);
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

        std::string title = Localize("Craft") + ": " + Localize(r.name);
        int tw = 0, th = 0;
        TTF_GetStringSize(font, title.c_str(), 0, &tw, &th);

        std::string time_label = Localize("Time");
        char time_buf[64];
        snprintf(time_buf, sizeof(time_buf), "%s: %.1fs", time_label.c_str(), r.craft_duration);
        int ttw = 0, tth = 0;
        TTF_GetStringSize(font, time_buf, 0, &ttw, &tth);

        std::string labels = Localize("Ingredients");
        int lw = 0, lh = 0;
        TTF_GetStringSize(font, labels.c_str(), 0, &lw, &lh);

        int max_ing_w = 0;
        int line_height = 0;
        float icon_size = 18.0f;
        float line_spacing = 2.0f;
        std::vector<std::pair<ItemType, std::string>> ingredient_lines;
        ingredient_lines.reserve(r.ingredients.size());
        for (const RecipeIngredient& ing : r.ingredients) {
            char count_buf[16];
            snprintf(count_buf, sizeof(count_buf), "x%d", ing.amount);
            int cw = 0, ch = 0;
            TTF_GetStringSize(font, count_buf, 0, &cw, &ch);
            max_ing_w = std::max(max_ing_w, (int)(icon_size + 2.0f + cw));
            line_height = std::max(line_height, std::max((int)icon_size, ch));
            ingredient_lines.emplace_back(ing.type, std::string(count_buf));
        }

        if (line_height == 0) {
            line_height = (int)icon_size;
        }

        float pad = 4.0f;
        float tip_w = std::max({(float)tw, (float)ttw, (float)lw, (float)max_ing_w}) + pad * 2.0f;
        float tip_h = (float)th + (float)tth + (float)lh + ingredient_lines.size() * (float)line_height + line_spacing * (ingredient_lines.empty() ? 0 : ingredient_lines.size() - 1) + pad * 4.0f;
        float tip_x = slot.x + slot.w * 0.5f - tip_w * 0.5f;
        float tip_y = slot.y - tip_h - 4.0f;

        if (tip_x < panel_rect.x + PANEL_PAD) tip_x = panel_rect.x + PANEL_PAD;
        if (tip_x + tip_w > panel_rect.x + panel_rect.w - PANEL_PAD)
            tip_x = panel_rect.x + panel_rect.w - PANEL_PAD - tip_w;
        if (tip_y < panel_rect.y + PANEL_PAD) {
            tip_y = slot.y + slot.h + 4.0f;
        }
        if (tip_y + tip_h > panel_rect.y + panel_rect.h - PANEL_PAD) {
            tip_y = panel_rect.y + panel_rect.h - PANEL_PAD - tip_h;
        }

        SDL_FRect bg = { tip_x, tip_y, tip_w, tip_h };
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 10, 11, 14, 220);
        SDL_RenderFillRect(renderer, &bg);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 60, 65, 75, 255);
        SDL_RenderRect(renderer, &bg);

        float text_x = tip_x + pad;
        float text_y = tip_y + pad;
        TextRenderer::DrawText(renderer, font, text_x, text_y, title.c_str(), {238, 240, 243, 255});

        text_y += (float)th + pad;
        TextRenderer::DrawText(renderer, font, text_x, text_y, time_buf, {170, 176, 184, 255});

        text_y += (float)tth + pad;
        TextRenderer::DrawText(renderer, font, text_x, text_y, labels.c_str(), {170, 176, 184, 255});

        text_y += (float)lh + line_spacing;
        for (const auto& entry : ingredient_lines) {
            ItemType type = entry.first;
            const std::string& count_str = entry.second;
            SDL_Texture* icon = ingredient_texture(tex, type);
            if (icon) {
                SDL_SetTextureScaleMode(icon, SDL_SCALEMODE_NEAREST);
                SDL_FRect icon_rect = { text_x, text_y, icon_size, icon_size };
                SDL_RenderTexture(renderer, icon, nullptr, &icon_rect);
            }
            float count_x = text_x + icon_size + 2.0f;
            float count_y = text_y + ((float)line_height - (float)TTF_GetFontHeight(font)) * 0.5f;
            TextRenderer::DrawText(renderer, font, count_x, count_y, count_str.c_str(), {220, 196, 134, 255});
            text_y += (float)line_height + line_spacing;
        }
    }
}

void CraftingSystem::draw_queue(SDL_Renderer* renderer, TTF_Font* font, const SDL_FRect& screen_rect) const {
    queue.draw(renderer, font, screen_rect);
}
