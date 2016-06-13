CC=gcc
CFLAGS=-Wall -g

GENIDS=genids
GENIDS_OBJS=genids.o

all : $(GENIDS) T_messages.txt.h T_IDs.h

$(GENIDS): $(GENIDS_OBJS)
	$(CC) $(CFLAGS) -o $(GENIDS) $(GENIDS_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

T_messages.txt.h: T_messages.txt
	xxd -i T_messages.txt > T_messages.txt.h

T_IDs.h: $(GENIDS) T_messages.txt
	./$(GENIDS) T_messages.txt T_IDs.h

clean:
	rm -f *.o $(GENIDS) core T_IDs.h T_messages.txt.h
	cd tracer && make clean
