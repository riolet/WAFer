#ifndef NOPE_H_
#define NOPE_H_

/* Define boolean */
typedef int bool;
#define true 1
#define false 0


#define SERVER_STRING "Server: nope.chttpd/0.1.0\r\n"
#define ToHex(Y) (Y>='0'&&Y<='9'?Y-'0':Y-'A'+10)
#define UNDEFINED "VALUE_UNDEFINED"

/* Settings */
#define MAX_HEADERS 1024
#define MAX_BUFFER_SIZE 1024
#define MAX_DPRINTF_SIZE 64
#define MAX_FD_SIZE 1024
#define MAX_METHOD_SIZE 64
#define MAX_VER_SIZE 64
#define MAX_REQUEST_SIZE 8192
#define MAX_EVENTS 1024

/* Define HTTP request parsing states */
#define STATE_PRE_REQUEST 0
#define STATE_METHOD 1
#define STATE_URI 2
#define STATE_VERSION 3
#define STATE_HEADER 4
#define STATE_COMPLETE_READING 5

#define STATUS_HTTP_OK 200


typedef struct struct_request {
	int client;
	char * reqStr;
	char * method;
	char **headers;
} Request;

void freeHeaders(char **);
long dbgprintf (const char *format, ...);

void server(Request request);

#endif /* NOPE_H_ */
