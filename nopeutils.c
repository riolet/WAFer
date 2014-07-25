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

	char bufferAmpersand[MAX_BUFFER_SIZE];
	char bufferQuestion[MAX_BUFFER_SIZE];
	snprintf(bufferQuestion,MAX_BUFFER_SIZE-1,"?%s=",name);
	snprintf(bufferAmpersand,MAX_BUFFER_SIZE-1,"&%s=",name);
	char *buffer;
	char *pos1;


	char *value = malloc(MAX_BUFFER_SIZE*sizeof(char));
	int i;

	buffer=bufferQuestion;
	pos1 = strstr(queryString,bufferQuestion);
	printf ("Buffer %s Pos %d\n",buffer,pos1);
    if (!pos1) {
    	buffer=bufferAmpersand;
        pos1 = strstr(queryString, bufferAmpersand);
    }
	if (pos1) {
		pos1 += strlen(buffer);
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

		/* initial buffer large enough for most cases, will resize if required */
		char *buf = malloc(MAX_BUFFER_SIZE);
		int len;

		va_list arg;
		long done;
		va_start (arg, format);
		len = vsnprintf (buf, MAX_BUFFER_SIZE, format, arg);
		va_end (arg);

		if (len > MAX_BUFFER_SIZE) {
			/* buffer size was not enough */
			free(buf);
			buf = malloc(len+1);
			va_start (arg, format);
			vsnprintf (buf, len+1, format, arg);
			va_end (arg);
		}

		/* printf("Buffer length %d",strlen(buf)); */
		if (len<MAX_BUFFER_SIZE) {
			done = (int) send(client, buf, len, 0);
		} else {
			done = writeLongString(client,buf,len);
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
	/*Todo: Close the FD if numchars<=0 */
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
	int i;
	for (i=0;headers[i]!=NULL;i++) {
		/* Work in Progress */
	}
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

	STATIC_SEND(client, "HTTP/1.0 200 OK\r\n", 0);
	STATIC_SEND(client, SERVER_STRING, 0);

	nprintf(client, "Content-Type: %s\r\n",type);
	nprintf(client, "Content-Disposition: attachment; filename=\"%s\"\r\n",displayFilename);

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

	STATIC_SEND(client, "HTTP/1.0 200 OK\r\n", 0);
	STATIC_SEND(client, SERVER_STRING, 0);

	nprintf(client, "Content-Type: %s\r\n",type);

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
ssize_t writeLongString(int client,const char* longString, size_t len)
{
	size_t remain = len;
	size_t sent=0;

	while (remain)
	{
		/* printf("To send %d\n",remain); */
		size_t toCpy = remain > MAX_BUFFER_SIZE ? MAX_BUFFER_SIZE : remain;
		ssize_t sent_once = send(client, longString, toCpy, 0);
		if (sent_once <= 0) return -1;
		sent += sent_once;
		longString += sent_once;
		remain -= sent_once;
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
		if (n<0) {
			return n;
		}
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

bool routef(Request request, const char * path, void (* function)(int,const char *, const char*)) {
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

bool routefh(Request request, const char * path, void (* function)(int,const char *, const char*)) {
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
