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
#include "ximage.h" /* verification */

static Bool
crop_image_to_mask(XImage **xdraw, XImage **xmask)
{
	unsigned i, j;
	unsigned xmin, xmax, ymin, ymax;
	XRectangle box;
	XImage *draw = NULL, *mask = NULL, *newdraw = NULL, *newmask = NULL;
	unsigned long bits;

	if (!xdraw || !(draw = *xdraw))
		return (False);

	Bool hasalpha = (draw->depth == 32) ? True : False;

	if (!xmask && !hasalpha)
		return (False);

	mask = hasalpha ? draw : *xmask;
	bits = hasalpha ? 0xFF000000 :0x00000001;

	if (!mask)
		return (False);

	unsigned w = draw->width;
	unsigned h = draw->height;

	xmin = w; xmax = 0;
	ymin = h; ymax = 0;
	/* chromium and ROXTerm are placing too much space around icons */
	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			if (XGetPixel(mask, i, j) & bits) {
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
		XPRINTF("clipping to bounding box of %hux%hu+%hd+%hd from %ux%u+0+0\n",
			box.width, box.height, box.x, box.y, w, h);
		if (!(newdraw = XSubImage(draw, box.x, box.y, box.width, box.height)))
			goto error;
		if (xmask && !(newmask = XSubImage(mask, box.x, box.y, box.width, box.height)))
			goto error;
		*xdraw = newdraw;
		if (xmask)
			*xmask = newmask;
		XDestroyImage(draw);
		if (mask && mask != draw)
			XDestroyImage(mask);
		return (True);
	}
	return (False);

      error:
	if (newdraw)
		XDestroyImage(newdraw);
	else
		EPRINTF("could not alloc subimage %hux%hu+%hd+%d from %ux%u+0+0\n",
			box.width, box.height, box.x, box.y, w, h);
	if (newmask)
		XDestroyImage(newmask);
	else
		EPRINTF("could not alloc subimage %hux%hu+%hd+%d from %ux%u+0+0\n",
			box.width, box.height, box.x, box.y, w, h);
	return (False);
}

