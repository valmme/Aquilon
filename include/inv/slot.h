#ifndef AQUILON_SLOT_H
#define AQUILON_SLOT_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "textures.h"
#include "item.h"

class Slot {
public:
    Item* item = nullptr;
    SDL_FRect dest = {0, 0, 50, 50};
    bool selected = false;

    Slot(float x, float y);

    void update(Item*& cursor_item, const SDL_Event& e);
    void draw(SDL_Renderer* renderer, Textures tex, TTF_Font* font) const;
};

#endif // AQUILON_SLOT_H