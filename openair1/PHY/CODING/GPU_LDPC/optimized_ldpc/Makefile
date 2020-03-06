EXEC = gen_idx
TXT = gen_matrix.txt
DEBUG = debug
FILE = channel_output.txt
num = 1
CFLAGS=

gen: clean gen_idx.c
	$(CC) $(EXEC).c -o $(EXEC) -g

ldpc: clean ldpc.cu 
	cp ../test_input/8448/$(num).txt $(FILE)
	nvcc $(CFLAGS) ldpc.cu -o ldpc
	mkdir debug

test: ldpc
	./check.sh

cgdb: CFLAGS += -g -G
cgdb: clean ldpc
	cuda-gdb --silent --args ldpc $(FILE)

prof: clean ldpc
	cp ../test_input/8448/$(num).txt $(FILE)
	nvprof ./ldpc $(FILE)

.PHONY: clean
clean:
	$(RM) -rf $(DEBUG) $(EXEC) $(TXT) ldpc log.txt $(FILE)
