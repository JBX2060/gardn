#include <Client/Render/Renderer.hh>
#include "RendererQueue.hh"

#include <Shared/Helpers.hh>

#include <cmath>
#include <iostream>
#include <emscripten.h>

std::vector<Renderer *> Renderer::renderers;

RenderContext::RenderContext() {}


void RenderContext::reset() {
    amount = 0;
    color_filter = 0;
    clip_x = renderer->width / 2;
    clip_y = renderer->height / 2;
    clip_w = renderer->width;
    clip_h = renderer->height;
}


Renderer::Renderer() : context() {
    id = EM_ASM_INT({
        let idx;
        if (Module.availableCtxs.length > 0)
            idx = Module.availableCtxs.pop();
        else
            idx = Module.ctxs.length;
        if (idx === 0) {
            Module.ctxs[idx] = document.getElementById('canvas').getContext('2d');
        } else {
            const canvas = new OffscreenCanvas(1,1);
            Module.ctxs[idx] = canvas.getContext('2d');
        }
        return idx;
    });
    DEBUG_ONLY(std::cout << "created canvas " << id << '\n';)
    Renderer::renderers.push_back(this);
    context.renderer = this;
    context.reset();
}

Renderer::~Renderer() {
    EM_ASM({
        if ($0 == 0)
            throw new Error('Tried to delete the main context');
        Module.ctxs[$0] = null;
        Module.availableCtxs.push($0);
    }, id);
    DEBUG_ONLY(std::cout << "removed canvas " << id << '\n';)
}

uint32_t Renderer::HSV(uint32_t c, float v) {
    return MIX(c >> 24 << 24, c, v);
}

uint32_t Renderer::MIX(uint32_t base, uint32_t mix, float v) {
    uint8_t b = fclamp((mix & 255) * v + (base & 255) * (1 - v), 0, 255);
    uint8_t g = fclamp(((mix >> 8) & 255) * v + ((base >> 8) & 255) * (1 - v), 0, 255);
    uint8_t r = fclamp(((mix >> 16) & 255) * v + ((base >> 16) & 255) * (1 - v), 0, 255);
    uint8_t a = base >> 24;
    return (a << 24) | (r << 16) | (g << 8) | b;
}

void Renderer::reset() {
    reset_transform();
    round_line_cap();
    round_line_join();
    center_text_align();
    center_text_baseline();
    context.reset();
}
void Renderer::set_dimensions(float w, float h) {
    width = w;
    height = h;
    EM_ASM({ Module.ctxs[$0].canvas.width = $1; Module.ctxs[$0].canvas.height = $2; }, id, w, h);
}

void Renderer::add_color_filter(uint32_t c, float v) {
    context.color_filter = c;
    context.amount = v;
}

void Renderer::set_fill(uint32_t v) {
    v = MIX(v, context.color_filter, context.amount);
    rr_emitColor(RROp::SetFill, (uint16_t)id, v);
}
void Renderer::set_stroke(uint32_t v) {
    v = MIX(v, context.color_filter, context.amount);
    rr_emitColor(RROp::SetStroke, (uint16_t)id, v);
}

void Renderer::set_line_width(float v)   { rr_emitFF(RROp::LineWidth,   (uint16_t)id, v); }
void Renderer::set_text_size(float v)    { rr_emitFF(RROp::FontSize,    (uint16_t)id, v); }
void Renderer::set_global_alpha(float v) { rr_emitFF(RROp::GlobalAlpha, (uint16_t)id, v); }

void Renderer::round_line_cap()      { rr_emit0(RROp::LineCapRound,   (uint16_t)id); }
void Renderer::round_line_join()     { rr_emit0(RROp::LineJoinRound,  (uint16_t)id); }
void Renderer::center_text_align()   { rr_emit0(RROp::TextAlignCenter,(uint16_t)id); }
void Renderer::center_text_baseline(){ rr_emit0(RROp::TextBaselineMiddle,(uint16_t)id); }

