#include "gui.h"
#include "textrenderer.h"
#include "logger.h"
#include <cmath>

GUIButton::GUIButton(SDL_FRect rect, const std::string& label)
    : rect(rect), label(label), hovered(false), pressed(false), font_scale(1.0f), last_ticks(0) {}

bool GUIButton::IsMouseOver(float mouse_x, float mouse_y) const {
    return mouse_x >= rect.x && mouse_x <= rect.x + rect.w &&
           mouse_y >= rect.y && mouse_y <= rect.y + rect.h;
}

void GUIButton::HandleMouseDown(float mouse_x, float mouse_y) {
    if (IsMouseOver(mouse_x, mouse_y)) {
        pressed = true;
    }
}

void GUIButton::HandleMouseUp(float mouse_x, float mouse_y) {
    if (pressed && IsMouseOver(mouse_x, mouse_y)) {
        if (on_click) {
            on_click();
        }
    }
    pressed = false;
}

void GUIButton::HandleMouseMove(float mouse_x, float mouse_y) {
    hovered = IsMouseOver(mouse_x, mouse_y);
    if (!hovered) {
        pressed = false;
    }
}

SDL_Color GUIButton::GetBackgroundColor() const {
    if (pressed) return {38, 41, 48, 255};
    if (hovered) return {30, 33, 39, 255};
    return {22, 24, 29, 255};
}

SDL_Color GUIButton::GetTextColor() const {
    if (pressed) return {242, 244, 246, 255};
    if (hovered) return {210, 213, 218, 255};
    return {155, 160, 168, 255};
}

void GUIButton::Draw(SDL_Renderer* renderer, TTF_Font* font) {
    Uint64 current_ticks = SDL_GetTicks();
    if (last_ticks == 0) last_ticks = current_ticks;
    float dt = (float)(current_ticks - last_ticks) / 1000.0f;
    last_ticks = current_ticks;

    if (dt > 0.1f) dt = 0.1f; 

    float target = hovered ? 1.2f : 1.0f;
    font_scale += (target - font_scale) * 10.0f * dt;

    SDL_Color bg_color = GetBackgroundColor();
    SDL_Color border_color = hovered
    ? SDL_Color{60, 65, 75, 255}
    : SDL_Color{42, 46, 54, 255};
    
    SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderFillRect(renderer, &rect);
    
    SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_FRect border_top = {rect.x, rect.y, rect.w, 2.0f};
    SDL_FRect border_bottom = {rect.x, rect.y + rect.h - 2.0f, rect.w, 2.0f};
    SDL_FRect border_left = {rect.x, rect.y, 2.0f, rect.h};
    SDL_FRect border_right = {rect.x + rect.w - 2.0f, rect.y, 2.0f, rect.h};
    
    SDL_RenderFillRect(renderer, &border_top);
    SDL_RenderFillRect(renderer, &border_bottom);
    SDL_RenderFillRect(renderer, &border_left);
    SDL_RenderFillRect(renderer, &border_right);
    
    if (font) {
        SDL_Color text_color = GetTextColor();
        int text_w = 0;
        int text_h = 0;
        TTF_GetStringSize(font, label.c_str(), 0, &text_w, &text_h);

        float scaled_w = (float)text_w * font_scale;
        float scaled_h = (float)text_h * font_scale;
        float text_x = rect.x + (rect.w - scaled_w) * 0.5f;
        float text_y = rect.y + (rect.h - scaled_h) * 0.5f;
        
        TextRenderer::DrawTextScaled(renderer, font, text_x, text_y, label, text_color, font_scale);
    }
}

GUICheckbox::GUICheckbox(SDL_FRect rect, const std::string& label, bool initial)
    : rect(rect), label(label), checked(initial), hovered(false), font_scale(1.0f), last_ticks(0) {}

void GUICheckbox::HandleMouseDown(float mx, float my) {
    if (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h) {
        checked = !checked;
        if (on_change) on_change(checked);
    }
}

void GUICheckbox::HandleMouseMove(float mx, float my) {
    hovered = (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h);
}

void GUICheckbox::Draw(SDL_Renderer* renderer, TTF_Font* font) {
    Uint64 current_ticks = SDL_GetTicks();
    if (last_ticks == 0) last_ticks = current_ticks;
    float dt = (float)(current_ticks - last_ticks) / 1000.0f;
    last_ticks = current_ticks;
    if (dt > 0.1f) dt = 0.1f;

    float target = hovered ? 1.15f : 1.0f;
    font_scale += (target - font_scale) * 10.0f * dt;

    float box_size = rect.h * 0.6f;
    SDL_FRect box_rect = { rect.x, rect.y + (rect.h - box_size) * 0.5f, box_size, box_size };

    SDL_SetRenderDrawColor(renderer, 22, 24, 29, 255);
    SDL_RenderFillRect(renderer, &box_rect);
    SDL_SetRenderDrawColor(renderer, hovered ? 60 : 42, hovered ? 65 : 46, hovered ? 75 : 54, 255);
    SDL_RenderRect(renderer, &box_rect);

    if (checked) {
        SDL_SetRenderDrawColor(renderer, 220, 196, 134, 255);
        SDL_FRect check_mark = { box_rect.x + 4, box_rect.y + 4, box_rect.w - 8, box_rect.h - 8 };
        SDL_RenderFillRect(renderer, &check_mark);
    }

    if (font) {
        SDL_Color text_color = hovered ? SDL_Color{210, 213, 218, 255} : SDL_Color{155, 160, 168, 255};
        TextRenderer::DrawTextScaled(renderer, font, box_rect.x + box_rect.w + 10.0f, rect.y + (rect.h - (float)TTF_GetFontHeight(font) * font_scale) * 0.5f, label, text_color, font_scale);
    }
}

