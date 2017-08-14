
ifndef PROJDIR
PROJDIR = $(PWD)/../../..
endif

include $(PROJDIR)/Makerules
include $(PROJDIR)/Makefile.inc

LIBS		= -lutil
INCLUDES	= -I. -I$(INCDIR) -I$(UTILDIR)

LIBSUTIL	= $(LIBDIR)/$(LIBUTIL).a $(LIBDIR)/$(LIBUTIL).so

TIMER_OBJ	= timer.o

TIMER_TARGET	= timer

TARGETS = $(TIMER_TARGET)

all: $(TARGETS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@ 

$(TIMER_TARGET): $(TIMER_OBJ) $(LIBSUTIL)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) $(OBJS) $(TARGETS) *.bak *~

depend:
	makedepend -- ${CFLAGS} -- ${SRCS}

# DO NOT DELETE THIS LINE -- make depend depends on it.

timer.o: $(UTILDIR)/timer.h $(INCDIR)/commonDef.h
