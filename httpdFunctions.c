#include "httpdHeader.h"



/* This function takes a socket fd and a file size and
   sends an ok/default header to socket (with file size)*/
void sendHeader( int fd, off_t fileSize) {
   char buf[128];
   sprintf(buf,
	   "HTTP/1.0 200 OK\r\n"
	   "Content-Type: text/html\r\n"
	   "Content-Length: %ld\r\n"
	   "\r\n", fileSize);

   send(fd, &buf, strlen(buf), 0); /* to remote */
}


/* Takes a socket fd and an error code and sends corresponding
   header message.
   TODO: improvements-- Have it send a nice error page.
         make buf dynamic? (not really necessary though)
 */
void sendError( int fd, int error) {
   char buf[128];		/* #!@! make sure size of buf is big enough */
   char *errorMsg;
   switch(error) {
   case 400:
      errorMsg = "Bad Request";
      break;
   case 403:
      errorMsg = "Permission Denied";
      break;
   case 404:
      errorMsg = "Not Found";
      break;
   case 500:
      errorMsg = "Internal Error";
      break;
   case 501:
      errorMsg = "Not Implemented";
      break;
   default:
      errorMsg = "Unknown Error";
      break;
   }
   
   
   sprintf(buf,
	   "HTTP/1.0 %i %s\r\n"
	   "Content-Type: text/html\r\n"
	   "Content-Length: 0\r\n"
	   "\r\n" , error, errorMsg);

   send(fd, &buf, strlen(buf), 0); /* to remote */
}



/* Takes in a string in the format of:
   ./cgi-like/ls?-l&index.html&main.html HTTP/1.1
   processes to generate array for exec, then 
   forks, changes stdout to temp file, then sendsfile
   ** It does not close the connection
 */
void cgilike(int fd, char *cmd, int isGet) {
   char **args, *fname;
   int i, size, status, tmpf;
   struct stat buf;
   pid_t pid;

   i = 0;
   size = 32;			/* Dynamic */
   args = cmalloc(sizeof(char*) * size);

   cmd = args[i++] = strtok(cmd, "?");
   /* Generate array*/
   while( (args[i] = strtok(NULL, "& ")) != NULL ) {
      if(i + 2 >= size) args = crealloc(args, sizeof(char*) * (size*=2)); /* increase array */
      i++;
   }
   args[i++] = '\0';

   /*********** gen/open filename  ***********/
   fname = cmalloc(16 * sizeof(char));
   if( sprintf(fname, ".%ld.tmp", (long int) getpid()) < 0) { /* IF sprint fails, error out */
      perror("Could not generate a filename");
      sendError(fd, 500);
      return;
   }
   if( (tmpf = open( fname, O_RDWR | O_CREAT, 0666)) < 0) {
   	 perror("couldn't open tmp file");
   	 sendError(fd, 500);
   	 exit(-1);
   }

   /*********** fork ***********/
   if( (pid = fork()) < 0) {
      /* Send error - bad fork */
      perror("bad fork cgi-like");
      sendError(fd, 500);  return;
      
   } else if(pid == 0) {     /* CHILD *******************************/
      dup2(tmpf, 1);	     /* Dup stdin and std err to file */
      dup2(tmpf, 2);	 

      execv(cmd, args);
      /* If exec fails, do this stuff vvv */
      free(args);
      close(tmpf);
      exit( 17 );
      
   } else {        	/********************* Parent *********************/
      free(args);	/* free args since don't use in parent */
      wait(&status);	/* wait for exec to finish */
      if( WIFEXITED(status) && (WEXITSTATUS(status) == 17) ) {	/* If exec failed*/
	 close(tmpf);
	 remove(fname);
	 sendError(fd, 404); return;				/* was 500, but it makes more sense to do 404 because otherwise the command sends its own error code */
      } 

      lseek(tmpf, 0, SEEK_SET);	/* seek to beginning of file */
      fstat(tmpf, &buf);	/* get size */
      sendHeader(fd, buf.st_size);
      if(isGet)			
	 sendfile(fd, tmpf, NULL, buf.st_size);
      close(tmpf);
      remove(fname);
      free(fname);
   }
   
}




/* Checked version of malloc */
void *cmalloc(size_t size) {
   void *p = malloc(size);
   if(p==NULL) {
      perror("cmalloc");
      exit(-1);
   }
   return p;
}

/* Checked version of calloc */
void *ccalloc(size_t nmemb, size_t size) {
   void *p = calloc(nmemb, size);
   if(p==NULL) {
      perror("ccalloc");
      exit(-1);
   }
   return p;
}
/* Checked version of realloc */
void *crealloc(void *ptr,size_t size) {
   void *p = realloc(ptr, size);
   if(p==NULL) {
      perror("crealloc");
      exit(-1);
   }
   return p;
}


/* Wrapper for printf so I can enable/disable debug printin */
void debug( char *s) {
   if(DEBUG)
      printf("\t\t?>>%s",s);
}



