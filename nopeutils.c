/* This is a Swiss army knife of utilities that make
 * writing a network application a wee bit easier
 * Rohana Rezel
 * Riolet Corporation
 */

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdarg.h>
#include "nopeutils.h"
#include "nope.h"

/* Ver 0.0.5 Functions */
/* This is like dprintf on post 2008 POSIX
 * You can use FDPRINTF which might use dprintf if it is available
 * Parameters: the client, format
 * Returns: the number of characters printed
 */
long resprintf(Request request, const char *format, ...)
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

    /* printf("Buffer length %d",strlen(buf)); */
    if (len < MAX_BUFFER_SIZE) {
        done = (int)send(request.client, buf, len, 0);
    } else {
        done = writeLongString(request.client, buf, len);
    }

    free(buf);
    return done;
}

/* String functions */
bool stringEqualsLitLen(const char *varStr, const char *litStr, int litStrLen)
{
    int i;
    for (i = 0; i < litStrLen - 1; i++) {
        if (varStr[i] == 0)
            return false;
        if (varStr[i] != litStr[i])
            return false;
    }
    return true;
}

/**********************************************************************/
/* Return the value of a query parameter.
 * Parameters: the query string
 *             the name of the parameter
 * Returns: the value of the query parameter */
/**********************************************************************/

char *getQueryParam(const char *queryString, const char *name)
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
    pos1 = strstr(queryString, bufferQuestion);
    dbgprintf("Buffer %s Pos %s\n", buffer, pos1);
    if (!pos1) {
        buffer = bufferAmpersand;
        pos1 = strstr(queryString, bufferAmpersand);
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

    strcpy(value, UNDEFINED);
    return value;
}

/**********************************************************************/
/* Return the query path. For example the query path for
 * http://nopedotc.com/faq is /faq
 * Note the / at the beginning
 * Parameters: the query string
 * Returns: the query path */
/**********************************************************************/

char *getQueryPath(const char *reqString)
{
    char *queryPath;
    queryPath = strdup(reqString);
    u_int i;
    for (i = 0; i < strlen(reqString) && (queryPath[i] != '?') && (queryPath[i] != '\0');
         i++) {
    }

    queryPath[i] = '\0';

    return queryPath;
}

/* This is like dprintf on post 2008 POSIX
 * You can use FDPRINTF which might use dprintf if it is available
 * Parameters: the client, format
 * Returns: the number of characters printed
 */
long nprintf(int client, const char *format, ...)
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

    /* printf("Buffer length %d",strlen(buf)); */
    if (len < MAX_BUFFER_SIZE) {
        done = (int)send(client, buf, len, 0);
    } else {
        done = writeLongString(client, buf, len);
    }

    free(buf);
    return done;
}

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

void sendHeadersFromString(Request request, const char *headerString)
{
    int client = request.client;
    FDPRINTF(client, "%s", headerString, 0);
    STATIC_SEND(client, "\r\n", 0);
}

void sendHeadersTypeEncoding(Request request, const char *type, const char *encoding)
{
    int client = request.client;
    STATIC_SEND(client, "HTTP/1.0 200 OK\r\n", 0);
    STATIC_SEND(client, SERVER_STRING, 0);
    FDPRINTF(client, "Content-Type: %s\r\n", type, 0);
    if (encoding != NULL) {
        FDPRINTF(client, "Content-Encoding: %s\r\n", encoding, 0);
    }
    STATIC_SEND(client, "Vary: Accept-Encoding\r\n", 0);
    STATIC_SEND(client, "\r\n", 0);
}

/* Deprecated */
void writeStandardHeaders(int client)
{
    STATIC_SEND(client, "HTTP/1.1 200 OK\r\n", 0);
    STATIC_SEND(client, SERVER_STRING, 0);
    STATIC_SEND(client, "Content-Type: text/html\r\n", 0);
    STATIC_SEND(client, "Vary: Accept-Encoding\r\n", 0);
    STATIC_SEND(client, "\r\n", 0);
}

/*End deprecated */

void cat(int client, FILE * pFile)
{
    char buf[1024];

    int result = fread(buf, 1, 1024, pFile);

    while (result != 0) {
        send(client, buf, result, 0);
        result = fread(buf, 1, 1024, pFile);
    }
}

/* Serve downloadable file. */
void serveDownloadableFile(int client, const char *filename, const char *displayFilename,
                           const char *type)
{
    FILE *resource = NULL;

    STATIC_SEND(client, "HTTP/1.0 200 OK\r\n", 0);
    STATIC_SEND(client, SERVER_STRING, 0);

    FDPRINTF(client, "Content-Type: %s\r\n", type);
    FDPRINTF(client, "Content-Disposition: attachment; filename=\"%s\"\r\n",
             displayFilename);

    STATIC_SEND(client, "\r\n", 0);

    resource = fopen(filename, "r");
    if (resource == NULL) {
        not_found(client);
    } else {
        cat(client, resource);
    }
    fclose(resource);
}

