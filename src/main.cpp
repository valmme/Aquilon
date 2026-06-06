#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_filesystem.h>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include "player.h"
#include "textures.h"
#include "gui.h"
#include "textrenderer.h"
#include "localization.h"
#include "gen/world.h"
#include "config.h"
#include "logger.h"
#include "inv/crafting.h"
#include "inv/inventory.h"
#include "inv/item.h"
#include "audio.h"
#include "inv/panels/furnace_panel.h"

const int TILE_SIZE = 32;

static const char* log_level_name(Logger::Level level) {
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

static void log_available_renderers() {
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

    std::string localization_path = "resources/localization/" + config.language + ".loc";
    if (const char* base_path = SDL_GetBasePath(); base_path && *base_path) {
        localization_path = std::string(base_path) + "resources/localization/" + config.language + ".loc";
    }
    LoadLocalization(localization_path);

    AudioEngine audio;
    if (!audio.Init()) {
        Logger::Log("AUDIO", Logger::Level::Warn, "Audio engine failed to initialize.");
    }

    Logger::Log("APPLICATION", Logger::Level::Info, "Starting Aquilon...");
    Logger::Log("APPLICATION", Logger::Level::Info, "Log level: %s", log_level_name(config.log_level));

    SDL_Window* window = SDL_CreateWindow("Aquilon", config.window_width, config.window_height, 0);
    if (!window) {
        Logger::Log("SYSTEM", Logger::Level::Fatal, "Failed to create window: %s", SDL_GetError());
        audio.Shutdown();
        SDL_Quit();
        return 1;
    }
    Logger::Log("SYSTEM", Logger::Level::Info, "Created window: %dx%d.", config.window_width, config.window_height);

    if (!config.renderer_backend.empty()) {
        Logger::Log("SYSTEM", Logger::Level::Info, "Requested renderer backend: %s", config.renderer_backend.c_str());
    } else {
        Logger::Log("SYSTEM", Logger::Level::Info, "No renderer backend requested; SDL will choose the default.");
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window,
        config.renderer_backend.empty() ? nullptr : config.renderer_backend.c_str());
    if (!renderer) {
        Logger::Log("SYSTEM", Logger::Level::Fatal, "Failed to create renderer: %s", SDL_GetError());
        log_available_renderers();
        audio.Shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    Logger::Log("SYSTEM", Logger::Level::Info, "Created SDL renderer backend: %s",
                SDL_GetRendererName(renderer) ? SDL_GetRendererName(renderer) : "<unknown>");

    if (SDL_SetRenderVSync(renderer, config.vsync_enabled ? 1 : 0) != 0) {
        Logger::Log("SYSTEM", Logger::Level::Warn,
                    "SDL could not apply VSync=%s: %s",
                    config.vsync_enabled ? "on" : "off", SDL_GetError());
    } else {
        Logger::Log("SYSTEM", Logger::Level::Info, "VSync %s.", config.vsync_enabled ? "enabled" : "disabled");
    }

    Textures tex = LoadTextures(renderer);
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
    Player player(config.input);
    Camera cam;

    Inventory inv(gui_engine, tex, debug_font.get(), config.input);
    CraftingSystem* crafting = new CraftingSystem(tex, debug_font.get());
    FurnacePanel* furnace_panel = new FurnacePanel(tex, debug_font.get());
    initialize_items(tex);
    inv.set_crafting_system(crafting);
    inv.set_active_panel(crafting);

    player.on_object_clicked = [&](const Player::PlacedObject& obj) {
        if (obj.type == ItemType::FURNACE) {
            if (!inv.open) inv.open_window();
            inv.set_active_panel(furnace_panel);
        }
    };

    inv.pick(item_stack(Item::FURNACE, 67));
    inv.pick(item_stack(Item::IRON_PLATE, 31));
    inv.pick(item_stack(Item::COAL, 32));

    crafting->initialize_recipes(&tex);

    Logger::Log("GAMEPLAY", Logger::Level::Info, "Initialized world, player, and camera.");

    int win_w = 800, win_h = 600;
    SDL_GetWindowSize(window, &win_w, &win_h);

    float player_cx_init = player.player.x + player.player.w * 0.5f;
    float player_cy_init = player.player.y + player.player.h * 0.5f;
    cam.x = player_cx_init - (win_w * 0.5f) / cam.zoom;
    cam.y = player_cy_init - (win_h * 0.5f) / cam.zoom;

    int initial_player_tile_x = (int)player.player.x / TILE_SIZE;
    int initial_player_tile_y = (int)player.player.y / TILE_SIZE;
    world.update(initial_player_tile_x, initial_player_tile_y, config.chunk_distance);

    float fps = 0.0f;
    bool right_hold_blocked = false;
    bool resource_panel_visible = false;
    std::string resource_panel_name;
    int resource_panel_yield = 0;
    TileType resource_panel_type = EMPTY;

    const std::string game_status_title = Localize("Game Status");
    GUIWindow* main_window = gui_engine.CreateWindow(SDL_FRect{10, 10, 330, 220}, game_status_title);
    GUIWindow* resource_panel = gui_engine.CreateInfoWindow(SDL_FRect{0, 0, 220, 110});
    GUIWindow* crafting_queue_window = gui_engine.CreateQueueWindow(SDL_FRect{10.0f, (float)win_h - 120.0f, 260.0f, 100.0f});
    crafting_queue_window->SetVisible(false);
    crafting_queue_window->SetBackgroundColor(SDL_Color{14, 15, 18, 230});
    crafting_queue_window->SetBorderColor(SDL_Color{42, 46, 54, 255});
    crafting_queue_window->SetContentDrawCallback([&](SDL_Renderer* renderer, const SDL_FRect& content_rect) {
        if (crafting) crafting->draw_queue(renderer, debug_font.get(), content_rect);
    });
    resource_panel->SetVisible(false);
    resource_panel->SetBackgroundColor(SDL_Color{21, 24, 29, 245});
    resource_panel->SetBorderColor(SDL_Color{66, 74, 86, 255});
    resource_panel->SetContentDrawCallback([&](SDL_Renderer* renderer, const SDL_FRect& content_rect) {
        SDL_SetRenderDrawColor(renderer, 16, 17, 21, 255);
        SDL_RenderFillRect(renderer, &content_rect);

        SDL_SetRenderDrawColor(renderer, 44, 48, 56, 255);
        SDL_FRect accent = {content_rect.x, content_rect.y, content_rect.w, 1.0f};
        SDL_RenderFillRect(renderer, &accent);

        SDL_FRect icon_bg = {content_rect.x + 10.0f, content_rect.y + 10.0f, 28.0f, 28.0f};
        SDL_SetRenderDrawColor(renderer, 10, 11, 13, 255);
        SDL_RenderFillRect(renderer, &icon_bg);

        SDL_Texture* icon = nullptr;
        if (resource_panel_type == STONE) icon = tex.stone;
        else if (resource_panel_type == IRON_ORE) icon = tex.iron_ore;

        if (icon) {
            SDL_FRect icon_dst = {icon_bg.x + 2.0f, icon_bg.y + 2.0f, icon_bg.w - 4.0f, icon_bg.h - 4.0f};
            SDL_RenderTexture(renderer, icon, nullptr, &icon_dst);
        }

        const float left = icon_bg.x + icon_bg.w + 10.0f;
        float y = content_rect.y + 10.0f;
        const SDL_Color title_color = {238, 240, 243, 255};
        const SDL_Color muted = {170, 176, 184, 255};
        const SDL_Color accent_color = {220, 196, 134, 255};

        TextRenderer::DrawText(renderer, debug_font.get(), left, y, resource_panel_name, title_color);
        y += 18.0f;

        char line[128];
        const std::string yield_label = Localize("Yield");
        snprintf(line, sizeof(line), "%s: %d", yield_label.c_str(), resource_panel_yield);
        TextRenderer::DrawText(renderer, debug_font.get(), left, y, line, muted);
        y += 18.0f;

        const std::string type_label = Localize("Type");
        const std::string resource_name = resource_panel_type == IRON_ORE ? Localize("Iron Ore") : Localize("Stone");
        snprintf(line, sizeof(line), "%s: %s", type_label.c_str(), resource_name.c_str());
        TextRenderer::DrawText(renderer, debug_font.get(), left, y, line, muted);
        y += 18.0f;

        TextRenderer::DrawText(renderer, debug_font.get(), left, y, Localize("Hold RMB to mine"), accent_color);
    });

    main_window->SetContentDrawCallback([&](SDL_Renderer* renderer, const SDL_FRect& content_rect) {
        SDL_Color panel_fill = {16, 17, 21, 255};
        SDL_SetRenderDrawColor(renderer, panel_fill.r, panel_fill.g, panel_fill.b, panel_fill.a);
        SDL_RenderFillRect(renderer, &content_rect);

        SDL_Color accent = {58, 62, 70, 255};
        SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, accent.a);
        SDL_FRect top_line = {content_rect.x, content_rect.y, content_rect.w, 1.0f};
        SDL_RenderFillRect(renderer, &top_line);

        const float left = content_rect.x + 8.0f;
        float y = content_rect.y + 8.0f;
        const float line_step = 18.0f;
        const SDL_Color label = {238, 239, 242, 255};
        const SDL_Color muted = {165, 172, 180, 255};

        char line[128];
        const std::string fps_label = Localize("FPS");
        const std::string renderer_label = Localize("Renderer");
        const std::string vsync_label = Localize("VSync");
        const std::string player_label = Localize("Player");
        const std::string tile_label = Localize("Tile");
        const std::string camera_label = Localize("Camera");
        const std::string chunks_label = Localize("Chunks");
        const std::string log_level_label = Localize("Log level");

        snprintf(line, sizeof(line), "%s: %.1f", fps_label.c_str(), fps);
        TextRenderer::DrawText(renderer, debug_font.get(), left, y, line, label);
        y += line_step;

        snprintf(line, sizeof(line), "%s: %s", renderer_label.c_str(), SDL_GetRendererName(renderer) ? SDL_GetRendererName(renderer) : "<unknown>");
        TextRenderer::DrawText(renderer, debug_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "%s: %s", vsync_label.c_str(), config.vsync_enabled ? "on" : "off");
        TextRenderer::DrawText(renderer, debug_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "%s: x=%.1f y=%.1f", player_label.c_str(), player.player.x, player.player.y);
        TextRenderer::DrawText(renderer, debug_font.get(), left, y, line, muted);
        y += line_step;

        int debug_player_tile_x = (int)player.player.x / TILE_SIZE;
        int debug_player_tile_y = (int)player.player.y / TILE_SIZE;
        snprintf(line, sizeof(line), "%s: %d, %d", tile_label.c_str(), debug_player_tile_x, debug_player_tile_y);
        TextRenderer::DrawText(renderer, debug_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "%s: x=%.1f y=%.1f zoom=%.2f", camera_label.c_str(), cam.x, cam.y, cam.zoom);
        TextRenderer::DrawText(renderer, debug_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "%s: %zu", chunks_label.c_str(), world.get_chunks().size());
        TextRenderer::DrawText(renderer, debug_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "%s: %s", log_level_label.c_str(), log_level_name(config.log_level));
        TextRenderer::DrawText(renderer, debug_font.get(), left, y, line, muted);
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
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }

            bool gui_consumed = gui_engine.HandleEvent(e);
            main_window = gui_engine.GetWindow();

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_RIGHT && gui_consumed) {
                player.stop_mining();
                right_hold_blocked = true;
            }

            if (!gui_consumed && e.type == SDL_EVENT_MOUSE_WHEEL) {
                float factor = powf(1.1f, (float)e.wheel.y);
                cam.zoom *= factor;
                if (cam.zoom < 0.25f) cam.zoom = 0.25f;
                if (cam.zoom > 4.0f) cam.zoom = 4.0f;
            }

            if (!gui_consumed && e.type == SDL_EVENT_KEY_DOWN) {
                if (KeyBindMatches(config.input.zoom_out, e.key.key)) {
                    cam.zoom /= 1.1f;
                    if (cam.zoom < 0.25f) cam.zoom = 0.25f;
                } else if (KeyBindMatches(config.input.zoom_in, e.key.key)) {
                    cam.zoom *= 1.1f;
                    if (cam.zoom > 4.0f) cam.zoom = 4.0f;
                }
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_RIGHT && !gui_consumed) {
                player.stop_mining();
                right_hold_blocked = false;
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_RIGHT) {
                player.stop_mining();
                right_hold_blocked = false;
            }

            if (!gui_consumed) {
                player.handle_input(e);
            }

            inv.handle_event(e);
            player.handle_item_placement(e, cam, inv, !gui_consumed);
        }

        player.update(delta_time);

        int player_tile_x = (int)player.player.x / TILE_SIZE;
        int player_tile_y = (int)player.player.y / TILE_SIZE;

        std::size_t chunk_count_before = world.get_chunks().size();
        world.update(player_tile_x, player_tile_y, config.chunk_distance);
        std::size_t chunk_count_after = world.get_chunks().size();
        if (chunk_count_after != chunk_count_before) {
            Logger::Log("SYSTEM", Logger::Level::Debug,
                        "Chunk cache changed: %zu -> %zu around chunk (%d, %d).",
                        chunk_count_before, chunk_count_after,
                        player_tile_x / CHUNK_SIZE, player_tile_y / CHUNK_SIZE);
        }
        cam.update(player.player, win_w * 0.5f, win_h * 0.5f);

        float mouse_x = 0.0f, mouse_y = 0.0f;
        SDL_MouseButtonFlags mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);

        furnace_panel->update(delta_time);
        crafting->update(delta_time, inv);
        inv.update(mouse_x, mouse_y);

        const float view_left_world = cam.x;
        const float view_top_world = cam.y;
        const float view_right_world = cam.x + (float)win_w / cam.zoom;
        const float view_bottom_world = cam.y + (float)win_h / cam.zoom;

        const int visible_min_tile_x = (int)std::floor(view_left_world / (float)TILE_SIZE) - 1;
        const int visible_min_tile_y = (int)std::floor(view_top_world / (float)TILE_SIZE) - 1;
        const int visible_max_tile_x = (int)std::floor(view_right_world / (float)TILE_SIZE) + 1;
        const int visible_max_tile_y = (int)std::floor(view_bottom_world / (float)TILE_SIZE) + 1;

        SDL_Point hovered_tile = {
            (int)std::floor((cam.x + mouse_x / cam.zoom) / (float)TILE_SIZE),
            (int)std::floor((cam.y + mouse_y / cam.zoom) / (float)TILE_SIZE)
        };
        Tile hovered_tile_data = world.get_tile(hovered_tile.x, hovered_tile.y);

        bool mouse_over_gui = gui_engine.IsMouseOverAnyWindow(Vec2{(float)mouse_x, (float)mouse_y});
        resource_panel_visible = !mouse_over_gui && (hovered_tile_data.type == STONE || hovered_tile_data.type == IRON_ORE);
        resource_panel_name = hovered_tile_data.type == IRON_ORE ? Localize("Iron Ore") : Localize("Stone");
        resource_panel_yield = hovered_tile_data.yield;
        resource_panel_type = hovered_tile_data.type;
        resource_panel->size.x = (float)win_w - resource_panel->size.w - 16.0f;
        resource_panel->size.y = 16.0f;
        resource_panel->SetVisible(resource_panel_visible);

        if (!(mouse_buttons & SDL_BUTTON_RMASK)) {
            player.stop_mining();
            right_hold_blocked = false;
        } else if (!right_hold_blocked && inv.open == false) {
            if (!player.is_mining() && (hovered_tile_data.type == STONE || hovered_tile_data.type == IRON_ORE)) {
                player.start_mining(hovered_tile.x, hovered_tile.y, hovered_tile_data.type);
            }
        }

        player.update_mining(delta_time, world, inv, tex);

        SDL_SetRenderDrawColor(renderer, 230, 245, 255, 255);
        SDL_RenderClear(renderer);

        for (auto& [key, chunk] : world.get_chunks()) {
            const int chunk_tile_x0 = (int)chunk.pos.x * CHUNK_SIZE;
            const int chunk_tile_y0 = (int)chunk.pos.y * CHUNK_SIZE;
            const int chunk_tile_x1 = chunk_tile_x0 + CHUNK_SIZE - 1;
            const int chunk_tile_y1 = chunk_tile_y0 + CHUNK_SIZE - 1;

            if (chunk_tile_x1 < visible_min_tile_x || chunk_tile_x0 > visible_max_tile_x ||
                chunk_tile_y1 < visible_min_tile_y || chunk_tile_y0 > visible_max_tile_y) {
                continue;
            }

            const int local_min_x = std::max(0, visible_min_tile_x - chunk_tile_x0);
            const int local_min_y = std::max(0, visible_min_tile_y - chunk_tile_y0);
            const int local_max_x = std::min(CHUNK_SIZE - 1, visible_max_tile_x - chunk_tile_x0);
            const int local_max_y = std::min(CHUNK_SIZE - 1, visible_max_tile_y - chunk_tile_y0);

            for (int ty = 0; ty < CHUNK_SIZE; ty++) {
                if (ty < local_min_y || ty > local_max_y) {
                    continue;
                }

                for (int tx = 0; tx < CHUNK_SIZE; tx++) {
                    if (tx < local_min_x || tx > local_max_x) {
                        continue;
                    }

                    int world_x = chunk_tile_x0 + tx;
                    int world_y = chunk_tile_y0 + ty;

                    Tile t = chunk.tiles[tx][ty];

                    SDL_Texture* current = nullptr;
                    if (t.type == ICE) current = tex.ice;
                    else if (t.type == SNOW) current = tex.snow;
                    else if (t.type == STONE) current = tex.stone;
                    else if (t.type == IRON_ORE) current = tex.iron_ore;
                    else if (t.type == COAL) current = tex.coal;

                    if (!current) continue;

                    SDL_FRect dst = cam.world_to_screen_rect(
                        world_x * TILE_SIZE,
                        world_y * TILE_SIZE,
                        (float)TILE_SIZE,
                        (float)TILE_SIZE
                    );

                    float angle = t.type == ICE ? 0 : (float)(((world_x * 928371 + world_y * 12347) % 360 + 360) % 360);
                    SDL_RenderTextureRotated(renderer, current, nullptr, &dst, angle, nullptr, SDL_FLIP_NONE);
                }
            }
        }

        player.render_placed_objects(renderer, cam, visible_min_tile_x, visible_min_tile_y, visible_max_tile_x, visible_max_tile_y);

        player.render(renderer, cam);

        player.draw_item_placement_preview(renderer, cam, inv, mouse_x, mouse_y);

        if (crafting_queue_window) {
            crafting_queue_window->size.x = 10.0f;
            crafting_queue_window->size.y = (float)win_h - crafting_queue_window->size.h - 10.0f;
            crafting_queue_window->SetVisible(crafting && crafting->has_queue());
        }

        gui_engine.RenderAll();
        inv.draw(renderer, debug_font.get());

        player.draw_mining_progress_bar(renderer, win_w, win_h);
        SDL_RenderPresent(renderer);
    }

    Logger::Log("APPLICATION", Logger::Level::Info, "Leaving main loop.");

    FreeTextures(tex);
    Logger::Log("APPLICATION", Logger::Level::Info, "Released textures.");

    delete crafting;
    delete furnace_panel;

    TextRenderer::ClearCache();

    audio.Shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    Logger::Log("APPLICATION", Logger::Level::Info, "Aquilon shutdown complete.");
    return 0;
}