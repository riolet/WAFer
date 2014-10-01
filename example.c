#include "nope.h"
#include "nopeapi.h"
#include <string.h>

static void factor(Request *request, Response *response);

void server(Request * request, Response * response)
{
	printf("%s %s",request->method,request->reqStr);
	routeRequest(request, response,"/factor", factor);
}

static void factor(Request *request, Response *response)
{
    char *nStr = QUICK_FORM_TEXT(request,response,"Number to factor:");

    if (nStr != NULL) {
        long n = strtol(nStr, NULL, 10);
        resPrintf(response, "Factors of %li are: ", n);
        long l;
        for (l = 2; l <= n; ++l) {
            if (n % l == 0) {
            	resPrintf(response, "%li ", l);
            }
        }
    }
}
