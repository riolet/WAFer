#ifndef NOPEUTILS_H_
#define NOPEUTILS_H_

/* ENTL */
#define STR(X) #X
#define WSPC " "
#define CRLF "\r\n"
#define ATTR(key,value) STR(key) STR(=) STR(value)

#define LT(tag) STR(<) STR(tag) WSPC	/*<tag */
#define LTA(tag,attributes) LT(tag) STR(attributes) WSPC	/*<tag */
#define GT STR(>)						/*<*/


#define OTAG(tag) LT(tag) GT	/*<tag>*/
#define OTAGA(tag,attributes) LTA(tag,attributes) GT	/*<tag attributes>*/
#define CTAG(tag) STR(</) STR(tag) GT	/*</tag>*/

#define ESTAG(tag) LT(tag) WSPC STR(/) GT	/*<tag attributes /> */
#define STAG(tag,attributes) LTA(tag,attributes) WSPC STR(/) GT	/*<tag attributes /> */

#define QTAG(tag,text) OTAG(tag) text CTAG(tag)	/*<tag> text </tag>*/
#define QTAGA(tag,attributes,text) OTAGA(tag,attributes) text CTAG(tag)	/*<tag attributes> text </tag>*/

/* Extensions */
#define QLINK(url,text) QTAGA(a,href=url,text)
#define QLINKA(url,attributes,text) QTAGA(a,href=url attributes,text)
#define QIMG(srcurl) STAG(img,src=srcurl)
#define QIMGA(srcurl) STAG(img,src=srcurl attributes)
#define QBR ESTAG(br)
/* End ENTL */

#define MVHP_OPEN(l,c,t,h)  LT(!DOCTYPE html) GT CRLF\
							LT(html) STR(lang=l) GT CRLF\
									OTAG(head)\
										STAG(meta, charset=c) CRLF\
										QTAG(title,t) CRLF\
										h CRLF\
									CTAG(head) CRLF\
									OTAG(body) CRLF

#define MVHPEN8(t,h) MVHP_OPEN("en","utf-8",t,h)

#define HP_CLOSE CRLF CTAG(body) CRLF CTAG(html)

#define STATIC_SEND(_socket, _str, _flags) send(_socket, _str, sizeof(_str)-1, _flags)

#include <stdarg.h>
#include <stdlib.h>
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
ssize_t writeLongString(int,const char*, size_t);
void serveFile(int, const char *, const char *);
char * dupstr (const char *);

char * _hscan(int client, const char * reqStr, const char *msg,const char *inputstr);
char * _hscanIfEmpty(int client, const char * reqStr, const char *msg,const char * inputstr);

#define STAGPARAMQ(tag,attributes) LTA(tag,attributes) ATTR(name,STR(q)) WSPC STR(/) GT	/*<tag attributes name="q" /> */
#define QTAGAPARAMQ(tag,attributes,text) OTAGA(tag,attributes name=STR(q)) text CTAG(tag)	/*<tag attributes> text </tag>*/
#define HSCANIT(client,reqStr,msg) _hscan(client,reqStr,msg,STAGPARAMQ(input,type="text"))
#define HSCANITIE(client,reqStr,msg) _hscanIfEmpty(client,reqStr,msg,STAGPARAMQ(input,type="text"))
#define HSCAN(client,reqStr,msg,tag,attributes,text) _hscan(client,reqStr,msg,QTAGAPARAMQ(tag,attributes,text))

#define SERVER_STRING "Server: nope.chttpd/0.1.0\r\n"
#define ToHex(Y) (Y>='0'&&Y<='9'?Y-'0':Y-'A'+10)
#define UNDEFINED "VALUE_UNDEFINED"
#define MAX_HEADERS 1024
#define MAX_BUFFER_SIZE 1024
#define MAX_DPRINTF_SIZE 64
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
bool routef(Request request, const char * path, void (* function)(int, const char *, const char*));
bool routeh(Request request, const char * path);
bool routefh(Request request, const char * path, void (* function)(int, const char *, const char*));


#endif /* NOPEUTILS_H_ */
