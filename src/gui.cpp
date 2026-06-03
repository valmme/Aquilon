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

GUIWindow::GUIWindow(float x, float y, float width, float height, const std::string& title, TTF_Font* title_font, SDL_Renderer* renderer)
    : position({x, y}), size({width, height}), title(title),
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

void GUIWindow::set_background_color(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    background_color = SDL_Color{r, g, b, a};
}

void GUIWindow::set_border_color(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    border_color = SDL_Color{r, g, b, a};
}

void GUIWindow::set_border_width(float width) {
    border_width = width;
}

void GUIWindow::set_title(const std::string& new_title) {
    title = new_title;
}

void GUIWindow::set_title_font(TTF_Font* font) {
    title_font = font;
}

void GUIWindow::set_chrome_visible(bool visible) {
    chrome_visible = visible;
}

void GUIWindow::set_visible(bool is_visible) {
    visible = is_visible;
}

void GUIWindow::set_content_draw_callback(const std::function<void(SDL_Renderer*, const SDL_FRect&)>& callback) {
    content_draw_callback = callback;
}

bool GUIWindow::handle_event(const SDL_Event& e) {
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
            SDL_FRect title_rect = {position.x + border_width, position.y + border_width, size.x - border_width * 2.0f, title_bar_height};
            SDL_FRect close_rect = get_close_button_rect();
            if (mouse_x >= close_rect.x && mouse_x <= close_rect.x + close_rect.w &&
                mouse_y >= close_rect.y && mouse_y <= close_rect.y + close_rect.h) {
                close_button_pressed = true;
                return true;
            }

            if (mouse_x >= title_rect.x && mouse_x <= title_rect.x + title_rect.w &&
                mouse_y >= title_rect.y && mouse_y <= title_rect.y + title_rect.h) {
                dragging = true;
                drag_offset = {mouse_x - position.x, mouse_y - position.y};
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
        SDL_FRect close_rect = get_close_button_rect();
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
            SDL_FRect close_rect = get_close_button_rect();

            if (mouse_x < close_rect.x || mouse_x > close_rect.x + close_rect.w ||
                mouse_y < close_rect.y || mouse_y > close_rect.y + close_rect.h) {
                close_button_pressed = false;
            }
            return true;
        }

        if (dragging) {
            position.x = motion.x - drag_offset.x;
            position.y = motion.y - drag_offset.y;
            return true;
        }
    }

    return false;
}

void GUIWindow::draw_filled_rect(SDL_Renderer* renderer, float x, float y, float w, float h, const SDL_Color& color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_FRect rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

void GUIWindow::draw_rect(SDL_Renderer* renderer, float x, float y, float w, float h, const SDL_Color& color, float thickness) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    SDL_FRect top = {x, y, w, thickness};
    SDL_RenderFillRect(renderer, &top);
    
    SDL_FRect bottom = {x, y + h - thickness, w, thickness};
    SDL_RenderFillRect(renderer, &bottom);
    
    SDL_FRect left = {x, y, thickness, h};
    SDL_RenderFillRect(renderer, &left);
    
    SDL_FRect right = {x + w - thickness, y, thickness, h};
    SDL_RenderFillRect(renderer, &right);
}


