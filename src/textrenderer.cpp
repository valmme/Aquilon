#include "textrenderer.h"

#include <cmath>
#include <deque>
#include <string>
#include <unordered_map>

namespace TextRenderer {

namespace {
struct CacheKey {
    TTF_Font* font = nullptr;
    std::string text;
    Uint8 r = 0;
    Uint8 g = 0;
    Uint8 b = 0;
    Uint8 a = 0;

    bool operator==(const CacheKey& other) const {
        return font == other.font &&
               r == other.r && g == other.g && b == other.b && a == other.a &&
               text == other.text;
    }
};

struct CacheKeyHash {
    std::size_t operator()(const CacheKey& key) const {
        std::size_t h = std::hash<void*>{}(key.font);
        h ^= std::hash<std::string>{}(key.text) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= (std::size_t)key.r + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= (std::size_t)key.g + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= (std::size_t)key.b + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= (std::size_t)key.a + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

struct CachedTexture {
    SDL_Texture* texture = nullptr;
    int w = 0;
    int h = 0;
};

static constexpr std::size_t CACHE_LIMIT = 256;
static std::unordered_map<CacheKey, CachedTexture, CacheKeyHash> cache;
static std::deque<CacheKey> cache_order;

static void EvictOldest() {
    if (cache_order.empty()) {
        return;
    }

    CacheKey key = std::move(cache_order.front());
    cache_order.pop_front();

    auto it = cache.find(key);
    if (it != cache.end()) {
        if (it->second.texture) {
            SDL_DestroyTexture(it->second.texture);
        }
        cache.erase(it);
    }
}

static const CachedTexture* GetCachedTexture(SDL_Renderer* renderer,
                                             TTF_Font* font,
                                             std::string_view text,
                                             const SDL_Color& color) {
    if (!renderer || !font || text.empty()) {
        return nullptr;
    }

    CacheKey key;
    key.font = font;
    key.text.assign(text.begin(), text.end());
    key.r = color.r;
    key.g = color.g;
    key.b = color.b;
    key.a = color.a;

    auto it = cache.find(key);
    if (it != cache.end()) {
        return &it->second;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font, key.text.c_str(), static_cast<int>(key.text.size()), color);
    if (!surface) {
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_DestroySurface(surface);
        return nullptr;
    }

    CachedTexture entry;
    entry.texture = texture;
    entry.w = surface->w;
    entry.h = surface->h;

    SDL_DestroySurface(surface);

    auto [inserted_it, _] = cache.emplace(std::move(key), entry);
    cache_order.push_back(inserted_it->first);

    while (cache.size() > CACHE_LIMIT) {
        EvictOldest();
    }

    return &inserted_it->second;
}
}

bool DrawText(SDL_Renderer* renderer,
              TTF_Font* font,
              float x,
              float y,
              std::string_view text,
              const SDL_Color& color,
              SDL_FRect* out_dst) {
    const CachedTexture* cached = GetCachedTexture(renderer, font, text, color);
    if (!cached || !cached->texture) {
        return false;
    }

    SDL_FRect dst = {
        std::floor(x),
        std::floor(y),
        (float)cached->w,
        (float)cached->h
    };

    if (out_dst) {
        *out_dst = dst;
    }

    SDL_RenderTexture(renderer, cached->texture, nullptr, &dst);
    return true;
}

bool DrawTextShadow(SDL_Renderer* renderer,
                    TTF_Font* font,
                    float x,
                    float y,
                    std::string_view text,
                    const SDL_Color& fill_color,
                    const SDL_Color& shadow_color,
                    SDL_FRect* out_dst,
                    float shadow_offset_x,
                    float shadow_offset_y) {
    const CachedTexture* fill = GetCachedTexture(renderer, font, text, fill_color);
    const CachedTexture* shadow = GetCachedTexture(renderer, font, text, shadow_color);
    if (!fill || !shadow || !fill->texture || !shadow->texture) {
        return false;
    }

    SDL_FRect dst = {
        std::floor(x),
        std::floor(y),
        (float)fill->w,
        (float)fill->h
    };

    if (out_dst) {
        *out_dst = dst;
    }

    SDL_FRect shadow_dst = {
        dst.x + shadow_offset_x,
        dst.y + shadow_offset_y,
        (float)shadow->w,
        (float)shadow->h
    };

    SDL_RenderTexture(renderer, shadow->texture, nullptr, &shadow_dst);
    SDL_RenderTexture(renderer, fill->texture, nullptr, &dst);
    return true;
}

void ClearCache() {
    for (auto& [_, entry] : cache) {
        if (entry.texture) {
            SDL_DestroyTexture(entry.texture);
            entry.texture = nullptr;
        }
    }

    cache.clear();
    cache_order.clear();
}

}
