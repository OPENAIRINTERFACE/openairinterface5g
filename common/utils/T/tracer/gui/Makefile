CC=gcc
CFLAGS=-Wall -g -pthread -I/usr/include/X11/Xft -I/usr/include/freetype2

OBJS=init.o loop.o toplevel_window.o x.o container.o widget.o \
     gui.o label.o textarea.o event.o xy_plot.o textlist.o notify.o \
     positioner.o timeline.o space.o image.o

gui.a: $(OBJS)
	ar cr gui.a $(OBJS)

test: test.o gui.a
	$(CC) -o test $(OBJS) test.o -lX11 -pthread -lm

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.a *.o test
