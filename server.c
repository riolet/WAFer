#include "server.h"
#include "nopeutils.h"
	
void server_main(int client, const char * request, const char * method) 
{ 
	  if (strcmp(request,"/")==0) {	 		
		char **headers=readHeaders(client);
		nprintf(client, "<!DOCTYPE html><HTML><HEAD></HEAD><BODY>");
		nprintf(client, "<P>Hello World</P>");
		nprintf(client, "</BODY></HTML>\r\n");
	 }
}
	
