#ifndef AQUILON_INVENTORY_H
#define AQUILON_INVENTORY_H

#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "slot.h"
#include "item.h"
#include "gui.h"

class Inventory {
public:
    std::vector<Slot> slots;
    Item* cursor_item = nullptr;
    bool open = false;

    Inventory(GUIEngine& gui, Textures tex, TTF_Font* font);
    ~Inventory();

    Textures tex;

    void handle_event(const SDL_Event& e);
    void update(float mx, float my);
    void draw(SDL_Renderer* renderer, TTF_Font* font) const;

    void pick(Item* item);
    void remove(ItemType type, int amount);
    int get_amount(ItemType type) const;

    bool is_dragging_placeable() const;
    const Item* get_dragged_item() const;

    Item* release_cursor_item();
    void set_cursor_item(Item* item);

    bool consume_cursor_item_one() {
        if (!cursor_item) return false;

        cursor_item->amount -= 1;
        if (cursor_item->amount <= 0) {
            delete cursor_item;
            cursor_item = nullptr;
        }
        return true;
    }

private:
    GUIEngine& gui;
    GUIWindow* window = nullptr;
    TTF_Font* font = nullptr;

    void open_window();
    void close_window();
};



#endif // AQUILON_INVENTORY_H