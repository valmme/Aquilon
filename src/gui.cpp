#include "gui.h"
#include "textrenderer.h"
#include "logger.h"
#include <cmath>

static constexpr SDL_Color GUI_BG_COLOR = {14, 15, 18, 248};
static constexpr SDL_Color GUI_BORDER_COLOR = {42, 46, 54, 255};
static constexpr SDL_Color GUI_TITLE_BG_COLOR = {18, 19, 23, 255};
static constexpr SDL_Color GUI_TITLE_TEXT_COLOR = {236, 238, 242, 255};
static constexpr SDL_Color GUI_TITLE_SHADOW_COLOR = {6, 7, 9, 210};
static constexpr SDL_Color GUI_CLOSE_ICON_COLOR = {155, 160, 168, 255};
static constexpr SDL_Color GUI_CLOSE_ICON_PRESSED_COLOR = {242, 244, 246, 255};

GUIWindow::GUIWindow(SDL_FRect size, const std::string& title, TTF_Font* title_font, SDL_Renderer* renderer)
    : size(size), title(title),
      background_color(GUI_BG_COLOR),
      border_color(GUI_BORDER_COLOR),
      border_width(1.0f),
      title_font(title_font),
      renderer(renderer),
      dragging(false),
      drag_offset({0.0f, 0.0f}),
      closed(false),
      visible(true),
      chrome_visible(true),
      title_bar_height(23.0f),
      close_button_size(16.0f),
      close_button_pressed(false),
      close_button_rect({0, 0, 0, 0}),
      close_button_texture(nullptr),
      close_callback(nullptr) {}

      
GUIWindow::~GUIWindow() {
    if (close_button_texture) {
        SDL_DestroyTexture(close_button_texture);
        close_button_texture = nullptr;
    }
}

void GUIWindow::SetBackgroundColor(SDL_Color color) {
    background_color = color;
}

void GUIWindow::SetBorderColor(SDL_Color color) {
    border_color = color;
}

void GUIWindow::SetBorderWidth(float width) {
    border_width = width;
}

void GUIWindow::SetTitle(const std::string& new_title) {
    title = new_title;
}

void GUIWindow::SetTitleFont(TTF_Font* font) {
    title_font = font;
}

void GUIWindow::SetChromeVisible(bool visible) {
    chrome_visible = visible;
}

void GUIWindow::SetVisible(bool is_visible) {
    visible = is_visible;
}

void GUIWindow::SetContentDrawCallback(const std::function<void(SDL_Renderer*, const SDL_FRect&)>& callback) {
    content_draw_callback = callback;
}

bool GUIWindow::HandleEvent(const SDL_Event& e) {
    if (closed || !visible) {
        return false;
    }

    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        const SDL_MouseButtonEvent& button = e.button;
        if (button.button == SDL_BUTTON_LEFT) {
            if (!chrome_visible) {
                return false;
            }

            float mouse_x = (float)button.x;
            float mouse_y = (float)button.y;
            SDL_FRect title_rect = {size.x + border_width, size.y + border_width, size.w - border_width * 2.0f, title_bar_height};
            SDL_FRect close_rect = GetCloseButtonRect();
            if (mouse_x >= close_rect.x && mouse_x <= close_rect.x + close_rect.w &&
                mouse_y >= close_rect.y && mouse_y <= close_rect.y + close_rect.h) {
                close_button_pressed = true;
                return true;
            }

            if (mouse_x >= title_rect.x && mouse_x <= title_rect.x + title_rect.w &&
                mouse_y >= title_rect.y && mouse_y <= title_rect.y + title_rect.h) {
                dragging = true;
                drag_offset = {mouse_x - size.x, mouse_y - size.y};
                return true;
            }
        }
    } 
    
    else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        if (!chrome_visible) {
            return false;
        }

        const SDL_MouseButtonEvent& button = e.button;
        float mouse_x = (float)button.x;
        float mouse_y = (float)button.y;
        SDL_FRect close_rect = GetCloseButtonRect();
        if (button.button == SDL_BUTTON_LEFT && close_button_pressed) {
            if (mouse_x >= close_rect.x && mouse_x <= close_rect.x + close_rect.w &&
                mouse_y >= close_rect.y && mouse_y <= close_rect.y + close_rect.h) {
                closed = true;
                if (close_callback) {
                    close_callback();
                }
            }

            close_button_pressed = false;
            return true;
        }

        if (button.button == SDL_BUTTON_LEFT && dragging) {
            dragging = false;
            return true;
        }
    } 
    
    else if (e.type == SDL_EVENT_MOUSE_MOTION) {
        if (!chrome_visible) {
            return false;
        }

        const SDL_MouseMotionEvent& motion = e.motion;
        if (close_button_pressed) {
            float mouse_x = (float)motion.x;
            float mouse_y = (float)motion.y;
            SDL_FRect close_rect = GetCloseButtonRect();

            if (mouse_x < close_rect.x || mouse_x > close_rect.x + close_rect.w ||
                mouse_y < close_rect.y || mouse_y > close_rect.y + close_rect.h) {
                close_button_pressed = false;
            }
            return true;
        }

        if (dragging) {
            size.x = motion.x - drag_offset.x;
            size.y = motion.y - drag_offset.y;
            return true;
        }
    }

    return false;
}

void GUIWindow::DrawFilledRect(SDL_Renderer* renderer, SDL_FRect rect, const SDL_Color& color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void GUIWindow::DrawRect(SDL_Renderer* renderer, SDL_FRect rect, const SDL_Color& color, float thickness) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    SDL_FRect top = {rect.x, rect.y, rect.w, thickness};
    SDL_RenderFillRect(renderer, &top);
    
    SDL_FRect bottom = {rect.x, rect.y + rect.h - thickness, rect.w, thickness};
    SDL_RenderFillRect(renderer, &bottom);
    
    SDL_FRect left = {rect.x, rect.y, thickness, rect.h};
    SDL_RenderFillRect(renderer, &left);
    
    SDL_FRect right = {rect.x + rect.w - thickness, rect.y, thickness, rect.h};
    SDL_RenderFillRect(renderer, &right);
}


void GUIWindow::DrawTitle(SDL_Renderer* renderer) {
    if (!chrome_visible || title.empty()) return;

    float padding = 6.0f;
    float available_width = size.w - border_width * 2.0f;

    SDL_SetRenderDrawColor(renderer, GUI_TITLE_BG_COLOR.r, GUI_TITLE_BG_COLOR.g, GUI_TITLE_BG_COLOR.b, GUI_TITLE_BG_COLOR.a);
    SDL_FRect title_bg = {size.x + border_width, size.y + border_width, available_width, title_bar_height};
    SDL_RenderFillRect(renderer, &title_bg);

    SDL_SetRenderDrawColor(renderer, 32, 35, 41, 255);
    SDL_FRect title_line = {size.x + border_width, size.y + border_width + title_bar_height - 1.0f, available_width, 1.0f};
    SDL_RenderFillRect(renderer, &title_line);

    DrawCloseButton(renderer);

    if (!title_font) return;

    float text_y = std::floor(size.y + border_width + (title_bar_height - (float)TTF_GetFontHeight(title_font)) * 0.5f);
    float text_x = std::floor(size.x + border_width + padding);

    SDL_FRect title_dst = {0, 0, 0, 0};
    if (!TextRenderer::DrawTextShadow(renderer,
                                      title_font,
                                      text_x,
                                      text_y,
                                      title,
                                      GUI_TITLE_TEXT_COLOR,
                                      GUI_TITLE_SHADOW_COLOR,
                                      &title_dst)) {
        return;
    }
}

void GUIWindow::Render(SDL_Renderer* renderer) {
    if (closed || !visible) {
        return;
    }

    float inner_x = size.x + border_width;
    float inner_y = size.y + border_width;
    float inner_w = size.w - border_width * 2.0f;
    float inner_h = size.h - border_width * 2.0f;

    if (inner_w > 0.0f && inner_h > 0.0f) {
        DrawFilledRect(renderer, {inner_x, inner_y, inner_w, inner_h}, background_color);
    }

    if (content_draw_callback) {
        SDL_FRect content_rect = GetContentRect();
        content_draw_callback(renderer, content_rect);
    }

    DrawRect(renderer, size, border_color, border_width);
    if (chrome_visible) {
        DrawTitle(renderer);
    }
}

SDL_FRect GUIWindow::GetContentRect() const {
    float content_x = size.x + border_width;
    float content_y = size.y + border_width + (chrome_visible ? title_bar_height : 0.0f);
    float content_w = size.w - border_width * 2.0f;
    float content_h = size.h - border_width * 2.0f - (chrome_visible ? title_bar_height : 0.0f);
    return {content_x, content_y, content_w, content_h};
}

bool GUIWindow::IsClosed() const {
    return closed;
}

void GUIWindow::SetCloseCallback(const std::function<void()>& callback) {
    close_callback = callback;
}

SDL_FRect GUIWindow::GetCloseButtonRect() const {
    return {
        size.x + size.w - border_width - close_button_size - 6.0f,
        size.y + border_width + (title_bar_height - close_button_size) * 0.5f,
        close_button_size,
        close_button_size
    };
}

void GUIWindow::DrawCloseButton(SDL_Renderer* renderer) {
    close_button_rect = GetCloseButtonRect();
    
    SDL_Color icon_color = close_button_pressed ? GUI_CLOSE_ICON_PRESSED_COLOR : GUI_CLOSE_ICON_COLOR;
    SDL_SetRenderDrawColor(renderer, icon_color.r, icon_color.g, icon_color.b, icon_color.a);
    float inset = 3.0f;
    SDL_RenderLine(renderer,
                   (int)(close_button_rect.x + inset),
                   (int)(close_button_rect.y + inset),
                   (int)(close_button_rect.x + close_button_rect.w - inset),
                   (int)(close_button_rect.y + close_button_rect.h - inset));
    SDL_RenderLine(renderer,
                   (int)(close_button_rect.x + close_button_rect.w - inset),
                   (int)(close_button_rect.y + inset),
                   (int)(close_button_rect.x + inset),
                   (int)(close_button_rect.y + close_button_rect.h - inset));
}

