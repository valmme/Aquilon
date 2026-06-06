#include "inv/panels/drill_panel.h"
#include "inv/inventory.h"
#include "textrenderer.h"
#include "localization.h"
#include <cstdio>

static constexpr float PANEL_PAD   = 10.0f;
static constexpr float SLOT_SIZE   = 35.0f;
static constexpr float ICON_SIZE   = 72.0f;
static constexpr float BAR_H       = 8.0f;

static SDL_FRect drill_icon_rect(const SDL_FRect& panel) {
    return {
        panel.x + panel.w * 0.5f - ICON_SIZE * 0.5f,
        panel.y + 16.0f,
        ICON_SIZE,
        ICON_SIZE
    };
}

static SDL_FRect fuel_slot_rect(const SDL_FRect& panel) {
    return { panel.x + PANEL_PAD, panel.y + PANEL_PAD + ICON_SIZE + 28.0f, SLOT_SIZE, SLOT_SIZE };
}

static SDL_FRect fuel_bar_rect(const SDL_FRect& panel) {
    SDL_FRect fuel_r = fuel_slot_rect(panel);
    float x = fuel_r.x + fuel_r.w + 8.0f;
    float y = fuel_r.y + fuel_r.h * 0.5f - BAR_H * 0.5f;
    float w = panel.x + panel.w - PANEL_PAD - x;
    return { x, y, w, BAR_H };
}

static bool point_in_rect(float mx, float my, const SDL_FRect& r) {
    return mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
}

static bool accepts_fuel(Item* item) {
    return item && item->type == ItemType::COAL;
}

static void sync_drill_target(DrillPanel& panel) {
    if (!panel.target) return;
    panel.target->fuel_amount = panel.fuel_slot ? panel.fuel_slot->amount : 0;
    panel.target->fuel_remaining = panel.fuel_remaining;
}

static void load_drill_target(DrillPanel& panel) {
    if (!panel.target) return;
    panel.fuel_remaining = panel.target->fuel_remaining;
    if (panel.fuel_slot) {
        delete panel.fuel_slot;
        panel.fuel_slot = nullptr;
    }
    if (panel.target->fuel_amount > 0) {
        panel.fuel_slot = Item::COAL.copy();
        panel.fuel_slot->amount = panel.target->fuel_amount;
    }
}

DrillPanel::DrillPanel(Textures& tex, TTF_Font* font) : tex(tex), font(font) {}

DrillPanel::~DrillPanel() {
    delete fuel_slot;
}

void DrillPanel::set_target(Player::PlacedObject* obj) {
    if (target == obj) return;
    if (target) {
        sync_drill_target(*this);
    }
    target = obj;
    if (fuel_slot) {
        delete fuel_slot;
        fuel_slot = nullptr;
    }
    if (target) {
        load_drill_target(*this);
    } else {
        fuel_remaining = 0.0f;
    }
}

void DrillPanel::update(float delta_time) {
    if (!target) return;

    bool has_fuel = fuel_slot && fuel_slot->amount > 0;
    if (fuel_remaining <= 0.0f) {
        if (!has_fuel) {
            sync_drill_target(*this);
            return;
        }
        fuel_remaining = 1.0f;
        fuel_slot->amount--;
        if (fuel_slot->amount <= 0) {
            delete fuel_slot;
            fuel_slot = nullptr;
        }
    }

    fuel_remaining -= delta_time * 0.2f;
    if (fuel_remaining < 0.0f) fuel_remaining = 0.0f;
    sync_drill_target(*this);
}

void DrillPanel::select_by_mouse(float mx, float my, const SDL_FRect& panel_rec) {
    hovered_slot = -1;
    if (point_in_rect(mx, my, fuel_slot_rect(panel_rec))) hovered_slot = 0;
}

void DrillPanel::handle_event(const SDL_Event& e, Inventory& inv, const SDL_FRect& panel_rec) {
    if (e.type != SDL_EVENT_MOUSE_BUTTON_DOWN || e.button.button != SDL_BUTTON_LEFT) return;

    float mx = (float)e.button.x;
    float my = (float)e.button.y;
    if (!point_in_rect(mx, my, fuel_slot_rect(panel_rec))) return;
    if (!target) return;

    Item* cursor = inv.get_dragged_item() ? inv.release_cursor_item() : nullptr;
    if (cursor && !accepts_fuel(cursor)) {
        inv.set_cursor_item(cursor);
        return;
    }

    if (cursor && !fuel_slot) {
        fuel_slot = cursor;
        return;
    }

    if (cursor && fuel_slot) {
        fuel_slot->amount += cursor->amount;
        delete cursor;
        cursor = nullptr;
        return;
    }

    if (!cursor && fuel_slot) {
        inv.set_cursor_item(fuel_slot);
        fuel_slot = nullptr;
        return;
    }
}