// Transform: keep your matrix, just queue the JS call.
void Renderer::set_transform(float a, float b, float c, float d, float e, float f) {
    context.transform_matrix[0] = a;
    context.transform_matrix[1] = b;
    context.transform_matrix[2] = c;
    context.transform_matrix[3] = d;
    context.transform_matrix[4] = e;
    context.transform_matrix[5] = f;
    rr_emitF6(RROp::SetTransform, (uint16_t)id, a,b,c,d,e,f);
}
void Renderer::scale(float s) {
    context.transform_matrix[0] *= s;
    context.transform_matrix[1] *= s;
    context.transform_matrix[3] *= s;
    context.transform_matrix[4] *= s;
    rr_emitF6(RROp::SetTransform,(uint16_t)id,
              context.transform_matrix[0],context.transform_matrix[1],
              context.transform_matrix[2],context.transform_matrix[3],
              context.transform_matrix[4],context.transform_matrix[5]);
}
void Renderer::scale(float x, float y) {
    context.transform_matrix[0] *= x;
    context.transform_matrix[1] *= x;
    context.transform_matrix[3] *= y;
    context.transform_matrix[4] *= y;
    rr_emitF6(RROp::SetTransform,(uint16_t)id,
              context.transform_matrix[0],context.transform_matrix[1],
              context.transform_matrix[2],context.transform_matrix[3],
              context.transform_matrix[4],context.transform_matrix[5]);
}
void Renderer::translate(float x, float y) {
    context.transform_matrix[2] += x * context.transform_matrix[0] + y * context.transform_matrix[3];
    context.transform_matrix[5] += y * context.transform_matrix[4] + x * context.transform_matrix[1];
    rr_emitF6(RROp::SetTransform,(uint16_t)id,
              context.transform_matrix[0],context.transform_matrix[1],
              context.transform_matrix[2],context.transform_matrix[3],
              context.transform_matrix[4],context.transform_matrix[5]);
}
void Renderer::rotate(float a) {
    float cos_a = cosf(a), sin_a = sinf(a);
    float o0 = context.transform_matrix[0];
    float o1 = context.transform_matrix[1];
    float o3 = context.transform_matrix[3];
    float o4 = context.transform_matrix[4];
    context.transform_matrix[0] = o0 * cos_a + o1 * -sin_a;
    context.transform_matrix[1] = o0 * sin_a + o1 *  cos_a;
    context.transform_matrix[3] = o3 * cos_a + o4 * -sin_a;
    context.transform_matrix[4] = o3 * sin_a + o4 *  cos_a;
    rr_emitF6(RROp::SetTransform,(uint16_t)id,
              context.transform_matrix[0],context.transform_matrix[1],
              context.transform_matrix[2],context.transform_matrix[3],
              context.transform_matrix[4],context.transform_matrix[5]);
}
void Renderer::reset_transform() { set_transform(1,0,0,0,1,0); }

// Path ops
void Renderer::begin_path()                 { rr_emit0 (RROp::BeginPath, (uint16_t)id); }
void Renderer::move_to(float x,float y)     { rr_emitF2(RROp::MoveTo,    (uint16_t)id, x,y); }
void Renderer::line_to(float x,float y)     { rr_emitF2(RROp::LineTo,    (uint16_t)id, x,y); }
void Renderer::qcurve_to(float x,float y,float x1,float y1) { rr_emitF4(RROp::QCurveTo,(uint16_t)id,x,y,x1,y1); }
void Renderer::bcurve_to(float x,float y,float x1,float y1,float x2,float y2) { rr_emitF6(RROp::BCurveTo,(uint16_t)id,x,y,x1,y1,x2,y2); }
void Renderer::partial_arc(float x,float y,float r,float sa,float ea,uint8_t ccw) { rr_emitF6(RROp::Arc,(uint16_t)id,x,y,r,sa,ea,(float)ccw); }
void Renderer::arc(float x,float y,float r)            { partial_arc(x,y,r,0,2*M_PI,0); }
void Renderer::reverse_arc(float x,float y,float r)    { partial_arc(x,y,r,0,2*M_PI,1); }
void Renderer::ellipse(float x,float y,float r1,float r2,float a) { rr_emitF4(RROp::Ellipse,(uint16_t)id,x,y,r1,r2); /* angle fixed in JS */ }
void Renderer::ellipse(float x,float y,float r1,float r2) { ellipse(x,y,r1,r2,0); }
void Renderer::fill_rect(float x,float y,float w,float h)   { rr_emitF4(RROp::FillRect,(uint16_t)id,x,y,w,h); }
void Renderer::stroke_rect(float x,float y,float w,float h) { rr_emitF4(RROp::StrokeRect,(uint16_t)id,x,y,w,h); }
void Renderer::rect(float x,float y,float w,float h)        { rr_emitF4(RROp::Rect,(uint16_t)id,x,y,w,h); }
void Renderer::round_rect(float x,float y,float w,float h,float r) { rr_emitF6(RROp::RoundRect,(uint16_t)id,x,y,w,h,r,0.0f); }
void Renderer::close_path()  { rr_emit0(RROp::ClosePath,(uint16_t)id); }
void Renderer::fill(uint8_t) { rr_emit0(RROp::Fill,(uint16_t)id); }   // winding rule is not currently varied in your code
void Renderer::stroke()      { rr_emit0(RROp::Stroke,(uint16_t)id); }
void Renderer::clip()        { rr_emit0(RROp::Clip,(uint16_t)id); }

