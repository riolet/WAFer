long resPrintf(Request request, const char *format, ...);
void serveFile(Request request, const char *filename, const char *displayFilename,
                           const char *type);

/*Internal stuff follows. Could change in future. Do not use */
ssize_t writeLongString(int client, const char *longString, size_t len);
static void cat(int client, FILE * pFile);
#define STATIC_SEND(_socket, _str) send(_socket, _str, sizeof(_str)-1, 0)
