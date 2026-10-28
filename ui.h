#ifndef UI_H
#define UI_H
#include "gfx.h"
int  ui_menu(int *sel, int n);     /* up/down moves, returns 1 when X pressed */
void ui_draw_menu(int y, const char *const *items, int n, int sel);
void ui_draw_menu_ex(int y, const char *const *items, int n, int sel, int scale, int gap);
void ui_streaks(int t);            /* animated speed-line background (pack colors) */
void ui_bg(const Tex *bg, int t);  /* pack image if loaded, else streaks/gradient  */
#endif
