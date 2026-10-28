#ifndef GAME_H
#define GAME_H
#include "content.h"

typedef enum { MODE_VERSUS, MODE_TRAINING } GameMode;

/* Shared "blackboard" that screens use to pass choices to each other. */
typedef struct {
    GameMode mode;
    int p1_char, p2_char;     /* indices into g_chars */
    int stage;                /* index into g_stages, -1 = built-in default */
    int winner;               /* 0 = P1, 1 = P2, -1 = draw */
    int p1_rounds, p2_rounds;
} MatchSetup;

typedef struct {
    int rounds_to_win;        /* 1..5 */
    int round_time;           /* seconds, 0 = infinite */
    int show_hitboxes;
    int music_vol, sfx_vol;   /* 0..10 */
    char pack[32];            /* active screenpack folder name, "" = built-in */
} Settings;

extern MatchSetup g_match;
extern Settings   g_set;

unsigned game_rand(void);
void settings_load(void);
void settings_save(void);

/* assets.c: load/free textures for a fighter or stage while it is in a match */
void char_assets_load(CharDef *c);
void char_assets_free(CharDef *c);
void stage_assets_load(StageDef *s);
void stage_assets_free(StageDef *s);

#endif
