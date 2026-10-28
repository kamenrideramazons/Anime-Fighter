#include "screen.h"
#include "screenpack.h"
#include "game.h"
#include "gfx.h"
#include "input.h"
#include "ui.h"
#include <pspctrl.h>
#include <math.h>
#include <string.h>

#define BODY_W 32
#define MAX_SPARKS 8
#define GRAV 0.6f

typedef enum { ST_IDLE, ST_ATTACK, ST_HITSTUN, ST_BLOCKSTUN, ST_KO } FState;
typedef struct { unsigned int held, pressed; } Pad;

typedef struct {
    const CharDef *def;
    unsigned int color;
    float x, y, vx, vy, kb;
    int facing, st, frame, stun, cur_move, has_hit, blocking, flash;
    int hp, maxhp; float hp_shown;
    int anim_id, anim_st, anim_t, cur_frame;
} Fighter;

typedef struct { float x, y; int life, blocked; } Spark;
enum { P_INTRO, P_FIGHT, P_END };

static Fighter F[2];
static Spark sparks[MAX_SPARKS];
static const StageDef *stage;
static int ground, stage_w, cam_x;
static int phase, phase_t, hitstop, shake, ko_flash, tick, round_no;
static int timer, timed, paused, pause_sel, last_hit_t, ai_t, ai_act, end_ko;
static const char *pause_items[] = { "RESUME", "RESTART", "CHARACTER SELECT", "TITLE" };

/* ------------------------------------------------------------ helpers */
static int  training(void)            { return g_match.mode == MODE_TRAINING; }
static int  grounded(const Fighter *f){ return f->y >= ground; }
static int  body_h(const Fighter *f)  { return f->def->frame_h * 7 / 10; }
static const MoveDef *cur_move(const Fighter *f) { return &f->def->moves[f->cur_move]; }
static int  overlap(int ax,int ay,int aw,int ah,int bx,int by,int bw,int bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}
static void hit_rect(const Fighter *a, const MoveDef *m, int *x, int *y) {
    *x = a->facing > 0 ? (int)a->x + m->hb_x : (int)a->x - m->hb_x - m->hb_w;
    *y = (int)a->y + m->hb_y;
}
static int anim_index(const CharDef *d, const char *n) {
    for (int i = 0; i < d->nanims; i++) if (!strcmp(d->anims[i].name, n)) return i;
    return -1;
}

/* ------------------------------------------------------------ animation */
static void update_anim(Fighter *f) {
    const CharDef *d = f->def;
    int ai;
    switch (f->st) {
    case ST_ATTACK:    ai = d->nmoves ? cur_move(f)->anim : -1; break;
    case ST_HITSTUN:   ai = anim_index(d, "hit");   break;
    case ST_BLOCKSTUN: ai = anim_index(d, "block"); break;
    case ST_KO:        ai = anim_index(d, "ko");    break;
    default:           ai = anim_index(d, !grounded(f) ? "jump" : f->vx != 0 ? "walk" : "idle");
    }
    if (ai != f->anim_id || f->st != f->anim_st) { f->anim_id = ai; f->anim_st = f->st; f->anim_t = 0; }
    else f->anim_t++;
    f->cur_frame = 0;
    if (ai >= 0 && d->anims[ai].nframes > 0) {
        const Anim *a = &d->anims[ai];
        int step = f->anim_t / (a->speed > 0 ? a->speed : 1);
        if (a->loop) step %= a->nframes; else if (step >= a->nframes) step = a->nframes - 1;
        f->cur_frame = a->frames[step];
    }
}

