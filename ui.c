#include "ui.h"
#include "screenpack.h"
#include "input.h"
#include <pspctrl.h>

int ui_menu(int *sel, int n) {
    if (g_in.pressed & PSP_CTRL_UP)   *sel = (*sel + n - 1) % n;
    if (g_in.pressed & PSP_CTRL_DOWN) *sel = (*sel + 1) % n;
    return (g_in.pressed & PSP_CTRL_CROSS) ? 1 : 0;
}

void ui_draw_menu_ex(int y, const char *const *items, int n, int sel, int scale, int gap) {
    for (int i = 0; i < n; i++)
        gfx_text_center(y + i * gap, scale, i == sel ? g_sp.col_accent : g_sp.col_dim,
                        "%s%s%s", i == sel ? "> " : "  ", items[i], i == sel ? " <" : "  ");
}

void ui_draw_menu(int y, const char *const *items, int n, int sel) {
    ui_draw_menu_ex(y, items, n, sel, 2, 24);
}

void ui_streaks(int t) {
    gfx_vgrad(0, SCR_H, g_sp.col_bg_top, g_sp.col_bg_bot);
    if (!g_sp.streaks) return;
    for (int i = 0; i < 18; i++) {
        int speed = 4 + (i % 5) * 3;
        int x = ((i * 97 + t * speed) % 640) - 120;
        int y = (i * 37) % SCR_H;
        gfx_rect(x, y, 70 + (i % 4) * 30, 1 + (i % 3), 0x30FFFFFF);
    }
}

void ui_bg(const Tex *bg, int t) {
    if (bg) {
        gfx_tex(bg, 0, 0, bg->w < SCR_W ? bg->w : SCR_W, bg->h < SCR_H ? bg->h : SCR_H, 0, 0, 0, 0xFFFFFFFF);
        return;
    }
    ui_streaks(t);
}
