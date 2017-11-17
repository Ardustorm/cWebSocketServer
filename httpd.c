
#include "httpdHeader.h"

#define BUFSIZE 4096
#define QUEUESIZE 10
#define PORT 4180

/* 
TODO:
  Make buffer dynamic (using fdopen?)
*/


static void handle_sigchld(int sig);

int main(int argc, char **argv) {
   int sock, con, port;
   struct sigaction sa;

   /* GET PORT NUMBER */
   if(argc == 2) {
      port=strtol(argv[1], NULL, 10);
   } else if(argc < 2) {
      fprintf(stderr, "Expected port argument, defaulting to %d\n", PORT);
      port = PORT;
   } else {
      fprintf(stderr, "To many arguments.\nUsage:\thttpd PORTNUM\n");
      exit(-1);
   }
   
   
   /*********** SIGACTION handler ***********/
   sa.sa_handler = &handle_sigchld;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
   if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror(0);
      exit(1);
   }
   /********************************/

   sock = create_service(port, QUEUESIZE);

   while(1) {
      con = accept_connection(sock);
      handleRequest(con);
   }
   
   return -1;
}



/* 
con is the fd for the oppen socket between server and host.
This function waits for request from user and processes it,
calling necessary function, and closes the socket when done.
 */
void handleRequest(int con) {
   char buf[BUFSIZE], *type, *filename, *version, *line;
   struct stat statBuf;
   int size, fd, isHead, isGet, isCgi;
   off_t fileSize;
   pid_t pid;
   isHead = isGet = isCgi = 0;
   if( (pid = fork()) < 0) {
      /* Send error - bad fork */
      sendError(con, 500);
      fprintf(stderr,"bad fork handleRequest\n");
      close(con);

   } else if(pid == 0) {		/* CHILD *******************************/

      size = recv(con, &buf, BUFSIZE, 0);

      /* printf(buf); */
      if(size < 0) printf("ERROR\n");
      /**************** process input ***********/
      if( !*buf ||			    /* Make sure buf is not null */
	 !(type = strtok(buf, " ")) ||      /* Type of command */
	 !(filename = strtok(NULL, " ")) || /* what filename is provided */
	 !(version = strtok(NULL, " \n"))){ /* makes sure version exists  */
	 sendError(con, 400);
	 close(con); return;
      }

      /*
      while( (line = strtok(NULL, "\n")) ) {
	 printf("\n>> ");
	 printf(line);
      }
      */
      
      /* Checks to make sure not trying to access files above  */
      if( strstr(filename, "..") ) { 
	 sendError(con, 403);
	 close(con); return;
      }

      /************** GET ARGUMENTS ******************/
      if( strcmp(type, "HEAD") == 0)
	 isHead = 1;
      if( strcmp(type, "GET") == 0)
	 isGet = 1;
      if( strstr(filename, "cgi-like"))
	 isCgi = 1; 
      if( isHead == isGet) { 	/* Makes sure at least 1 is valid */
	 sendError(con, 501);
	 close(con); return;
      }
      
      /* get file stats */
      *(filename-2)= '.';	/* prepends  './' overwriting buf */
      *(filename-1)= '/';
      if( !isCgi &&  ((stat(filename-2, &statBuf)) < 0 || S_ISDIR(statBuf.st_mode)) ) { /* fn-2 to start at prepended  './' */
	 sendError(con,404);	/* file not found */
	 close(con); return;
      }

      fileSize = statBuf.st_size;


      if( isHead && !isCgi) { /*********************HEAD request ********** */
	 sendHeader(con, fileSize);

      } else if( isGet  && !isCgi) { /*********************GET request ********** */
	 fd = open(filename-2, O_RDONLY); /* fn-2 to start at appended '.' */
	 if(fd < 0) {
	    sendError(con,403);	/* Permission denied*/
	    close(con); return;
	 }
	 sendHeader(con, fileSize);
	 sendfile(con, fd, NULL, fileSize);

      } else if( isCgi ) { /********************* cgi-like ********** */
	 cgilike(con, filename-2, isGet);
      }

      close(con);
      exit(0);
      
   } else {   /******** PARENT ************/
      close(con);
   }
}








/* http://www.microhowto.info/howto/reap_zombie_processes_using_a_sigchld_handler.html
 */
static void handle_sigchld(int sig) {
  int saved_errno;
   /* saves error num so we can restore later */
  saved_errno = errno;
  /* use a loop so we can use no hang */
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
  errno = saved_errno;
}
