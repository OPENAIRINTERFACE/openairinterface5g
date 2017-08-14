CFLAGS +=  -I$(RLC_DIR)
CFLAGS +=  -I$(RLC_UM_DIR)
CFLAGS +=  -I$(RLC_AM_DIR)
CFLAGS +=  -I$(COMMON_DIR)
CFLAGS +=  -I$(TRACE_DIR)
CFLAGS +=  -I$(LISTS_DIR)
CFLAGS +=  -I$(MEM_DIR)

DEBUG_FLAGS += -O2

OBJ = rlc_um.o rlc_um_segment.o 


############################################################
#
# The following should be identical in all Makefiles
#
#define the target object files

ifdef TARGETDIR
	TARGETOBJ = $(addprefix ${TARGETDIR}/, ${OBJ})
	TARGETLIB = ${TARGETDIR}/Lib.a
endif

include $(TARGETOBJ:.o=.d)

# Look for source files in the parent directory
%.o: ../%.c %.d
	$(CCC) -c ${DEBUG_FLAGS} ${CFLAGS} ${INCLUDE} -o $@ $<

# Rule to remake and keep the .d dependency Makefiles
# Note that the sed command includes adding the TARGETDIR
#  and that ! is used for s delimiter
%.d: ../%.c
	$(SHELL) -ec '$(CCC) -MM $(CFLAGS) $< \
	| sed '\''s!\($(*F)\)\.o[ :]*!$(TARGETDIR)/\1.o $@ : !g'\'' > $@ ;\
	[ -s $@ ] || rm -f $@'
.PRECIOUS: %.d

all: ${TARGETLIB}

${TARGETLIB}: ${TARGETOBJ}
	ar -rv ${TARGETLIB} ${TARGETOBJ}

clean:
	rm -f ${TARGETDIR}/*.o ${TARGETDIR}/*.a

clean_dep:
	rm -f ${TARGETDIR}/*.d