/* ------------------------------------------------------------ fighters */
static void update_fighter(Fighter *f, Pad pad) {
    const CharDef *d = f->def;
    if (f->flash > 0) f->flash--;

    f->x += f->kb; f->kb *= 0.85f; if (fabsf(f->kb) < 0.2f) f->kb = 0;

    if (f->st == ST_KO) {
        f->vy += GRAV; f->y += f->vy;
        if (f->y >= ground) { f->y = ground; f->vy = 0; }
        update_anim(f);
        return;
    }
    if ((f->st == ST_HITSTUN || f->st == ST_BLOCKSTUN) && --f->stun <= 0) f->st = ST_IDLE;

    f->blocking = 0;
    if (f->st == ST_IDLE) {
        int back = f->facing > 0 ? PSP_CTRL_LEFT : PSP_CTRL_RIGHT;
        if (grounded(f)) {
            f->vx = 0;
            if (pad.held & PSP_CTRL_RIGHT) f->vx =  d->walk_speed;
            if (pad.held & PSP_CTRL_LEFT)  f->vx = -d->walk_speed;
            f->blocking = (pad.held & back) != 0;
            if (pad.held & PSP_CTRL_UP) f->vy = d->jump_vel;   /* keeps vx as air momentum */
        }
        for (int i = 0; i < d->nmoves; i++)
            if (pad.pressed & d->moves[i].button) {
                f->cur_move = i; f->st = ST_ATTACK; f->frame = 0; f->has_hit = 0;
                break;
            }
    } else if (f->st == ST_ATTACK) {
        const MoveDef *m = cur_move(f);
        if (grounded(f)) f->vx = 0;
        if (++f->frame > m->startup + m->active + m->recovery) f->st = ST_IDLE;
    } else if (grounded(f)) {
        f->vx = 0;
    }

    f->x += f->vx;
    f->vy += GRAV; f->y += f->vy;
    if (f->y >= ground) { f->y = ground; f->vy = 0; }
    update_anim(f);
}

static void spawn_spark(float x, float y, int blocked) {
    for (int i = 0; i < MAX_SPARKS; i++)
        if (sparks[i].life <= 0) { sparks[i] = (Spark){x, y, 8, blocked}; return; }
}

static void check_hit(Fighter *a, Fighter *d) {
    if (a->st != ST_ATTACK || a->has_hit || !a->def->nmoves || d->st == ST_KO) return;
    const MoveDef *m = cur_move(a);
    if (a->frame < m->startup || a->frame >= m->startup + m->active) return;
    int hx, hy; hit_rect(a, m, &hx, &hy);
    if (!overlap(hx, hy, m->hb_w, m->hb_h, (int)d->x - BODY_W / 2, (int)d->y - body_h(d), BODY_W, body_h(d)))
        return;

    a->has_hit = 1;
    hitstop = m->hitstop;
    last_hit_t = 0;
    int blocked = d->st == ST_BLOCKSTUN || (d->st == ST_IDLE && d->blocking);
    /* total slide distance is roughly 3.3 x pushback pixels */
    float kb = a->facing * m->pushback * 0.5f;
    spawn_spark(hx + m->hb_w / 2, hy + m->hb_h / 2, blocked);

    if (blocked) {
        d->st = ST_BLOCKSTUN; d->stun = m->blockstun; d->kb = kb * 0.8f;
        shake = 2;
    } else {
        d->hp -= m->damage;
        d->flash = 6;
        shake = 3 + m->damage / 4;
        if (training() && d->hp <= 0) d->hp = 1;          /* dummy can't die */
        if (d->hp <= 0) { d->st = ST_KO; d->kb = kb * 1.6f; d->vy = -6; }
        else            { d->st = ST_HITSTUN; d->stun = m->hitstun; d->kb = kb; }
    }
}

/* ------------------------------------------------------------ AI */
static Pad ai_pad(const Fighter *me, const Fighter *opp) {
    Pad p = {0, 0};
    if (training() || me->def->nmoves == 0) return p;          /* standing dummy */
    if (--ai_t <= 0) { ai_t = 15 + game_rand() % 30; ai_act = game_rand() % 6; }
    int toward = opp->x > me->x ? PSP_CTRL_RIGHT : PSP_CTRL_LEFT;
    int away   = opp->x > me->x ? PSP_CTRL_LEFT  : PSP_CTRL_RIGHT;
    if (fabsf(opp->x - me->x) > 90) { p.held |= toward; return p; }
    switch (ai_act) {
    case 0: case 1:
        if ((game_rand() & 7) == 0)
            p.pressed |= me->def->moves[game_rand() % me->def->nmoves].button;
        break;
    case 2: p.held |= away; break;      /* blocks */
    case 3: p.held |= toward; break;
    default: break;
    }
    return p;
}

