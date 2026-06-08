#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_filesystem.h>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <memory>
#include <ctime>
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
#include "inv/panels/drill_panel.h"
#include "version.h"

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

enum class GameState {
    Menu,
    Playing
};

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

    SDL_SetAppMetadata("Aquilon", GAME_VERSION, "com.valme.aquilon");

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

    std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> menu_font(
        TTF_OpenFont("resources/fonts/arial.ttf", 20), &TTF_CloseFont
    );
    std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> game_font(
        TTF_OpenFont("resources/fonts/arial.ttf", 14), &TTF_CloseFont
    );

    if (!menu_font || !game_font) {
        Logger::Log("UI", Logger::Level::Warn, "Fonts not found; UI text will not render properly.");
    }

    std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> title_font(
        TTF_OpenFont("resources/fonts/arial.ttf", 72), &TTF_CloseFont
    );


    World world;
    Player player(config.input);
    Camera cam;

    Inventory inv(gui_engine, tex, game_font.get(), config.input);
    CraftingSystem* crafting = new CraftingSystem(tex, game_font.get());
    FurnacePanel* furnace_panel = new FurnacePanel(tex, game_font.get());
    DrillPanel* drill_panel = new DrillPanel(tex, game_font.get());

    furnace_panel->initialize_recipes();
    initialize_items(tex);

    inv.set_crafting_system(crafting);
    inv.set_active_panel(crafting);

    player.on_object_clicked = [&](const Player::PlacedObject& obj) {
        if (obj.type == ItemType::FURNACE) {
            drill_panel->set_target(nullptr);
            inv.set_active_panel(furnace_panel);
            if (!inv.open) {
                inv.open = true;
                inv.open_window();
            }
            return;
        }

        if (obj.type == ItemType::DRILL) {
            drill_panel->set_target(const_cast<Player::PlacedObject*>(&obj));
            inv.set_active_panel(drill_panel);
            if (!inv.open) {
                inv.open = true;
                inv.open_window();
            }
            return;
        }

        if (!inv.open && obj.opens_inv) {
            inv.open = true;
            inv.open_window();
        }
    };

    inv.pick(item_stack(Item::IRON_PLATE, 32));
    inv.pick(item_stack(Item::COAL, 32));
    inv.pick(item_stack(Item::STONE, 32));
    inv.pick(item_stack(Item::CONVEYOR, 64));

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

    std::srand((unsigned int)std::time(nullptr));
    GameState state = GameState::Menu;
    float fps = 0.0f;
    bool right_hold_blocked = false;
    float menu_cam_timer = 0.0f;
    float menu_origin_x = (float)(std::rand() % 100000 - 50000);
    float menu_origin_y = (float)(std::rand() % 100000 - 50000);

    bool resource_panel_visible = false;
    std::string resource_panel_name;
    int resource_panel_yield = 0;
    TileType resource_panel_type = EMPTY;

    GUIButton play_button({ 0, 0, 200, 45 }, Localize("Play"));
    GUIButton settings_button({ 0, 0, 200, 45 }, Localize("Settings"));
    GUIButton exit_button({ 0, 0, 200, 45 }, Localize("Exit"));

    std::vector<std::string> display_backends = {"Auto", "Vulkan", "OpenGL", "DirectX 11", "DirectX 12", "Metal", "Software"};
    std::vector<std::string> backends = {"auto", "vulkan", "opengl", "direct3d11", "direct3d12", "metal", "software"};
    int initial_backend = 0;
    for (int i = 0; i < (int)backends.size(); ++i) {
        if (config.renderer_backend == backends[i]) {
            initial_backend = i;
            break;
        }
    }
    GUICombo backend_combo({ 0, 0, 200, 35 }, Localize("Backend"), display_backends, initial_backend);
    backend_combo.max_visible_items = 5;
    backend_combo.on_change = [&](int idx) {
        config.renderer_backend = (backends[idx] == "auto") ? "" : backends[idx];
    };

    GUICheckbox vsync_checkbox({ 0, 0, 200, 30 }, Localize("VSync"), config.vsync_enabled);
    vsync_checkbox.on_change = [&](bool val) {
        config.vsync_enabled = val;
        SDL_SetRenderVSync(renderer, config.vsync_enabled ? 1 : 0);
    };

    GUISlider music_slider({ 0, 0, 200, 40 }, Localize("Music"), 1.0f);
    music_slider.on_change = [&](float val) {
        audio.SetMusicVolume((int)(val * 128));
    };

    GUISlider sfx_slider({ 0, 0, 200, 40 }, Localize("SFX"), 1.0f);
    sfx_slider.on_change = [&](float val) {
        audio.SetSFXVolume((int)(val * 128));
    };

    GUIWindow* main_window = nullptr;
    auto setup_menu_window = [&]() {
        const std::string menu_title = Localize("Main Menu");
        GUIWindow* w = gui_engine.CreateWindow(SDL_FRect{ (float)win_w / 2 - 125, (float)win_h / 2 - 110, 250, 220 }, menu_title);
        w->SetChromeVisible(false);
        
        w->SetContentDrawCallback([&](SDL_Renderer* r, const SDL_FRect& content) {
            float start_y = content.y + 20.0f;
            float center_x = content.x + (content.w - 200.0f) * 0.5f;

            play_button.rect = { center_x, start_y, 200, 45 };
            settings_button.rect = { center_x, start_y + 55, 200, 45 };
            exit_button.rect = { center_x, start_y + 110, 200, 45 };

            play_button.Draw(r, menu_font.get());
            settings_button.Draw(r, menu_font.get());
            exit_button.Draw(r, menu_font.get());
        });
        return w;
    };

    bool in_settings = false;
    enum class SettingsState { Categories, Graphics, Audio, Controls };
    SettingsState settings_state = SettingsState::Categories;

    int music_vol_idx = 10;
    int sfx_vol_idx = 10;

    GUIButton graphics_cat_btn({ 0, 0, 200, 45 }, Localize("Graphics"));
    GUIButton audio_cat_btn({ 0, 0, 200, 45 }, Localize("Audio"));
    GUIButton controls_cat_btn({ 0, 0, 200, 45 }, Localize("Controls"));
    GUIButton back_to_menu_btn({ 0, 0, 200, 45 }, Localize("Back"));

    GUIButton back_to_cats_btn({ 0, 0, 200, 45 }, Localize("Back"));

    std::function<GUIWindow*()> setup_settings_categories_window;

    auto setup_graphics_window = [&]() {
        GUIWindow* w = gui_engine.CreateWindow(SDL_FRect{ (float)win_w / 2 - 125, (float)win_h / 2 - 120, 250, 240 }, Localize("Settings"));
        w->SetContentDrawCallback([&](SDL_Renderer* r, const SDL_FRect& content) {
            float start_y = content.y + 20.0f;
            float center_x = content.x + (content.w - 200.0f) * 0.5f;
            backend_combo.rect = { center_x, start_y, 200, 35 };
            vsync_checkbox.rect = { center_x, start_y + 50, 200, 30 };
            back_to_cats_btn.rect = { center_x, start_y + 130, 200, 45 };
            vsync_checkbox.Draw(r, menu_font.get());
            back_to_cats_btn.Draw(r, menu_font.get());
            backend_combo.Draw(r, menu_font.get());
        });
        return w;
    };

    auto setup_audio_window = [&]() {
        GUIWindow* w = gui_engine.CreateWindow(SDL_FRect{ (float)win_w / 2 - 125, (float)win_h / 2 - 120, 250, 240 }, Localize("Settings"));
        w->SetContentDrawCallback([&](SDL_Renderer* r, const SDL_FRect& content) {
            float start_y = content.y + 20.0f;
            float center_x = content.x + (content.w - 200.0f) * 0.5f;
            music_slider.rect = { center_x, start_y, 200, 40 };
            sfx_slider.rect = { center_x, start_y + 60, 200, 40 };
            back_to_cats_btn.rect = { center_x, start_y + 130, 200, 45 };
            music_slider.Draw(r, menu_font.get());
            sfx_slider.Draw(r, menu_font.get());
            back_to_cats_btn.Draw(r, menu_font.get());
        });
        return w;
    };

    auto setup_controls_window = [&]() {
        GUIWindow* w = gui_engine.CreateWindow(SDL_FRect{ (float)win_w / 2 - 125, (float)win_h / 2 - 110, 250, 220 }, Localize("Settings"));
        w->SetContentDrawCallback([&](SDL_Renderer* r, const SDL_FRect& content) {
            float start_y = content.y + 20.0f;
            float center_x = content.x + (content.w - 200.0f) * 0.5f;
            
            TextRenderer::DrawText(r, menu_font.get(), content.x + 20, start_y, Localize("WASD to Move"), {200, 200, 200, 255});
            TextRenderer::DrawText(r, menu_font.get(), content.x + 20, start_y + 25, Localize("E for Inventory"), {200, 200, 200, 255});
            TextRenderer::DrawText(r, menu_font.get(), content.x + 20, start_y + 50, Localize("RMB to Mine"), {200, 200, 200, 255});

            back_to_cats_btn.rect = { center_x, start_y + 110, 200, 45 };
            back_to_cats_btn.Draw(r, menu_font.get());
        });
        return w;
    };

    setup_settings_categories_window = [&]() {
        settings_state = SettingsState::Categories;
        GUIWindow* w = gui_engine.CreateWindow(SDL_FRect{ (float)win_w / 2 - 125, (float)win_h / 2 - 140, 250, 280 }, Localize("Settings"));
        w->SetChromeVisible(false);
        
        w->SetContentDrawCallback([&](SDL_Renderer* r, const SDL_FRect& content) {
            float start_y = content.y + 20.0f;
            float center_x = content.x + (content.w - 200.0f) * 0.5f;

            graphics_cat_btn.rect = { center_x, start_y, 200, 45 };
            audio_cat_btn.rect = { center_x, start_y + 55, 200, 45 };
            controls_cat_btn.rect = { center_x, start_y + 110, 200, 45 };
            back_to_menu_btn.rect = { center_x, start_y + 165, 200, 45 };

            graphics_cat_btn.Draw(r, menu_font.get());
            audio_cat_btn.Draw(r, menu_font.get());
            controls_cat_btn.Draw(r, menu_font.get());
            back_to_menu_btn.Draw(r, menu_font.get());
        });
        return w;
    };

    graphics_cat_btn.on_click = [&]() {
        settings_state = SettingsState::Graphics;
        main_window = setup_graphics_window();
    };

    audio_cat_btn.on_click = [&]() {
        settings_state = SettingsState::Audio;
        main_window = setup_audio_window();
    };

    controls_cat_btn.on_click = [&]() {
        settings_state = SettingsState::Controls;
        main_window = setup_controls_window();
    };

    back_to_menu_btn.on_click = [&]() {
        in_settings = false;
        SaveAppConfig(config_path, config);
        main_window = setup_menu_window();
    };

    back_to_cats_btn.on_click = [&]() {
        main_window = setup_settings_categories_window();
    };

    main_window = setup_menu_window();
    GUIWindow* resource_panel = gui_engine.CreateInfoWindow(SDL_FRect{0, 0, 220, 110});
    GUIWindow* crafting_queue_window = gui_engine.CreateQueueWindow(SDL_FRect{10.0f, (float)win_h - 120.0f, 260.0f, 100.0f});
    crafting_queue_window->SetVisible(false);
    crafting_queue_window->SetBackgroundColor(SDL_Color{14, 15, 18, 230});
    crafting_queue_window->SetBorderColor(SDL_Color{42, 46, 54, 255});
    crafting_queue_window->SetContentDrawCallback([&](SDL_Renderer* renderer, const SDL_FRect& content_rect) {
        if (crafting) crafting->draw_queue(renderer, game_font.get(), content_rect);
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

        TextRenderer::DrawText(renderer, game_font.get(), left, y, resource_panel_name, title_color);
        y += 18.0f;

        char line[128];
        const std::string yield_label = Localize("Yield");
        snprintf(line, sizeof(line), "%s: %d", yield_label.c_str(), resource_panel_yield);
        TextRenderer::DrawText(renderer, game_font.get(), left, y, line, muted);
        y += 18.0f;

        const std::string type_label = Localize("Type");
        const std::string resource_name = resource_panel_type == IRON_ORE ? Localize("Iron Ore") : Localize("Stone");
        snprintf(line, sizeof(line), "%s: %s", type_label.c_str(), resource_name.c_str());
        TextRenderer::DrawText(renderer, game_font.get(), left, y, line, muted);
        y += 18.0f;

        TextRenderer::DrawText(renderer, game_font.get(), left, y, Localize("Hold RMB to mine"), accent_color);
    });

    auto status_draw_callback = [&](SDL_Renderer* renderer, const SDL_FRect& content_rect) {
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
        TextRenderer::DrawText(renderer, game_font.get(), left, y, line, label);
        y += line_step;

        snprintf(line, sizeof(line), "%s: %s", renderer_label.c_str(), SDL_GetRendererName(renderer) ? SDL_GetRendererName(renderer) : "<unknown>");
        TextRenderer::DrawText(renderer, game_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "%s: %s", vsync_label.c_str(), config.vsync_enabled ? "on" : "off");
        TextRenderer::DrawText(renderer, game_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "%s: x=%.1f y=%.1f", player_label.c_str(), player.player.x, player.player.y);
        TextRenderer::DrawText(renderer, game_font.get(), left, y, line, muted);
        y += line_step;

        int debug_player_tile_x = (int)player.player.x / TILE_SIZE;
        int debug_player_tile_y = (int)player.player.y / TILE_SIZE;
        snprintf(line, sizeof(line), "%s: %d, %d", tile_label.c_str(), debug_player_tile_x, debug_player_tile_y);
        TextRenderer::DrawText(renderer, game_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "%s: x=%.1f y=%.1f zoom=%.2f", camera_label.c_str(), cam.x, cam.y, cam.zoom);
        TextRenderer::DrawText(renderer, game_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "%s: %zu", chunks_label.c_str(), world.get_chunks().size());
        TextRenderer::DrawText(renderer, game_font.get(), left, y, line, muted);
        y += line_step;

        snprintf(line, sizeof(line), "%s: %s", log_level_label.c_str(), log_level_name(config.log_level));
        TextRenderer::DrawText(renderer, game_font.get(), left, y, line, muted);
    };

    play_button.on_click = [&]() {
        state = GameState::Playing;
        const std::string game_status_title = Localize("Game Status");
        main_window = gui_engine.CreateWindow(SDL_FRect{ 10, 10, 330, 220 }, game_status_title);
        main_window->SetContentDrawCallback(status_draw_callback);
    };

    settings_button.on_click = [&]() {
        in_settings = true;
        main_window = setup_settings_categories_window();
    };

    bool running = true;
    exit_button.on_click = [&]() { running = false; };

    Uint64 last_counter = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    float delta_time = 0.0f;
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

            bool event_handled_by_gui_element = false;

            if (state == GameState::Menu && !gui_consumed) {
                float mx, my;
                SDL_GetMouseState(&mx, &my);
                if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
                    if (!in_settings) {
                        if (play_button.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                        if (!event_handled_by_gui_element && settings_button.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                        if (!event_handled_by_gui_element && exit_button.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                    } else {
                        if (settings_state == SettingsState::Categories) {
                            if (graphics_cat_btn.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && audio_cat_btn.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && controls_cat_btn.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && back_to_menu_btn.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                        } else if (settings_state == SettingsState::Graphics) {
                            if (backend_combo.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && vsync_checkbox.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && back_to_cats_btn.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                        } else if (settings_state == SettingsState::Audio) {
                            if (music_slider.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && sfx_slider.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && back_to_cats_btn.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                        } else if (settings_state == SettingsState::Controls) {
                            if (back_to_cats_btn.HandleMouseDown(mx, my)) event_handled_by_gui_element = true;
                        }
                    }
                } else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
                    if (!in_settings) {
                        if (play_button.HandleMouseUp(mx, my)) event_handled_by_gui_element = true;
                        if (!event_handled_by_gui_element && settings_button.HandleMouseUp(mx, my)) event_handled_by_gui_element = true;
                        if (!event_handled_by_gui_element && exit_button.HandleMouseUp(mx, my)) event_handled_by_gui_element = true;
                    } else {
                        if (settings_state == SettingsState::Categories) {
                            if (graphics_cat_btn.HandleMouseUp(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && audio_cat_btn.HandleMouseUp(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && controls_cat_btn.HandleMouseUp(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && back_to_menu_btn.HandleMouseUp(mx, my)) event_handled_by_gui_element = true;
                        } else if (settings_state == SettingsState::Graphics) {
                            if (backend_combo.HandleMouseUp(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && back_to_cats_btn.HandleMouseUp(mx, my)) event_handled_by_gui_element = true;
                        } else if (settings_state == SettingsState::Audio) {
                            if (music_slider.HandleMouseUp(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && sfx_slider.HandleMouseUp(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && back_to_cats_btn.HandleMouseUp(mx, my)) event_handled_by_gui_element = true;
                        } else if (settings_state == SettingsState::Controls) {
                            if (!event_handled_by_gui_element && back_to_cats_btn.HandleMouseUp(mx, my)) event_handled_by_gui_element = true;
                        }
                    }
                } else if (e.type == SDL_EVENT_MOUSE_MOTION) {
                    if (!in_settings) {
                        if (play_button.HandleMouseMove(mx, my)) event_handled_by_gui_element = true;
                        if (!event_handled_by_gui_element && settings_button.HandleMouseMove(mx, my)) event_handled_by_gui_element = true;
                        if (!event_handled_by_gui_element && exit_button.HandleMouseMove(mx, my)) event_handled_by_gui_element = true;
                    } else {
                        if (settings_state == SettingsState::Categories) {
                            if (graphics_cat_btn.HandleMouseMove(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && audio_cat_btn.HandleMouseMove(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && controls_cat_btn.HandleMouseMove(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && back_to_menu_btn.HandleMouseMove(mx, my)) event_handled_by_gui_element = true;
                        } else {
                            if (backend_combo.HandleMouseMove(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && vsync_checkbox.HandleMouseMove(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && music_slider.HandleMouseMove(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && sfx_slider.HandleMouseMove(mx, my)) event_handled_by_gui_element = true;
                            if (!event_handled_by_gui_element && back_to_cats_btn.HandleMouseMove(mx, my)) event_handled_by_gui_element = true;
                        }
                    }
                }

                if (state == GameState::Menu && in_settings && settings_state == SettingsState::Graphics) {
                    if (!event_handled_by_gui_element) {
                        backend_combo.HandleMouseWheel((float)e.wheel.y);
                    }
                }
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_RIGHT && (gui_consumed || event_handled_by_gui_element)) {
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
                if (state == GameState::Playing) {
                    player.handle_input(e);
                }
            }

            inv.handle_event(e);
            if (state == GameState::Playing) {
                player.handle_item_placement(e, cam, inv, world, !gui_consumed);
            }
        }

        if (state == GameState::Playing) {
            player.update(delta_time);

            int player_tile_x = (int)player.player.x / TILE_SIZE;
            int player_tile_y = (int)player.player.y / TILE_SIZE;

            std::size_t chunk_count_before = world.get_chunks().size();
            world.update(player_tile_x, player_tile_y, config.chunk_distance);
            player.update_placed_drills(delta_time, world, inv, tex);
            std::size_t chunk_count_after = world.get_chunks().size();
            if (chunk_count_after != chunk_count_before) {
                Logger::Log("SYSTEM", Logger::Level::Debug,
                            "Chunk cache changed: %zu -> %zu around chunk (%d, %d).",
                            chunk_count_before, chunk_count_after,
                            player_tile_x / CHUNK_SIZE, player_tile_y / CHUNK_SIZE);
            }
            cam.update(player.player, win_w * 0.5f, win_h * 0.5f);
            player.update_mining(delta_time, world, inv, tex);
        } else if (state == GameState::Menu) {
            menu_cam_timer += delta_time;
            cam.x = menu_origin_x + std::sin(menu_cam_timer * 0.4f) * 250.0f;
            cam.y = menu_origin_y + std::cos(menu_cam_timer * 0.3f) * 250.0f;
            cam.zoom = 1.0f;
            
            world.update((int)cam.x / TILE_SIZE, (int)cam.y / TILE_SIZE, config.chunk_distance);
        }

        float mouse_x = 0.0f, mouse_y = 0.0f;
        SDL_MouseButtonFlags mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);

        furnace_panel->update(delta_time);
        drill_panel->update(delta_time);
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
        resource_panel->SetVisible(state == GameState::Playing && resource_panel_visible);

        if (!(mouse_buttons & SDL_BUTTON_RMASK)) {
            player.stop_mining();
            right_hold_blocked = false;
        } else if (!right_hold_blocked && inv.open == false) {
            if (!player.is_mining() && (hovered_tile_data.type == STONE || hovered_tile_data.type == IRON_ORE)) {
                player.start_mining(hovered_tile.x, hovered_tile.y, hovered_tile_data.type);
            }
        }

        if (state == GameState::Playing || state == GameState::Menu) {
            SDL_SetRenderDrawColor(renderer, 230, 245, 255, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 24, 26, 31, 255);
        }
        SDL_RenderClear(renderer);

        if (state == GameState::Playing || state == GameState::Menu) {
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

                        bool is_ice = t.type == ICE;

                        SDL_FRect dst = cam.world_to_screen_rect(
                            world_x * TILE_SIZE,
                            world_y * TILE_SIZE,
                            (float)TILE_SIZE * (!is_ice ? 1 : 1.3),
                            (float)TILE_SIZE * (!is_ice ? 1 : 1.3)
                        );

                        float angle = (float)(((world_x * 928371 + world_y * 12347) % 360 + 360) % 360);
                        SDL_RenderTextureRotated(renderer, current, nullptr, &dst, angle, nullptr, SDL_FLIP_NONE);
                    }
                }
            }
        }

        if (state == GameState::Playing) {
            player.render_placed_objects(renderer, cam, visible_min_tile_x, visible_min_tile_y, visible_max_tile_x, visible_max_tile_y);
            player.render(renderer, cam);
            player.draw_item_placement_preview(renderer, cam, inv, world, mouse_x, mouse_y);
        }

        if (state == GameState::Menu) {
            const std::string game_title = "Aquilon";
            int title_w, title_h;
            TTF_Font* draw_font = title_font ? title_font.get() : menu_font.get();
            TTF_GetStringSize(draw_font, game_title.c_str(), 0, &title_w, &title_h);
            float title_x = (win_w - title_w) / 2.0f;
            float title_y = 50.0f;
            TextRenderer::DrawTextShadow(renderer, draw_font, title_x, title_y, game_title, {255, 255, 255, 255}, {0, 0, 0, 200});

            char version_text[128];
            snprintf(version_text, sizeof(version_text), "v%s", GAME_VERSION);
            int version_w, version_h;
            TTF_GetStringSize(menu_font.get(), version_text, 0, &version_w, &version_h);
            float version_x = 10.0f;
            float version_y = win_h - version_h - 10.0f;
            TextRenderer::DrawTextShadow(renderer, menu_font.get(), version_x, version_y, version_text, {220, 220, 220, 255}, {0, 0, 0, 255});

            char build_number_text[128];
            snprintf(build_number_text, sizeof(build_number_text), "Build: %s", BUILD_NUMBER);
            int build_number_w, build_number_h;
            TTF_GetStringSize(menu_font.get(), build_number_text, 0, &build_number_w, &build_number_h);
            TextRenderer::DrawTextShadow(renderer, menu_font.get(), (float)win_w - build_number_w - 10.0f, win_h - build_number_h - 10.0f - (float)TTF_GetFontHeight(menu_font.get()), build_number_text, {220, 220, 220, 255}, {0, 0, 0, 255});

            char build_date_text[128];
            snprintf(build_date_text, sizeof(build_date_text), "%s", BUILD_DATE);
            int build_date_w, build_date_h;
            TTF_GetStringSize(menu_font.get(), build_date_text, 0, &build_date_w, &build_date_h);
            TextRenderer::DrawTextShadow(renderer, menu_font.get(), (float)win_w - build_date_w - 10.0f, win_h - build_date_h - 10.0f, build_date_text, {220, 220, 220, 255}, {0, 0, 0, 255});
        }


        if (crafting_queue_window) {
            crafting_queue_window->size.x = 10.0f;
            crafting_queue_window->size.y = (float)win_h - crafting_queue_window->size.h - 10.0f;
            crafting_queue_window->SetVisible(crafting && crafting->has_queue());
        }

        gui_engine.RenderAll();

        if (state == GameState::Playing) {
            inv.draw(renderer, game_font.get());
            player.draw_mining_progress_bar(renderer, win_w, win_h);
        }

        SDL_RenderPresent(renderer);
    }

    Logger::Log("APPLICATION", Logger::Level::Info, "Leaving main loop.");

    SaveAppConfig(config_path, config);
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