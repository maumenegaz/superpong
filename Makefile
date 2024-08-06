CC=gcc
CFLAGS= -W -Wall `sdl2-config --cflags --libs` -lSDL2_ttf -lm

super: super_pong.c
	$(CC) -o super_pong super_pong.c $(CFLAGS)

.PHONY: clean

clean:
	rm -f  *~ core super_pong
