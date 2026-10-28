#include "content.h"
#include <pspiofilemgr.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

CharDef  g_chars[MAX_CHARS];   int g_nchars;
StageDef g_stages[MAX_STAGES]; int g_nstages;

static const struct { const char *n; unsigned int b; } BTN[] = {
    {"SQUARE",PSP_CTRL_SQUARE},{"TRIANGLE",PSP_CTRL_TRIANGLE},
    {"CROSS",PSP_CTRL_CROSS},{"CIRCLE",PSP_CTRL_CIRCLE},
    {"L",PSP_CTRL_LTRIGGER},{"R",PSP_CTRL_RTRIGGER},{0,0}
};

static unsigned int parse_button(const char *s) {
    for (int i = 0; BTN[i].n; i++) if (!strcmp(s, BTN[i].n)) return BTN[i].b;
    return 0;
}

static int find_anim(CharDef *c, const char *n) {
    for (int i = 0; i < c->nanims; i++) if (!strcmp(c->anims[i].name, n)) return i;
    return -1;
}

static void trim(char *s) {
    char *e = s + strlen(s);
    while (e > s && (e[-1]=='\n'||e[-1]=='\r'||e[-1]==' ')) *--e = 0;
}

static void char_kv(CharDef *c, int sec, int idx, const char *k, const char *v) {
    if (sec == 0) {
        if      (!strcmp(k,"name"))       strncpy(c->name, v, 31);
        else if (!strcmp(k,"sprite"))     strncpy(c->sprite_file, v, 31);
        else if (!strcmp(k,"frame_w"))    c->frame_w = atoi(v);
        else if (!strcmp(k,"frame_h"))    c->frame_h = atoi(v);
        else if (!strcmp(k,"pivot_x"))    c->pivot_x = atoi(v);
        else if (!strcmp(k,"pivot_y"))    c->pivot_y = atoi(v);
        else if (!strcmp(k,"hp"))         c->hp = atoi(v);
        else if (!strcmp(k,"walk_speed")) c->walk_speed = atof(v);
        else if (!strcmp(k,"jump_vel"))   c->jump_vel = atof(v);
        else if (!strcmp(k,"color"))      c->color = 0xFF000000 | strtoul(v,0,16);
    } else if (sec == 1) {
        Anim *a = &c->anims[idx];
        if (!strcmp(k,"speed")) a->speed = atoi(v);
        else if (!strcmp(k,"loop")) a->loop = atoi(v);
        else if (!strcmp(k,"frames")) {
            char buf[96]; strncpy(buf, v, 95); buf[95] = 0;
            for (char *t = strtok(buf, ","); t && a->nframes < MAX_FRAMES; t = strtok(0, ","))
                a->frames[a->nframes++] = atoi(t);
        }
    } else if (sec == 2) {
        MoveDef *m = &c->moves[idx];
        if      (!strcmp(k,"input"))     m->button = parse_button(v);
        else if (!strcmp(k,"anim"))      m->anim = find_anim(c, v);
        else if (!strcmp(k,"startup"))   m->startup = atoi(v);
        else if (!strcmp(k,"active"))    m->active = atoi(v);
        else if (!strcmp(k,"recovery"))  m->recovery = atoi(v);
        else if (!strcmp(k,"damage"))    m->damage = atoi(v);
        else if (!strcmp(k,"hitstun"))   m->hitstun = atoi(v);
        else if (!strcmp(k,"blockstun")) m->blockstun = atoi(v);
        else if (!strcmp(k,"pushback"))  m->pushback = atoi(v);
        else if (!strcmp(k,"hitstop"))   m->hitstop = atoi(v);
        else if (!strcmp(k,"hitbox"))
            sscanf(v, "%hd,%hd,%hd,%hd", &m->hb_x,&m->hb_y,&m->hb_w,&m->hb_h);
    }
}

