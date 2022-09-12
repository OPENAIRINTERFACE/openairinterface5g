CC=gcc
CFLAGS=-Wall -g --coverage -I. -I../../../..

LIB=nr_rlc_entity.o nr_rlc_entity_am.o nr_rlc_entity_um.o nr_rlc_entity_tm.o \
    nr_rlc_pdu.o nr_rlc_sdu.o time_stat.o

tests:
	@./run_tests.sh

all: clean_run $(TEST).run

%.run: $(TEST).bin
	#valgrind ./$(TEST).bin > $(TEST).run_pre 2> $(TEST).valgrind
	./$(TEST).bin > $(TEST).run_pre
	grep ^TEST $(TEST).run_pre > $(TEST).run
	gunzip -c $(TEST).txt.gz > $(TEST).txt
	diff -q $(TEST).txt $(TEST).run

$(TEST).bin: $(TEST).o $(LIB)
	$(CC) $(CFLAGS) -o $@ $^

%.o: ../%.c
	$(CC) $(CFLAGS) -I.. -c -o $@ $<

$(TEST).o: test.c
	$(CC) $(CFLAGS) -c -o $@ $< -DTEST='"$(TEST).h"'

clean_run:
	rm -f $(TEST).run $(TEST).bin $(TEST).o

clean:
	rm -f *.o *.bin *.run *.run_pre *.gcov *.gcda *.gcno test*.txt a.out \
		*.valgrind
