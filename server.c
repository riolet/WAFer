#include "server.h"

void server_main(int client, const char * url, const char * method) { 
	char buf[1024];
	 int numchars = 1;

	 buf[0] = 'A'; buf[1] = '\0';
     while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
		numchars = get_line(client, buf, sizeof(buf));
  
	 strcpy(buf, "HTTP/1.0 200 OK\r\n");
	 send(client, buf, strlen(buf), 0);
	 strcpy(buf, "Server: ritchiehttpd/0.1.0\r\n");
	 send(client, buf, strlen(buf), 0);
	 sprintf(buf, "Content-Type: text/html\r\n");
	 send(client, buf, strlen(buf), 0);
	 strcpy(buf, "\r\n");
	 send(client, buf, strlen(buf), 0);
 
	 sprintf(buf, "<HTML><HEAD></HEAD><BODY>Hello World<BR/>");
	 send(client, buf, strlen(buf), 0);

	 sprintf(buf,url);
	 send(client, buf, strlen(buf), 0); 
	
	 sprintf(buf, "</BODY></HTML>\r\n");
	 send(client, buf, strlen(buf), 0);
	
}
	
