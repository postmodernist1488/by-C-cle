CC=cc
CFLAGS=-Wall -Wextra
BIN=bycicle errors.log

all: bycicle

bycicle: bycicle.c
	$(CC) $(CFLAGS) -o bycicle bycicle.c

clean:
	for file in $(BIN); do \
	if [ -e $$file ]; then rm $$file; fi;\
	done
