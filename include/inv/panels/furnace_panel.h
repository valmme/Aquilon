#pragma once
#include "inv/panel.h"
#include "inv/item.h"
#include "textures.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

class FurnacePanel : public Panel {
public:
    FurnacePanel(Textures& tex, TTF_Font* font);

    void draw_panel(SDL_Renderer* r, const SDL_FRect& panel_rec, const Inventory& inv) const override;
    void handle_event(const SDL_Event& e, Inventory& inv, const SDL_FRect& panel_rec) override;
    void select_by_mouse(float mx, float my, const SDL_FRect& panel_rec) override;

    Item* input_slot = nullptr;
    Item* fuel_slot = nullptr;
    Item* output_slot = nullptr;

    float smelt_progress = 0.0f;
    float fuel_remaining = 0.0f;

public:
    Textures& tex;
    TTF_Font* font;
    int hovered_slot = -1;
};