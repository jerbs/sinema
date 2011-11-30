dnl ======================
dnl =    configure.in    =
dnl ======================
dnl
dnl Use autoscan to detect missing checks.


dnl
dnl 1. Boilerplate
dnl --------------
dnl
dnl AC_INIT must be the first.

AC_INIT([sinema], [0.1])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR([gui/main.cpp])
dnl AC_CONFIG_HEADER
dnl AC_REVISION
AC_PATH_X
AC_PREREQ

dnl 1.1 Libtool
dnl

dnl Build executables that can be run from build tree without relinking:
AC_DISABLE_FAST_INSTALL

dnl Don't build static and shared libraries to speed up development builds:
AC_DISABLE_SHARED
AC_DISABLE_STATIC

AC_PROG_LIBTOOL

dnl
dnl 2. Options
dnl ----------
dnl
dnl Add command line options to configure.


dnl
dnl 3. Programs
dnl -----------
dnl
dnl Check for programs.

AC_PROG_CXX
AC_PROG_CC
dnl AC_CHECK_PROG
dnl AC_PATH_TOOL


dnl
dnl 4. Libraries
dnl ------------
dnl
dnl Check for libraries. Libraries may be needed for further checks.

PKG_CHECK_MODULES([PLAYER], [libavformat libavcodec libavutil libswscale])
AC_SUBST(PLAYER_CFLAGS)
AC_SUBST(PLAYER_LIBS)

PKG_CHECK_MODULES([GUI], [gtkmm-2.4 >= 2.8.0])
AC_SUBST(GUI_CFLAGS)
AC_SUBST(GUI_LIBS)

AX_BOOST_BASE([1.42.0])
AX_BOOST_FILESYSTEM
AX_BOOST_SERIALIZATION
AX_BOOST_SYSTEM
AX_BOOST_THREAD

dnl
dnl 5. Headers
dnl ----------
dnl
dnl Check for existence of headers.

dnl AC_HEADER_STDC
dnl AC_CHECK_HEADERS(strings.h unistd.h)
AC_HEADER_STDBOOL
AC_C_INLINE
AC_CHECK_HEADERS([stdint.h])
dnl AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
dnl AC_CHECK_HEADERS([strings.h]

dnl
dnl 6. Typedefs and Structures
dnl --------------------------
dnl
dnl Headers to exist. Check if they have the needed types.

AC_TYPE_INT16_T
AC_TYPE_INT64_T
AC_TYPE_OFF_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T

dnl
dnl 7. Functions
dnl --------------------------
dnl
dnl Finally check for functions.

AC_CHECK_FUNCS([bzero])
AC_CHECK_FUNCS([clock_gettime])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([gettid])
AC_TYPE_UINT8_T

dnl
dnl 8. Output
dnl --------------------------
dnl
dnl Generate Makefiles

AC_OUTPUT(tools/Makefile platform/Makefile platform/test/Makefile common/Makefile player/Makefile deinterlacer/Makefile deinterlacer/src/Makefile deinterlacer/plugins/Makefile receiver/Makefile recorder/Makefile dproxy/Makefile gui/Makefile daemon/Makefile Makefile)
