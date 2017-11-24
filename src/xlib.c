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
#include "xlib.h" /* verification */

void
xlib_removepixmap(AdwmPixmap *p)
{
	if (p->pixmap.draw) {
		XFreePixmap(dpy, p->pixmap.draw);
		p->pixmap.draw = None;
	}
	if (p->pixmap.mask) {
		XFreePixmap(dpy, p->pixmap.mask);
		p->pixmap.mask = None;
	}
	if (p->bitmap.draw) {
		XFreePixmap(dpy, p->bitmap.draw);
		p->bitmap.draw = None;
	}
	if (p->bitmap.mask) {
		XFreePixmap(dpy, p->bitmap.mask);
		p->bitmap.mask = None;
	}
}

Bool
xlib_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h)
{
	XImage *xicon = NULL, *xmask = NULL, *ximage = NULL;
	ButtonImage **bis;
	unsigned i, j, k, l, d = ds->depth, th = ds->style.titleheight;
	unsigned long pixel, fg, bg;
	double *chanls = NULL; unsigned m; /* we all float down here... */
	double *counts = NULL; unsigned n;
	double *colors = NULL;

	/* for now */
	fg = ds->style.color.norm[ColFG].pixel;
	bg = ds->style.color.norm[ColBG].pixel;

	if (ds->style.outline)
		th--;

	if (!(xicon = XGetImage(dpy, icon, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", icon, w, h);
		goto error;
	}
	if (mask && !(xmask = XGetImage(dpy, mask, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", mask, w, h);
		goto error;
	}
	if (xmask) {
		unsigned xmin, xmax, ymin, ymax;
		XRectangle box;
		XImage *newicon, *newmask;

		xmin = w; xmax = 0;
		ymin = h; ymax = 0;
		/* chromium and ROXTerm are placing too much space around icons */
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				if (XGetPixel(xmask, i, j)) { /* not fully transparent */
					if (i < xmin) xmin = i;
					if (j < ymin) ymin = j;
					if (i > xmax) xmax = i;
					if (j > ymax) ymax = j;
				}
			}
		}
		box.x = xmin;
		box.y = ymin;
		box.width = xmax + 1 - xmin;
		box.height = ymax + 1 - ymin;

		if (box.x || box.y || box.width < w || box.height < h) {
			XPRINTF(__CFMTS(c) "clipping to bounding box of %hux%hu+%hd+%hd from %ux%u+0+0\n",
					__CARGS(c), box.width, box.height, box.x, box.y, w, h);
			if (!(newicon = XSubImage(xicon, box.x, box.y, box.width, box.height))) {
				EPRINTF("could not allocate subimage %hux%hu+%hd+%d from %ux%u+0+0\n",
					box.width, box.height, box.x, box.y, w, h);
				goto error;
			}
			if (!(newmask = XSubImage(xmask, box.x, box.y, box.width, box.height))) {
				EPRINTF("could not allocate subimage %hux%hu+%hd+%d from %ux%u+0+0\n",
					box.width, box.height, box.x, box.y, w, h);
				XDestroyImage(newicon);
				goto error;
			}
			XDestroyImage(xicon); xicon = newicon;
			XDestroyImage(xmask); xmask = newmask;
			w = box.width;
			h = box.height;
		}
	}
	if (h > th) {
		double scale = (double) th / (double) h;

		unsigned sh = lround(h * scale);
		unsigned sw = lround(w * scale);

		XPRINTF(__CFMTS(c) "scaling bitmap 0x%lx from %ux%u to %ux%u\n", __CARGS(c), icon, w, h, sw, sh);

		if (!(ximage = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, sw, sh, 8, 0))) {
			EPRINTF("could not create image %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(ximage->data = emallocz(ximage->bytes_per_line * ximage->height))) {
			EPRINTF("could not allocate image data %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(chanls = ecalloc(ximage->bytes_per_line * ximage->height, 4 * sizeof(*chanls)))) {
			EPRINTF("could not allocate chanls %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(counts = ecalloc(ximage->bytes_per_line * ximage->height, sizeof(*counts)))) {
			EPRINTF("could not allocate counts %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(colors = ecalloc(ximage->bytes_per_line * ximage->height, sizeof(*colors)))) {
			EPRINTF("could not allocate colors %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		double pppx = (double) w / (double) sw;
		double pppy = (double) h / (double) sh;

		double lx, rx, ty, by, xf, yf, ff;

		for (ty = 0.0, by = pppy, l = 0; l < sh; l++, ty += pppy, by += pppy) {

			for (j = floor(ty); j < by; j++) {

				if (j < ty && j + 1 > ty) yf = ty - j;
				else if (j < by && j + 1 > by) yf = by - j;
				else yf = 1.0;

				for (lx = 0.0, rx = pppx, k = 0; k < sw; k++, lx += pppx, rx += pppx) {

					for (i = floor(lx); i < rx; i++) {

						if (i < lx && i + 1 > lx) xf = lx - i;
						else if (i < rx && i + 1 > rx) xf = rx - i;
						else xf = 1.0;

						ff = xf * yf;
						m = l * sw + k;
						n = m << 2;
						counts[m] += ff;
						if (!xmask || XGetPixel(xmask, i, j)) {
							if (XGetPixel(xicon, i, j)) {
								colors[m] += ff;
								chanls[n+0] += 255 * ff;
								chanls[n+1] += 255 * ff;
								chanls[n+2] += 255 * ff;
								chanls[n+3] += 255 * ff;
							}
						}
					}
				}
			}
		}
		unsigned amax = 0;
		for (l = 0; l < sh; l++) {
			for (k = 0; k < sw; k++) {
				n = l * sw + k;
				m = n << 2;
				pixel = 0;
				if (counts[n]) {
					pixel |= (lround(chanls[m+0] / counts[n]) & 0xff) << 24;
				}
				if (colors[n]) {
					pixel |= (lround(chanls[m+1] / colors[n]) & 0xff) << 16;
					pixel |= (lround(chanls[m+2] / colors[n]) & 0xff) <<  8;
					pixel |= (lround(chanls[m+3] / colors[n]) & 0xff) <<  0;
				}
				XPutPixel(ximage, k, l, pixel);
				amax = max(amax, ((pixel >> 24) & 0xff));
			}
		}
		if (!amax) {
			/* no opacity at all! */
			for (l = 0; l < sh; l++) {
				for (k = 0; k < sw; k++) {
					XPutPixel(ximage, k, l, XGetPixel(ximage, k, l) | 0xff000000);
				}
			}
		} else if (amax < 255) {
			/* something has to be opaque, bump the ximage */
			double bump = (double) 255 / (double) amax;
			for (l = 0; l < sh; l++) {
				for (k = 0; k < sw; k++) {
					pixel = XGetPixel(ximage, k, l);
					amax = (pixel >> 24) & 0xff;
					amax = lround(amax * bump);
					if (amax > 255)
						amax = 255;
					pixel = (pixel & 0x00ffffff) | (amax << 24);
					XPutPixel(ximage, k, l, pixel);
				}
			}
		}
		w = sw;
		h = sh;
		free(chanls);
		chanls = NULL;
		free(counts);
		counts = NULL;
		free(colors);
		colors = NULL;
	} else {
		if (!(ximage = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, w, h, 8, 0))) {
			EPRINTF("could not create image %ux%ux%u\n", w, h, d);
			goto error;
		}
		if (!(ximage->data = emallocz(ximage->bytes_per_line * ximage->height))) {
			EPRINTF("could not allocate image data %ux%ux%u\n", w, h, d);
			goto error;
		}
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				if (!xmask || XGetPixel(xmask, i, j)) {
					if (XGetPixel(xicon, i, j)) {
						XPutPixel(ximage, i, j, fg | 0xFF000000);
					} else {
						XPutPixel(ximage, i, j, bg | 0xFF000000);
					}
				} else {
					XPutPixel(ximage, i, j, 0x00000000);
				}
			}
		}
	}
	XDestroyImage(xicon);
	if (xmask)
		XDestroyImage(xmask);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		ButtonImage *bi = *bis;
		AdwmPixmap *px = &bi->px;

		px->x = px->y = px->b = 0;
		px->d = ds->depth;
		px->w = w;
		px->h = h;
		if (px->pixmap.draw) {
			XFreePixmap(dpy, px->pixmap.draw);
			px->pixmap.draw = None;
		}
		if (px->pixmap.mask) {
			XFreePixmap(dpy, px->pixmap.mask);
			px->pixmap.mask = None;
		}
		px->pixmap.draw = XCreatePixmap(dpy, ds->drawable, w, h, ds->depth);
		XPRINTF(__CFMTS(c) "assigning pixmap 0x%lx\n", __CARGS(c), px->pixmap.draw);
		XPutImage(dpy, px->pixmap.draw, ds->dc.gc, ximage, 0, 0, 0, 0, w, h);
		bi->present = True;
	}
	XDestroyImage(ximage);
	return (True);

      error:
	free(chanls);
	free(counts);
	free(colors);
	if (ximage)
		XDestroyImage(ximage);
	if (xmask)
		XDestroyImage(xmask);
	if (xicon)
		XDestroyImage(xicon);
      return (False);
}

Bool
xlib_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h, unsigned d)
{
	XImage *xicon = NULL, *xmask = NULL, *ximage = NULL;
	ButtonImage **bis;
	unsigned i, j, k, l, th = ds->style.titleheight;
	unsigned long pixel, bg;
	double *chanls = NULL; unsigned m;
	double *counts = NULL; unsigned n;
	double *colors = NULL;

	/* for now */
	bg = ds->style.color.norm[ColBG].pixel;
	(void) bg;  /* XXX: this should be used to blend against background */

	if (ds->style.outline)
		th--;

	if (!(xicon = XGetImage(dpy, icon, 0, 0, w, h, AllPlanes, ZPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", icon, w, h);
		goto error;
	}
	if (mask && !(xmask = XGetImage(dpy, mask, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", mask, w, h);
		goto error;
	}
	if (xmask) {
		unsigned xmin, xmax, ymin, ymax;
		XRectangle box;
		XImage *newicon, *newmask;

		xmin = w; xmax = 0;
		ymin = h; ymax = 0;
		/* chromium and ROXTerm are placing too much space around icons */
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				if (XGetPixel(xmask, i, j)) { /* not fully transparent */
					if (i < xmin) xmin = i;
					if (j < ymin) ymin = j;
					if (i > xmax) xmax = i;
					if (j > ymax) ymax = j;
				}
			}
		}
		box.x = xmin;
		box.y = ymin;
		box.width = xmax + 1 - xmin;
		box.height = ymax + 1 - ymin;

		if (box.x || box.y || box.width < w || box.height < h) {
			XPRINTF(__CFMTS(c) "clipping to bounding box of %hux%hu+%hd+%hd from %ux%u+0+0\n",
					__CARGS(c), box.width, box.height, box.x, box.y, w, h);
			if (!(newicon = XSubImage(xicon, box.x, box.y, box.width, box.height))) {
				EPRINTF("could not allocate subimage %hux%hu+%hd+%d from %ux%u+0+0\n",
					box.width, box.height, box.x, box.y, w, h);
				goto error;
			}
			if (!(newmask = XSubImage(xmask, box.x, box.y, box.width, box.height))) {
				EPRINTF("could not allocate subimage %hux%hu+%hd+%d from %ux%u+0+0\n",
					box.width, box.height, box.x, box.y, w, h);
				XDestroyImage(newicon);
				goto error;
			}
			XDestroyImage(xicon); xicon = newicon;
			XDestroyImage(xmask); xmask = newmask;
			w = box.width;
			h = box.height;
		}
	}
	d = ds->depth;
	if (h > th) {
		double scale = (double) th / (double) h;

		unsigned sh = lround(h * scale);
		unsigned sw = lround(w * scale);

		XPRINTF(__CFMTS(c) "scaling pixmap 0x%lx from %ux%u to %ux%u\n", __CARGS(c), icon, w, h, sw, sh);

		if (!(ximage = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, sw, sh, 8, 0))) {
			EPRINTF("could not create image %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(ximage->data = emallocz(ximage->bytes_per_line * ximage->height))) {
			EPRINTF("could not allocate image data %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(chanls = ecalloc(ximage->bytes_per_line * ximage->height, 4 * sizeof(*chanls)))) {
			EPRINTF("could not allocate chanls %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(counts = ecalloc(ximage->bytes_per_line * ximage->height, sizeof(*counts)))) {
			EPRINTF("could not allocate counts %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(colors = ecalloc(ximage->bytes_per_line * ximage->height, sizeof(*colors)))) {
			EPRINTF("could not allocate colors %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		double pppx = (double) w / (double) sw;
		double pppy = (double) h / (double) sh;

		double lx, rx, ty, by, xf, yf, ff;

		unsigned R, G, B;

		for (ty = 0.0, by = pppy, l = 0; l < sh; l++, ty += pppy, by += pppy) {

			for (j = floor(ty); j < by; j++) {

				if (j < ty && j + 1 > ty) yf = ty - j;
				else if (j < by && j + 1 > by) yf = by - j;
				else yf = 1.0;

				for (lx = 0.0, rx = pppx, k = 0; k < sw; k++, lx += pppx, rx += pppx) {

					for (i = floor(lx); i < rx; i++) {

						if (i < lx && i + 1 > lx) xf = lx - i;
						else if (i < rx && i + 1 > rx) xf = rx - i;
						else xf = 1.0;

						ff = xf * yf;
						m = l * sw + k;
						n = m << 2;
						counts[m] += ff;
						if (!xmask || XGetPixel(xmask, i, j)) {
							colors[m] += ff;
							pixel = XGetPixel(xicon, i, j);
							R = (pixel >> 16) & 0xff;
							G = (pixel >>  8) & 0xff;
							B = (pixel >>  0) & 0xff;
							chanls[n+0] += 255 * ff;
							chanls[n+1] += R * ff;
							chanls[n+2] += G * ff;
							chanls[n+3] += B * ff;
						}
					}
				}
			}
		}
		unsigned amax = 0;
		for (l = 0; l < sh; l++) {
			for (k = 0; k < sw; k++) {
				n = l * sw + k;
				m = n << 2;
				pixel = 0;
				if (counts[n]) {
					pixel |= (lround(chanls[m+0] / counts[n]) & 0xff) << 24;
				}
				if (colors[n]) {
					pixel |= (lround(chanls[m+1] / colors[n]) & 0xff) << 16;
					pixel |= (lround(chanls[m+2] / colors[n]) & 0xff) <<  8;
					pixel |= (lround(chanls[m+3] / colors[n]) & 0xff) <<  0;
				}
				XPutPixel(ximage, k, l, pixel);
				amax = max(amax, ((pixel >> 24) & 0xff));
			}
		}
		if (!amax) {
			/* no opacity at all! */
			for (l = 0; l < sh; l++) {
				for (k = 0; k < sw; k++) {
					XPutPixel(ximage, k, l, XGetPixel(ximage, k, l) | 0xff000000);
				}
			}
		} else if (amax < 255) {
			/* something has to be opaque, bump the ximage */
			double bump = (double) 255 / (double) amax;
			for (l = 0; l < sh; l++) {
				for (k = 0; k < sw; k++) {
					pixel = XGetPixel(ximage, k, l);
					amax = (pixel >> 24) & 0xff;
					amax = lround(amax * bump);
					if (amax > 255)
						amax = 255;
					pixel = (pixel & 0x00ffffff) | (amax << 24);
					XPutPixel(ximage, k, l, pixel);
				}
			}
		}
		w = sw;
		h = sh;
		free(chanls);
		chanls = NULL;
		free(counts);
		counts = NULL;
		free(colors);
		colors = NULL;
	} else {
		if (!(ximage = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, w, h, 8, 0))) {
			EPRINTF("could not create image %ux%ux%u\n", w, h, d);
			goto error;
		}
		if (!(ximage->data = emallocz(ximage->bytes_per_line * ximage->height))) {
			EPRINTF("could not allocate image data %ux%ux%u\n", w, h, d);
			goto error;
		}
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				if (!xmask || XGetPixel(xmask, i, j)) {
					XPutPixel(ximage, i, j, XGetPixel(xicon, i, j) | 0xFF000000);
				} else {
					XPutPixel(ximage, i, j, XGetPixel(xicon, i, j) & 0x00FFFFFF);
				}
			}
		}
	}
	XDestroyImage(xicon);
	if (xmask)
		XDestroyImage(xmask);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		ButtonImage *bi = *bis;
		AdwmPixmap *px = &bi->px;

		px->x = px->y = px->b = 0;
		px->d = ds->depth;
		px->w = w;
		px->h = h;
		if (px->pixmap.draw) {
			XFreePixmap(dpy, px->pixmap.draw);
			px->pixmap.draw = None;
		}
		if (px->pixmap.mask) {
			XFreePixmap(dpy, px->pixmap.mask);
			px->pixmap.mask = None;
		}
		px->pixmap.draw = XCreatePixmap(dpy, ds->drawable, w, h, ds->depth);
		XPRINTF(__CFMTS(c) "assigning pixmap 0x%lx\n", __CARGS(c), px->pixmap.draw);
		XPutImage(dpy, px->pixmap.draw, ds->dc.gc, ximage, 0, 0, 0, 0, w, h);
		bi->present = True;
	}
	XDestroyImage(ximage);
	return (True);

      error:
	free(chanls);
	free(counts);
	free(colors);
	if (ximage)
		XDestroyImage(ximage);
	if (xmask)
		XDestroyImage(xmask);
	if (xicon)
		XDestroyImage(xicon);
      return (False);
}

Bool
xlib_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data)
{
	XImage *ximage = NULL;
	ButtonImage **bis;
	unsigned i, j, k, l, d = ds->depth, th = ds->style.titleheight;
	unsigned long pixel, bg;
	double *chanls = NULL; unsigned m;
	double *counts = NULL; unsigned n;
	double *colors = NULL;
	long *p, *copy;

	/* for now */
	bg = ds->style.color.norm[ColBG].pixel;
	(void) bg; /* this should be used to blend */

	if (ds->style.outline)
		th--;

	{
		unsigned xmin, xmax, ymin, ymax;
		XRectangle box;
		long *o;

		xmin = w; xmax = 0;
		ymin = h; ymax = 0;
		/* chromium and ROXTerm are placing too much space around icons */
		for (p = data, j = 0; j < h; j++) {
			for (i = 0; i < w; i++, p++) {
				if (*p & 0xFF000000) { /* not fully transparent */
					if (i < xmin) xmin = i;
					if (j < ymin) ymin = j;
					if (i > xmax) xmax = i;
					if (j > ymax) ymax = j;
				}
			}
		}
		box.x = xmin;
		box.y = ymin;
		box.width = xmax + 1 - xmin;
		box.height = ymax + 1 - ymin;

		if (box.x || box.y || box.width < w || box.height < h) {
			XPRINTF(__CFMTS(c) "clipping to bounding box of %hux%hu+%hd+%hd from %ux%u+0+0\n",
					__CARGS(c), box.width, box.height, box.x, box.y, w, h);
			if (!(copy = ecalloc(box.width * box.height, sizeof(*copy)))) {
				EPRINTF("could not allocate data copy %hux%hu\n", box.width, box.height);
				goto error;
			}
			/* copy data for bounding box only */
			for (p = data, o = copy, j = 0; j < h; j++, p += w)
				if (ymin <= j && j <= ymax)
					for (i = 0; i < w; i++)
						if (xmin <= i && i <= xmax)
							*o++ = p[i];
			w = box.width;
			h = box.height;
			data = copy;
		}
	}
	if (h > th) {
		double scale = (double) th / (double) h;

		unsigned sh = lround(h * scale);
		unsigned sw = lround(w * scale);

		XPRINTF(__CFMTS(c) "scaling data %p from %ux%u to %ux%u\n", __CARGS(c), data, w, h, sw, sh);

		if (!(ximage = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, sw, sh, 8, 0))) {
			EPRINTF("could not create image %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(ximage->data = emallocz(ximage->bytes_per_line * ximage->height))) {
			EPRINTF("could not allocate image data %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(chanls = ecalloc(ximage->bytes_per_line * ximage->height, 4 * sizeof(*chanls)))) {
			EPRINTF("could not allocate chanls %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(counts = ecalloc(ximage->bytes_per_line * ximage->height, sizeof(*counts)))) {
			EPRINTF("could not allocate counts %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(colors = ecalloc(ximage->bytes_per_line * ximage->height, sizeof(*colors)))) {
			EPRINTF("could not allocate colors %ux%ux%u\n", sw, sh, d);
			goto error;
		}

		double pppx = (double) w / (double) sw;
		double pppy = (double) h / (double) sh;

		double lx, rx, ty, by, xf, yf, ff;

		unsigned A, R, G, B, o;

		for (ty = 0.0, by = pppy, l = 0; l < sh; l++, ty += pppy, by += pppy) {

			for (j = floor(ty); j < by; j++) {

				if (j < ty && j + 1 > ty) yf = ty - j;
				else if (j < by && j + 1 > by) yf = by - j;
				else yf = 1.0;

				for (lx = 0.0, rx = pppx, k = 0; k < sw; k++, lx += pppx, rx += pppx) {

					for (i = floor(lx); i < rx; i++) {

						if (i < lx && i + 1 > lx) xf = lx - i;
						else if (i < rx && i + 1 > rx) xf = rx - i;
						else xf = 1.0;

						ff = xf * yf;
						m = l * sw + k;
						n = m << 2;
						o = j * w + i;
						pixel = data[o];
						A = (pixel >> 24) & 0xff;
						R = (pixel >> 16) & 0xff;
						G = (pixel >>  8) & 0xff;
						B = (pixel >>  0) & 0xff;
						counts[m] += ff;
						if (A) {
							colors[m] += ff;
							chanls[n+0] += A * ff;
							chanls[n+1] += R * ff;
							chanls[n+2] += G * ff;
							chanls[n+3] += B * ff;
						}
					}
				}
			}
		}
		unsigned amax = 0;
		for (l = 0; l < sh; l++) {
			for (k = 0; k < sw; k++) {
				n = l * sw + k;
				m = n << 2;
				pixel = 0;
				if (counts[n]) {
					pixel |= (lround(chanls[m+0] / counts[n]) & 0xff) << 24;
				}
				if (colors[n]) {
					pixel |= (lround(chanls[m+1] / colors[n]) & 0xff) << 16;
					pixel |= (lround(chanls[m+2] / colors[n]) & 0xff) <<  8;
					pixel |= (lround(chanls[m+3] / colors[n]) & 0xff) <<  0;
				}
				XPutPixel(ximage, k, l, pixel);
				amax = max(amax, ((pixel >> 24) & 0xff));
			}
		}
		if (!amax) {
			/* no opacity at all! */
			for (l = 0; l < sh; l++) {
				for (k = 0; k < sw; k++) {
					XPutPixel(ximage, k, l, XGetPixel(ximage, k, l) | 0xff000000);
				}
			}
		} else if (amax < 255) {
			/* something has to be opaque, bump the alpha */
			double bump = (double) 255 / (double) amax;
			for (l = 0; l < sh; l++) {
				for (k = 0; k < sw; k++) {
					pixel = XGetPixel(ximage, k, l);
					amax = (pixel >> 24) & 0xff;
					amax = lround(amax * bump);
					if (amax > 255)
						amax = 255;
					pixel = (pixel & 0x00ffffff) | (amax << 24);
					XPutPixel(ximage, k, l, pixel);
				}
			}
		}
		w = sw;
		h = sh;
		free(chanls);
		chanls = NULL;
		free(counts);
		counts = NULL;
		free(colors);
		colors = NULL;
	} else {
		if (!(ximage = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, w, h, 8, 0))) {
			EPRINTF("could not create image %ux%ux%u\n", w, h, d);
			goto error;
		}
		if (!(ximage->data = emallocz(ximage->bytes_per_line * ximage->height))) {
			EPRINTF("could not allocate image data %ux%ux%u\n", w, h, d);
			goto error;
		}
		for (p = data, j = 0; j < h; j++) {
			for (i = 0; i < w; i++, p++) {
				XPutPixel(ximage, i, j, *p);
			}
		}
	}

	for (bis = getbuttons(c); bis && *bis; bis++) {
		ButtonImage *bi = *bis;
		AdwmPixmap *px = &bi->px;

		px->x = px->y = px->b = 0;
		px->d = d;
		px->w = w;
		px->h = h;
		if (px->pixmap.draw) {
			XFreePixmap(dpy, px->pixmap.draw);
			px->pixmap.draw = None;
		}
		if (px->pixmap.mask) {
			XFreePixmap(dpy, px->pixmap.mask);
			px->pixmap.mask = None;
		}
		px->pixmap.draw = XCreatePixmap(dpy, ds->drawable, w, h, d);
		XPRINTF(__CFMTS(c) "assigning pixmap 0x%lx\n", __CARGS(c), px->pixmap.draw);
		XPutImage(dpy, px->pixmap.draw, ds->dc.gc, ximage, 0, 0, 0, 0, w, h);
		bi->present = True;
	}
	XDestroyImage(ximage);
	return (True);

      error:
	free(chanls);
	free(counts);
	free(colors);
	if (ximage)
		XDestroyImage(ximage);
      return (False);
}

Bool
xlib_createpngicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use XLIB to create PNG icon %s\n", file);
	/* for now */
	return (False);
}

Bool
xlib_createsvgicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use XLIB to create SVG icon %s\n", file);
	/* for now */
	return (False);
}

Bool
xlib_createxpmicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use XLIB to create XPM icon %s\n", file);
	/* for now */
	return (False);
}

Bool
xlib_createxbmicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use XLIB to create XBM icon %s\n", file);
	/* for now */
	return (False);
}

#ifdef DAMAGE
Bool
xlib_drawdamage(Client *c, XDamageNotifyEvent * ev)
{
	return (False);
}
#endif

int
xlib_drawbutton(AScreen *ds, Client *c, ElementType type, XftColor *col, int x)
{
	ElementClient *ec = &c->element[type];
	Drawable d = ds->dc.draw.pixmap;
	XftColor *fg, *bg;
	Geometry g = { 0, };
	ButtonImage *bi;
	int status, th;

	if (!(bi = buttonimage(ds, c, type)) || !bi->present) {
		XPRINTF("button %d has no button image\n", type);
		return 0;
	}
	AdwmPixmap *px = &bi->px;

	th = ds->dc.h;
	if (ds->style.outline)
		th--;

	/* geometry of the container */
	g.x = x;
	g.y = 0;
	g.w = max(th, px->w);
	g.h = th;

	/* element client geometry used to detect element under pointer */
	ec->eg.x = g.x + (g.w - px->w) / 2;
	ec->eg.y = g.y + (g.h - px->h) / 2;
	ec->eg.w = px->w;
	ec->eg.h = px->h;

	fg = ec->pressed ? &col[ColFG] : &col[ColButton];
	bg = bi->bg.pixel ? &bi->bg : &col[ColBG];

	if (px->bitmap.ximage) {
		XImage *ximage;

		if ((ximage = XSubImage(px->bitmap.ximage, 0, 0,
					px->bitmap.ximage->width,
					px->bitmap.ximage->height))) {
			unsigned long pixel;
			unsigned A, R, G, B;
			int i, j;

			unsigned rf = (fg->pixel >> 16) & 0xff;
			unsigned gf = (fg->pixel >>  8) & 0xff;
			unsigned bf = (fg->pixel >>  0) & 0xff;

			unsigned rb = (bg->pixel >> 16) & 0xff;
			unsigned gb = (bg->pixel >>  8) & 0xff;
			unsigned bb = (bg->pixel >>  0) & 0xff;

			/* blend image with foreground and background */
			for (j = 0; j < ximage->height; j++) {
				for (i = 0; i < ximage->width; i++) {
					pixel = XGetPixel(ximage, i, j);

					A = (pixel >> 24) & 0xff;
					R = (pixel >> 16) & 0xff;
					G = (pixel >>  8) & 0xff;
					B = (pixel >>  0) & 0xff;

					R = ((A * rf) + ((255 - A) * rb)) / 255;
					G = ((A * gf) + ((255 - A) * gb)) / 255;
					B = ((A * bf) + ((255 - A) * bb)) / 255;

					pixel = (A << 24) | (R << 16) | (G << 8) | B;
					XPutPixel(ximage, i, j, pixel);
				}
			}
			{
				/* TODO: eventually this should be a texture */
				/* always draw the element background */
				XSetForeground(dpy, ds->dc.gc, bg->pixel);
				XSetFillStyle(dpy, ds->dc.gc, FillSolid);
				status = XFillRectangle(dpy, d, ds->dc.gc, ec->eg.x, ec->eg.y, ec->eg.w, ec->eg.h);
				if (!status)
					XPRINTF("Could not fill rectangle, error %d\n", status);
				XSetForeground(dpy, ds->dc.gc, fg->pixel);
				XSetBackground(dpy, ds->dc.gc, bg->pixel);
			}
			XPRINTF(__CFMTS(c) "copying bitmap ximage %dx%dx%d+%d+%d to +%d+%d\n", __CARGS(c),
					px->w, px->h, px->d, px->x, px->y, ec->eg.x, ec->eg.y);
			XPutImage(dpy, d, ds->dc.gc, ximage, px->x, px->y, ec->eg.x, ec->eg.y, px->w, px->h);
			XDestroyImage(ximage);
			return g.w;
		}
	}
	if (px->pixmap.ximage) {
		XImage *ximage;

		if ((ximage = XSubImage(px->pixmap.ximage, 0, 0,
					px->pixmap.ximage->width,
					px->pixmap.ximage->height))) {
			unsigned long pixel;
			unsigned A, R, G, B;
			int i, j;

			unsigned rb = (bg->pixel >> 16) & 0xff;
			unsigned gb = (bg->pixel >>  8) & 0xff;
			unsigned bb = (bg->pixel >>  0) & 0xff;

			/* blend image with foreground and background */
			for (j = 0; j < ximage->height; j++) {
				for (i = 0; i < ximage->width; i++) {
					pixel = XGetPixel(ximage, i, j);

					A = (pixel >> 24) & 0xff;
					R = (pixel >> 16) & 0xff;
					G = (pixel >>  8) & 0xff;
					B = (pixel >>  0) & 0xff;

					R = ((A * R) + ((255 - A) * rb)) / 255;
					G = ((A * G) + ((255 - A) * gb)) / 255;
					B = ((A * B) + ((255 - A) * bb)) / 255;

					pixel = (A << 24) | (R << 16) | (G << 8) | B;
					XPutPixel(ximage, i, j, pixel);
				}
			}
			{
				/* TODO: eventually this should be a texture */
				/* always draw the element background */
				XSetForeground(dpy, ds->dc.gc, bg->pixel);
				XSetFillStyle(dpy, ds->dc.gc, FillSolid);
				status = XFillRectangle(dpy, d, ds->dc.gc, ec->eg.x, ec->eg.y, ec->eg.w, ec->eg.h);
				if (!status)
					XPRINTF("Could not fill rectangle, error %d\n", status);
				XSetForeground(dpy, ds->dc.gc, fg->pixel);
				XSetBackground(dpy, ds->dc.gc, bg->pixel);
			}
			XPRINTF(__CFMTS(c) "copying pixmap ximage %dx%dx%d+%d+%d to +%d+%d\n", __CARGS(c),
					px->w, px->h, px->d, px->x, px->y, ec->eg.x, ec->eg.y);
			XPutImage(dpy, d, ds->dc.gc, ximage, px->x, px->y, ec->eg.x, ec->eg.y, px->w, px->h);
			XDestroyImage(ximage);
			return g.w;
		}
	}
#if defined IMLIB2
	if (px->pixmap.image) {
		Imlib_Image image;

		imlib_context_push(scr->context);
		imlib_context_set_image(px->pixmap.image);
		imlib_context_set_mask(None);
		image = imlib_create_image(ec->eg.w, ec->eg.h);
		imlib_context_set_image(image);
		imlib_context_set_mask(None);
		imlib_context_set_color(bg->color.red, bg->color.green, bg->color.blue, 255);
		imlib_image_fill_rectangle(0, 0, ec->eg.w, ec->eg.h);
		imlib_context_set_blend(0);
		imlib_context_set_anti_alias(1);
		imlib_blend_image_onto_image(px->pixmap.image, False,
				0, 0, px->w, px->h,
				0, 0, ec->eg.w, ec->eg.h);
		imlib_context_set_drawable(d);
		imlib_render_image_on_drawable(ec->eg.x, ec->eg.y);
		imlib_free_image();
		imlib_context_pop();
		return g.w;
	} else
	if (px->bitmap.image) {
		Imlib_Image image, mask;

		imlib_context_push(scr->context);
		imlib_context_set_image(px->bitmap.image);
		imlib_context_set_mask(None);
		image = imlib_create_image(ec->eg.w, ec->eg.h);
		imlib_context_set_image(image);
		imlib_context_set_mask(None);
		imlib_context_set_color(bg->color.red, bg->color.green, bg->color.blue, 255);
		imlib_image_fill_rectangle(0, 0, ec->eg.w, ec->eg.h);
		mask = imlib_create_image(px->w, px->h);
		imlib_context_set_image(mask);
		imlib_context_set_mask(None);
		imlib_context_set_color(fg->color.red, fg->color.green, fg->color.blue, 255);
		imlib_image_fill_rectangle(0, 0, px->w, px->h);
		imlib_image_copy_alpha_to_image(px->bitmap.image, 0, 0);
		imlib_context_set_image(image);
		imlib_context_set_mask(None);
		imlib_context_set_blend(0);
		imlib_context_set_anti_alias(1);
		imlib_blend_image_onto_image(mask, False,
				0, 0, px->w, px->h,
				0, 0, ec->eg.w, ec->eg.h);
		imlib_context_set_drawable(d);
		imlib_render_image_on_drawable(ec->eg.x, ec->eg.y);
		imlib_free_image();
		imlib_context_set_image(mask);
		imlib_context_set_mask(None);
		imlib_free_image();
		imlib_context_pop();
		return g.w;
	} else
#endif
	if (px->pixmap.draw) {
		{
			/* TODO: eventually this should be a texture */
			/* always draw the element background */
			XSetForeground(dpy, ds->dc.gc, bg->pixel);
			XSetFillStyle(dpy, ds->dc.gc, FillSolid);
			status = XFillRectangle(dpy, d, ds->dc.gc, ec->eg.x, ec->eg.y, ec->eg.w, ec->eg.h);
			if (!status)
				XPRINTF("Could not fill rectangle, error %d\n", status);
			XSetForeground(dpy, ds->dc.gc, fg->pixel);
			XSetBackground(dpy, ds->dc.gc, bg->pixel);
		}
		/* position clip mask over button image */
		XSetClipOrigin(dpy, ds->dc.gc, ec->eg.x - px->x, ec->eg.y - px->y);

		XPRINTF("Copying pixmap 0x%lx mask 0x%lx with geom %dx%d+%d+%d to drawable 0x%lx\n",
		        px->pixmap.draw, px->pixmap.mask, ec->eg.w, ec->eg.h, ec->eg.x, ec->eg.y, d);
		XSetClipMask(dpy, ds->dc.gc, px->pixmap.mask);
		XPRINTF(__CFMTS(c) "copying pixmap %dx%dx%d+%d+%d to +%d+%d\n", __CARGS(c),
				px->w, px->h, px->d, px->x, px->y, ec->eg.x, ec->eg.y);
		XCopyArea(dpy, px->pixmap.draw, d, ds->dc.gc,
			  px->x, px->y, px->w, px->h, ec->eg.x, ec->eg.y);
		XSetClipMask(dpy, ds->dc.gc, None);
		return g.w;
	} else
	if (px->bitmap.draw) {
		{
			/* TODO: eventually this should be a texture */
			/* always draw the element background */
			XSetForeground(dpy, ds->dc.gc, bg->pixel);
			XSetFillStyle(dpy, ds->dc.gc, FillSolid);
			status = XFillRectangle(dpy, d, ds->dc.gc, ec->eg.x, ec->eg.y, ec->eg.w, ec->eg.h);
			if (!status)
				XPRINTF("Could not fill rectangle, error %d\n", status);
			XSetForeground(dpy, ds->dc.gc, fg->pixel);
			XSetBackground(dpy, ds->dc.gc, bg->pixel);
		}
		/* position clip mask over button image */
		XSetClipOrigin(dpy, ds->dc.gc, ec->eg.x - px->x, ec->eg.y - px->y);

		XSetClipMask(dpy, ds->dc.gc, px->bitmap.mask ? : px->bitmap.draw);
		XPRINTF(__CFMTS(c) "copying bitmap %dx%dx%d+%d+%d to +%d+%d\n", __CARGS(c),
				px->w, px->h, px->d, px->x, px->y, ec->eg.x, ec->eg.y);
		XCopyPlane(dpy, px->bitmap.draw, d, ds->dc.gc,
			   px->x, px->y, px->w, px->h, ec->eg.x, ec->eg.y, 1);
		XSetClipMask(dpy, ds->dc.gc, None);
		return g.w;
	}
	XPRINTF("button %d has no pixmap or bitmap\n", type);
	return 0;
}

int
xlib_drawsep(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw,
	 XftColor *col, int hilite, int x, int y, int mw)
{
	int th;
	Geometry g;

	th = ds->dc.h;
	if (ds->style.outline)
		th--;

	g.x = x + th / 4;
	g.y = 0;
	g.w = th / 4 + th / 4;
	g.h = th;

	XSetForeground(dpy, ds->dc.gc, col->pixel);
	XDrawLine(dpy, drawable, ds->dc.gc, g.x, g.y, g.x, g.y + g.h);
	return (g.w);
}

int
xlib_drawtext(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw,
	      XftColor *col, int hilite, int x, int y, int mw)
{
	int w, h;
	char buf[256];
	unsigned int len, olen;
	int gap, status;
	XftFont *font = ds->style.font[hilite];
	struct _FontInfo *info = &ds->dc.font[hilite];
	XftColor *fcol = ds->style.color.hue[hilite];
	int drop = ds->style.drop[hilite];

	if (!text)
		return 0;
	olen = len = strlen(text);
	w = 0;
	if (len >= sizeof buf)
		len = sizeof buf - 1;
	memcpy(buf, text, len);
	buf[len] = 0;
	h = ds->style.titleheight;
	y = ds->dc.h / 2 + info->ascent / 2 - 1 - ds->style.outline;
	gap = info->height / 2;
	x += gap;
	/* shorten text if necessary */
	while (len && (w = textnw(ds, buf, len, hilite)) > mw) {
		buf[--len] = 0;
	}
	if (len < olen) {
		if (len > 1)
			buf[len - 1] = '.';
		if (len > 2)
			buf[len - 2] = '.';
		if (len > 3)
			buf[len - 3] = '.';
	}
	if (w > mw)
		return 0;	/* too long */
	while (x <= 0)
		x = ds->dc.x++;

	XSetForeground(dpy, ds->dc.gc, col[ColBG].pixel);
	XSetFillStyle(dpy, ds->dc.gc, FillSolid);
	status = XFillRectangle(dpy, drawable, ds->dc.gc, x - gap, 0, w + gap * 2, h);
	if (!status)
		XPRINTF("Could not fill rectangle, error %d\n", status);
	if (drop)
		XftDrawStringUtf8(xftdraw, &fcol[ColShadow], font, x + drop, y + drop,
				  (unsigned char *) buf, len);
	XftDrawStringUtf8(xftdraw, &fcol[ColFG], font, x, y, (unsigned char *) buf, len);
	return w + gap * 2;
}

void
xlib_drawdockapp(AScreen *ds, Client *c)
{
	int status;
	unsigned long pixel;

	pixel = getpixel(ds, c, ColBG);
	/* to avoid clearing window, initiallly set to norm[ColBG] */
	// XSetWindowBackground(dpy, c->frame, pixel);
	// XClearArea(dpy, c->frame, 0, 0, 0, 0, True);

	ds->dc.x = ds->dc.y = 0;
	if (!(ds->dc.w = c->c.w))
		return;
	if (!(ds->dc.h = c->c.h))
		return;
	XSetForeground(dpy, ds->gc, pixel);
	XSetLineAttributes(dpy, ds->gc, ds->style.border, LineSolid, CapNotLast,
			   JoinMiter);
	XSetFillStyle(dpy, ds->gc, FillSolid);
	status = XFillRectangle(dpy, c->frame, ds->gc,
			ds->dc.x, ds->dc.y, ds->dc.w, ds->dc.h);
	if (!status)
		XPRINTF("Could not fill rectangle, error %d\n", status);
	CPRINTF(c, "Filled dockapp frame %dx%d+%d+%d\n", ds->dc.w, ds->dc.h, ds->dc.x,
		ds->dc.y);
	/* note that ParentRelative dockapps need the background set to the foregroudn */
	XSetWindowBackground(dpy, c->frame, pixel);
	/* following are quite disruptive - many dockapps ignore expose events and simply
	   update on timer */
	// XClearWindow(dpy, c->icon ? : c->win);
	// XClearArea(dpy, c->icon ? : c->win, 0, 0, 0, 0, True);
}

void
xlib_drawnormal(AScreen *ds, Client *c)
{
	size_t i;
	int status;

	ds->dc.x = ds->dc.y = 0;
	ds->dc.w = c->c.w;
	ds->dc.h = ds->style.titleheight;
	if (ds->dc.draw.w < ds->dc.w) {
		XFreePixmap(dpy, ds->dc.draw.pixmap);
		ds->dc.draw.w = ds->dc.w;
		XPRINTF(__CFMTS(c) "creating title pixmap %dx%dx%d\n", __CARGS(c),
				ds->dc.w, ds->dc.draw.h, ds->depth);
		ds->dc.draw.pixmap = XCreatePixmap(dpy, ds->root, ds->dc.w,
						   ds->dc.draw.h, ds->depth);
		XftDrawChange(ds->dc.draw.xft, ds->dc.draw.pixmap);
	}
	XSetForeground(dpy, ds->dc.gc, getpixel(ds, c, ColBG));
	XSetLineAttributes(dpy, ds->dc.gc, ds->style.border, LineSolid, CapNotLast,
			   JoinMiter);
	XSetFillStyle(dpy, ds->dc.gc, FillSolid);
	status = XFillRectangle(dpy, ds->dc.draw.pixmap, ds->dc.gc,
				ds->dc.x, ds->dc.y, ds->dc.w, ds->dc.h);
	if (!status)
		XPRINTF("Could not fill rectangle, error %d\n", status);
	/* Don't know about this... */
	if (ds->dc.w < textw(ds, c->name, gethilite(c))) {
		ds->dc.w -= elementw(ds, c, CloseBtn);
		xlib_drawtext(ds, c->name, ds->dc.draw.pixmap, ds->dc.draw.xft,
				gethues(ds, c), gethilite(c), ds->dc.x, ds->dc.y, ds->dc.w);
		xlib_drawbutton(ds, c, CloseBtn, gethues(ds, c), ds->dc.w);
		goto end;
	}
	/* Left */
	ds->dc.x += (ds->style.spacing > ds->style.border) ?
	    ds->style.spacing - ds->style.border : 0;
	for (i = 0; i < strlen(ds->style.titlelayout); i++) {
		if (ds->style.titlelayout[i] == ' ' || ds->style.titlelayout[i] == '-')
			break;
		ds->dc.x +=
		    drawelement(ds, ds->style.titlelayout[i], ds->dc.x, AlignLeft, c);
		ds->dc.x += ds->style.spacing;
	}
	if (i == strlen(ds->style.titlelayout) || ds->dc.x >= ds->dc.w)
		goto end;
	/* Center */
	ds->dc.x = ds->dc.w / 2;
	for (i++; i < strlen(ds->style.titlelayout); i++) {
		if (ds->style.titlelayout[i] == ' ' || ds->style.titlelayout[i] == '-')
			break;
		ds->dc.x -= elementw(ds, c, ds->style.titlelayout[i]) / 2;
		ds->dc.x += drawelement(ds, ds->style.titlelayout[i], 0, AlignCenter, c);
	}
	if (i == strlen(ds->style.titlelayout) || ds->dc.x >= ds->dc.w)
		goto end;
	/* Right */
	ds->dc.x = ds->dc.w;
	ds->dc.x -= (ds->style.spacing > ds->style.border) ?
	    ds->style.spacing - ds->style.border : 0;
	for (i = strlen(ds->style.titlelayout); i--;) {
		if (ds->style.titlelayout[i] == ' ' || ds->style.titlelayout[i] == '-')
			break;
		ds->dc.x -= elementw(ds, c, ds->style.titlelayout[i]);
		drawelement(ds, ds->style.titlelayout[i], 0, AlignRight, c);
		ds->dc.x -= ds->style.spacing;
	}
      end:
	if (ds->style.outline) {
		XSetForeground(dpy, ds->dc.gc, getpixel(ds, c, ColBorder));
		XPRINTF(__CFMTS(c) "drawing line from +%d+%d to +%d+%d\n", __CARGS(c),
				0, ds->dc.h - 1, ds->dc.w, ds->dc.h - 1);
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, 0, ds->dc.h - 1, ds->dc.w,
			  ds->dc.h - 1);
	}
	if (c->title) {
		XPRINTF(__CFMTS(c) "copying title pixmap to %dx%d+%d+%d to +%d+%d\n",
			__CARGS(c), c->c.w, ds->dc.h, 0, 0, 0, 0);
		XCopyArea(dpy, ds->dc.draw.pixmap, c->title, ds->dc.gc, 0, 0, c->c.w,
			  ds->dc.h, 0, 0);
	}
	ds->dc.x = ds->dc.y = 0;
	ds->dc.w = c->c.w;
	ds->dc.h = ds->style.gripsheight;
	XSetForeground(dpy, ds->dc.gc, getpixel(ds, c, ColBG));
	XSetLineAttributes(dpy, ds->dc.gc, ds->style.border, LineSolid, CapNotLast,
			   JoinMiter);
	XSetFillStyle(dpy, ds->dc.gc, FillSolid);
	status = XFillRectangle(dpy, ds->dc.draw.pixmap, ds->dc.gc,
				ds->dc.x, ds->dc.y, ds->dc.w, ds->dc.h);
	if (!status)
		XPRINTF("Could not fill rectangle, error %d\n", status);
	if (ds->style.outline) {
		XSetForeground(dpy, ds->dc.gc, getpixel(ds, c, ColBorder));
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, 0, 0, ds->dc.w, 0);
		/* needs to be adjusted to do ds->style.gripswidth instead */
		ds->dc.x = ds->dc.w / 2 - ds->dc.w / 5;
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x, 0, ds->dc.x,
			  ds->dc.h);
		ds->dc.x = ds->dc.w / 2 + ds->dc.w / 5;
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x, 0, ds->dc.x,
			  ds->dc.h);
	}
	if (c->grips)
		XCopyArea(dpy, ds->dc.draw.pixmap, c->grips, ds->dc.gc, 0, 0, c->c.w,
			  ds->dc.h, 0, 0);
}

Bool
xlib_initpng(char *path, AdwmPixmap *px)
{
	return (False);
}

Bool
xlib_initsvg(char *path, AdwmPixmap *px)
{
	return (False);
}

#ifdef XPM
Bool
xlib_initxpm(char *path, AdwmPixmap *px)
{
	Pixmap draw = None, mask = None;
	unsigned w, h;
	int status;

	{
		XpmAttributes xa = { 0, };

		xa.visual = scr->visual;
		xa.valuemask |= XpmVisual;
		xa.colormap = scr->colormap;
		xa.valuemask |= XpmColormap;
		xa.depth = scr->depth;
		xa.valuemask |= XpmDepth;

		status = XpmReadFileToPixmap(dpy, scr->drawable, path, &draw, &mask, &xa);
		if (status != XpmSuccess || !draw) {
			EPRINTF("could not load xpm file: %s on %s\n", xpm_status_string(status), path);
			return False;
		}
		w = xa.width;
		h = xa.height;
		XpmFreeAttributes(&xa);
	}
	if (px->pixmap.draw) {
		XFreePixmap(dpy, px->pixmap.draw);
		px->pixmap.draw = None;
	}
	if (px->pixmap.mask) {
		XFreePixmap(dpy, px->pixmap.mask);
		px->pixmap.mask = None;
	}
	px->pixmap.draw = draw;
	px->pixmap.mask = mask;
	px->w = w;
	px->h = h;
	XPRINTF("%s pixmap geometry is %dx%dx%d+%d+%d\n", path, px->w, px->h, scr->depth, px->x, px->y);
	if (px->h > scr->style.titleheight) {
		/* read lower down into image to clip top and bottom by same amount */
		px->y += (px->h - scr->style.titleheight) / 2;
		px->h = scr->style.titleheight;
		XPRINTF("%s pixmap clipped to %dx%dx%d+%d+%d\n", path, px->w, px->h, scr->depth, px->x, px->y);
	}
	px->file = path;
	return True;
}
#else
Bool
xlib_initxpm(char *path, AdwmPixmap *px)
{
	return (False);
}
#endif				/* !XPM */

