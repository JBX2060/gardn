#include <Client/Ui/TextInput.hh>

#include <Client/Ui/Extern.hh>
#include <Client/Input.hh>
#include <Shared/Helpers.hh>

using namespace Ui;

TextInput *TextInput::active = nullptr;

TextInput::TextInput(std::string &r, float w, float h, uint32_t m, Style s)
    : Element(w, h, s), ref(r), max(m) {
    style.fill = 0xffeeeeee;
    style.stroke_hsv = 0;
}

void TextInput::on_render(Renderer &ctx) {
    Element::on_render(ctx);
    active = this;

    float font_size = height * 0.55f;
    ctx.set_text_size(font_size);
    ctx.center_text_baseline();

    process_keypresses();

    float padding = 8.0f;
    float content_left = -width / 2 + padding;
    float text_w = ref.empty() ? 0.0f : font_size * ctx.get_ascii_text_size(ref.c_str());

    if (!ref.empty()) {
        RenderContext text_ctx(&ctx);
        float text_center_x = content_left + text_w / 2.0f;
        ctx.translate(text_center_x, 0);
        ctx.draw_text(ref.c_str(), { .fill = 0xff212121, .stroke = 0xffffffff, .size = font_size, .stroke_scale = 0.08f });
    }

    float capped_dt = std::min((float)Ui::dt / 1000.0f, 0.0167f);
    cursor_blink_time += capped_dt;
    if (cursor_blink_time >= 0.04f) {
        cursor_blink_time -= 0.04f;
        if (cursor_alpha_direction_back) {
            cursor_alpha -= 0.08f;
            if (cursor_alpha <= 0.0f) {
                cursor_alpha_direction_back = false;
                cursor_alpha = 0.0f;
            }
        } else {
            cursor_alpha += 0.08f;
            if (cursor_alpha >= 1.0f) {
                cursor_alpha_direction_back = true;
                cursor_alpha = 1.0f;
            }
        }
    }

    if (active == this && cursor_alpha > 0.0f) {
        float cw = 2.8f;
        float ch = font_size * 1.05f;
        float caret_left_x = content_left + text_w + 0.5f;
        ctx.set_global_alpha(cursor_alpha);
        ctx.set_fill(0xff000000);
        ctx.fill_rect(caret_left_x, -ch / 2, cw, ch);
        float white_width = cw * 0.65f;
        float white_x_offset = (cw - white_width) / 2.0f;
        ctx.set_fill(0xffffffff);
        ctx.fill_rect(caret_left_x + white_x_offset, -ch / 2, white_width, ch);
        ctx.set_global_alpha(1.0f);
    }
}


void TextInput::process_keypresses() {
    static float backspace_hold_time = 0.0f;
    static float last_repeat_time = 0.0f;

    if (Input::keys_pressed.contains(8)) {
        if (Input::keys_pressed_this_tick.contains(8)) {
            pop_utf8_codepoint(ref);
            backspace_hold_time = 0.0f;
            last_repeat_time = 0.0f;
        } else {
            backspace_hold_time += (float)Ui::dt / 1000.0f;
            if (backspace_hold_time >= 0.2f) {
                if (backspace_hold_time - last_repeat_time >= 0.05f) {
                    pop_utf8_codepoint(ref);
                    last_repeat_time = backspace_hold_time;
                }
            }
        }
    } else {
        backspace_hold_time = 0.0f;
        last_repeat_time = 0.0f;
    }

    uint32_t curr_len = 0;
    UTF8Parser p(ref.c_str());
    while (p.next_symbol()) ++curr_len;

    bool shift_down = Input::keys_pressed.contains('\x10');
    for (char c = 65; c <= 90; ++c) {
        if (Input::keys_pressed_this_tick.contains(c)) {
            char out = shift_down ? c : static_cast<char>(c + 32);
            if (curr_len < max) {
                ref.push_back(out);
                ++curr_len;
            }
        }
    }

    for (char c = 32; c <= 126; ++c) {
        if (c >= 65 && c <= 90) continue;
        if (Input::keys_pressed_this_tick.contains(c)) {
            if (curr_len < max) {
                ref.push_back(c);
                ++curr_len;
            }
        }
    }
}

void TextInput::pop_utf8_codepoint(std::string &s) {
    if (s.empty()) return;
    size_t i = s.size();
    do { --i; } while (i > 0 && ((unsigned char)s[i] & 0xC0) == 0x80);
    s.erase(i);
}

void TextInput::on_render_skip(Renderer &ctx) {
}

void TextInput::on_event(uint8_t event) {
    if (event == kMouseDown || event == kClick) {
        active = this;
    } else if (event == kFocusLost) {
        if (active == this) active = nullptr;
    }
}