#include "server.h"
#include "nopeutils.h"
#include <string.h>

void factor(int client,const char * reqStr, const char * method);

void server(Request request) {
	routefh(request,"/factor",&factor);
}

void factor(int client,const char * reqStr, const char * factor) {
	char *nStr=HSCANIT(client,reqStr,"Number to factor:");

	if (strcmp(nStr,UNDEFINED)!=0) {
		long n = strtol(nStr,NULL,10);
		nprintf(client,"Factors of %li are: ", n);
		long l;
		for(l=2;l<=n;++l) {
			if(n%l==0) {
				nprintf (client,"%li ",l);
			}
		}
	}
}

