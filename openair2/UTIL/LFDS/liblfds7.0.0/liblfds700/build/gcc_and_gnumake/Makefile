##### paths #####
BINDIR    := ../../bin
INCDIR    := ../../inc
OBJDIR    := ../../obj
SRCDIR    := ../../src

##### misc #####
QUIETLY        := 1>/dev/null 2>/dev/null
VERSION_NUMBER := 1
MINOR_NUMBER   := 0
RELEASE_NUMBER := 0

##### sources, objects and libraries #####
BINNAME    := liblfds700
ARFILENAME := $(BINNAME).a
ARPATHNAME := $(BINDIR)/$(ARFILENAME)
SOBASENAME := $(BINNAME).so
SONAME     := $(SOBASENAME).$(VERSION_NUMBER)
SOFILENAME := $(SONAME).$(MINOR_NUMBER).$(RELEASE_NUMBER)
SOPATHNAME := $(BINDIR)/$(SOFILENAME)
INCNAME    := $(INCDIR)/$(BINNAME).h
SRCDIRS := lfds700_btree_addonly_unbalanced lfds700_freelist lfds700_hash_addonly lfds700_list_addonly_ordered_singlylinked lfds700_list_addonly_singlylinked_unordered lfds700_misc lfds700_queue lfds700_queue_bounded_singleconsumer_singleproducer lfds700_ringbuffer lfds700_stack
SOURCES := lfds700_hash_addonly_cleanup.c lfds700_hash_addonly_get.c lfds700_hash_addonly_init.c lfds700_hash_addonly_insert.c lfds700_hash_addonly_iterate.c lfds700_hash_addonly_query.c \
           lfds700_list_addonly_ordered_singlylinked_cleanup.c lfds700_list_addonly_ordered_singlylinked_get.c lfds700_list_addonly_ordered_singlylinked_init.c lfds700_list_addonly_ordered_singlylinked_insert.c lfds700_list_addonly_ordered_singlylinked_query.c \
           lfds700_list_addonly_singlylinked_unordered_cleanup.c lfds700_list_addonly_singlylinked_unordered_get.c lfds700_list_addonly_singlylinked_unordered_init.c lfds700_list_addonly_singlylinked_unordered_insert.c lfds700_list_addonly_singlylinked_unordered_query.c \
           lfds700_btree_addonly_unbalanced_cleanup.c lfds700_btree_addonly_unbalanced_get.c lfds700_btree_addonly_unbalanced_init.c lfds700_btree_addonly_unbalanced_insert.c lfds700_btree_addonly_unbalanced_query.c \
           lfds700_freelist_cleanup.c lfds700_freelist_init.c lfds700_freelist_pop.c lfds700_freelist_push.c lfds700_freelist_query.c \
           lfds700_misc_cleanup.c lfds700_misc_globals.c lfds700_misc_init.c lfds700_misc_prng.c lfds700_misc_query.c \
           lfds700_queue_cleanup.c lfds700_queue_dequeue.c lfds700_queue_enqueue.c lfds700_queue_init.c lfds700_queue_query.c \
           lfds700_queue_bounded_singleconsumer_singleproducer_cleanup.c lfds700_queue_bounded_singleconsumer_singleproducer_dequeue.c lfds700_queue_bounded_singleconsumer_singleproducer_enqueue.c lfds700_queue_bounded_singleconsumer_singleproducer_init.c lfds700_queue_bounded_singleconsumer_singleproducer_query.c \
           lfds700_ringbuffer_cleanup.c lfds700_ringbuffer_init.c lfds700_ringbuffer_query.c lfds700_ringbuffer_read.c lfds700_ringbuffer_write.c \
           lfds700_stack_cleanup.c lfds700_stack_init.c lfds700_stack_pop.c lfds700_stack_push.c lfds700_stack_query.c
OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SOURCES)))
SYSLIBS := 

##### default paths fix up #####
CPATH   := $(subst : ,:,$(SRCDIR):$(INCDIR))

##### tools #####
MAKE    := make
MFLAGS  := 

DG      := gcc
DGFLAGS := -MM -std=gnu89

CC      := gcc
CFBASE  := -c -fno-strict-aliasing -std=gnu89 -Wall -Werror -Wno-unknown-pragmas -Wno-unused-but-set-variable -Wno-unused-variable
CFCOV   := -O0 -ggdb -DCOVERAGE -fprofile-arcs -ftest-coverage
CFDBG   := -O0 -ggdb -D_DEBUG
CFPROF  := -O0 -ggdb -DPROF     -pg
CFREL   := -O2       -DNDEBUG   -finline-functions
CFTSAN  := -O0 -ggdb -DTSAN     -fsanitize=thread -fPIC
CFBARE  := -ffreestanding -nodefaultlibs -nostdinc -nostdlib

AR      := ar
AFLAGS  := -rcs

LD      := gcc
LFBASE  := -pthread -shared -std=gnu89 -Wl,-soname,$(SONAME) -o $(SOPATHNAME) -Wall -Werror
LFCOV   := -O0 -fprofile-arcs -ftest-coverage
LFDBG   := -O0 -ggdb
LFPROF  := -O0 -pg
LFREL   := -O2 -s -finline-functions
LFTSAN  := -O0 -fsanitize=thread -fPIC
LFBARE  := -ffreestanding -nodefaultlibs -nostdinc -nostdlib

