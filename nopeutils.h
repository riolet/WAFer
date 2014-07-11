char ** readHeaders(int);
int getLine(int, char *, int);
void notFound(int);
void docwrite(int,const char*);
long hprintf (int, const char *, ...);
char ** sendAndReceiveHeaders(int);
char * getQueryPath(const char *);
char * getQueryParam(const char *, const char *);
void writeStandardHeaders(int);
long writeLongString(int,const char*);
void serveFile(int, const char *, const char *);


#define SERVER_STRING "Server: nope.chttpd/0.1.0\r\n"
#define ToHex(Y) (Y>='0'&&Y<='9'?Y-'0':Y-'A'+10)
#define UNDEFINED "VALUE_UNDEFINED"
#define MAX_HEADERS 1024
#define MAX_BUFFER_SIZE 1024
#define MAX_DPRINTF_SIZE 8
