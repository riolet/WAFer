#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include "server.h"
#include "nopeutils.h"
	
void server_main(int client, const char * request, const char * method) 
{ 
	  if (strcmp(request,"/")==0) {	 		
		headers(client); 
		docwrite(client, "<!DOCTYPE html><HTML><HEAD></HEAD><BODY>");
		docwrite(client, "<P>Hello World</P>");
		docwrite(client, "</BODY></HTML>\r\n");
	 }
}
	
