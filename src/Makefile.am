
AM_CFLAGS = \
	$(X11_CFLAGS) \
	$(XFIXES_CFLAGS) \
	$(XFT_CFLAGS) \
	$(XSHAPE_CFLAGS) \
	$(XSYNC_CFLAGS) \
	$(XINERAMA_CFLAGS) \
	$(XRANDR_CFLAGS) \
	$(XDAMAGE_CFLAGS) \
	$(XCOMPOSITE_CFLAGS) \
	$(XRENDER_CFLAGS) \
	$(SN_CFLAGS) \
	$(XPM_CFLAGS) \
	$(LIBPNG_CFLAGS) \
	$(LIBJPEG_CFLAGS) \
	$(LIBSVG_CFLAGS) \
	$(PIXBUF_CFLAGS) \
	$(IMLIB2_CFLAGS) \
	$(XCAIRO_CFLAGS) \
	$(PANGOXFT_CFLAGS) \
	$(PANGOCAIRO_CFLAGS) \
	$(SMLIB_CFLAGS)
AM_CPPFLAGS = -include autoconf.h

bin_PROGRAMS = adwm

adwm_SOURCES = adwm.h actions.h config.h draw.h imlib.h pixbuf.h xcairo.h render.h ximage.h xlib.h \
	       ewmh.h image.h layout.h parse.h resource.h tags.h texture.h icons.h util.h session.h save.h restore.h \
	       adwm.c actions.c config.c draw.c imlib.c pixbuf.c xcairo.c render.c ximage.c xlib.c \
	       ewmh.c image.c layout.c parse.c resource.c tags.c texture.c icons.c session.c save.c restore.c
adwm_LDADD = \
	$(SMLIB_LIBS) \
	$(PANGOCAIRO_LIBS) \
	$(PANGOXFT_LIBS) \
	$(XCAIRO_LIBS) \
	$(IMLIB2_LIBS) \
	$(PIXBUF_LIBS) \
	$(LIBSVG_LIBS) \
	$(LIBJPEG_LIBS) \
	$(LIBPNG_LIBS) \
	$(XPM_LIBS) \
	$(SN_LIBS) \
	$(XRENDER_LIBS) \
	$(XCOMPOSITE_LIBS) \
	$(XDAMAGE_LIBS) \
	$(XRANDR_LIBS) \
	$(XINERAMA_LIBS) \
	$(XSYNC_LIBS) \
	$(XSHAPE_LIBS) \
	$(XFT_LIBS) \
	$(XFIXES_LIBS) \
	$(X11_LIBS) \
	-lm
adwm_LDFLAGS = -export-dynamic -R $(adwmmoddir) -ldl -dlpreopen adwm-adwm.la

noinst_PROGRAMS = ewmhpanel

ewmhpanel_SOURCES = util.h ewmhpanel.c util.c
ewmhpanel_LDADD = $(X11_LIBS) $(XFT_LIBS)

adwmmod_LTLIBRARIES = \
	adwm-adwm.la

adwm_adwm_la_SOURCES = adwm_adwm.c
adwm_adwm_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_adwm_la_LIBADD = $(X11_LIBS)

adwm_blackbox_la_SOURCES = adwm_blackbox.c
adwm_blackbox_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_blackbox_la_LIBADD = $(X11_LIBS)

if WITH_BLACKBOX
adwmmod_LTLIBRARIES += adwm-blackbox.la
endif

adwm_fluxbox_la_SOURCES = adwm_fluxbox.c
adwm_fluxbox_la_LDFLAGS =  -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_fluxbox_la_LIBADD = 

if WITH_FLUXBOX
adwmmod_LTLIBRARIES += adwm-fluxbox.la
endif

adwm_icewm_la_SOURCES = adwm_icewm.c
adwm_icewm_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_icewm_la_LIBADD = 

if WITH_ICEWM
adwmmod_LTLIBRARIES += adwm-icewm.la
endif

adwm_metacity_la_SOURCES = adwm_metacity.c
adwm_metacity_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_metacity_la_LIBADD = 

if WITH_OPENBOX
adwmmod_LTLIBRARIES += adwm-openbox.la
endif

adwm_mutter_la_SOURCES = adwm_mutter.c
adwm_mutter_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_mutter_la_LIBADD = 

if WITH_METACITY
adwmmod_LTLIBRARIES += adwm-metacity.la
endif

adwm_openbox_la_SOURCES = adwm_openbox.c
adwm_openbox_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_openbox_la_LIBADD = $(X11_LIBS)

if WITH_MUTTER
adwmmod_LTLIBRARIES += adwm-mutter.la
endif

adwm_pekwm_la_SOURCES = adwm_pekwm.c
adwm_pekwm_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_pekwm_la_LIBADD = 

if WITH_PEKWM
adwmmod_LTLIBRARIES += adwm-pekwm.la
endif

adwm_waimea_la_SOURCES = adwm_waimea.c
adwm_waimea_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_waimea_la_LIBADD = 

if WITH_WAIMEA
adwmmod_LTLIBRARIES += adwm-waimea.la
endif

adwm_wmaker_la_SOURCES = adwm_wmaker.c
adwm_wmaker_la_LDFLAGS = -module -export-dynamic -shared -rpath $(adwmmoddir) -avoid-version
adwm_wmaker_la_LIBADD = 

if WITH_WMAKER
adwmmod_LTLIBRARIES += adwm-wmaker.la
endif

EXTRA_adwm_DEPENDENCIES = $(adwmmod_LTLIBRARIES)

