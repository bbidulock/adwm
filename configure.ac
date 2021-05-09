#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([ADWM],[0.7.12],[https://github.com/bbidulock/adwm/issues],[adwm],[https://github.com/bbidulock/adwm])
AC_REVISION([0.7.12])

# set better defaults
AC_PREFIX_DEFAULT([/usr])
if test "$prefix" = /usr -o "$prefix" = NONE; then
	test "$sysconfdir" = '${prefix}/etc' && sysconfdir=/etc
	test "$sharedstatedir" = '${prefix}/com' && sharedstatedir=/var
	test "$localstatedir" = '${prefix}/var' && localstatedir=/var
fi

AC_CONFIG_SRCDIR([src/adwm.h])
AC_CONFIG_HEADERS([src/autoconf.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([scripts])
AC_USE_SYSTEM_EXTENSIONS
AC_CANONICAL_TARGET

DATE='2021-05-08'
AC_SUBST([DATE])

MDOCDATE='May 8, 2021'
AC_SUBST([MDOCDATE])

BRANCH='0.7.12'
AC_SUBST([BRANCH])

# Initialize Automake
AM_INIT_AUTOMAKE([gnits 1.16 dist-lzip no-dist-gzip std-options -Wall color-tests silent-rules tar-pax])
AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])

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
AC_SUBST([LIBTOOL_DEPS])

PKG_PROG_PKG_CONFIG([0.14.0])

# Checks for libraries.

# Checks for header files.
AS_BOX([Header Files])
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_CXX
AC_PROG_EGREP

AC_PATH_X
AC_PATH_XTRA
AC_CHECK_HEADERS([fcntl.h locale.h malloc.h stdlib.h string.h strings.h unistd.h sys/time.h])
AC_CHECK_HEADER_STDBOOL

# Checks for typedefs, structures, and compiler characteristics.
AS_BOX([Typedefs, Structures, Compiler])
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_OFF_T
AC_TYPE_UID_T

# Checks for library functions.
AS_BOX([Library Functions])
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([getcwd memmove regcomp select setlocale strchr strdup strerror strrchr atexit floor gethostname gettimeofday memset sqrt strcasecmp strncasecmp strndup strspn strstr strtol strtoul])
AC_FUNC_STRNLEN
AC_FUNC_STRTOD

AS_BOX([X11 Extension Libraries])

PKG_CHECK_MODULES([X11],[x11])
PKG_CHECK_MODULES([XFIXES],[xfixes])
dnl PKG_CHECK_MODULES([SYSTEMD],[libsystemd])

AC_ARG_ENABLE([xdamage],
	AS_HELP_STRING([--disable-xdamage],[Disable DAMAGE support @<:@default=auto@:>@]))
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
	AS_HELP_STRING([--disable-xcomposite],[Disable Composite support @<:@default=auto@:>@]))
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
	AS_HELP_STRING([--disable-xrandr],[Disable XRANDR support @<:@default=auto@:>@]))
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
	AS_HELP_STRING([--disable-xinerama],[Disable XINERAMA support @<:@default=auto@:>@]))
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
	AS_HELP_STRING([--disable-xsync],[Disable XSYNC support @<:@default=auto@:>@]))
if test "x$enable_xsync" != xno ; then
	PKG_CHECK_MODULES([XSYNC],[xext],
		[AC_DEFINE([SYNC],[1], [Define to 1 to support XSYNC extension.])],
		[enable_xsync=no])
else
	AC_MSG_CHECKING([for xsync])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([xshape],
	AS_HELP_STRING([--disable-xshape],[Disable SHAPE support @<:@default=auto@:>@]))
if test "x$enable_xshape" != xno ; then
	PKG_CHECK_MODULES([XSHAPE],[xext],
		[AC_DEFINE([SHAPE],[1], [Define to 1 to support SHAPE extension.])],
		[enable_xshape=no])
else
	AC_MSG_CHECKING([for xshape])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([sm],
	AS_HELP_STRING([--disable-sm],[Disable session management support @<:@default=auto@:>@]))
if test "x$enable_sm" != xno ; then
	PKG_CHECK_MODULES([SMLIB],[ice sm],
		[AC_DEFINE([SMLIB],[1],[Default to 1 to support SM library.])],
		[enable_smlib=no])
else
	AC_MSG_CHECKING([for smlib])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([startup-notification],
	AS_HELP_STRING([--disable-startup-notification],[Disable startup notification support @<:@default=auto@:>@]))
if test "x$enable_startup_notification" != xno ; then
	PKG_CHECK_MODULES([SN],[libstartup-notification-1.0],
		[AC_DEFINE([STARTUP_NOTIFICATION],[1], [Define to 1 to support startup notification.])],
		[enable_startup_notification=no])
