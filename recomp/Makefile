# Static recompilation of Stunt Racer 2000: the BASIC program (basic.c, from
# $(SS)/tools/bas2c.py, on the BASIC runtime $(SS)/runtime/basic_runtime.c),
# the machine code (gen/, from SuperSpider's analyse and gen via
# ../tools/images.py) and game.c, linked with libroger (the RISC OS runtime).
ROGER   ?= $(HOME)/stuff/libroger
SS      ?= $(HOME)/stuff/superspider
CC      ?= gcc
CFLAGS  ?= -O2 -g -w -fno-strict-aliasing $(if $(TRACE),-DRECOMP_TRACE)
GEN     := $(wildcard gen/*/code*.c) $(wildcard gen/*/entries.c)
OBJ     := $(GEN:.c=.o) basic.o basic_runtime.o game.o
LIBS    := $(shell pkg-config --libs sdl3) -ljpeg -lm -lpthread
INC     := -I$(ROGER)/include
PROG    := ../extracted/disc1/!SR2000/Code/3d_data,fffffd45-ba1eda72

stuntracer: $(OBJ) $(ROGER)/libroger.a
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(ROGER)/libroger.a $(LIBS)

$(ROGER)/libroger.a: FORCE
	$(MAKE) -C $(ROGER) libroger.a

gen/%.o: gen/%.c $(ROGER)/include/roger_cpu.h
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

basic.c: $(PROG) $(SS)/tools/bas2c.py
	python3 $(SS)/tools/bas2c.py '$(PROG)' $@

%.o: %.c $(SS)/runtime/basic_runtime.h $(ROGER)/include/roger.h
	$(CC) $(CFLAGS) $(INC) -I$(SS)/runtime -c -o $@ $<

basic_runtime.o: $(SS)/runtime/basic_runtime.c $(SS)/runtime/basic_runtime.h $(ROGER)/include/roger.h
	$(CC) $(CFLAGS) $(INC) -I$(SS)/runtime -c -o $@ $<

# make regen: unpack the CLOAD files, analyse and translate the machine code
regen:
	python3 ../tools/images.py unpack analyse gen

# make basic: the BASIC programs, detokenised into ../basic, to read
# (basic.c is the main one translated; game.c has Intro by hand)
basic:
	mkdir -p ../basic
	for p in Code/3d_data:3d_data Intro/Intro:Intro !Runimage:Runimage; do \
	    python3 -I $(SS)/tools/detok.py "$$(ls ../game/!SR2000/$${p%%:*},* | head -1)" > "../basic/$${p##*:}.bas"; done

LOG ?= untranslated.log
roots:
	python3 ../tools/images.py roots $(LOG)

clean:
	rm -f gen/*.o *.o stuntracer

.PHONY: basic FORCE regen roots clean

game.o: gen/modules.inc
