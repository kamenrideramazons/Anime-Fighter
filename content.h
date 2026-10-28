#ifndef CONTENT_H
#define CONTENT_H

#define MAX_ANIMS   24
#define MAX_MOVES   16
#define MAX_FRAMES  16
#define MAX_CHARS   32
#define MAX_STAGES  16
#define MAX_LAYERS  4

typedef struct {
    char name[24];
    short frames[MAX_FRAMES];
    int   nframes, speed, loop;
} Anim;

typedef struct {
    char name[24];
    unsigned int button;           /* PSP_CTRL_* mask */
    int anim;                      /* index into anims, -1 = none */
    int startup, active, recovery;
    int damage, hitstun, blockstun, pushback, hitstop;
    short hb_x, hb_y, hb_w, hb_h;  /* relative to pivot, facing right */
} MoveDef;

typedef struct {
    char dir[64];
    char name[32];
    char sprite_file[32];
    int  frame_w, frame_h, pivot_x, pivot_y;
    int  hp;
    float walk_speed, jump_vel;
    unsigned int color;            /* fallback box color */
    Anim anims[MAX_ANIMS];    int nanims;
    MoveDef moves[MAX_MOVES]; int nmoves;
    void *texture;                 /* NULL until loaded */
} CharDef;

typedef struct {
    char dir[64], name[32];
    int ground, width;
    struct { char file[32]; float scroll; void *texture; } layers[MAX_LAYERS];
    int nlayers;
} StageDef;

extern CharDef  g_chars[MAX_CHARS];   extern int g_nchars;
extern StageDef g_stages[MAX_STAGES]; extern int g_nstages;

void content_scan(void);

#endif
