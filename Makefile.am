
SUBDIRS = tests

AM_CPPFLAGS = -include autoconf.h -DSYSCONFPATH=\"$(pkgconfdir)\"
AM_CFLAGS = $(X11_CFLAGS) $(XFT_CFLAGS) $(XRANDR_CFLAGS) $(XSYNC_CFLAGS) $(XINERAMA_CFLAGS) $(SN_CFLAGS) $(XPM_CFLAGS) $(IMLIB2_CFLAGS)

bin_PROGRAMS = adwm

adwm_SOURCES = draw.c adwm.c ewmh.c parse.c config.h adwm.h
adwm_LDADD = $(IMLIB2_LIBS) $(XPM_LIBS) $(SN_LIBS) $(XINERAMA_LIBS) $(XSYNC_LIBS) $(XRANDR_LIBS) $(XFT_LIBS) $(X11_LIBS) -lm

adwm.man: adwm.1
	/bin/cp -f $< $@

nobase_dist_data_DATA = xsessions/adwm.desktop

nodist_man1_MANS = adwm.man

dist_pkgdata_DATA = close.xbm adwmrc iconify.xbm max.xbm

nobase_dist_pkgdata_DATA = \
	styles/Default/keysrc \
	styles/Default/stylerc \
	styles/Default/pixmaps/close_focused.xpm \
	styles/Default/pixmaps/close_hovered.xpm \
	styles/Default/pixmaps/close_pressed.xpm \
	styles/Default/pixmaps/close_unfocus.xpm \
	styles/Default/pixmaps/float_focused.xpm \
	styles/Default/pixmaps/float_hovered.xpm \
	styles/Default/pixmaps/float_pressed.xpm \
	styles/Default/pixmaps/float_unfocus.xpm \
	styles/Default/pixmaps/max_focused.xpm \
	styles/Default/pixmaps/max_hovered.xpm \
	styles/Default/pixmaps/max_pressed.xpm \
	styles/Default/pixmaps/max_unfocus.xpm \
	styles/Default/pixmaps/menu_focused.xpm \
	styles/Default/pixmaps/menu_hovered.xpm \
	styles/Default/pixmaps/menu_pressed.xpm \
	styles/Default/pixmaps/menu_unfocus.xpm \
	styles/Default/pixmaps/min_focused.xpm \
	styles/Default/pixmaps/min_hovered.xpm \
	styles/Default/pixmaps/min_pressed.xpm \
	styles/Default/pixmaps/min_unfocus.xpm \
	styles/Default/pixmaps/shade_focused.xpm \
	styles/Default/pixmaps/shade_hovered.xpm \
	styles/Default/pixmaps/shade_pressed.xpm \
	styles/Default/pixmaps/shade_unfocus.xpm \
	styles/Default/pixmaps/stick_focused.xpm \
	styles/Default/pixmaps/stick_hovered.xpm \
	styles/Default/pixmaps/stick_pressed.xpm \
	styles/Default/pixmaps/stick_unfocus.xpm \
	styles/Default/pixmaps/stuck_focused.xpm \
	styles/Default/pixmaps/stuck_hovered.xpm \
	styles/Default/pixmaps/stuck_pressed.xpm \
	styles/Default/pixmaps/stuck_unfocus.xpm \
	styles/Default/pixmaps/tile_focused.xpm \
	styles/Default/pixmaps/tile_hovered.xpm \
	styles/Default/pixmaps/tile_pressed.xpm \
	styles/Default/pixmaps/tile_unfocus.xpm \
	styles/Default/pixmaps/unmax_focused.xpm \
	styles/Default/pixmaps/unmax_hovered.xpm \
	styles/Default/pixmaps/unmax_pressed.xpm \
	styles/Default/pixmaps/unmax_unfocus.xpm \
	styles/Default/pixmaps/unshade_focused.xpm \
	styles/Default/pixmaps/unshade_hovered.xpm \
	styles/Default/pixmaps/unshade_pressed.xpm \
	styles/Default/pixmaps/unshade_unfocus.xpm

install-data-hook:
	mkdir -p $(DESTDIR)$(pkgdatadir)
	rm -f $(DESTDIR)$(pkgdatadir)/style
	rm -f $(DESTDIR)$(pkgdatadir)/keys
	ln -sf styles/Default $(DESTDIR)$(pkgdatadir)/keys
	ln -sf styles/Default $(DESTDIR)$(pkgdatadir)/style

uninstall-hook:
	rm -f $(DESTDIR)$(pkgdatadir)/style
	rm -f $(DESTDIR)$(pkgdatadir)/keys

SUFFIXES = .man .pod

adwm.1.man: adwm.1.pod

.pod.man:
	pod2man --center="User Commands" \
		--date="$(DATE)" \
		--errors="none" \
		--fixed="CW" \
		--fixedbold="CB" \
		--fixeditalic="CI" \
		--fixedbolditalic="CX" \
		--nourls \
		--release="$(PACKAGE) $(VERSION)" \
		--section="1" \
		$< >$@

EXTRA_DIST = LICENSE adwm.1 README README.echinus TODO ChangeLog

CLEANFILES = adwm.man

