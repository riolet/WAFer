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
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>



#include "server.h"
#include "nopeutils.h"

#define ISspace(x) isspace((int)(x))

#define LISTENQ  1024  /* second argument to listen() */
#define MAXLINE 1024   /* max length of a line */
#define RIO_BUFSIZE 1024

typedef struct {
    short int state;
    char *readBuffer;
    short  readBufferIdx;
    short  readBufferLen;
    char *method;
    short methodIdx;
    char *uri;
    short uriIdx;
    char *ver;
    short verIdx;
    char **headers;
    short headersIdx;
    short withinHeaderIdx;
} FdData;

typedef struct {
    int rio_fd;                 /* descriptor for this buf */
    size_t rio_cnt;                /* unread byte in this buf */
    char *rio_bufptr;           /* next unread byte in this buf */
    char rio_buf[RIO_BUFSIZE];  /* internal buffer */
} rio_t;

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

typedef struct {
    char filename[MAX_BUFFER_SIZE];
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
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n) {
    ssize_t cnt, tmp;
    if (rp->rio_cnt == 0) {  /* refill if buf is empty */

        tmp = read(rp->rio_fd, rp->rio_buf,
                           sizeof(rp->rio_buf));
        if (tmp < 0){
            if (errno != EINTR) /* interrupted by sig handler return */
                return -1;
        }
        else if (tmp == 0)  /* EOF */
            return 0;

        rp->rio_bufptr = rp->rio_buf; /* reset buffer ptr */
        rp->rio_cnt = tmp;
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
    size_t n;
    ssize_t rc;
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

int process(int fd, char **readBuffer, fd_set *pMaster, struct sockaddr_in *clientaddr){
    printf("accept request, fd is %d, pid is %d\n", fd, getpid());

    int status = 200;

	char buf[MAX_BUFFER_SIZE];
	unsigned int numchars;
	char method[MAX_BUFFER_SIZE];
	char url[MAX_BUFFER_SIZE];
	size_t i, j;
    int client=fd;

	numchars = getLine(client, buf, sizeof(buf)-1);

	/* Null requests. Safe to ignore */
	if (numchars==0)
		return numchars;

	i = 0; j = 0;
	while (!ISspace(buf[j]) && (i < sizeof(method) - 1) && i<numchars)
	{
		method[i] = buf[j];
		i++; j++;

	}
	method[i] = '\0';

	if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
	{
		unimplemented(client);
		return 1;
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

}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

initializeFdData(FdData *fdData) {
	fdData->state=STATE_METHOD;
	fdData->readBuffer=malloc((MAX_REQUEST_SIZE+1)*sizeof(char));
	fdData->method=malloc((MAX_METHOD_SIZE+1)*sizeof(char));
	fdData->uri=malloc((MAX_REQUEST_SIZE+1)*sizeof(char));
	fdData->headers=malloc(MAX_HEADERS*sizeof(char*));
	fdData->headers[0]=malloc(MAX_BUFFER_SIZE*sizeof(char));
	fdData->ver=malloc(MAX_HEADERS*sizeof(char*));
	fdData->readBufferIdx=0;
	fdData->readBufferLen=0;
	fdData->readBufferIdx=0;
	fdData->methodIdx=0;
	fdData->uriIdx=0;
	fdData->verIdx=0;
	fdData->headersIdx=0;
	fdData->withinHeaderIdx=0;
}

void selectLoop(int listener)
{
	/* Thank you Brian "Beej Jorgensen" Hall */
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[MAX_BUFFER_SIZE];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int i;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    FdData fdData[MAX_FD_SIZE];
    for (i=0;i<fdmax;i++) {
    	fdData[i].state=STATE_PRE_REQUEST;
    }

    /*
    short int state[MAX_FD_SIZE];
    char *readBuffer[MAX_FD_SIZE];
    short int readBufferIdx[MAX_FD_SIZE];
    char *method[MAX_FD_SIZE];
    char *uri[MAX_FD_SIZE];
    char **headers[MAX_FD_SIZE];
     */

    // main loop
    char printBuffer[1024];
    char *method;
    char *uri;
    int idx,j,len;
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);
                    if (newfd>MAX_FD_SIZE) {
                    	fprintf(stderr,"Max FD Size reached. Can't continue");
                    	return;
                    }

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                        fdData[i].state=STATE_PRE_REQUEST;
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                    	// we got some data from a client

                    	snprintf(printBuffer,1024,"%s",buf);
                    	printf("Received %d as %s\n",nbytes,printBuffer);
                    	bool read=false;
                    	if (fdData[i].state==STATE_PRE_REQUEST) {
                    		initializeFdData(&fdData[i]);
                    		memcpy(fdData[i].readBuffer,buf,nbytes);
                    		fdData[i].readBufferLen += nbytes;
                    		read=true;
                    	}

                    	if (fdData[i].state==STATE_METHOD) {
                    		snprintf(printBuffer,fdData[i].readBufferLen,"%s",fdData[i].readBuffer);
                    		printf("STATE=METHOD -> %d %d %s\n",fdData[i].readBufferLen,fdData[i].readBufferIdx, printBuffer);
                    		if (!read) {
                    			if (nbytes+fdData[i].readBufferIdx<MAX_REQUEST_SIZE) {
                    				memcpy(fdData[i].readBuffer+fdData[i].readBufferIdx,buf,nbytes);
                    				fdData[i].readBufferLen += nbytes;
                    				read=true;
                    			}
                    		}

                    		idx=fdData[i].methodIdx;
                    		j=fdData[i].readBufferIdx;
                    		len=fdData[i].readBufferLen;

                    		while (j<len && idx<MAX_METHOD_SIZE)
                    		{
                    			if (fdData[i].readBuffer[j]=='\n') {
                         			fdData[i].method[idx]=0;
                            		fdData[i].state=STATE_HEADER;
                            		break;
                    			} else if (ISspace(fdData[i].readBuffer[j])) {
                        			fdData[i].method[idx]=0;
                        			fdData[i].state=STATE_VERSION;
                        			break;
                        		} else if (fdData[i].readBuffer[j]=='\r') {
                        			/*Skip over \r */
                    			} else {
                    				fdData[i].method[idx] = fdData[i].readBuffer[j];
                    				idx++;
                    			}
                    			j++;
                    		}

                    		fdData[i].methodIdx=idx;
                    		fdData[i].readBufferIdx=j+1;

                    		if (idx==MAX_METHOD_SIZE) {	/*We don't like really long methods. Cut em off */
                    			fdData[i].method[idx]=0;
                    			fdData[i].state=STATE_URI;
                    		}

                    	}

                    	if (fdData[i].state==STATE_URI) {
                    		printf("Method of %d is %s\n",i,fdData[i].method);

                    		snprintf(printBuffer,fdData[i].readBufferLen,"%s",fdData[i].readBuffer);
                    		//printf("STATE=URI -> %d %d %s\n",fdData[i].readBufferLen,fdData[i].readBufferIdx, printBuffer);
                    		if (!read) {
                    			if (nbytes+fdData[i].readBufferIdx<MAX_REQUEST_SIZE) {
                    				memcpy(fdData[i].readBuffer+fdData[i].readBufferIdx,buf,nbytes);
                    				fdData[i].readBufferLen += nbytes;
                    				read=true;
                    			}
                    		}

                    		idx=fdData[i].uriIdx;
                    		j=fdData[i].readBufferIdx;
                    		len=fdData[i].readBufferLen;

                    		while (j<len && idx<MAX_REQUEST_SIZE)
                    		{
                    			if (fdData[i].readBuffer[j]=='\n') {
                         			fdData[i].uri[idx]=0;
                            		fdData[i].state=STATE_HEADER;
                            		break;
                    			} else if (ISspace(fdData[i].readBuffer[j])) {
                        			fdData[i].uri[idx]=0;
                        			fdData[i].state=STATE_VERSION;
                        			break;
                        		} else if (fdData[i].readBuffer[j]=='\r') {
                        			/*Skip over \r */
                    			} else {
                    				fdData[i].uri[idx] = fdData[i].readBuffer[j];
                    				idx++;
                    			}
                    			j++;
                    		}

                    		fdData[i].uriIdx=idx;
                    		fdData[i].readBufferIdx=j+1;

                    		if (idx==MAX_REQUEST_SIZE) {	/*We don't like really long URIs either. Cut em off */
                    			fdData[i].uri[idx]=0;
                    			fdData[i].state=STATE_VERSION;
                    		}

                    	}

                    	if (fdData[i].state==STATE_VERSION) {
                    		printf("URI of %d is %s\n",i,fdData[i].uri);

                    		snprintf(printBuffer,fdData[i].readBufferLen,"%s",fdData[i].readBuffer);
                    		//printf("STATE=VERSION -> %d %d %s\n",fdData[i].readBufferLen,fdData[i].readBufferIdx, printBuffer);

                    		if (!read) {
                    			if (nbytes+fdData[i].readBufferIdx<MAX_REQUEST_SIZE) {
                    				memcpy(fdData[i].readBuffer+fdData[i].readBufferIdx,buf,nbytes);
                    				fdData[i].readBufferLen += nbytes;
                    				read=true;
                    			} else {
                    				return; /* How big do you want your request to be?? */
                    			}
                    		}

                    		idx=fdData[i].verIdx;
                    		j=fdData[i].readBufferIdx;
                    		len=fdData[i].readBufferLen;

                    		while (j<len && idx<MAX_VER_SIZE)
                    		{
                    			if (fdData[i].readBuffer[j]=='\n') {
                         			fdData[i].ver[idx]=0;
                            		fdData[i].state=STATE_HEADER;
                            		break;
                    			} else if (fdData[i].readBuffer[j]=='\r') {
                        			/*Skip over \r */
                    			} else {
                    				fdData[i].ver[idx] = fdData[i].readBuffer[j];
                    				idx++;
                    			}
                    			j++;
                    		}
                    		fdData[i].verIdx=idx;
                    		fdData[i].readBufferIdx=j+1;

                    		/*We don't like really long version either. Cut em off */
                    		if (idx==MAX_VER_SIZE) {	/*We don't like really long URIs either. Cut em off */
                    			fdData[i].ver[idx]=0;
                    			fdData[i].state=STATE_HEADER;
                    		}

                    	}

                    	if (fdData[i].state==STATE_HEADER) {
                    		printf("VER of %d is %s\n",i,fdData[i].ver);

                    		printf("Start reading headers\n");
                    		if (!read) {
                    			if (nbytes+fdData[i].readBufferIdx<MAX_REQUEST_SIZE) {
                    				memcpy(fdData[i].readBuffer+fdData[i].readBufferIdx,buf,nbytes);
                    				fdData[i].readBufferLen += nbytes;
                    				read=true;
                    			} else {
                    				return; /* How big do you want your request to be?? */
                    			}
                    		}

                    		j=fdData[i].readBufferIdx,
                    		len=fdData[i].readBufferLen;
                    		idx=fdData[i].withinHeaderIdx;

                    		while (j<len) {
                    			if (fdData[i].readBuffer[j]=='\n') {
                    				if (fdData[i].withinHeaderIdx==0) {
                    					break; /* The last header */
                    				}
                    				fdData[i].headersIdx++;
                         			fdData[i].headers[fdData[i].headersIdx]=malloc(MAX_BUFFER_SIZE*sizeof(char));
                         			idx=fdData[i].withinHeaderIdx=0;
                            		break;
                    			} else if (fdData[i].readBuffer[j]=='\r') {
                        			/*Skip over \r */
                    			} else {
                    				fdData[i].ver[idx] = fdData[i].readBuffer[j];
                    				idx++;
                    			}
                    			j++;
                    		}
                    		fdData[i].withinHeaderIdx=idx;
                    		fdData[i].readBufferIdx=j+1;

                    		Request request;
                    		request.client=i;
                    		request.reqStr=fdData[i].uri;
                    		request.method=fdData[i].method;
                    		request.headers=fdData[i].headers;
                    		server(request);
                    		close(i);

                    		http_request req;
                    		snprintf(req.filename,sizeof(req.filename)-1,"%s",fdData[i].uri);
                    		char * clientaddr = "TODO";
                    	    log_access(STATUS_HTTP_OK, clientaddr, &req);
                    	    initializeFdData(&fdData[i]);
                            close(i); // bye!
                            FD_CLR(i, &master); // remove from master set
                    	}
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

    return;
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
        	selectLoop(listenfd);
        } else if (pid > 0) {   //  parent
            printf("child pid is %d\n", pid);
        } else {
            perror("fork");
        }
    }

	selectLoop(listenfd);
    return 0;
}
