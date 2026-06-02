#include "inv/slot.h"
#include "vmath.h"
#include <cstring>

static constexpr float SLOT_SIZE = 25.0f;
static constexpr float AMOUNT_FONT_SIZE = 8.0f;

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

    if (e.type != SDL_EVENT_MOUSE_BUTTON_DOWN) return;

    float mx = (float)e.button.x;
    float my = (float)e.button.y;
    if (!point_in_rec(mx, my, dest)) return;

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
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 220);
        SDL_RenderFillRect(renderer, &dest);
    }

    SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);

    if (item) {
        item->draw(renderer, SLOT_SIZE);

        if (item->amount > 1 && font) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", item->amount);

            SDL_Color white = {255, 255, 255, 255};
            SDL_Surface* surf = TTF_RenderText_Blended(font, buf, strlen(buf), white);
            if (surf) {
                SDL_Texture* textTex = SDL_CreateTextureFromSurface(renderer, surf);
                if (textTex) {
                    float tw = (float)surf->w;
                    float th = (float)surf->h;

                    SDL_FRect shadow = {
                        dest.x + dest.w - tw - 2,
                        dest.y + dest.h - th - 2,
                        tw + 1,
                        th
                    };
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                    SDL_RenderFillRect(renderer, &shadow);

                    float scale = AMOUNT_FONT_SIZE / 10.0f;

                    SDL_FRect tdst = {
                        dest.x + dest.w - tw * scale - 2,
                        dest.y + dest.h - th * scale - 2,
                        tw * scale,
                        th * scale
                    };
                    SDL_RenderTexture(renderer, textTex, nullptr, &tdst);
                    SDL_DestroyTexture(textTex);
                }
                SDL_DestroySurface(surf);
            }
        }
    }

    if (selected) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 60);
        SDL_RenderFillRect(renderer, &dest);
    }
}