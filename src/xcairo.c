/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "actions.h"
#include "config.h"
#include "icons.h"
#include "draw.h"
#include "image.h"
#if 1
#include "ximage.h"
#else
#include "xlib.h"
#endif
#include "xcairo.h" /* verification */

#ifdef XCAIRO

Bool
xcairo_initxbmdata(const unsigned char *bits, int w, int h, AdwmPixmap *px)
{
	Pixmap draw;
	Screen *screen;
	XRenderPictFormat *format;
	cairo_surface_t *surface = NULL;

	if (!(draw = XCreateBitmapFromData(dpy, scr->drawable, (char *) bits, w, h))) {
		EPRINTF("could not load xbm data\n");
		goto error;
	}
	format = XRenderFindStandardFormat(dpy, PictStandardA1);
	screen = ScreenOfDisplay(dpy, scr->screen);
#if 1
	surface = cairo_xlib_surface_create_with_xrender_format(dpy, draw, screen, format, w, h);
#else
	surface = cairo_xlib_surface_create_for_bitmap(dpy, draw, screen, w, h);
#endif
	if (!surface) {
		EPRINTF("could not create surface\n");
		goto error;
	}
	if (px->bitmap.surface) {
		cairo_surface_destroy(px->bitmap.surface);
		px->bitmap.surface = NULL;
	}
	if (px->file) {
		free(px->file);
		px->file = NULL;
	}
	px->bitmap.surface = surface;
	px->x = px->y = px->b = 0;
	px->w = px->w;
	px->h = px->h;
error:
	if (draw)
		XFreePixmap(dpy, draw);
	return (surface ? True : False);
}

#endif				/* XCAIRO */

