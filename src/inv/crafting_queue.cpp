#include "inv/crafting_queue.h"
#include "inv/crafting.h"
#include "inv/inventory.h"
#include <unordered_map>
#include <algorithm>
#include "textrenderer.h"
#include "localization.h"

static constexpr float QUEUE_BG_ALPHA = 230.0f;
static constexpr float QUEUE_ITEM_SIZE = 48.0f;
static constexpr float QUEUE_PADDING = 10.0f;
static constexpr float QUEUE_BAR_HEIGHT = 12.0f;
static constexpr float DEFAULT_CRAFT_DURATION = 2.5f;

static int get_inventory_amount(const Inventory& inv, ItemType type) {
    return inv.get_amount(type);
}

bool CraftingQueue::enqueue(const Recipe& recipe, Inventory& inv, const std::vector<Recipe>& known_recipes) {
    std::unordered_map<ItemType, int> available;
    available.clear();

    available[ItemType::STONE] = inv.get_amount(ItemType::STONE);
    available[ItemType::IRON_ORE] = inv.get_amount(ItemType::IRON_ORE);
    available[ItemType::IRON_PLATE] = inv.get_amount(ItemType::IRON_PLATE);
    available[ItemType::COPPER_ORE] = inv.get_amount(ItemType::COPPER_ORE);
    available[ItemType::COAL] = inv.get_amount(ItemType::COAL);
    available[ItemType::FURNACE] = inv.get_amount(ItemType::FURNACE);
    available[ItemType::DRILL] = inv.get_amount(ItemType::DRILL);

    for (const QueuedCraft& entry : items) {
        if (!entry.recipe) continue;
        available[entry.recipe->result_type] += entry.recipe->result_amount * entry.count;
    }

    std::vector<QueuedCraft> plan;
    std::vector<ItemType> stack;

    if (!build_queue_for_recipe(&recipe, inv, known_recipes, available, plan, stack)) {
        return false;
    }

    items.insert(items.end(), plan.begin(), plan.end());
    return true;
}

const Recipe* CraftingQueue::find_recipe_for(ItemType type, const std::vector<Recipe>& known_recipes) const {
    for (const Recipe& recipe : known_recipes) {
        if (recipe.result_type == type) return &recipe;
    }
    return nullptr;
}

bool CraftingQueue::build_queue_for_recipe(const Recipe* recipe,
                                           Inventory& inv,
                                           const std::vector<Recipe>& known_recipes,
                                           std::unordered_map<ItemType, int>& available,
                                           std::vector<QueuedCraft>& out,
                                           std::vector<ItemType>& stack) {
    if (!recipe) return false;
    if (std::find(stack.begin(), stack.end(), recipe->result_type) != stack.end()) {
        return false;
    }

    stack.push_back(recipe->result_type);
    std::unordered_map<ItemType, int> work = available;

    for (const RecipeIngredient& ingredient : recipe->ingredients) {
        int have = get_inventory_amount(inv, ingredient.type) + work[ingredient.type];
        if (have >= ingredient.amount) {
            work[ingredient.type] -= ingredient.amount;
            continue;
        }

        int missing = ingredient.amount - have;
        const Recipe* sub_recipe = find_recipe_for(ingredient.type, known_recipes);
        if (!sub_recipe) {
            stack.pop_back();
            return false;
        }

        int times = (missing + sub_recipe->result_amount - 1) / sub_recipe->result_amount;
        for (int i = 0; i < times; ++i) {
            if (!build_queue_for_recipe(sub_recipe, inv, known_recipes, work, out, stack)) {
                stack.pop_back();
                return false;
            }
            work[sub_recipe->result_type] += sub_recipe->result_amount;
        }

        work[ingredient.type] -= ingredient.amount;
    }

    stack.pop_back();

    QueuedCraft queued;
    queued.recipe = recipe;
    queued.progress = 0.0f;
    queued.duration = recipe->craft_duration;
    queued.count = 1;
    out.push_back(queued);
    available[recipe->result_type] += recipe->result_amount;
    return true;
}

void CraftingQueue::consume_ingredients(const Recipe& recipe, Inventory& inv) {
    for (const RecipeIngredient& ingredient : recipe.ingredients) {
        inv.remove(ingredient.type, ingredient.amount);
    }
}

void CraftingQueue::update(float dt, Inventory& inv) {
    if (items.empty()) return;

    QueuedCraft& current = items.front();
    if (!current.recipe) {
        items.erase(items.begin());
        return;
    }

    current.progress += dt;
    if (current.progress < current.duration) return;

    consume_ingredients(*current.recipe, inv);

    bool can_place = false;
    vec2 size = {1, 1};
    
    if (current.recipe->result_type == ItemType::FURNACE) {
        can_place = true;
        size = {2, 2};
    } else if (current.recipe->result_type == ItemType::DRILL) {
        can_place = true;
        size = {3, 3};
    }

    Item* crafted = new Item(current.recipe->result_type,
                             current.recipe->name,
                             current.recipe->result_amount,
                             current.recipe->result_texture,
                             can_place,
                             size);
    inv.pick(crafted);

    items.erase(items.begin());
}

