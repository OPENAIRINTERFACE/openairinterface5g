AC_INIT([open-nFAPI], [1.0])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign serial-tests])
AM_PROG_AR

LT_INIT([shared static])

# Dependencies
AC_PROG_CC
AC_PROG_CXX
AC_PROG_AWK
AC_CONFIG_HEADERS([config.h])

AC_PROG_LIBTOOL

AC_CHECK_FILE([/usr/include/CUnit/CUnit.h],
	[CFLAGS_CUNIT=-I/usr/include/CUnit AC_SUBST(CFLAGS_CUNIT)],
	[AC_MSG_NOTICE([Have cunit *************])])

# Need doxygen
AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN";
	then AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
fi
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([docs/Doxyfile])])

AC_REQUIRE_AUX_FILE([tap-driver.sh])

AC_CONFIG_FILES([Makefile
                 sim_common/Makefile
                 common/Makefile
                 nfapi/Makefile
                 pnf/Makefile
                 vnf/Makefile
                 nfapi/tests/Makefile
                 pnf/tests/Makefile
                 vnf/tests/Makefile
                 pnf_sim/Makefile
                 vnf_sim/Makefile
                 integration_tests/Makefile
                 docs/Makefile
])
AC_OUTPUT
