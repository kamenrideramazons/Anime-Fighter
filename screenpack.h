#ifndef SCREENPACK_H
#define SCREENPACK_H
#include "gfx.h"

/* A screenpack = everything that decides how the menus and HUD look.
   Built-in defaults live in sp_defaults(); a pack folder in
   data/screenpacks/<name>/system.ini overrides any subset of them. */

enum { SP_BG_TITLE, SP_BG_SELECT, SP_BG_RESULTS, SP_BG_OPTIONS, SP_BG_COUNT };

typedef struct { char label[24]; char action[12]; } SpItem;

typedef struct {
    char name[32], dir[64];

    /* [general] / [colors] */
    int streaks;
    unsigned int col_accent, col_text, col_dim, col_bg_top, col_bg_bot;

    /* [title] */
    char t_line1[16], t_line2[16];
    int  t_logo_y, t_logo_scale, t_menu_y, t_menu_gap, t_menu_scale;
    SpItem t_items[8]; int t_nitems;

    /* [select] */
    int  s_cols, s_rows, s_cw, s_ch, s_gap, s_grid_y, s_info;
    char s_title1[24], s_title2[24];

    /* [stage] */
    char st_title[24];

    /* [hud] */
    int  h_bar_w, h_bar_h, h_bar_y, h_margin, h_timer_y, h_timer_scale, h_pips;
    unsigned int h_life, h_trail;
    char h_round[12], h_fight[12], h_ko[12], h_time[12];

    /* [results] */
    char r_win[16];

    /* backgrounds (optional images, 480x272 crop of a power-of-2 8-bit PNG) */
    char bg_file[SP_BG_COUNT][32];
    Tex *bg[SP_BG_COUNT];
} Screenpack;

extern Screenpack g_sp;

void sp_scan(void);                       /* find packs in data/screenpacks/ */
void sp_init(const char *saved_name);     /* pick the one saved in settings   */
int  sp_count(void);
const char *sp_name_at(int i);
int  sp_current(void);
void sp_select(int idx);                  /* loads it and records it in g_set */

/* assets.c */
Tex *tex_load_png(const char *path);      /* NULL for now (TODO) */
void tex_free_png(Tex *t);

#endif