void CraftingQueue::draw(SDL_Renderer* renderer, TTF_Font* font, const SDL_FRect& screen_rect) const {
    if (!renderer || items.empty() || !font) return;

    const QueuedCraft& current = items.front();
    SDL_FRect bg = { screen_rect.x, screen_rect.y, screen_rect.w, screen_rect.h };
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 14, 15, 18, 248);
    SDL_RenderFillRect(renderer, &bg);
    SDL_SetRenderDrawColor(renderer, 42, 46, 54, 255);
    SDL_RenderRect(renderer, &bg);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    const float icon_size = QUEUE_ITEM_SIZE;
    SDL_FRect icon_rect = { bg.x + QUEUE_PADDING, bg.y + QUEUE_PADDING, icon_size, icon_size };
    if (current.recipe->result_texture) {
        SDL_SetTextureScaleMode(current.recipe->result_texture, SDL_SCALEMODE_NEAREST);
        SDL_RenderTexture(renderer, current.recipe->result_texture, nullptr, &icon_rect);
    }

    const SDL_Color title_color = {236, 238, 242, 255};
    const SDL_Color muted       = {120, 125, 135, 255};

    std::string localized_name = Localize(current.recipe->name);
    TextRenderer::DrawText(renderer, font, icon_rect.x + icon_rect.w + 8.0f, icon_rect.y, localized_name.c_str(), title_color);

    char buf[64];
    std::string in_queue = Localize("in queue");
    snprintf(buf, sizeof(buf), "%d %s", size(), in_queue.c_str());
    TextRenderer::DrawText(renderer, font, icon_rect.x + icon_rect.w + 8.0f, icon_rect.y + 18.0f, buf, muted);

    float bar_x = icon_rect.x;
    float bar_y = icon_rect.y + icon_rect.h + 10.0f;
    float bar_w = bg.w - QUEUE_PADDING * 2.0f;
    SDL_FRect bar_bg = { bar_x, bar_y, bar_w, QUEUE_BAR_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 32, 35, 41, 255);
    SDL_RenderFillRect(renderer, &bar_bg);

    float progress_pct = current.duration > 0.0f ? current.progress / current.duration : 0.0f;
    if (progress_pct > 1.0f) progress_pct = 1.0f;
    SDL_FRect bar_fg = { bar_x, bar_y, bar_w * progress_pct, QUEUE_BAR_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 106, 140, 255, 255);
    SDL_RenderFillRect(renderer, &bar_fg);

    char percent_buf[24];
    snprintf(percent_buf, sizeof(percent_buf), "%.0f%%", progress_pct * 100.0f);
    TextRenderer::DrawText(renderer, font, bar_x + bar_w - 40.0f, bar_y - 2.0f, percent_buf, title_color);

    float mouse_x = 0.0f, mouse_y = 0.0f;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    if (mouse_x >= bg.x && mouse_x <= bg.x + bg.w && mouse_y >= bg.y && mouse_y <= bg.y + bg.h) {
        char time_buf[64];
        snprintf(time_buf, sizeof(time_buf), "Time: %.1fs", current.duration);

        int name_w = 0, name_h = 0;
        TTF_GetStringSize(font, current.recipe->name.c_str(), 0, &name_w, &name_h);
        int time_w = 0, time_h = 0;
        TTF_GetStringSize(font, time_buf, 0, &time_w, &time_h);

        float pad = 6.0f;
        float tip_w = std::max((float)name_w, (float)time_w) + pad * 2.0f;
        float tip_h = (float)name_h + (float)time_h + pad * 3.0f;
        float tip_x = (float)mouse_x + 12.0f;
        float tip_y = (float)mouse_y + 12.0f;

        if (tip_x + tip_w > screen_rect.x + screen_rect.w) {
            tip_x = screen_rect.x + screen_rect.w - tip_w;
        }
        if (tip_y + tip_h > screen_rect.y + screen_rect.h) {
            tip_y = (float)mouse_y - tip_h - 12.0f;
        }
        if (tip_x < screen_rect.x) tip_x = screen_rect.x;
        if (tip_y < screen_rect.y) tip_y = screen_rect.y;

        SDL_FRect tip_bg = { tip_x, tip_y, tip_w, tip_h };
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 10, 11, 14, 220);
        SDL_RenderFillRect(renderer, &tip_bg);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 60, 65, 75, 255);
        SDL_RenderRect(renderer, &tip_bg);

        TextRenderer::DrawText(renderer, font, tip_x + pad, tip_y + pad, current.recipe->name.c_str(), title_color);
        TextRenderer::DrawText(renderer, font, tip_x + pad, tip_y + pad + (float)name_h + pad, time_buf, muted);
    }
}