void DrillPanel::draw_panel(SDL_Renderer* renderer, const SDL_FRect& panel_rec, const Inventory& inv) const {
    (void)inv;

    SDL_SetRenderDrawColor(renderer, 14, 15, 18, 255);
    SDL_RenderFillRect(renderer, &panel_rec);
    SDL_SetRenderDrawColor(renderer, 40, 44, 52, 255);
    SDL_RenderRect(renderer, &panel_rec);

    const SDL_Color white = {238, 240, 243, 255};
    const SDL_Color muted = {120, 126, 134, 255};
    const SDL_Color amber = {220, 196, 134, 255};
    const SDL_Color red   = {200,  60,  60, 255};

    bool has_fuel = fuel_slot && fuel_slot->amount > 0;
    bool active   = fuel_remaining > 0.0f;
    std::string status_text;
    SDL_Color status_col;

    if (!has_fuel && !active) {
        status_text = Localize("No fuel");
        status_col  = red;
    } else {
        status_text = Localize("Drilling...");
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

    SDL_FRect icon_dst = drill_icon_rect(panel_rec);
    if (tex.drill) {
        SDL_SetTextureScaleMode(tex.drill, SDL_SCALEMODE_NEAREST);
        SDL_RenderTexture(renderer, tex.drill, nullptr, &icon_dst);
    }

    SDL_FRect fuel_r = fuel_slot_rect(panel_rec);
    SDL_FRect fuel_b = fuel_bar_rect(panel_rec);

    SDL_Texture* fuel_slot_tex = tex.fuel_slot ? tex.fuel_slot : tex.slot;
    if (fuel_slot_tex) {
        SDL_SetTextureScaleMode(fuel_slot_tex, SDL_SCALEMODE_NEAREST);
        SDL_RenderTexture(renderer, fuel_slot_tex, nullptr, &fuel_r);
    } else {
        SDL_SetRenderDrawColor(renderer, 22, 24, 29, 255);
        SDL_RenderFillRect(renderer, &fuel_r);
        SDL_SetRenderDrawColor(renderer, 44, 48, 58, 255);
        SDL_RenderRect(renderer, &fuel_r);
    }

    if (hovered_slot == 0) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 22);
        SDL_RenderFillRect(renderer, &fuel_r);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    if (fuel_slot && fuel_slot->texture) {
        SDL_FRect icon2 = { fuel_r.x + 5.0f, fuel_r.y + 5.0f, fuel_r.w - 10.0f, fuel_r.h - 10.0f };
        SDL_SetTextureScaleMode(fuel_slot->texture, SDL_SCALEMODE_NEAREST);
        SDL_RenderTexture(renderer, fuel_slot->texture, nullptr, &icon2);
    }

    if (font && fuel_slot && fuel_slot->amount > 1) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", fuel_slot->amount);
        int tw_int = 0, th_int = 0;
        TTF_GetStringSize(font, buf, 0, &tw_int, &th_int);
        float bg_w = (float)tw_int + 4.0f;
        float bg_h = (float)TTF_GetFontHeight(font) + 2.0f;
        SDL_FRect bg = {
            fuel_r.x + fuel_r.w - bg_w - 2.0f,
            fuel_r.y + fuel_r.h - bg_h - 2.0f,
            bg_w, bg_h
        };

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 8, 9, 11, 210);
        SDL_RenderFillRect(renderer, &bg);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        TextRenderer::DrawText(renderer, font, bg.x + 2.0f, bg.y + 1.0f, buf, {235, 237, 241, 255});
    }

    SDL_SetRenderDrawColor(renderer, 26, 29, 35, 255);
    SDL_RenderFillRect(renderer, &fuel_b);
    SDL_SetRenderDrawColor(renderer, 40, 44, 52, 255);
    SDL_RenderRect(renderer, &fuel_b);

    if (fuel_remaining > 0.0f) {
        SDL_FRect fill = { fuel_b.x, fuel_b.y, fuel_b.w * fuel_remaining, BAR_H };
        SDL_SetRenderDrawColor(renderer, 160, 40, 40, 255);
        SDL_RenderFillRect(renderer, &fill);
    }

    if (font) {
        char pct[16];
        snprintf(pct, sizeof(pct), "%.0f%%", fuel_remaining * 100.0f);
        int tw = 0, th = 0;
        TTF_GetStringSize(font, pct, 0, &tw, &th);
        TextRenderer::DrawText(renderer, font,
            fuel_b.x + fuel_b.w - (float)tw,
            fuel_b.y - (float)th - 2.0f,
            pct, muted);
    }
}
