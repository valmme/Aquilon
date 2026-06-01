#ifndef AQUILON_INVENTORY_H
#define AQUILON_INVENTORY_H

#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "slot.h"
#include "item.h"

class Inventory {
public:
    std::vector<Slot> slots;
    Item* cursor_item = nullptr;

    Inventory();
    ~Inventory();

    void handle_event(const SDL_Event& e);
    void update(float mx, float my);
    void draw(SDL_Renderer* renderer, TTF_Font* font) const;

    void pick(Item* item);
    void remove(ItemType type, int amount);
    int get_amount(ItemType type) const;
};

#endif // AQUILON_INVENTORY_H