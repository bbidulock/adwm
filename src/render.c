/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "actions.h"
#include "config.h"
#include "icons.h"
#include "draw.h"
#if 1
#include "ximage.h"
#else
#include "xlib.h"
#endif
#include "render.h" /* verification */

#if defined RENDER
void
render_removebutton(ButtonImage *bi)
{
	if (bi->pixmap.pict) {
		XRenderFreePicture(dpy, bi->pixmap.pict);
		bi->pixmap.pict = None;
	}
	if (bi->bitmap.pict) {
		XRenderFreePicture(dpy, bi->bitmap.pict);
		bi->bitmap.pict = None;
	}
}

Bool
render_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h)
{
	XImage *xicon = NULL, *xmask = NULL, *alpha;
	ButtonImage *bi, **bis;
	unsigned i, j, d = ds->depth, th = ds->style.titleheight;
	unsigned long valuemask = 0;
	Pixmap pixmap;
	Picture pict;
	XRenderPictureAttributes pa = { 0, };

	if (!(xicon = XGetImage(dpy, icon, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", icon, w, h);
		return (False);
	}
	if (mask && !(xmask = XGetImage(dpy, mask, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", mask, w, h);
		XDestroyImage(xicon);
		return (False);
	}
	if (mask && !(alpha = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, w, h, 8, 0))) {
		EPRINTF("could not create ximage %ux%ux%u\n", w, h, d);
		XDestroyImage(xicon);
		XDestroyImage(xmask);
		return (False);
	}
	if (alpha && !(alpha->data = emallocz(alpha->bytes_per_line * alpha->height))) {
		EPRINTF("could not allocate ximage data %ux%ux%u\n", w, h, d);
		XDestroyImage(xicon);
		XDestroyImage(xmask);
		XDestroyImage(alpha);
		return (False);
	}
	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			if (XGetPixel(xicon, i, j)) {
				XPutPixel(alpha, i, j, 0xffffffff);
			} else {
				XPutPixel(alpha, i, j, 0xff000000);
			}
			if (xmask && j < xmask->height && i < xmask->width && XGetPixel(xmask, i, j)) {
				XPutPixel(alpha, i, j,
					  XGetPixel(alpha, i, j) | 0xff000000);
			} else {
				XPutPixel(alpha, i, j,
					  XGetPixel(alpha, i, j) & 0x00ffffff);
			}
		}
	}
	XDestroyImage(xicon);
	if (xmask)
		XDestroyImage(xmask);
	pixmap = XCreatePixmap(dpy, ds->drawable, w, h, d);
	XPutImage(dpy, pixmap, ds->dc.gc, alpha, 0, 0, 0, 0, w, h);
	XDestroyImage(alpha);

	pa.repeat = RepeatNone;
	valuemask |= CPRepeat;
	pa.poly_edge = PolyEdgeSmooth;
	valuemask |= CPPolyEdge;
	pa.component_alpha = True;
	valuemask |= CPComponentAlpha;
	pict = XRenderCreatePicture(dpy, pixmap, ds->format, valuemask, &pa);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		bi = *bis;
		bi->x = bi->y = bi->b = 0;
		bi->d = d;
		bi->w = w;
		bi->h = h;
		if (h > th) {
			XDouble scale = (XDouble) th / (XDouble) h;
			XTransform trans = {
				{ { XDoubleToFixed(scale), 0, 0 },
				  { 0, XDoubleToFixed(scale), 0 },
				  { 0, 0, XDoubleToFixed(1.0)   } }
			};
			XRenderSetPictureTransform(dpy, pict, &trans);
			XRenderSetPictureFilter(dpy, pict, FilterBilinear, NULL, 0);
			bi->w = w * scale;
			bi->h = h * scale;
		}
		if (bi->bitmap.pict) {
			XRenderFreePicture(dpy, bi->bitmap.pict);
			bi->bitmap.pict = None;
		}
		bi->bitmap.pict = pict;
		bi->present = True;
	}
	return (True);
}

