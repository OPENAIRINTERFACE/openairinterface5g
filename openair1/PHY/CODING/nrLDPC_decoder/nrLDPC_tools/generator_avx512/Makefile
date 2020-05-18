C=gcc
CFLAGS=-W -Wall 
LDFLAGS=
EXEC=cnProc_gen_avx512
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)

all: $(EXEC)

cnProc_gen_avx512: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) -O2  -pg

main.o: cnProc_gen_avx512.h

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS) -I ${OPENAIR_HOME}/openair1 -g   -pg

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

zip: 
	tar -zcvf sauvegarde.tar.gz main.c cnProc_gen_avx512.c cnProc_gen_avx512.h Makefile
