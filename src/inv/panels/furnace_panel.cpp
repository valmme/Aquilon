#include "inv/panels/furnace_panel.h"
#include "inv/inventory.h"
#include "textrenderer.h"
#include "localization.h"
#include <cstdio>
#include <algorithm>

static constexpr float PANEL_PAD   = 10.0f;
static constexpr float SLOT_SIZE    = 35.0f;
static constexpr float ICON_SIZE    = 72.0f;
static constexpr float BAR_H        = 8.0f;
static constexpr float BAR_W        = 126.0f;

static SDL_FRect furnace_icon_rect(const SDL_FRect& panel) {
    return {
        panel.x + panel.w * 0.5f - ICON_SIZE * 0.5f,
        panel.y + 16.0f,
        ICON_SIZE,
        ICON_SIZE
    };
}

static float slots_row_y(const SDL_FRect& panel) {
    float main_area_y = panel.y + PANEL_PAD + 22.0f;
    float main_area_h = ICON_SIZE + 20.0f;
    return main_area_y + main_area_h + 10.0f;
}

static SDL_FRect input_slot_rect(const SDL_FRect& panel) {
    return { panel.x + PANEL_PAD, slots_row_y(panel), SLOT_SIZE, SLOT_SIZE };
}

static SDL_FRect fuel_slot_rect(const SDL_FRect& panel) {
    return { panel.x + PANEL_PAD, slots_row_y(panel) + SLOT_SIZE + 8.0f, SLOT_SIZE, SLOT_SIZE };
}

static SDL_FRect output_slot_rect(const SDL_FRect& panel) {
    return { panel.x + panel.w - PANEL_PAD - SLOT_SIZE, slots_row_y(panel), SLOT_SIZE, SLOT_SIZE };
}

static SDL_FRect progress_rect(const SDL_FRect& panel) {
    SDL_FRect in = input_slot_rect(panel);
    SDL_FRect out = output_slot_rect(panel);
    float x = in.x + in.w + 10.0f;
    float y = in.y + SLOT_SIZE * 0.5f - BAR_H * 0.5f;
    float w = std::max(24.0f, out.x - 10.0f - x);
    return { x, y, w, BAR_H };
}

static bool point_in_rect(float mx, float my, const SDL_FRect& r) {
    return mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
}

static const char* item_name_for(ItemType type) {
    switch (type) {
        case ItemType::IRON_ORE:   return "Iron Ore";
        case ItemType::IRON_PLATE: return "Iron Plate";
        case ItemType::COAL:       return "Coal";
        case ItemType::STONE:      return "Stone";
        default:                   return "Item";
    }
}

static SDL_Texture* texture_for_item(const FurnacePanel& p, ItemType type) {
    switch (type) {
        case ItemType::IRON_ORE:   return p.tex.iron_ore;
        case ItemType::IRON_PLATE: return p.tex.iron_plate;
        case ItemType::COAL:       return p.tex.coal;
        case ItemType::STONE:      return p.tex.stone;
        default:                   return nullptr;
    }
}

FurnacePanel::FurnacePanel(Textures& tex, TTF_Font* font) : tex(tex), font(font) {}

FurnacePanel::~FurnacePanel() {
    delete input_slot;
    delete fuel_slot;
    delete output_slot;
}

void FurnacePanel::initialize_recipes() {
    add_recipe(&Item::IRON_ORE, &Item::IRON_PLATE, 1.0f);
}

void FurnacePanel::add_recipe(Item* input, Item* output, float time) {
    if (!input || !output) return;
    recipes.push_back(FurnaceRecipe{input, output, time});
}

const FurnaceRecipe* FurnacePanel::find_recipe(ItemType input_type) const {
    for (const auto& r : recipes) {
        if (r.input && r.input->type == input_type) return &r;
    }
    return nullptr;
}

