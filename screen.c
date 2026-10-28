#include "screen.h"
#include "input.h"
#include "gfx.h"

enum { PH_RUN, PH_OUT, PH_IN };
#define FADE_STEP 28   /* ~9 frames each way */

static const Screen *tbl[SCR_COUNT];
static int cur = -1, pending = -1, phase = PH_OUT, fade = 255;

void screens_init(void) {
    tbl[SCR_TITLE]    = &scr_title;
    tbl[SCR_CHARSEL]  = &scr_charsel;
    tbl[SCR_STAGESEL] = &scr_stagesel;
    tbl[SCR_FIGHT]    = &scr_fight;
    tbl[SCR_RESULTS]  = &scr_results;
    tbl[SCR_OPTIONS]  = &scr_options;
    cur = -1; pending = -1; phase = PH_OUT; fade = 255;
}

void screen_go(ScreenId id) {
    if (pending >= 0) return;          /* already transitioning */
    pending = id;
    phase = PH_OUT;
}

void screens_update(void) {
    input_update();

    if (phase == PH_OUT) {
        fade += FADE_STEP;
        if (fade >= 255) {
            fade = 255;
            if (cur >= 0 && tbl[cur]->leave) tbl[cur]->leave();
            cur = pending; pending = -1;
            if (tbl[cur]->enter) tbl[cur]->enter();
            phase = PH_IN;
        }
        return;                         /* frozen while fading out */
    }
    if (phase == PH_IN) {
        fade -= FADE_STEP;
        if (fade <= 0) { fade = 0; phase = PH_RUN; }
    }
    if (cur >= 0 && tbl[cur]->update) tbl[cur]->update();
}

void screens_draw(void) {
    if (cur >= 0 && tbl[cur]->draw) tbl[cur]->draw();
    if (fade > 0) gfx_rect(0, 0, SCR_W, SCR_H, ((unsigned)fade << 24));
}
