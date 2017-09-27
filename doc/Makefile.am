
adwm_pod_files = \
	adwm.pod \
	adwm-config.pod \
	adwm-keys.pod \
	adwm-style.pod

adwm_man1_files = \
	adwm.man

adwm_man5_files = \
	adwm-keys.man \
	adwm-style.man \
	adwm-config.man

dist_man1_MANS = $(adwm_man1_files)
dist_man5_MANS = $(adwm_man5_files)

SUFFIXES = .man .pod

adwm.man: MANTITLE = User Commands
adwm.man: MANSECTION = 1
adwm.man: adwm.pod

adwm-config.man: MANTITLE = File Formats
adwm-config.man: MANSECTION = 5
adwm-config.man: adwm-config.pod

adwm-keys.man: MANTITLE = File Formats
adwm-keys.man: MANSECTION = 5
adwm-keys.man: adwm-keys.pod

adwm-style.man: MANTITLE = File Formats
adwm-style.man: MANSECTION = 5
adwm-style.man: adwm-style.pod


.pod.man:
	$(AM_V_GEN)pod2man \
		--center="$(MANTITLE)" \
		--date="$(DATE)" \
		--errors="none" \
		--fixed="CW" \
		--fixedbold="CB" \
		--fixeditalic="CI" \
		--fixedbolditalic="CX" \
		--nourls \
		--release="$(PACKAGE) $(VERSION)" \
		--section="$(MANSECTION)" \
		$< \
		|sed 's,[@]PACKAGE[@],$(PACKAGE),g;s,[@]VERSION[@],$(VERSION),g;s,[@]DATE[@],$(DATE),g;s,[@]BRANCH[@],$(BRANCH),g' \
		>$@

CLEANFILES = $(adwm_man1_files) $(adwm_man5_files)

DISTCLEANFILES = $(adwm_man1_files) $(adwm_man5_files)

EXTRA_DIST = $(adwm_pod_files)

