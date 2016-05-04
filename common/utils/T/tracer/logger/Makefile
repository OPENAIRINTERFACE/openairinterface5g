CC=gcc
CFLAGS=-Wall -g -pthread -I..

OBJS=textlog.o

logger.a: $(OBJS)
	ar cr logger.a $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.a *.o
