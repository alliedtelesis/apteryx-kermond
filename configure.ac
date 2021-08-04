AC_INIT([apteryx_kermond], [1.0])
AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability foreign subdir-objects])

AC_PROG_CC_STDC
AM_PROG_AR

LT_INIT([disable-static pic-only])

AC_CONFIG_MACRO_DIRS([m4])

PKG_CHECK_MODULES([GLIB],[glib-2.0])
PKG_CHECK_MODULES([APTERYX],[apteryx])
PKG_CHECK_MODULES([LIBNL],[libnl-3.0])
PKG_CHECK_MODULES([LIBNLROUTE],[libnl-route-3.0])

AC_ARG_ENABLE([tests],
[  --enable-tests          build unit tests (default=yes)],
[case "${enableval}" in
  yes) tests=true ;;
  no)  tests=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-tests]) ;;
esac],[tests=true])
AM_CONDITIONAL([HAVE_TESTS], [test x$tests = xtrue])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
