CC=gcc
CFLAGS=-Wall -g -pthread -DT_TRACER -I. -I../incgen

PROG=tracee
OBJS=tracee.o T.o ../local_tracer.o

all: T_dep $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) -lrt

tracee.o: tracee.c
	$(CC) $(CFLAGS) -c -o $@ $<

T.o: ../T.c
	$(CC) $(CFLAGS) -c -o $@ $<

T_dep:
	cd .. && make

clean:
	rm -f *.o $(PROG) core
