#ifndef AQUILON_DRILL_PANEL_H
#define AQUILON_DRILL_PANEL_H

#include "inv/panel.h"
#include "inv/item.h"
#include "player.h"
#include "textures.h"
#include <SDL3_ttf/SDL_ttf.h>

class Inventory;

class DrillPanel : public Panel {
public:
    DrillPanel(Textures& tex, TTF_Font* font);
    ~DrillPanel() override;

    void set_target(Player::PlacedObject* obj);
    void update(float delta_time);

    void draw_panel(SDL_Renderer* renderer, const SDL_FRect& panel_rec, const Inventory& inv) const override;
    void handle_event(const SDL_Event& e, Inventory& inv, const SDL_FRect& panel_rec) override;
    void select_by_mouse(float mx, float my, const SDL_FRect& panel_rec) override;

    Player::PlacedObject* target = nullptr;
    Item* fuel_slot = nullptr;
    float fuel_remaining = 0.0f;
    int hovered_slot = -1;

    Textures& tex;
    TTF_Font* font;
};

#endif // AQUILON_DRILL_PANEL_H