void Renderer::clip_rect(float x,float y,float w,float h) {
    // assumes axis oriented scaling
    context.clip_x = x * context.transform_matrix[0] + context.transform_matrix[2];
    context.clip_w = w * context.transform_matrix[0];
    context.clip_y = y * context.transform_matrix[4] + context.transform_matrix[5];
    context.clip_h = h * context.transform_matrix[4];
    begin_path();
    rect(x - w/2, y - h/2, w, h);
    clip();
}

// Text
void Renderer::fill_text(char const* text)   { rr_emitText(RROp::FillText,   (uint16_t)id, text); }
void Renderer::stroke_text(char const* text) { rr_emitText(RROp::StrokeText, (uint16_t)id, text); }

void Renderer::draw_text(char const *text, struct TextArgs const args) {
    set_fill(args.fill);
    set_stroke(args.stroke);
    set_text_size(args.size);
    if (args.stroke_scale > 0) {
        set_line_width(args.size * args.stroke_scale);
        stroke_text(text);
    }
    fill_text(text);
}

RenderContext::RenderContext(Renderer* r) {
    *this = r->context;
    renderer = r;
    rr_emit0(RROp::Save, (uint16_t)r->id);
}
RenderContext::~RenderContext() {
    renderer->context = *this;
    rr_emit0(RROp::Restore, (uint16_t)renderer->id);
}

void Renderer::flush() { rr_renderer_flush(); }

//precalculated for standard Ubuntu font 
constexpr float CHAR_WIDTHS[128] = {0,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0,0.24,0.24,0.24,0.24,0.24,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0,0.5,0.5,0.24,0.286,0.465,0.699,0.568,0.918,0.705,0.247,0.356,0.356,0.502,0.568,0.246,0.34,0.246,0.437,0.568,0.568,0.568,0.568,0.568,0.568,0.568,0.568,0.568,0.568,0.246,0.246,0.568,0.568,0.568,0.455,0.974,0.721,0.672,0.648,0.737,0.606,0.574,0.702,0.734,0.316,0.529,0.684,0.563,0.897,0.756,0.79,0.644,0.79,0.667,0.582,0.614,0.707,0.722,0.948,0.675,0.661,0.61,0.371,0.437,0.371,0.568,0.5,0.286,0.553,0.604,0.5,0.604,0.584,0.422,0.594,0.589,0.289,0.289,0.579,0.316,0.862,0.589,0.607,0.604,0.604,0.422,0.485,0.444,0.589,0.55,0.784,0.554,0.547,0.5,0.371,0.322,0.371,0.568,0.5};
float Renderer::get_ascii_text_size(char const* text) {
  float w = 0.0f;
  UTF8Parser parser(text);
  while (1) {
    uint32_t c = parser.next_symbol();
    if (c == 0) break;
    if (c < 128) w += CHAR_WIDTHS[c];
    else w += 1.0f;
  }
  return w;
}