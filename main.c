#include <pspkernel.h>
#include "gfx.h"
#include "input.h"
#include "screen.h"
#include "content.h"
#include "game.h"
#include "screenpack.h"

PSP_MODULE_INFO("AnimeFighter", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static int exit_cb(int a, int b, void *c) { sceKernelExitGame(); return 0; }
static int cb_thread(SceSize args, void *argp) {
    int id = sceKernelCreateCallback("exit", exit_cb, NULL);
    sceKernelRegisterExitCallback(id);
    sceKernelSleepThreadCB();
    return 0;
}
static void setup_callbacks(void) {
    int t = sceKernelCreateThread("cb", cb_thread, 0x11, 0xFA0, 0, NULL);
    if (t >= 0) sceKernelStartThread(t, 0, NULL);
}

int main(void) {
    setup_callbacks();
    gfx_init();
    input_init();
    settings_load();
    content_scan();
    sp_scan();
    sp_init(g_set.pack);
    screens_init();
    screen_go(SCR_TITLE);

    while (1) {
        screens_update();
        gfx_begin(COL_BLACK);
        screens_draw();
        gfx_end();
    }
    return 0;
}
