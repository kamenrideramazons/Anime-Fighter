/* Texture hooks. The screens already call these at the right moments
   (fight enter/leave), so adding the PNG loader later means filling in
   the TODOs here and nothing else changes. Until then everything falls
   back to colored boxes / gradients. */
#include "game.h"
#include "gfx.h"
#include "screenpack.h"
#include <stdlib.h>

static void tex_free(void **slot) {
    Tex *t = (Tex *)*slot;
    if (!t) return;
    free(t->pixels); free(t->clut); free(t);
    *slot = NULL;
}

void char_assets_load(CharDef *c) {
    if (c->texture) return;
    /* TODO: load c->dir "/" c->sprite_file (8-bit indexed PNG) into a Tex
       (memalign 16, sceKernelDcacheWritebackAll after filling) */
}
void char_assets_free(CharDef *c) { tex_free(&c->texture); }

void stage_assets_load(StageDef *s) {
    if (!s) return;
    /* TODO: for each layer, load s->dir "/" layers[i].file into layers[i].texture */
}
void stage_assets_free(StageDef *s) {
    if (!s) return;
    for (int i = 0; i < s->nlayers; i++) tex_free(&s->layers[i].texture);
}

/* Generic PNG -> Tex loader used by screenpack backgrounds (and later portraits).
   TODO: libpng, convert/keep 8-bit indexed, memalign(16) pixels + clut,
   sceKernelDcacheWritebackAll(). Returning NULL makes the pack fall back
   to its procedural background, so nothing breaks in the meantime. */
Tex *tex_load_png(const char *path) { (void)path; return NULL; }

void tex_free_png(Tex *t) {
    if (!t) return;
    free(t->pixels); free(t->clut); free(t);
}
