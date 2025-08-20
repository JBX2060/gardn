// RendererQueue.cc
#include "RendererQueue.hh"
#include <emscripten.h>

Instruction gTape[INSTR_MAX];
std::uint32_t gSize = 0;
std::uint32_t gRollovers = 0;

static inline Instruction& rr_alloc() {
    if (gSize >= INSTR_MAX) {
        rr_renderer_flush(); // auto-flush to make room
    }
    return gTape[gSize++];
}

void rr_emit0(RROp op, uint16_t ctx) {
    Instruction& i = rr_alloc();
    i.op = (uint8_t)op; i._pad0 = 0; i.ctx = ctx; i.ctx2 = 0; i._pad1 = 0;
    // zero out payload for safety
    i.f[0]=i.f[1]=i.f[2]=i.f[3]=i.f[4]=i.f[5]=0.0f;
    i.sPtr = 0;
}

void rr_emitF(RROp op, uint16_t ctx, const float* f, int n) {
    Instruction& i = rr_alloc();
    i.op = (uint8_t)op; i._pad0 = 0; i.ctx = ctx; i.ctx2 = 0; i._pad1 = 0;
    for (int k=0;k<6;++k) i.f[k] = (k<n ? f[k] : 0.0f);
    i.sPtr = 0;
}
void rr_emitFF(RROp op, uint16_t ctx, float f0)                       { float f[1]={f0}; rr_emitF(op,ctx,f,1); }
void rr_emitF2(RROp op, uint16_t ctx, float a,float b)                 { float f[2]={a,b}; rr_emitF(op,ctx,f,2); }
void rr_emitF3(RROp op, uint16_t ctx, float a,float b,float c)         { float f[3]={a,b,c}; rr_emitF(op,ctx,f,3); }
void rr_emitF4(RROp op, uint16_t ctx, float a,float b,float c,float d) { float f[4]={a,b,c,d}; rr_emitF(op,ctx,f,4); }
void rr_emitF6(RROp op, uint16_t ctx, float a,float b,float c,float d,float e,float f6){ float f[6]={a,b,c,d,e,f6}; rr_emitF(op,ctx,f,6); }

void rr_emitText(RROp op, uint16_t ctx, const char* s) {
    // Copy to HEAP so JS can UTF8ToString and free.
    const size_t len = std::strlen(s);
    char* buf = (char*)std::malloc(len + 1);
    if (!buf) return; // best-effort; drop on OOM
    std::memcpy(buf, s, len + 1);

    Instruction& i = rr_alloc();
    i.op = (uint8_t)op; i._pad0 = 0; i.ctx = ctx; i.ctx2 = 0; i._pad1 = 0;
    i.f[0]=i.f[1]=i.f[2]=i.f[3]=i.f[4]=i.f[5]=0.0f;
    i.sPtr = (uint32_t)(uintptr_t)buf;
}

void rr_renderer_flush() {
    if (!gSize) return;

    ++gRollovers;

    // A single boundary-crossing: iterate the tape on the JS side.
    EM_ASM(
    {
        let ip   = $0;
        let size = $1;
        const stride = $2;

        for (let n = 0; n < size; ++n) {
            const op   = HEAPU8[ip];
            const ctx  = HEAPU16[(ip + 2) >> 1];
            const ctx2 = HEAPU16[(ip + 4) >> 1];
            const f    = HEAPF32.subarray((ip + 8) >> 2, (ip + 32) >> 2);
            const sPtr = HEAPU32[(ip + 32) >> 2];
            let str;

            switch (op) {
            case 0:  Module.ctxs[ctx].fillStyle   = `rgba(${f[0]},${f[1]},${f[2]},${f[3]})`; break; // SetFill
            case 1:  Module.ctxs[ctx].strokeStyle = `rgba(${f[0]},${f[1]},${f[2]},${f[3]})`; break; // SetStroke
            case 2:  Module.ctxs[ctx].lineWidth = f[0]; break;
            case 3:  Module.ctxs[ctx].font = f[0] + 'px Ubuntu'; break;
            case 4:  Module.ctxs[ctx].globalAlpha = f[0]; break;
            case 5:  Module.ctxs[ctx].lineCap = 'round'; break;
            case 6:  Module.ctxs[ctx].lineJoin = 'round'; break;
            case 7:  Module.ctxs[ctx].textAlign = 'center'; break;
            case 8:  Module.ctxs[ctx].textBaseline = 'middle'; break;
            case 9:  Module.ctxs[ctx].setTransform(f[0], f[1], f[3], f[4], f[2], f[5]); break; // (a,b,c,d,e,f) -> (a,b,d,e,c,f)
            case 10: Module.ctxs[ctx].save(); break;
            case 11: Module.ctxs[ctx].restore(); break;
            case 12: Module.ctxs[ctx].beginPath(); break;
            case 13: Module.ctxs[ctx].moveTo(f[0], f[1]); break;
            case 14: Module.ctxs[ctx].lineTo(f[0], f[1]); break;
            case 15: Module.ctxs[ctx].quadraticCurveTo(f[0], f[1], f[2], f[3]); break;
            case 16: Module.ctxs[ctx].bezierCurveTo(f[0], f[1], f[2], f[3], f[4], f[5]); break;
            case 17: Module.ctxs[ctx].arc(f[0], f[1], f[2], f[3], f[4], !!f[5]); break;
            case 18: Module.ctxs[ctx].ellipse(f[0], f[1], f[2], f[3], 0, Math.PI * 2, 0); break;
            case 19: Module.ctxs[ctx].rect(f[0], f[1], f[2], f[3]); break;
            case 20: Module.ctxs[ctx].roundRect(f[0], f[1], f[2], f[3], f[4]); break;
            case 21: Module.ctxs[ctx].fillRect(f[0], f[1], f[2], f[3]); break;
            case 22: Module.ctxs[ctx].strokeRect(f[0], f[1], f[2], f[3]); break;
            case 23: Module.ctxs[ctx].fill(); break;
            case 24: Module.ctxs[ctx].stroke(); break;
            case 25: Module.ctxs[ctx].clip(); break;
            case 26: str = UTF8ToString(sPtr); Module.ctxs[ctx].fillText(str, 0, 0); _free(sPtr); break;
            case 27: str = UTF8ToString(sPtr); Module.ctxs[ctx].strokeText(str, 0, 0); _free(sPtr); break;
            case 28: Module.ctxs[ctx].closePath(); break;
            case 29: Module.ctxs[ctx].textAlign = 'left'; break;
            case 30: Module.ctxs[ctx].textAlign = 'right'; break;
            }
            ip += stride;
        }
    }, gTape, gSize, (int)sizeof(Instruction));

    gSize = 0; // clear for next frame
}
