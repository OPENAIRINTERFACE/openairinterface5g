TURBO_SRC    = 3gpplte_sse.c 3gpplte_turbo_decoder_sse.c crc_byte.c
RATE13CC_SRC = ccoding_byte_lte.c viterbi_lte.c crc_byte.c
RATE12CC_SRC = ccoding_byte.c viterbi.c crc_byte.c

all: 3gpplte_sse 

3gpplte_sse: $(TURBO_SRC)
	gcc -o 3gpplte_sse 3gpplte_sse.c -msse4 -Wall -g -ggdb -DTC_MAIN -I../..



clean: 
	rm *.o
