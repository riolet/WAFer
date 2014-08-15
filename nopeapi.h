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

#define STAGPARAMQ(tag,attributes) LTA(tag,attributes) ATTR(name,STR(q)) WSPC STR(/) GT /*<tag attributes name="q" /> */
#define QTAGAPARAMQ(tag,attributes,text) OTAGA(tag,attributes name=STR(q)) text CTAG(tag)       /*<tag attributes> text </tag> */
/* End ENTL */

long resPrintf(Request request, const char *format, ...);
void serveFile(Request request, const char *filename, const char *displayFilename,
                           const char *type);

void sendHeadersTypeEncoding(Request request, const char *type, const char *encoding);
void sendResourceNotFound(Request request);

char *resQuickForm(Request request, const char *msg, const char *inputstr,bool onlyIfNull);
#define RES_QUICK_FORM_TEXT(request,msg,onlyIfNull) resQuickForm(request,msg,STAGPARAMQ(input,type="text"),onlyIfNull)

char *getQueryParam(Request request, const char *name);
char *getQueryPath(Request request);
bool routeRequest(Request request, const char *path, void (*function) (Request),
                bool send_headers);
/*Internal stuff follows. Could change in future. Do not use */
#define STATIC_SEND(_socket, _str) send(_socket, _str, sizeof(_str)-1, 0)
