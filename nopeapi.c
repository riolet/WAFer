#include "nope.h"
#include "nopeapi.h"

/* Ver 0.0.5 Functions
 * Parameters: request, format, (optional) variadargs
 * Returns: the number of characters printed
 */

/*Internal functions don't use */
ssize_t writeLongString(int client, const char *longString, size_t len);
static void cat(int client, FILE * pFile);
/*End internal functions */

long resPrintf(Request request, const char *format, ...)
{
    /* initial buffer large enough for most cases, will resize if required */
    char *buf = malloc(MAX_BUFFER_SIZE);
    int len;

    va_list arg;
    long done;
    va_start(arg, format);
    len = vsnprintf(buf, MAX_BUFFER_SIZE, format, arg);
    va_end(arg);

    if (len > MAX_BUFFER_SIZE) {
        /* buffer size was not enough */
        free(buf);
        buf = malloc(len + 1);
        if (buf == NULL) {
            printf("Could not allocate memory.");
            exit(EXIT_FAILURE);
        }
        va_start(arg, format);
        vsnprintf(buf, len + 1, format, arg);
        va_end(arg);
    }

    if (len < MAX_BUFFER_SIZE) {
        done = (int)send(request.client, buf, len, 0);
    } else {
        done = writeLongString(request.client, buf, len);
    }

    free(buf);
    return done;
}

/*Writes the given null-terminated string to the compressed file, excluding the terminating null character.
Returns the number of characters written, or -1 in case of error */
long resPuts(Request request, const char *buffer)
{
	send(request.client, buffer, strlen(buffer), 0);
}

/* Serve file. */
/* If displayFilename is not null, the file will be downloadable */
void serveFile(Request request, const char *filename, const char *displayFilename,
                           const char *type)
{
    FILE *resource = NULL;

    STATIC_SEND(request.client, "HTTP/1.0 200 OK\r\n");
    STATIC_SEND(request.client, SERVER_STRING);

    resPrintf(request, "Content-Type: %s\r\n", type);

    if (displayFilename!=NULL) {
    	resPrintf(request, "Content-Disposition: attachment; filename=\"%s\"\r\n",
             displayFilename);
    }

    STATIC_SEND(request.client, "\r\n");

    resource = fopen(filename, "r");
    if (resource == NULL) {
    	sendResourceNotFound(request);
    } else {
        cat(request.client, resource);
    }
    fclose(resource);
}

void sendHeadersTypeEncoding(Request request, const char *type, const char *encoding)
{
    STATIC_SEND(request.client, "HTTP/1.0 200 OK\r\n");
    STATIC_SEND(request.client, SERVER_STRING);
    resPrintf(request, "Content-Type: %s\r\n", type);
    if (encoding != NULL) {
        resPrintf(request, "Content-Encoding: %s\r\n", encoding);
    }
    STATIC_SEND(request.client, "Vary: Accept-Encoding\r\n");
    STATIC_SEND(request.client, "\r\n");
}

void sendResourceNotFound(Request request)
{
	STATIC_SEND(request.client, "HTTP/1.0 404 NOT FOUND\r\n");
	STATIC_SEND(request.client, "Content-Type: text/html\r\n");
	STATIC_SEND(request.client, "\r\n");
	STATIC_SEND(request.client, "<HTML><TITLE>Not Found</TITLE>\r\n");
	STATIC_SEND(request.client, "<BODY><P>The server could not fulfill\r\n");
	STATIC_SEND(request.client, "your request because the resource specified\r\n");
	STATIC_SEND(request.client, "is unavailable or nonexistent.\r\n");
	STATIC_SEND(request.client, "</P></BODY></HTML>\r\n");
}

/*Input Functions */
char *resQuickForm(Request request, const char *msg, const char *inputstr,bool onlyIfNull)
{
    char *qpath = getQueryPath(request);
    char *qparam = getQueryParam(request,"q");
    if (!(onlyIfNull&&qparam != NULL)) {
        resPrintf(request, OTAGA(form, action = "%s")
                 "%s%s" STAG(input, type = "submit") CTAG(form), qpath, msg, inputstr);
    }
    free(qpath);
    return qparam;
}

