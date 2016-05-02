CC=gcc
CFLAGS=-Wall -g -pthread -DT_TRACER -DT_USE_SHARED_MEMORY -I.

PROG=tracee
OBJS=tracee.o ../T.o

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) -lrt

tracee.o: tracee.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(PROG) core