Bool
render_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h, unsigned d)
{
	XImage *xicon = NULL, *xmask = NULL, *alpha;
	ButtonImage *bi, **bis;
	unsigned i, j, th = ds->style.titleheight;
	unsigned long valuemask = 0, pixel;
	Pixmap pixmap;
	Picture pict;
	XRenderPictureAttributes pa = { 0, };

	if (!(xicon = XGetImage(dpy, icon, 0, 0, w, h, DefaultDepth(dpy, ds->screen), ZPixmap))) {
		EPRINTF("could not get pixmap 0x%lx %ux%u\n", icon, w, h);
		return (False);
	}
	if (mask && !(xmask = XGetImage(dpy, mask, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", mask, w, h);
		return (False);
	}
	if (mask && !(alpha = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, w, h, 8, 0))) {
		EPRINTF("could not create ximage %ux%ux%u\n", w, h, d);
		XDestroyImage(xicon);
		XDestroyImage(xmask);
		return (False);
	}
	if (alpha && !(alpha->data = emallocz(alpha->bytes_per_line * alpha->height))) {
		EPRINTF("could not allocate ximage data %ux%ux%u\n", w, h, d);
		XDestroyImage(xicon);
		XDestroyImage(xmask);
		XDestroyImage(alpha);
		return (False);
	}
	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			pixel = XGetPixel(xicon, i, j);
			if (xmask && j < xmask->height && i < xmask->width && XGetPixel(xmask, i, j)) {
				XPutPixel(alpha, i, j, pixel | 0xff000000);
			} else {
				XPutPixel(alpha, i, j, pixel & 0x00ffffff);
			}
		}
	}
	XDestroyImage(xicon);
	if (xmask)
		XDestroyImage(xmask);
	pixmap = XCreatePixmap(dpy, ds->drawable, w, h, ds->depth);
	XPutImage(dpy, pixmap, ds->dc.gc, alpha, 0, 0, 0, 0, w, h);
	XDestroyImage(alpha);

	pa.repeat = RepeatNone;
	valuemask |= CPRepeat;
	pa.poly_edge = PolyEdgeSmooth;
	valuemask |= CPPolyEdge;
	pa.component_alpha = True;
	valuemask |= CPComponentAlpha;
	pict = XRenderCreatePicture(dpy, pixmap, ds->format, valuemask, &pa);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		bi = *bis;
		bi->x = bi->y = bi->b = 0;
		bi->d = ds->depth;
		bi->w = w;
		bi->h = h;
		if (h > th) {
			/* get XRender to scale the image for us */
			XDouble scale = (XDouble) th / (XDouble) h;
			XTransform trans = {
				{ { XDoubleToFixed(scale), 0, 0 },
				  { 0, XDoubleToFixed(scale), 0 },
				  { 0, 0, XDoubleToFixed(1.0)   } }
			};
			XRenderSetPictureTransform(dpy, pict, &trans);
			XRenderSetPictureFilter(dpy, pict, FilterBilinear, NULL, 0);
			bi->w = w * scale;
			bi->h = h * scale;
		}
		if (bi->pixmap.pict) {
			XRenderFreePicture(dpy, bi->pixmap.pict);
			bi->pixmap.pict = None;
		}
		bi->pixmap.pict = pict;
		bi->present = True;
	}
	return (True);
}

Bool
render_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data)
{
	XImage *alpha;
	ButtonImage *bi, **bis;
	unsigned i, j, d = ds->depth, th = ds->style.titleheight;
	unsigned long mask = 0;
	long *p;
	Pixmap pixmap;
	Picture pict;
	XRenderPictureAttributes pa = { 0, };

	if (!(alpha = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, w, h, 8, 0))) {
		EPRINTF("could not create ximage %ux%ux%u\n", w, h, d);
		return (False);
	}
	for (p = data, j = 0; j < h; j++)
		for (i = 0; i < w; i++, p++)
			XPutPixel(alpha, i, j, *p);

	pixmap = XCreatePixmap(dpy, ds->drawable, w, h, d);
	XPutImage(dpy, pixmap, ds->dc.gc, alpha, 0, 0, 0, 0, w, h);
	XDestroyImage(alpha);

	pa.repeat = RepeatNone;
	mask |= CPRepeat;
	pa.poly_edge = PolyEdgeSmooth;
	mask |= CPPolyEdge;
	pa.component_alpha = True;
	mask |= CPComponentAlpha;
	pict = XRenderCreatePicture(dpy, pixmap, ds->format, mask, &pa);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		bi = *bis;
		bi->x = bi->y = bi->b = 0;
		bi->d = d;
		bi->w = w;
		bi->h = h;
		if (h > th) {
			/* get XRender to scale the image for us */
			XDouble scale = (XDouble) th / (XDouble) h;
			XTransform trans = {
				{ { XDoubleToFixed(scale), 0, 0 },
				  { 0, XDoubleToFixed(scale), 0 },
				  { 0, 0, XDoubleToFixed(1.0)   } }
			};
			XRenderSetPictureTransform(dpy, pict, &trans);
			XRenderSetPictureFilter(dpy, pict, FilterBilinear, NULL, 0);
			bi->w = w * scale;
			bi->h = h * scale;
		}
		if (bi->pixmap.pict) {
			XRenderFreePicture(dpy, bi->pixmap.pict);
			bi->pixmap.pict = None;
		}
		bi->pixmap.pict = pict;
		bi->present = True;
	}
	return (True);
}

