
#ifndef __sfindHeader
#define __sfindHeader

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>


/* Signal Processing */
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include <sys/socket.h>


#define DEBUG 1
#define STARTING_LENGTH 20

/* Simple Net */
int create_service(unsigned short port, int queue_size);
int accept_connection(int fd);

/* HTTPD stuff */

void sendHeader( int fd, long fileSize);
void sendError( int fd, int error);
void handleRequest(int con);
void cgilike(int fd, char *cmd, int isGet);

void *crealloc(void *ptr,size_t size);
void *cmalloc(size_t size);
void *ccalloc(size_t nmemb, size_t size);
void debug( char *s);
#endif
