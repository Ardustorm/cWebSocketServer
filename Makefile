CC = gcc
CFLAGS = -Wall -std=c99 -pedantic  -g
CFLAGS = -Wall  -pedantic -g
MAIN = httpd
OBJS =  httpdFunctions.o httpd.o simple_net.o
all : $(MAIN)

$(MAIN) : $(OBJS) httpdFunctions.o httpdHeader.h
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS)


httpd.o : httpd.c httpdHeader.h
	$(CC) $(CFLAGS) -c httpd.c

simple_net.o : simple_net.c
	$(CC) $(CFLAGS) -c simple_net.c

httpdFunctions.o : httpdFunctions.c httpdHeader.h
	$(CC) $(CFLAGS) -c httpdFunctions.c

clean :
	rm *.o $(MAIN) 

test:	$(MAIN)
	./httpd
