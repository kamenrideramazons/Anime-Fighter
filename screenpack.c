#include "screenpack.h"
#include "game.h"
#include <pspiofilemgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PACKS 12

Screenpack g_sp;
static char names[MAX_PACKS][32], dirs[MAX_PACKS][64];
static int npacks, cur;

static void cp(char *dst, int n, const char *src) { strncpy(dst, src, n - 1); dst[n - 1] = 0; }

static unsigned int hexcol(const char *v) {            /* RRGGBB -> 0xAABBGGRR */
    unsigned int c = (unsigned int)strtoul(v, 0, 16);
    return 0xFF000000 | ((c & 0xFF) << 16) | (c & 0xFF00) | ((c >> 16) & 0xFF);
}

static void sp_defaults(Screenpack *p) {
    memset(p, 0, sizeof *p);
    cp(p->name, 32, "Built-in");
    p->streaks = 1;
    p->col_accent = 0xFF00E0FF; p->col_text = 0xFFFFFFFF; p->col_dim = 0xFF909090;
    p->col_bg_top = 0xFF301010; p->col_bg_bot = 0xFF603020;

    cp(p->t_line1, 16, "ANIME"); cp(p->t_line2, 16, "FIGHTER");
    p->t_logo_y = 28; p->t_logo_scale = 5;
    p->t_menu_y = 140; p->t_menu_gap = 24; p->t_menu_scale = 2;
    static const char *L[] = { "VERSUS CPU", "TRAINING", "OPTIONS", "EXIT" };
    static const char *A[] = { "versus", "training", "options", "exit" };
    for (int i = 0; i < 4; i++) { cp(p->t_items[i].label, 24, L[i]); cp(p->t_items[i].action, 12, A[i]); }
    p->t_nitems = 4;

    p->s_cols = 6; p->s_rows = 2; p->s_cw = 64; p->s_ch = 52; p->s_gap = 8; p->s_grid_y = 40; p->s_info = 1;
    cp(p->s_title1, 24, "SELECT YOUR FIGHTER"); cp(p->s_title2, 24, "SELECT OPPONENT");
    cp(p->st_title, 24, "SELECT STAGE");

    p->h_bar_w = 200; p->h_bar_h = 12; p->h_bar_y = 14; p->h_margin = 12;
    p->h_timer_y = 10; p->h_timer_scale = 2; p->h_pips = 1;
    p->h_life = 0xFF30FFA0; p->h_trail = 0xFF2060FF;
    cp(p->h_round, 12, "ROUND"); cp(p->h_fight, 12, "FIGHT!");
    cp(p->h_ko, 12, "K.O.");     cp(p->h_time, 12, "TIME UP");
    cp(p->r_win, 16, "WINS!");
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

#define S(k, f) if (!strcmp(key, k)) { cp(f, sizeof f, v); return; }
#define I(k, f) if (!strcmp(key, k)) { f = atoi(v); return; }
#define C(k, f) if (!strcmp(key, k)) { f = hexcol(v); return; }

static void kv(Screenpack *p, const char *sec, const char *key, const char *v) {
    if (!strcmp(sec, "info")) { S("name", p->name) }
    else if (!strcmp(sec, "general")) { I("streaks", p->streaks) }
    else if (!strcmp(sec, "colors")) {
        C("accent", p->col_accent) C("text", p->col_text) C("dim", p->col_dim)
        C("bg_top", p->col_bg_top) C("bg_bottom", p->col_bg_bot)
    } else if (!strcmp(sec, "title")) {
        S("bg", p->bg_file[SP_BG_TITLE]) S("logo_line1", p->t_line1) S("logo_line2", p->t_line2)
        I("logo_y", p->t_logo_y) I("logo_scale", p->t_logo_scale)
        I("menu_y", p->t_menu_y) I("menu_spacing", p->t_menu_gap) I("menu_scale", p->t_menu_scale)
        if (!strcmp(key, "item") && p->t_nitems < 8) {       /* item=LABEL,action */
            SpItem *it = &p->t_items[p->t_nitems];
            const char *comma = strrchr(v, ',');
            if (!comma) return;
            int n = (int)(comma - v); if (n > 23) n = 23;
            memcpy(it->label, v, n); it->label[n] = 0;
            cp(it->action, 12, comma + 1);
            p->t_nitems++;
        }
    } else if (!strcmp(sec, "select")) {
        S("bg", p->bg_file[SP_BG_SELECT]) S("title_p1", p->s_title1) S("title_p2", p->s_title2)
        I("cols", p->s_cols) I("rows", p->s_rows) I("cell_w", p->s_cw) I("cell_h", p->s_ch)
        I("gap", p->s_gap) I("grid_y", p->s_grid_y) I("show_info", p->s_info)
    } else if (!strcmp(sec, "stage")) { S("title", p->st_title) }
    else if (!strcmp(sec, "hud")) {
        I("bar_w", p->h_bar_w) I("bar_h", p->h_bar_h) I("bar_y", p->h_bar_y) I("margin", p->h_margin)
        I("timer_y", p->h_timer_y) I("timer_scale", p->h_timer_scale) I("pips", p->h_pips)
        C("life_color", p->h_life) C("trail_color", p->h_trail)
        S("round_text", p->h_round) S("fight_text", p->h_fight)
        S("ko_text", p->h_ko) S("time_text", p->h_time)
    } else if (!strcmp(sec, "results")) {
        S("bg", p->bg_file[SP_BG_RESULTS]) S("win_text", p->r_win)
    } else if (!strcmp(sec, "options")) {
        S("bg", p->bg_file[SP_BG_OPTIONS])
    }
}

static void sanitize(Screenpack *p) {
    p->s_cols = clampi(p->s_cols, 1, 10);   p->s_rows = clampi(p->s_rows, 1, 4);
    p->s_cw = clampi(p->s_cw, 16, 120);     p->s_ch = clampi(p->s_ch, 16, 100);
    p->s_gap = clampi(p->s_gap, 0, 20);     p->s_grid_y = clampi(p->s_grid_y, 20, 150);
    p->t_menu_scale = clampi(p->t_menu_scale, 1, 4);
    p->t_logo_scale = clampi(p->t_logo_scale, 1, 6);
    p->h_bar_w = clampi(p->h_bar_w, 60, 220);
    p->h_bar_h = clampi(p->h_bar_h, 4, 24);
    p->h_timer_scale = clampi(p->h_timer_scale, 1, 4);
    if (p->t_nitems == 0) {                 /* a pack with no items would be a dead end */
        cp(p->t_items[0].label, 24, "VERSUS CPU"); cp(p->t_items[0].action, 12, "versus");
        cp(p->t_items[1].label, 24, "EXIT");       cp(p->t_items[1].action, 12, "exit");
        p->t_nitems = 2;
    }
}

static void parse_file(Screenpack *p, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[160], sec[16] = "";
    int first_item = 1;
    while (fgets(line, sizeof line, f)) {
        char *e = line + strlen(line);
        while (e > line && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' ')) *--e = 0;
        if (!line[0] || line[0] == ';' || line[0] == '#') continue;
        if (line[0] == '[') {
            cp(sec, sizeof sec, line + 1);
            sec[strcspn(sec, "]")] = 0;
            continue;
        }
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        /* the first item= line in a pack replaces the built-in menu */
        if (!strcmp(sec, "title") && !strcmp(line, "item") && first_item) { p->t_nitems = 0; first_item = 0; }
        kv(p, sec, line, eq + 1);
    }
    fclose(f);
}

