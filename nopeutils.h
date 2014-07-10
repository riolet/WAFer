void read_and_discard_headers(int);
void print_standard_headers(int);
int get_line(int, char *, int);
void not_found(int);
void docwrite(int,const char*);
void headers(int);

#define SERVER_STRING "Server: nope.chttpd/0.1.0\r\n"
