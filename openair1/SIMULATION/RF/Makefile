include $(OPENAIR_DIR)/common/utils/Makefile.inc
OBJS = rf.o ../../PHY/TOOLS/file_output.o ../TOOLS/rangen_double.o
CFLAGS += -DRF_MAIN -DDEBUG_PHY 

all: $(OBJS)
	gcc -o rf $(OBJS) -lm 

$(OBJ) : %.o : %.c 
	$(CC) -c $(CFLAGS) -Wall -I$(TOP_DIR) -o $@ $<

clean:
	rm -f $(OBJS)
