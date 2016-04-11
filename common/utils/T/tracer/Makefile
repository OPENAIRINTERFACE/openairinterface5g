CC=gcc
CFLAGS=-Wall -g -pthread -DT_TRACER

#CFLAGS += -O3 -ffast-math -fomit-frame-pointer

LIBS=-lX11 -lm

#comment those two lines to NOT use shared memory
CFLAGS += -DT_USE_SHARED_MEMORY
LIBS += -lrt

PROG=tracer
OBJS=main.o plot.o database.o forward.o gui.o

$(PROG): $(OBJS) gui/gui.a
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LIBS) gui/gui.a

.PHONY: gui/gui.a

gui/gui.a:
	cd gui && make

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: ../T_IDs.h ../T_defs.h

clean:
	rm -f *.o $(PROG) core
	cd gui && make clean
