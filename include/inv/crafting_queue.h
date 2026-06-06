#ifndef AQUILON_CRAFTING_QUEUE_H
#define AQUILON_CRAFTING_QUEUE_H

#include <vector>
#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "item.h"

class Inventory;
struct Recipe;

struct QueuedCraft {
    const Recipe* recipe = nullptr;
    float progress = 0.0f;
    float duration = 2.5f;
    int count = 1;
};

class CraftingQueue {
public:
    CraftingQueue() = default;

    bool enqueue(const Recipe& recipe, Inventory& inv, const std::vector<Recipe>& known_recipes);
    void update(float dt, Inventory& inv);
    void draw(SDL_Renderer* renderer, TTF_Font* font, const SDL_FRect& screen_rect) const;

    std::unordered_map<ItemType, int> compute_available(const Inventory& inv) const;

    int size() const { return (int)items.size(); }
    bool empty() const { return items.empty(); }

private:
    std::vector<QueuedCraft> items;

    bool build_queue_for_recipe(const Recipe* recipe,
                                Inventory& inv,
                                const std::vector<Recipe>& known_recipes,
                                std::unordered_map<ItemType, int>& available,
                                std::vector<QueuedCraft>& out,
                                std::vector<ItemType>& stack);
    const Recipe* find_recipe_for(ItemType type, const std::vector<Recipe>& known_recipes) const;
    void consume_ingredients(const Recipe& recipe, Inventory& inv);
};

#endif // AQUILON_CRAFTING_QUEUE_H