void FurnacePanel::select_by_mouse(float mx, float my, const SDL_FRect& panel_rec) {
    hovered_slot = -1;

    if (point_in_rect(mx, my, input_slot_rect(panel_rec))) hovered_slot = 0;
    else if (point_in_rect(mx, my, fuel_slot_rect(panel_rec))) hovered_slot = 1;
    else if (point_in_rect(mx, my, output_slot_rect(panel_rec))) hovered_slot = 2;
}

void FurnacePanel::handle_event(const SDL_Event& e, Inventory& inv, const SDL_FRect& panel_rec) {
    if (e.type != SDL_EVENT_MOUSE_BUTTON_DOWN || e.button.button != SDL_BUTTON_LEFT) return;

    float mx = (float)e.button.x;
    float my = (float)e.button.y;

    SDL_FRect rects[3] = {
        input_slot_rect(panel_rec),
        fuel_slot_rect(panel_rec),
        output_slot_rect(panel_rec)
    };

    int clicked = -1;
    for (int i = 0; i < 3; ++i) {
        if (point_in_rect(mx, my, rects[i])) {
            clicked = i;
            break;
        }
    }
    if (clicked < 0) return;

    Item** slot_ptr = nullptr;
    if (clicked == 0) slot_ptr = &input_slot;
    else if (clicked == 1) slot_ptr = &fuel_slot;
    else slot_ptr = &output_slot;

    Item* cursor = inv.get_dragged_item() ? inv.release_cursor_item() : nullptr;

    if (clicked == 2) {
        if (cursor) {
            inv.set_cursor_item(cursor);
        } else if (*slot_ptr) {
            inv.set_cursor_item(*slot_ptr);
            *slot_ptr = nullptr;
        }
        return;
    }

    if (cursor && *slot_ptr == nullptr) {
        *slot_ptr = cursor;
        return;
    }

    if (cursor && *slot_ptr) {
        inv.set_cursor_item(*slot_ptr);
        *slot_ptr = cursor;
        return;
    }

    if (!cursor && *slot_ptr) {
        inv.set_cursor_item(*slot_ptr);
        *slot_ptr = nullptr;
        return;
    }
}

void FurnacePanel::update(float delta_time) {
    bool has_input = input_slot && input_slot->amount > 0;
    bool has_fuel  = fuel_slot && fuel_slot->amount > 0;

    if (!has_input) {
        smelt_progress = 0.0f;
        return;
    }

    if (fuel_remaining <= 0.0f) {
        if (!has_fuel) return;
        fuel_remaining = 1.0f;
        fuel_slot->amount--;

        if (fuel_slot->amount <= 0) {
            delete fuel_slot;
            fuel_slot = nullptr;
        }
    }

    fuel_remaining -= delta_time * 0.2f;
    if (fuel_remaining < 0.0f) fuel_remaining = 0.0f;

    smelt_progress += delta_time * 0.2f;
    if (smelt_progress < 1.0f) return;
    smelt_progress = 0.0f;

    const FurnaceRecipe* recipe = find_recipe(input_slot->type);
    if (!recipe) return;

    const Item* out_proto = recipe->output;
    if (!out_proto) return;

    if (out_proto->type == ItemType::NONE) return;

    input_slot->amount--;
    if (input_slot->amount <= 0) {
        delete input_slot;
        input_slot = nullptr;
    }

    if (output_slot == nullptr) {
        output_slot = out_proto->copy();
        output_slot->amount = 1;
    } 
    
    else if (output_slot->type == out_proto->type) {
        output_slot->amount++;
    }
    
    else {
        delete output_slot;
        output_slot = out_proto->copy();
        output_slot->amount = 1;
    }
}

