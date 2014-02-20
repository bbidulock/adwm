
adwm.man: adwm.1
	sed 's,[@]PACKAGE[@],$(PACKAGE),g;s,[@]VERSION[@],$(VERSION),g' $< >$@

adwm-keys.man: adwm-keys.1
	sed 's,[@]PACKAGE[@],$(PACKAGE),g;s,[@]VERSION[@],$(VERSION),g' $< >$@

nodist_man1_MANS = adwm.man adwm-keys.man

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

CLEANFILES = adwm.man adwm-keys.man adwm.1.man

EXTRA_DIST = adwm.1 adwm-keys.1 adwm.1.pod
