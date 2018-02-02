CC=gcc
CFLAGS=-Wall -g -pthread -DT_TRACER -I.

#CFLAGS += -O3 -ffast-math -fomit-frame-pointer

LIBS=-lm
XLIBS=-lX11 -lpng -lXft

all: record replay extract_config textlog enb ue vcd macpdu2wireshark \
     extract_input_subframe extract_output_subframe to_vcd extract

record: utils.o record.o database.o config.o
	$(CC) $(CFLAGS) -o record $^ $(LIBS)

replay: utils.o replay.o
	$(CC) $(CFLAGS) -o replay $^ $(LIBS)

extract_config: extract_config.o
	$(CC) $(CFLAGS) -o extract_config $^ $(LIBS)

extract_input_subframe: extract_input_subframe.o database.o event.o utils.o \
    config.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

extract_output_subframe: extract_output_subframe.o database.o event.o utils.o \
    config.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

extract: extract.o database.o event.o utils.o config.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

textlog: utils.o textlog.o database.o event.o handler.o config.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o textlog $^ $(LIBS) $(XLIBS)

enb: utils.o enb.o database.o event.o handler.o config.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o enb $^ $(LIBS) $(XLIBS)

ue: utils.o ue.o database.o event.o handler.o config.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o ue $^ $(LIBS) $(XLIBS)

vcd: utils.o vcd.o database.o event.o handler.o config.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o vcd $^ $(LIBS) $(XLIBS)

to_vcd: to_vcd.o database.o event.o handler.o utils.o config.o \
         logger/logger.a filter/filter.a
	$(CC) $(CFLAGS) -o to_vcd $^ $(LIBS)

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
	rm -f *.o core tracer_remote textlog enb ue vcd record replay
	rm -f extract_config macpdu2wireshark extract_input_subframe
	rm -f extract_output_subframe to_vcd extract
	cd gui && make clean
	cd view && make clean
	cd logger && make clean
	cd filter && make clean
	cd hacks && make clean
