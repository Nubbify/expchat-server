CC = gcc
CFLAGS = -Wall -Iincludes -Wextra -std=c99 -ggdb 
VPATH = src


all: expserver

expserver: server.c errors.c utilities.c packet.c packet.h utilities.h errors.h
	$(CC) $(CFLAGS) errors.c utilities.c packet.c server.c -o expserver

clean:
	rm expserver

.PHONY: clean all
