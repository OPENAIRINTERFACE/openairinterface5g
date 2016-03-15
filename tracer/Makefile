CC=gcc
CFLAGS=-Wall -g -pthread

LIBS=-lX11 -lm

#comment those two lines to NOT use shared memory
CFLAGS += -DT_USE_SHARED_MEMORY
LIBS += -lrt

PROG=tracer
OBJS=main.o plot.o

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: ../T_IDs.h ../T_defs.h

clean:
	rm -f *.o $(PROG) core
