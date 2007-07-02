PSPSDK=$(shell psp-config --pspsdk-path)

TARGET=sppsp
EXTRA_TARGETS=EBOOT.PBP
PSP_EBOOT_TITLE=SMS Plus PSP 1.2.1
#PSP_EBOOT_ICON=$(DATA)/vba-icon.png

DATA=./data
PSPLIB=./psplib

BUILD_Z80=cpu/z80.o
BUILD_SMS=sms.o	pio.o	memz80.o render.o	vdp.o	\
          system.o error.o fileio.o	state.o	loadrom.o
BUILD_MINIZIP=unzip/ioapi.o	unzip/unzip.o
BUILD_SOUND=sound/sound.o sound/sn76489.o sound/emu2413.o \
            sound/ym2413.o sound/fmintf.o sound/stream.o
BUILD_PSPLIB=$(PSPLIB)/psp.o $(PSPLIB)/font.o $(PSPLIB)/image.o \
             $(PSPLIB)/video.o $(PSPLIB)/audio.o $(PSPLIB)/fileio.o \
             $(PSPLIB)/menu.o $(PSPLIB)/ui.o $(PSPLIB)/ctrl.o \
             $(PSPLIB)/kybd.o $(PSPLIB)/perf.o
BUILD_SMSPLUS=psp/main.o

OBJS=$(BUILD_PSPLIB) $(BUILD_SOUND) $(BUILD_Z80) $(BUILD_MINIZIP) \
     $(BUILD_SMS) $(BUILD_SMSPLUS)

CFLAGS = -O2 -G0 -Wall -DLSB_FIRST -DPSP
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
INCDIR += $(PSPLIB) ./cpu ./sound ./unzip
LIBS= -lz -lm -lc -lpng -lpspgu -lpsppower -lpspaudio -lpsprtc

include $(PSPSDK)/lib/build.mak

#PSPLIB dependencies

$(PSPLIB)/audio.o:  $(PSPLIB)/audio.c $(PSPLIB)/audio.h
$(PSPLIB)/perf.o:   $(PSPLIB)/perf.c $(PSPLIB)/perf.h
$(PSPLIB)/ctrl.o:   $(PSPLIB)/ctrl.c $(PSPLIB)/ctrl.h
$(PSPLIB)/fileio.o: $(PSPLIB)/fileio.c $(PSPLIB)/fileio.h
$(PSPLIB)/font.o:   $(PSPLIB)/font.c $(PSPLIB)/font.h \
                    $(PSPLIB)/stockfont.h
$(PSPLIB)/image.o:  $(PSPLIB)/image.c $(PSPLIB)/image.h
$(PSPLIB)/kybd.o:   $(PSPLIB)/kybd.c $(PSPLIB)/kybd.h \
                    $(PSPLIB)/ctrl.c $(PSPLIB)/video.c \
                    $(PSPLIB)/image.c $(PSPLIB)/font.c
$(PSPLIB)/menu.o:   $(PSPLIB)/menu.c $(PSPLIB)/menu.h
$(PSPLIB)/psp.o:    $(PSPLIB)/psp.c $(PSPLIB)/psp.h \
                    $(PSPLIB)/fileio.c
$(PSPLIB)/ui.o:     $(PSPLIB)/ui.c $(PSPLIB)/ui.h \
                    $(PSPLIB)/video.c $(PSPLIB)/menu.c \
                    $(PSPLIB)/psp.c $(PSPLIB)/fileio.c \
                    $(PSPLIB)/ctrl.c
$(PSPLIB)/video.o:  $(PSPLIB)/video.c $(PSPLIB)/video.h \
                    $(PSPLIB)/font.c $(PSPLIB)/image.c
$(PSPLIB)/stockfont.h: $(DATA)/genfont $(PSPLIB)/stockfont.fd
	$< < $(word 2,$^) > $@
$(DATA)/genfont: $(PSPLIB)/genfont.c
	cc $< -o $@

