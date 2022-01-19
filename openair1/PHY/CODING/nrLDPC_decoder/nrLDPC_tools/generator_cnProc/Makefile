C=gcc
CFLAGS=-W -Wall -mavx2 -DCODEGEN=1
LDFLAGS=
EXEC=cnProc_gen_avx2
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)

all: $(EXEC)

cnProc_gen_avx2: $(OBJ)
	@$(CC) -o $@ $^ $(LDFLAGS) -O2 

#main.o: cnProc_gen_avx2.h

%.o: %.c
	@$(CC) -o $@ -c $< $(CFLAGS) -I ${OPENAIR_HOME}/openair1 -I ${OPENAIR_HOME} -I ${OPENAIR_HOME}/common/utils -I ${OPENAIR_HOME}/common/utils/T -I ${OPENAIR_HOME}/openair2/COMMON -std=c99   

.PHONY: clean mrproper

clean:
	@rm -rf *.o

mrproper: clean
	@rm -rf $(EXEC)

zip: 
	@tar -zcvf sauvegarde.tar.gz main.c cnProc_gen_BG1_avx2.c cnProc_gen_BG2_avx2.c Makefile
