# Static recompilation of Stunt Racer 2000: the BASIC program (basic.c, from
# $(SS)/tools/bas2c.py) on bas.c, the machine code (gen/, from SuperSpider's
# analyse and gen via ../tools/images.py) and game.c, linked with libroger
# (the RISC OS runtime).
ROGER   ?= $(HOME)/stuff/libroger
SS      ?= $(HOME)/stuff/superspider
CC      ?= gcc
CFLAGS  ?= -O2 -g -w -fno-strict-aliasing $(if $(TRACE),-DRECOMP_TRACE)
GEN     := $(wildcard gen/code*.c) $(wildcard gen/entries.c)
OBJ     := $(GEN:.c=.o) basic.o bas.o game.o
LIBS    := $(shell pkg-config --libs sdl3) -lm -lpthread
INC     := -I$(ROGER)/include
PROG    := ../extracted/disc1/!SR2000/Code/3d_data,fffffd45-00000000

stuntracer: $(OBJ) $(ROGER)/libroger.a
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(ROGER)/libroger.a $(LIBS)

$(ROGER)/libroger.a: FORCE
	$(MAKE) -C $(ROGER) libroger.a

gen/%.o: gen/%.c $(ROGER)/include/roger_cpu.h gen/gen.h
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

basic.c: $(PROG) $(SS)/tools/bas2c.py
	python3 $(SS)/tools/bas2c.py '$(PROG)' $@

%.o: %.c bas.h $(ROGER)/include/roger.h
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

# make regen: unpack the CLOAD files, analyse and translate the machine code
regen:
	python3 ../tools/images.py unpack analyse gen

LOG ?= untranslated.log
roots:
	python3 ../tools/images.py roots $(LOG)

clean:
	rm -f gen/*.o *.o stuntracer

.PHONY: FORCE regen roots clean
