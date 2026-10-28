#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MatchSetup g_match = { MODE_VERSUS, 0, 0, -1, -1, 0, 0 };
Settings   g_set   = { 2, 99, 0, 8, 8, "" };

unsigned game_rand(void) {            /* xorshift32 */
    static unsigned s = 2463534242u;
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return s;
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

void settings_load(void) {
    FILE *f = fopen("data/settings.ini", "r");
    if (!f) return;
    char line[64];
    while (fgets(line, sizeof line, f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        char *val = eq + 1;
        val[strcspn(val, "\r\n")] = 0;
        int v = atoi(val);
        if      (!strcmp(line, "rounds"))    g_set.rounds_to_win = clampi(v, 1, 5);
        else if (!strcmp(line, "time"))      g_set.round_time = clampi(v, 0, 99);
        else if (!strcmp(line, "hitboxes"))  g_set.show_hitboxes = v != 0;
        else if (!strcmp(line, "music"))     g_set.music_vol = clampi(v, 0, 10);
        else if (!strcmp(line, "sfx"))       g_set.sfx_vol = clampi(v, 0, 10);
        else if (!strcmp(line, "pack"))      { strncpy(g_set.pack, val, 31); g_set.pack[31] = 0; }
    }
    fclose(f);
}

void settings_save(void) {
    FILE *f = fopen("data/settings.ini", "w");
    if (!f) return;
    fprintf(f, "rounds=%d\ntime=%d\nhitboxes=%d\nmusic=%d\nsfx=%d\npack=%s\n",
            g_set.rounds_to_win, g_set.round_time, g_set.show_hitboxes,
            g_set.music_vol, g_set.sfx_vol, g_set.pack);
    fclose(f);
}
