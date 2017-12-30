#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ADWM],[0.7.1],[https://github.com/bbidulock/adwm/issues],[adwm],[https://github.com/bbidulock/adwm])
AC_REVISION([0.7.1])

AC_CONFIG_SRCDIR([src/adwm.h])
AC_CONFIG_HEADERS([src/autoconf.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([scripts])
AC_USE_SYSTEM_EXTENSIONS
AC_CANONICAL_TARGET

DATE='2017-12-19'
AC_SUBST([DATE])

MDOCDATE='December 19, 2017'
AC_SUBST([MDOCDATE])

BRANCH='0.7.1'
AC_SUBST([BRANCH])

# Initialize Automake
AM_INIT_AUTOMAKE([gnits 1.14 dist-xz no-dist-gzip std-options -Wall color-tests silent-rules tar-pax])
AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])
AC_PREFIX_DEFAULT([/usr])

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

AC_ARG_VAR([MARKDOWN],[markdown command. @<:@default=markdown@:>@])
AC_CHECK_PROGS([MARKDOWN],[markdown],[${am_missing_run}markdown],[$PATH])

AC_ARG_VAR([ASCIIDOC],[asciidoc command. @<:@default=asciidoctor@:>@])
AC_CHECK_PROGS([ASCIIDOC],[asciidoc asciidoctor],[${am_missing_run}asciidoc],[$PATH])

AC_ARG_VAR([ASCIIDOC_BACKEND],[asciidoc backend @<:@default=NONE@:>@])
if test -z "$ASCIIDOC_BACKEND"; then
	case "$ASCIIDOC" in
		asciidoc) ASCIIDOC_BACKEND="--backend=html4" ;;
		asciidoctor) ASCIIDOC_BACKEND="--backend=html5" ;;
		*) AC_MSG_WARN([ASCIIDOC $ASCIIDOC is not supported.]) ;;
	esac
fi

AC_ARG_VAR([ASCIIMAN],[asciidoc to manpage converter. @<:@default=a2x@:>@])
AC_CHECK_PROGS([ASCIIMAN],[a2x asciidoctor],[${am_missing_run}a2x],[$PATH])

AC_ARG_VAR([ASCIIMAN_FORMAT],[asciidoc manpage option @<:@default=NONE@:>@])
if test -z "$ASCIIMAN_FORMAT"; then
	case "$ASCIIMAN" in
		a2x) ASCIIMAN_FORMAT="--format=manpage" ;;
		asciidoctor) ASCIIMAN_FORMAT="--backend=manpage" ;;
		*) AC_MSG_WARN([ASCIIMAN $ASCIIMAN is not supported.]) ;;
	esac
fi

AC_ARG_VAR([LINKS],[links command. @<:@default=lynx@:>@])
AC_CHECK_PROGS([LINKS],[lynx links],[${am_missing_run}lynx],[$PATH])
if test "$LINKS" = "links"; then
	LINKS="links -codepage utf-8"
fi

AC_DISABLE_STATIC
LT_PATH_LD
LT_PREREQ([2.4.2])
LT_INIT([dlopen])
LT_LANG([C])
AC_PROG_LIBTOOL
AC_SUBST([LIBTOOL_DEPS])

PKG_PROG_PKG_CONFIG([0.14.0])

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_PATH_X
AC_PATH_XTRA
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

AC_ARG_ENABLE([xrender],
	AC_HELP_STRING([--disable-xrender],
		[Disable RENDER support @<:@default=auto@:>@]))
if test "x$enable_xrender" != xno ; then
	PKG_CHECK_MODULES([XRENDER],[xrender],
		[AC_DEFINE([RENDER],[1], [Define to 1 to support RENDER extension.])],
		[enable_xrender=no])
else
	AC_MSG_CHECKING([for xrender])
	AC_MSG_RESULT([disabled])
fi
AM_CONDITIONAL([HAVE_RENDER],[test  "x$enable_xrender" != "xno"])

AC_ARG_ENABLE([use-xrender],
	AC_HELP_STRING([--enable-use-xrender],
		[Enable use of xrenderfor drawing @<:@default=auto@:>@]))
if test "x$enable_xrender" != xno -a "x$enable_use_xrender" = xyes ; then
	AC_DEFINE([USE_RENDER],[1],[Define to 1 to use RENDER for rendering pixmaps.])
else
	enable_use_xrender=no
fi
AC_MSG_CHECKING([for xrender use])
AC_MSG_RESULT([${enable_use_xrender:-no}])

AC_ARG_ENABLE([xdamage],
	AC_HELP_STRING([--disable-xdamage],
		[Disable DAMAGE support @<:@default=auto@:>@]))
if test "x$enable_xdamage" != xno ; then
	PKG_CHECK_MODULES([XDAMAGE],[xdamage],
		[AC_DEFINE([DAMAGE],[1], [Define to 1 to support DAMAGE extension.])],
		[enable_xdamage=no])
else
	AC_MSG_CHECKING([for xdamage])
	AC_MSG_RESULT([disabled])
fi
AM_CONDITIONAL([HAVE_DAMAGE],[test  "x$enable_xdamage" != "xno"])

AC_ARG_ENABLE([xcomposite],
	AC_HELP_STRING([--disable-xcomposite],
		[Disable Composite support @<:@default=auto@:>@]))
if test "x$enable_xcomposite" != xno ; then
	PKG_CHECK_MODULES([XCOMPOSITE],[xcomposite],
		[AC_DEFINE([XCOMPOSITE],[1], [Define to 1 to support Composite extension.])],
		[enable_xcomposite=no])
else
	AC_MSG_CHECKING([for xcomposite])
	AC_MSG_RESULT([disabled])
fi
AM_CONDITIONAL([HAVE_XCOMPOSITE],[test  "x$enable_xcomposite" != "xno"])

AC_ARG_ENABLE([xrandr],
	AC_HELP_STRING([--disable-xrandr],
		[Disable XRANDR support @<:@default=auto@:>@]))
if test "x$enable_xrandr" != xno ; then
	PKG_CHECK_MODULES([XRANDR],[xrandr],
		[AC_DEFINE([XRANDR],[1], [Define to 1 to support XRANDR extension.])],
		[enable_xrandr=no])
else
	AC_MSG_CHECKING([for xrandr])
	AC_MSG_RESULT([disabled])
fi
AM_CONDITIONAL([HAVE_XRANDR],[test  "x$enable_xrandr" != "xno"])

AC_ARG_ENABLE([xinerama],
	AC_HELP_STRING([--disable-xinerama],
		[Disable XINERAMA support @<:@default=auto@:>@]))
if test "x$enable_xinerama" != xno ; then
	PKG_CHECK_MODULES([XINERAMA],[xinerama],
		[AC_DEFINE([XINERAMA],[1], [Define to 1 to support XINERAMA extension.])],
		[enable_xinerama=no])
else
	AC_MSG_CHECKING([for xinerama])
	AC_MSG_RESULT([disabled])
fi
AM_CONDITIONAL([HAVE_XINERAMA],[test  "x$enable_xinerama" != "xno"])

AC_ARG_ENABLE([xsync],
	AC_HELP_STRING([--disable-xsync],
		[Disable XSYNC support @<:@default=auto@:>@]))
if test "x$enable_xsync" != xno ; then
	PKG_CHECK_MODULES([XSYNC],[xext],
		[AC_DEFINE([SYNC],[1], [Define to 1 to support XSYNC extension.])],
		[enable_xsync=no])
else
	AC_MSG_CHECKING([for xsync])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([xshape],
	AC_HELP_STRING([--disable-xshape],
		[Disable SHAPE support @<:@default=auto@:>@]))
if test "x$enable_xshape" != xno ; then
	PKG_CHECK_MODULES([XSHAPE],[xext],
		[AC_DEFINE([SHAPE],[1], [Define to 1 to support SHAPE extension.])],
		[enable_xshape=no])
else
	AC_MSG_CHECKING([for xshape])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([startup-notification],
	AC_HELP_STRING([--disable-startup-notification],
		[Disable startup notification support @<:@default=auto@:>@]))
if test "x$enable_startup_notification" != xno ; then
	PKG_CHECK_MODULES([SN],[libstartup-notification-1.0],
		[AC_DEFINE([STARTUP_NOTIFICATION],[1], [Define to 1 to support startup notification.])],
		[enable_startup_notification=no])
else
	AC_MSG_CHECKING([for libstartup-notification-1.0])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([gdk-pixbuf],
	AC_HELP_STRING([--disable-gdk-pixbuf],
		[Disable gdk-pixbuf library @<:@default=auto@:>@]))
if test "x$enable_gdk_pixbuf" != xno ; then
	PKG_CHECK_MODULES([PIXBUF],[gdk-pixbuf-xlib-2.0],
		[AC_DEFINE([PIXBUF],[1],[Define to 1 to support GDK PIXBUF X11 graphics support.])],
		[enable_gdk_pixbuf=no])
else
	AC_MSG_CHECKING([for gdk-pixbuf-xlib-2.0])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([use-gdk-pixbuf],
	AC_HELP_STRING([--enable-use-gdk-pixbuf],
		[Enable use of gdk-pixbuf for drawing @<:@default=no@:>@]))
if test "x$enable_gdk_pixbuf" != xno -a "x$enable_use_gdk_pixbuf" = xyes ; then
	AC_DEFINE([USE_PIXBUF],[1],[Define to 1 to use GDK PIXBUF X11 for rendering pixmaps.])
else
	enable_use_gdk_pixbuf=no
fi
AC_MSG_CHECKING([for gdk-pixbuf use])
AC_MSG_RESULT([${enable_use_gdk_pixbuf:-no}])

AC_ARG_ENABLE([imlib2],
	AC_HELP_STRING([--disable-imlib2],
		[Disable imlib2 library @<:@default=auto@:>@]))
if test "x$enable_imlib2" != xno ; then
	PKG_CHECK_MODULES([IMLIB2],[imlib2],
		[AC_DEFINE([IMLIB2],[1],[Define to 1 to support IMLIB2 advanced graphics support.])],
		[enable_imlib2=no])
else
	AC_MSG_CHECKING([for imlib2])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([use-imlib2],
	AC_HELP_STRING([--enable-use-imlib2],
		[Enable use of imlib2 for drawing textures @<:@default=no@:>@]))
if test "x$enable_imlib2" != xno -a "x$enable_use_imlib2" = xyes ; then
	AC_DEFINE([USE_IMLIB2],[1],[Define to 1 to use IMLIB2 for rendering pixmaps and textures.])
else
	enable_use_imlib2=no
fi
AC_MSG_CHECKING([for imlib2 use])
AC_MSG_RESULT([${enable_use_imlib2:-no}])

AC_ARG_ENABLE([xcairo],
	AC_HELP_STRING([--disable-xcairo],
		[Disable cairo-xlib-xrender library @<:@default=auto@:>@]))
if test "x$enable_xcairo" != xno ; then
	PKG_CHECK_MODULES([XCAIRO],[cairo-xlib-xrender],
		[AC_DEFINE([XCAIRO],[1],[Define to 1 to support CAIRO XLIB XRENDER graphics support.])],
		[enable_xcairo=no])
else
	AC_MSG_CHECKING([for xcairo])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([use-xcairo],
	AC_HELP_STRING([--enable-use-xcairo],
		[Enable use of cairo-xlib-xrender to drawing @<:@default=no@:>@]))
if test "x$enable_xcairo" != xno -a "x$enable_use_xcairo" = xyes ; then
	AC_DEFINE([USE_XCAIRO],[1],[Define to 1 to use CAIRO XLIB XRENDER for rendering pixmaps.])
else
	enable_use_xcairo=no
fi
AC_MSG_CHECKING([for xcairo use])
AC_MSG_RESULT([${enable_use_xcairo:-no}])

AC_ARG_ENABLE([xpm],
	AC_HELP_STRING([--disable-libxpm],
		[Disable xpm library @<:@default=auto@:>@]))
if test "x$enable_xpm" != xno ; then
	PKG_CHECK_MODULES([XPM],[xpm],
		[AC_DEFINE([XPM],[1],[Define to 1 for XPM library support.])],
		[enable_xpm=no])
else
	AC_MSG_CHECKING([for xpm])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([libpng],
	AC_HELP_STRING([--disable-libpng],
		[Disable png library @<:@default=auto@:>@]))
if test "x$enable_libpng" != xno; then
	PKG_CHECK_MODULES([LIBPNG],[libpng >= 1.6],
		[AC_DEFINE([LIBPNG],[1],[Define to 1 for LIBPNG library support.])],
		[enable_libpng=no])
else
	AC_MSG_CHECKING([for libpng])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([libjpeg],
	AC_HELP_STRING([--disable-libjpeg],
		[Disable jpeg library @<:@default=auto@:>@]))
if test "x$enable_libjpeg" != xno; then
	PKG_CHECK_MODULES([LIBJPEG],[libjpeg >= 1.5],
		[AC_DEFINE([LIBJPEG],[1],[Define to 1 for LIBJPEG library support.])],
		[enable_libjpeg=no])
else
	AC_MSG_CHECKING([for libjpeg])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([libsvg],
	AC_HELP_STRING([--disable-libsvg],
		[Disable svg library @<:@default=auto@:>@]))
if test "x$enable_svg" != xno; then
	PKG_CHECK_MODULES([LIBSVG],[libsvg-cairo],
		[AC_DEFINE([LIBSVG],[1],[Define to 1 for LIBSVG library support.])],
		[enable_libsvg=no])
