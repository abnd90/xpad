# Process this file with autoconf to produce a configure script.

# Sets up autoconf.
AC_INIT([Xpad],[4.2],[xpad-hackers@lists.launchpad.net])
AC_CONFIG_SRCDIR(src/xpad-app.c)
AC_CONFIG_HEADERS([config.h:config.h.in])

# Sets up automake.
AM_INIT_AUTOMAKE([1.9 gnu no-dist-gzip dist-bzip2])
AM_BINRELOC

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_INTLTOOL([0.31], [no-xml])

# Checks for libraries.
AC_PATH_XTRA
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.12 gio-2.0)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Sets up gettext.
GETTEXT_PACKAGE="$PACKAGE"
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [gettext domain])
AM_GLIB_GNU_GETTEXT

# Sets up a few prefix-relative directories
if test "x${prefix}" = "xNONE"; then
  PREFIX="${ac_default_prefix}"
else
  PREFIX="${prefix}"
fi
AC_DEFINE_UNQUOTED(LOCALE_DIR, "${PREFIX}/${DATADIRNAME}/locale",[Locale directory])
AC_DEFINE_UNQUOTED(THEME_DIR, "${PREFIX}/${DATADIRNAME}/icons",[Icon directory])

# Determines if we're debugging.
AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug=[no/yes]],[turn on debugging [default=no]]),
  enable_debug="$enableval",
  enable_debug=no)
if test "$enable_debug" = "yes"; then
  DEBUG_CFLAGS="-ansi -Wall -Werror -g -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGTK_MULTIHEAD_SAFE"
else
  DEBUG_CFLAGS=""
fi
AC_SUBST(DEBUG_CFLAGS)

# Sets up output files.
AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile images/Makefile po/Makefile.in autopackage/Makefile autopackage/default.apspec])
AC_OUTPUT