void FurnacePanel::draw_panel(SDL_Renderer* renderer, const SDL_FRect& panel_rec, const Inventory& inv) const {
    (void)inv;

    SDL_SetRenderDrawColor(renderer, 14, 15, 18, 255);
    SDL_RenderFillRect(renderer, &panel_rec);
    SDL_SetRenderDrawColor(renderer, 40, 44, 52, 255);
    SDL_RenderRect(renderer, &panel_rec);

    const SDL_Color white = {238, 240, 243, 255};
    const SDL_Color muted = {120, 126, 134, 255};
    const SDL_Color amber = {220, 196, 134, 255};
    const SDL_Color red   = {200,  60,  60, 255};

    {
        bool active    = fuel_remaining > 0.0f;
        bool has_input = input_slot && input_slot->amount > 0;
        bool has_fuel  = fuel_slot  && fuel_slot->amount  > 0;

        std::string status_text;
        SDL_Color   status_col;

        if (!has_input) {
            status_text = Localize("No input");
            status_col  = muted;
        } 
        
        else if (!active && !has_fuel) {
            status_text = Localize("No fuel");
            status_col  = red;
        } 
        
        else {
            status_text = Localize("Smelting...");
            status_col  = amber;
        }

        float dot_r = 4.0f;
        float dot_x = panel_rec.x + PANEL_PAD + dot_r;
        float dot_y = panel_rec.y + PANEL_PAD + dot_r + 2.0f;

        SDL_FRect status_bg = {
            panel_rec.x + PANEL_PAD - 2.0f,
            panel_rec.y + PANEL_PAD - 2.0f,
            panel_rec.w - PANEL_PAD * 2.0f + 4.0f,
            18.0f
        };
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, status_col.r, status_col.g, status_col.b, 20);
        SDL_RenderFillRect(renderer, &status_bg);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer, status_col.r, status_col.g, status_col.b, 255);
        SDL_FRect dot = { dot_x - dot_r, dot_y - dot_r, dot_r * 2.0f, dot_r * 2.0f };
        SDL_RenderFillRect(renderer, &dot);

        if (font)
            TextRenderer::DrawText(renderer, font,
                dot_x + dot_r + 4.0f, panel_rec.y + PANEL_PAD,
                status_text, status_col);
    }

    float main_area_y = panel_rec.y + PANEL_PAD + 22.0f;
    float main_area_h = ICON_SIZE + 20.0f;
    SDL_FRect main_area = {
        panel_rec.x + PANEL_PAD,
        main_area_y,
        panel_rec.w - PANEL_PAD * 2.0f,
        main_area_h
    };

    SDL_SetRenderDrawColor(renderer, 20, 22, 27, 255);
    SDL_RenderFillRect(renderer, &main_area);
    SDL_SetRenderDrawColor(renderer, 32, 36, 44, 255);
    SDL_RenderRect(renderer, &main_area);

    if (tex.furnace) {
        SDL_FRect ficon = {
            main_area.x + main_area.w * 0.5f - ICON_SIZE * 0.5f,
            main_area.y + main_area.h * 0.5f - ICON_SIZE * 0.5f,
            ICON_SIZE, ICON_SIZE
        };
        SDL_SetTextureScaleMode(tex.furnace, SDL_SCALEMODE_NEAREST);
        SDL_RenderTexture(renderer, tex.furnace, nullptr, &ficon);
    }

    float slots_y = main_area.y + main_area.h + 10.0f;
    float left_x  = panel_rec.x + PANEL_PAD;
    float right_x = panel_rec.x + panel_rec.w - PANEL_PAD - SLOT_SIZE;

    SDL_FRect input_r  = { left_x,  slots_y,               SLOT_SIZE, SLOT_SIZE };
    SDL_FRect fuel_r   = { left_x,  slots_y + SLOT_SIZE + 8.0f, SLOT_SIZE, SLOT_SIZE };
    SDL_FRect output_r = { right_x, slots_y,               SLOT_SIZE, SLOT_SIZE };

    float craft_bar_x = input_r.x + SLOT_SIZE + 8.0f;
    float craft_bar_w = output_r.x - craft_bar_x - 8.0f;
    float craft_bar_y = input_r.y + SLOT_SIZE * 0.5f - BAR_H * 0.5f;
    SDL_FRect craft_bar = { craft_bar_x, craft_bar_y, craft_bar_w, BAR_H };

    float fuel_bar_x = fuel_r.x + SLOT_SIZE + 8.0f;
    float fuel_bar_w = output_r.x + SLOT_SIZE - fuel_bar_x;
    float fuel_bar_y = fuel_r.y + SLOT_SIZE * 0.5f - BAR_H * 0.5f;
    SDL_FRect fuel_bar = { fuel_bar_x, fuel_bar_y, fuel_bar_w, BAR_H };

    auto draw_slot = [&](const SDL_FRect& r, Item* item, bool hov, bool is_fuel = false) {
        SDL_Texture* slot_tex = (is_fuel && tex.fuel_slot) ? tex.fuel_slot : tex.slot;

        if (slot_tex) {
            SDL_SetTextureScaleMode(slot_tex, SDL_SCALEMODE_NEAREST);
            SDL_RenderTexture(renderer, slot_tex, nullptr, &r);
        } 
        
        else {
            SDL_SetRenderDrawColor(renderer, 22, 24, 29, 255);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 44, 48, 58, 255);
            SDL_RenderRect(renderer, &r);
        }

        if (hov) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 22);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }

        if (item && item->texture) {
            SDL_FRect icon2 = { r.x + 5.0f, r.y + 5.0f, r.w - 10.0f, r.h - 10.0f };
            SDL_SetTextureScaleMode(item->texture, SDL_SCALEMODE_NEAREST);
            SDL_RenderTexture(renderer, item->texture, nullptr, &icon2);
        }

        if (font && item && item->amount > 1) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", item->amount);
            float fh = (float)TTF_GetFontHeight(font);
            TextRenderer::DrawText(renderer, font,
                r.x + r.w - 16.0f, r.y + r.h - fh - 3.0f, buf, amber);
        }
    };

    draw_slot(input_r,  input_slot,  hovered_slot == 0);
    draw_slot(output_r, output_slot, hovered_slot == 2);
    draw_slot(fuel_r,   fuel_slot,   hovered_slot == 1, true);

    SDL_SetRenderDrawColor(renderer, 26, 29, 35, 255);
    SDL_RenderFillRect(renderer, &craft_bar);
    SDL_SetRenderDrawColor(renderer, 40, 44, 52, 255);
    SDL_RenderRect(renderer, &craft_bar);

    if (smelt_progress > 0.0f) {
        SDL_FRect fill = { craft_bar.x, craft_bar.y, craft_bar.w * smelt_progress, BAR_H };
        SDL_SetRenderDrawColor(renderer, 220, 140, 60, 255);
        SDL_RenderFillRect(renderer, &fill);
    }

    if (font) {
        char pct[16];
        snprintf(pct, sizeof(pct), "%.0f%%", smelt_progress * 100.0f);
        int tw = 0, th = 0;
        TTF_GetStringSize(font, pct, 0, &tw, &th);
        TextRenderer::DrawText(renderer, font,
            craft_bar.x + craft_bar.w - (float)tw,
            craft_bar.y - (float)th - 2.0f,
            pct, muted);
    }

    SDL_SetRenderDrawColor(renderer, 26, 29, 35, 255);
    SDL_RenderFillRect(renderer, &fuel_bar);
    SDL_SetRenderDrawColor(renderer, 40, 44, 52, 255);
    SDL_RenderRect(renderer, &fuel_bar);

    if (fuel_remaining > 0.0f) {
        SDL_FRect fill = { fuel_bar.x, fuel_bar.y, fuel_bar.w * fuel_remaining, BAR_H };
        SDL_SetRenderDrawColor(renderer, 80, 180, 100, 255);
        SDL_RenderFillRect(renderer, &fill);
    }
}