CC=gcc
CFLAGS=-Wall -g -pthread -DT_TRACER -I. -I../incgen

#CFLAGS += -O3 -ffast-math -fomit-frame-pointer

LIBS=-lm
XLIBS=-lX11 -lpng -lXft

all: record replay extract_config textlog enb ue vcd macpdu2wireshark \
     extract_input_subframe extract_output_subframe to_vcd extract multi \
     gnb

record: utils.o record.o database.o configuration.o
	$(CC) $(CFLAGS) -o record $^ $(LIBS)

replay: utils.o replay.o
	$(CC) $(CFLAGS) -o replay $^ $(LIBS)

extract_config: extract_config.o
	$(CC) $(CFLAGS) -o extract_config $^ $(LIBS)

extract_input_subframe: extract_input_subframe.o database.o event.o utils.o \
    configuration.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

extract_output_subframe: extract_output_subframe.o database.o event.o utils.o \
    configuration.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

extract: extract.o database.o event.o utils.o configuration.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

textlog: utils.o textlog.o database.o event.o handler.o configuration.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o textlog $^ $(LIBS) $(XLIBS)

enb: utils.o enb.o database.o event.o handler.o configuration.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o enb $^ $(LIBS) $(XLIBS)

ue: utils.o ue.o database.o event.o handler.o configuration.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o ue $^ $(LIBS) $(XLIBS)

vcd: utils.o vcd.o database.o event.o handler.o configuration.o \
         event_selector.o view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o vcd $^ $(LIBS) $(XLIBS)

to_vcd: to_vcd.o database.o event.o handler.o utils.o configuration.o \
         logger/logger.a filter/filter.a
	$(CC) $(CFLAGS) -o to_vcd $^ $(LIBS)

macpdu2wireshark: macpdu2wireshark.o database.o utils.o handler.o event.o \
                  configuration.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

multi: multi.o utils.o database.o configuration.o
	$(CC) $(CFLAGS) -o multi $^ $(LIBS)

gnb: utils.o gnb.o database.o event.o handler.o configuration.o \
         view/view.a gui/gui.a logger/logger.a \
         filter/filter.a
	$(CC) $(CFLAGS) -o gnb $^ $(LIBS) $(XLIBS)

multi.o: ../incgen/T_IDs.h

../incgen/T_IDs.h: ../T_messages.txt
	cd .. && $(MAKE)

.PHONY: all gui/gui.a view/view.a logger/logger.a filter/filter.a

gui/gui.a:
	cd gui && $(MAKE)

view/view.a:
	cd view && $(MAKE)

logger/logger.a:
	cd logger && $(MAKE)

filter/filter.a:
	cd filter && $(MAKE)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o core tracer_remote textlog enb ue vcd record replay
	rm -f extract_config macpdu2wireshark extract_input_subframe
	rm -f extract_output_subframe to_vcd extract multi gnb
	cd gui && $(MAKE) clean
	cd view && $(MAKE) clean
	cd logger && $(MAKE) clean
	cd filter && $(MAKE) clean
	cd hacks && $(MAKE) clean
