#ifndef SCREEN_H
#define SCREEN_H

/* Every screen in the game. Add a new one here, write a Screen struct
   for it, and register it in screens_init() (screen.c). */
typedef enum {
    SCR_TITLE, SCR_CHARSEL, SCR_STAGESEL, SCR_FIGHT, SCR_RESULTS, SCR_OPTIONS,
    SCR_COUNT
} ScreenId;

typedef struct {
    void (*enter)(void);    /* called once when the screen becomes active   */
    void (*update)(void);   /* one logic step, called at 60 Hz              */
    void (*draw)(void);     /* draw into the current frame                  */
    void (*leave)(void);    /* called once before the next screen's enter   */
} Screen;

extern const Screen scr_title, scr_charsel, scr_stagesel,
                    scr_fight, scr_results, scr_options;

void screens_init(void);
void screen_go(ScreenId id);   /* fade out, leave(), enter(), fade in */
void screens_update(void);
void screens_draw(void);

#endif
