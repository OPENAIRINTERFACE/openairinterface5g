CC=gcc
CFLAGS=-Wall -g -pthread -DT_TRACER

#comment those two lines to NOT use shared memory
CFLAGS += -DT_USE_SHARED_MEMORY
LIBS += -lrt

PROG=t
OBJS=main.o T.o

GENIDS=genids
GENIDS_OBJS=genids.o

ALL=$(PROG) $(GENIDS)

all : $(ALL)

$(GENIDS): $(GENIDS_OBJS)
	$(CC) $(CFLAGS) -o $(GENIDS) $(GENIDS_OBJS)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

T.o: T_messages.txt.h

T_messages.txt.h: T_messages.txt
	xxd -i T_messages.txt > T_messages.txt.h

T_IDs.h: $(GENIDS) T_messages.txt
	./$(GENIDS) T_messages.txt T_IDs.h

main.o: T.h T_IDs.h T_defs.h

clean:
	rm -f *.o $(PROG) $(GENIDS) core T_IDs.h T_messages.txt.h
	cd tracer && make clean
