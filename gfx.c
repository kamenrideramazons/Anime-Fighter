#include "gfx.h"
#include <pspdisplay.h>
#include <pspgu.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern unsigned char msx[];   /* 8x8 font that ships in libpspdebug */

typedef struct { unsigned int color; short x, y, z; } Vtx;
typedef struct { short u, v; unsigned int color; short x, y, z; } TVtx;

static unsigned int __attribute__((aligned(16))) list[262144];
static int ox, oy;

void gfx_init(void) {
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void *)0, 512);
    sceGuDispBuffer(480, 272, (void *)0x88000, 512);
    sceGuDepthBuffer((void *)0x110000, 512);
    sceGuOffset(2048 - 240, 2048 - 136);
    sceGuViewport(2048, 2048, 480, 272);
    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void gfx_begin(unsigned int clear) {
    sceGuStart(GU_DIRECT, list);
    sceGuClearColor(clear);
    sceGuClear(GU_COLOR_BUFFER_BIT);
    ox = oy = 0;
}

void gfx_end(void) {
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

void gfx_origin(int x, int y) { ox = x; oy = y; }

void gfx_rect(int x, int y, int w, int h, unsigned int col) {
    if (w <= 0 || h <= 0) return;
    Vtx *v = (Vtx *)sceGuGetMemory(2 * sizeof(Vtx));
    v[0] = (Vtx){col, x + ox, y + oy, 0};
    v[1] = (Vtx){col, x + ox + w, y + oy + h, 0};
    sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, v);
}

void gfx_outline(int x, int y, int w, int h, unsigned int col) {
    gfx_rect(x, y, w, 1, col);
    gfx_rect(x, y + h - 1, w, 1, col);
    gfx_rect(x, y, 1, h, col);
    gfx_rect(x + w - 1, y, 1, h, col);
}

void gfx_vgrad(int y0, int y1, unsigned int top, unsigned int bot) {
    int h = y1 - y0; if (h <= 0) return;
    for (int y = y0; y < y1; y += 4) {
        int t = (y - y0) * 255 / h;
        unsigned int c = 0xFF000000;
        for (int k = 0; k < 3; k++) {
            int a = (top >> (8 * k)) & 255, b = (bot >> (8 * k)) & 255;
            c |= (unsigned)((a + (b - a) * t / 255) & 255) << (8 * k);
        }
        gfx_rect(0, y, SCR_W, (y + 4 <= y1) ? 4 : y1 - y, c);
    }
}

/* ---- text: bitmap font drawn as horizontal pixel runs (few sprites) ---- */
#define TEXT_MAX 48
static void text_raw(int x, int y, int s, unsigned int col, const char *str) {
    static Vtx tmp[4096];
    int n = 0;
    for (int ci = 0; str[ci] && ci < TEXT_MAX; ci++) {
        const unsigned char *g = &msx[(unsigned char)str[ci] * 8];
        int gx = x + ci * 8 * s;
        for (int r = 0; r < 8; r++) {
            int c = 0;
            while (c < 8) {
                if (g[r] & (0x80 >> c)) {
                    int st = c;
                    while (c < 8 && (g[r] & (0x80 >> c))) c++;
                    if (n + 2 <= 4096) {
                        tmp[n++] = (Vtx){col, gx + st * s + ox, y + r * s + oy, 0};
                        tmp[n++] = (Vtx){col, gx + c * s + ox, y + (r + 1) * s + oy, 0};
                    }
                } else c++;
            }
        }
    }
    if (!n) return;
    Vtx *v = (Vtx *)sceGuGetMemory(n * sizeof(Vtx));
    memcpy(v, tmp, n * sizeof(Vtx));
    sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, n, 0, v);
}

void gfx_text(int x, int y, int s, unsigned int col, const char *fmt, ...) {
    char b[64]; va_list ap;
    va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    text_raw(x, y, s, col, b);
}

void gfx_text_center(int y, int s, unsigned int col, const char *fmt, ...) {
    char b[64]; va_list ap;
    va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    int len = (int)strlen(b); if (len > TEXT_MAX) len = TEXT_MAX;
    int x = (SCR_W - len * 8 * s) / 2;
    text_raw(x + s, y + s, s, col & 0xFF000000, b);     /* drop shadow */
    text_raw(x, y, s, col, b);
}

/* ---- textured sprites ---- */
void gfx_tex(const Tex *t, int u, int v, int w, int h, int x, int y,
             int flip, unsigned int tint) {
    sceGuEnable(GU_TEXTURE_2D);
    sceGuClutMode(GU_PSM_8888, 0, 0xFF, 0);
    sceGuClut(t->clut, 32);                           /* 256 entries / 8 */
    sceGuTexMode(GU_PSM_T8, 0, 0, 0);
    sceGuTexImage(0, t->w, t->h, t->w, t->pixels);    /* power-of-2 sizes */
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    TVtx *vt = (TVtx *)sceGuGetMemory(2 * sizeof(TVtx));
    int x0 = x + ox, y0 = y + oy;
    int ua = flip ? u + w : u, ub = flip ? u : u + w;  /* mirror by swapping U */
    vt[0] = (TVtx){ua, v,     tint, x0,     y0,     0};
    vt[1] = (TVtx){ub, v + h, tint, x0 + w, y0 + h, 0};
    sceGuDrawArray(GU_SPRITES,
        GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vt);
    sceGuDisable(GU_TEXTURE_2D);
}

void gfx_char_frame(const CharDef *c, int frame, int x, int y, int facing,
                    unsigned int tint, int flash) {
    if (!c->texture) {                       /* fallback: colored box + nose */
        int bh = c->frame_h * 7 / 10;
        unsigned int col = flash ? COL_WHITE : tint;
        gfx_rect(x - 16, y - bh, 32, bh, col);
        gfx_rect(facing > 0 ? x + 4 : x - 16, y - bh + 12, 12, 6, COL_BLACK);
        return;
    }
    const Tex *t = (const Tex *)c->texture;
    int cols = t->w / c->frame_w; if (cols < 1) cols = 1;
    int u = (frame % cols) * c->frame_w, v = (frame / cols) * c->frame_h;
    int dx = facing > 0 ? x - c->pivot_x : x - (c->frame_w - c->pivot_x);
    gfx_tex(t, u, v, c->frame_w, c->frame_h, dx, y - c->pivot_y,
            facing < 0, flash ? 0xFFFFFFFF : 0xFFFFFFFF);
}

/* ---- stage background ---- */
void gfx_stage_bg(const StageDef *s, int cam_x) {
    int ground = s ? s->ground : 220;
    gfx_vgrad(0, ground, 0xFF602020, 0xFF7090F0);
    int textured = 0;
    if (s) {
        for (int i = 0; i < s->nlayers; i++) {
            const Tex *t = (const Tex *)s->layers[i].texture;
            if (!t) continue;
            textured = 1;
            int off = (int)(cam_x * s->layers[i].scroll) % t->w;
            for (int x = -off; x < SCR_W; x += t->w)
                gfx_tex(t, 0, 0, t->w, t->h > SCR_H ? SCR_H : t->h, x, 0, 0, 0xFFFFFFFF);
        }
    }
    if (!textured) {
        gfx_rect(0, ground, SCR_W, SCR_H - ground, 0xFF403030);
        gfx_rect(0, ground, SCR_W, 2, 0xFF807070);
    }
}
