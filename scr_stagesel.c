#include "screen.h"
#include "screenpack.h"
#include "game.h"
#include "gfx.h"
#include "input.h"
#include <pspctrl.h>

static int sel;   /* 0..g_nstages-1, g_nstages = RANDOM */

static void enter(void) { sel = 0; }

static void update(void) {
    int n = g_nstages;
    if (n == 0) {                       /* nothing installed: use built-in stage */
        g_match.stage = -1;
        screen_go(SCR_FIGHT);
        return;
    }
    if (g_in.pressed & PSP_CTRL_LEFT)  sel = (sel + n) % (n + 1);        /* n+1 slots */
    if (g_in.pressed & PSP_CTRL_RIGHT) sel = (sel + 1) % (n + 1);
    if (g_in.pressed & PSP_CTRL_CROSS) {
        g_match.stage = (sel == n) ? (int)(game_rand() % n) : sel;
        screen_go(SCR_FIGHT);
    }
    if (g_in.pressed & PSP_CTRL_CIRCLE) screen_go(SCR_CHARSEL);
}

static void draw(void) {
    int n = g_nstages;
    gfx_stage_bg(sel < n ? &g_stages[sel] : NULL, 0);   /* the stage IS the preview */
    gfx_rect(0, 200, SCR_W, 72, 0xC0000000);
    gfx_text_center(10, 2, g_sp.col_text, "%s", g_sp.st_title);
    if (sel < n) {
        gfx_text_center(210, 3, g_sp.col_accent, "< %.12s >", g_stages[sel].name);
        gfx_text_center(242, 1, g_sp.col_dim, "%d/%d  %d LAYERS  WIDTH %d",
                        sel + 1, n, g_stages[sel].nlayers, g_stages[sel].width);
    } else {
        gfx_text_center(210, 3, g_sp.col_accent, "< RANDOM >");
    }
    gfx_text(8, 262, 1, g_sp.col_dim, "X: FIGHT  O: BACK");
}

const Screen scr_stagesel = { enter, update, draw, 0 };