else
	AC_MSG_CHECKING([for libstartup-notification-1.0])
	AC_MSG_RESULT([disabled])
fi

AS_BOX([Rendering Libraries])

AC_ARG_ENABLE([xrender],
	AS_HELP_STRING([--disable-xrender],[Disable RENDER support @<:@default=auto@:>@]))
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
	AS_HELP_STRING([--enable-use-xrender],[Enable use of xrender for drawing @<:@default=auto@:>@]))
if test "x$enable_xrender" != xno -a "x$enable_use_xrender" = xyes ; then
	AC_DEFINE([USE_RENDER],[1],[Define to 1 to use RENDER for rendering pixmaps.])
	enable_use_pango_cairo=no
else
	enable_use_xrender=no
fi
AC_MSG_CHECKING([for xrender use])
AC_MSG_RESULT([${enable_use_xrender:-no}])

AC_ARG_ENABLE([gdk-pixbuf],
	AS_HELP_STRING([--disable-gdk-pixbuf],[Disable gdk-pixbuf library @<:@default=auto@:>@]))
if test "x$enable_gdk_pixbuf" != xno ; then
	PKG_CHECK_MODULES([PIXBUF],[gdk-pixbuf-xlib-2.0],
		[AC_DEFINE([PIXBUF],[1],[Define to 1 to support GDK PIXBUF X11 graphics support.])],
		[enable_gdk_pixbuf=no])
else
	AC_MSG_CHECKING([for gdk-pixbuf-xlib-2.0])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([use-gdk-pixbuf],
	AS_HELP_STRING([--enable-use-gdk-pixbuf],[Enable use of gdk-pixbuf for drawing @<:@default=no@:>@]))
if test "x$enable_gdk_pixbuf" != xno -a "x$enable_use_gdk_pixbuf" = xyes ; then
	AC_DEFINE([USE_PIXBUF],[1],[Define to 1 to use GDK PIXBUF X11 for rendering pixmaps.])
else
	enable_use_gdk_pixbuf=no
fi
AC_MSG_CHECKING([for gdk-pixbuf use])
AC_MSG_RESULT([${enable_use_gdk_pixbuf:-no}])

AC_ARG_ENABLE([imlib2],
	AS_HELP_STRING([--disable-imlib2],[Disable imlib2 library @<:@default=auto@:>@]))
if test "x$enable_imlib2" != xno ; then
	PKG_CHECK_MODULES([IMLIB2],[imlib2],
		[AC_DEFINE([IMLIB2],[1],[Define to 1 to support IMLIB2 advanced graphics support.])],
		[enable_imlib2=no])
else
	AC_MSG_CHECKING([for imlib2])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([use-imlib2],
	AS_HELP_STRING([--enable-use-imlib2],[Enable use of imlib2 for drawing textures @<:@default=no@:>@]))
if test "x$enable_imlib2" != xno -a "x$enable_use_imlib2" = xyes ; then
	AC_DEFINE([USE_IMLIB2],[1],[Define to 1 to use IMLIB2 for rendering pixmaps and textures.])
else
	enable_use_imlib2=no
fi
AC_MSG_CHECKING([for imlib2 use])
AC_MSG_RESULT([${enable_use_imlib2:-no}])

AC_ARG_ENABLE([xcairo],
	AS_HELP_STRING([--disable-xcairo],[Disable cairo-xlib-xrender library @<:@default=auto@:>@]))
if test "x$enable_xcairo" != xno ; then
	PKG_CHECK_MODULES([XCAIRO],[cairo-xlib-xrender],
		[AC_DEFINE([XCAIRO],[1],[Define to 1 to support CAIRO XLIB XRENDER graphics support.])],
		[enable_xcairo=no])
else
	AC_MSG_CHECKING([for xcairo])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([use-xcairo],
	AS_HELP_STRING([--enable-use-xcairo],[Enable use of cairo-xlib-xrender to drawing @<:@default=no@:>@]))
if test "x$enable_xcairo" != xno -a "x$enable_use_xcairo" = xyes ; then
	AC_DEFINE([USE_XCAIRO],[1],[Define to 1 to use CAIRO for rendering pixmaps.])
	enable_use_xft=no
	enable_use_pango_xft=no
	enable_use_pango_cairo=yes
else
	enable_use_xcairo=no
fi
AC_MSG_CHECKING([for xcairo use])
AC_MSG_RESULT([${enable_use_xcairo:-no}])

AS_BOX([Image Loading Libraries])

AC_ARG_ENABLE([xpm],
	AS_HELP_STRING([--disable-libxpm],[Disable xpm library @<:@default=auto@:>@]))