void GUIWindow::draw_title(SDL_Renderer* renderer) {
    if (!chrome_visible || title.empty()) return;

    float padding = 6.0f;
    float available_width = size.x - border_width * 2.0f;

    SDL_SetRenderDrawColor(renderer, GUI_TITLE_BG_COLOR.r, GUI_TITLE_BG_COLOR.g, GUI_TITLE_BG_COLOR.b, GUI_TITLE_BG_COLOR.a);
    SDL_FRect title_bg = {position.x + border_width, position.y + border_width, available_width, title_bar_height};
    SDL_RenderFillRect(renderer, &title_bg);

    SDL_SetRenderDrawColor(renderer, 32, 35, 41, 255);
    SDL_FRect title_line = {position.x + border_width, position.y + border_width + title_bar_height - 1.0f, available_width, 1.0f};
    SDL_RenderFillRect(renderer, &title_line);

    draw_close_button(renderer);

    if (!title_font) return;

    float text_y = std::floor(position.y + border_width + (title_bar_height - (float)TTF_GetFontHeight(title_font)) * 0.5f);
    float text_x = std::floor(position.x + border_width + padding);

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

void GUIWindow::render(SDL_Renderer* renderer) {
    if (closed || !visible) {
        return;
    }

    float inner_x = position.x + border_width;
    float inner_y = position.y + border_width;
    float inner_w = size.x - border_width * 2.0f;
    float inner_h = size.y - border_width * 2.0f;

    if (inner_w > 0.0f && inner_h > 0.0f) {
        draw_filled_rect(renderer, inner_x, inner_y, inner_w, inner_h, background_color);
    }

    if (content_draw_callback) {
        SDL_FRect content_rect = get_content_rect();
        content_draw_callback(renderer, content_rect);
    }

    draw_rect(renderer, position.x, position.y, size.x, size.y, border_color, border_width);
    if (chrome_visible) {
        draw_title(renderer);
    }
}

SDL_FRect GUIWindow::get_content_rect() const {
    float content_x = position.x + border_width;
    float content_y = position.y + border_width + (chrome_visible ? title_bar_height : 0.0f);
    float content_w = size.x - border_width * 2.0f;
    float content_h = size.y - border_width * 2.0f - (chrome_visible ? title_bar_height : 0.0f);
    return {content_x, content_y, content_w, content_h};
}

bool GUIWindow::is_closed() const {
    return closed;
}

void GUIWindow::set_close_callback(const std::function<void()>& callback) {
    close_callback = callback;
}

SDL_FRect GUIWindow::get_close_button_rect() const {
    return {
        position.x + size.x - border_width - close_button_size - 6.0f,
        position.y + border_width + (title_bar_height - close_button_size) * 0.5f,
        close_button_size,
        close_button_size
    };
}

void GUIWindow::draw_close_button(SDL_Renderer* renderer) {
    close_button_rect = get_close_button_rect();
    
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

static TTF_Font* load_default_font() {
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
        title_font = load_default_font();
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

GUIWindow* GUIEngine::create_window(float x, float y, float width, float height, const std::string& title) {
    if (main_window) {
        delete main_window;
    }
    main_window = new GUIWindow(x, y, width, height, title, title_font, renderer);
    return main_window;
}

GUIWindow* GUIEngine::create_info_window(float x, float y, float width, float height) {
    if (info_window) {
        delete info_window;
    }

    info_window = new GUIWindow(x, y, width, height, "", title_font, renderer);
    info_window->set_chrome_visible(false);
    return info_window;
}

bool GUIEngine::handle_event(const SDL_Event& e) {
    bool consumed = false;

    if (inv_window) {
        consumed = inv_window->handle_event(e);
        if (inv_window->is_closed()) {
            delete inv_window;
            inv_window = nullptr;
            return true;
        }

        if (consumed) return true;
    }

    if (main_window) {
        consumed = main_window->handle_event(e);
        if (main_window->is_closed()) {
            delete main_window;
            main_window = nullptr;
            return true;
        }

        return consumed;
    }

    return consumed;
}

void GUIEngine::render_all() {
    if (main_window) main_window->render(renderer);
    if (inv_window) inv_window->render(renderer);
    if (info_window) info_window->render(renderer);
}

void GUIEngine::clear_windows() {
    if (main_window) {
        delete main_window;
        main_window = nullptr;
    }
}

GUIWindow* GUIEngine::get_window() const {
    return main_window;
}

GUIWindow* GUIEngine::create_inv_window(float x, float y, float w, float h, const std::string& title) {
    if (inv_window) delete inv_window;

    inv_window = new GUIWindow(x, y, w, h, title, title_font, renderer);
    return inv_window;
}

void GUIEngine::close_inv_window() {
    if (inv_window) {
        delete inv_window;
        inv_window = nullptr;
    }
}

void GUIEngine::close_info_window() {
    if (info_window) {
        delete info_window;
        info_window = nullptr;
    }
}
