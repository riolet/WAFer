#include <arpa/inet.h>          /* inet_ntoa */
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

#include "server.h"
#include "nopeutils.h"

#define ISspace(x) isspace((int)(x))

#define LISTENQ  1024  /* second argument to listen() */
#define MAXLINE 1024   /* max length of a line */
#define RIO_BUFSIZE 1024

typedef struct {
    int rio_fd;                 /* descriptor for this buf */
    int rio_cnt;                /* unread byte in this buf */
    char *rio_bufptr;           /* next unread byte in this buf */
    char rio_buf[RIO_BUFSIZE];  /* internal buffer */
} rio_t;

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

typedef struct {
    char filename[512];
    off_t offset;              /* for support Range */
    size_t end;
} http_request;

char *default_mime_type = "text/plain";

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client)
{
	STATIC_SEND(client, "HTTP/1.0 501 Method Not Implemented\r\n", 0);
	STATIC_SEND(client, SERVER_STRING, 0);
	STATIC_SEND(client, "Content-Type: text/html\r\n", 0);
	STATIC_SEND(client, "\r\n", 0);
	STATIC_SEND(client, "<HTML><HEAD><TITLE>Method Not Implemented\r\n", 0);
	STATIC_SEND(client, "</TITLE></HEAD>\r\n", 0);
	STATIC_SEND(client, "<BODY><P>HTTP request method not supported.\r\n", 0);
	STATIC_SEND(client, "</P></BODY></HTML>\r\n", 0);
}

void rio_readinitb(rio_t *rp, int fd){
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

ssize_t writen(int fd, void *usrbuf, size_t n){
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0){
        if ((nwritten = write(fd, bufp, nleft)) <= 0){
            if (errno == EINTR)  /* interrupted by sig handler return */
                nwritten = 0;    /* and call write() again */
            else
                return -1;       /* errorno set by write() */
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}


/*
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n){
    int cnt;
    while (rp->rio_cnt <= 0){  /* refill if buf is empty */

        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
                           sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0){
            if (errno != EINTR) /* interrupted by sig handler return */
                return -1;
        }
        else if (rp->rio_cnt == 0)  /* EOF */
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf; /* reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;
    if (rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

/*
 * rio_readlineb - robustly read a text line (buffered)
 */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen){
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++){
        if ((rc = rio_read(rp, &c, 1)) == 1){
            *bufp++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0){
            if (n == 1)
                return 0; /* EOF, no data read */
            else
                break;    /* EOF, some data was read */
        } else
            return -1;    /* error */
    }
    *bufp = 0;
    return n;
}

void format_size(char* buf, struct stat *stat){
    if(S_ISDIR(stat->st_mode)){
        sprintf(buf, "%s", "[DIR]");
    } else {
        off_t size = stat->st_size;
        if(size < 1024){
            sprintf(buf, "%lu", size);
        } else if (size < 1024 * 1024){
            sprintf(buf, "%.1fK", (double)size / 1024);
        } else if (size < 1024 * 1024 * 1024){
            sprintf(buf, "%.1fM", (double)size / 1024 / 1024);
        } else {
            sprintf(buf, "%.1fG", (double)size / 1024 / 1024 / 1024);
        }
    }
}

int open_listenfd(int port){
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    // 6 is TCP's protocol number
    // enable this, much faster : 4000 req/s -> 17000 req/s
    if (setsockopt(listenfd, 6, TCP_CORK,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
}

void log_access(int status, struct sockaddr_in *c_addr, http_request *req){
    printf("%s:%d %d - %s\n", inet_ntoa(c_addr->sin_addr),
           ntohs(c_addr->sin_port), status, req->filename);
}

void client_error(int fd, int status, char *msg, char *longmsg){
    char buf[MAXLINE];
    sprintf(buf, "HTTP/1.1 %d %s\r\n", status, msg);
    sprintf(buf + strlen(buf),
            "Content-length: %lu\r\n\r\n", strlen(longmsg));
    sprintf(buf + strlen(buf), "%s", longmsg);
    writen(fd, buf, strlen(buf));
}

void process(int fd, struct sockaddr_in *clientaddr){
    printf("accept request, fd is %d, pid is %d\n", fd, getpid());

    int status = 200;

	char buf[1024];
	u_int numchars;
	char method[255];
	char url[1024];
	size_t i, j;
    int client=fd;

	numchars = getLine(client, buf, sizeof(buf));
	if (numchars==0) {
		printf("Keep alive null packet. Ignore %d\n",numchars);
		return;
	}

	i = 0; j = 0;
	while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
	{
		method[i] = buf[j];
		i++; j++;

	}
	method[i] = '\0';

	if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
	{
		unimplemented(client);
		return;
	}


	i = 0;
	while (ISspace(buf[j]) && (j < sizeof(buf)))
		j++;
	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
	{
		url[i] = buf[j];
		i++; j++;
	}
	url[i] = '\0';

	Request request;
	request.client=client;
	request.reqStr=url;
	request.method=method;

	/*server(client,url,method);*/

	server(request);

	close(client);

	http_request req;
	strcpy(req.filename,url);
    log_access(status, clientaddr, &req);
}

int main(void){
	int i;
    struct sockaddr_in clientaddr;
    int default_port = 4242,
    	nChildren = 15,
        listenfd,
        connfd;

    socklen_t clientlen = sizeof clientaddr;

    char* pPort = getenv ("PORT");

	if (pPort!=NULL)
		default_port = (u_short) strtol (pPort,(char **)NULL, 10);

    char* pChildren = getenv ("CHILDREN");

	if (pChildren!=NULL)
		nChildren = (u_short) strtol (pChildren,(char **)NULL, 10);

    listenfd = open_listenfd(default_port);
    if (listenfd > 0) {
        printf("listen on port %d, fd is %d\n", default_port, listenfd);
    } else {
        perror("ERROR");
        exit(listenfd);
    }
    // Ignore SIGPIPE signal, so if browser cancels the request, it
    // won't kill the whole process.
    signal(SIGPIPE, SIG_IGN);

    for(i = 0; i < nChildren; i++) {
        int pid = fork();
        if (pid == 0) {         //  child
            while(1){
                connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
                process(connfd, &clientaddr);
                close(connfd);
            }
        } else if (pid > 0) {   //  parent
            printf("child pid is %d\n", pid);
        } else {
            perror("fork");
        }
    }

    while(1){
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        process(connfd, &clientaddr);
        close(connfd);
    }

    return 0;
}
