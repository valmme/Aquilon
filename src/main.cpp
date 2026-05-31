#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cstdio>
#include <cstring>
#include "player.h"
#include "textures.h"
#include "gui.h"
#include "gen/world.h"
#include "logger.h"

const int TILE_SIZE = 32;

static const char* ParseRendererBackend(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (!arg) {
            continue;
        }

        if (std::strcmp(arg, "-vulkan") == 0) return "vulkan";
        if (std::strcmp(arg, "-opengl") == 0) return "opengl";
        if (std::strcmp(arg, "-d3d11") == 0) return "direct3d11";
        if (std::strcmp(arg, "-d3d12") == 0) return "direct3d12";
        if (std::strcmp(arg, "-software") == 0) return "software";
        if (std::strcmp(arg, "-gpu") == 0) return "gpu";
        if (std::strcmp(arg, "-metal") == 0) return "metal";

        if (std::strncmp(arg, "-renderer=", 10) == 0) return arg + 10;
        if (std::strncmp(arg, "--renderer=", 11) == 0) return arg + 11;

        if (std::strcmp(arg, "-renderer") == 0 || std::strcmp(arg, "--renderer") == 0) {
            if (i + 1 < argc && argv[i + 1] != nullptr) {
                return argv[i + 1];
            }
            return "";
        }
    }

    return nullptr;
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

int main(int argc, char* argv[]) {
    Logger::SetLogFile("aquilon.log");
    Logger::SetConsoleOutput(true);
    Logger::Log("APPLICATION", Logger::Level::Info, "Starting Aquilon...");

    Logger::Log("SYSTEM", Logger::Level::Info, "Initializing SDL video subsystem.");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        Logger::Log("SYSTEM", Logger::Level::Fatal, "Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Aquilon", 800, 600, 0);
    if (!window) {
        Logger::Log("SYSTEM", Logger::Level::Fatal, "Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    Logger::Log("SYSTEM", Logger::Level::Info, "Created window: 800x600.");

    const char* requested_backend = ParseRendererBackend(argc, argv);
    if (requested_backend && requested_backend[0] == '\0') {
        Logger::Log("SYSTEM", Logger::Level::Error,
                    "Renderer backend flag given without a value. Use -renderer <name> or -vulkan.");
        LogAvailableRenderers();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (requested_backend) {
        Logger::Log("SYSTEM", Logger::Level::Info, "Requested renderer backend: %s", requested_backend);
    } else {
        Logger::Log("SYSTEM", Logger::Level::Info, "No renderer backend requested; SDL will choose the default.");
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, requested_backend);
    if (!renderer) {
        Logger::Log("SYSTEM", Logger::Level::Fatal, "Failed to create renderer: %s", SDL_GetError());
        LogAvailableRenderers();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    Logger::Log("SYSTEM", Logger::Level::Info, "Created SDL renderer backend: %s",
                SDL_GetRendererName(renderer) ? SDL_GetRendererName(renderer) : "<unknown>");

    SDL_SetRenderVSync(renderer, 1);
    Logger::Log("SYSTEM", Logger::Level::Info, "Enabled VSync.");

    Textures tex = load_textures(renderer);
    Logger::Log("APPLICATION", Logger::Level::Info, "Loaded textures.");

    GUIEngine gui_engine(renderer);
    Logger::Log("UI", Logger::Level::Info, "Initialized GUI engine.");
    
    GUIWindow* main_window = gui_engine.create_window(10, 10, 300, 200, "Game Status");
    main_window->set_content_draw_callback([](SDL_Renderer* renderer, const SDL_FRect& content_rect) {
        SDL_Color fill_color = {40, 50, 60, 255};
        SDL_SetRenderDrawColor(renderer, fill_color.r, fill_color.g, fill_color.b, fill_color.a);
        SDL_RenderFillRect(renderer, &content_rect);

        SDL_FRect inner_rect = {content_rect.x + 8.0f, content_rect.y + 8.0f,
                                content_rect.w - 16.0f, content_rect.h - 16.0f};
        SDL_Color accent = {100, 160, 200, 255};
        SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, accent.a);
        SDL_RenderFillRect(renderer, &inner_rect);
    });

    const SDL_FRect gui_content_items[] = {
        {10.0f, 10.0f, 120.0f, 18.0f},
        {10.0f, 36.0f, 120.0f, 18.0f},
        {10.0f, 62.0f, 120.0f, 18.0f}
    };
    const int gui_content_item_count = sizeof(gui_content_items) / sizeof(gui_content_items[0]);

    World world;
    Player player;
    Camera cam;
    Logger::Log("GAMEPLAY", Logger::Level::Info, "Initialized world, player, and camera.");

    int win_w = 800, win_h = 600;
    SDL_GetWindowSize(window, &win_w, &win_h);

    float player_cx_init = player.player.x + player.player.w * 0.5f;
    float player_cy_init = player.player.y + player.player.h * 0.5f;
    cam.x = player_cx_init - (win_w * 0.5f) / cam.zoom;
    cam.y = player_cy_init - (win_h * 0.5f) / cam.zoom;

    Uint64 last_counter = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    float delta_time = 0.0f;
    float fps = 0.0f;

    bool running = true;
    SDL_Event e;
    Logger::Log("APPLICATION", Logger::Level::Info, "Entering main loop.");

    while (running) {
        Uint64 current_counter = SDL_GetPerformanceCounter();
        delta_time = (float)(current_counter - last_counter) / frequency;
        last_counter = current_counter;
        fps = 1.0f / delta_time;

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
        }

        player.update(delta_time);

        int player_tile_x = (int)player.player.x / TILE_SIZE;
        int player_tile_y = (int)player.player.y / TILE_SIZE;

        world.update(player_tile_x, player_tile_y);
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

        GUIWindow* active_window = gui_engine.get_window();
        if (active_window) {
            SDL_FRect content_rect = active_window->get_content_rect();
            SDL_Color item_color = {200, 200, 100, 255};
            SDL_SetRenderDrawColor(renderer, item_color.r, item_color.g, item_color.b, item_color.a);
            for (int i = 0; i < gui_content_item_count; ++i) {
                SDL_FRect rect = {
                    content_rect.x + gui_content_items[i].x,
                    content_rect.y + gui_content_items[i].y,
                    gui_content_items[i].w,
                    gui_content_items[i].h
                };
                SDL_RenderFillRect(renderer, &rect);
            }
        }

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
