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
	xxd -i T_messages.txt T_messages.txt.h

T_IDs.h: $(GENIDS) T_messages.txt
	./$(GENIDS) T_messages.txt T_IDs.h

check_vcd: T_IDs.h T_messages.txt.h
	gcc -Wall -I. -I.. -I../itti -I../../../openair2/COMMON -Itracer -o _check_vcd check_vcd.c tracer/database.c tracer/utils.c -lm -pthread
	./_check_vcd || (rm -f ./_check_vcd ./T_IDs.h ./T_messages.txt.h && false)
	rm -f ./_check_vcd

.PHONY: check_vcd

clean:
	rm -f *.o $(GENIDS) core T_IDs.h T_messages.txt.h _check_vcd

cleanall: clean
	cd tracer && $(MAKE) clean
