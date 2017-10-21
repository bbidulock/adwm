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

static void
monocle(Monitor *m)
{
	Client *c;
	Workarea w;
	View *v;

	getworkarea(m, &w);
	v = &scr->views[m->curtag];
	for (c = nexttiled(scr->clients, m); c; c = nexttiled(c->next, m)) {
		ClientGeometry g;

		memcpy(&g, &w, sizeof(w));
		g.t = (v->dectiled && c->has.title) ? scr->style.titleheight : 0;
		g.g = (v->dectiled && c->has.grips) ? scr->style.gripsheight : 0;
		g.b = (g.t || g.g) ? scr->style.border : 0;
		g.w -= 2 * g.b;
		g.h -= 2 * g.b;

		DPRINTF("CALLING reconfigure()\n");
		reconfigure(c, &g);
	}
}

Layout adwm_layouts[] = {
	/* *INDENT-OFF* */
	/* function	symbol	features				major		minor		placement	 */
	{ &monocle,	'm',	0,					0,		0,		ColSmartPlacement },
	{ NULL,		'\0',	0,					0,		0,		0		  }
	/* *INDENT-ON* */
};

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
