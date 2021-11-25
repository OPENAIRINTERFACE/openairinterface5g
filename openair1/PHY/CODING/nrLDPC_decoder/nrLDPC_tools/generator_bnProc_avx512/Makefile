C=gcc
CFLAGS=-W -Wall -mavx2
LDFLAGS=
EXEC=bnProc_gen_avx512
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)

all: $(EXEC)

bnProc_gen_avx512: $(OBJ)
	@$(CC) -o $@ $^ $(LDFLAGS) -O2 



%.o: %.c
	@$(CC) -o $@ -c $< $(CFLAGS) -I ${OPENAIR_HOME}/openair1 -g -std=c99  

.PHONY: clean mrproper

clean:
	@rm -rf *.o

mrproper: clean
	@rm -rf $(EXEC)

zip: 
	@tar -zcvf sauvegarde.tar.gz main.c  bnProc_gen_BG1_avx512.c  bnProc_gen_BG2_avx512.c  Makefile
