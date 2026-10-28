#include "screen.h"
#include "screenpack.h"
#include "game.h"
#include "gfx.h"
#include "input.h"
#include "ui.h"
#include <pspctrl.h>
#include <stdio.h>

static int sel, t;
enum { O_PACK, O_ROUNDS, O_TIME, O_HITBOX, O_MUSIC, O_SFX, O_BACK, O_COUNT };
static const int TIMES[] = { 0, 30, 60, 99 };

static void enter(void) { sel = 0; t = 0; }
static void leave(void) { settings_save(); }

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static void update(void) {
    t++;
    if (g_in.pressed & PSP_CTRL_UP)   sel = (sel + O_COUNT - 1) % O_COUNT;
    if (g_in.pressed & PSP_CTRL_DOWN) sel = (sel + 1) % O_COUNT;
    int d = ((g_in.pressed & PSP_CTRL_RIGHT) ? 1 : 0) - ((g_in.pressed & PSP_CTRL_LEFT) ? 1 : 0);
    if (d) {
        switch (sel) {
        case O_PACK: sp_select((sp_current() + d + sp_count()) % sp_count()); break;
        case O_ROUNDS: g_set.rounds_to_win = clampi(g_set.rounds_to_win + d, 1, 5); break;
        case O_TIME: {
            int i = 0;
            for (int k = 0; k < 4; k++) if (TIMES[k] == g_set.round_time) i = k;
            g_set.round_time = TIMES[(i + d + 4) % 4];
            break; }
        case O_HITBOX: g_set.show_hitboxes ^= 1; break;
        case O_MUSIC:  g_set.music_vol = clampi(g_set.music_vol + d, 0, 10); break;
        case O_SFX:    g_set.sfx_vol   = clampi(g_set.sfx_vol + d, 0, 10); break;
        }
    }
    if ((g_in.pressed & PSP_CTRL_CROSS) && sel == O_BACK) screen_go(SCR_TITLE);
    if (g_in.pressed & PSP_CTRL_CIRCLE) screen_go(SCR_TITLE);
}

static void row(int i, const char *label, const char *fmt, int v) {
    unsigned int col = i == sel ? g_sp.col_accent : g_sp.col_dim;
    int y = 52 + i * 26;
    gfx_text(110, y, 2, col, "%s", label);
    if (fmt) {
        char b[16];
        if (v == -1) snprintf(b, sizeof b, "INF"); else snprintf(b, sizeof b, fmt, v);
        gfx_text(310, y, 2, col, "%s", b);
    }
}

static void draw(void) {
    ui_bg(g_sp.bg[SP_BG_OPTIONS], t);
    gfx_text_center(12, 3, g_sp.col_text, "OPTIONS");
    row(O_PACK,   "SCREENPACK",    0, 0);
    gfx_text(310, 52 + O_PACK * 26, 2, sel == O_PACK ? g_sp.col_accent : g_sp.col_dim,
             "%.10s", sp_name_at(sp_current()));
    row(O_ROUNDS, "ROUNDS TO WIN", "%d", g_set.rounds_to_win);
    row(O_TIME,   "ROUND TIME",    "%d", g_set.round_time == 0 ? -1 : g_set.round_time);
    row(O_HITBOX, "SHOW HITBOXES", 0, 0);
    gfx_text(310, 52 + O_HITBOX * 26, 2, sel == O_HITBOX ? g_sp.col_accent : g_sp.col_dim,
             "%s", g_set.show_hitboxes ? "ON" : "OFF");
    row(O_MUSIC,  "MUSIC VOLUME",  "%d", g_set.music_vol);
    row(O_SFX,    "SFX VOLUME",    "%d", g_set.sfx_vol);
    row(O_BACK,   "BACK",          0, 0);
    gfx_text(8, 262, 1, g_sp.col_dim, "LEFT/RIGHT: CHANGE  O: BACK (SAVES)");
}

const Screen scr_options = { enter, update, draw, leave };
