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

    GUIWindow(Vec2 position, Vec2 size, 
              const std::string& title = "", TTF_Font* title_font = nullptr,
              SDL_Renderer* renderer = nullptr);
    ~GUIWindow();
    
    void SetBackgroundColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void SetBorderColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void SetBorderWidth(float width);
    void SetTitle(const std::string& new_title);
    void SetTitleFont(TTF_Font* font);
    void SetChromeVisible(bool visible);
    void SetVisible(bool visible);
    void SetContentDrawCallback(const std::function<void(SDL_Renderer*, const SDL_FRect&)>& callback);
    void SetCloseCallback(const std::function<void()>& callback);
    bool HandleEvent(const SDL_Event& e);
    bool IsClosed() const;
    void Render(SDL_Renderer* renderer);
    SDL_FRect GetContentRect() const;

private:
    TTF_Font* title_font;
    bool dragging;
    vec2 drag_offset;
    bool closed;
    bool visible;
    bool chrome_visible;
    float title_bar_height;
    float close_button_size;
    bool close_button_pressed;
    SDL_FRect close_button_rect;
    SDL_Renderer* renderer;
    SDL_Texture* close_button_texture;
    std::function<void(SDL_Renderer*, const SDL_FRect&)> content_draw_callback;
    std::function<void()> close_callback;

    SDL_FRect GetCloseButtonRect() const;
    void DrawCloseButton(SDL_Renderer* renderer);
    void DrawFilledRect(SDL_Renderer* renderer, Vec2 position, Vec2 size, const SDL_Color& color);
    void DrawRect(SDL_Renderer* renderer, Vec2 position, Vec2 size, const SDL_Color& color, float thickness);
    void DrawTitle(SDL_Renderer* renderer);
};

class GUIEngine {
public:
    GUIEngine(SDL_Renderer* renderer);
    ~GUIEngine();
    
    GUIWindow* CreateWindow(Vec2 position, Vec2 size, const std::string& title = "");
    GUIWindow* CreateInfoWindow(Vec2 position, Vec2 size);
    void RenderAll();
    void ClearWindows();
    bool HandleEvent(const SDL_Event& e);
    GUIWindow* GetWindow() const;

    // inventory
    GUIWindow* CreateInvWindow(Vec2 position, Vec2 size, const std::string& title);
    void CloseInvWindow();
    void CloseInfoWindow();

private:
    SDL_Renderer* renderer;
    GUIWindow* main_window;
    GUIWindow* inv_window;
    GUIWindow* info_window;
    TTF_Font* title_font;
};

#endif // AQUILON_GUI_H