##### CPU variants #####
GCCARCH := native
CFBASE  += -march=$(GCCARCH)

##### build variants #####
ifeq ($(findstring so,$(MAKECMDGOALS)),so)
  CFBASE += -fPIC
endif

CFLAGS += $(CFBASE)
LFLAGS += $(LFBASE)

ifeq ($(MAKECMDGOALS),)
  CFLAGS += $(CFDBG)
  LFLAGS += $(LFDBG)
endif

ifeq ($(findstring cov,$(MAKECMDGOALS)),cov)
  CFLAGS  += $(CFCOV)
  LFLAGS  += $(LFCOV)
  SYSLIBS += -lgcov
endif

ifeq ($(findstring dbg,$(MAKECMDGOALS)),dbg)
  CFLAGS += $(CFDBG)
  LFLAGS += $(LFDBG)
endif

ifeq ($(findstring prof,$(MAKECMDGOALS)),prof)
  CFLAGS += $(CFPROF)
  LFLAGS += $(LFPROF)
endif

ifeq ($(findstring rel,$(MAKECMDGOALS)),rel)
  CFLAGS += $(CFREL)
  LFLAGS += $(LFREL)
endif

ifeq ($(findstring tsan,$(MAKECMDGOALS)),tsan)
  CFLAGS += $(CFTSAN)
  LFLAGS += $(LFTSAN)
endif

ifeq ($(findstring b_,$(MAKECMDGOALS)),b_)
  CFLAGS += $(CFBARE)
  LFLAGS += $(CFBARE)
endif

##### search paths #####
vpath %.c $(patsubst %,$(SRCDIR)/%:,$(SRCDIRS))

##### implicit rules #####
$(OBJDIR)/%.o : %.c
	$(DG) $(DGFLAGS) $< >$(OBJDIR)/$*.d
	$(CC) $(CFLAGS) -o $@ $<

##### explicit rules #####
$(ARPATHNAME) : $(OBJECTS)
	$(AR) $(AFLAGS) $(ARPATHNAME) $(OBJECTS)

$(SOPATHNAME) : $(OBJECTS)
	$(LD) $(LFLAGS) $(OBJECTS) -o $(SOPATHNAME)
	@ln -fs $(SOFILENAME) $(BINDIR)/$(SONAME)
	@ln -fs $(SOFILENAME) $(BINDIR)/$(SOBASENAME)

##### phony #####
.PHONY : clean bare_ar_cov bare_ar_dbg bare_ar_prof bare_ar_rel bare_ar_tsan bare_so_cov bare_so_dbg bare_so_prof bare_so_rel bare_so_tsan hosted_ar_cov hosted_ar_dbg hosted_ar_prof hosted_ar_rel hosted_ar_tsan hosted_so_cov hosted_so_dbg hosted_so_prof hosted_so_rel hosted_so_tsan

clean : 
	@rm -f $(BINDIR)/* $(OBJDIR)/*

bare_ar_cov   : $(ARPATHNAME) # bare, archive (.a), coverage
bare_ar_dbg   : $(ARPATHNAME) # bare, archive (.a), debug
bare_ar_prof  : $(ARPATHNAME) # bare, archive (.a), profiling
bare_ar_rel   : $(ARPATHNAME) # bare, archive (.a), release
bare_ar_tsan  : $(ARPATHNAME) # bare, archive (.a), thread sanitizer

bare_so_cov   : $(SOPATHNAME) # bare, shared (.so), coverage
bare_so_dbg   : $(SOPATHNAME) # bare, shared (.so), debug
bare_so_prof  : $(SOPATHNAME) # bare, shared (.so), profiling
bare_so_rel   : $(SOPATHNAME) # bare, shared (.so), release
bare_so_tsan  : $(SOPATHNAME) # bare, shared (.so), thread sanitizer

hosted_ar_cov  : $(ARPATHNAME) # hosted implementation, archive (.a), coverage
hosted_ar_dbg  : $(ARPATHNAME) # hosted implementation, archive (.a), debug
hosted_ar_prof : $(ARPATHNAME) # hosted implementation, archive (.a), profiling
hosted_ar_rel  : $(ARPATHNAME) # hosted implementation, archive (.a), release
hosted_ar_tsan : $(ARPATHNAME) # hosted implementation, archive (.a), thread sanitizer

hosted_so_cov  : $(SOPATHNAME) # hosted implementation, shared (.so), coverage
hosted_so_dbg  : $(SOPATHNAME) # hosted implementation, shared (.so), debug
hosted_so_prof : $(SOPATHNAME) # hosted implementation, shared (.so), profiling
hosted_so_rel  : $(SOPATHNAME) # hosted implementation, shared (.so), release
hosted_so_tsan : $(SOPATHNAME) # hosted implementation, shared (.so), thread sanitizer

##### dependencies #####
-include $(DEPENDS)

##### notes #####
# TRD : we use -std=gnu89 for C++ style comments
#       hosted implementation differs from bare simply in that <assert.h> ends up being included

