#include "inv/panels/furnace_panel.h"
#include "inv/inventory.h"
#include "textrenderer.h"
#include "localization.h"
#include <cstdio>
#include <algorithm>

static constexpr float PANEL_PAD   = 10.0f;
static constexpr float SLOT_SIZE    = 44.0f;
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

static SDL_FRect input_slot_rect(const SDL_FRect& panel) {
    SDL_FRect icon = furnace_icon_rect(panel);
    return {
        icon.x - SLOT_SIZE - 18.0f,
        icon.y + icon.h + 24.0f,
        SLOT_SIZE,
        SLOT_SIZE
    };
}

static SDL_FRect fuel_slot_rect(const SDL_FRect& panel) {
    SDL_FRect in = input_slot_rect(panel);
    return {
        in.x + SLOT_SIZE + 16.0f,
        in.y,
        SLOT_SIZE,
        SLOT_SIZE
    };
}

static SDL_FRect output_slot_rect(const SDL_FRect& panel) {
    SDL_FRect icon = furnace_icon_rect(panel);
    return {
        icon.x + icon.w + 22.0f,
        icon.y + icon.h + 10.0f,
        SLOT_SIZE,
        SLOT_SIZE
    };
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

    if (font) {
        TextRenderer::DrawText(renderer, font, panel_rec.x + PANEL_PAD, panel_rec.y + 4.0f, Localize("Furnace"), white);
    }

    SDL_FRect icon = furnace_icon_rect(panel_rec);
    SDL_SetRenderDrawColor(renderer, 26, 28, 34, 255);
    SDL_RenderFillRect(renderer, &icon);
    SDL_SetRenderDrawColor(renderer, 70, 76, 90, 255);
    SDL_RenderRect(renderer, &icon);

    if (tex.furnace) {
        SDL_FRect icon_in = { icon.x + 8.0f, icon.y + 8.0f, icon.w - 16.0f, icon.h - 16.0f };
        SDL_RenderTexture(renderer, tex.furnace, nullptr, &icon_in);
    }

    SDL_FRect input_r = input_slot_rect(panel_rec);
    SDL_FRect fuel_r  = fuel_slot_rect(panel_rec);
    SDL_FRect out_r   = output_slot_rect(panel_rec);
    SDL_FRect bar_r   = progress_rect(panel_rec);

    auto draw_slot = [&](const SDL_FRect& r, Item* item, const char* label, bool hov) {
        SDL_SetRenderDrawColor(renderer, 22, 24, 29, 255);
        SDL_RenderFillRect(renderer, &r);

        if (hov) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 18);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }

        SDL_SetRenderDrawColor(renderer, 44, 48, 58, 255);
        SDL_RenderRect(renderer, &r);

        if (item && item->texture) {
            SDL_FRect icon2 = { r.x + 5.0f, r.y + 5.0f, r.w - 10.0f, r.h - 10.0f };
            SDL_RenderTexture(renderer, item->texture, nullptr, &icon2);
        }

        if (font && item && item->amount > 1) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", item->amount);
            TextRenderer::DrawText(renderer, font, r.x + r.w - 16.0f, r.y + r.h - (float)TTF_GetFontHeight(font) - 3.0f, buf, amber);
        }

        if (font) {
            TextRenderer::DrawText(renderer, font, r.x, r.y + r.h + 3.0f, Localize(label), muted);
        }
    };

    draw_slot(input_r, input_slot, "Input", hovered_slot == 0);
    draw_slot(fuel_r, fuel_slot, "Fuel", hovered_slot == 1);
    draw_slot(out_r, output_slot, "Output", hovered_slot == 2);

    SDL_SetRenderDrawColor(renderer, 60, 65, 75, 255);
    SDL_RenderLine(renderer, input_r.x + input_r.w, input_r.y + input_r.h * 0.5f, out_r.x, out_r.y + out_r.h * 0.5f);

    SDL_SetRenderDrawColor(renderer, 30, 33, 40, 255);
    SDL_RenderFillRect(renderer, &bar_r);
    SDL_SetRenderDrawColor(renderer, 44, 48, 58, 255);
    SDL_RenderRect(renderer, &bar_r);

    if (smelt_progress > 0.0f) {
        SDL_FRect fill = { bar_r.x, bar_r.y, bar_r.w * smelt_progress, bar_r.h };
        SDL_SetRenderDrawColor(renderer, 220, 140, 60, 255);
        SDL_RenderFillRect(renderer, &fill);
    }

    if (fuel_remaining > 0.0f) {
        SDL_FRect fuel_bar = { fuel_r.x, fuel_r.y - BAR_H - 6.0f, fuel_r.w, BAR_H };
        SDL_SetRenderDrawColor(renderer, 30, 33, 40, 255);
        SDL_RenderFillRect(renderer, &fuel_bar);
        SDL_FRect fill = { fuel_bar.x, fuel_bar.y, fuel_bar.w * fuel_remaining, fuel_bar.h };
        SDL_SetRenderDrawColor(renderer, 80, 180, 100, 255);
        SDL_RenderFillRect(renderer, &fill);
        SDL_SetRenderDrawColor(renderer, 44, 48, 58, 255);
        SDL_RenderRect(renderer, &fuel_bar);
    }
}