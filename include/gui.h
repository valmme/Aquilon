#ifndef AQUILON_GUI_H
#define AQUILON_GUI_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "vmath.h"
#include <functional>
#include <string>

class GUIButton {
public:
    SDL_FRect rect;
    std::string label;
    bool hovered;
    bool pressed;
    float font_scale;
    Uint64 last_ticks;
    std::function<void()> on_click;
    
    GUIButton(SDL_FRect rect, const std::string& label);
    
    bool IsMouseOver(float mouse_x, float mouse_y) const;
    bool HandleMouseDown(float mouse_x, float mouse_y);
    bool HandleMouseUp(float mouse_x, float mouse_y);
    bool HandleMouseMove(float mouse_x, float mouse_y);
    void Draw(SDL_Renderer* renderer, TTF_Font* font);
    
private:
    SDL_Color GetBackgroundColor() const;
    SDL_Color GetTextColor() const;
};

class GUICheckbox {
public:
    SDL_FRect rect;
    std::string label;
    bool checked;
    bool hovered;
    float font_scale;
    Uint64 last_ticks;
    std::function<void(bool)> on_change;

    GUICheckbox(SDL_FRect rect, const std::string& label, bool initial = false);
    bool HandleMouseDown(float mx, float my);
    bool HandleMouseMove(float mx, float my);
    void Draw(SDL_Renderer* renderer, TTF_Font* font);
};

class GUISlider {
public:
    SDL_FRect rect;
    std::string label;
    float value;
    bool hovered;
    bool dragging;
    float font_scale;
    Uint64 last_ticks;
    std::function<void(float)> on_change;

    GUISlider(SDL_FRect rect, const std::string& label, float initial = 0.5f);
    bool HandleMouseDown(float mx, float my);
    bool HandleMouseUp(float mx, float my);
    bool HandleMouseMove(float mx, float my);
    void Draw(SDL_Renderer* renderer, TTF_Font* font);

private:
    void UpdateValueFromMouse(float mx);
};

class GUICombo {
public:
    SDL_FRect rect;
    std::string label;
    std::vector<std::string> options;
    int selected_index;
    bool expanded;
    bool hovered;
    int scroll_index;
    int max_visible_items;
    bool dragging_scroll;
    float font_scale;
    Uint64 last_ticks;
    std::function<void(int)> on_change;

    GUICombo(SDL_FRect rect, const std::string& label, const std::vector<std::string>& options, int initial = 0);
    bool HandleMouseDown(float mx, float my);
    bool HandleMouseUp(float mx, float my);
    void HandleMouseWheel(float y);
    bool HandleMouseMove(float mx, float my);
    void Draw(SDL_Renderer* renderer, TTF_Font* font);

private:
    SDL_FRect GetOptionRect(int index) const;
    SDL_FRect GetScrollbarRect() const;
    SDL_FRect scroll_handle_rect;
    float drag_offset_y;
};

class GUIWindow {
public:
    SDL_FRect size;
    SDL_Color background_color;
    SDL_Color border_color;
    float border_width;
    std::string title;

    GUIWindow(SDL_FRect size, 
              const std::string& title = "", TTF_Font* title_font = nullptr,
              SDL_Renderer* renderer = nullptr);
    ~GUIWindow();
    
    void SetBackgroundColor(SDL_Color color);
    void SetBorderColor(SDL_Color color);
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
    void DrawFilledRect(SDL_Renderer* renderer, SDL_FRect rect, const SDL_Color& color);
    void DrawRect(SDL_Renderer* renderer, SDL_FRect rect, const SDL_Color& color, float thickness);
    void DrawTitle(SDL_Renderer* renderer);
};

class GUIEngine {
public:
    GUIEngine(SDL_Renderer* renderer);
    ~GUIEngine();
    
    GUIWindow* CreateWindow(SDL_FRect size, const std::string& title = "");
    GUIWindow* CreateInfoWindow(SDL_FRect size);
    GUIWindow* CreateQueueWindow(SDL_FRect size);
    void RenderAll();
    void ClearWindows();
    bool HandleEvent(const SDL_Event& e);
    GUIWindow* GetWindow() const;

    bool IsMouseOverAnyWindow(const vec2& mouse_pos) const;

    // inventory
    GUIWindow* CreateInvWindow(SDL_FRect size, const std::string& title);
    void CloseInvWindow();
    void CloseInfoWindow();
    void CloseQueueWindow();

private:
    SDL_Renderer* renderer;
    GUIWindow* main_window;
    GUIWindow* inv_window;
    GUIWindow* info_window;
    GUIWindow* queue_window;
    TTF_Font* title_font;
};

#endif // AQUILON_GUI_H