GUISlider::GUISlider(SDL_FRect rect, const std::string& label, float initial)
    : rect(rect), label(label), value(initial), hovered(false), dragging(false), font_scale(1.0f), last_ticks(0) {}

void GUISlider::UpdateValueFromMouse(float mx) {
    value = (mx - rect.x) / rect.w;
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    if (on_change) on_change(value);
}

void GUISlider::HandleMouseDown(float mx, float my) {
    if (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h) {
        dragging = true;
        UpdateValueFromMouse(mx);
    }
}

void GUISlider::HandleMouseUp(float mx, float my) {
    dragging = false;
}

void GUISlider::HandleMouseMove(float mx, float my) {
    hovered = (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h);
    if (dragging) UpdateValueFromMouse(mx);
}

void GUISlider::Draw(SDL_Renderer* renderer, TTF_Font* font) {
    Uint64 current_ticks = SDL_GetTicks();
    if (last_ticks == 0) last_ticks = current_ticks;
    float dt = (float)(current_ticks - last_ticks) / 1000.0f;
    last_ticks = current_ticks;
    if (dt > 0.1f) dt = 0.1f;

    float target = (hovered || dragging) ? 1.1f : 1.0f;
    font_scale += (target - font_scale) * 10.0f * dt;

    SDL_FRect bar = { rect.x, rect.y + rect.h * 0.6f, rect.w, 6.0f };
    SDL_SetRenderDrawColor(renderer, 22, 24, 29, 255);
    SDL_RenderFillRect(renderer, &bar);
    
    SDL_FRect fill = { bar.x, bar.y, bar.w * value, bar.h };
    SDL_SetRenderDrawColor(renderer, 220, 196, 134, 255);
    SDL_RenderFillRect(renderer, &fill);

    SDL_FRect handle = { bar.x + bar.w * value - 5.0f, bar.y - 4.0f, 10.0f, 14.0f };
    SDL_SetRenderDrawColor(renderer, 242, 244, 246, 255);
    SDL_RenderFillRect(renderer, &handle);

    if (font) {
        std::string full_label = label + ": " + std::to_string((int)(value * 100)) + "%";
        SDL_Color text_color = (hovered || dragging) ? SDL_Color{210, 213, 218, 255} : SDL_Color{155, 160, 168, 255};
        TextRenderer::DrawTextScaled(renderer, font, rect.x, rect.y, full_label, text_color, font_scale);
    }
}

GUICombo::GUICombo(SDL_FRect rect, const std::string& label, const std::vector<std::string>& options, int initial)
    : rect(rect), label(label), options(options), selected_index(initial), expanded(false), hovered(false), font_scale(1.0f), last_ticks(0) {}

SDL_FRect GUICombo::GetOptionRect(int index) const {
    return { rect.x, rect.y + rect.h + (float)index * rect.h, rect.w, rect.h };
}

void GUICombo::HandleMouseDown(float mx, float my) {
    if (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h) {
        expanded = !expanded;
        return;
    }

    if (expanded) {
        for (int i = 0; i < (int)options.size(); ++i) {
            SDL_FRect opt_rect = GetOptionRect(i);
            if (mx >= opt_rect.x && mx <= opt_rect.x + opt_rect.w && my >= opt_rect.y && my <= opt_rect.y + opt_rect.h) {
                selected_index = i;
                expanded = false;
                if (on_change) on_change(selected_index);
                return;
            }
        }
        expanded = false;
    }
}

void GUICombo::HandleMouseMove(float mx, float my) {
    bool over_main = (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h);
    bool over_options = false;
    if (expanded) {
        float total_h = (float)options.size() * rect.h;
        over_options = (mx >= rect.x && mx <= rect.x + rect.w && 
                        my >= rect.y + rect.h && my <= rect.y + rect.h + total_h);
    }
    hovered = over_main || over_options;
}