/* ------------------------------------------------------------ rounds */
static void reset_round(void) {
    for (int i = 0; i < 2; i++) {
        Fighter *f = &F[i];
        f->x = stage_w / 2 + (i ? 90 : -90); f->y = ground;
        f->vx = f->vy = f->kb = 0;
        f->facing = i ? -1 : 1;
        f->st = ST_IDLE; f->frame = f->stun = f->cur_move = f->has_hit = f->blocking = f->flash = 0;
        f->hp = f->maxhp; f->hp_shown = (float)f->maxhp;
        f->anim_id = -2; f->anim_t = 0; f->cur_frame = 0;
    }
    timed = !training() && g_set.round_time > 0;
    timer = g_set.round_time * 60;
    hitstop = shake = ko_flash = last_hit_t = 0;
    memset(sparks, 0, sizeof sparks);
    phase = training() ? P_FIGHT : P_INTRO;
    phase_t = training() ? 99 : 0;
}

static void restart_match(void) {
    g_match.p1_rounds = g_match.p2_rounds = 0;
    round_no = 1;
    paused = 0;
    reset_round();
}

static void end_round(int ko) {
    int w;
    if      (F[0].hp <= 0 && F[1].hp <= 0) w = -1;
    else if (F[1].hp <= 0) w = 0;
    else if (F[0].hp <= 0) w = 1;
    else w = F[0].hp > F[1].hp ? 0 : F[1].hp > F[0].hp ? 1 : -1;     /* time over */
    if (w == 0) g_match.p1_rounds++;
    else if (w == 1) g_match.p2_rounds++;
    else { g_match.p1_rounds++; g_match.p2_rounds++; }
    end_ko = ko;
    phase = P_END; phase_t = 0;
    if (ko) { ko_flash = 10; hitstop = 18; shake = 10; }
}

static void finish_round(void) {
    int need = g_set.rounds_to_win;
    if (g_match.p1_rounds >= need || g_match.p2_rounds >= need) {
        g_match.winner = g_match.p1_rounds > g_match.p2_rounds ? 0 :
                         g_match.p2_rounds > g_match.p1_rounds ? 1 : -1;
        screen_go(SCR_RESULTS);
    } else {
        round_no++;
        reset_round();
    }
}

/* ------------------------------------------------------------ screen hooks */
static void enter(void) {
    int n = g_nchars;
    if (n == 0) { screen_go(SCR_TITLE); return; }
    if (g_match.p1_char >= n) g_match.p1_char = 0;
    if (g_match.p2_char >= n) g_match.p2_char = 0;

    CharDef *c1 = &g_chars[g_match.p1_char], *c2 = &g_chars[g_match.p2_char];
    char_assets_load(c1); char_assets_load(c2);
    F[0].def = c1; F[1].def = c2;
    F[0].color = c1->color;
    F[1].color = (g_match.p1_char == g_match.p2_char)       /* mirror match: invert P2 */
                 ? (0xFF000000 | (~c2->color & 0x00FFFFFF)) : c2->color;
    F[0].maxhp = c1->hp > 0 ? c1->hp : 1;
    F[1].maxhp = c2->hp > 0 ? c2->hp : 1;

    StageDef *s = (g_match.stage >= 0 && g_match.stage < g_nstages) ? &g_stages[g_match.stage] : NULL;
    stage_assets_load(s);
    stage = s;
    ground  = s ? s->ground : 220;
    stage_w = (s && s->width > SCR_W) ? s->width : SCR_W;
    ai_t = 0; tick = 0;
    restart_match();
}

static void leave(void) {
    if (!F[0].def) return;
    char_assets_free(&g_chars[g_match.p1_char]);
    char_assets_free(&g_chars[g_match.p2_char]);
    stage_assets_free(g_match.stage >= 0 && g_match.stage < g_nstages ? &g_stages[g_match.stage] : NULL);
    F[0].def = F[1].def = NULL;
}

static void separate_and_clamp(void) {
    Fighter *a = &F[0], *b = &F[1];
    float dx = b->x - a->x;
    if (fabsf(dx) < BODY_W && fabsf(a->y - b->y) < body_h(a) * 0.8f) {
        float push = (BODY_W - fabsf(dx)) / 2;
        float s = dx >= 0 ? 1.0f : -1.0f;
        a->x -= s * push; b->x += s * push;
    }
    for (int i = 0; i < 2; i++) {
        if (F[i].x < 20) F[i].x = 20;
        if (F[i].x > stage_w - 20) F[i].x = (float)(stage_w - 20);
    }
}

