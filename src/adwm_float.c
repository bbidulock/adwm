/* See COPYING file for copyright and license details. */
#include <unistd.h>
#include <errno.h>
#include <regex.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xft/Xft.h>
#include "adwm.h"

Layout adwm_layouts[] = {
	/* *INDENT-OFF* */
	/* function	symbol	features				major		minor		placement	 */
	{ NULL,		'i',	OVERLAP,				0,		0,		ColSmartPlacement },
	{ NULL,		'f',	OVERLAP,				0,		0,		ColSmartPlacement },
	{ NULL,		'\0',	0,					0,		0,		0		  }
	/* *INDENT-ON* */
};

/*
 * This is a fully free floating layout.  Not to be confused with the floating
 * versions of the other layouts.
 */

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