/* Serve and entire file. */
void serveFile(int client, const char *filename, const char *type)
{
    FILE *resource = NULL;

    FDPRINT(client, "HTTP/1.0 200 OK\r\n");
    FDPRINT(client, SERVER_STRING);

    FDPRINTF(client, "Content-Type: %s\r\n", type);

    FDPRINT(client, "\r\n");

    resource = fopen(filename, "r");
    if (resource == NULL) {
        not_found(client);
    } else {
        cat(client, resource);
    }
    fclose(resource);
}

/* Write strings that are two big for our buffer */
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

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client)
{
    FDPRINT(client, "HTTP/1.0 501 Method Not Implemented\r\n");
    FDPRINT(client, SERVER_STRING);
    FDPRINT(client, "Content-Type: text/html\r\n");
    FDPRINT(client, "\r\n");
    FDPRINT(client, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    FDPRINT(client, "</TITLE></HEAD>\r\n");
    FDPRINT(client, "<BODY><P>HTTP request method not supported.\r\n");
    FDPRINT(client, "</P></BODY></HTML>\r\n");
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client)
{
    FDPRINT(client, "HTTP/1.0 404 NOT FOUND\r\n");
    FDPRINT(client, "Content-Type: text/html\r\n");
    FDPRINT(client, "\r\n");
    FDPRINT(client, "<HTML><TITLE>Not Found</TITLE>\r\n");
    FDPRINT(client, "<BODY><P>The server could not fulfill\r\n");
    FDPRINT(client, "your request because the resource specified\r\n");
    FDPRINT(client, "is unavailable or nonexistent.\r\n");
    FDPRINT(client, "</P></BODY></HTML>\r\n");
}

char *_hscanIfEmpty(int client, const char *reqStr, const char *msg, const char *inputstr)
{
    char *qpath = getQueryPath(reqStr);
    char *qparam = getQueryParam(reqStr, "q");
    if (strcmp(qparam, UNDEFINED) == 0) {
        FDPRINTF(client, OTAGA(form, action = "%s")
                 "%s%s" STAG(input, type = "submit") CTAG(form), qpath, msg, inputstr);
    }
    free(qpath);
    return qparam;
}

char *_hscan(int client, const char *reqStr, const char *msg, const char *inputstr)
{
    char *qpath = getQueryPath(reqStr);
    char *qparam = getQueryParam(reqStr, "q");
    FDPRINTF(client, OTAGA(form, action = "%s")
             "%s%s" STAG(input, type = "submit") CTAG(form), qpath, msg, inputstr);
    free(qpath);
    return qparam;
}

bool nope_route(Request request, const char *path, void (*function) (Request),
                bool send_headers)
{
    const char *q = request.reqStr;
    while(*path && *q && *path++ == *q++);
    if (*path == 0 && (*q == 0 || *q == '?')) {
        if (send_headers)
            writeStandardHeaders(request.client);
        if (function != NULL)
            function(request);
        return true;
    } else {
        return false;
    }
}

/* Deprecated functions */
bool route(Request request, const char *path)
{
    char *queryPath = getQueryPath(request.reqStr);
    if (strcmp(queryPath, path) == 0) {
        free(queryPath);
        return true;
    } else {
        free(queryPath);
        return false;
    }
}

bool routeh(Request request, const char *path)
{
    char *queryPath = getQueryPath(request.reqStr);
    if (strcmp(queryPath, path) == 0) {
        free(queryPath);
        writeStandardHeaders(request.client);
        return true;
    } else {
        free(queryPath);
        return false;
    }
}

bool routef(Request request, const char *path,
            void (*function) (int, const char *, const char *))
{
    char *queryPath = getQueryPath(request.reqStr);
    if (strcmp(queryPath, path) == 0) {
        free(queryPath);
        function(request.client, request.reqStr, request.method);
        return true;
    } else {
        free(queryPath);
        return false;
    }
}

bool routefh(Request request, const char *path,
             void (*function) (int, const char *, const char *))
{
    char *queryPath = getQueryPath(request.reqStr);
    if (strcmp(queryPath, path) == 0) {
        free(queryPath);
        writeStandardHeaders(request.client);
        function(request.client, request.reqStr, request.method);
        return true;
    } else {
        free(queryPath);
        return false;
    }
}

/* End deprecated functions */
