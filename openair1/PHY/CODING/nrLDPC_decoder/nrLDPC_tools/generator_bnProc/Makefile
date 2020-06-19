C=gcc
CFLAGS=-W -Wall -mavx2
LDFLAGS=
EXEC=bnProc_gen_avx2
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)

all: $(EXEC)

bnProc_gen_avx2: $(OBJ)
	@$(CC) -o $@ $^ $(LDFLAGS) -O2 



%.o: %.c
	@$(CC) -o $@ -c $< $(CFLAGS) -I ${OPENAIR_HOME}/openair1 -g -std=c99  

.PHONY: clean mrproper

clean:
	@rm -rf *.o

mrproper: clean
	@rm -rf $(EXEC)

zip: 
	@tar -zcvf sauvegarde.tar.gz main.c bnProcPc_gen_BG1_avx2.c bnProcPc_gen_BG2_avx2.c  bnProc_gen_BG1_avx2.c  bnProc_gen_BG2_avx2.c  Makefile
