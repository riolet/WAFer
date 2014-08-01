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
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>

#include "nope.h"

#define LISTENQ  1024  /* second argument to listen() */
#define MAXLINE 1024   /* max length of a line */
#define RIO_BUFSIZE 1024

#define DEFAULT_PORT 4242
#define DEFAULT_N_CHILDREN 15

#define SIZE_OF_CHAR sizeof(char)

#define ON_SPACE_TERMINATE_STRING_CHANGE_STATE(_str_,_state_)\
	if (isspace(fdDataList[i].readBuffer[j])) {\
		_str_[idx]=0;\
		fdDataList[i].state = _state_;\
		j++;\
		break;\
	}

#define ON_SLASH_N_TERMINATE_STRING_CHANGE_STATE(_str_,_state_)\
	if (fdDataList[i].readBuffer[j]=='\n') {\
		_str_[idx]=0;\
		fdDataList[i].state = _state_;\
		j++;\
		break;\
	}

#define ON_SLASH_R_IGNORE\
	if (fdDataList[i].readBuffer[j]=='\r') { /*Do nothing*/ }

#define ON_EVERYTHING_ELSE_CONSUME(_str_)\
	{\
		_str_[idx] = fdDataList[i].readBuffer[j];\
		idx++;\
	}

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

void new_fd_data(FdData *fd) {
	fd->state = STATE_PRE_REQUEST;
	fd->readBuffer = malloc((MAX_REQUEST_SIZE+1)*SIZE_OF_CHAR);
	fd->method = malloc((MAX_METHOD_SIZE+1)*SIZE_OF_CHAR);
	fd->uri = malloc((MAX_REQUEST_SIZE+1)*SIZE_OF_CHAR);
	fd->ver = malloc((MAX_VER_SIZE+1)*SIZE_OF_CHAR);
	fd->headers = malloc(MAX_HEADERS*sizeof(char*));
	fd->readBufferIdx = 0;
	fd->readBufferLen = 0;
	fd->readBufferIdx = 0;
	fd->methodIdx = 0;
	fd->uriIdx = 0;
	fd->verIdx = 0;
	fd->headersIdx = 0;
	fd->headers[fd->headersIdx]=NULL;
	fd->withinHeaderIdx = 0;
}

void free_fd_data(FdData *fd) {
	free(fd->readBuffer);
	free(fd->method);
	free(fd->uri);
	freeHeaders(fd->headers);
	free(fd->ver);
	fd->readBufferIdx = 0;
	fd->readBufferLen = 0;
	fd->readBufferIdx = 0;
	fd->methodIdx = 0;
	fd->uriIdx = 0;
	fd->verIdx = 0;
	fd->headersIdx = 0;
	fd->withinHeaderIdx = 0;
}

/* Free thy mallocs */
void freeHeaders(char **headers)
{
	int i=0;
	while (headers[i]!=NULL) {
		free(headers[i]);
		i++;
	}
	free(headers);
}

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

typedef struct {
	char filename[MAX_BUFFER_SIZE];
	off_t offset;              /* for support Range */
	size_t end;
} http_request;

int open_listenfd(int port){
	int listenfd, optval = 1;
	struct sockaddr_in serveraddr;

	/* Create a socket descriptor */
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	/* Eliminates "Address already in use" error from bind. */
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
		       (const void *)&optval , sizeof(int)) < 0)
		return -1;

	/* TCP_CORK: If set, don't send out partial frames. */
	#ifdef TCP_CORK
	if (setsockopt(listenfd, IPPROTO_TCP, TCP_CORK,
		       (const void *)&optval , sizeof(int)) < 0)
		return -1;
	#endif

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

