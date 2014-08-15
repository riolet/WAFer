long resPrintf(Request request, const char *format, ...);
void serveFile(Request request, const char *filename, const char *displayFilename,
                           const char *type);
char *getQueryParam(Request request, const char *name);
char *getQueryPath(Request request);
void sendHeadersTypeEncoding(Request request, const char *type, const char *encoding);
void sendResourceNotFound(Request request);

/*Internal stuff follows. Could change in future. Do not use */
#define STATIC_SEND(_socket, _str) send(_socket, _str, sizeof(_str)-1, 0)

/* ENTL */
#define STR(X) #X
#define WSPC " "
#define CRLF "\r\n"
#define ATTR(key,value) STR(key) STR(=) STR(value)

#define LT(tag) STR(<) STR(tag) WSPC    /*<tag */
#define LTA(tag,attributes) LT(tag) STR(attributes) WSPC        /*<tag */
#define GT STR(>)               /*< */

#define OTAG(tag) LT(tag) GT    /*<tag> */
#define OTAGA(tag,attributes) LTA(tag,attributes) GT    /*<tag attributes> */
#define CTAG(tag) STR(</) STR(tag) GT   /*</tag> */

#define ESTAG(tag) LT(tag) WSPC STR(/) GT       /*<tag attributes /> */
#define STAG(tag,attributes) LTA(tag,attributes) WSPC STR(/) GT /*<tag attributes /> */

#define QTAG(tag,text) OTAG(tag) text CTAG(tag) /*<tag> text </tag> */
#define QTAGA(tag,attributes,text) OTAGA(tag,attributes) text CTAG(tag) /*<tag attributes> text </tag> */

/* Extensions */
#define QLINK(url,text) QTAGA(a,href=url,text)
#define QLINKA(url,attributes,text) QTAGA(a,href=url attributes,text)
#define QIMG(srcurl) STAG(img,src=srcurl)
#define QIMGA(srcurl) STAG(img,src=srcurl attributes)
#define QBR ESTAG(br)
/* End ENTL */
