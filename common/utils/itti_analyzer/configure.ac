AC_PREREQ([2.68])

define([svnversion], esyscmd([sh -c "svnversion ..|tr -d '\n'"]))

AC_DEFINE(SVN_REVISION, "svnversion", [SVN Revision])

AC_INIT([itti_analyzer], [0.2.svnversion], [openair_admin@eurecom.fr])
AC_CANONICAL_BUILD
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([1.11 silent-rules])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([config.h])

AM_MAINTAINER_MODE

AM_SILENT_RULES([yes])

AM_PROG_LIBTOOL

AC_PROG_RANLIB
AC_PROG_CXX

AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([strstr])

AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([libintl.h])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([netinet/in.h])

AC_CHECK_LIB(m, log10)

PKG_CHECK_MODULES(XML2, [libxml-2.0 >= 2.7.8], [HAVE_XML2=true], [AC_MSG_ERROR(lixml-2.0 not installed)])
CFLAGS="$CFLAGS $XML2_CFLAGS"
LIBS="$LIBS $XML2_LIBS"

AM_PATH_GTK_3_0(3.0.0, [], AC_MSG_ERROR(Requirement not met: gtk >= 3.0.0))
CFLAGS="$CFLAGS $GTK_CFLAGS"
LIBS="$LIBS $GTK_LIBS"

dnl Add these flags
CFLAGS="$CFLAGS -Wall"
CFLAGS="$CFLAGS -Wshadow"
CFLAGS="$CFLAGS -Wcast-align"
CFLAGS="$CFLAGS -Wchar-subscripts"
CFLAGS="$CFLAGS -Wmissing-prototypes"
CFLAGS="$CFLAGS -Wmissing-declarations"
CFLAGS="$CFLAGS -Werror=implicit-function-declaration"

AC_SUBST([AM_CFLAGS])
AC_SUBST(ADD_CFLAGS)

dnl *** Autoconf support ***
AC_ARG_ENABLE(autoconf,
  [  --disable-autoconf      disable automatic generation of configure script ],
  enable_autoconf=$enableval, enable_autoconf=yes
)
AC_PATH_PROG(AUTOCONF, autoconf, @echo autoconf not available)
AC_PATH_PROG(AUTOHEADER, autoheader, @echo autoheader not available)
if test -z "$AUTOCONF"; then enable_autoconf=no ; fi
if test -z "$AUTOHEADER"; then enable_autoconf=no ; fi
if test x$enable_autoconf = xyes; then
  CONFIGURE_DEPENDS="configure.in aclocal.m4"
fi
AC_SUBST(CONFIGURE_DEPENDS)

AC_CONFIG_FILES(
    libbuffers/Makefile             \
    libparser/Makefile              \
    libresolver/Makefile            \
    libui/Makefile                  \
    Makefile                        \
)
AC_OUTPUT
