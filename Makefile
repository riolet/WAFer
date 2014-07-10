all: server

server: nope.c
	gcc -W -Wall -pthread -o server nopeutils.c nope.c server.c

clean:
	rm server
