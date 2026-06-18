CC = gcc
CFLAGS = -Wall -Wextra

all: server client

server: server.c protocol.h
	$(CC) $(CFLAGS) -pthread server.c -o server

client: client.c protocol.h
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f server client

.PHONY: all clean
