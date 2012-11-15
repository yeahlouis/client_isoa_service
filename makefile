CC=gcc
LDFLAGS= -g -Wall
all: isoaclient tcp_server

clean:
		@rm -f *.o isoaclient
