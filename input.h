#ifndef INPUT_H
#define INPUT_H
typedef struct { unsigned int held, pressed, released; } Input;
extern Input g_in;               /* refreshed once per frame by screens_update() */
void input_init(void);
void input_update(void);
#endif
