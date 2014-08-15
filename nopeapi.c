#include "nope.h"
/* Ver 0.0.5 Functions
 * Parameters: request, format, (optional) variadargs
 * Returns: the number of characters printed
 */
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

    resprintf(request, "Content-Type: %s\r\n", type);

    if (displayFilename!=NULL) {
    	resprintf(request, "Content-Disposition: attachment; filename=\"%s\"\r\n",
             displayFilename);
    }

    STATIC_SEND(request.client, "\r\n");

    resource = fopen(filename, "r");
    if (resource == NULL) {
    	respondResourceNotFound(request);
    } else {
        cat(request.client, resource);
    }
    fclose(resource);
}


void respondResourceNotFound(Request request)
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

bool routeRequest(Request request, const char *path, void (*function) (Request),
                bool send_headers)
{
    char *queryPath = getQueryPath(request.reqStr);
    if (strcmp(queryPath, path) == 0) {
        free(queryPath);
        if (send_headers)
            writeStandardHeaders(request.client);
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
