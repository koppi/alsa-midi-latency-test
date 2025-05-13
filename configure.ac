dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.72])
AC_INIT([alsa-midi-latency-test],[0.0.6])
AC_CONFIG_SRCDIR([src/alsa-midi-latency-test.c])
AC_PREFIX_DEFAULT(/usr)
AM_INIT_AUTOMAKE([subdir-objects])

AM_MAINTAINER_MODE([enable])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl Checks for programs.

AC_PROG_CC
dnl AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_LN_S
AC_PROG_SED
AC_DISABLE_STATIC
LT_INIT
PKG_PROG_PKG_CONFIG

dnl try to gues cross-compiler if not set
if test "x$target" != "x$host" -a -z "`echo $CC | grep -e '-gcc'`";
then
  AC_MSG_CHECKING(for cross-compiler)

  which ${program_prefix}gcc >/dev/null 2>&1 && CC=${program_prefix}gcc
  which ${target_cpu}-${target_os}-gcc >/dev/null 2>&1 \
  && CC=${target_cpu}-${target-os}-gcc
  which ${target_cpu}-${target_vendor}-${target_os}-gcc >/dev/null 2>&1 \
  && CC=${target_cpu}-${target_vendor}-${target_os}-gcc

  AC_MSG_RESULT($CC)
fi

dnl Checks for header files.
AC_CHECK_HEADERS([alsa/rawmidi.h], [have_rawmidi="yes"], [have_rawmidi="no"],
  [#include <alsa/asoundlib.h>])
AC_CHECK_HEADERS([alsa/seq.h], [have_seq="yes"], [have_seq="no"],
  [#include <alsa/asoundlib.h>])

AM_CONDITIONAL(HAVE_RAWMIDI, test "$have_rawmidi" = "yes")
AM_CONDITIONAL(HAVE_SEQ, test "$have_seq" = "yes")



test "x$prefix" = xNONE && prefix=$ac_default_prefix

eval dir="$datadir"
dnl Check for librt
LIBRT=""
AC_MSG_CHECKING(for librt)
AC_ARG_WITH(librt,
  AS_HELP_STRING([--with-librt], [Use librt for monotonic clock (default = yes)]),
  [ have_librt="$withval" ], [ have_librt="yes" ])
if test "$have_librt" = "yes"; then
  AC_CHECK_LIB([rt], [clock_gettime], [HAVE_LIBRT="yes"])
  if test "$HAVE_LIBRT" = "yes" ; then
    LIBRT="-lrt"
    AC_DEFINE([HAVE_LIBRT], 1, [Have librt])
    AC_DEFINE([HAVE_CLOCK_GETTIME], 1, [Have clock gettime])
  fi
else
  AC_MSG_RESULT(no)
fi

AC_CHECK_FUNCS([clock_gettime], [CLOCK_LIB=], [AC_CHECK_LIB([rt],
               [clock_gettime], [CLOCK_LIB=-lrt],
               [AC_MSG_ERROR([Couldn't find clock_gettime])])])
AC_SUBST([CLOCK_LIB])

case "$dir" in
/*) ;;
*) dir="$prefix/share"
esac

AC_CONFIG_HEADERS(include/aconfig.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_CHECK_HEADERS_ONCE([sys/time.h])

dnl Checks for library functions.

dnl Enable largefile support
AC_SYS_LARGEFILE

AC_CONFIG_FILES([Makefile m4/Makefile src/Makefile include/Makefile])
AC_OUTPUT
