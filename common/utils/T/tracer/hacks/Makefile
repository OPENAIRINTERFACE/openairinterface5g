CC=gcc
CFLAGS=-Wall -g -pthread -DT_TRACER -I. -I..

LIBS=-lX11 -lm -lpng -lXft

all: dump_nack_signal

dump_nack_signal: ../utils.o ../database.o ../config.o ../event.o \
                  dump_nack_signal.o
	$(CC) $(CFLAGS) -o dump_nack_signal $^ $(LIBS)

.PHONY: all

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o core dump_nack_signal
