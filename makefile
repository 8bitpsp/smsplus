# SMS Plus - Sega Master System / Game Gear emulator
# (c) 1998-2004  Charles MacDonald

# -DLSB_FIRST   - Leave undefined for big-endian processors.
# -DALIGN_DWORD - Align 32-bit memory transfers
# -DDOS		- Set when compiling the DOS version

CC	=	gcc
AS	=	nasm -f coff -O1
LDFLAGS	=
FLAGS	=	-I. -Icpu -Idos -Isound -Iunzip \
		-Werror -Wall \
		-DLSB_FIRST -DX86_ASM -DDOS \
		-O6 -mcpu=athlon -fomit-frame-pointer -ffast-math

LIBS	=	-lalleg -laudio -lz -lm

OBJ	=       obj/z80.oa	\
		obj/sms.o	\
		obj/pio.o	\
		obj/memz80.o	\
		obj/render.o	\
		obj/vdp.o	\
		obj/system.o	\
		obj/error.o
	        
OBJ	+=	obj/fileio.o	\
		obj/state.o	\
		obj/loadrom.o
	        
OBJ	+=	obj/ioapi.o	\
		obj/unzip.o

OBJ	+=      obj/sound.o	\
		obj/sn76489.o	\
		obj/emu2413.o	\
		obj/ym2413.o	\
		obj/fmintf.o	\
		obj/stream.o

OBJ	+=	obj/main.o	\
		obj/sealintf.o	\
		obj/config.o	\
		obj/expand.o	\
		obj/blur.o	\
		obj/blit.o	\
		obj/ui.o	\
		obj/video.o	\
		obj/input.o

EXE	=	sp.exe

all	:	$(EXE)

$(EXE)	:	$(OBJ)
		$(CC) -o $(EXE) $(OBJ) $(LIBS) $(LDFLAGS)
	        
obj/%.oa :	cpu/%.c cpu/%.h
		$(CC) -c $< -o $@ $(FLAGS)

obj/%.o : 	%.c %.h
		$(CC) -c $< -o $@ $(FLAGS)
	        
obj/%.o :	dos/%.s
		$(AS) $< -o $@
	        
obj/%.o :	sound/%.c sound/%.h	        
		$(CC) -c $< -o $@ $(FLAGS)
	        
obj/%.o	:	unzip/%.c unzip/%.h
		$(CC) -c $< -o $@ $(FLAGS)
	        
obj/%.o :	cpu/%.c cpu/%.h	        
		$(CC) -c $< -o $@ $(FLAGS)

obj/%.o :	dos/%.c dos/%.h	        
		$(CC) -c $< -o $@ $(FLAGS)
	        
pack	:
		strip $(EXE)
		upx -1 $(EXE)	        

clean	:	        
		rm -f obj/*.o
		rm -f *.bak
		rm -f *.exe
		rm -f *.log
		rm -f *.wav
		rm -f *.zip
cleancpu :		
		rm -f obj/z80.oa

makedir :
		mkdir obj
	        
archive:	        
		pk -dir -add -max \
		-excl=rom -excl=src -excl=test -excl=zip \
		-excl=obj -excl=doc -excl=bak -excl=out \
		mdsrc.zip *.*
	        
#
# end of makefile
#