static int load_char(const char *dir, CharDef *c) {
    char path[128]; snprintf(path, sizeof path, "%s/char.ini", dir);
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    memset(c, 0, sizeof *c);
    strncpy(c->dir, dir, 63);
    c->frame_w = 64; c->frame_h = 96; c->pivot_x = 32; c->pivot_y = 96;
    c->hp = 1000; c->walk_speed = 2.5f; c->jump_vel = -9.0f; c->color = 0xFFFF8040;

    char line[160]; int sec = -1, idx = 0;
    while (fgets(line, sizeof line, f)) {
        trim(line);
        if (!line[0] || line[0]==';' || line[0]=='#') continue;
        if (line[0] == '[') {
            if (!strcmp(line,"[character]")) sec = 0;
            else if (!strncmp(line,"[anim:",6) && c->nanims < MAX_ANIMS) {
                sec = 1; idx = c->nanims++;
                strncpy(c->anims[idx].name, line+6, 23);
                c->anims[idx].name[strcspn(c->anims[idx].name,"]")] = 0;
                c->anims[idx].speed = 6;
            } else if (!strncmp(line,"[move:",6) && c->nmoves < MAX_MOVES) {
                sec = 2; idx = c->nmoves++;
                strncpy(c->moves[idx].name, line+6, 23);
                c->moves[idx].name[strcspn(c->moves[idx].name,"]")] = 0;
                c->moves[idx].anim = -1;
            } else sec = -1;
            continue;
        }
        char *eq = strchr(line, '=');
        if (!eq || sec < 0) continue;
        *eq = 0;
        char_kv(c, sec, idx, line, eq + 1);
    }
    fclose(f);
    return 1;
}

static int load_stage(const char *dir, StageDef *s) {
    char path[128]; snprintf(path, sizeof path, "%s/stage.ini", dir);
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    memset(s, 0, sizeof *s);
    strncpy(s->dir, dir, 63); s->ground = 220; s->width = 480;

    char line[160]; int sec = -1;
    while (fgets(line, sizeof line, f)) {
        trim(line);
        if (!line[0] || line[0]==';' || line[0]=='#') continue;
        if (line[0]=='[') {
            if (!strcmp(line,"[stage]")) sec = 0;
            else if (!strcmp(line,"[layer]") && s->nlayers < MAX_LAYERS) { sec = 1; s->nlayers++; }
            else sec = -1;
            continue;
        }
        char *eq = strchr(line,'='); if (!eq || sec < 0) continue;
        *eq = 0; const char *k = line, *v = eq + 1;
        if (sec == 0) {
            if (!strcmp(k,"name")) strncpy(s->name, v, 31);
            else if (!strcmp(k,"ground")) s->ground = atoi(v);
            else if (!strcmp(k,"width")) s->width = atoi(v);
        } else {
            int L = s->nlayers - 1;
            if (!strcmp(k,"file")) strncpy(s->layers[L].file, v, 31);
            else if (!strcmp(k,"scroll")) s->layers[L].scroll = atof(v);
        }
    }
    fclose(f);
    return 1;
}

static void scan(const char *root, int is_char) {
    SceUID d = sceIoDopen(root);
    if (d < 0) return;
    SceIoDirent e;
    memset(&e, 0, sizeof e);
    while (sceIoDread(d, &e) > 0) {
        /* skip '.' and '_' folders (e.g. _template) */
        if (FIO_S_ISDIR(e.d_stat.st_mode) && e.d_name[0] != '.' && e.d_name[0] != '_') {
            char sub[96]; snprintf(sub, sizeof sub, "%s/%s", root, e.d_name);
            if (is_char && g_nchars < MAX_CHARS) {
                if (load_char(sub, &g_chars[g_nchars])) g_nchars++;
            } else if (!is_char && g_nstages < MAX_STAGES) {
                if (load_stage(sub, &g_stages[g_nstages])) g_nstages++;
            }
        }
        memset(&e, 0, sizeof e);
    }
    sceIoDclose(d);
}

void content_scan(void) {
    g_nchars = g_nstages = 0;
    scan("data/chars", 1);
    scan("data/stages", 0);
}
