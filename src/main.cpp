#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cstdio>
#include "player.h"
#include "textures.h"
#include "gui.h"
#include "gen/world.h"

const int TILE_SIZE = 32;

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;

    SDL_Window* window = SDL_CreateWindow("Aquilon", 800, 600, 0);
    if (!window) {
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderVSync(renderer, 1);

    Textures tex = load_textures(renderer);
    GUIEngine gui_engine(renderer);
    
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

    free_textures(tex);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}