#include "screen.h"
#include "screenpack.h"
#include "game.h"
#include "gfx.h"
#include "input.h"
#include "ui.h"
#include <pspkernel.h>
#include <pspctrl.h>
#include <string.h>

static int sel, t, msg;

static void enter(void) { sel = 0; t = 0; msg = 0; }

static void run_action(const char *a) {
    if      (!strcmp(a, "versus"))   { g_match.mode = MODE_VERSUS;   screen_go(SCR_CHARSEL); }
    else if (!strcmp(a, "training")) { g_match.mode = MODE_TRAINING; screen_go(SCR_CHARSEL); }
    else if (!strcmp(a, "options"))  screen_go(SCR_OPTIONS);
    else if (!strcmp(a, "exit"))     sceKernelExitGame();
    /* unknown actions are ignored so a typo in a pack can't crash the game */
}

static void update(void) {
    t++;
    if (msg > 0) msg--;
    if (g_in.pressed & PSP_CTRL_SELECT) {          /* hot reload characters, stages AND skin */
        content_scan();
        sp_scan();
        sp_init(g_set.pack);
        sel = 0;
        msg = 120;
    }
    if (ui_menu(&sel, g_sp.t_nitems)) run_action(g_sp.t_items[sel].action);
}

static void draw(void) {
    ui_bg(g_sp.bg[SP_BG_TITLE], t);
    int y = g_sp.t_logo_y, s = g_sp.t_logo_scale;
    if (g_sp.t_line1[0]) { gfx_text_center(y, s, g_sp.col_text, "%s", g_sp.t_line1); y += s * 8 + 4; }
    if (g_sp.t_line2[0]) gfx_text_center(y, s, g_sp.col_accent, "%s", g_sp.t_line2);

    const char *labels[8];
    for (int i = 0; i < g_sp.t_nitems; i++) labels[i] = g_sp.t_items[i].label;
    ui_draw_menu_ex(g_sp.t_menu_y, labels, g_sp.t_nitems, sel, g_sp.t_menu_scale, g_sp.t_menu_gap);

    gfx_text(8, 258, 1, g_sp.col_dim, "%d CHARACTERS  %d STAGES  SKIN: %.12s   SELECT: RELOAD",
             g_nchars, g_nstages, g_sp.name);
    if (msg > 0) gfx_text_center(120, 1, g_sp.col_accent, "CONTENT RELOADED");
}

const Screen scr_title = { enter, update, draw, 0 };
