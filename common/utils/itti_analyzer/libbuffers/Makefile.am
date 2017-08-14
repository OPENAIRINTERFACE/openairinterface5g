AM_CFLAGS =	\
	@ADD_CFLAGS@	\
	-I$(top_srcdir)/common	\
	-I$(top_srcdir)/libparser	\
	-I$(top_srcdir)/libui

noinst_LTLIBRARIES = libbuffers.la

libbuffers_la_LDFLAGS = -all-static
libbuffers_la_SOURCES = \
	buffers.c	buffers.h	\
	file.c	file.h	\
	socket.c	socket.h
