#ifndef AQUILON_CRAFTING_H
#define AQUILON_CRAFTING_H

#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "item.h"
#include "inv/panel.h"
#include "textures.h"

class Inventory;

struct RecipeIngredient {
    ItemType type = ItemType::NONE;
    int amount = 0;
};

struct Recipe {
    std::string name;
    int result_amount = 1;
    SDL_Texture* result_texture = nullptr;
    float craft_duration = 2.5f;
    ItemType result_type = ItemType::NONE;
    std::vector<RecipeIngredient> ingredients;
};

#include "inv/crafting_queue.h"

class CraftingSystem : public Panel {
public:
    CraftingSystem(Textures& tex, TTF_Font* font);

    void add_recipe(const Recipe& recipe);
    void initialize_recipes(Textures* tex);

    bool can_craft(const Inventory& inv, const Recipe& recipe) const;
    bool craft_selected(Inventory& inv);
    bool has_queue() const;

    void update(float delta_time, Inventory& inv);
    void handle_event(const SDL_Event& e, Inventory& inv, const SDL_FRect& panel_rect);
    void draw_panel(SDL_Renderer* renderer, const SDL_FRect& panel_rect, const Inventory& inv) const;
    void draw_queue(SDL_Renderer* renderer, TTF_Font* font, const SDL_FRect& screen_rect) const;
    void select_by_mouse(float mx, float my, const SDL_FRect& panel_rect);

private:
    Textures& tex;
    TTF_Font* font = nullptr;

    std::vector<Recipe> recipes;
    int selected = -1;
    int hovered = -1;
    CraftingQueue queue;

    static const char* item_type_name(ItemType type);
};

#endif