void log_access(int status, const char * remote_ip, Request request)
{
	printf("%s:%d %d - %s\n", remote_ip, request.reqStr);
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void accept_connection(FdData *fds, int i, int listenfd, char *remote_ip, int *fdmax, fd_set *master)
{
	socklen_t addrlen;
	struct sockaddr_storage remoteaddr; // client address
	int newfd;        // newly accept()ed socket descriptor

	/* handle new connections */
	addrlen = sizeof remoteaddr;
	newfd = accept(listenfd,
		       (struct sockaddr *)&remoteaddr,
		       &addrlen);
	if (newfd>MAX_FD_SIZE) {
		fprintf(stderr,"Max FD Size reached. Can't continue");
		return;
	}

	if (newfd == -1) {
		perror("accept");
	} else {
		FD_SET(newfd, master); // add to master set
		if (newfd > *fdmax) {    // keep track of the max
			*fdmax = newfd;
		}
		printf("selectserver: new connection from %s on "
		       "socket %d\n",
		       inet_ntop(remoteaddr.ss_family,
				 get_in_addr((struct sockaddr*)&remoteaddr),
				 remote_ip, INET6_ADDRSTRLEN),
		       newfd);
		fds[newfd].state = STATE_PRE_REQUEST;
		new_fd_data(&fds[newfd]);
	}
}

void shutdown_connection(FdData *fds, int i, ssize_t nbytes, fd_set *master)
{
	/* got error or connection closed by client */
	if (nbytes == 0) {
		/* connection closed */
		printf("selectserver: socket %d hung up\n", i);
	} else {
		perror("recv");
	}
	if (fds[i].state != STATE_PRE_REQUEST)
		free_fd_data(&fds[i]);
	fds[i].state = STATE_PRE_REQUEST;
	FD_CLR(i, master); /* remove from master set */
	close(i);
}

void select_loop(int listenfd)
{
	/* Thank you Brian "Beej Jorgensen" Hall */
	fd_set master;    // master file descriptor list
	fd_set read_fds;  // temp file descriptor list for select()
	int fdmax;        // maximum file descriptor number
	Request request;


	int nbytes;

	char remote_ip[INET6_ADDRSTRLEN];

	int i;

	FD_ZERO(&master);    /* clear the master and temp sets */
	FD_ZERO(&read_fds);

	/* add the listener to the master set */
	FD_SET(listenfd, &master);

	/* keep track of the biggest file descriptor */
	fdmax = listenfd; /* so far, it's this one */

	FdData fdDataList[MAX_FD_SIZE];
	for (i = 0;i<fdmax;i++) {
		fdDataList[i].state = STATE_PRE_REQUEST;
	}

	/* main loop */
	int idx,j,len;

	while(1) {
		read_fds = master; /* copy it */
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}

		/* run through the existing connections looking for data to read */
		for(i = 0; i <= fdmax; i++) {
			if (!FD_ISSET(i, &read_fds)) // we got one!!
				continue;
			if (i == listenfd) {
				accept_connection(fdDataList, i, listenfd, remote_ip, &fdmax, &master);
				break;
			}
			nbytes = recv(i, fdDataList[i].readBuffer+fdDataList[i].readBufferIdx, MAX_REQUEST_SIZE - fdDataList[i].readBufferLen, 0);
			/* read failure */
			if (nbytes <= 0) {
				shutdown_connection(fdDataList, i, nbytes, &master);
				break;
			}

			fdDataList[i].readBufferLen += nbytes;

			/* we got some data from a client */
			if (fdDataList[i].state == STATE_PRE_REQUEST) {
				fdDataList[i].state = STATE_METHOD;
			}

			if (fdDataList[i].state == STATE_METHOD) {
				idx = fdDataList[i].methodIdx;
				j = fdDataList[i].readBufferIdx;
				len = fdDataList[i].readBufferLen;

				while (j<len && idx<MAX_METHOD_SIZE)
				{
					ON_SLASH_N_TERMINATE_STRING_CHANGE_STATE(fdDataList[i].method,STATE_HEADER)
					else ON_SPACE_TERMINATE_STRING_CHANGE_STATE(fdDataList[i].method,STATE_URI)
					else ON_SLASH_R_IGNORE
					else ON_EVERYTHING_ELSE_CONSUME(fdDataList[i].method)
					j++;
				}

				fdDataList[i].methodIdx = idx;
				fdDataList[i].readBufferIdx = j;

				if (idx == MAX_METHOD_SIZE) {	/*We don't like really long methods. Cut em off */
					fdDataList[i].method[idx]=0;
					fdDataList[i].state = STATE_URI;
				}
			}

			if (fdDataList[i].state == STATE_URI) {
				idx = fdDataList[i].uriIdx;
				j = fdDataList[i].readBufferIdx;
				len = fdDataList[i].readBufferLen;

				while (j<len && idx<MAX_REQUEST_SIZE)
				{
					ON_SLASH_N_TERMINATE_STRING_CHANGE_STATE(fdDataList[i].uri,STATE_HEADER)
					else ON_SPACE_TERMINATE_STRING_CHANGE_STATE(fdDataList[i].uri,STATE_VERSION)
					else ON_SLASH_R_IGNORE
					else ON_EVERYTHING_ELSE_CONSUME(fdDataList[i].uri)
					j++;
				}

				fdDataList[i].uriIdx = idx;
				fdDataList[i].readBufferIdx = j;

				if (idx == MAX_REQUEST_SIZE) {	/*We don't like really long URIs either. Cut em off */
					fdDataList[i].uri[idx]=0;
					fdDataList[i].state = STATE_VERSION;
				}
			}

			if (fdDataList[i].state == STATE_VERSION) {
				idx = fdDataList[i].verIdx;
				j = fdDataList[i].readBufferIdx;
				len = fdDataList[i].readBufferLen;

				while (j<len && idx<MAX_VER_SIZE)
				{
					ON_SLASH_N_TERMINATE_STRING_CHANGE_STATE(fdDataList[i].ver,STATE_HEADER)
					else ON_SLASH_R_IGNORE
					else ON_EVERYTHING_ELSE_CONSUME(fdDataList[i].ver)
					j++;
				}
				fdDataList[i].verIdx = idx;
				fdDataList[i].readBufferIdx = j;

				/*We don't like really long version either. Cut em off */
				if (idx == MAX_VER_SIZE) {	/*We don't like really long URIs either. Cut em off */
					fdDataList[i].ver[idx]=0;
					fdDataList[i].state = STATE_HEADER;
				}
			}

			if (fdDataList[i].state == STATE_HEADER) {
				idx = fdDataList[i].withinHeaderIdx;
				j = fdDataList[i].readBufferIdx;
				len = fdDataList[i].readBufferLen;


				while (j<len) {
					if (fdDataList[i].readBuffer[j]=='\n') {
						if (idx == 0) {
							fdDataList[i].state = STATE_COMPLETE_READING;
							j++;
							break; /* The last of headers */
						}
						fdDataList[i].headers[fdDataList[i].headersIdx][idx]=0;
						if (fdDataList[i].headersIdx<MAX_HEADERS)
							fdDataList[i].headersIdx++;
						else {
							/* OK, buddy, you have sent us MAX_HEADERS headers. That's all yer get */
							fdDataList[i].state = STATE_COMPLETE_READING;
							j++;
							break;
						}
						fdDataList[i].headers[fdDataList[i].headersIdx]=NULL;
						idx = 0;
					} else if (fdDataList[i].readBuffer[j]=='\r') {
						/*Skip over \r */
					} else {
						if (idx == 0)
							fdDataList[i].headers[fdDataList[i].headersIdx]=malloc(MAX_BUFFER_SIZE*sizeof(char));
						fdDataList[i].headers[fdDataList[i].headersIdx][idx] = fdDataList[i].readBuffer[j];
						idx++;
					}
					j++;
				}

				fdDataList[i].withinHeaderIdx = idx;
				fdDataList[i].readBufferIdx = j;
			}

			if (fdDataList[i].state == STATE_COMPLETE_READING) {
				request.client = i;
				request.reqStr = fdDataList[i].uri;
				request.method = fdDataList[i].method;
				request.headers = fdDataList[i].headers;
				server(request);
				close(i);

				http_request req;
				snprintf(req.filename,sizeof(req.filename)-1,"%s",fdDataList[i].uri);
				/* log_access(STATUS_HTTP_OK, remote_ip, request); */
				if (fdDataList[i].state!=STATE_PRE_REQUEST)
					free_fd_data(&fdDataList[i]);
				fdDataList[i].state = STATE_PRE_REQUEST;
				printf("A job well done on %d\n",i);
				close(i); // bye!
				FD_CLR(i, &master); // remove from master set
			}
		} // END looping through file descriptors
	} // END for(;;)--and you thought it would never end!

	return;
}

int main(void){
	int i;
	int default_port = DEFAULT_PORT;
	int nChildren = DEFAULT_N_CHILDREN;
	int listenfd;

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
			select_loop(listenfd);
		} else if (pid > 0) {   //  parent
			printf("child pid is %d\n", pid);
		} else {
			perror("fork");
		}
	}

	select_loop(listenfd);
	return 0;
}
