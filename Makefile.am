
SUBDIRS = tests doc data

AM_CPPFLAGS = -include autoconf.h -DSYSCONFPATH=\"$(pkgconfdir)\"
AM_CFLAGS = $(X11_CFLAGS) $(XFT_CFLAGS) $(XRANDR_CFLAGS) $(XSYNC_CFLAGS) $(XINERAMA_CFLAGS) $(SN_CFLAGS) $(XPM_CFLAGS) $(IMLIB2_CFLAGS)

bin_PROGRAMS = adwm

adwm_SOURCES = draw.c adwm.c ewmh.c parse.c config.h adwm.h
adwm_LDADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(SN_LIBS) $(XINERAMA_LIBS) $(XSYNC_LIBS) $(XRANDR_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm

EXTRA_DIST = LICENSE README README.echinus TODO ChangeLog

