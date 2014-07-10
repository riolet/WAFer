void read_and_discard_headers(int);
void print_standard_headers(int);
int get_line(int, char *, int);
void not_found(int);
void docwrite(int,const char*);
void headers(int);
char * getQueryPath(const char *);
char * getQueryParam(const char *, const char *);
void write_standard_headers(int);
void write_long_string(int,const char*);
void serve_file(int, const char *, const char *);


#define SERVER_STRING "Server: nope.chttpd/0.1.0\r\n"
#define ToHex(Y) (Y>='0'&&Y<='9'?Y-'0':Y-'A'+10)
#define UNDEFINED "VALUE_UNDEFINED"
