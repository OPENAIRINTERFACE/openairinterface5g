##### paths #####
OUTDIR  := .
BINDIR  = $(OUTDIR)/bin
OBJDIR  = $(OUTDIR)/obj
INCDIR  = inc
SRCDIR  = src

##### misc #####
QUIETLY = 1>/dev/null 2>/dev/null

##### sources, objects and libraries #####
BINNAME = liblfds611
AR_BINARY = $(BINDIR)/$(BINNAME).a
$(info $(AR_BINARY))
SO_BINARY = $(BINDIR)/$(BINNAME).so
SRCDIRS = lfds611_abstraction lfds611_freelist lfds611_liblfds lfds611_queue lfds611_ringbuffer lfds611_slist lfds611_stack
# TRD : be aware - in the linux makefile, with the one-pass linking behaviour of the GNU linker, the order
#       of source files matters!  this is because it leads to the ordering of objects in the library and
#       that in turn, since the data structures all use the freelist API and the abstraction API, has to be
#       correct 
# TRD : lfds611_abstraction_cas.c lfds611_abstraction_dcas.c lfds611_abstraction_increment.c are inlined and are compiled by every C file
SOURCES = lfds611_queue_delete.c lfds611_queue_new.c lfds611_queue_query.c lfds611_queue_queue.c \
          lfds611_ringbuffer_delete.c lfds611_ringbuffer_get_and_put.c lfds611_ringbuffer_new.c lfds611_ringbuffer_query.c \
          lfds611_slist_delete.c lfds611_slist_get_and_set.c lfds611_slist_link.c lfds611_slist_new.c \
          lfds611_stack_delete.c lfds611_stack_new.c lfds611_stack_push_pop.c lfds611_stack_query.c \
          lfds611_freelist_delete.c lfds611_freelist_get_and_set.c lfds611_freelist_new.c lfds611_freelist_query.c lfds611_freelist_pop_push.c \
          lfds611_liblfds_abstraction_test_helpers.c lfds611_liblfds_aligned_free.c lfds611_liblfds_aligned_malloc.c \
          lfds611_abstraction_free.c lfds611_abstraction_malloc.c
OBJECTS = $(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SOURCES)))

##### CPU variants #####
UNAME   = $(shell uname -m)
GCCARCH = -march=$(UNAME)

ifeq ($(UNAME),x86_64)
  GCCARCH = -march=core2
endif

ifeq ($(findstring arm,$(UNAME)),arm)
  GCCARCH = -march=armv6k -marm
endif

##### tools #####
MAKE    = make
MFLAGS  = 

DG      = gcc
DGFLAGS = -MM -std=c99 -I"$(SRCDIR)" -I"$(INCDIR)"

CC      = gcc
CBASE   = -Wall -Wno-unknown-pragmas -std=c99 $(GCCARCH) -pthread -c -I"$(SRCDIR)" -I"$(INCDIR)"
CFREL   = -O2 -finline-functions -Wno-strict-aliasing
CFDBG   = -O0 -g

AR      = ar
AFLAGS  = -rcs

LD      = gcc
LFBASE  = -Wall -std=c99 -shared
LFREL   = -O2 -s
LFDBG   = -O0 -g

##### rel/dbg .a/.so variants #####
ifeq ($(findstring so,$(MAKECMDGOALS)),so)
  CBASE := $(CBASE) -fpic
endif

CFLAGS  = $(CBASE) $(CFDBG)
LFLAGS  = $(LFBASE) $(LFDBG)

ifeq ($(findstring rel,$(MAKECMDGOALS)),rel)
  CFLAGS  = $(CBASE) $(CFREL)
  LFLAGS  = $(LFBASE) $(LFREL)
endif

##### search paths #####
vpath %.c $(patsubst %,$(SRCDIR)/%:,$(SRCDIRS))

##### implicit rules #####
$(OBJDIR)/%.o : %.c
	@echo "Compiling $<"
	@$(DG) $(DGFLAGS) $< >$(OBJDIR)/$*.d
	@$(CC) $(CFLAGS) -o $@ $<

##### explicit rules #####
$(AR_BINARY) : $(OBJECTS)
	@echo "Creating $(AR_BINARY)"
	@$(AR) $(AFLAGS) $(AR_BINARY) $(OBJECTS)

$(SO_BINARY) : $(OBJECTS)
	@echo "Creating $(SO_BINARY)"
	$(LD) $(LFLAGS) $(SYSLIBS) $(OBJECTS) -o $(SO_BINARY)

##### phony #####
.PHONY : clean arrel ardbg sorel sodbg

clean : 
	@rm -f $(BINDIR)/$(BINNAME).* $(OBJDIR)/*.o $(OBJDIR)/*.d

arrel : $(AR_BINARY)
ardbg : $(AR_BINARY)

sorel : $(SO_BINARY)
sodbg : $(SO_BINARY)

##### dependencies #####
-include $(DEPENDS)

##### notes #####
# TRD : we use -std=c99 purely to permit C++ style comments

