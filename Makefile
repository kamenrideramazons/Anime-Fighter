TARGET = anime_fighter
OBJS = src/main.o src/gfx.o src/input.o src/screen.o src/ui.o src/game.o \
       src/assets.o src/content.o src/screenpack.o \
       src/scr_title.o src/scr_charsel.o src/scr_stagesel.o \
       src/scr_fight.o src/scr_results.o src/scr_options.o

CFLAGS = -O2 -G0 -Wall -std=gnu99
LIBS = -lpspgu -lpspdisplay -lpspctrl -lpspdebug -lm
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Anime Fighter
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
