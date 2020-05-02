C=gcc
CFLAGS=-W -Wall -mavx2
LDFLAGS=
EXEC=cnProc_gen
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)

all: $(EXEC)

cnProc_gen: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

main.o: cnProc_gen.h

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

zip: 
	tar -zcvf sauvegarde.tar.gz main.c cnProc_gen.c cnProc_geno.h Makefile
