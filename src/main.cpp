#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_filesystem.h>
#include <cstddef>
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
    Inventory inv(gui_engine, debug_font.get());
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

    GUIWindow* main_window = gui_engine.create_window(10, 10, 330, 220, "Game Status");
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

            if (!gui_consumed) {
                player.handle_input(e);
            }

            inv.handle_event(e);
        }

        player.update(delta_time);

        float mx, my;
        SDL_GetMouseState(&mx, &my);
        inv.update(mx, my);

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

        SDL_SetRenderDrawColor(renderer, 50, 130, 230, 255);
        SDL_RenderClear(renderer);

        for (auto& [key, chunk] : world.get_chunks()) {
            for (int ty = 0; ty < CHUNK_SIZE; ty++) {
                for (int tx = 0; tx < CHUNK_SIZE; tx++) {
                    int world_x = (int)(chunk.pos.x * CHUNK_SIZE) + tx;
                    int world_y = (int)(chunk.pos.y * CHUNK_SIZE) + ty;

                    Tile t = chunk.tiles[tx][ty];

                    SDL_Texture* current = tex.ice;
                    if (t.type == ROCK) current = tex.rock;
                    else if (t.type == SNOW) current = tex.snow;
                    else if (t.type == ORE)  current = tex.ore;

                    SDL_FRect dst = cam.WorldToScreenRect(world_x * TILE_SIZE, world_y * TILE_SIZE, (float)TILE_SIZE, (float)TILE_SIZE);

                    SDL_RenderTexture(renderer, current, NULL, &dst);
                }
            }
        }

        player.render(renderer, cam);
        gui_engine.render_all();

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
