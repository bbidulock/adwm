
bin_PROGRAMS = adwm

adwm_SOURCES = draw.c adwm.c ewmh.c parse.c config.h adwm.h
adwm_LDADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(SN_LIBS) $(XINERAMA_LIBS) $(XSYNC_LIBS) $(XRANDR_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm
adwm_CPPFLAGS = -include autoconf.h -DSYSCONFPATH=\"$(pkgconfdir)\"
adwm_CFLAGS = $(X11_CFLAGS) $(XFT_CFLAGS) $(XRANDR_CFLAGS) $(XSYNC_CFLAGS) $(XINERAMA_CFLAGS) $(SN_CFLAGS) $(XPM_CFLAGS) $(IMLIB2_CFLAGS)

noinst_PROGRAMS = ewmhpanel

ewmhpanel_SOURCES = ewmhpanel.c util.c util.h
ewmhpanel_LDADD = $(X11_LIBS) $(XFT_LIBS)
ewmhpanel_CPPFLAGS = -include autoconf.h
ewmhpanel_CFLAGS = $(X11_CFLAGS) $(XFT_CFLAGS)
