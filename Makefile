CC = gcc
CFLAGS = -Wall -Wextra
PTHREAD = -pthread

all: server client

server: OLMS_Server.c
	$(CC) $(CFLAGS) OLMS_Server.c -o server $(PTHREAD)

client: OLMS_Client.c
	$(CC) $(CFLAGS) OLMS_Client.c -o client

clean:
	rm -f server client

run-server: server
	./server

run-client: client
	./client

.PHONY: all clean run-server run-client
