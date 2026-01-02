# Makefile for CSE232 Project
CC = gcc
CFLAGS = -Wall -I.

all: assembler loader linker_exec

assembler: main.c parser.c pass1.c pass2.c tables.c
	$(CC) $(CFLAGS) -o assembler main.c parser.c pass1.c pass2.c tables.c

loader: loader_main.c loader.c tables.c
	$(CC) $(CFLAGS) -o loader loader_main.c loader.c tables.c

linker_exec: linker/linker.c
	$(CC) $(CFLAGS) -o linker_exec linker/linker.c

clean:
	rm -f assembler loader linker_exec *.o output.* exp.* test_main.o test_main.s test_main.t sub.o sub.s sub.t data.o data.s data.t

.PHONY: all clean
