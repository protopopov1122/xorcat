CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -pedantic -O3 -march=native
XORCAT=xorcat

$(XORCAT): xorcat.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(XORCAT)

all: $(XORCAT)

.PHONY: all clean
