/* See COPYING file for copyright and license details. */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <regex.h>
#include <wordexp.h>
#include <signal.h>
#include <math.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/XF86keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xfixes.h>
#ifdef RENDER
#include <X11/extensions/Xrender.h>
#include <X11/extensions/render.h>
#endif
#ifdef XCOMPOSITE
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/composite.h>
#endif
#ifdef DAMAGE
#include <X11/extensions/Xdamage.h>
#endif
#ifdef XRANDR
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/randr.h>
#endif
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#ifdef SYNC
#include <X11/extensions/sync.h>
#endif
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif
#ifdef STARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>
#endif
#include "adwm.h"
#include "draw.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "actions.h"
#include "config.h"
#include "image.h" /* verification */

#if 0
static const unsigned char dither4[4][4] = {
	{0, 4, 1, 5},
	{6, 2, 7, 3},
	{1, 5, 0, 4},
	{7, 3, 6, 2}
};

static const unsigned char dither8[8][8] = {
	{0, 32, 8, 40, 2, 34, 10, 42},
	{48, 16, 56, 24, 50, 18, 58, 26},
	{12, 44, 4, 36, 14, 46, 6, 38},
	{60, 28, 52, 20, 62, 30, 54, 22},
	{3, 35, 11, 43, 1, 33, 9, 41},
	{51, 19, 59, 27, 49, 17, 57, 25},
	{15, 47, 7, 39, 13, 45, 5, 37},
	{63, 31, 55, 23, 61, 29, 53, 21}
};
#endif

void
renderimage(AScreen *ds, const ARGB *argb, const unsigned width, const unsigned height)
{
	XImage *image;
	unsigned long r, g, b, pixel;
	unsigned char *p, *pp, *data;
	unsigned x, y;
	const ARGB *c;

	image = XCreateImage(dpy, ds->visual, ds->bpp, ZPixmap, 0, NULL, width, height, 32, 0);
	if (!image) {
		DPRINTF("Could not create image\n");
		return;
	}
	image->data = NULL;
	p = pp = data = calloc(image->bytes_per_line * (height + 1), sizeof(*data));

	switch (ds->visual->class) {
	case StaticColor:
	case PseudoColor:
		for (c = argb, y = 0; y < height; y++) {
			for (x = 0; x < width; x++, c++) {
				r = ds->rctab[c->red];
				g = ds->gctab[c->green];
				b = ds->bctab[c->blue];

				pixel = (r * ds->cpc * ds->cpc) + (g * ds->cpc) + b;
				*p++ = ds->colors[pixel].pixel;
			}
			p = (pp += image->bytes_per_line);
		}
		break;
	case TrueColor:
		for (c = argb, y = 0; y < height; y++) {
			for (x = 0; x < width; x++, c++) {
				r = ds->rctab[c->red];
				g = ds->gctab[c->green];
				b = ds->bctab[c->blue];

				pixel = (((r << ds->visual->bits_per_rgb) | g) << ds->visual->bits_per_rgb) | b;
				switch (ds->bpp + ((image->byte_order == MSBFirst) ? 1 : 0)) {
				case 8:	/* 8bpp */
					*p++ = pixel;
					break;
				case 16:	/* 16bpp LSB */
					*p++ = pixel;
					*p++ = pixel >> 8;
					break;
				case 17:	/* 16bpp MSB */
					*p++ = pixel >> 8;
					*p++ = pixel;
					break;
				case 24:	/* 24bpp LSB */
					*p++ = pixel;
					*p++ = pixel >> 8;
					*p++ = pixel >> 16;
					break;
				case 25:	/* 24bpp MSB */
					*p++ = pixel >> 16;
					*p++ = pixel >> 8;
					*p++ = pixel;
					break;
				case 32:	/* 32bpp LSB */
					*p++ = pixel;
					*p++ = pixel >> 8;
					*p++ = pixel >> 16;
					*p++ = pixel >> 24;
					break;
				case 33:	/* 32bpp MSB */
					*p++ = pixel >> 24;
					*p++ = pixel >> 16;
					*p++ = pixel >> 8;
					*p++ = pixel;
					break;
				}
			}
			p = (pp += image->bytes_per_line);
		}
		break;
	case StaticGray:
	case GrayScale:
		for (c = argb, y = 0; y < height; y++) {
			for (x = 0; x < width; x++, c++) {
				r = ds->rctab[c->red];
				g = ds->gctab[c->green];
				b = ds->bctab[c->blue];

				g = ((r * 30) + (g * 59) + (b * 11)) / 100;
				*p++ = ds->colors[g].pixel;
			}
			p = (pp += image->bytes_per_line);
		}
		break;
	default:
		DPRINTF("Unsupported visual class\n");
		free(data);
		XDestroyImage(image);
		return;
	}
	image->data = (char *) data;
}

void
initimage(void)
{
	int i, count, nvi;
	XPixmapFormatValues *pmv;
	XVisualInfo *xvi, templ = { .screen = scr->screen, .depth = 32, .class = TrueColor, };
	Visual *visual = NULL;

	if ((xvi = XGetVisualInfo(dpy, VisualScreenMask|VisualDepthMask|VisualClassMask, &templ, &nvi))) {
		XRenderPictFormat *format;

		for (i = 0; i < nvi; i++) {
			format = XRenderFindVisualFormat(dpy, xvi[i].visual);
			if (format->type == PictTypeDirect && format->direct.alphaMask) {
				visual = xvi[i].visual;
				break;
			}
		}
	}
	XFree(xvi);

	xtrap_push(False);
	if (visual) {
		XSetWindowAttributes wa = { 0, };
		unsigned long mask = 0;

		scr->depth = 32;
		scr->visual = visual;
		scr->colormap = XCreateColormap(dpy, scr->root, visual, AllocNone);

		/* make a window with proper depth for creating GCs */
		wa.override_redirect = True;
		mask |= CWOverrideRedirect;
		wa.colormap = scr->colormap;
		mask |= CWColormap;
		/* cannot inherit pixels but can set them to anything */
		wa.background_pixel = BlackPixel(dpy, scr->screen);
		mask |= CWBackPixel;
		wa.border_pixel = BlackPixel(dpy, scr->screen);
		mask |= CWBorderPixel;
		wa.background_pixmap = None;
		mask |= CWBackPixmap;
		scr->drawable = XCreateWindow(dpy, scr->root, 0, 0, 1, 1, 0,
				scr->depth, InputOutput, scr->visual,
				mask, &wa);
	} else {
		scr->depth = DefaultDepth(dpy, scr->screen);
		scr->visual = DefaultVisual(dpy, scr->screen);
		scr->colormap = DefaultColormap(dpy, scr->screen);
		scr->drawable = scr->root;
	}
	xtrap_pop();
#ifdef PIXBUF
	gdk_pixbuf_xlib_init_with_depth(dpy, scr->screen, scr->depth);
#endif
#ifdef IMLIB2
	scr->context = imlib_context_new();
	imlib_context_push(scr->context);
	imlib_context_set_display(dpy);
	imlib_context_set_drawable(None);
	imlib_context_set_visual(scr->visual);
	imlib_context_set_colormap(scr->colormap);
	imlib_context_set_drawable(scr->drawable);
	imlib_context_set_dither_mask(1);
	imlib_context_set_anti_alias(1);
	imlib_context_set_dither(1);
	imlib_context_set_blend(1);
	imlib_context_set_mask(None);
	imlib_context_pop();

#if 0
	scr->rootctx = imlib_context_new();
	imlib_context_push(scr->rootctx);
	imlib_context_set_display(dpy);
	imlib_context_set_drawable(None);
	imlib_context_set_visual(DefaultVisual(dpy, scr->screen));
	imlib_context_set_colormap(DefaultColormap(dpy, scr->screen));
	imlib_context_set_drawable(scr->root);
	imlib_context_set_dither_mask(1);
	imlib_context_set_anti_alias(1);
	imlib_context_set_dither(1);
	imlib_context_set_blend(1);
	imlib_context_set_mask(None);
	imlib_context_pop();
#endif
#endif
	scr->dither = True;
	scr->bpp = 0;
#if 0
	scr->cpc = options.colorsPerChannel;
#else
	scr->cpc = (1<<16);
#endif

	free(scr->rctab);
	free(scr->gctab);
	free(scr->bctab);

	scr->rctab = ecalloc(256, sizeof(*scr->rctab));
	scr->gctab = ecalloc(256, sizeof(*scr->gctab));
	scr->bctab = ecalloc(256, sizeof(*scr->bctab));

	if ((pmv = XListPixmapFormats(dpy, &count))) {
		for (i = 0; i < count; i++) {
			if (pmv[i].depth == scr->depth) {
				scr->bpp = pmv[i].bits_per_pixel;
				break;
			}
		}
		XFree(pmv);
	}
	if (scr->bpp == 0)
		scr->bpp = scr->depth;
	if (scr->bpp >= 24)
		scr->dither = False;

	switch (scr->visual->class) {
	case TrueColor:{
		unsigned long rmask = scr->visual->red_mask;
		unsigned long gmask = scr->visual->green_mask;
		unsigned long bmask = scr->visual->blue_mask;

		int roff = 0, rbits;
		int goff = 0, gbits;
		int boff = 0, bbits;

		for (; !(rmask & 0x1); rmask >>= 1, roff++) ;
		for (; !(gmask & 0x1); gmask >>= 1, goff++) ;
		for (; !(bmask & 0x1); bmask >>= 1, boff++) ;

		rbits = 255 / rmask;
		gbits = 255 / gmask;
		bbits = 255 / bmask;

		for (i = 0; i < 256; i++) {
			scr->rctab[i] = i / rbits;
			scr->gctab[i] = i / gbits;
			scr->bctab[i] = i / bbits;
		}

		break;
	}
	case PseudoColor:
	case StaticColor:{
		XColor icolors[256];
		int incolors;
		int rbits;
		int gbits;
		int bbits;
		int i, ii, p, r, g, b, bits;

		scr->ncolors = scr->cpc * scr->cpc * scr->cpc;
		if (scr->ncolors > (1 << scr->depth)) {
			scr->cpc = (1 << scr->depth) / 3;
			scr->ncolors = scr->cpc * scr->cpc * scr->cpc;
		}
		if (scr->cpc < 2 || scr->ncolors > (1 << scr->depth)) {
			DPRINTF("invalid color map size\n");
			scr->cpc = (1 << scr->depth) / 3;
		}
		scr->colors = calloc(scr->ncolors, sizeof(*scr->colors));
#ifdef ORDEREDPSEUDO
		bits = 256 / scr->cpc;
#else				/* ORDEREDPSEUDO */
		bits = 256 / (scr->cpc - 1);
#endif				/* ORDEREDPSEUDO */
		rbits = gbits = bbits = bits;

		for (i = 0; i < 256; i++) {
			scr->rctab[i] = i / rbits;
			scr->gctab[i] = i / gbits;
			scr->bctab[i] = i / bbits;
		}
		for (r = 0, i = 0; r < scr->cpc; r++) {
			for (g = 0; g < scr->cpc; g++) {
				for (b = 0; b < scr->cpc; b++) {
					scr->colors[i].red = (r * 0xffff) / (scr->cpc - 1);
					scr->colors[i].green = (g * 0xffff) / (scr->cpc - 1);
					scr->colors[i].blue = (b * 0xffff) / (scr->cpc - 1);
					scr->colors[i].flags = DoRed | DoGreen | DoBlue;
				}
			}
		}
		XGrabServer(dpy);
		{
			for (i = 0; i < scr->ncolors; i++) {
				if (!XAllocColor(dpy, scr->colormap, &scr->colors[i])) {
					DPRINTF("could not allocate color\n");
					scr->colors[i].flags = 0;
				} else
					scr->colors[i].flags = DoRed | DoGreen | DoBlue;
			}
		}
		XUngrabServer(dpy);

		incolors = ((1 << scr->depth) > 256) ? 256 : (1 << scr->depth);

		for (i = 0; i < incolors; i++)
			icolors[i].pixel = i;

		XQueryColors(dpy, scr->colormap, icolors, incolors);
		for (i = 0; i < scr->ncolors; i++) {
			if (!scr->colors[i].flags) {
				unsigned long chk = 0xffffffff, pixel, close = 0;

				p = 2;
				while (p--) {
					for (ii = 0; ii < incolors; i++) {
						r = (scr->colors[i].red - icolors[i].red) >> 8;
						g = (scr->colors[i].green - icolors[i].green) >> 8;
						b = (scr->colors[i].blue - icolors[i].blue) >> 8;
						pixel = (r * r) + (g * g) + (b * b);

						if (pixel < chk) {
							chk = pixel;
							close = ii;
						}

						scr->colors[i].red = icolors[close].red;
						scr->colors[i].green = icolors[close].green;
						scr->colors[i].blue = icolors[close].blue;

						if (XAllocColor(dpy, scr->colormap, &scr->colors[i])) {
							scr->colors[i].flags = DoRed | DoGreen | DoBlue;
							break;
						}
					}
				}
			}
		}
		break;
	}
	case GrayScale:
	case StaticGray:{
		int i, ii, p, bits;
		XColor icolors[256];
		int incolors;

		int rbits;
		int gbits;
		int bbits;

		if (scr->visual->class == StaticGray) {
			scr->ncolors = 1 << scr->depth;
		} else {
			scr->ncolors = scr->cpc * scr->cpc * scr->cpc;

			if (scr->ncolors > (1 << scr->depth)) {
				scr->cpc = (1 << scr->depth) / 3;
				scr->ncolors = scr->cpc * scr->cpc * scr->cpc;
			}
		}
		if (scr->cpc < 2 || scr->ncolors > (1 << scr->depth)) {
			DPRINTF("invalid colormap size\n");
			scr->cpc = (1 << scr->depth) / 3;
		}
		scr->colors = ecalloc(scr->ncolors, sizeof(*scr->colors));
		bits = 255 / (scr->cpc - 1);

		rbits = gbits = bbits = bits;

		for (i = 0; i < 256; i++) {
			scr->rctab[i] = i / rbits;
			scr->gctab[i] = i / gbits;
			scr->bctab[i] = i / bbits;
		}

		XGrabServer(dpy);
		{
			for (i = 0; i < scr->ncolors; i++) {
				scr->colors[i].red = (i * 0xffff) / (scr->cpc - 1);
				scr->colors[i].green = (i * 0xffff) / (scr->cpc - 1);
				scr->colors[i].blue = (i * 0xffff) / (scr->cpc - 1);
				scr->colors[i].flags = DoRed | DoGreen | DoBlue;
			}
			if (!XAllocColor(dpy, scr->colormap, &scr->colors[i])) {
				DPRINTF("could not allocate color\n");
				scr->colors[i].flags = 0;
			} else
				scr->colors[i].flags = DoRed | DoGreen | DoBlue;
		}
		XUngrabServer(dpy);
		incolors = ((1 << scr->depth) > 256) ? 256 : (1 << scr->depth);

		for (i = 0; i < incolors; i++)
			icolors[i].pixel = i;

		XQueryColors(dpy, scr->colormap, icolors, incolors);
		for (i = 0; i < scr->ncolors; i++) {
			if (!scr->colors[i].flags) {
				unsigned long chk = 0xffffffff, pixel, close = 0;

				p = 2;
				while (p--) {
					for (ii = 0; ii < incolors; ii++) {
						int r = (scr->colors[i].red - icolors[i].red) >> 8;
						int g = (scr->colors[i].green - icolors[i].green) >> 8;
						int b = (scr->colors[i].blue - icolors[i].blue) >> 8;

						pixel = (r * r) + (g * g) + (b * b);
						if (pixel < chk) {
							chk = pixel;
							close = ii;
						}
						scr->colors[i].red = icolors[close].red;
						scr->colors[i].green = icolors[close].green;
						scr->colors[i].blue = icolors[close].blue;

						if (XAllocColor(dpy, scr->colormap, &scr->colors[i])) {
							scr->colors[i].flags = DoRed | DoGreen | DoBlue;
							break;
						}
					}
				}
			}
		}
		break;
	}
	default:
		eprint("Unsuppoted visual class\n");
		break;
	}
}

