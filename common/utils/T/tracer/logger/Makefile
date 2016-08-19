CC=gcc
CFLAGS=-Wall -g -pthread -I..

OBJS=logger.o textlog.o framelog.o ttilog.o timelog.o ticklog.o iqlog.o \
     iqdotlog.o

logger.a: $(OBJS)
	ar cr logger.a $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.a *.o