else
	AC_MSG_CHECKING([for libsvg])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([debug],
	AC_HELP_STRING([--enable-debug],
		[Enable debugging support @<:@default=disabled@:>@]))
if test "x$enable_debug" = xyes ; then
	AC_DEFINE_UNQUOTED([DEBUG],[],[Define to enable debugging])
fi
AC_MSG_CHECKING([for debug support])
AC_MSG_RESULT([${enable_debug:-no}])

AC_ARG_WITH([blackbox],
	AC_HELP_STRING([--with-blackbox],
		[Enable blackbox support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_BLACKBOX],[test  "x$with_blackbox" = "xyes"])
AC_MSG_CHECKING([for blackbox support])
AC_MSG_RESULT([${with_blackbox:-no}])
AC_ARG_WITH([fluxbox],
	AC_HELP_STRING([--with-fluxbox],
		[Enable fluxbox support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_FLUXBOX], [test  "x$with_fluxbox"  = "xyes"])
AC_MSG_CHECKING([for fluxbox support])
AC_MSG_RESULT([${with_fluxbox:-no}])
AC_ARG_WITH([icewm],
	AC_HELP_STRING([--with-icewm],
		[Enable icewm support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_ICEWM],   [test  "x$with_icewm"    = "xyes"])
AC_MSG_CHECKING([for icewm support])
AC_MSG_RESULT([${with_icewm:-no}])
AC_ARG_WITH([openbox],
	AC_HELP_STRING([--with-openbox],
		[Enable openbox support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_OPENBOX], [test  "x$with_openbox"  = "xyes"])
AC_MSG_CHECKING([for openbox support])
AC_MSG_RESULT([${with_openbox:-no}])
AC_ARG_WITH([metacity],
	AC_HELP_STRING([--with-metacity],
		[Enable metacity support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_METACITY],[test  "x$with_metacity" = "xyes"])
AC_MSG_CHECKING([for metacity support])
AC_MSG_RESULT([${with_metacity:-no}])
AC_ARG_WITH([mutter],
	AC_HELP_STRING([--with-mutter],
		[Enable mutter support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_MUTTER],  [test  "x$with_mutter"   = "xyes"])
AC_MSG_CHECKING([for mutter support])
AC_MSG_RESULT([${with_mutter:-no}])
AC_ARG_WITH([pekwm],
	AC_HELP_STRING([--with-pekwm],
		[Enable pekwm support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_PEKWM],   [test  "x$with_pekwm"    = "xyes"])
AC_MSG_CHECKING([for pekwm support])
AC_MSG_RESULT([${with_pekwm:-no}])
AC_ARG_WITH([waimea],
	AC_HELP_STRING([--with-waimea],
		[Enable waimea support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_WAIMEA],  [test  "x$with_waimea"   = "xyes"])
AC_MSG_CHECKING([for waimea support])
AC_MSG_RESULT([${with_waimea:-no}])
AC_ARG_WITH([wmaker],
	AC_HELP_STRING([--with-wmaker],
		[Enable wmaker support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_WMAKER],  [test  "x$with_wmaker"   = "xyes"])
AC_MSG_CHECKING([for wmaker support])
AC_MSG_RESULT([${with_wmaker:-no}])

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
AC_DEFINE([FOCBORDERCOLOR],["#ff9900"],[Focused border color.])
AC_DEFINE([FOCBUTTONCOLOR],["#ff9900"],[Focused button color.])
AC_DEFINE([FOCBGCOLOR],["#ff9900"],[Focused background color.])
AC_DEFINE([FOCFGCOLOR],["#ffffff"],[Focused forefoungd color.])
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
AC_DEFINE([WINPIXMAP],["window.xbm"],[Window icon pixmap.])

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
AC_DEFINE([VGRIPMAX],[40],[Virtual grip maximum size in pixels.])
AC_DEFINE([VGRIPMIN],[20],[Virtual grip minimum size in pixels.])

AC_DEFINE_UNQUOTED([MAXTAGS],[(sizeof(unsigned long long) * 8)],[Maximum number of tags.])
AC_DEFINE_UNQUOTED([SYSCONFPATH],["$pkgconfdir"],[System configuration path.])

AC_OUTPUT([Makefile
	   src/Makefile
	   doc/Makefile
	   doc/adwm.1
	   doc/adwm-config.5
	   doc/adwm-keys.5
	   doc/adwm-style.5
	   doc/adwm-theme.5
	   doc/adwm-dock.5
	   data/Makefile])

# vim: set ft=config:
