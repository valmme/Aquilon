#include "inv/slot.h"
#include "textrenderer.h"
#include "vmath.h"

static constexpr float SLOT_SIZE = 35.0f;

Slot::Slot(float x, float y) {
    dest.x = x;
    dest.y = y;
    dest.w = SLOT_SIZE;
    dest.h = SLOT_SIZE;
}

void Slot::update(Item*& cursor_item, const SDL_Event& e) {
    if (item) {
        item->dest.x = dest.x;
        item->dest.y = dest.y;
        item->dest.w = SLOT_SIZE;
        item->dest.h = SLOT_SIZE;
    }

    if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        pressed = false;
        return;
    }

    if (e.type != SDL_EVENT_MOUSE_BUTTON_DOWN) return;

    float mx = (float)e.button.x;
    float my = (float)e.button.y;
    if (!point_in_rec(mx, my, dest)) return;

    pressed = true;

    if (e.button.button == SDL_BUTTON_LEFT) {
        if (!item) {
            if (cursor_item) {
                item = cursor_item;
                cursor_item = nullptr;

                item->dest.x = dest.x;
                item->dest.y = dest.y;
                item->dest.w = SLOT_SIZE;
                item->dest.h = SLOT_SIZE;
            }
        } 
        
        else {
            if (cursor_item) {
                if (cursor_item->type == item->type) {
                    item->amount += cursor_item->amount;
                    delete cursor_item;
                    cursor_item = nullptr;
                } 
                
                else {
                    std::swap(cursor_item, item);
                    item->dest.x = dest.x;
                    item->dest.y = dest.y;
                    item->dest.w = SLOT_SIZE;
                    item->dest.h = SLOT_SIZE;
                }
            } 
            
            else {
                cursor_item = item;
                item = nullptr;
            }
        }
    }

    if (e.button.button == SDL_BUTTON_RIGHT) {
        if (item && cursor_item) {
            if (cursor_item->type == item->type) {
                cursor_item->amount += item->amount;
                delete item;
                item = nullptr;
            } 
            
            else {
                std::swap(cursor_item, item);
                item->dest.x = dest.x;
                item->dest.y = dest.y;
                item->dest.w = SLOT_SIZE;
                item->dest.h = SLOT_SIZE;
            }
        } 
        
        else if (item && !cursor_item) {
            if (item->amount > 1) {
                int half = item->amount / 2;
                int remainder = item->amount - half;

                item->amount = half;
                cursor_item = item->copy();
                cursor_item->amount = remainder;
            } 
            
            else {
                cursor_item = item;
                item = nullptr;
            }
        }
    }
}
void Slot::draw(SDL_Renderer* renderer, Textures tex, TTF_Font* font) const {
    if (tex.slot) {
        SDL_RenderTexture(renderer, tex.slot, nullptr, &dest);
    } 
    
    else {
        SDL_SetRenderDrawColor(renderer, 24, 25, 29, 235);
        SDL_RenderFillRect(renderer, &dest);
    }

    SDL_SetRenderDrawColor(renderer, 44, 48, 56, 255);
    SDL_RenderRect(renderer, &dest);

    if (item) {
        item->draw(renderer, SLOT_SIZE);

        if (item->amount > 1 && font) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", item->amount);
            float bg_w = 18.0f;
            float bg_h = (float)TTF_GetFontHeight(font) + 2.0f;

            SDL_FRect shadow = {
                dest.x + dest.w - bg_w - 2.0f,
                dest.y + dest.h - bg_h - 2.0f,
                bg_w,
                bg_h
            };
            SDL_SetRenderDrawColor(renderer, 8, 9, 11, 210);
            SDL_RenderFillRect(renderer, &shadow);

            TextRenderer::DrawText(renderer, font, shadow.x + 2.0f, shadow.y + 1.0f, buf, SDL_Color{235, 237, 241, 255});
        }
    }

    if (selected) {
        SDL_SetRenderDrawColor(renderer, 220, 196, 134, 38);
        SDL_RenderFillRect(renderer, &dest);
        SDL_SetRenderDrawColor(renderer, 220, 196, 134, 120);
        SDL_RenderRect(renderer, &dest);
    }

    if (pressed) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 120);
        SDL_RenderFillRect(renderer, &dest);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_RenderRect(renderer, &dest);
    }
}
