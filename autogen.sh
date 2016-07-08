#!/bin/bash

PACKAGE='adwm'
VERSION=
DATE="`date -uI`"

if [ -z "$VERSION" ]; then
	VERSION='0.1'
	if [ -x "`which git 2>/dev/null`" -a -d .git ]; then
		VERSION=$(git describe --tags|sed 's,[-_],.,g;s,\.g.*$,,')
		DATE=$(git show -s --format=%ci HEAD^{commit}|awk '{print$1}')
		(
		   echo -e "# created with git log --stat=75 -M -C | fmt -sct -w80\n"
		   git log --stat=76 -M -C| fmt -sct -w80
		)>ChangeLog
		(
		   echo "@PACKAGE@ -- authors file.  @DATE@"
		   echo ""
		   git log|grep '^Author:'|awk '{if(!authors[$0]){print$0;authors[$0]=1;}}'
		)>AUTHORS.in
		(
		   echo "@PACKAGE@ -- history of user-visible changes.  @DATE@"
		   PACKAGE="$PACKAGE" ./gennews.sh
		)>NEWS.in
	fi
fi

sed -r -e "s:AC_INIT\([[]$PACKAGE[]],[[][^]]*[]]:AC_INIT([$PACKAGE],[$VERSION]:
	   s:AC_REVISION\([[][^]]*[]]\):AC_REVISION([$VERSION]):
	   s:^DATE=.*$:DATE='$DATE':" \
	  configure.template >configure.ac

subst="s:@PACKAGE@:$PACKAGE:g
       s:@VERSION@:$VERSION:g
       s:@DATE@:$DATE:g"

sed -r -e "$subst" README.in >README
sed -r -e "$subst" NEWS.in >NEWS
sed -r -e "$subst" AUTHORS.in >AUTHORS

mkdir m4 2>/dev/null

#autoreconf -fiv
