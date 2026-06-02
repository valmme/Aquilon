#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_filesystem.h>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include "player.h"
#include "textures.h"
#include "gui.h"
#include "gen/world.h"
#include "config.h"
#include "logger.h"
#include "inv/inventory.h"

const int TILE_SIZE = 32;

static const char* LogLevelName(Logger::Level level) {
    switch (level) {
        case Logger::Level::Trace: return "trace";
        case Logger::Level::Debug: return "debug";
        case Logger::Level::Info:  return "info";
        case Logger::Level::Warn:  return "warn";
        case Logger::Level::Error: return "error";
        case Logger::Level::Fatal: return "fatal";
        default:                   return "unknown";
    }
}

static void LogAvailableRenderers() {
    int count = SDL_GetNumRenderDrivers();
    if (count <= 0) {
        Logger::Log("SYSTEM", Logger::Level::Warn, "SDL reports no available render drivers.");
        return;
    }

    Logger::Log("SYSTEM", Logger::Level::Info, "Available SDL render drivers:");
    for (int i = 0; i < count; ++i) {
        const char* driver = SDL_GetRenderDriver(i);
        Logger::Log("SYSTEM", Logger::Level::Info, "  [%d] %s", i, driver ? driver : "<unknown>");
    }
}

static void DrawDebugText(SDL_Renderer* renderer, TTF_Font* font, float x, float y, const char* text, SDL_Color color) {
    if (!renderer || !font || !text || !*text) {
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font, text, std::strlen(text), color);
    if (!surface) {
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_DestroySurface(surface);
        return;
    }

    SDL_FRect dst = {x, y, (float)surface->w, (float)surface->h};
    SDL_DestroySurface(surface);
    SDL_RenderTexture(renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

struct MiningState {
    bool active = false;
    int tile_x = 0;
    int tile_y = 0;
    TileType tile_type = EMPTY;
    float duration = 0.0f;
    float progress = 0.0f;
};

static SDL_Point ScreenToTile(const Camera& cam, float screen_x, float screen_y) {
    const float world_x = cam.x + screen_x / cam.zoom;
    const float world_y = cam.y + screen_y / cam.zoom;
    return {
        (int)std::floor(world_x / (float)TILE_SIZE),
        (int)std::floor(world_y / (float)TILE_SIZE)
    };
}

static float MiningDurationFor(TileType type) {
    switch (type) {
        case STONE: return 0.80f;
        case ORE:  return 1.10f;
        default:   return 0.0f;
    }
}

static bool IsMineable(TileType type) {
    return type == STONE || type == ORE;
}

static const char* TileResourceName(TileType type) {
    switch (type) {
        case STONE: return "Stone";
        case ORE:  return "Iron Ore";
        default:   return "Unknown";
    }
}

static Item* MakeDropForTile(const Tile& tile, const Textures& tex) {
    switch (tile.type) {
        case STONE:
            return new Item{ItemType::STONE, "Stone", 1, tex.stone};
        case ORE:
            return new Item{ItemType::IRON_ORE, "Iron Ore", 1, tex.ore};
        default:
            return nullptr;
    }
}

static void DrawMiningProgressBar(SDL_Renderer* renderer, int win_w, int win_h, const MiningState& mining) {
    if (!renderer || !mining.active || mining.duration <= 0.0f) {
        return;
    }

    float progress = mining.progress / mining.duration;
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    float bar_w = (float)(win_w - 48);
    if (bar_w > 420.0f) bar_w = 420.0f;
    if (bar_w < 0.0f) bar_w = 0.0f;
    const float bar_x = ((float)win_w - bar_w) * 0.5f;
    const float bar_y = (float)win_h - 20.0f;
    const float bar_h = 6.0f;

    SDL_SetRenderDrawColor(renderer, 12, 15, 18, 220);
    SDL_FRect bg = {bar_x, bar_y, bar_w, bar_h};
    SDL_RenderFillRect(renderer, &bg);

    SDL_SetRenderDrawColor(renderer, 58, 66, 76, 220);
    SDL_FRect top = {bar_x, bar_y, bar_w, 1.0f};
    SDL_FRect bottom = {bar_x, bar_y + bar_h - 1.0f, bar_w, 1.0f};
    SDL_RenderFillRect(renderer, &top);
    SDL_RenderFillRect(renderer, &bottom);

    const float fill_w = (bar_w - 2.0f) * progress;
    if (fill_w > 0.0f) {
        SDL_SetRenderDrawColor(renderer, 216, 176, 80, 255);
        SDL_FRect fill = {bar_x + 1.0f, bar_y + 1.0f, fill_w, bar_h - 2.0f};
        SDL_RenderFillRect(renderer, &fill);
    }
}

int main() {
    Logger::SetLogFile("aquilon.log");
    Logger::SetConsoleOutput(true);

    Logger::Log("SYSTEM", Logger::Level::Info, "Initializing SDL video subsystem.");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        Logger::Log("SYSTEM", Logger::Level::Fatal, "Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    std::string config_path = "aquilon.cfg";
    if (const char* base_path = SDL_GetBasePath(); base_path && *base_path) {
        config_path = std::string(base_path) + "aquilon.cfg";
    }

    AppConfig config;
    LoadAppConfig(config_path, config);
    Logger::SetLogLevel(config.log_level);

    Logger::Log("APPLICATION", Logger::Level::Info, "Starting Aquilon...");
    Logger::Log("APPLICATION", Logger::Level::Info, "Log level: %s", LogLevelName(config.log_level));

    SDL_Window* window = SDL_CreateWindow("Aquilon", 800, 600, 0);
    if (!window) {
        Logger::Log("SYSTEM", Logger::Level::Fatal, "Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    Logger::Log("SYSTEM", Logger::Level::Info, "Created window: 800x600.");

    if (!config.renderer_backend.empty()) {
        Logger::Log("SYSTEM", Logger::Level::Info, "Requested renderer backend: %s", config.renderer_backend.c_str());
    } else {
        Logger::Log("SYSTEM", Logger::Level::Info, "No renderer backend requested; SDL will choose the default.");
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window,
        config.renderer_backend.empty() ? nullptr : config.renderer_backend.c_str());
    if (!renderer) {
        Logger::Log("SYSTEM", Logger::Level::Fatal, "Failed to create renderer: %s", SDL_GetError());
        LogAvailableRenderers();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    Logger::Log("SYSTEM", Logger::Level::Info, "Created SDL renderer backend: %s",
                SDL_GetRendererName(renderer) ? SDL_GetRendererName(renderer) : "<unknown>");

    if (!SDL_SetRenderVSync(renderer, config.vsync_enabled ? 1 : 0)) {
        Logger::Log("SYSTEM", Logger::Level::Warn,
                    "SDL could not apply VSync=%s: %s",
                    config.vsync_enabled ? "on" : "off", SDL_GetError());
    } else {
        Logger::Log("SYSTEM", Logger::Level::Info, "VSync %s.", config.vsync_enabled ? "enabled" : "disabled");
    }

    Textures tex = load_textures(renderer);
    Logger::Log("APPLICATION", Logger::Level::Info, "Loaded textures.");

    GUIEngine gui_engine(renderer);
    Logger::Log("UI", Logger::Level::Info, "Initialized GUI engine.");

    std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> debug_font(
        TTF_OpenFont("resources/fonts/arial.ttf", 14), &TTF_CloseFont
    );
    if (!debug_font) {
        Logger::Log("UI", Logger::Level::Warn,
                    "Debug overlay font not found; Game Status text will not render.");
    }

    World world;
    Player player;
    Camera cam;
    Inventory inv(gui_engine, tex, debug_font.get());
    inv.pick(new Item{ItemType::UNDEFINED, "Undefined", 67, tex.none});
    Logger::Log("GAMEPLAY", Logger::Level::Info, "Initialized world, player, and camera.");

    int win_w = 800, win_h = 600;
    SDL_GetWindowSize(window, &win_w, &win_h);

    float player_cx_init = player.player.x + player.player.w * 0.5f;
    float player_cy_init = player.player.y + player.player.h * 0.5f;
    cam.x = player_cx_init - (win_w * 0.5f) / cam.zoom;
    cam.y = player_cy_init - (win_h * 0.5f) / cam.zoom;

    int initial_player_tile_x = (int)player.player.x / TILE_SIZE;
    int initial_player_tile_y = (int)player.player.y / TILE_SIZE;
    world.update(initial_player_tile_x, initial_player_tile_y);

    float fps = 0.0f;
    MiningState mining;
    bool right_hold_blocked = false;
    bool resource_panel_visible = false;
    const char* resource_panel_name = "";
    int resource_panel_yield = 0;
    TileType resource_panel_type = EMPTY;

    GUIWindow* main_window = gui_engine.create_window(10, 10, 330, 220, "Game Status");
    GUIWindow* resource_panel = gui_engine.create_info_window(0, 0, 220, 110);
    resource_panel->set_visible(false);
    resource_panel->set_background_color(21, 24, 29, 245);
    resource_panel->set_border_color(66, 74, 86, 255);
    resource_panel->set_content_draw_callback([&](SDL_Renderer* renderer, const SDL_FRect& content_rect) {
        SDL_SetRenderDrawColor(renderer, 31, 35, 42, 255);
        SDL_RenderFillRect(renderer, &content_rect);

        SDL_SetRenderDrawColor(renderer, 58, 70, 86, 255);
        SDL_FRect accent = {content_rect.x, content_rect.y, content_rect.w, 2.0f};
        SDL_RenderFillRect(renderer, &accent);

        SDL_FRect icon_bg = {content_rect.x + 10.0f, content_rect.y + 10.0f, 28.0f, 28.0f};
        SDL_SetRenderDrawColor(renderer, 12, 14, 18, 255);
        SDL_RenderFillRect(renderer, &icon_bg);

        SDL_Texture* icon = nullptr;
        if (resource_panel_type == STONE) icon = tex.stone;
        else if (resource_panel_type == ORE) icon = tex.ore;

        if (icon) {
            SDL_FRect icon_dst = {icon_bg.x + 2.0f, icon_bg.y + 2.0f, icon_bg.w - 4.0f, icon_bg.h - 4.0f};
            SDL_RenderTexture(renderer, icon, nullptr, &icon_dst);
        }

        const float left = icon_bg.x + icon_bg.w + 10.0f;
        float y = content_rect.y + 10.0f;
        const SDL_Color title_color = {240, 243, 247, 255};
        const SDL_Color muted = {182, 189, 197, 255};
        const SDL_Color accent_color = {216, 176, 80, 255};

        DrawDebugText(renderer, debug_font.get(), left, y, resource_panel_name, title_color);
        y += 18.0f;

        char line[128];
        snprintf(line, sizeof(line), "Yield: %d", resource_panel_yield);
        DrawDebugText(renderer, debug_font.get(), left, y, line, muted);
        y += 18.0f;

        snprintf(line, sizeof(line), "Type: %s", resource_panel_type == ORE ? "Ore" : "Stone");
        DrawDebugText(renderer, debug_font.get(), left, y, line, muted);
        y += 18.0f;

        DrawDebugText(renderer, debug_font.get(), left, y, "Hold RMB to mine", accent_color);
    });
    main_window->set_content_draw_callback([&](SDL_Renderer* renderer, const SDL_FRect& content_rect) {
        SDL_Color panel_fill = {24, 30, 34, 255};
        SDL_SetRenderDrawColor(renderer, panel_fill.r, panel_fill.g, panel_fill.b, panel_fill.a);
        SDL_RenderFillRect(renderer, &content_rect);

        SDL_Color accent = {90, 140, 170, 255};
        SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, accent.a);
        SDL_FRect top_line = {content_rect.x, content_rect.y, content_rect.w, 2.0f};
        SDL_RenderFillRect(renderer, &top_line);

        const float left = content_rect.x + 8.0f;
        float y = content_rect.y + 8.0f;
        const float line_step = 18.0f;
        const SDL_Color label = {235, 235, 235, 255};
        const SDL_Color muted = {180, 190, 200, 255};

        char line[128];

        snprintf(line, sizeof(line), "FPS: %.1f", fps);
        DrawDebugText(renderer, debug_font.get(), left, y, line, label);
        y += line_step;

        snprintf(line, sizeof(line), "Renderer: %s", SDL_GetRendererName(renderer) ? SDL_GetRendererName(renderer) : "<unknown>");
        DrawDebugText(renderer, debug_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "VSync: %s", config.vsync_enabled ? "on" : "off");
        DrawDebugText(renderer, debug_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "Player: x=%.1f y=%.1f", player.player.x, player.player.y);
        DrawDebugText(renderer, debug_font.get(), left, y, line, muted);
        y += line_step;

        int debug_player_tile_x = (int)player.player.x / TILE_SIZE;
        int debug_player_tile_y = (int)player.player.y / TILE_SIZE;
        snprintf(line, sizeof(line), "Tile: %d, %d", debug_player_tile_x, debug_player_tile_y);
        DrawDebugText(renderer, debug_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "Camera: x=%.1f y=%.1f zoom=%.2f", cam.x, cam.y, cam.zoom);
        DrawDebugText(renderer, debug_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "Chunks: %zu", world.get_chunks().size());
        DrawDebugText(renderer, debug_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "Log level: %s", LogLevelName(config.log_level));
        DrawDebugText(renderer, debug_font.get(), left, y, line, muted);
    });

    Uint64 last_counter = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    float delta_time = 0.0f;

    bool running = true;
    SDL_Event e;
    Logger::Log("APPLICATION", Logger::Level::Info, "Entering main loop.");

    while (running) {
        Uint64 current_counter = SDL_GetPerformanceCounter();
        delta_time = (float)(current_counter - last_counter) / frequency;
        last_counter = current_counter;
        if (delta_time > 0.0f) {
            fps = 1.0f / delta_time;
        }

        char title[128];
        snprintf(title, sizeof(title), "Aquilon - FPS: %.1f", fps);
        SDL_SetWindowTitle(window, title);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;

            bool gui_consumed = gui_engine.handle_event(e);
            main_window = gui_engine.get_window();

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_RIGHT && gui_consumed) {
                mining.active = false;
                mining.progress = 0.0f;
                right_hold_blocked = true;
            }

            if (!gui_consumed && e.type == SDL_EVENT_MOUSE_WHEEL) {
                float prev_zoom = cam.zoom;
                float factor = powf(1.1f, (float)e.wheel.y);
                float new_zoom = prev_zoom * factor;
                if (new_zoom < 0.25f) new_zoom = 0.25f;
                if (new_zoom > 4.0f) new_zoom = 4.0f;

                cam.zoom = new_zoom;
            }

            if (!gui_consumed && e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_MINUS || e.key.key == SDLK_KP_MINUS) {
                    cam.zoom /= 1.1f;
                    if (cam.zoom < 0.25f) cam.zoom = 0.25f;
                } else if (e.key.key == SDLK_EQUALS || e.key.key == SDLK_KP_PLUS) {
                    cam.zoom *= 1.1f;
                    if (cam.zoom > 4.0f) cam.zoom = 4.0f;
                }
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_RIGHT && !gui_consumed) {
                mining.active = false;
                mining.progress = 0.0f;
                right_hold_blocked = false;
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_RIGHT) {
                mining.active = false;
                mining.progress = 0.0f;
                right_hold_blocked = false;
            }

            if (!gui_consumed) {
                player.handle_input(e);
            }

            inv.handle_event(e);
        }

        player.update(delta_time);

        int player_tile_x = (int)player.player.x / TILE_SIZE;
        int player_tile_y = (int)player.player.y / TILE_SIZE;

        std::size_t chunk_count_before = world.get_chunks().size();
        world.update(player_tile_x, player_tile_y);
        std::size_t chunk_count_after = world.get_chunks().size();
        if (chunk_count_after != chunk_count_before) {
            Logger::Log("SYSTEM", Logger::Level::Debug,
                        "Chunk cache changed: %zu -> %zu around chunk (%d, %d).",
                        chunk_count_before, chunk_count_after,
                        player_tile_x / CHUNK_SIZE, player_tile_y / CHUNK_SIZE);
        }
        cam.update(player.player, win_w * 0.5f, win_h * 0.5f);

        float mx, my;
        SDL_MouseButtonFlags mouse_buttons = SDL_GetMouseState(&mx, &my);
        inv.update(mx, my);
        SDL_Point hovered_tile = ScreenToTile(cam, mx, my);
        Tile hovered_tile_data = world.get_tile(hovered_tile.x, hovered_tile.y);

        resource_panel_visible = IsMineable(hovered_tile_data.type);
        resource_panel_name = TileResourceName(hovered_tile_data.type);
        resource_panel_yield = hovered_tile_data.yield;
        resource_panel_type = hovered_tile_data.type;
        resource_panel->position.x = (float)win_w - resource_panel->size.x - 16.0f;
        resource_panel->position.y = 16.0f;
        resource_panel->set_visible(resource_panel_visible);

        if (!(mouse_buttons & SDL_BUTTON_RMASK)) {
            mining.active = false;
            mining.progress = 0.0f;
            right_hold_blocked = false;
        } else if (!right_hold_blocked) {
            if (!mining.active && IsMineable(hovered_tile_data.type)) {
                mining.active = true;
                mining.tile_x = hovered_tile.x;
                mining.tile_y = hovered_tile.y;
                mining.tile_type = hovered_tile_data.type;
                mining.duration = MiningDurationFor(hovered_tile_data.type);
                mining.progress = 0.0f;
            }
        }

        if (mining.active) {
            Tile mined_tile = world.get_tile(mining.tile_x, mining.tile_y);

            if (hovered_tile.x != mining.tile_x || hovered_tile.y != mining.tile_y || mined_tile.type != mining.tile_type || !IsMineable(mined_tile.type)) {
                mining.active = false;
                mining.progress = 0.0f;
            } else {
                mining.progress += delta_time;

                if (mining.progress >= mining.duration) {
                    world.set_tile(mining.tile_x, mining.tile_y, Tile{EMPTY, false, 0});
                    if (Item* drop = MakeDropForTile(mined_tile, tex)) {
                        inv.pick(drop);
                    }
                    mining.active = false;
                    mining.progress = 0.0f;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 50, 130, 230, 255);
        SDL_RenderClear(renderer);

        for (auto& [key, chunk] : world.get_chunks()) {
            for (int ty = 0; ty < CHUNK_SIZE; ty++) {
                for (int tx = 0; tx < CHUNK_SIZE; tx++) {
                    int world_x = (int)(chunk.pos.x * CHUNK_SIZE) + tx;
                    int world_y = (int)(chunk.pos.y * CHUNK_SIZE) + ty;

                    Tile t = chunk.tiles[tx][ty];

                    SDL_Texture* current = nullptr;
                    if (t.type == ICE) current = tex.ice;
                    else if (t.type == SNOW) current = tex.snow;
                    else if (t.type == STONE) current = tex.stone;
                    else if (t.type == ORE)  current = tex.ore;

                    if (!current) continue;

                    SDL_FRect dst = cam.WorldToScreenRect(world_x * TILE_SIZE, world_y * TILE_SIZE, (float)TILE_SIZE, (float)TILE_SIZE);

                    SDL_RenderTexture(renderer, current, NULL, &dst);
                }
            }
        }

        player.render(renderer, cam);
        gui_engine.render_all();
        inv.draw(renderer, debug_font.get());
        DrawMiningProgressBar(renderer, win_w, win_h, mining);

        SDL_RenderPresent(renderer);
    }

    Logger::Log("APPLICATION", Logger::Level::Info, "Leaving main loop.");

    free_textures(tex);
    Logger::Log("APPLICATION", Logger::Level::Info, "Released textures.");

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    Logger::Log("APPLICATION", Logger::Level::Info, "Aquilon shutdown complete.");
    return 0;
}
