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
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "nopeutils.h"

void headers(int client)
{
	read_and_discard_headers(client);
	write_standard_headers(client);
}

void docwrite(int client,const char* string) 
{
	char buf[1024];
	
	if (strlen(string)<1024) 
	{
		sprintf(buf, "%s", string);
		send(client, buf, strlen(buf), 0);
	} else 
	{
		write_long_string(client,string);
	}
}

void read_and_discard_headers(client)
{
	 char buf[1024];
	 int numchars = 1;

	 buf[0] = 'A'; buf[1] = '\0';
     while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
		numchars = get_line(client, buf, sizeof(buf));
}

void write_standard_headers(client)
{
	 char buf[1024];
  
	 strcpy(buf, "HTTP/1.0 200 OK\r\n");
	 send(client, buf, strlen(buf), 0);
	 strcpy(buf, SERVER_STRING);
	 send(client, buf, strlen(buf), 0);
	 sprintf(buf, "Content-Type: text/html\r\n");
	 send(client, buf, strlen(buf), 0);
	 strcpy(buf, "\r\n");
	 send(client, buf, strlen(buf), 0);
}

void cat(int client, FILE *resource)
{
 char buf[1024];

 fgets(buf, sizeof(buf), resource);
 while (!feof(resource))
 {
  send(client, buf, strlen(buf), 0);
  fgets(buf, sizeof(buf), resource);
 }
}

void serve_file(int client, const char *filename)
{
 FILE *resource = NULL;

 resource = fopen(filename, "r");
 if (resource == NULL)
  not_found(client);
 else
 {
  cat(client, resource);
 }
 fclose(resource);
}

void write_long_string(int client,const char* longString)
{
    char buf[1024];
    int remain = strlen(longString);
    while (remain)
    {
      int toCpy = remain > sizeof(buf) ? sizeof(buf) : remain;
      memcpy(buf, longString, toCpy);
      longString += toCpy;
      remain -= toCpy;
      send(client, buf, strlen(buf), 0);
    }
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
int get_line(int sock, char *buf, int size)
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
void not_found(int client)
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, SERVER_STRING);
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-Type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "your request because the resource specified\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "is unavailable or nonexistent.\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "</BODY></HTML>\r\n");
 send(client, buf, strlen(buf), 0);
}
