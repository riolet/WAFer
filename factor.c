#include "nope.h"
#include "nopeutils.h"
#include <string.h>

void factor(Request request);

void server(Request request)
{
    nope_route(request, "/factor", &factor, true);
}

void factor(Request request)
{
    int client = request.client;
    char *nStr = HSCANIT(client, request.reqStr, "Number to factor:");

    if (strcmp(nStr, UNDEFINED) != 0) {
        long n = strtol(nStr, NULL, 10);
        nprintf(client, "Factors of %li are: ", n);
        long l;
        for (l = 2; l <= n; ++l) {
            if (n % l == 0) {
                nprintf(client, "%li ", l);
            }
        }
    }
}
