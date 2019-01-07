##### paths #####
BINDIR := ..\..\bin
OBJDIR := ..\..\obj
SRCDIR := ..\..\src

##### misc #####
QUIETLY := 1>nul 2>nul
NULL    := 
SPACE   := $(NULL) # TRD : necessary trailing space after the close bracket

##### sources, objects and libraries #####
BINNAME := test
BINARY  := $(BINDIR)\$(BINNAME).exe
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
OBJECTS := $(patsubst %.c,$(OBJDIR)/%.obj,$(notdir $(SOURCES)))
RESFILE := $(patsubst %.rc,$(OBJDIR)/%.res,$(notdir $(RCFILE)))
SYSLIBS := kernel32.lib
EXTLIBS := 
USRLIBS := ../../../liblfds700/bin/liblfds700.lib

##### default paths fix up #####
INCLUDE += ;../../../../liblfds700/inc/
LIB     += ;../../../../liblfds700/bin/

##### tools #####
MAKE    := make
MFLAGS  := 

CC      := cl
CFBASE  := /c /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /DUNICODE /D_UNICODE /DUNICODE "/Fd$(BINDIR)\$(BINNAME).pdb" /nologo /W4 /WX
CFREL   := /DNDEBUG /Ox
CFDBG   := /D_DEBUG /Od /Gm /Zi

LD      := link
LFBASE  := /nologo /subsystem:console /nodefaultlib /nxcompat /wx
LFREL   := /incremental:no
LFDBG   := /debug "/pdb:$(BINDIR)\$(BINNAME).pdb"

##### variants #####
CFLAGS  := $(CFBASE) $(CFDBG) /MTd
LFLAGS  := $(LFBASE) $(LFDBG)
CLIB    := libcmtd.lib

ifeq ($(MAKECMDGOALS),librel)
  CFLAGS := $(CFBASE) $(CFREL) /MT
  LFLAGS := $(LFBASE) $(LFREL)
  CLIB   := libcmt.lib
endif

ifeq ($(MAKECMDGOALS),libdbg)
  CFLAGS := $(CFBASE) $(CFDBG) /MTd
  LFLAGS := $(LFBASE) $(LFDBG)
  CLIB   := libcmtd.lib
endif

ifeq ($(MAKECMDGOALS),dllrel)
  CFLAGS := $(CFBASE) $(CFREL) /MD
  LFLAGS := $(LFBASE) $(LFREL)
  CLIB   := msvcrt.lib
endif

ifeq ($(MAKECMDGOALS),dlldbg)
  CFLAGS := $(CFBASE) $(CFDBG) /MDd
  LFLAGS := $(LFBASE) $(LFDBG)
  CLIB   := msvcrtd.lib
endif

##### search paths #####
vpath %.c $(patsubst %,$(SRCDIR)/%;,$(SRCDIRS))

##### implicit rules #####
$(OBJDIR)/%.obj : %.c
	$(CC) $(CFLAGS) "/Fo$@" $<

##### explicit rules #####
$(BINARY) : $(OBJECTS) $(USRLIBS)
	$(LD) $(LFLAGS) $(CLIB) $(SYSLIBS) $(EXTLIBS) $(USRLIBS) $(OBJECTS) /out:$(BINARY)

##### phony #####
.PHONY : clean librel libdbg dllrel dlldbg

clean : 
	@erase /Q $(OBJDIR)\*.obj $(OBJDIR)\*.res $(BINDIR)\$(BINNAME).* $(QUIETLY)

dlldbg : $(BINARY)
dllrel : $(BINARY)

libdbg : $(BINARY)
librel : $(BINARY)

##### notes #####
# TRD : we fix up the default paths because cl and link require an extra argument per additional path, which is ugly as hell

