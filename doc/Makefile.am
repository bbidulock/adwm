
adwm.man: adwm.1
	sed 's,[@]PACKAGE[@],$(PACKAGE),g;s,[@]VERSION[@],$(VERSION),g;s,[@]DATE[@],$(DATE),g;s,[@]BRANCH[@],$(BRANCH),g' $< >$@

adwm-keys.man: adwm-keys.1
	sed 's,[@]PACKAGE[@],$(PACKAGE),g;s,[@]VERSION[@],$(VERSION),g;s,[@]DATE[@],$(DATE),g;s,[@]BRANCH[@],$(BRANCH),g' $< >$@

adwm-style.man: adwm-style.1
	sed 's,[@]PACKAGE[@],$(PACKAGE),g;s,[@]VERSION[@],$(VERSION),g;s,[@]DATE[@],$(DATE),g;s,[@]BRANCH[@],$(BRANCH),g' $< >$@

adwm-config.man: adwm-config.1
	sed 's,[@]PACKAGE[@],$(PACKAGE),g;s,[@]VERSION[@],$(VERSION),g;s,[@]DATE[@],$(DATE),g;s,[@]BRANCH[@],$(BRANCH),g' $< >$@

nodist_man1_MANS = adwm.man adwm-keys.man adwm-style.man adwm-config.man

SUFFIXES = .man .pod

adwm.1.man: MANNAME = ADWM
adwm.1.man: adwm.1.pod

adwm-keys.1.man: MANNAME = ADWM-KEYS
adwm-keys.1.man: adwm-keys.1.pod

.pod.man:
	pod2man --center="User Commands" \
		--date="$(DATE)" \
		--errors="none" \
		--fixed="CW" \
		--fixedbold="CB" \
		--fixeditalic="CI" \
		--fixedbolditalic="CX" \
		--name="$(MANNAME)" \
		--nourls \
		--release="$(PACKAGE) $(VERSION)" \
		--section="1" \
		$< >$@

pods: adwm.1.man adwm-keys.1.man

CLEANFILES = adwm.man adwm-keys.man adwm-style.man adwm-config.man adwm.1.man adwm-keys.1.man

EXTRA_DIST = adwm.1 adwm-keys.1 adwm-style.1 adwm-config.1 adwm.1.pod adwm-keys.1.pod

.PHONY: pods
