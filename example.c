#include "nope.h"
#include "nopeapi.h"
#include <string.h>

static void factor(Request request);

void server(Request request)
{
    routeRequest(request, "/factor", &factor,true);
}

static void factor(Request request)
{
    char *nStr = resQuickForm(request, "Number to factor:");

    if (strcmp(nStr, UNDEFINED) != 0) {
        long n = strtol(nStr, NULL, 10);
        resPrintf(request, "Factors of %li are: ", n);
        long l;
        for (l = 2; l <= n; ++l) {
            if (n % l == 0) {
            	resPrintf(request, "%li ", l);
            }
        }
    }
}
