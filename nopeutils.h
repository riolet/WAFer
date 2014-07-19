#ifndef NOPEUTILS_H_
#define NOPEUTILS_H_

#include <stdarg.h>
char ** readHeaders(int);
void freeHeaders(char **);
int getLine(int, char *, int);
void notFound(int);
void docwrite(int,const char*);
long nprintf (int, const char *, ...);
char ** sendAndReceiveHeaders(int);
char * getQueryPath(const char *);
char * getQueryParam(const char *, const char *);
void writeStandardHeaders(int);
long writeLongString(int,const char*);
void serveFile(int, const char *, const char *);
char * dupstr (const char *);

void mvhpen8(int client, const char * title, const char * head);
void mvhpOpen(int client, const char * language,const char * charset, const char * title, const char * head);
void hClose(int client);
void hto(int client, const char * tag, const char *attribs, ...);
void htc(int client, const char * tag);
void htoc(int client, const char *tag,const char *text,const char *attribs, ...);
void hts(int client, const char *tag,const char *attribs, ...);

int attrprintf(int client, const char *attribs, va_list args);
char * hscan(int client, const char * reqStr, const char *msg,...);

#define SERVER_STRING "Server: nope.chttpd/0.1.0\r\n"
#define ToHex(Y) (Y>='0'&&Y<='9'?Y-'0':Y-'A'+10)
#define UNDEFINED "VALUE_UNDEFINED"
#define MAX_HEADERS 1024
#define MAX_BUFFER_SIZE 1024
#define MAX_DPRINTF_SIZE 8
#define true 1
#define false 0

typedef int bool;

typedef struct struct_request Request;

struct struct_request {
	int client;
	const char * reqStr;
	const char * method;
};

bool route(Request request, const char * path);
bool routef(Request request, const char * path, void (* function)(int,char *, char*));
bool routeh(Request request, const char * path);
bool routefh(Request request, const char * path, void (* function)(int,char *, char*));


#endif /* NOPEUTILS_H_ */
