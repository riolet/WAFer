#ifndef NOPE_H_
#define NOPE_H_

/* Colors. Why not ? */
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET   "\033[0m"


/* Define boolean */
typedef int bool;
#define true 1
#define false 0

#define SERVER_STRING "Server: nope.chttpd/0.1.0\r\n"
#define ToHex(Y) (Y>='0'&&Y<='9'?Y-'0':Y-'A'+10)
#define UNDEFINED "VALUE_UNDEFINED"

/* Settings */
#define NOPE_ONE_K 1024
#define MAX_HEADERS 1024
#define MAX_BUFFER_SIZE 1024
#define MAX_DPRINTF_SIZE 64
#ifdef NOPE_MAX_CON_CONS
#define MAX_NO_FDS NOPE_MAX_CON_CONS
#else
#define MAX_NO_FDS 1024
#endif
#define MAX_METHOD_SIZE 64
#define MAX_VER_SIZE 64
#define MAX_REQUEST_SIZE 8192
#define MAX_EVENTS MAX_NO_FDS/2
#define POLL_TIMEOUT 1000 /*1 second */
/* Define HTTP request parsing states */
#define STATE_PRE_REQUEST 0
#define STATE_METHOD 1
#define STATE_URI 2
#define STATE_VERSION 3
#define STATE_HEADER 4
#define STATE_COMPLETE_READING 5

#define STATUS_HTTP_OK 200

typedef struct struct_request {
    int client;
    char *reqStr;
    char *method;
    char **headers;
} Request;

typedef struct {
	short int state;
	char *readBuffer;
	short readBufferIdx;
	short readBufferLen;
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

#define LISTENQ  1024           /* second argument to listen() */
#define MAXLINE 1024            /* max length of a line */
#define RIO_BUFSIZE 1024

#define DEFAULT_PORT 4242
#define DEFAULT_N_CHILDREN 0

/*Preprocessor abuse */
#define SIZE_OF_CHAR sizeof(char)
#define LOG_ERROR_ON(_statement_,_condition_,_message_) do { if ((_statement_)==_condition_) fprintf(stderr,_message_); } while(0)
#define LOG_ERROR_ON_NULL(_statement_,_message_) LOG_ERROR_ON(_statement_,NULL,_message_)
#define NEW(T,v) do { T * v =  malloc(sizeof(T)); } while (0)
/* Globals */
int default_port;

/*Thread stuff!*/
#ifdef NOPE_THREADS
#define QUEUESIZE MAX_NO_FDS

/* select_loop stuff */
typedef struct {
	Request request;
	int fd;
	FdData *fdDataList;
	fd_set *pMaster;
} THREAD_DATA;

typedef struct {
	THREAD_DATA buf[QUEUESIZE];
	long head, tail;
	int full, empty;
	pthread_mutex_t *mut;
	pthread_cond_t *notFull, *notEmpty;
} queue;

pthread_mutex_t *fd_mutex;

/*Globals*/
queue * fifo;
queue * cleaner_fifo;
int socketpair_fd[2];

/*Functions*/
void farmer_thread (THREAD_DATA);
void *worker_thread (void *arg);
void cleaner_thread (void);
queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, THREAD_DATA in);
void queueDel (queue *q, THREAD_DATA *out);
#endif

void freeHeaders(char **);
long dbgprintf(const char *format, ...);

void server(Request request);

#endif                          /* NOPE_H_ */
