#!/bin/bash

# need --enable-maintainer-mode to be able to run in place
#      must be disabled to build an installable package

# *FLAGS are what Arch Linux makepkg uses with the exception
#      that -Wall -Werror is added

case "`uname -m`" in
	i686)
		CPPFLAGS=""
		CFLAGS="-march=i686 -mtune=generic -O2 -pipe -fno-plt -fexceptions -Wp,-D_FORTIFY_SOURCE=2 -Wformat -Werror=format-security -fstack-clash-protection -fcf-protection"
		CXXFLAGS="$CFLAGS -Wp,-D_GLIBCXX_ASSERTIONS"
		LDFLAGS="-Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now"
		DEBUG_CFLAGS="-g -ggdb -fvar-tracking-assignments"
		DEBUG_CXXFLAGS="-g -ggdb -fvar-tracking-assignments"
	;;
	x86_64)
		CPPFLAGS=""
		CFLAGS="-march=x86-64 -mtune=generic -O2 -pipe -fno-plt -fexceptions -Wp,-D_FORTIFY_SOURCE=2 -Wformat -Werror=format-security -fstack-clash-protection -fcf-protection"
		CXXFLAGS="$CFLAGS -Wp,-D_GLIBCXX_ASSERTIONS"
		LDFLAGS="-Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now"
		DEBUG_CFLAGS="-g -ggdb -fvar-tracking-assignments"
		DEBUG_CXXFLAGS="-g -ggdb -fvar-tracking-assignments"
	;;
esac

./configure \
	--enable-maintainer-mode \
	--enable-dependency-tracking \
	--disable-imlib2 \
	--disable-gdk-pixbuf \
	CPPFLAGS="$CPPFLAGS" \
	CFLAGS="$DEBUG_CFLAGS -Wall -Wextra -Werror -Wno-clobbered $CFLAGS" \
	CXXFLAGS="$DEBUG_CXXFLAGS -Wall -Wextra -Werror -Wno-clobbered $CXXFLAGS" \
	LDFLAGS="$LDFLAGS" \
	DEBUG_CFLAGS="$DEBUG_CFLAGS" \
	DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS"

# cscope target won't work without this
#
[ -f po/Makefile ] && echo -e '\n%:\n\t@:\n\n' >> po/Makefile
