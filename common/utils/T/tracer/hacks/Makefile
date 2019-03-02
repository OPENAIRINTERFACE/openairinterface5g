CC=gcc
CFLAGS=-Wall -g -pthread -DT_TRACER -I. -I..

LIBS=-lX11 -lm -lpng -lXft

all: dump_nack_signal time_meas timeplot multi-rru-clean

dump_nack_signal: ../utils.o ../database.o ../config.o ../event.o \
                  dump_nack_signal.o
	$(CC) $(CFLAGS) -o dump_nack_signal $^ $(LIBS)

time_meas: ../utils.o ../database.o ../config.o ../event.o \
                  time_meas.o
	$(CC) $(CFLAGS) -o time_meas $^ $(LIBS)

timplot: timeplot.o
	$(CC) $(CFLAGS) -o timeplot $^ $(LIBS)

multi-rru-clean: ../utils.o ../database.o ../event.o ../config.o multi-rru-clean.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY: all

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o core dump_nack_signal time_meas timeplot
