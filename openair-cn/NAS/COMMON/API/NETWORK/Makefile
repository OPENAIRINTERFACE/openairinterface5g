ifndef PROJDIR
PROJDIR  = $(PWD)/../../..
INCLUDES = -I. -I$(INCDIR) -I$(UTILDIR) -I$(IESDIR) -I$(EMMMSGDIR) -I$(ESMMSGDIR) 
endif

include $(PROJDIR)/Makerules
include $(PROJDIR)/Makefile.inc

all: $(OBJS)

%.o: %.c Makefile
	@echo Compiling $<
	@$(CC) $(CFLAGS) $(INCLUDES)   -c $< -o $@

clean:
	$(RM) $(OBJS) *.bak *~

depend:
	makedepend -- ${CFLAGS} -- ${SRCS}

# DO NOT DELETE THIS LINE -- make depend depends on it.

