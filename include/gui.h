#ifndef AQUILON_GUI_H
#define AQUILON_GUI_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "vmath.h"
#include <functional>
#include <string>

class GUIWindow {
public:
    vec2 position;
    vec2 size;
    SDL_Color background_color;
    SDL_Color border_color;
    float border_width;
    std::string title;

    GUIWindow(float x, float y, float width, float height, 
              const std::string& title = "", TTF_Font* title_font = nullptr,
              SDL_Renderer* renderer = nullptr);
    ~GUIWindow();
    
    void set_background_color(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void set_border_color(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void set_border_width(float width);
    void set_title(const std::string& new_title);
    void set_title_font(TTF_Font* font);
    void set_content_draw_callback(const std::function<void(SDL_Renderer*, const SDL_FRect&)>& callback);
    void set_close_callback(const std::function<void()>& callback);
    bool handle_event(const SDL_Event& e);
    bool is_closed() const;
    void render(SDL_Renderer* renderer);
    SDL_FRect get_content_rect() const;

private:
    TTF_Font* title_font;
    bool dragging;
    vec2 drag_offset;
    bool closed;
    float title_bar_height;
    float close_button_size;
    bool close_button_pressed;
    SDL_FRect close_button_rect;
    SDL_Renderer* renderer;
    SDL_Texture* close_button_texture;
    std::function<void(SDL_Renderer*, const SDL_FRect&)> content_draw_callback;
    std::function<void()> close_callback;

    SDL_FRect get_close_button_rect() const;
    void draw_close_button(SDL_Renderer* renderer);
    void draw_filled_rect(SDL_Renderer* renderer, float x, float y, float w, float h, const SDL_Color& color);
    void draw_rect(SDL_Renderer* renderer, float x, float y, float w, float h, const SDL_Color& color, float thickness);
    void draw_title(SDL_Renderer* renderer);
};

class GUIEngine {
public:
    GUIEngine(SDL_Renderer* renderer);
    ~GUIEngine();
    
    GUIWindow* create_window(float x, float y, float width, float height, const std::string& title = "");
    void render_all();
    void clear_windows();
    bool handle_event(const SDL_Event& e);
    GUIWindow* get_window() const;

    // inventory
    GUIWindow* create_inv_window(float x, float y, float w, float h, const std::string& title);
    void close_inv_window();

private:
    SDL_Renderer* renderer;
    GUIWindow* main_window;
    GUIWindow* inv_window;
    TTF_Font* title_font;
};

#endif // AQUILON_GUI_H
