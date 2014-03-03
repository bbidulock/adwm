
AM_CFLAGS = $(X11_CFLAGS) $(XFT_CFLAGS) $(XRANDR_CFLAGS) $(XSYNC_CFLAGS) $(XINERAMA_CFLAGS) $(SN_CFLAGS) $(XPM_CFLAGS) $(IMLIB2_CFLAGS)
AM_CPPFLAGS = -include autoconf.h -DSYSCONFPATH=\"$(pkgconfdir)\"

bin_PROGRAMS = adwm

adwm_SOURCES = adwm.h draw.h layout.h parse.h adwm.c draw.c ewmh.c layout.c parse.c resource.c texture.c
adwm_LDADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(SN_LIBS) $(XINERAMA_LIBS) $(XSYNC_LIBS) $(XRANDR_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm
adwm_CPPFLAGS = $(AM_CPPFLAGS)
adwm_LDFLAGS = -export-dynamic -R $(adwmmoddir) -ldl -dlpreopen adwm-adwm.la

noinst_PROGRAMS = ewmhpanel

ewmhpanel_SOURCES = ewmhpanel.c util.c util.h
ewmhpanel_LDADD = $(X11_LIBS) $(XFT_LIBS)

adwmmod_LTLIBRARIES = \
	adwm-adwm.la \
	adwm-blackbox.la \
	adwm-fluxbox.la \
	adwm-icewm.la \
	adwm-openbox.la \
	adwm-metacity.la \
	adwm-mutter.la \
	adwm-pekwm.la \
	adwm-waimea.la \
	adwm-wmaker.la

adwm_adwm_la_CPPFLAGS = $(AM_CPPFLAGS) -DNAME=\"adwm-adwm\"
adwm_adwm_la_SOURCES = adwm.h resource.h parse.h adwm_adwm.c
adwm_adwm_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_adwm_la_LIBADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm

adwm_blackbox_la_CPPFLAGS = $(AM_CPPFLAGS) -DNAME=\"adwm-blackbox\"
adwm_blackbox_la_SOURCES = adwm.h resource.h parse.h adwm_blackbox.c
adwm_blackbox_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_blackbox_la_LIBADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm

adwm_fluxbox_la_CPPFLAGS = $(AM_CPPFLAGS) -DNAME=\"adwm-fluxbox\"
adwm_fluxbox_la_SOURCES = adwm.h resource.h parse.h adwm_fluxbox.c
adwm_fluxbox_la_LDFLAGS =  -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_fluxbox_la_LIBADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm

adwm_icewm_la_CPPFLAGS = $(AM_CPPFLAGS) -DNAME=\"adwm-icewm\"
adwm_icewm_la_SOURCES = adwm.h resource.h parse.h adwm_icewm.c
adwm_icewm_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_icewm_la_LIBADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm

adwm_metacity_la_CPPFLAGS = $(AM_CPPFLAGS) -DNAME=\"adwm-metacity\"
adwm_metacity_la_SOURCES = adwm.h resource.h parse.h adwm_metacity.c
adwm_metacity_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_metacity_la_LIBADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm

adwm_mutter_la_CPPFLAGS = $(AM_CPPFLAGS) -DNAME=\"adwm-mutter\"
adwm_mutter_la_SOURCES = adwm.h resource.h parse.h adwm_mutter.c
adwm_mutter_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_mutter_la_LIBADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm

adwm_openbox_la_CPPFLAGS = $(AM_CPPFLAGS) -DNAME=\"adwm-openbox\"
adwm_openbox_la_SOURCES = adwm.h resource.h parse.h adwm_openbox.c
adwm_openbox_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_openbox_la_LIBADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm

adwm_pekwm_la_CPPFLAGS = $(AM_CPPFLAGS) -DNAME=\"adwm-pekwm\"
adwm_pekwm_la_SOURCES = adwm.h resource.h parse.h adwm_pekwm.c
adwm_pekwm_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_pekwm_la_LIBADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm

adwm_waimea_la_CPPFLAGS = $(AM_CPPFLAGS) -DNAME=\"adwm-waimea\"
adwm_waimea_la_SOURCES = adwm.h resource.h parse.h adwm_waimea.c
adwm_waimea_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_waimea_la_LIBADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm

adwm_wmaker_la_CPPFLAGS = $(AM_CPPFLAGS) -DNAME=\"adwm-wmaker\"
adwm_wmaker_la_SOURCES = adwm.h resource.h parse.h adwm_wmaker.c
adwm_wmaker_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_wmaker_la_LIBADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm

