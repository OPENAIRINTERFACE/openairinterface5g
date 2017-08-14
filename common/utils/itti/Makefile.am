AM_CFLAGS = @ADD_CFLAGS@ 	\
	-I$(top_srcdir)/UTILS   \
	-I$(top_srcdir)/COMMON	\
	-I$(OPENAIR2_DIR)/UTIL/LFDS/liblfds6.1.1/liblfds611/inc	\
	-DENABLE_EVENT_FD

noinst_LTLIBRARIES = libitti.la

BUILT_SOURCES = $(abs_top_builddir)/INTERTASK_INTERFACE/UTIL/LFDS/liblfds6.1.1/liblfds611/bin/liblfds611.a

$(abs_top_builddir)/INTERTASK_INTERFACE/UTIL/LFDS/liblfds6.1.1/liblfds611/bin/liblfds611.a:
	@if [ ! -d $(abs_top_builddir)/INTERTASK_INTERFACE/UTIL/LFDS/liblfds6.1.1/liblfds611/bin ]; then mkdir -p $(abs_top_builddir)/INTERTASK_INTERFACE/UTIL/LFDS/liblfds6.1.1/liblfds611/bin; fi;
	@if [ ! -d $(abs_top_builddir)/INTERTASK_INTERFACE/UTIL/LFDS/liblfds6.1.1/liblfds611/obj ]; then mkdir -p $(abs_top_builddir)/INTERTASK_INTERFACE/UTIL/LFDS/liblfds6.1.1/liblfds611/obj; fi;
	@$(MAKE) -C $(OPENAIR2_DIR)/UTIL/LFDS/liblfds6.1.1/liblfds611/ -f makefile.linux OUTDIR=$(abs_top_builddir)/INTERTASK_INTERFACE/UTIL/LFDS/liblfds6.1.1/liblfds611

libitti_la_LDFLAGS = -all-static
libitti_la_SOURCES =							\
	gtpv1_u_messages_def.h	gtpv1_u_messages_types.h		\
	nas_messages_def.h	nas_messages_types.h			\
	timer_messages_def.h	timer_messages_types.h			\
	s11_messages_def.h	s11_messages_types.h			\
	s1ap_messages_def.h	s1ap_messages_types.h			\
	s6a_messages_def.h	s6a_messages_types.h			\
	sgw_lite_def.h		sgw_lite_messages_types.h		\
	sctp_messages_def.h	sctp_messages_types.h			\
	udp_message_def.h	udp_messages_types.h			\
	intertask_interface.c 	intertask_interface.h			\
	intertask_interface_dump.c intertask_interface_dump.h		\
	assertions.h							\
	backtrace.c backtrace.h						\
	signals.c signals.h						\
	timer.c	timer.h
