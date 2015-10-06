
SUBDIRS = . HASHTABLE

AM_CFLAGS = @ADD_CFLAGS@	\
	-I$(top_srcdir)/COMMON	\
	-I$(top_srcdir)/NAS/EURECOM-NAS/src/api/network	\
	-I$(top_srcdir)/NAS/EURECOM-NAS/src/include \
	-I$(top_srcdir)/SGW-LITE \
	-I$(top_srcdir)/INTERTASK_INTERFACE

AM_YFLAGS = -d

noinst_LTLIBRARIES = libutils.la
libutils_la_LDFLAGS = -all-static
libutils_la_SOURCES = \
	conversions.h	conversions.c	\
	enum_string.h	enum_string.c	\
	log.c log.h	\
	mme_config.c mme_config.h	\
    mcc_mnc_itu.c mcc_mnc_itu.h   \
	mme_default_values.h	\
	queue.h	tree.h
