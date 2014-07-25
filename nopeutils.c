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


/**********************************************************************/
/* Return the value of a query parameter.
 * Parameters: the query string
 *             the name of the parameter
 * Returns: the value of the query parameter */
/**********************************************************************/

char * getQueryParam(const char * queryString, const char *name) { 

	char *pos1 = strstr(queryString, name);
	char *value = malloc(MAX_BUFFER_SIZE*sizeof(char));
	int i;

    int queryStringPos = pos1-queryString;
    char *pos2 = pos1;
    while (queryStringPos > 0 && queryString[queryStringPos-1] != '&') {
        pos1 = strstr(pos2, name);
        queryStringPos += pos2-pos1;
        pos2 += strlen(name);
    }

	if (pos1) {
		pos1 += strlen(name);

		if (*pos1 == '=') {
			pos1++;
			i=0;
			while (*pos1 && *pos1 != '&') {
				if (*pos1 == '%') {
					value[i]= (char)ToHex(pos1[1]) * 16 + ToHex(pos1[2]);
					pos1 += 3;
				} else if( *pos1=='+' ) {
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

char * getQueryPath(const char * reqString)
{ 
	char * queryPath;
	queryPath = strdup(reqString);
	u_int i;
	for (i=0;i<strlen(reqString) && (queryPath[i] != '?') && (queryPath[i] != '\0');i++) {
	}

	queryPath[i] = '\0';

	return queryPath;
}

/**********************************************************************/
/* There's a bunch of headers that are exchanged at the beginning
 * between the web browser and the server. If you are ok with just
 * using the default, you may use this function
 * Parameters: the client
 * Returns: an array of headers */
/**********************************************************************/
char ** sendAndReceiveHeaders(int client)
{
	char **headers=readHeaders(client);
	int i=0;
	while (headers[i]!=NULL) {
		printf("Header -> %s",headers[i]);
		i++;
	}
	writeStandardHeaders(client);
	return headers;
}

/* Just like fprintf, but writing to the socket instead of a
 * file.
 * Parameters: the client, format
 * Returns: the number of characters printed
 */
long nprintf (int client, const char *format, ...) {

		/* Need to figure out a better method for memory management */
		char *buf = malloc(MAX_BUFFER_SIZE*MAX_DPRINTF_SIZE*sizeof(char));

		va_list arg;
		long done;
		va_start (arg, format);
		vsprintf (buf, format, arg);
		va_end (arg);

		/* printf("Buffer length %d",strlen(buf)); */
		if (strlen(buf)<MAX_BUFFER_SIZE) {
			done = (int) send(client, buf, strlen(buf), 0);
		} else {
			done = writeLongString(client,buf);
		}


		free(buf);
		return done;

}

/**********************************************************************/
/* There's a bunch of headers that browsers send us.
 * This function reads 'em.
 * Parameters: the client
 * Returns: an array of headers */
/**********************************************************************/

char ** readHeaders(int client) {
	char buf[MAX_BUFFER_SIZE];
	char **headers;
	int numchars;
	int i=0;

	/* printf("Mallocing %ld \n",sizeof(char*)*MAX_HEADERS); */
	headers=malloc(sizeof(char*)*MAX_HEADERS);


	numchars = getLine(client, buf, sizeof(buf));
	while ((numchars > 0) && strcmp("\n", buf)) {
		/* printf("Numchars %d %s\n",numchars,buf); */
		headers[i]=malloc((numchars+1)*sizeof(char));
		memcpy(headers[i],buf,numchars);
		headers[i][numchars]=0;
		i++;
		numchars = getLine(client, buf, sizeof(buf));
	}
	headers[i]=NULL;
	return headers;
}

/* Free thy mallocs */
void freeHeaders(char **headers) {
	int i=0;
	while (headers[i]!=NULL) {
		free(headers[i]);
		i++;
	}
	free(headers);
}

char * getHeader(char **headers, char *header) {
    char * current_header, * matching_header;
    // Not sure if MAX_BUFFER_SIZE is right.
    char * value = malloc(MAX_BUFFER_SIZE*sizeof(char));
    int i, j;
    for (i=0;headers[i]!=NULL;i++) {
        current_header = headers[i];
        for (j=0;j<strlen(header);j++) {
            if ((matching_header = strstr(current_header, header))) {
                value = matching_header+strlen(header);
                if (value+1 == ':') {
                    while (*value == ' ' || *value == ':')
                        value = value+1;
                    return value;
                }
            }
        }
    }
    strcpy(value, UNDEFINED);
    return value;
}

void writeStandardHeaders(int client)
{
	STATIC_SEND(client, "HTTP/1.0 200 OK\r\n", 0);
	STATIC_SEND(client, SERVER_STRING, 0);
	STATIC_SEND(client, "Content-Type: text/html\r\n", 0);
	STATIC_SEND(client, "\r\n", 0);
}

void cat(int client, FILE *pFile)
{
	char buf[1024];

	int result = fread (buf,1,1024,pFile);

	while (result!=0) {
		send(client, buf, result, 0);
		result = fread (buf,1,1024,pFile);
	} 
}

/* Serve downloadable file. */
void serveDownloadableFile(int client, const char *filename, const char *displayFilename, const char * type)
{

	FILE *resource = NULL;
	char buf[1024];
	size_t len;

	STATIC_SEND(client, "HTTP/1.0 200 OK\r\n", 0);
	STATIC_SEND(client, SERVER_STRING, 0);

	len = snprintf(buf, sizeof(buf), "Content-Type: %s\r\n",type);
	send(client, buf, len, 0);

	len = snprintf(buf, sizeof(buf), "Content-Disposition: attachment; filename=\"%s\"\r\n",displayFilename);
	send(client, buf, len, 0);

	STATIC_SEND(client, "\r\n", 0);

	resource = fopen(filename, "r");
	if (resource == NULL)
		notFound(client);
	else
	{
		cat(client, resource);
	}
	fclose(resource);
}

/* Serve and entire file. */
void serveFile(int client, const char *filename, const char * type)
{

	FILE *resource = NULL;
	char buf[1024];
	size_t len;

	STATIC_SEND(client, "HTTP/1.0 200 OK\r\n", 0);
	STATIC_SEND(client, SERVER_STRING, 0);

	len = snprintf(buf, sizeof(buf), "Content-Type: %s\r\n",type);
	send(client, buf, len, 0);

	STATIC_SEND(client, "\r\n", 0);

	resource = fopen(filename, "r");
	if (resource == NULL)
		notFound(client);
	else
	{
		cat(client, resource);
	}
	fclose(resource);
}

/* Write strings that are two big for our buffer */
long writeLongString(int client,const char* longString)
{
	char buf[MAX_BUFFER_SIZE];
	u_int maxSize = sizeof(buf);
	u_int remain = strlen(longString);

	u_long sent=0;
	while (remain)
	{
		/* printf("To send %d\n",remain); */
		u_int toCpy = remain > maxSize ? maxSize : remain;
		strncpy(buf, longString, toCpy);
		longString += toCpy;
		remain -= toCpy;
		sent += send(client, buf, toCpy, 0);
	}
	/* printf("Sent  %d\n",sent); */
	return sent;
}



/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int getLine(int sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;

	while ((i < size - 1) && (c != '\n'))
	{
		n = recv(sock, &c, 1, 0);
		/* DEBUG printf("%02X\n", c); */
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				/* DEBUG printf("%02X\n", c); */
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					c = '\n';
			}
			buf[i] = c;
			i++;
		}
		else
			c = '\n';
	}
	buf[i] = '\0';

	return(i);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void notFound(int client)
{
	nprintf(client, "HTTP/1.0 404 NOT FOUND\r\n");
	nprintf(client, "Content-Type: text/html\r\n");
	nprintf(client, "\r\n");
	nprintf(client, "<HTML><TITLE>Not Found</TITLE>\r\n");
	nprintf(client, "<BODY><P>The server could not fulfill\r\n");
	nprintf(client, "your request because the resource specified\r\n");
	nprintf(client, "is unavailable or nonexistent.\r\n");
	nprintf(client, "</P></BODY></HTML>\r\n");
}

char * _hscanIfEmpty(int client, const char * reqStr, const char *msg,const char * inputstr) {
	char * qpath=getQueryPath(reqStr);
	char * qparam=getQueryParam(reqStr,"q");
	if (strcmp(qparam,UNDEFINED)==0) {
		nprintf(client,
				OTAGA(form,action="%s")
					"%s%s"
					STAG(input,type="submit")
				CTAG(form)
				,qpath,msg,inputstr);
	}
	free(qpath);
	return qparam;
}

char * _hscan(int client, const char * reqStr, const char *msg,const char * inputstr) {
	char * qpath=getQueryPath(reqStr);
	char * qparam=getQueryParam(reqStr,"q");
	nprintf(client,
			OTAGA(form,action="%s")
				"%s%s"
				STAG(input,type="submit")
			CTAG(form)
			,qpath,msg,inputstr);
	free(qpath);
	return qparam;
}

bool route(Request request, const char * path) {
	char * queryPath = getQueryPath(request.reqStr);
	if (strcmp(queryPath,path)==0) {
		free(queryPath);
		return true;
	} else {
		free(queryPath);
		return false;
	}
}

bool routeh(Request request, const char * path) {
	char * queryPath = getQueryPath(request.reqStr);
	if (strcmp(queryPath,path)==0) {
		free(queryPath);
		char ** headers = sendAndReceiveHeaders(request.client);
		if (headers)
			freeHeaders(headers);
		return true;
	} else {
		free(queryPath);
		return false;
	}
}

bool routef(Request request, const char * path, void (* function)(int,char *, char*)) {
	char * queryPath = getQueryPath(request.reqStr);
	if (strcmp(queryPath,path)==0) {
		free(queryPath);
		function(request.client,request.reqStr,request.method);
		return true;
	} else {
		free(queryPath);
		return false;
	}
}

bool routefh(Request request, const char * path, void (* function)(int,char *, char*)) {
	char * queryPath = getQueryPath(request.reqStr);
	if (strcmp(queryPath,path)==0) {
		free(queryPath);
		char ** headers = sendAndReceiveHeaders(request.client);
		function(request.client,request.reqStr,request.method);
		if (headers)
			freeHeaders(headers);
		return true;
	} else {
		free(queryPath);
		return false;
	}
}
