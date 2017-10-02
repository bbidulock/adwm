
nobase_dist_data_DATA = xsessions/adwm.desktop

dist_pkgdata_DATA = \
	adwmrc \
	close.xbm \
	fill.xbm \
	float.xbm \
	iconify.xbm \
	lhalf.xbm \
	max.xbm \
	menu.xbm \
	resize.xbm \
	rhalf.xbm \
	shade.xbm \
	stick.xbm

nobase_dist_pkgdata_DATA = \
	styles/Default/keysrc \
	styles/Default/stylerc \
	styles/Default/pixmaps/close_focused.xpm \
	styles/Default/pixmaps/close_hovered.xpm \
	styles/Default/pixmaps/close_pressed.xpm \
	styles/Default/pixmaps/close_unfocus.xpm \
	styles/Default/pixmaps/xkill_focused.xpm \
	styles/Default/pixmaps/xkill_hovered.xpm \
	styles/Default/pixmaps/xkill_pressed.xpm \
	styles/Default/pixmaps/xkill_unfocus.xpm \
	styles/Default/pixmaps/float_focused.xpm \
	styles/Default/pixmaps/float_hovered.xpm \
	styles/Default/pixmaps/float_pressed.xpm \
	styles/Default/pixmaps/float_unfocus.xpm \
	styles/Default/pixmaps/lhalf_focused.xpm \
	styles/Default/pixmaps/lhalf_hovered.xpm \
	styles/Default/pixmaps/lhalf_pressed.xpm \
	styles/Default/pixmaps/lhalf_unfocus.xpm \
	styles/Default/pixmaps/lback_focused.xpm \
	styles/Default/pixmaps/lback_hovered.xpm \
	styles/Default/pixmaps/lback_pressed.xpm \
	styles/Default/pixmaps/lback_unfocus.xpm \
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
	styles/Default/pixmaps/rhalf_focused.xpm \
	styles/Default/pixmaps/rhalf_hovered.xpm \
	styles/Default/pixmaps/rhalf_pressed.xpm \
	styles/Default/pixmaps/rhalf_unfocus.xpm \
	styles/Default/pixmaps/rback_focused.xpm \
	styles/Default/pixmaps/rback_hovered.xpm \
	styles/Default/pixmaps/rback_pressed.xpm \
	styles/Default/pixmaps/rback_unfocus.xpm \
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
	rm -f $(DESTDIR)$(pkgdatadir)/keysrc
	rm -f $(DESTDIR)$(pkgdatadir)/stylerc
	ln -sf styles/Default $(DESTDIR)$(pkgdatadir)/style
	ln -sf styles/Default/keysrc $(DESTDIR)$(pkgdatadir)/keysrc
	ln -sf styles/Default/stylerc $(DESTDIR)$(pkgdatadir)/stylerc

uninstall-hook:
	rm -f $(DESTDIR)$(pkgdatadir)/style
	rm -f $(DESTDIR)$(pkgdatadir)/keysrc
	rm -f $(DESTDIR)$(pkgdatadir)/stylerc

