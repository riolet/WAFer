#include "server.h"
#include "nopeutils.h"
	
void server(int client, const char * queryString, const char * method)
{ 
	  if (strcmp(queryString,"/")==0) {
		char **headers=readHeaders(client);
		nprintf(client, "<!DOCTYPE html><HTML><HEAD></HEAD><BODY>");
		nprintf(client, "<P>Hello World</P>");
		nprintf(client, "</BODY></HTML>\r\n");
	 }
}
	
