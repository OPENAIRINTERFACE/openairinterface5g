CC=gcc
CFLAGS=-Wall -g -pthread -I..

OBJS=textlog.o framelog.o

logger.a: $(OBJS)
	ar cr logger.a $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.a *.o