/* Utility Functions */
char *getHeader(char **headers, char *header)
{
    char *current_header, *matching_header, *value;
    // Not sure if MAX_BUFFER_SIZE is right.
    char *retval = malloc(MAX_BUFFER_SIZE * sizeof(char));
    if (retval == NULL) {
        printf("Could not allocate memory.");
        exit(EXIT_FAILURE);
    }
    int i;
    for (i = 0; headers[i] != NULL; i++) {
        current_header = headers[i];
        if ((matching_header = strstr(current_header, header))) {
            value = matching_header + strlen(header);
            if (*value == ':') {
                while (*value == ' ' || *value == ':') {
                    value++;
                }
                strcpy(retval, value);
                return retval;
            }
        }
    }
    memcpy(retval, UNDEFINED, sizeof(UNDEFINED));
    return retval;
}

/**********************************************************************/
/* Return the value of a query parameter.
 * Parameters: the query string
 *             the name of the parameter
 * Returns: the value of the query parameter */
/**********************************************************************/

char *getQueryParam(Request request, const char *name)
{
    char bufferAmpersand[MAX_BUFFER_SIZE];
    char bufferQuestion[MAX_BUFFER_SIZE];
    snprintf(bufferQuestion, MAX_BUFFER_SIZE - 1, "?%s=", name);
    snprintf(bufferAmpersand, MAX_BUFFER_SIZE - 1, "&%s=", name);
    char *buffer;
    char *pos1;

    char *value = malloc(MAX_BUFFER_SIZE * sizeof(char));
    if (value == NULL) {
        printf("Could not allocate memory.");
        exit(EXIT_FAILURE);
    }
    int i;

    buffer = bufferQuestion;
    pos1 = strstr(request.reqStr, bufferQuestion);
    dbgprintf("Buffer %s Pos %s\n", buffer, pos1);
    if (!pos1) {
        buffer = bufferAmpersand;
        pos1 = strstr(request.reqStr, bufferAmpersand);
    }
    if (pos1) {
        pos1 += strlen(buffer);
        i = 0;
        while (*pos1 && *pos1 != '&' && i < MAX_BUFFER_SIZE) {
            if (*pos1 == '%') {
                value[i] = (char)ToHex(pos1[1]) * 16 + ToHex(pos1[2]);
                pos1 += 3;
            } else if (*pos1 == '+') {
                value[i] = ' ';
                pos1++;
            } else {
                value[i] = *pos1++;
            }
            i++;
        }
        value[i] = '\0';
        return value;
    }

    free(value);
    value=NULL;
    return value;
}

/**********************************************************************/
/* Return the query path. For example the query path for
 * http://nopedotc.com/faq is /faq
 * Note the / at the beginning
 * Parameters: the request
 * Returns: the query path */
/**********************************************************************/

char *getQueryPath(Request request)
{
    char *queryPath;
    queryPath = strdup(request.reqStr);
    u_int i;
    for (i = 0; i < strlen(request.reqStr) && (queryPath[i] != '?') && (queryPath[i] != '\0');
         i++) {
    }

    queryPath[i] = '\0';

    return queryPath;
}

bool routeRequest(Request request, const char *path, void (*function) (Request),
                bool send_headers)
{
    char *queryPath = getQueryPath(request);
    if (strcmp(queryPath, path) == 0) {
        free(queryPath);
        if (send_headers)
        	sendHeadersTypeEncoding(request,"text/html",NULL);
        if (function != NULL)
            function(request);
        return true;
    } else {
        free(queryPath);
        return false;
    }
}

/*Internal stuff follows. Could change in future. Do not use */
static void cat(int client, FILE * pFile)
{
    char buf[1024];

    int result = fread(buf, 1, 1024, pFile);

    while (result != 0) {
        send(client, buf, result, 0);
        result = fread(buf, 1, 1024, pFile);
    }
}

ssize_t writeLongString(int client, const char *longString, size_t len)
{
    size_t remain = len;
    size_t sent = 0;

    while (remain) {
        size_t toCpy = remain > MAX_BUFFER_SIZE ? MAX_BUFFER_SIZE : remain;
        ssize_t sent_once = send(client, longString, toCpy, 0);
        if (sent_once <= 0)
            return -1;
        sent += sent_once;
        longString += sent_once;
        remain -= sent_once;
    }
    return sent;
}
