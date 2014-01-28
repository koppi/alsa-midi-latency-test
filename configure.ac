dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT([alsa-midi-latency-test], [0.0.4])
AC_CONFIG_SRCDIR([src/alsa-midi-latency-test.c])
AM_INIT_AUTOMAKE

AC_PREFIX_DEFAULT(/usr)

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.15])

dnl Checks for programs.

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

AC_PROG_CC
dnl AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PATH_ALSA(1.0.13)

dnl Checks for header files.
AC_HEADER_STDC

test "x$prefix" = xNONE && prefix=$ac_default_prefix

eval dir="$datadir"
case "$dir" in
/*) ;;
*) dir="$prefix/share"
esac

AM_CONFIG_HEADER(include/aconfig.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME

AC_CHECK_FUNCS([clock_gettime], [CLOCK_LIB=], [AC_CHECK_LIB([rt],
               [clock_gettime], [CLOCK_LIB=-lrt],
               [AC_MSG_ERROR([Couldn't find clock_gettime])])])
AC_SUBST([CLOCK_LIB])

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL

dnl Enable largefile support
AC_SYS_LARGEFILE

AC_OUTPUT(Makefile m4/Makefile src/Makefile include/Makefile)
