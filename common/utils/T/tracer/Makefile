CC=gcc
CFLAGS=-Wall -g -pthread -DT_TRACER -I.

#CFLAGS += -O3 -ffast-math -fomit-frame-pointer

LIBS=-lX11 -lm -lpng -lXft

all: textlog enb vcd

textlog: utils.o textlog.o database.o event.o handler.o config.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o textlog $^ $(LIBS)

enb: utils.o enb.o database.o event.o handler.o config.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o enb $^ $(LIBS)

vcd: utils.o vcd.o database.o event.o handler.o config.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o vcd $^ $(LIBS)

.PHONY: all gui/gui.a view/view.a logger/logger.a filter/filter.a

gui/gui.a:
	cd gui && make

view/view.a:
	cd view && make

logger/logger.a:
	cd logger && make

filter/filter.a:
	cd filter && make

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o core tracer_remote textlog enb vcd
	cd gui && make clean
	cd view && make clean
	cd logger && make clean
	cd filter && make clean
