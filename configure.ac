dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.71])
AC_INIT([alsa-midi-latency-test],[0.0.6])
AC_CONFIG_SRCDIR([src/alsa-midi-latency-test.c])
AC_PREFIX_DEFAULT(/usr)
AM_INIT_AUTOMAKE([foreign subdir-objects])

AM_MAINTAINER_MODE([enable])

dnl Checks for programs.

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MKDIR_P
PKG_PROG_PKG_CONFIG

dnl Checks for header files.
AC_CHECK_HEADERS([alsa/asoundlib.h], [], [AC_MSG_ERROR([alsa-lib not found])])

AC_CONFIG_HEADERS(include/aconfig.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_CHECK_HEADERS_ONCE([sys/time.h])

dnl Checks for library functions.
AC_CHECK_FUNCS([clock_gettime], [CLOCK_LIB=], [AC_CHECK_LIB([rt],
               [clock_gettime], [CLOCK_LIB=-lrt],
               [AC_MSG_ERROR([Couldn't find clock_gettime])])])
AC_SUBST([CLOCK_LIB])

dnl Enable largefile support
AC_SYS_LARGEFILE

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT
