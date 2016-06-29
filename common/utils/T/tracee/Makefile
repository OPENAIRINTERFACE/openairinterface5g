CC=gcc
CFLAGS=-Wall -g -pthread -DT_TRACER -I.

PROG=tracee
OBJS=tracee.o ../T.o ../local_tracer.o

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) -lrt

tracee.o: tracee.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(PROG) core
