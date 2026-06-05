#ifndef AQUILON_PANEL_H
#define AQUILON_PANEL_H

#include <SDL3/SDL.h>

class Inventory;

class Panel {
public:
    virtual ~Panel() = default;
    virtual void draw_panel(SDL_Renderer* renderer, const SDL_FRect& panel_rec, const Inventory& inv) const = 0;
    virtual void handle_event(const SDL_Event& e, Inventory& inv, const SDL_FRect& panel_rec) = 0;
    virtual void select_by_mouse(float mx, float my, const SDL_FRect& panel_rec) = 0;  
};

#endif // AQUILON_PANEL_H