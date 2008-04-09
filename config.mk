# echinus wm version
VERSION = 0.2.5

# Customize below to fit your system

# paths
PREFIX = ${HOME}
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# includes and libs
INCS = -I. -I/usr/include -I${X11INC} `pkg-config --cflags xft`
LIBS = -L/usr/lib -lc -L${X11LIB} -lX11 `pkg-config --libs xft`

# flags
CFLAGS = -Os ${INCS} -DVERSION=\"${VERSION}\"
LDFLAGS = -s ${LIBS}
CFLAGS = -g3 -ggdb3 -std=c99 -pedantic -Wall -O0 ${INCS} -DVERSION=\"${VERSION}\" 
LDFLAGS = -g3 -ggdb3 ${LIBS}

# Solaris
#CFLAGS = -fast ${INCS} -DVERSION=\"${VERSION}\"
#LDFLAGS = ${LIBS}
#CFLAGS += -xtarget=ultra

# compiler and linker
CC = cc
