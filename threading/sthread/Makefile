#
## Makefile for SThreads
#
#
#CC=gcc
CFLAGS=-g -O2 -Wall
LDFLAGS=-g

.c.s:
	$(CC) $(CCFLAGS) $(CPPFLAGS) -S $<

libsthread.a: sthread.o sync.o
	$(AR) cr $@ $cr^

test: test.o libsthread.a
	$(CC) $(LDFLAGS) -o $@ test.o -L. -lsthread

clean:
	$(RM) libsthread.a test *.o *~