static XImage *
scale_pixmap_and_mask(AScreen *ds, XImage *xdraw, XImage *xmask, int newh)
{
	XImage *ximage = NULL, *mask;
	unsigned w = xdraw->width;
	unsigned h = xdraw->height;
	unsigned long bits;

	Bool ispixmap = (xdraw->depth > 1) ? True : False;
	Bool hasalpha = (xdraw->depth == 32) ? True : False;

	mask = hasalpha ? xdraw : xmask;
	bits = hasalpha ? 0xFF000000 : 0x00000001;

	unsigned i, j, k, l;
	unsigned long pixel;
	double *chanls = NULL; unsigned m;
	double *counts = NULL; unsigned n;
	double *colors = NULL;

	double scale = (double) newh / (double) h;

	unsigned sh = lround(h * scale);
	unsigned sw = lround(w * scale);

	XPRINTF("scaling bitmap from %ux%u to %ux%u\n", w, h, sw, sh);

	if (!(ximage = XCreateImage(dpy, ds->visual, ds->depth, ZPixmap, 0, NULL, sw, sh, 8, 0))) {
		EPRINTF("could not create image %ux%ux%u\n", sw, sh, ds->depth);
		return (ximage);
	}
	ximage->data = emallocz(ximage->bytes_per_line * ximage->height);
	chanls = ecalloc(ximage->bytes_per_line * ximage->height, 4 * sizeof(*chanls));
	counts = ecalloc(ximage->bytes_per_line * ximage->height, sizeof(*counts));
	colors = ecalloc(ximage->bytes_per_line * ximage->height, sizeof(*colors));

	double pppx = (double) sw / (double) w;
	double pppy = (double) sh / (double) h;

	double lx, rx, ty, by, xf, yf, ff;

	unsigned A, R, G, B;

	for (ty = 0.0, by = pppy, l = 0; l < h; l++, ty = by, by = (l + 1) * pppy) {

		for (j = floor(ty); j < by; j++) {

			if (ty < (j + 1) && (j + 1) < by) yf = (j + 1) - ty;
			else if (ty < j && j < by) yf = by - j;
			else yf = 1.0;

			for (lx = 0.0, rx = pppx, k = 0; k < w; k++, lx = rx, rx = (k + 1) * pppx) {

				for (i = floor(lx); i < rx; i++) {

					if (lx < (i + 1) && (i + 1) < rx) yf = (i + 1) - lx;
					else if (lx < i && i < rx) yf = rx - i;
					else xf = 1.0;

					ff = xf * yf;
					m = j * sw + i;
					n = m << 2;
					counts[m] += ff;
					if (!mask || (XGetPixel(mask, k, l) & bits)) {
						pixel = XGetPixel(xdraw, k, l);
						A = hasalpha ? (pixel >> 24) & 0xff : 255;
						if (ispixmap) {
							R = (pixel >> 16) & 0xff;
							G = (pixel >>  8) & 0xff;
							B = (pixel >>  0) & 0xff;
							A = hasalpha ? A : 255;
						} else if (pixel) {
							R = G = B = 255;
						} else
							continue;
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
	for (j = 0; j < sh; j++) {
		for (i = 0; i < sw; i++) {
			n = j * sw + i;
			m = n << 2;
			pixel = 0;
			if (counts[n]) {
				pixel |= (min(255, lround(chanls[m+0] / counts[n])) & 0xff) << 24;
			}
			if (colors[n]) {
				pixel |= (min(255, lround(chanls[m+1] / colors[n])) & 0xff) << 16;
				pixel |= (min(255, lround(chanls[m+2] / colors[n])) & 0xff) <<  8;
				pixel |= (min(255, lround(chanls[m+3] / colors[n])) & 0xff) <<  0;
			}
			XPutPixel(ximage, i, j, pixel);
			amax = max(amax, ((pixel >> 24) & 0xff));
		}
	}
	if (!amax) {
		/* no opacity at all! */
		for (j = 0; j < sh; j++) {
			for (i = 0; i < sw; i++) {
				XPutPixel(ximage, i, j, XGetPixel(ximage, i, j) | 0xff000000);
			}
		}
	} else if (amax < 255) {
		/* something has to be opaque, bump the ximage */
		double bump = (double) 255 / (double) amax;
		for (j = 0; j < sh; j++) {
			for (i = 0; i < sw; i++) {
				pixel = XGetPixel(ximage, i, j);
				amax = (pixel >> 24) & 0xff;
				amax = min(255, lround(amax * bump));
				pixel = (pixel & 0x00ffffff) | (amax << 24);
				XPutPixel(ximage, i, j, pixel);
			}
		}
	}
	free(chanls);
	free(counts);
	free(colors);
	return (ximage);
}

void
ximage_removepixmap(AdwmPixmap *p)
{
	if (p->pixmap.draw) {
		XFreePixmap(dpy, p->pixmap.draw);
		p->pixmap.draw = None;
	}
	if (p->pixmap.mask) {
		XFreePixmap(dpy, p->pixmap.mask);
		p->pixmap.mask = None;
	}
	if (p->pixmap.ximage) {
		XDestroyImage(p->pixmap.ximage);
		p->pixmap.ximage = NULL;
	}
	if (p->bitmap.draw) {
		XFreePixmap(dpy, p->bitmap.draw);
		p->bitmap.draw = None;
	}
	if (p->bitmap.mask) {
		XFreePixmap(dpy, p->bitmap.mask);
		p->bitmap.mask = None;
	}
	if (p->bitmap.ximage) {
		XDestroyImage(p->bitmap.ximage);
		p->bitmap.ximage = NULL;
	}
}

static Bool
ximage_createicon(AScreen *ds, Client *c, XImage **xicon, XImage **xmask, Bool cropscale)
{
	XImage *ximage = NULL;
	ButtonImage **bis;
	unsigned th = ds->style.titleheight;
	Bool ispixmap = ((*xicon)->depth > 1) ? True : False;

	if (ds->style.outline)
		th--;

	if (cropscale && (*xicon)->height > th && crop_image_to_mask(xicon, xmask))
		XPRINTF("cropped ximage to %ux%u\n", (*xicon)->width, (*xicon)->height);
	ximage = ((*xicon)->height > th)
	    ? scale_pixmap_and_mask(ds, (*xicon), xmask ? (*xmask) : NULL, th)
	    : combine_pixmap_and_mask(ds, (*xicon), xmask ? (*xmask) : NULL);
	if (!ximage) {
		EPRINTF("could not scale or combine xicon and xmask\n");
		goto error;
	}
	XPRINTF("scaled xicon to %ux%u\n", icon, mask, ximage->width, ximage->height);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		ButtonImage *bi = *bis;
		AdwmPixmap *px = &bi->px;

		px->x = px->y = px->b = 0;
		px->d = ds->depth;
		px->w = ximage->width;
		px->h = ximage->height;
		if (ispixmap) {
			if (px->pixmap.ximage) {
				XDestroyImage(px->pixmap.ximage);
				px->pixmap.ximage = NULL;
			}
			XPRINTF(__CFMTS(c) "assigning ximage %p\n", __CARGS(c), ximage);
			if ((px->pixmap.ximage = XSubImage(ximage, 0, 0,
							   ximage->width, ximage->height)))
				bi->present = True;
		} else {
			if (px->bitmap.ximage) {
				XDestroyImage(px->bitmap.ximage);
				px->bitmap.ximage = NULL;
			}
			XPRINTF(__CFMTS(c) "assigning ximage %p\n", __CARGS(c), ximage);
			if ((px->bitmap.ximage = XSubImage(ximage, 0, 0,
							   ximage->width, ximage->height)))
				bi->present = True;
		}
	}
	XDestroyImage(ximage);
	return (True);

      error:
	if (ximage)
		XDestroyImage(ximage);
	return (False);
}

Bool
ximage_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w,
			unsigned h)
{
	XImage *xicon = NULL, *xmask = NULL;
	Bool result = False;

	if (!(xicon = XGetImage(dpy, icon, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", icon, w, h);
		goto error;
	}
	if (mask && !(xmask = XGetImage(dpy, mask, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", mask, w, h);
		goto error;
	}
	result = ximage_createicon(ds, c, &xicon, &xmask, False);
      error:
	if (xmask)
		XDestroyImage(xmask);
	if (xicon)
		XDestroyImage(xicon);
	return (result);
}

Bool
ximage_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w,
			unsigned h, unsigned d)
{
	XImage *xicon = NULL, *xmask = NULL;
	Bool result = False;

	if (!(xicon = XGetImage(dpy, icon, 0, 0, w, h, AllPlanes, ZPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", icon, w, h);
		goto error;
	}
	if (mask && !(xmask = XGetImage(dpy, mask, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", mask, w, h);
		goto error;
	}
	result = ximage_createicon(ds, c, &xicon, &xmask, False);
      error:
	if (xmask)
		XDestroyImage(xmask);
	if (xicon)
		XDestroyImage(xicon);
	return (result);
}

Bool
ximage_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data)
{
	XImage *xicon = NULL;
	unsigned i, j;
	Bool result = False;

	if (!(xicon = XCreateImage(dpy, ds->visual, 32, ZPixmap, 0, NULL, w, h, 8, 0))) {
		EPRINTF("could not create image %ux%ux%u\n", w, h, 32);
		goto error;
	}
	xicon->data = emallocz(xicon->bytes_per_line * xicon->height);
	for (j = 0; j < h; j++)
		for (i = 0; i < w; i++, data++)
			XPutPixel(xicon, i, j, *data);
	result = ximage_createicon(ds, c, &xicon, NULL, False);
      error:
	if (xicon) {
		free(xicon->data);
		xicon->data = NULL;
		XDestroyImage(xicon);
	}
	return (result);
}

Bool
ximage_createpngicon(AScreen *ds, Client *c, const char *file)
{
	Bool result = False;

#ifdef LIBPNG
	XImage *xicon = NULL;

	if (!(xicon = png_read_file_to_ximage(dpy, ds->visual, file))) {
		EPRINTF("could not read png file %s\n", file);
		goto error;
	}
	result = ximage_createicon(ds, c, &xicon, NULL, True);
      error:
	if (xicon) {
		free(xicon->data);
		xicon->data = NULL;
		XDestroyImage(xicon);
	}
#endif				/* LIBPNG */
	return (result);
}

Bool
ximage_createjpgicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use XIMAGE to create JPG icon %s\n", file);
	/* for now */
	return (False);
}

Bool
ximage_createsvgicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use XIMAGE to create SVG icon %s\n", file);
	/* for now */
	return (False);
}

Bool
ximage_createxpmicon(AScreen *ds, Client *c, const char *file)
{
	Bool result = False;
#ifdef XPM
	XImage *xicon = NULL, *xmask = NULL;
	int status;

	{
		XpmAttributes xa = { 0, };

		xa.visual = DefaultVisual(dpy, ds->screen);
		xa.valuemask |= XpmVisual;
		xa.colormap = DefaultColormap(dpy, ds->screen);
		xa.valuemask |= XpmColormap;
		xa.depth = DefaultDepth(dpy, ds->screen);
		xa.valuemask |= XpmDepth;

		status = XpmReadFileToImage(dpy, file, &xicon, &xmask, &xa);
		if (status != XpmSuccess || !xicon) {
			EPRINTF("could not load xpm file: %s on %s\n", xpm_status_string(status), file);
			goto error;
		}
		XpmFreeAttributes(&xa);
	}
	result = ximage_createicon(ds, c, &xicon, &xmask, True);
      error:
	if (xmask)
		XDestroyImage(xmask);
	if (xicon)
		XDestroyImage(xicon);
#endif				/* XPM */
	return (result);
}

Bool
ximage_createxbmicon(AScreen *ds, Client *c, const char *file)
{
	XImage *xicon = NULL;
	unsigned w, h;
	int x, y, status;
	Bool result = False;

	status = XReadBitmapFileImage(dpy, ds->visual, file, &w, &h, &xicon, &x, &y);
	if (status != BitmapSuccess || !xicon) {
		EPRINTF("could not load xbm file: %s on %s\n", xbm_status_string(status), file);
		goto error;
	}
	result = ximage_createicon(ds, c, &xicon, &xicon, True);
      error:
	if (xicon)
		XDestroyImage(xicon);
	return (result);
}

#ifdef DAMAGE
Bool
ximage_drawdamage(Client *c, XDamageNotifyEvent * ev)
{
	return (False);
}
#endif

int
ximage_drawbutton(AScreen *ds, Client *c, ElementType type, XftColor *col, int x)
{
	ElementClient *ec = &c->element[type];
	Drawable d = ds->dc.draw.pixmap;
	XftColor *fg, *bg;
	Geometry g = { 0, };
	ButtonImage *bi;
	AdwmPixmap *px;
	int status, th = titleheight(ds);

	if (!(bi = buttonimage(ds, c, type)) || !bi->present) {
		XPRINTF("button %d has no button image\n", type);
		return 0;
	}
	px = &bi->px;

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

					R = min(255, lround(((A * rf) + ((255 - A) * rb)) / 255));
					G = min(255, lround(((A * gf) + ((255 - A) * gb)) / 255));
					B = min(255, lround(((A * bf) + ((255 - A) * bb)) / 255));

					A = 255;
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

					R = min(255, lround(((A * R) + ((255 - A) * rb)) / 255));
					G = min(255, lround(((A * G) + ((255 - A) * gb)) / 255));
					B = min(255, lround(((A * B) + ((255 - A) * bb)) / 255));

					A = 255;
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
	XPRINTF("button %d has no pixmap or bitmap\n", type);
	return 0;
}

int
ximage_drawsep(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw,
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

	if (g.w > mw)
		return (0);

	XSetForeground(dpy, ds->dc.gc, col->pixel);
	XDrawLine(dpy, drawable, ds->dc.gc, g.x, g.y, g.x, g.y + g.h);
	return (g.w);
}

int
ximage_drawtext(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw,
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
		XftDrawStringUtf8(xftdraw, &fcol[ColShadow], font, x + drop,
				  y + drop, (unsigned char *) buf, len);
	XftDrawStringUtf8(xftdraw, &fcol[ColFG], font, x, y, (unsigned char *) buf, len);
	return w + gap * 2;
}

void
ximage_drawdockapp(AScreen *ds, Client *c)
{
	int status;
	unsigned long pixel;

	pixel = getpixel(ds, c, ColBG);
	/* to avoid clearing window, initiallly set to norm[ColBG] */
	// XSetWindowBackground(dpy, c->frame, pixel);
	// XClearWindow(dpy, c->frame);
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
	XPRINTF(c, "Filled dockapp frame %dx%d+%d+%d\n", ds->dc.w, ds->dc.h, ds->dc.x,
		ds->dc.y);
	/* note that ParentRelative dockapps need the background set to the foregroudn */
	XSetWindowBackground(dpy, c->frame, pixel);
	/* following are quite disruptive - many dockapps ignore expose events and simply
	   update on timer */
	// XClearWindow(dpy, c->icon ? : c->win);
	// XClearArea(dpy, c->icon ? : c->win, 0, 0, 0, 0, True);
	/* so, instead of actually clearing the area, just send a synthetic Expose event
	   to the icon window.  Those docapps that don't response to expose events will
	   not, and those that do can clear themselves before updating.  */
	{
		XEvent xev = { 0, };

		xev.xexpose.type = Expose;
		xev.xexpose.send_event = False;
		xev.xexpose.display = dpy;
		xev.xexpose.window = c->icon ? : c->win;
		xev.xexpose.x = 0;
		xev.xexpose.y = 0;
		xev.xexpose.width = max(c->c.w, 56);
		xev.xexpose.height = max(c->c.h, 56);
		xev.xexpose.count = 0;

		XSendEvent(dpy, c->icon ? : c->win, False, ExposureMask, &xev);
	}
}

void
ximage_drawnormal(AScreen *ds, Client *c)
{
	size_t i;
	int status;
	unsigned long bg, bc;

	if (!c->title)
		return;

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

	bg = getpixel(ds, c, ColBG);
	bc = getpixel(ds, c, ColBorder);

	{
	xtrap_push(True,NULL);
	XSetForeground(dpy, ds->dc.gc, bg);
	XSetLineAttributes(dpy, ds->dc.gc, ds->style.border, LineSolid, CapNotLast, JoinMiter);
	XSetFillStyle(dpy, ds->dc.gc, FillSolid);
	status = XFillRectangle(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x, ds->dc.y, ds->dc.w, ds->dc.h);
	if (!status)
		XPRINTF("Could not fill rectangle, error %d\n", status);
	xtrap_pop();
	}

	/* Don't know about this... */
	if (ds->dc.w < textw(ds, c->name, gethilite(c))) {
		ds->dc.w -= elementw(ds, c, CloseBtn);
		ximage_drawtext(ds, c->name, ds->dc.draw.pixmap, ds->dc.draw.xft, gethues(ds, c), gethilite(c), ds->dc.x, ds->dc.y, ds->dc.w);
		ximage_drawbutton(ds, c, CloseBtn, gethues(ds, c), ds->dc.w);
		goto end;
	}
	/* Left */
	ds->dc.x += (ds->style.spacing > ds->style.border) ?
	    ds->style.spacing - ds->style.border : 0;
	for (i = 0; i < strlen(ds->style.titlelayout); i++) {
		if (ds->style.titlelayout[i] == ' ' || ds->style.titlelayout[i] == '-')
			break;
		ds->dc.x += drawelement(ds, ds->style.titlelayout[i], ds->dc.x, AlignLeft, c);
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
		XSetForeground(dpy, ds->dc.gc, bc);
		XPRINTF(__CFMTS(c) "drawing line from +%d+%d to +%d+%d\n", __CARGS(c), 0, ds->dc.h - 1, ds->dc.w, ds->dc.h - 1);
		xtrap_push(True,NULL);
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, 0, ds->dc.h - 1, ds->dc.w, ds->dc.h - 1);
		xtrap_pop();
	}
	XPRINTF(__CFMTS(c) "copying title pixmap to %dx%d+%d+%d to +%d+%d\n", __CARGS(c), c->c.w, ds->dc.h, 0, 0, 0, 0);
	XCopyArea(dpy, ds->dc.draw.pixmap, c->title, ds->dc.gc, 0, 0, c->c.w, ds->dc.h, 0, 0);

	if (!c->grips)
		return;

	ds->dc.x = ds->dc.y = 0;
	ds->dc.w = c->c.w;
	ds->dc.h = ds->style.gripsheight;

	{
	xtrap_push(True,NULL);
	XSetForeground(dpy, ds->dc.gc, bg);
	XSetLineAttributes(dpy, ds->dc.gc, ds->style.border, LineSolid, CapNotLast, JoinMiter);
	XSetFillStyle(dpy, ds->dc.gc, FillSolid);
	status = XFillRectangle(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x, ds->dc.y, ds->dc.w, ds->dc.h);
	if (!status)
		XPRINTF("Could not fill rectangle, error %d\n", status);
	xtrap_pop();
	}

	if (ds->style.outline) {
		XSetForeground(dpy, ds->dc.gc, bc);
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, 0, 0, ds->dc.w, 0);
		/* needs to be adjusted to do ds->style.gripswidth instead */
		ds->dc.x = ds->dc.w / 2 - ds->dc.w / 5;
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x, 0, ds->dc.x, ds->dc.h);
		ds->dc.x = ds->dc.w / 2 + ds->dc.w / 5;
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x, 0, ds->dc.x, ds->dc.h);
	}
	XCopyArea(dpy, ds->dc.draw.pixmap, c->grips, ds->dc.gc, 0, 0, c->c.w, ds->dc.h, 0, 0);
}

Bool
ximage_initpng(char *path, AdwmPixmap *px)
{
#ifdef LIBPNG
	/* for now */
	return (False);
#else				/* LIBPNG */
	return (False);
#endif				/* LIBPNG */
}

Bool
ximage_initjpg(char *path, AdwmPixmap *px)
{
#ifdef LIBJPEG
	/* for now */
	return (False);
#else				/* LIBJPEG */
	return (False);
#endif				/* LIBJPEG */
}

Bool
ximage_initsvg(char *path, AdwmPixmap *px)
{
	return (False);
}

Bool
ximage_initxpm(char *path, AdwmPixmap *px)
{
#ifdef XPM
	XImage *xdraw = NULL, *xmask = NULL, *ximage;
	int status;

	{
		XpmAttributes xa = { 0, };

		xa.visual = DefaultVisual(dpy, scr->screen);
		xa.valuemask |= XpmVisual;
		xa.colormap = DefaultColormap(dpy, scr->screen);
		xa.valuemask |= XpmColormap;
		xa.depth = DefaultDepth(dpy, scr->screen);
		xa.valuemask |= XpmDepth;

		status = XpmReadFileToImage(dpy, path, &xdraw, &xmask, &xa);
		if (status != XpmSuccess || !xdraw) {
			EPRINTF("could not load xpm file: %s on %s\n", xpm_status_string(status), path);
			return False;
		}
		XpmFreeAttributes(&xa);
	}
	ximage = combine_pixmap_and_mask(scr, xdraw, xmask);
	if (!ximage) {
		EPRINTF("could not combine images\n");
		goto error;
	}
	XDestroyImage(xdraw);
	if (xmask)
		XDestroyImage(xmask);

	px->x = px->y = px->b = 0;
	px->w = ximage->width;
	px->h = ximage->height;
	px->d = ximage->depth;
	if (px->pixmap.ximage) {
		XDestroyImage(px->pixmap.ximage);
		px->pixmap.ximage = NULL;
	}
	px->pixmap.ximage = ximage;
	px->file = path;
	return (True);
      error:
	XDestroyImage(xdraw);
	if (xmask)
		XDestroyImage(xmask);
#endif				/* !XPM */
	return (False);
}

Bool
ximage_initxbm(char *path, AdwmPixmap *px)
{
	XImage *xdraw = NULL, *ximage = NULL;
	int status;

	status = XReadBitmapFileImage(dpy, scr->visual, path, &px->w, &px->h, &xdraw, &px->x, &px->y);
	if (status != BitmapSuccess || !xdraw) {
		EPRINTF("could not load xbm file: %s on %s\n", xbm_status_string(status), path);
		goto error;
	}
	ximage = combine_pixmap_and_mask(scr, xdraw, xdraw);
	if (!ximage) {
		EPRINTF("could not combine images\n");
		goto error;
	}
	XDestroyImage(xdraw);
	px->x = px->y = px->b = 0;
	px->w = ximage->width;
	px->h = ximage->height;
	px->d = ximage->depth;
	if (px->bitmap.ximage) {
		XDestroyImage(px->bitmap.ximage);
		px->bitmap.ximage = NULL;
	}
	px->bitmap.ximage = ximage;
	px->file = path;
	return (True);

      error:
	if (ximage)
		XDestroyImage(ximage);
	if (xdraw)
		XDestroyImage(xdraw);
	return (False);
}

Bool
ximage_initxbmdata(const unsigned char *bits, int w, int h, AdwmPixmap *px)
{
	XImage *xdraw = NULL, *ximage = NULL;
	size_t len;

	if (!(xdraw = XCreateImage(dpy, scr->visual, 1, XYBitmap, 0, NULL, w, h, 8, 0))) {
		EPRINTF("could not create image\n");
		goto error;
	}
	len = xdraw->bytes_per_line * h;
	xdraw->data = emallocz(len);
	memcpy(xdraw->data, bits, len);
	ximage = combine_pixmap_and_mask(scr, xdraw, xdraw);
	if (!ximage) {
		EPRINTF("could not combine images\n");
		goto error;
	}
	XDestroyImage(xdraw);
	px->x = px->y = px->b = 0;
	px->w = ximage->width;
	px->h = ximage->height;
	px->d = ximage->depth;
	if (px->bitmap.ximage) {
		XDestroyImage(px->bitmap.ximage);
		px->bitmap.ximage = NULL;
	}
	px->bitmap.ximage = ximage;
	free(px->file);
	px->file = NULL;
	return (True);

      error:
	if (ximage)
		XDestroyImage(ximage);
	if (xdraw)
		XDestroyImage(xdraw);
	return (False);
}
