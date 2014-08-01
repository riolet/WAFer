#ifndef NOPEUTILS_H_
#define NOPEUTILS_H_

#include <stdarg.h>
#include <stdlib.h>
#include "nope.h"

#define NOPE_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define NOPE_MIN(x, y) (((x) < (y)) ? (x) : (y))

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

#define FDPRINTF nprintf
#define FDPRINT(_socket, _str) STATIC_SEND(_socket, _str,0)

bool stringEqualsLitLen(const char *varStr,const char *litStr,int litStrLen);
#define STREQLIT(var,lit) stringEqualsLitLen(var,lit,sizeof lit)


#include <stdarg.h>
#include <stdlib.h>
#ifdef __APPLE__
	#include <sys/uio.h>
#endif
char ** readHeaders(int);
int getLine(int, char *, int);
void not_found(int);
void docwrite(int,const char*);
long nprintf (int, const char *, ...);
char ** sendAndReceiveHeaders(int);
char * getQueryPath(const char *);
char * getQueryParam(const char *, const char *);
void writeStandardHeaders(int);
ssize_t writeLongString(int,const char*, size_t);
void serveFile(int, const char *, const char *);
char * dupstr (const char *);

void unimplemented(int client);
void not_found(int client);

char * _hscan(int client, const char * reqStr, const char *msg,const char *inputstr);
char * _hscanIfEmpty(int client, const char * reqStr, const char *msg,const char * inputstr);

#define STAGPARAMQ(tag,attributes) LTA(tag,attributes) ATTR(name,STR(q)) WSPC STR(/) GT	/*<tag attributes name="q" /> */
#define QTAGAPARAMQ(tag,attributes,text) OTAGA(tag,attributes name=STR(q)) text CTAG(tag)	/*<tag attributes> text </tag>*/
#define HSCANIT(client,reqStr,msg) _hscan(client,reqStr,msg,STAGPARAMQ(input,type="text"))
#define HSCANITIE(client,reqStr,msg) _hscanIfEmpty(client,reqStr,msg,STAGPARAMQ(input,type="text"))
#define HSCAN(client,reqStr,msg,tag,attributes,text) _hscan(client,reqStr,msg,QTAGAPARAMQ(tag,attributes,text))


/* Deprecated */
bool route(Request request, const char * path);
bool routef(Request request, const char * path, void (* function)(int, const char *, const char*));
bool routeh(Request request, const char * path);
bool routefh(Request request, const char * path, void (* function)(int, const char *, const char*));
/* End deprecated */

bool nope_route(Request request, const char * path, void (* function)(Request),bool send_headers);

#define ROUTE_INLINE(request,path)	nope_route(request, path, NULL,false)
#define ROUTE_FUNCTION(request,path,function)	nope_route(request, path, function,false)
#define ROUTE_INLINE_HEADERS(request,path)	nope_route(request, path, NULL,true)
#define ROUTE_FUNCTION_HEADERS(request,path,function)	nope_route(request, path, function,true)

#define MIME_TEXT_HTML "text/html"
#define ENCODING_GZIP "gzip"

#define SEND_TEXT_HTML_HEADER(request) sendHeadersTypeEncoding(request,MIME_TEXT_HTML, NULL)

#endif /* NOPEUTILS_H_ */
