#pragma once
#include "inv/panel.h"
#include "inv/item.h"
#include "textures.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <vector>

struct FurnaceRecipe {
    Item* input;
    Item* output;
    float time = 1.0f;
};

class FurnacePanel : public Panel {
public:
    FurnacePanel(Textures& tex, TTF_Font* font);
    ~FurnacePanel() override;

    void initialize_recipes();

    void add_recipe(Item* input, Item* output, float time);
    const FurnaceRecipe* find_recipe(ItemType input) const;
    const Item* get_recipe_output(ItemType input) const;

    void update(float delta_time);
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

    std::vector<FurnaceRecipe> recipes;
};