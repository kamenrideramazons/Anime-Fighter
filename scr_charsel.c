#include "screen.h"
#include "screenpack.h"
#include "game.h"
#include "gfx.h"
#include "input.h"
#include "ui.h"
#include <pspctrl.h>

static int sel, phase, scroll, t;   /* phase 0 = picking P1, 1 = picking opponent */

static void enter(void) { sel = 0; phase = 0; scroll = 0; t = 0; }

static void update(void) {
    t++;
    int n = g_nchars, COLS = g_sp.s_cols, ROWS = g_sp.s_rows;
    if (n == 0) {
        if (g_in.pressed & PSP_CTRL_CIRCLE) screen_go(SCR_TITLE);
        return;
    }
    if ((g_in.pressed & PSP_CTRL_LEFT)  && sel > 0)        sel--;
    if ((g_in.pressed & PSP_CTRL_RIGHT) && sel < n - 1)    sel++;
    if ((g_in.pressed & PSP_CTRL_UP)    && sel >= COLS)    sel -= COLS;
    if ((g_in.pressed & PSP_CTRL_DOWN)  && sel + COLS < n) sel += COLS;
    if (g_in.pressed & PSP_CTRL_RTRIGGER) sel = game_rand() % n;

    int row = sel / COLS;
    if (row < scroll) scroll = row;
    if (row >= scroll + ROWS) scroll = row - ROWS + 1;

    if (g_in.pressed & PSP_CTRL_CROSS) {
        if (phase == 0) {
            g_match.p1_char = sel;
            if (g_match.mode == MODE_TRAINING) { g_match.p2_char = sel; screen_go(SCR_STAGESEL); }
            else phase = 1;
        } else {
            g_match.p2_char = sel;
            screen_go(SCR_STAGESEL);
        }
    }
    if (g_in.pressed & PSP_CTRL_CIRCLE) {
        if (phase == 1) phase = 0; else screen_go(SCR_TITLE);
    }
}

static void draw(void) {
    ui_bg(g_sp.bg[SP_BG_SELECT], t);
    if (g_nchars == 0) {
        gfx_text_center(100, 2, COL_RED, "NO CHARACTERS FOUND");
        gfx_text_center(130, 1, g_sp.col_text, "ADD A FOLDER WITH char.ini TO data/chars/");
        gfx_text_center(150, 1, g_sp.col_dim, "O: BACK");
        return;
    }
    const int COLS = g_sp.s_cols, ROWS = g_sp.s_rows;
    const int CW = g_sp.s_cw, CH = g_sp.s_ch, GAP = g_sp.s_gap, Y0 = g_sp.s_grid_y;
    const int X0 = (SCR_W - (COLS * CW + (COLS - 1) * GAP)) / 2;
    const int chars_fit = CW / 8 > 1 ? CW / 8 - 1 : 1;

    gfx_text_center(12, 2, g_sp.col_text, "%s", phase == 0 ? g_sp.s_title1 : g_sp.s_title2);

    int n = g_nchars;
    for (int i = scroll * COLS; i < n && i < (scroll + ROWS) * COLS; i++) {
        int r = i / COLS - scroll, c = i % COLS;
        int x = X0 + c * (CW + GAP), y = Y0 + r * (CH + GAP);
        gfx_rect(x, y, CW, CH, g_chars[i].color);           /* TODO: portrait.png */
        gfx_rect(x, y + CH - 12, CW, 12, 0xA0000000);
        gfx_text(x + 2, y + CH - 10, 1, g_sp.col_text, "%.*s", chars_fit, g_chars[i].name);
        if (phase == 1 && i == g_match.p1_char) gfx_text(x + 2, y + 2, 1, g_sp.col_text, "1P");
        if (i == sel) {
            unsigned int c2 = (t / 6 & 1) ? g_sp.col_accent : g_sp.col_text;
            gfx_outline(x - 2, y - 2, CW + 4, CH + 4, c2);
            gfx_outline(x - 3, y - 3, CW + 6, CH + 6, c2);
        }
    }
    int grid_bottom = Y0 + ROWS * (CH + GAP);
    if (scroll > 0) gfx_text(SCR_W / 2 - 4, Y0 - 10, 1, g_sp.col_text, "^");
    if ((scroll + ROWS) * COLS < n) gfx_text(SCR_W / 2 - 4, grid_bottom - 6, 1, g_sp.col_text, "v");

    int py = grid_bottom + 4, ph = 262 - py;
    if (g_sp.s_info && ph >= 22) {
        const CharDef *d = &g_chars[sel];
        gfx_rect(28, py, 424, ph, 0xB0000000);
        if (ph >= 80) {
            gfx_text(40, py + 6, 3, d->color, "%.14s", d->name);
            gfx_text(40, py + 34, 1, g_sp.col_text, "HP %d   SPEED %.1f   JUMP %.0f", d->hp, d->walk_speed, -d->jump_vel);
            for (int i = 0; i < d->nmoves && i < 6; i++) {
                const MoveDef *m = &d->moves[i];
                gfx_text(40 + (i / 3) * 210, py + 50 + (i % 3) * 12, 1, g_sp.col_dim,
                         "%-8.8s DMG %-3d F%d", m->name, m->damage, m->startup);
            }
        } else {                                              /* compact one-liner */
            gfx_text(40, py + (ph - 8) / 2, 1, g_sp.col_text, "%.12s  HP %d  SPD %.1f  MOVES %d",
                     d->name, d->hp, d->walk_speed, d->nmoves);
        }
    }
    gfx_text(8, 262, 1, g_sp.col_dim, "X: OK  O: BACK  R: RANDOM");
}

const Screen scr_charsel = { enter, update, draw, 0 };
