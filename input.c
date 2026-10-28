#include "input.h"
#include <pspctrl.h>

Input g_in;

void input_init(void) {
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

void input_update(void) {
    SceCtrlData p;
    sceCtrlPeekBufferPositive(&p, 1);
    unsigned int b = p.Buttons;
    /* analog nub doubles as a d-pad */
    if (p.Lx < 50)  b |= PSP_CTRL_LEFT;
    if (p.Lx > 205) b |= PSP_CTRL_RIGHT;
    if (p.Ly < 50)  b |= PSP_CTRL_UP;
    if (p.Ly > 205) b |= PSP_CTRL_DOWN;
    g_in.pressed  = b & ~g_in.held;
    g_in.released = g_in.held & ~b;
    g_in.held     = b;
}
