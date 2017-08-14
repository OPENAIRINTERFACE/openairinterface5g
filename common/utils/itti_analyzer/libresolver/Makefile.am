AM_CFLAGS =	\
	@ADD_CFLAGS@	\
	-I$(top_srcdir)/common	\
	-I$(top_srcdir)/libparser	\
	-I$(top_srcdir)/libui	\
	-I$(top_srcdir)/libbuffers

noinst_LTLIBRARIES = libresolver.la

libresolver_la_LDFLAGS = -all-static
libresolver_la_SOURCES = \
	locate_root.c	locate_root.h	\
	resolvers.c	resolvers.h
