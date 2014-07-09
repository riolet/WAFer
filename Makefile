all: server

server: httpd.c
	gcc -W -Wall -pthread -o server httpd.c server.c

clean:
	rm server
