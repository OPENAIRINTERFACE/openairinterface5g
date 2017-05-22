##### paths #####
BINDIR := ..\..\bin
INCDIR := ..\..\inc
OBJDIR := ..\..\obj
SRCDIR := ..\..\src

##### misc #####
QUIETLY := 1>nul 2>nul
NULL := 
SPACE := $(NULL) # TRD : with a trailing space

##### sources, objects and libraries #####
BINNAME := liblfds700
LIB_BINARY := $(BINDIR)\$(BINNAME).lib
DLL_BINARY := $(BINDIR)\$(BINNAME).dll
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
OBJECTS := $(patsubst %.c,$(OBJDIR)/%.obj,$(notdir $(SOURCES)))
SYSLIBS := kernel32.lib

##### default paths fix up #####
INCDIRS := $(patsubst %,%;,$(INCDIR))
INCLUDE += $(subst $(SPACE),,$(INCDIRS))

##### tools #####
MAKE    := make
MFLAGS  := 

CC      := cl
CBASE   := /c "-I$(SRCDIR)" "/Fd$(BINDIR)\$(BINNAME).pdb" /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /DUNICODE /D_UNICODE /DUNICODE /nologo /W4 /wd 4068 /WX
CFREL   := /DNDEBUG /Ox
CFDBG   := /D_DEBUG /Gm /Od /Zi

AR      := lib
AFLAGS  := /nologo /subsystem:console /verbose /wx

LD      := link
LFBASE  := /def:$(BINNAME).def /dll /nodefaultlib /nologo /nxcompat /subsystem:console /wx
LFREL   := /incremental:no
LFDBG   := /debug "/pdb:$(BINDIR)\$(BINNAME).pdb"

##### variants #####
CFLAGS  := $(CBASE) $(CFDBG) /MTd
ASFLAGS := $(ASBASE) $(ASDBG)
LFLAGS  := $(LFBASE) $(LFDBG)
CLIB    := libcmtd.lib

ifeq ($(MAKECMDGOALS),librel)
  CFLAGS  := $(CBASE) $(CFREL) /MT
  ASFLAGS := $(ASBASE) $(ASREL)
  LFLAGS  := $(LFBASE) $(LFREL)
  CLIB    := libcmt.lib
endif

ifeq ($(MAKECMDGOALS),libdbg)
  CFLAGS  := $(CBASE) $(CFDBG) /MTd
  ASFLAGS := $(ASBASE) $(ASDBG)
  LFLAGS  := $(LFBASE) $(LFDBG)
  CLIB    := libcmtd.lib
endif

ifeq ($(MAKECMDGOALS),dllrel)
  CFLAGS  := $(CBASE) $(CFREL) /MD
  ASFLAGS := $(ASBASE) $(ASREL)
  LFLAGS  := $(LFBASE) $(LFREL)
  CLIB    := msvcrt.lib
endif

ifeq ($(MAKECMDGOALS),dlldbg)
  CFLAGS  := $(CBASE) $(CFDBG) /MDd
  ASFLAGS := $(ASBASE) $(ASDBG)
  LFLAGS  := $(LFBASE) $(LFDBG)
  CLIB    := msvcrtd.lib
endif

##### search paths #####
vpath %.c $(patsubst %,$(SRCDIR)/%;,$(SRCDIRS))

##### implicit rules #####
$(OBJDIR)/%.obj : %.c
	$(CC) $(CFLAGS) "/Fo$@" $<

##### explicit rules #####
$(LIB_BINARY) : $(OBJECTS)
	$(AR) $(AFLAGS) $(OBJECTS) /out:$(LIB_BINARY)

$(DLL_BINARY) : $(OBJECTS)
	$(LD) $(LFLAGS) $(CLIB) $(SYSLIBS) $(OBJECTS) /out:$(DLL_BINARY)

##### phony #####
.PHONY : clean librel libdbg dllrel dlldbg

clean : 
	@erase /Q $(BINDIR)\$(BINNAME).* $(OBJDIR)\*.obj $(QUIETLY)

dlldbg : $(DLL_BINARY)
dllrel : $(DLL_BINARY)

libdbg : $(LIB_BINARY)
librel : $(LIB_BINARY)

##### notes #####
# /wd 4068 : turn off "unknown pragma" warning