Bool
render_createpngicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use RENDER to create PNG icon %s\n", file);
	/* for now */
	return (False);
}

Bool
render_createsvgicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use RENDER to create SVG icon %s\n", file);
	/* for now */
	return (False);
}

Bool
render_createxpmicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use RENDER to create XPM icon %s\n", file);
	/* for now */
	return (False);
}

Bool
render_createxbmicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use RENDER to create XBM icon %s\n", file);
	/* for now */
	return (False);
}

#ifdef DAMAGE
Bool
render_drawdamage(Client *c, XDamageNotifyEvent * ev)
{
	return (False);
}
#endif

int
render_drawbutton(AScreen *ds, Client *c, ElementType type, XftColor *col, int x)
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

	th = ds->dc.h;
	if (ds->style.outline)
		th--;

	/* geometry of the container */
	g.x = x;
	g.y = 0;
	g.w = max(th, bi->w);
	g.h = th;

	/* element client geometry used to detect element under pointer */
	ec->eg.x = g.x + (g.w - bi->w) / 2;
	ec->eg.y = g.y + (g.h - bi->h) / 2;
	ec->eg.w = bi->w;
	ec->eg.h = bi->h;

	fg = ec->pressed ? &col[ColFG] : &col[ColButton];
	bg = bi->bg.pixel ? &bi->bg : &col[ColBG];

	if (bi->bitmap.ximage) {
		XImage *ximage;

		if ((ximage = XSubImage(bi->bitmap.ximage, 0, 0,
					bi->bitmap.ximage->width,
					bi->bitmap.ximage->height))) {
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
					bi->w, bi->h, bi->d, bi->x, bi->y, ec->eg.x, ec->eg.y);
			XPutImage(dpy, d, ds->dc.gc, ximage, bi->x, bi->y, ec->eg.x, ec->eg.y, bi->w, bi->h);
			XDestroyImage(ximage);
			return g.w;
		}
	}
	if (bi->pixmap.ximage) {
		XImage *ximage;

		if ((ximage = XSubImage(bi->pixmap.ximage, 0, 0,
					bi->pixmap.ximage->width,
					bi->pixmap.ximage->height))) {
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
					bi->w, bi->h, bi->d, bi->x, bi->y, ec->eg.x, ec->eg.y);
			XPutImage(dpy, d, ds->dc.gc, ximage, bi->x, bi->y, ec->eg.x, ec->eg.y, bi->w, bi->h);
			XDestroyImage(ximage);
			return g.w;
		}
	}
#if defined RENDER && defined USE_RENDER
	if (bi->pixmap.pict) {
		Picture dst = XftDrawPicture(ds->dc.draw.xft);

		XRenderFillRectangle(dpy, PictOpOver, dst, &bg->color, g.x, g.y, g.w, g.h);
		XRenderComposite(dpy, PictOpOver, bi->pixmap.pict, None, dst,
				0, 0, 0, 0, ec->eg.x, ec->eg.y, ec->eg.w, ec->eg.h);
	} else