if test "x$enable_xpm" != xno ; then
	PKG_CHECK_MODULES([XPM],[xpm],
		[AC_DEFINE([XPM],[1],[Define to 1 for XPM library support.])],
		[enable_xpm=no])
else
	AC_MSG_CHECKING([for xpm])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([libpng],
	AS_HELP_STRING([--disable-libpng],[Disable png library @<:@default=auto@:>@]))
if test "x$enable_libpng" != xno; then
	PKG_CHECK_MODULES([LIBPNG],[libpng >= 1.6],
		[AC_DEFINE([LIBPNG],[1],[Define to 1 for LIBPNG library support.])],
		[enable_libpng=no])
else
	AC_MSG_CHECKING([for libpng])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([libjpeg],
	AS_HELP_STRING([--disable-libjpeg],[Disable jpeg library @<:@default=auto@:>@]))
if test "x$enable_libjpeg" != xno; then
	PKG_CHECK_MODULES([LIBJPEG],[libjpeg >= 1.5],
		[AC_DEFINE([LIBJPEG],[1],[Define to 1 for LIBJPEG library support.])],
		[enable_libjpeg=no])
else
	AC_MSG_CHECKING([for libjpeg])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([libsvg],
	AS_HELP_STRING([--disable-libsvg],[Disable svg library @<:@default=auto@:>@]))
if test "x$enable_svg" != xno; then
	PKG_CHECK_MODULES([LIBSVG],[libsvg-cairo],
		[AC_DEFINE([LIBSVG],[1],[Define to 1 for LIBSVG library support.])],
		[enable_libsvg=no])
else
	AC_MSG_CHECKING([for libsvg])
	AC_MSG_RESULT([disabled])
fi

AS_BOX([Font Rendering])

AC_ARG_ENABLE([xft],
	AS_HELP_STRING([--disable-xft],[Disable XFT support @<:@default=auto@:>@]))
if test "x$enable_xft" != xno ; then
	PKG_CHECK_MODULES([XFT],[xft],
		[AC_DEFINE([XFT],[1],[Define to 1 to support XFT.])],
		[enable_xft=no])
else
	AC_MSG_CHECKING([for xft])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([use-xft],
	AS_HELP_STRING([--disable-use-xft],[Disable use of XFT for drawing glyphs @<:@default=true@:>@]))
if test "x$enable_xft" != xno -a "x$enable_use_xft" != xno ; then
	AC_DEFINE([USE_XFT],[1],[Define to 1 to use XFT for rendering glyphs.])
else
	if test "x$enable_use_xft" = xyes ; then
		AC_MSG_ERROR([xft unavailable])
	else
		enable_use_xft=no
	fi
fi
AC_MSG_CHECKING([for xft use])
AC_MSG_RESULT([${enable_use_xft:-no}])

AH_TEMPLATE([PANGO],[Define to 1 when PANGO is supported.])
AH_TEMPLATE([USE_PANGO],[Define to 1 to use PANGO for rendering glyphs.])

AC_ARG_ENABLE([pango-xft],
	AS_HELP_STRING([--disable-pango-xft],[Disable PANGO XFT support @<:@default=auto@:>@]))
if test "x$enable_pango_xft" != xno ; then
	PKG_CHECK_MODULES([PANGOXFT],[pangoxft],
		[AC_DEFINE([PANGO],[1])
		 AC_DEFINE([PANGOXFT],[1],[Define to 1 to support PANGO-XFT.])],
		[enable_pango_xft=no])
else
	AC_MSG_CHECKING([for pango xft])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([use-pango-xft],
	AS_HELP_STRING([--enable-use-pango-xft],[Enable use of PANGO-XFT for drawing glyphs @<:@default=false@:>@]))
if test "x$enable_pango_xft" = xyes -a "x$enable_use_pango_xft" = xyes ; then
	AC_DEFINE([USE_PANGO],[1])
	AC_DEFINE([USE_PANGO_XFT],[1],[Define to 1 to use PANGO XFT backend for rendering glyphs.])
else
	enable_use_pango_xft=no
fi
AC_MSG_CHECKING([for pango xft use])
AC_MSG_RESULT([${enable_use_pango_xft:-no}])

AC_ARG_ENABLE([pango-cairo],
	AS_HELP_STRING([--disable-pango-cairo],[Disable PANGO CAIRO support @<:@default=auto@:>@]))
if test "x$enable_pango_cairo" != xno ; then
	PKG_CHECK_MODULES([PANGOCAIRO],[pangocairo],
		[AC_DEFINE([PANGO],[1])
		 AC_DEFINE([PANGOCAIRO],[1],[Define to 1 to support PANGO-CAIRO.])],
		[enable_pango_cairo=no])