static TTF_Font* LoadDefaultFont() {
    const char* font_paths[] = {
        "resources/fonts/arial.ttf",
        nullptr
    };

    for (const char** path = font_paths; *path; ++path) {
        TTF_Font* font = TTF_OpenFont(*path, 16);
        if (font) {
            TTF_SetFontHinting(font, TTF_HINTING_LIGHT);
            return font;
        }
    }

    return nullptr;
}

GUIEngine::GUIEngine(SDL_Renderer* renderer)
    : renderer(renderer), main_window(nullptr), inv_window(nullptr), info_window(nullptr), title_font(nullptr) {
    if (!TTF_Init()) {
        Logger::Log("UI", Logger::Level::Error,
                    "Failed to initialize SDL_ttf: %s", SDL_GetError());
    } else {
        title_font = LoadDefaultFont();
        if (!title_font) {
            Logger::Log("UI", Logger::Level::Warn,
                        "SDL_ttf font not found; title text will not render.");
        }
    }
}

GUIEngine::~GUIEngine() {
    if (main_window) {
        delete main_window;
        main_window = nullptr;
    }

    if (inv_window) {
        delete inv_window;
        inv_window = nullptr;
    }

    if (info_window) {
        delete info_window;
        info_window = nullptr;
    }

    if (title_font) TTF_CloseFont(title_font);
    TTF_Quit();
}

GUIWindow* GUIEngine::CreateWindow(SDL_FRect size, const std::string& title) {
    if (main_window) {
        delete main_window;
    }
    main_window = new GUIWindow(size, title, title_font, renderer);
    return main_window;
}

GUIWindow* GUIEngine::CreateInfoWindow(SDL_FRect size) {
    if (info_window) {
        delete info_window;
    }

    info_window = new GUIWindow(size, "", title_font, renderer);
    info_window->SetChromeVisible(false);
    return info_window;
}

bool GUIEngine::HandleEvent(const SDL_Event& e) {
    bool consumed = false;

    if (inv_window) {
        consumed = inv_window->HandleEvent(e);
        if (inv_window->IsClosed()) {
            delete inv_window;
            inv_window = nullptr;
            return true;
        }

        if (consumed) return true;
    }

    if (main_window) {
        consumed = main_window->HandleEvent(e);
        if (main_window->IsClosed()) {
            delete main_window;
            main_window = nullptr;
            return true;
        }

        return consumed;
    }

    return consumed;
}

bool GUIEngine::IsMouseOverAnyWindow(const vec2& mouse_pos) const {
    if (main_window && main_window->GetContentRect().x <= mouse_pos.x && mouse_pos.x <= main_window->GetContentRect().x + main_window->GetContentRect().w &&
        main_window->GetContentRect().y <= mouse_pos.y && mouse_pos.y <= main_window->GetContentRect().y + main_window->GetContentRect().h) {
        return true;
    }

    if (inv_window && inv_window->GetContentRect().x <= mouse_pos.x && mouse_pos.x <= inv_window->GetContentRect().x + inv_window->GetContentRect().w &&
        inv_window->GetContentRect().y <= mouse_pos.y && mouse_pos.y <= inv_window->GetContentRect().y + inv_window->GetContentRect().h) {
        return true;
    }

    if (info_window && info_window->GetContentRect().x <= mouse_pos.x && mouse_pos.x <= info_window->GetContentRect().x + info_window->GetContentRect().w &&
        info_window->GetContentRect().y <= mouse_pos.y && mouse_pos.y <= info_window->GetContentRect().y + info_window->GetContentRect().h) {
        return true;
    }

    return false;
}

void GUIEngine::RenderAll() {
    if (main_window) main_window->Render(renderer);
    if (inv_window) inv_window->Render(renderer);
    if (info_window) info_window->Render(renderer);
}

void GUIEngine::ClearWindows() {
    if (main_window) {
        delete main_window;
        main_window = nullptr;
    }
}

GUIWindow* GUIEngine::GetWindow() const {
    return main_window;
}

GUIWindow* GUIEngine::CreateInvWindow(SDL_FRect size, const std::string& title) {
    if (inv_window) delete inv_window;

    inv_window = new GUIWindow(size, title, title_font, renderer);
    return inv_window;
}

void GUIEngine::CloseInvWindow() {
    if (inv_window) {
        delete inv_window;
        inv_window = nullptr;
    }
}

void GUIEngine::CloseInfoWindow() {
    if (info_window) {
        delete info_window;
        info_window = nullptr;
    }
}