Bool
xlib_initxbm(char *path, AdwmPixmap *px)
{
	Pixmap draw = None;
	int status;

	status = XReadBitmapFile(dpy, scr->root, path, &px->w, &px->h, &draw, &px->x, &px->y);
	if (status != BitmapSuccess || !draw) {
		EPRINTF("could not load xbm file %s\n", path);
		return False;
	}
	if (px->bitmap.draw) {
		XFreePixmap(dpy, px->bitmap.draw);
		px->bitmap.draw = None;
	}
	if (px->bitmap.mask) {
		XFreePixmap(dpy, px->bitmap.mask);
		px->bitmap.mask = None;
	}
	px->bitmap.draw = draw;
	px->bitmap.mask = None;
	/* don't care about hostspot: not a cursor */
	px->x = px->y = px->b = 0;
	if (px->h > scr->style.titleheight) {
		/* read lower down into image to clip top and bottom by same amount */
		px->y += (px->h - scr->style.titleheight) / 2;
		px->h = scr->style.titleheight;
	}
	px->file = path;
	return True;
}

Bool
xlib_initxbmdata(const unsigned char *bits, int w, int h, AdwmPixmap *px)
{
	Pixmap draw = None;

	if (!(draw = XCreateBitmapFromData(dpy, scr->drawable, (char *)bits, w, h))) {
		EPRINTF("could not load xbm data\n");
		return False;
	}
	if (px->bitmap.draw) {
		XFreePixmap(dpy, px->bitmap.draw);
		px->bitmap.draw = None;
	}
	if (px->bitmap.mask) {
		XFreePixmap(dpy, px->bitmap.mask);
		px->bitmap.mask = None;
	}
	px->bitmap.draw = draw;
	px->bitmap.mask = None;
	/* don't care about hostspot: not a cursor */
	px->x = px->y = px->b = 0;
	px->w = w;
	px->h = h;
	if (px->h > scr->style.titleheight) {
		/* read lower down into image to clip top and bottom by same amount */
		px->y += (px->h - scr->style.titleheight) / 2;
		px->h = scr->style.titleheight;
	}
	free(px->file);
	px->file = NULL;
	return True;
}