static void face(Fighter *f, const Fighter *o) {
    if (f->st == ST_IDLE && grounded(f)) f->facing = o->x >= f->x ? 1 : -1;
}

static void update(void) {
    Fighter *a = &F[0], *b = &F[1];
    if (!a->def) return;

    if (g_in.pressed & PSP_CTRL_START) { paused = !paused; pause_sel = 0; return; }
    if (paused) {
        if (g_in.pressed & PSP_CTRL_CIRCLE) paused = 0;
        if (ui_menu(&pause_sel, 4)) {
            switch (pause_sel) {
            case 0: paused = 0; break;
            case 1: restart_match(); break;
            case 2: screen_go(SCR_CHARSEL); break;
            case 3: screen_go(SCR_TITLE); break;
            }
        }
        return;
    }
    if (training() && (g_in.pressed & PSP_CTRL_SELECT)) reset_round();

    if (shake > 0) shake--;
    if (ko_flash > 0) ko_flash--;
    for (int i = 0; i < MAX_SPARKS; i++) if (sparks[i].life > 0) sparks[i].life--;
    phase_t++; tick++;

    if (phase == P_INTRO && phase_t >= 80) { phase = P_FIGHT; phase_t = 0; }

    if (hitstop > 0) {
        hitstop--;                                  /* freeze everything on impact */
    } else if (!(phase == P_END && phase_t < 60 && (phase_t & 1))) {   /* KO slow-mo */
        Pad p1 = {0, 0}, p2 = {0, 0};
        if (phase == P_FIGHT) {
            p1.held = g_in.held; p1.pressed = g_in.pressed;
            p2 = ai_pad(b, a);
        }
        face(a, b); face(b, a);
        update_fighter(a, p1);
        update_fighter(b, p2);
        separate_and_clamp();

        if (phase == P_FIGHT) {                     /* alternate priority so trades are fair */
            if (tick & 1) { check_hit(a, b); check_hit(b, a); }
            else          { check_hit(b, a); check_hit(a, b); }
        }
        for (int i = 0; i < 2; i++) {               /* lagging "damage trail" bar */
            Fighter *f = &F[i];
            if (f->hp_shown > f->hp) f->hp_shown -= f->maxhp * 0.004f;
            if (f->hp_shown < f->hp) f->hp_shown = (float)f->hp;
        }
        last_hit_t++;
        if (training() && last_hit_t > 90)          /* dummy heals when you stop */
            for (int i = 0; i < 2; i++) F[i].hp = F[i].hp + 6 > F[i].maxhp ? F[i].maxhp : F[i].hp + 6;

        if (phase == P_FIGHT) {
            if (a->hp <= 0 || b->hp <= 0) end_round(1);
            else if (timed && --timer <= 0) end_round(0);
        }
    }
    if (phase == P_END && phase_t >= 140) finish_round();

    cam_x = (int)((a->x + b->x) / 2) - SCR_W / 2;
    if (cam_x > stage_w - SCR_W) cam_x = stage_w - SCR_W;
    if (cam_x < 0) cam_x = 0;
}

/* ------------------------------------------------------------ drawing */
static void hud_bar(int x, int y, int w, int h, const Fighter *f, int rtl) {
    gfx_rect(x - 2, y - 2, w + 4, h + 4, COL_BLACK);
    gfx_rect(x, y, w, h, 0xFF303030);
    int ws = (int)(w * f->hp_shown / f->maxhp);
    int wh = f->hp > 0 ? w * f->hp / f->maxhp : 0;
    if (ws > w) ws = w;
    if (rtl) { gfx_rect(x + w - ws, y, ws, h, g_sp.h_trail); gfx_rect(x + w - wh, y, wh, h, g_sp.h_life); }
    else     { gfx_rect(x, y, ws, h, g_sp.h_trail);          gfx_rect(x, y, wh, h, g_sp.h_life); }
}

static void draw_sparks(void) {
    for (int i = 0; i < MAX_SPARKS; i++) {
        const Spark *s = &sparks[i];
        if (s->life <= 0) continue;
        int r = (9 - s->life) * 3 + 4;               /* grows as it fades */
        unsigned int c = s->blocked ? COL_CYAN : COL_YELLOW;
        int x = (int)s->x - cam_x, y = (int)s->y;
        gfx_rect(x - r, y - 1, r * 2, 3, c);
        gfx_rect(x - 1, y - r, 3, r * 2, c);
        gfx_rect(x - r / 2, y - r / 2, r, r, COL_WHITE);
    }
}

static void draw_hud(void) {
    const int w = g_sp.h_bar_w, h = g_sp.h_bar_h, y = g_sp.h_bar_y, m = g_sp.h_margin;
    const int x2 = SCR_W - m - w;
    hud_bar(m, y, w, h, &F[0], 0);
    hud_bar(x2, y, w, h, &F[1], 1);

    char nm[16]; strncpy(nm, F[1].def->name, 12); nm[12] = 0;
    gfx_text(m, y + h + 4, 1, g_sp.col_text, "%.12s", F[0].def->name);
    gfx_text(SCR_W - m - 8 * (int)strlen(nm), y + h + 4, 1, g_sp.col_text, "%s", nm);

    if (timed) gfx_text_center(g_sp.h_timer_y, g_sp.h_timer_scale, g_sp.col_text, "%02d", (timer + 59) / 60);
    else       gfx_text_center(g_sp.h_timer_y, g_sp.h_timer_scale, g_sp.col_dim,  "--");

    if (!training()) {
        if (g_sp.h_pips)
            for (int i = 0; i < g_set.rounds_to_win; i++) {
                gfx_rect(m + i * 12, y + h + 16, 8, 8, i < g_match.p1_rounds ? g_sp.col_accent : 0xFF404040);
                gfx_rect(SCR_W - m - 8 - i * 12, y + h + 16, 8, 8, i < g_match.p2_rounds ? g_sp.col_accent : 0xFF404040);
            }
    } else {
        gfx_text_center(y + h + 22, 1, g_sp.col_accent, "TRAINING   SELECT: RESET   START: MENU");
    }
}

static void draw(void) {
    if (!F[0].def) return;
    gfx_stage_bg(stage, cam_x);

    int sx = shake > 0 ? (int)(game_rand() % (shake * 2 + 1)) - shake : 0;
    int sy = shake > 0 ? (int)(game_rand() % (shake * 2 + 1)) - shake : 0;
    gfx_origin(sx, sy);

    for (int i = 0; i < 2; i++) {
        const Fighter *f = &F[i];
        gfx_char_frame(f->def, f->cur_frame, (int)f->x - cam_x, (int)f->y, f->facing, f->color, f->flash);
        if (g_set.show_hitboxes || training()) {
            gfx_outline((int)f->x - BODY_W / 2 - cam_x, (int)f->y - body_h(f), BODY_W, body_h(f), 0xFF00FF00);
            if (f->st == ST_ATTACK && f->def->nmoves) {
                const MoveDef *m = cur_move(f);
                if (f->frame >= m->startup && f->frame < m->startup + m->active) {
                    int hx, hy; hit_rect(f, m, &hx, &hy);
                    gfx_rect(hx - cam_x, hy, m->hb_w, m->hb_h, 0x800000FF);
                }
            }
        }
    }
    draw_sparks();
    gfx_origin(0, 0);

    draw_hud();

    if (phase == P_INTRO) {
        if (phase_t < 55) gfx_text_center(100, 4, g_sp.col_text, "%s %d", g_sp.h_round, round_no);
    } else if (phase == P_FIGHT && phase_t < 30 && !training()) {
        gfx_text_center(96, 5, g_sp.col_accent, "%s", g_sp.h_fight);
    } else if (phase == P_END) {
        gfx_text_center(96, 6, end_ko ? COL_RED : g_sp.col_text, "%s", end_ko ? g_sp.h_ko : g_sp.h_time);
    }
    if (ko_flash > 0) gfx_rect(0, 0, SCR_W, SCR_H, ((unsigned)(ko_flash * 25) << 24) | 0x00FFFFFF);

    if (paused) {
        gfx_rect(0, 0, SCR_W, SCR_H, 0xA0000000);
        gfx_text_center(70, 3, g_sp.col_text, "PAUSED");
        ui_draw_menu(120, pause_items, 4, pause_sel);
    }
}

const Screen scr_fight = { enter, update, draw, leave };
