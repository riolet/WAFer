#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include "server.h"
#include "nopeutils.h"

void server_main(int client, const char * url, const char * method) 
{ 
	
	 /* Reading and writing headers
	  * Roll your own if you would like more control */
	 read_and_discard_headers(client);
	 print_standard_headers(client);
	 /* Done reading and writing headers */
	 
	 /* Fun begins here */
	 char buf[1024];
	 
	 sprintf(buf, "<HTML><HEAD></HEAD><BODY>Hello World<BR/>");
	 send(client, buf, strlen(buf), 0);

	 sprintf(buf,"%s", url);
	 send(client, buf, strlen(buf), 0); 
	
	 sprintf(buf, "</BODY></HTML>\r\n");
	 send(client, buf, strlen(buf), 0);
	/* All done */
}
	
