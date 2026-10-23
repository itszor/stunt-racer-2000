# tools/cload: the DeComp module run by libroger (see cload.c).
ROGER   ?= $(HOME)/stuff/libroger
CC      ?= gcc

cload: cload.c $(ROGER)/libroger.a $(ROGER)/include/roger.h
	$(CC) -O2 -g -Wall -I$(ROGER)/include -o $@ cload.c $(ROGER)/libroger.a $(shell pkg-config --libs sdl3) -ljpeg -lm -lpthread

$(ROGER)/libroger.a: FORCE
	$(MAKE) -C $(ROGER) libroger.a

clean:
	rm -f cload

.PHONY: FORCE clean
