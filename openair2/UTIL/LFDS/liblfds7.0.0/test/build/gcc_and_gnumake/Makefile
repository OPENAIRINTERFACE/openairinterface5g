##### paths #####
BINDIR     := ../../bin
OBJDIR     := ../../obj
SRCDIR     := ../../src
LIBINCDIRS := ../../../liblfds700/inc/
LIBBINDIRS := ../../../liblfds700/bin/

##### misc #####
QUIETLY := 1>nul 2>nul

##### sources, objects and libraries #####
BINNAME := test
BINARY  := $(BINDIR)/$(BINNAME)
SRCDIRS := .
SOURCES := main.c misc.c \
           test_lfds700_btree_addonly_unbalanced.c test_lfds700_btree_addonly_unbalanced_alignment.c test_lfds700_btree_addonly_unbalanced_random_adds_fail.c test_lfds700_btree_addonly_unbalanced_random_adds_fail_and_overwrite.c test_lfds700_btree_addonly_unbalanced_random_adds_overwrite.c \
           test_lfds700_freelist.c test_lfds700_freelist_alignment.c test_lfds700_freelist_popping.c test_lfds700_freelist_popping_and_pushing.c test_lfds700_freelist_pushing.c test_lfds700_freelist_rapid_popping_and_pushing.c \
           test_lfds700_hash_addonly.c test_lfds700_hash_addonly_alignment.c test_lfds700_hash_addonly_iterate.c test_lfds700_hash_addonly_random_adds_fail.c test_lfds700_hash_addonly_random_adds_fail_and_overwrite.c test_lfds700_hash_addonly_random_adds_overwrite.c \
           test_lfds700_list_addonly_ordered_singlylinked.c test_lfds700_list_addonly_ordered_singlylinked_alignment.c test_lfds700_list_addonly_ordered_singlylinked_new_ordered.c test_lfds700_list_addonly_ordered_singlylinked_new_ordered_with_cursor.c \
           test_lfds700_list_addonly_singlylinked_unordered.c test_lfds700_list_addonly_singlylinked_unordered_alignment.c test_lfds700_list_addonly_singlylinked_unordered_new_after.c test_lfds700_list_addonly_singlylinked_unordered_new_end.c test_lfds700_list_addonly_singlylinked_unordered_new_start.c \
           test_lfds700_porting_abstraction_layer_atomic.c test_lfds700_porting_abstraction_layer_atomic_cas.c test_lfds700_porting_abstraction_layer_atomic_dcas.c test_lfds700_porting_abstraction_layer_atomic_exchange.c \
           test_lfds700_queue.c test_lfds700_queue_alignment.c test_lfds700_queue_dequeuing.c test_lfds700_queue_enqueuing.c test_lfds700_queue_enqueuing_and_dequeuing.c test_lfds700_queue_enqueuing_and_dequeuing_with_free.c test_lfds700_queue_enqueuing_with_malloc_and_dequeuing_with_free.c test_lfds700_queue_rapid_enqueuing_and_dequeuing.c \
           test_lfds700_queue_bounded_singleconsumer_singleproducer.c test_lfds700_queue_bounded_singleconsumer_singleproducer_dequeuing.c test_lfds700_queue_bounded_singleconsumer_singleproducer_enqueuing.c test_lfds700_queue_bounded_singleconsumer_singleproducer_enqueuing_and_dequeuing.c \
           test_lfds700_ringbuffer.c test_lfds700_ringbuffer_reading.c test_lfds700_ringbuffer_reading_and_writing.c test_lfds700_ringbuffer_writing.c \
           test_lfds700_stack.c test_lfds700_stack_alignment.c test_lfds700_stack_popping.c test_lfds700_stack_popping_and_pushing.c test_lfds700_stack_pushing.c test_lfds700_stack_rapid_popping_and_pushing.c \
           test_porting_abstraction_layer_get_logical_core_ids.c test_porting_abstraction_layer_thread_start.c test_porting_abstraction_layer_thread_wait.c \
           util_cmdline.c util_memory_helpers.c util_thread_starter.c
OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SOURCES)))
SYSLIBS := -lm -lpthread -lrt
USRLIBS := -llfds700

##### default paths #####
CPATH        += $(LIBINCDIRS)
LIBRARY_PATH += $(LIBBINDIRS)
export CPATH
export LIBRARY_PATH

##### tools #####
MAKE    := make
MFLAGS  := 

DG      := gcc
DGFLAGS := -MM -std=gnu89

CC      := gcc
CFBASE  := -c -pthread -std=gnu89 -Wall -Wno-unknown-pragmas
CFCOV   := -O0 -ggdb -DCOVERAGE -fprofile-arcs -ftest-coverage
CFDBG   := -O0 -ggdb -D_DEBUG
CFPROF  := -O0 -ggdb -DPROF     -pg
CFREL   := -O2       -DNDEBUG   -finline-functions -Wno-strict-aliasing
CFTSAN  := -O0 -ggdb -DTSAN     -fsanitize=thread -fPIE

LD      := gcc
LFBASE  := -pthread -std=gnu89 -Wall -Werror
LFCOV   := -O0 -fprofile-arcs -ftest-coverage
LFDBG   := -O0 -ggdb
LFPROF  := -O0 -pg
LFREL   := -O2 -s -finline-functions
LFTSAN  := -O0 -fsanitize=thread -pie

PROF    := gprof
PFBASE  := -b -p -Q $(BINARY) gmon.out

##### variants and libnuma check #####
CFLAGS += $(CFBASE)
LFLAGS += $(LFBASE)

ifeq ($(MAKECMDGOALS),)
  CFLAGS += $(CFDBG)
  LFLAGS += $(LFDBG)
endif

ifeq ($(MAKECMDGOALS),cov)
  CFLAGS  += $(CFCOV)
  LFLAGS  += $(LFCOV)
  SYSLIBS += -lgcov
endif

ifeq ($(MAKECMDGOALS),dbg)
  CFLAGS += $(CFDBG)
  LFLAGS += $(LFDBG)
endif

ifeq ($(MAKECMDGOALS),prof)
  CFLAGS += $(CFPROF)
  LFLAGS += $(LFPROF)
endif

ifeq ($(MAKECMDGOALS),rel)
  CFLAGS += $(CFREL)
  LFLAGS += $(LFREL)
endif

ifeq ($(MAKECMDGOALS),tsan)
  CFLAGS += $(CFTSAN)
  LFLAGS += $(LFTSAN)
endif

##### search paths #####
vpath %.c $(patsubst %,$(SRCDIR)/%:,$(SRCDIRS))

##### implicit rules #####
$(OBJDIR)/%.o : %.c
	$(DG) $(DGFLAGS) $< >$(OBJDIR)/$*.d
	$(CC) $(CFLAGS) -o $@ $<

##### explicit rules #####
$(BINARY) : $(OBJECTS)
	$(LD) -o $(BINARY) $(LFLAGS) $(OBJECTS) $(USRLIBS) $(SYSLIBS)
	chmod +x $(BINARY)

##### phony #####
.PHONY : clean cov dbg prof rel tsan

clean : 
	@rm -f $(BINDIR)/$(BINNAME) $(OBJDIR)/*.o $(OBJDIR)/*.d $(OBJDIR)/*.gcno

cov  : $(BINARY)
dbg  : $(BINARY)
prof : $(BINARY)
rel  : $(BINARY)
tsan : $(BINARY)

genprof :
	@$(PROF) $(PFBASE)

##### dependencies #####
-include $(DEPENDS)