else
	AC_MSG_CHECKING([for pango cairo])
	AC_MSG_RESULT([disabled])
fi

AC_ARG_ENABLE([use-pango-cairo],
	AS_HELP_STRING([--enable-use-pango-cairo],[Enable use of PANGO-CAIRO for drawing glyphs @<:@default=false@:>@]))
if test "x$enable_pango_cairo" = xyes -a "x$enable_use_pango_cairo" = xyes ; then
	AC_DEFINE([USE_PANGO],[1])
	AC_DEFINE([USE_PANGO_CAIRO],[1],[Define to 1 to use PANGO CAIRO backend for rendering glyphs.])
else
	if test "x$enable_use_pango_cairo" = xyes ; then
		AC_MSG_ERROR([pango cairo unavailable.])
	else
		enable_use_pango_cairo=no
	fi
fi
AC_MSG_CHECKING([for pango cairo use])
AC_MSG_RESULT([${enable_use_pango_cairo:-no}])

AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug],[Enable debugging support @<:@default=disabled@:>@]))
if test "x$enable_debug" = xyes ; then
	AC_DEFINE_UNQUOTED([DEBUG],[],[Define to enable debugging])
fi
AC_MSG_CHECKING([for debug support])
AC_MSG_RESULT([${enable_debug:-no}])

AS_BOX([Foreign Window Manager Support])

AC_ARG_WITH([blackbox],
	AS_HELP_STRING([--with-blackbox],[Enable blackbox support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_BLACKBOX],[test  "x$with_blackbox" = "xyes"])
AC_MSG_CHECKING([for blackbox support])
AC_MSG_RESULT([${with_blackbox:-no}])
AC_ARG_WITH([fluxbox],
	AS_HELP_STRING([--with-fluxbox],[Enable fluxbox support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_FLUXBOX], [test  "x$with_fluxbox"  = "xyes"])
AC_MSG_CHECKING([for fluxbox support])
AC_MSG_RESULT([${with_fluxbox:-no}])
AC_ARG_WITH([icewm],
	AS_HELP_STRING([--with-icewm],[Enable icewm support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_ICEWM],   [test  "x$with_icewm"    = "xyes"])
AC_MSG_CHECKING([for icewm support])
AC_MSG_RESULT([${with_icewm:-no}])
AC_ARG_WITH([openbox],
	AS_HELP_STRING([--with-openbox],[Enable openbox support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_OPENBOX], [test  "x$with_openbox"  = "xyes"])
AC_MSG_CHECKING([for openbox support])
AC_MSG_RESULT([${with_openbox:-no}])
AC_ARG_WITH([metacity],
	AS_HELP_STRING([--with-metacity],[Enable metacity support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_METACITY],[test  "x$with_metacity" = "xyes"])
AC_MSG_CHECKING([for metacity support])
AC_MSG_RESULT([${with_metacity:-no}])
AC_ARG_WITH([mutter],
	AS_HELP_STRING([--with-mutter],[Enable mutter support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_MUTTER],  [test  "x$with_mutter"   = "xyes"])
AC_MSG_CHECKING([for mutter support])
AC_MSG_RESULT([${with_mutter:-no}])
AC_ARG_WITH([pekwm],
	AS_HELP_STRING([--with-pekwm],[Enable pekwm support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_PEKWM],   [test  "x$with_pekwm"    = "xyes"])
AC_MSG_CHECKING([for pekwm support])
AC_MSG_RESULT([${with_pekwm:-no}])
AC_ARG_WITH([waimea],
	AS_HELP_STRING([--with-waimea],[Enable waimea support @<:@default=disabled@:>@]))
AM_CONDITIONAL([WITH_WAIMEA],  [test  "x$with_waimea"   = "xyes"])
AC_MSG_CHECKING([for waimea support])
AC_MSG_RESULT([${with_waimea:-no}])
AC_ARG_WITH([wmaker],
	AS_HELP_STRING([--with-wmaker],[Enable wmaker support @<:@default=disabled@:>@]))
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

AC_DEFINE_UNQUOTED([MAXTAGS],[(int)((sizeof(unsigned long long) * 8) - 1)],[Maximum number of tags.])
AC_DEFINE_UNQUOTED([SYSCONFPATH],["$pkgconfdir"],[System configuration path.])

AC_CONFIG_FILES([Makefile
	   src/Makefile
	   doc/Makefile
	   doc/adwm.1
	   doc/adwm-config.5
	   doc/adwm-keys.5
	   doc/adwm-buttons.5
	   doc/adwm-rules.5
	   doc/adwm-style.5
	   doc/adwm-theme.5
	   doc/adwm-dock.5
	   data/Makefile])
AC_OUTPUT

# vim: set ft=config:
