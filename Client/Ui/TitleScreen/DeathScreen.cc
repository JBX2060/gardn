#include <Client/Ui/TitleScreen/TitleScreen.hh>

#include <Client/Game.hh>
#include <Client/Ui/Button.hh>
#include <Client/Ui/DynamicText.hh>
#include <Client/Ui/Container.hh>
#include <Client/Ui/StaticText.hh>
#include <Client/Assets/Assets.hh>

using namespace Ui;

class DeathFlower final : public Ui::Element {
public:
    DeathFlower() : Ui::Element(100, 100, { .no_animation = 1 }) {}

    virtual void on_render(Renderer &ctx) override {
        ctx.rotate(-0.25);
        float radius = 40;
        ctx.scale(radius / 25);

        auto try_draw = [&](int idx, float tx, float ty) {
            PetalID::T id = Game::cached_loadout[idx];
            if (id == PetalID::kNone) return;
            RenderContext guard(&ctx);
            ctx.translate(tx, ty);
            draw_static_petal(id, ctx);
        };

        // Back/top petals
        try_draw(4, 25, -10);
        try_draw(3, -2, -22);
        try_draw(2, -28, -4);

        // Face
        uint32_t base_color = 0xffffe763;
        ctx.set_stroke(Renderer::HSV(base_color, 0.747f));
        ctx.set_fill(base_color);
        ctx.set_line_width(3);
        ctx.begin_path();
        ctx.arc(0, 0, 25);
        ctx.fill();
        ctx.stroke();

        // Eyes
        {
            ctx.round_line_cap();
            ctx.set_stroke(0xff222222);
            ctx.set_line_width(3.0);
            ctx.begin_path();
            ctx.move_to(-10, -8);
            ctx.line_to(-4, -2);
            ctx.move_to(-4, -8);
            ctx.line_to(-10, -2);
            ctx.move_to(10, -8);
            ctx.line_to(4, -2);
            ctx.move_to(4, -8);
            ctx.line_to(10, -2);
            ctx.stroke();
        }

        // Mouth
        ctx.set_stroke(0xff222222);
        ctx.set_line_width(1.5);
        ctx.round_line_cap();
        ctx.begin_path();
        ctx.move_to(-6, 10);
        ctx.qcurve_to(0, 5, 6, 10);
        ctx.stroke();

        // Bottom/front petals
        try_draw(1, -11, 20);
        try_draw(0, 20 + 12.5, 18); // + 12.5 offset
    }
};

Element *Ui::make_death_main_screen() {
    // Ui::Element *continue_button = new Ui::Button(
    //     145,
    //     40,
    //     new Ui::StaticText(28, "Continue"),
    //     [](Element *elt, uint8_t e){ if (e == Ui::kClick && Game::on_game_screen) Game::on_game_screen = 0; },
    //     [](){ return !Game::in_game(); },
    //     {.fill = 0xff1dd129, .line_width = 5, .round_radius = 3 }
    // );
    Ui::Element *container = new Ui::VContainer({
        new Ui::StaticText(17.5, "You were destroyed by:"),
        new Ui::DynamicText(22.5, [](){
            if (!Game::simulation.ent_exists(Game::camera_id))
                return std::string{""};
            if (Game::simulation.get_ent(Game::camera_id).killed_by == "") 
                return std::string{"a mysterious entity"};
            return Game::simulation.get_ent(Game::camera_id).killed_by;
        }),
        new Ui::Element(0, 20),
        new DeathFlower(),
        new Ui::Element(0, 15),
        new Ui::StaticText(18, "(press enter to continue)")
    }, 0, 10, { .animate = [](Element *elt, Renderer &ctx) {
        ctx.translate(0, (elt->animation - 1) * ctx.height * 0.6);
    }, .should_render = [](){ return !Game::alive() && Game::should_render_game_ui(); } });
    return container;
}

