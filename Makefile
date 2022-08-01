CC=cc
CFLAGS=-Wall -Wextra -ggdb
ALL=bycicle

.PHONY: clean

all: bycicle

bycicle: bycicle.c
	$(CC) $(CFLAGS) -o bycicle bycicle.c

clean:
	rm bycicle

