#include "screen.h"
#include "screenpack.h"
#include "game.h"
#include "gfx.h"
#include "input.h"
#include "ui.h"

static int sel, t;
static const char *items[] = { "REMATCH", "CHARACTER SELECT", "TITLE" };

static void enter(void) { sel = 0; t = 0; }

static void update(void) {
    t++;
    if (ui_menu(&sel, 3)) {
        switch (sel) {
        case 0: screen_go(SCR_FIGHT);   break;
        case 1: screen_go(SCR_CHARSEL); break;
        case 2: screen_go(SCR_TITLE);   break;
        }
    }
}

static void draw(void) {
    ui_bg(g_sp.bg[SP_BG_RESULTS], t);
    int w = g_match.winner;
    if (w < 0) {
        gfx_text_center(50, 4, g_sp.col_text, "DRAW");
    } else {
        const CharDef *c = &g_chars[w == 0 ? g_match.p1_char : g_match.p2_char];
        gfx_text_center(40, 2, g_sp.col_text, "%s", w == 0 ? "PLAYER 1" : "OPPONENT");
        gfx_text_center(70, 4, c->color, "%.10s", c->name);
        gfx_text_center(112, 3, g_sp.col_accent, "%s", g_sp.r_win);
    }
    gfx_text_center(150, 1, g_sp.col_dim, "ROUNDS  %d - %d", g_match.p1_rounds, g_match.p2_rounds);
    ui_draw_menu(176, items, 3, sel);
}

const Screen scr_results = { enter, update, draw, 0 };
