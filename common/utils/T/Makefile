CC=gcc
CFLAGS=-Wall -g -Iincgen

GENIDS=genids
GENIDS_OBJS=genids.o

all : $(GENIDS) incgen/T_messages.txt.h incgen/T_IDs.h

$(GENIDS): $(GENIDS_OBJS)
	$(CC) $(CFLAGS) -o $(GENIDS) $(GENIDS_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

incgen/T_messages.txt.h: T_messages.txt
	mkdir -p incgen
	xxd -i T_messages.txt incgen/T_messages.txt.h

incgen/T_IDs.h: $(GENIDS) T_messages.txt
	mkdir -p incgen
	./$(GENIDS) T_messages.txt incgen/T_IDs.h

check_vcd: incgen/T_IDs.h incgen/T_messages.txt.h
	gcc -Wall -I. -I.. -I../itti -I../../../openair2/COMMON -Itracer -o _check_vcd check_vcd.c tracer/database.c tracer/utils.c -lm -pthread
	./_check_vcd T_messages.txt ../LOG/vcd_signal_dumper.h || (rm -rf ./_check_vcd ./incgen && false)
	rm -f ./_check_vcd

.PHONY: check_vcd

clean:
	rm -f *.o $(GENIDS) core _check_vcd
	rm -rf incgen

cleanall: clean
	cd tracer && $(MAKE) clean
