CC=gcc
CFLAGS=-Wall -g -pthread -DT_TRACER -I.

#CFLAGS += -O3 -ffast-math -fomit-frame-pointer

LIBS=-lX11 -lm -lpng -lXft

all: record replay extract_config textlog enb ue vcd macpdu2wireshark

record: utils.o record.o database.o config.o
	$(CC) $(CFLAGS) -o record $^ $(LIBS)

replay: utils.o replay.o
	$(CC) $(CFLAGS) -o replay $^ $(LIBS)

extract_config: extract_config.o
	$(CC) $(CFLAGS) -o extract_config $^ $(LIBS)

textlog: utils.o textlog.o database.o event.o handler.o config.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o textlog $^ $(LIBS)

enb: utils.o enb.o database.o event.o handler.o config.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o enb $^ $(LIBS)

ue: utils.o ue.o database.o event.o handler.o config.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o ue $^ $(LIBS)

vcd: utils.o vcd.o database.o event.o handler.o config.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o vcd $^ $(LIBS)

macpdu2wireshark: macpdu2wireshark.o database.o utils.o handler.o event.o \
                  config.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

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
	rm -f *.o core tracer_remote textlog enb vcd record replay
	rm -f extract_config macpdu2wireshark
	cd gui && make clean
	cd view && make clean
	cd logger && make clean
	cd filter && make clean
	cd hacks && make clean
