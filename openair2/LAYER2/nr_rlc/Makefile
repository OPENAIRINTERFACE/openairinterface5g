CC=gcc
CFLAGS=-Wall -g -Itests

OBJS=nr_rlc_entity.o nr_rlc_entity_am.o nr_rlc_entity_um.o \
     nr_rlc_entity_tm.o nr_rlc_sdu.o nr_rlc_pdu.o test.o
PROG=test

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f test *.o
