#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([adwm],[0.6.5],[https://github.com/bbidulock/adwm/issues])
AC_REVISION([0.6.5])
AC_CONFIG_SRCDIR([src/adwm.h])
AC_CONFIG_HEADERS([src/autoconf.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([scripts])
AC_USE_SYSTEM_EXTENSIONS

DATE='2016-08-01'
AC_SUBST([DATE])

# Initialize Automake
AM_INIT_AUTOMAKE([dist-xz no-dist-gzip])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_SED
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_AR
AC_PROG_MKDIR_P
AC_ARG_VAR([LD], [Linker loader command])

AC_DISABLE_STATIC
LT_PREREQ([2.4.2])
LT_INIT([dlopen])
LT_LANG([C])
AC_SUBST([LIBTOOL_DEPS])

PKG_PROG_PKG_CONFIG

# Checks for libraries.

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h locale.h malloc.h stdlib.h string.h strings.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([getcwd memmove regcomp select setlocale strchr strdup strerror strrchr])

PKG_CHECK_MODULES([X11],[x11])
PKG_CHECK_MODULES([XFT],[xft])
PKG_CHECK_MODULES([XFIXES],[xfixes])

AC_ARG_ENABLE([xrandr],
	AC_HELP_STRING([--disable-xrandr],
		[Disable XRANDR support @<:@default=auto@:>@]))
if test "x$enable_xrandr" != xno ; then
	PKG_CHECK_MODULES([XRANDR],[xrandr],
		[AC_DEFINE([XRANDR],[1], [Define to 1 to support XRANDR extension.])],
		[enable_xrandr=no])
fi
AC_MSG_CHECKING([for XRANDR support])
AC_MSG_RESULT([${enable_xrandr:-yes}])

AC_ARG_ENABLE([xinerama],
	AC_HELP_STRING([--disable-xinerama],
		[Disable XINERAMA support @<:@default=auto@:>@]))
if test "x$enable_xinerama" != xno ; then
	PKG_CHECK_MODULES([XINERAMA],[xinerama],
		[AC_DEFINE([XINERAMA],[1], [Define to 1 to support XINERAMA extension.])],
		[enable_xinerama=no])
fi
AC_MSG_CHECKING([for XINERAMA support])
AC_MSG_RESULT([${enable_xinerama:-yes}])

AC_ARG_ENABLE([xsync],
	AC_HELP_STRING([--disable-xsync],
		[Disable XSYNC support @<:@default=auto@:>@]))
if test "x$enable_xsync" != xno ; then
	PKG_CHECK_MODULES([XSYNC],[xext],
		[AC_DEFINE([SYNC],[1], [Define to 1 to support XSYNC extension.])],
		[enable_xsync=no])
fi
AC_MSG_CHECKING([for XSYNC support])
AC_MSG_RESULT([${enable_xsync:-yes}])

AC_ARG_ENABLE([startup-notification],
	AC_HELP_STRING([--disable-startup-notification],
		[Disable startup notification support @<:@default=auto@:>@]))
if test "x$enable_startup_notification" != xno ; then
	PKG_CHECK_MODULES([SN],[libstartup-notification-1.0],
		[AC_DEFINE([STARTUP_NOTIFICATION],[1], [Define to 1 to support startup notification.])],
		[enable_startup_notification=no])
fi
AC_MSG_CHECKING([for startup notification support])
AC_MSG_RESULT([${enable_startup_notification:-yes}])

AC_ARG_ENABLE([imlib2],
	AC_HELP_STRING([--disable-imlib2],
		[Disable imlib2 library @<:@default=auto@:>@]))
if test "x$enable_imlib2" != xno ; then
	PKG_CHECK_MODULES([IMLIB2],[imlib2],
		[AC_DEFINE([IMLIB2],[1],[Define to 1 to support IMLIB2 advanced graphics support.])],
		[enable_imlib2=no])
fi
AC_MSG_CHECKING([for imlib2 support])
AC_MSG_RESULT([${enable_imlib2:-yes}])

AC_ARG_ENABLE([xpm],
	AC_HELP_STRING([--disable-libxpm],
		[Disable xpm library @<:@default=auto@:>@]))
if test "x$enable_xpm" != xno ; then
	PKG_CHECK_MODULES([XPM],[xpm],
		[AC_DEFINE([XPM],[1],[Defined to 1 for XPM library support.])],
		[enable_xpm=no])
fi
AC_MSG_CHECKING([for libxpm support])
AC_MSG_RESULT([${enable_libxpm:-yes}])

AC_ARG_ENABLE([debug],
	AC_HELP_STRING([--enable-debug],
		[Enable debugging support @<:@default=disabled@:>@]))
if test "x$enable_debug" = xyes ; then
	AC_DEFINE_UNQUOTED([DEBUG],[],[Define to enable debugging])
fi
AC_MSG_CHECKING([for debug support])
AC_MSG_RESULT([${enable_debug:-no}])

test "x$prefix" = xNONE && prefix="$ac_default_prefix"
sharedir="$datadir"
eval "sharedir=\"$sharedir\""
eval "sharedir=\"$sharedir\""
eval "sharedir=\"$sharedir\""
AC_SUBST([sharedir])

pkgconfdir="$datadir/adwm"
eval "pkgconfdir=\"$pkgconfdir\""
eval "pkgconfdir=\"$pkgconfdir\""
eval "pkgconfdir=\"$pkgconfdir\""
AC_SUBST([pkgconfdir])

adwmmoddir='${libdir}/adwm/modules'
AC_SUBST([adwmmoddir])

AC_DEFINE([BORDERPX],[1],[Number of pixels for window borders.])
AC_DEFINE([MARGINPX],[0],[Number of pixels for window margins.])
AC_DEFINE([NF_OPACITY],[0.9],[Opacity.])
AC_DEFINE([FONT],["fixed-9"],[Default font.])
AC_DEFINE([NORMBORDERCOLOR],["#cccccc"],[Normal border color.])
AC_DEFINE([NORMBUTTONCOLOR],["#cccccc"],[Normal button color.])
AC_DEFINE([NORMBGCOLOR],["#cccccc"],[Normal background color.])
AC_DEFINE([NORMFGCOLOR],["#000000"],[Normal forefoungd color.])
AC_DEFINE([SELBORDERCOLOR],["#ff9900"],[Selected border color.])
AC_DEFINE([SELBUTTONCOLOR],["#ff9900"],[Selected button color.])
AC_DEFINE([SELBGCOLOR],["#ff9900"],[Selected background color.])
AC_DEFINE([SELFGCOLOR],["#ffffff"],[Selected forefoungd color.])
AC_DEFINE([MENUPIXMAP],["menu.xbm"],[Menu pixmap.])
AC_DEFINE([ICONPIXMAP],["iconify.xbm"],[Iconify pixmap.])
AC_DEFINE([MAXPIXMAP],["max.xbm"],[Maximize pixmap.])
AC_DEFINE([CLOSEPIXMAP],["close.xbm"],[Close pixmap.])
AC_DEFINE([SHADEPIXMAP],["shade.xbm"],[Shade pixmap.])
AC_DEFINE([STICKPIXMAP],["stick.xbm"],[Stick pixmap.])
AC_DEFINE([LHALFPIXMAP],["lhalf.xbm"],[Left half pixmap.])
AC_DEFINE([RHALFPIXMAP],["rhalf.xbm"],[Right half pixmap.])
AC_DEFINE([FLOATPIXMAP],["float.xbm"],[Float pixmap.])
AC_DEFINE([FILLPIXMAP],["fill.xbm"],[Fill pixmap.])
AC_DEFINE([SIZEPIXMAP],["resize.xbm"],[Resize pixmap.])

AC_DEFINE([MINWIDTH],[12],[Minimum window width in pixels.])
AC_DEFINE([MINHEIGHT],[12],[Minimum window height in pixels.])
AC_DEFINE([TITLEHEIGHT],[0],[Default title height.])
AC_DEFINE([GRIPHEIGHT],[0],[Default grip height.])
AC_DEFINE([COMMAND],["xterm"],[Default root command (button 3).])
AC_DEFINE([COMMAND2],[NULL],[Default root command (button 2).])
AC_DEFINE([COMMAND3],[NULL],[Default root command (button 1).])
AC_DEFINE([MENUCOMMAND],[NULL],[Default menu command.])
AC_DEFINE([DEFMWFACT],[0.6],[Master width factor (0.0 .. 1.0).])
AC_DEFINE([DEFMHFACT],[1.0],[Master height factor (0.0 .. 1.0).])
AC_DEFINE([DEFNMASTER],[1],[Number of windows in master area.])
AC_DEFINE([DEFNCOLUMNS],[3],[Number of columns in grid layout.])
AC_DEFINE([SNAP],[5],[Snap pixel distance.])
AC_DEFINE([DECORATETILED],[0],[Set to 1 to draw titles (by default) in tiled layouts.])
AC_DEFINE([DECORATEMAX],[1],[Set to 1 to draw titles (by default) on maximized windows.])

AC_DEFINE_UNQUOTED([MAXTAGS],[(sizeof(unsigned long long) * 8)],[Maximum number of tags.])
AC_DEFINE_UNQUOTED([SYSCONFPATH],["$pkgconfdir"],[System configuration path.])

AC_OUTPUT([Makefile
	   src/Makefile
	   doc/Makefile
	   data/Makefile])

# vim: set ft=config:
