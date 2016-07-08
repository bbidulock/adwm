
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src doc data

EXTRA_DIST = INSTALL COPYING AUTHORS AUTHORS.in NEWS NEWS.in LICENSE README README.in README.echinus THANKS TODO ChangeLog

ChangeLog:
	( \
	  echo -e "# created with git log --stat=76 -M -C|fmt -sct -w80\n" ; \
	  git log --stat=76 -M -C|fmt -sct -w80 \
	)>$@

AUTHORS.in:
	( \
	  echo "\@PACKAGE\@ -- authors file. \@DATE\@" ; \
	  echo "" ; \
	  git log|grep '^Author:'|awk '{if(!authors[$$0]){print$$0;authors[$$0]=1;}}' \
	)>$@

NEWS.in:
	( \
	  echo "\@PACKAGE\@ -- history of user-visible changes.  \@DATE\@" ; \
	  PACKAGE="@PACKAGE@" $(srcdir)/gennews.sh \
	)>$@

README: README.in
	sed -r -e 's:\@PACKAGE\@:$(PACKAGE):g;s:\@VERSION\@:$(VERSION):g;s:\@DATE\@:$(DATE):g' $< >$@

NEWS: NEWS.in
	sed -r -e 's:\@PACKAGE\@:$(PACKAGE):g;s:\@VERSION\@:$(VERSION):g;s:\@DATE\@:$(DATE):g' $< >$@

AUTHORS: AUTHORS.in
	sed -r -e 's:\@PACKAGE\@:$(PACKAGE):g;s:\@VERSION\@:$(VERSION):g;s:\@DATE\@:$(DATE):g' $< >$@

dist_noinst_SCRIPTS = gennews.sh autogen.sh configure.sh rebuild.sh