#endif
#if defined IMLIB2
	if (bi->pixmap.image) {
		Imlib_Image image;

		imlib_context_push(scr->context);
		imlib_context_set_image(bi->pixmap.image);
		imlib_context_set_mask(None);
		image = imlib_create_image(ec->eg.w, ec->eg.h);
		imlib_context_set_image(image);
		imlib_context_set_mask(None);
		imlib_context_set_color(bg->color.red, bg->color.green, bg->color.blue, 255);
		imlib_image_fill_rectangle(0, 0, ec->eg.w, ec->eg.h);
		imlib_context_set_blend(0);
		imlib_context_set_anti_alias(1);
		imlib_blend_image_onto_image(bi->pixmap.image, False,
				0, 0, bi->w, bi->h,
				0, 0, ec->eg.w, ec->eg.h);
		imlib_context_set_drawable(d);
		imlib_render_image_on_drawable(ec->eg.x, ec->eg.y);
		imlib_free_image();
		imlib_context_pop();
		return g.w;
	} else
	if (bi->bitmap.image) {
		Imlib_Image image, mask;

		imlib_context_push(scr->context);
		imlib_context_set_image(bi->bitmap.image);
		imlib_context_set_mask(None);
		image = imlib_create_image(ec->eg.w, ec->eg.h);
		imlib_context_set_image(image);
		imlib_context_set_mask(None);
		imlib_context_set_color(bg->color.red, bg->color.green, bg->color.blue, 255);
		imlib_image_fill_rectangle(0, 0, ec->eg.w, ec->eg.h);
		mask = imlib_create_image(bi->w, bi->h);
		imlib_context_set_image(mask);
		imlib_context_set_mask(None);
		imlib_context_set_color(fg->color.red, fg->color.green, fg->color.blue, 255);
		imlib_image_fill_rectangle(0, 0, bi->w, bi->h);
		imlib_image_copy_alpha_to_image(bi->bitmap.image, 0, 0);
		imlib_context_set_image(image);
		imlib_context_set_mask(None);
		imlib_context_set_blend(0);
		imlib_context_set_anti_alias(1);
		imlib_blend_image_onto_image(mask, False,
				0, 0, bi->w, bi->h,
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
	if (bi->pixmap.draw) {
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
		XSetClipOrigin(dpy, ds->dc.gc, ec->eg.x - bi->x, ec->eg.y - bi->y);

		XPRINTF("Copying pixmap 0x%lx mask 0x%lx with geom %dx%d+%d+%d to drawable 0x%lx\n",
		        bi->pixmap.draw, bi->pixmap.mask, ec->eg.w, ec->eg.h, ec->eg.x, ec->eg.y, d);
		XSetClipMask(dpy, ds->dc.gc, bi->pixmap.mask);
		XPRINTF(__CFMTS(c) "copying pixmap %dx%dx%d+%d+%d to +%d+%d\n", __CARGS(c),
				bi->w, bi->h, bi->d, bi->x, bi->y, ec->eg.x, ec->eg.y);
		XCopyArea(dpy, bi->pixmap.draw, d, ds->dc.gc,
			  bi->x, bi->y, bi->w, bi->h, ec->eg.x, ec->eg.y);
		XSetClipMask(dpy, ds->dc.gc, None);
		return g.w;
	} else
	if (bi->bitmap.draw) {
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
		XSetClipOrigin(dpy, ds->dc.gc, ec->eg.x - bi->x, ec->eg.y - bi->y);

		XSetClipMask(dpy, ds->dc.gc, bi->bitmap.mask ? : bi->bitmap.draw);
		XPRINTF(__CFMTS(c) "copying bitmap %dx%dx%d+%d+%d to +%d+%d\n", __CARGS(c),
				bi->w, bi->h, bi->d, bi->x, bi->y, ec->eg.x, ec->eg.y);
		XCopyPlane(dpy, bi->bitmap.draw, d, ds->dc.gc,
			   bi->x, bi->y, bi->w, bi->h, ec->eg.x, ec->eg.y, 1);
		XSetClipMask(dpy, ds->dc.gc, None);
		return g.w;
	}
	XPRINTF("button %d has no pixmap or bitmap\n", type);
	return 0;
}

int
render_drawtext(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw,
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

#if defined RENDER && defined USE_RENDER
	Picture dst = XftDrawPicture(xftdraw);
	Picture src = XftDrawSrcPicture(xftdraw, &fcol[ColFG]);
	Picture shd = XftDrawSrcPicture(xftdraw, &fcol[ColShadow]);

	if (dst && src) {
		/* xrender */
		XRenderFillRectangle(dpy, PictOpSrc, dst, &col[ColBG].color, x - gap, 0,
				     w + gap * 2, h);
		if (drop)
			XftTextRenderUtf8(dpy, PictOpOver, shd, font, dst, 0, 0, x + drop,
					  y + drop, (FcChar8 *) buf, len);
		XftTextRenderUtf8(dpy, PictOpOver, src, font, dst, 0, 0, x + drop,
				  y + drop, (FcChar8 *) buf, len);
	} else
#endif
	{
		/* non-xrender */
		XSetForeground(dpy, ds->dc.gc, col[ColBG].pixel);
		XSetFillStyle(dpy, ds->dc.gc, FillSolid);
		status =
		    XFillRectangle(dpy, drawable, ds->dc.gc, x - gap, 0, w + gap * 2, h);
		if (!status)
			XPRINTF("Could not fill rectangle, error %d\n", status);
		if (drop)
			XftDrawStringUtf8(xftdraw, &fcol[ColShadow], font, x + drop,
					  y + drop, (unsigned char *) buf, len);
		XftDrawStringUtf8(xftdraw, &fcol[ColFG], font, x, y,
				  (unsigned char *) buf, len);
	}
	return w + gap * 2;
}

int
render_drawsep(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int mw)
{
	return ximage_drawsep(ds, text, drawable, xftdraw, col, hilite, x, y, mw);
}

void
render_drawdockapp(AScreen *ds, Client *c)
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
render_drawnormal(AScreen *ds, Client *c)
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
		render_drawtext(ds, c->name, ds->dc.draw.pixmap, ds->dc.draw.xft,
				gethues(ds, c), gethilite(c), ds->dc.x, ds->dc.y, ds->dc.w);
		render_drawbutton(ds, c, CloseBtn, gethues(ds, c), ds->dc.w);
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
render_initpng(char *path, ButtonImage *bi)
{
	return (False);
}

Bool
render_initsvg(char *path, ButtonImage *bi)
{
	return (False);
}

Bool
render_initxpm(char *path, ButtonImage *bi)
{
	XImage *xicon = NULL, *xmask = NULL, *alpha;
	XpmAttributes xa = { 0, };
	unsigned long pixel;
	XRenderPictureAttributes pa = { 0, };
	unsigned long mask = 0;
	unsigned w, h, d = scr->depth;
	int status, i, j;
	Pixmap pixmap;
	Picture pict;

	xa.visual = scr->visual;
	xa.valuemask |= XpmVisual;
	xa.colormap = scr->colormap;
	xa.valuemask |= XpmColormap;
	xa.depth = scr->depth;
	xa.valuemask |= XpmDepth;

	status = XpmReadFileToImage(dpy, path, &xicon, &xmask, &xa);
	if (status != Success || !xicon) {
		EPRINTF("could not load xpm file %s\n", path);
		return False;
	}
	w = xa.width;
	h = xa.height;

	if (!(alpha = XCreateImage(dpy, scr->visual, d, ZPixmap, 0, NULL, w, h, 8, 0))) {
		EPRINTF("could not create ximage %ux%ux%u\n", w, h, d);
		XDestroyImage(xicon);
		XDestroyImage(xmask);
		return (False);
	}
	for (j = 0; j < xicon->height; j++) {
		for (i = 0; i < xicon->width; i++) {
			pixel = XGetPixel(xicon, i, j);
			if (xmask && j < xmask->height && i < xmask->width && XGetPixel(xmask, i, j)) {
				XPutPixel(xicon, i, j, pixel | 0xff000000);
			} else {
				XPutPixel(xicon, i, j, pixel & 0x00ffffff);
			}

		}
	}
	XDestroyImage(xicon);
	if (xmask)
		XDestroyImage(xmask);
	pixmap = XCreatePixmap(dpy, scr->drawable, w, h, d);
	XPutImage(dpy, pixmap, scr->dc.gc, alpha, 0, 0, 0, 0, w, h);
	XDestroyImage(alpha);

	pa.repeat = RepeatNone;
	mask |= CPRepeat;
	pa.poly_edge = PolyEdgeSmooth;
	mask |= CPPolyEdge;
	pa.component_alpha = True;
	mask |= CPComponentAlpha;
	pict = XRenderCreatePicture(dpy, pixmap, scr->format, mask, &pa);

	if (bi->pixmap.pict) {
		XRenderFreePicture(dpy, bi->pixmap.pict);
		bi->pixmap.pict = None;
	}
	bi->pixmap.pict = pict;
	bi->w = xa.width;
	bi->h = xa.height;
	if (bi->h > scr->style.titleheight) {
		/* read lower down into image to clip top and bottom by same amount */
		bi->y += (bi->h - scr->style.titleheight) / 2;
		bi->h = scr->style.titleheight;
	}
	free(path);
	return True;
}

Bool
render_initxbm(char *path, ButtonImage *bi)
{
	return ximage_initxbm(path, bi);
}

#endif				/* defined RENDER */