void GUICombo::Draw(SDL_Renderer* renderer, TTF_Font* font) {
    Uint64 current_ticks = SDL_GetTicks();
    if (last_ticks == 0) last_ticks = current_ticks;
    float dt = (float)(current_ticks - last_ticks) / 1000.0f;
    last_ticks = current_ticks;
    if (dt > 0.1f) dt = 0.1f;

    float target = hovered ? 1.1f : 1.0f;
    font_scale += (target - font_scale) * 10.0f * dt;

    SDL_SetRenderDrawColor(renderer, 22, 24, 29, 255);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, hovered ? 60 : 42, hovered ? 65 : 46, hovered ? 75 : 54, 255);
    SDL_RenderRect(renderer, &rect);

    if (font) {
        std::string display = options.empty() ? "" : options[selected_index];
        SDL_Color text_color = {242, 244, 246, 255};
        
        float text_h = (float)TTF_GetFontHeight(font) * font_scale;
        TextRenderer::DrawTextScaled(renderer, font, rect.x + 8, rect.y + (rect.h - text_h) * 0.5f, display, text_color, font_scale);
        
        float arrow_h = (float)TTF_GetFontHeight(font) * font_scale;
        TextRenderer::DrawTextScaled(renderer, font, rect.x + rect.w - 20 * font_scale, rect.y + (rect.h - arrow_h) * 0.5f, expanded ? "^" : "v", text_color, font_scale);
    }

    if (expanded) {
        int visible_count = std::min((int)options.size(), max_visible_items);
        
        for (int i = 0; i < visible_count; ++i) {
            int idx = i + scroll_index;
            SDL_FRect opt_rect = { rect.x, rect.y + rect.h + (float)i * rect.h, rect.w, rect.h };
            
            float mx, my;
            SDL_GetMouseState(&mx, &my);
            bool opt_hovered = (mx >= opt_rect.x && mx <= opt_rect.x + opt_rect.w && my >= opt_rect.y && my <= opt_rect.y + opt_rect.h);

            SDL_SetRenderDrawColor(renderer, opt_hovered ? 38 : 14, opt_hovered ? 41 : 15, opt_hovered ? 48 : 18, 255);
            SDL_RenderFillRect(renderer, &opt_rect);
            SDL_SetRenderDrawColor(renderer, 42, 46, 54, 255);
            SDL_RenderRect(renderer, &opt_rect);

            if (font) {
                SDL_Color opt_text_color = (idx == selected_index) ? SDL_Color{220, 196, 134, 255} : SDL_Color{200, 200, 200, 255};
                TextRenderer::DrawText(renderer, font, opt_rect.x + 8, opt_rect.y + (opt_rect.h - (float)TTF_GetFontHeight(font)) * 0.5f, options[idx], opt_text_color);
            }
        }

        if (options.size() > (size_t)max_visible_items) {
            SDL_FRect scroll_track = GetScrollbarRect();
            SDL_SetRenderDrawColor(renderer, 30, 33, 39, 255);
            SDL_RenderFillRect(renderer, &scroll_track);
            SDL_SetRenderDrawColor(renderer, 42, 46, 54, 255);
            SDL_RenderRect(renderer, &scroll_track);

            float track_h = scroll_track.h;
            float handle_h = std::max(10.0f, (float)max_visible_items / (float)options.size() * track_h);
            float scroll_pct = (float)scroll_index / (float)(options.size() - max_visible_items);
            float handle_y = scroll_track.y + (track_h - handle_h) * scroll_pct;

            SDL_FRect scroll_handle = { scroll_track.x + 2, handle_y + 2, scroll_track.w - 4, handle_h - 4 };
            SDL_SetRenderDrawColor(renderer, 220, 196, 134, 255);
            SDL_RenderFillRect(renderer, &scroll_handle);
        }
    }
}

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
    : renderer(renderer), main_window(nullptr), inv_window(nullptr), info_window(nullptr), queue_window(nullptr), title_font(nullptr) {
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

    if (queue_window) {
        delete queue_window;
        queue_window = nullptr;
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

GUIWindow* GUIEngine::CreateQueueWindow(SDL_FRect size) {
    if (queue_window) {
        delete queue_window;
    }

    queue_window = new GUIWindow(size, "", title_font, renderer);
    queue_window->SetChromeVisible(false);
    return queue_window;
}

void GUIEngine::CloseQueueWindow() {
    if (queue_window) {
        delete queue_window;
        queue_window = nullptr;
    }
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

    if (queue_window) {
        consumed = queue_window->HandleEvent(e);
        if (queue_window->IsClosed()) {
            delete queue_window;
            queue_window = nullptr;
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

    if (queue_window && queue_window->GetContentRect().x <= mouse_pos.x && mouse_pos.x <= queue_window->GetContentRect().x + queue_window->GetContentRect().w &&
        queue_window->GetContentRect().y <= mouse_pos.y && mouse_pos.y <= queue_window->GetContentRect().y + queue_window->GetContentRect().h) {
        return true;
    }

    return false;
}

void GUIEngine::RenderAll() {
    if (main_window) main_window->Render(renderer);
    if (inv_window) inv_window->Render(renderer);
    if (info_window) info_window->Render(renderer);
    if (queue_window) queue_window->Render(renderer);
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
