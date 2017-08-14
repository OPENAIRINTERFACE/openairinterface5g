AM_CFLAGS =	\
	@ADD_CFLAGS@	\
	-I$(top_srcdir)/common	\
	-I$(top_srcdir)/libparser	\
	-I$(top_srcdir)/libresolver	\
	-I$(top_srcdir)/libbuffers	\
	-I$(top_srcdir)/libui

SUBDIRS = libparser libresolver libbuffers libui .

itti_analyzer_LDADD = \
	$(top_builddir)/libui/libui.la	\
	$(top_builddir)/libparser/libparser.la	\
	$(top_builddir)/libresolver/libresolver.la	\
	$(top_builddir)/libbuffers/libbuffers.la

bin_PROGRAMS = itti_analyzer
