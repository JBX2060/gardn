// RendererQueue.hh
#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>

enum class RROp : uint8_t {
    SetFill = 0,
    SetStroke,
    LineWidth,
    FontSize,
    GlobalAlpha,
    LineCapRound,
    LineJoinRound,
    TextAlignCenter,
    TextBaselineMiddle,
    SetTransform,
    Save,
    Restore,
    BeginPath,
    MoveTo,
    LineTo,
    QCurveTo,
    BCurveTo,
    Arc,
    Ellipse,
    Rect,
    RoundRect,
    FillRect,
    StrokeRect,
    Fill,
    Stroke,
    Clip,
    FillText,
    StrokeText,
    ClosePath,
    TextAlignLeft,
    TextAlignRight
};

struct Instruction {
    uint8_t  op;     // RROp
    uint8_t  _pad0;  // align to 2
    uint16_t ctx;    // primary canvas context index
    uint16_t ctx2;   // optional secondary context (unused for now)
    uint16_t _pad1;  // align to 4
    float    f[6];   // numeric payload (varies per op)
    uint32_t sPtr;   // HEAP pointer for strings (0 if unused)
};
static_assert(sizeof(Instruction) == 36, "Instruction layout must be 36 bytes");

#ifndef INSTR_MAX
#define INSTR_MAX 16384
#endif

extern Instruction gTape[INSTR_MAX];
extern std::uint32_t gSize;
extern std::uint32_t gRollovers;

// Core API
void rr_renderer_flush();
inline void rr_clear_tape() { gSize = 0; }

// Helpers to emit instructions (auto-flush on overflow)
void rr_emit0(RROp op, uint16_t ctx);
void rr_emitF(RROp op, uint16_t ctx, const float* f, int n);
void rr_emitFF(RROp op, uint16_t ctx, float f0);
void rr_emitF2(RROp op, uint16_t ctx, float f0, float f1);
void rr_emitF3(RROp op, uint16_t ctx, float f0, float f1, float f2);
void rr_emitF4(RROp op, uint16_t ctx, float f0, float f1, float f2, float f3);
void rr_emitF6(RROp op, uint16_t ctx, float f0, float f1, float f2, float f3, float f4, float f5);
void rr_emitText(RROp op, uint16_t ctx, const char* s);

// Convenience for colors packed as 0xAARRGGBB (your current format)
inline void rr_emitColor(RROp op, uint16_t ctx, uint32_t c) {
    const float r = float((c >> 16) & 255);
    const float g = float((c >> 8)  & 255);
    const float b = float( c        & 255);
    const float a = float((c >> 24) & 255) / 255.0f;
    rr_emitF4(op, ctx, r, g, b, a);
}
