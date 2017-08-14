AM_CFLAGS =	\
	@ADD_CFLAGS@	\
	-I$(top_srcdir)/common	\
	-I$(top_srcdir)/libbuffers	\
	-I$(top_srcdir)/libui

noinst_LTLIBRARIES = libparser.la

libparser_la_LDFLAGS = -all-static
libparser_la_SOURCES = \
	xml_parse.c	\
	array_type.c array_type.h	\
	enum_type.c enum_type.h	\
	enum_value_type.c enum_value_type.h	\
	file_type.c file_type.h	\
	field_type.c field_type.h	\
	fundamental_type.c fundamental_type.h	\
	pointer_type.h	pointer_type.c	\
	reference_type.c	reference_type.h	\
	struct_type.c struct_type.h	\
	typedef_type.c	typedef_type.h	\
	types.h	types.c	\
	union_type.c	union_type.h
