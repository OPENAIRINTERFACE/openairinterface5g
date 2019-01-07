# compiles the example

OPENAIRTARGETS_DIR ?=../../../..

OPENAIROBJS += $(OPENAIRTARGETS_DIR)/ARCH/EXMIMO/USERSPACE/LIB/openair0_lib.o
CFLAGS += -I$(OPENAIRTARGETS_DIR)/ARCH/EXMIMO/USERSPACE/LIB -I$(OPENAIRTARGETS_DIR)/ARCH/EXMIMO/DEFS


example: example.o $(OPENAIROBJS) 
	gcc -o $@ $(CFLAGS) -lm $(OPENAIROBJS) $<


clean:
	rm -f *.o *~
	rm -f example