static void free_bgs(void) {
    for (int i = 0; i < SP_BG_COUNT; i++) { tex_free_png(g_sp.bg[i]); g_sp.bg[i] = NULL; }
}

void sp_scan(void) {
    npacks = 1; cur = 0;
    cp(names[0], 32, "Built-in"); dirs[0][0] = 0;
    SceUID d = sceIoDopen("data/screenpacks");
    if (d < 0) return;
    SceIoDirent e; memset(&e, 0, sizeof e);
    while (sceIoDread(d, &e) > 0) {
        if (FIO_S_ISDIR(e.d_stat.st_mode) && e.d_name[0] != '.' && e.d_name[0] != '_' && npacks < MAX_PACKS) {
            snprintf(dirs[npacks], 64, "data/screenpacks/%s", e.d_name);
            cp(names[npacks], 32, e.d_name);
            npacks++;
        }
        memset(&e, 0, sizeof e);
    }
    sceIoDclose(d);
}

void sp_select(int idx) {
    if (idx < 0 || idx >= npacks) idx = 0;
    free_bgs();
    sp_defaults(&g_sp);
    if (idx > 0) {
        char path[128];
        cp(g_sp.dir, 64, dirs[idx]);
        snprintf(path, sizeof path, "%s/system.ini", dirs[idx]);
        parse_file(&g_sp, path);
        for (int i = 0; i < SP_BG_COUNT; i++)
            if (g_sp.bg_file[i][0]) {
                snprintf(path, sizeof path, "%s/%s", dirs[idx], g_sp.bg_file[i]);
                g_sp.bg[i] = tex_load_png(path);          /* NULL => procedural bg */
            }
    }
    sanitize(&g_sp);
    cur = idx;
    cp(g_set.pack, sizeof g_set.pack, idx ? names[idx] : "");
}

void sp_init(const char *saved) {
    int idx = 0;
    for (int i = 1; i < npacks; i++) if (!strcmp(names[i], saved)) idx = i;
    sp_select(idx);
}

int sp_count(void) { return npacks; }
const char *sp_name_at(int i) { return names[i]; }
int sp_current(void) { return cur; }
