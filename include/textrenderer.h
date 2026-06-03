#ifndef AQUILON_TEXTRENDERER_H
#define AQUILON_TEXTRENDERER_H

#include <string_view>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace TextRenderer {

bool DrawText(SDL_Renderer* renderer,
              TTF_Font* font,
              float x,
              float y,
              std::string_view text,
              const SDL_Color& color,
              SDL_FRect* out_dst = nullptr);

bool DrawTextShadow(SDL_Renderer* renderer,
                    TTF_Font* font,
                    float x,
                    float y,
                    std::string_view text,
                    const SDL_Color& fill_color,
                    const SDL_Color& shadow_color,
                    SDL_FRect* out_dst = nullptr,
                    float shadow_offset_x = 1.0f,
                    float shadow_offset_y = 1.0f);

void ClearCache();

}

#endif // AQUILON_TEXTRENDERER_H
