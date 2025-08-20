#pragma once

#include <Client/Ui/Element.hh>

#include <string>

namespace Ui {
    class TextInput : public Element {
        std::string &ref;
        uint32_t max;
        float cursor_alpha = 1.0f;
        float cursor_blink_time = 0.0f;
        bool cursor_alpha_direction_back = true;
        static TextInput *active;
        void process_keypresses();
        static void pop_utf8_codepoint(std::string &);
    public:
        TextInput(std::string &, float, float, uint32_t, Style = {});

        virtual void on_render(Renderer &) override;
        virtual void on_render_skip(Renderer &) override;
        virtual void on_event(uint8_t) override;
    };
}