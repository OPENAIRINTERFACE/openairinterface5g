CC=gcc
CFLAGS=-Wall -g -pthread -I..

OBJS=stdout.o textlist.o xy.o tti.o

view.a: $(OBJS)
	ar cr view.a $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.a *.o
