#ifndef GFX_H
#define GFX_H
#include "content.h"

#define SCR_W 480
#define SCR_H 272

/* colors are 0xAABBGGRR */
#define COL_WHITE  0xFFFFFFFF
#define COL_BLACK  0xFF000000
#define COL_GREY   0xFF909090
#define COL_YELLOW 0xFF00E0FF
#define COL_RED    0xFF3030FF
#define COL_CYAN   0xFFFFFF00

/* 8-bit indexed texture: pixels = w*h bytes, clut = 256 entries of 8888 */
typedef struct { unsigned char *pixels; unsigned int *clut; int w, h; } Tex;

void gfx_init(void);
void gfx_begin(unsigned int clear);
void gfx_end(void);                       /* flip + vsync => 60 FPS */

void gfx_origin(int ox, int oy);          /* offset for screen shake */
void gfx_rect(int x, int y, int w, int h, unsigned int col);
void gfx_outline(int x, int y, int w, int h, unsigned int col);
void gfx_vgrad(int y0, int y1, unsigned int top, unsigned int bottom);

void gfx_text(int x, int y, int scale, unsigned int col, const char *fmt, ...);
void gfx_text_center(int y, int scale, unsigned int col, const char *fmt, ...);

void gfx_tex(const Tex *t, int u, int v, int w, int h, int x, int y,
             int flip, unsigned int tint);
void gfx_char_frame(const CharDef *c, int frame, int x, int y, int facing,
                    unsigned int tint, int flash);
void gfx_stage_bg(const StageDef *s, int cam_x);   /* s == NULL => default */

#endif
