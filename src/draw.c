#include <math.h>
#include <regex.h>
#include <ctype.h>
#include <assert.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif
#include "adwm.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "actions.h"
#include "config.h"
#include "draw.h" /* verification */

#if defined IMLIB2 && defined USE_IMLIB2
static Bool
imlib_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h)
{
	Imlib_Image image;
	ButtonImage *bi;
	DATA32 *pixels, *p;
	XImage *xicon, *xmask = NULL;
	unsigned i, j, d = ds->depth , th = ds->style.titleheight, tw;

	XPRINTF("creating bitmap icon 0x%lx mask 0x%lx at %ux%ux%u\n", icon, mask, w, h, 1U);
	if (h <= th + 2) {
		bi = &c->iconbtn;
		bi->x = bi->y = bi->b = 0;
		bi->d = 1;
		bi->w = w;
		bi->h = h;
		if (bi->h > th) {
			/* read lower down into image to clip top and bottom by same
			   amount */
			bi->y += (bi->h - th) / 2;
			bi->h = th;
		}
		bi->bitmap.draw = icon;
		bi->bitmap.mask = mask;
		bi->present = True;
		return (True);
	}
	tw = lround((double)((double)((double) w/(double) h) * (double)th));
	XPRINTF("scaling bitmap icon 0x%lx mask 0x%lx from %ux%ux%u to %ux%ux%u \n", icon, mask, w, h, 1U, tw, th, d);
	/* need to scale: do it as 32-bit ARGB white/black visual */
	if (!(xicon = XGetImage(dpy, icon, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", icon, w, h);
		return (False);
	}
	if (mask && !(xmask = XGetImage(dpy, mask, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", icon, w, h);
		XDestroyImage(xicon);
		return (False);
	}
	imlib_context_push(ds->context);
	image = imlib_create_image(w, h);
	if (!image) {
		EPRINTF("could not create image %ux%u\n", w, h);
		imlib_context_pop();
		XDestroyImage(xicon);
		if (xmask)
			XDestroyImage(xmask);
		return (False);
	}
	imlib_context_set_image(image);
	imlib_context_set_mask(None);
	pixels = imlib_image_get_data();
	if (!pixels) {
		EPRINTF("could not get image data\n");
		imlib_free_image();
		imlib_context_pop();
		XDestroyImage(xicon);
		if (xmask)
			XDestroyImage(xmask);
		return (False);
	}
	for (p = pixels, i = 0; i < h; i++) {
		for (j = 0; j < w; j++, p++) {
			/* treat anything outside the bitmap as zero */
			if (i <= xicon->height && j <= xicon->width
			    && XGetPixel(xicon, j, i)) {
				/* a one in the bitmap */
				*p = 0xffffffff;	/* opaque white */
			} else {
				/* a zero in the bitmap */
				*p = 0xff000000;	/* opaque black */
			}
			if (xmask && i <= xmask->height && j <= xmask->width
					&& XGetPixel(xmask, j, i)) {
				/* a one in the bitmap */
				*p |= 0xff000000;	/* opaque */
			} else {
				/* a zero in the bitmap */
				*p &= 0x00ffffff;	/* transparent */
			}
		}
	}
	XDestroyImage(xicon);
	if (xmask)
		XDestroyImage(xmask);
	imlib_image_put_back_data(pixels);
	imlib_image_set_has_alpha(1);
	if (h > th) {
		Imlib_Image scaled =
			imlib_create_cropped_scaled_image(0, 0, w, h, tw, th);

		imlib_free_image();
		if (!scaled) {
			EPRINTF("could not scale image %ux%ux%u to %ux%ux%u\n", w, h, 1U, tw, th, d);
			imlib_context_pop();
			return (False);
		}
		imlib_context_set_image(scaled);
		imlib_context_set_mask(None);
		image = scaled;
		w = tw;
		h = th;
	}
	bi = &c->iconbtn;
	bi->x = bi->y = bi->b = 0;
	bi->d = d;
	bi->w = w;
	bi->h = h;
	if (bi->pixmap.draw) {
		imlib_free_pixmap_and_mask(bi->pixmap.draw);
		bi->pixmap.draw = None;
		bi->pixmap.mask = None;
	}
	imlib_render_pixmaps_for_whole_image(&bi->pixmap.draw, &bi->pixmap.mask);
	imlib_free_image();
	bi->present = True;
	imlib_context_pop();
	return True;
}
static Bool
imlib_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h, unsigned d)
{
	Imlib_Image image;
	ButtonImage *bi;
	unsigned th = ds->style.titleheight, tw;

	XPRINTF("creating pixmap icon 0x%lx mask 0x%lx at %ux%ux%u\n", icon, mask, w, h, d);
	if (d == ds->depth && h <= th + 2) {
		bi = &c->iconbtn;
		bi->x = bi->y = bi->b = 0;
		bi->d = 1;
		bi->w = w;
		bi->h = h;
		if (bi->h > th) {
			/* read lower down into image to clip top and bottom by same
			   amount */
			bi->y += (bi->h - th) / 2;
			bi->h = th;
		}
		bi->pixmap.draw = icon;
		bi->pixmap.mask = mask;
		bi->present = True;
		return (True);
	}
	tw = lround((double)((double)((double) w/(double) h) * (double)th));
	XPRINTF("scaling pixmap icon 0x%lx mask 0x%lx from %ux%ux%u to %ux%ux%u \n", icon, mask, w, h, d, tw, th, (unsigned) ds->depth);
	imlib_context_push(ds->context);
	imlib_context_set_drawable(None);
	if (d == DefaultDepth(dpy, ds->screen)) {
		imlib_context_set_visual(DefaultVisual(dpy, ds->screen));
		imlib_context_set_colormap(DefaultColormap(dpy, ds->screen));
		imlib_context_set_drawable(ds->root);
	} else if (d == ds->depth) {
		imlib_context_set_visual(ds->visual);
		imlib_context_set_colormap(ds->colormap);
		imlib_context_set_drawable(ds->drawable);
	} else {
		EPRINTF("Unexpected visual depth for pixmap 0x%lx: %ux%ux%u\n", icon, w, h, d);
		imlib_context_pop();
		return (False);
	}
	imlib_context_set_drawable(icon);

	image = imlib_create_scaled_image_from_drawable(mask, 0, 0, w, h, tw, th, 1, 0);
	imlib_context_set_drawable(None);
	imlib_context_set_visual(ds->visual);
	imlib_context_set_colormap(ds->colormap);
	imlib_context_set_drawable(ds->drawable);
	if (!image) {
		EPRINTF("could not load pixmap 0x%lx mask 0x%lx\n", icon, mask);
		imlib_context_pop();
		return (False);
	}
	imlib_context_set_image(image);
	imlib_context_set_mask(None);
	bi = &c->iconbtn;
	bi->x = bi->y = bi->b = 0;
	bi->d = ds->depth;
	bi->w = tw;
	bi->h = th;
	imlib_context_set_image(image);
	imlib_context_set_mask(None);
	if (bi->pixmap.draw) {
		imlib_free_pixmap_and_mask(bi->pixmap.draw);
		bi->pixmap.draw = None;
		bi->pixmap.mask = None;
	}
	imlib_render_pixmaps_for_whole_image(&bi->pixmap.draw, &bi->pixmap.mask);
	imlib_free_image();
	bi->present = True;
	imlib_context_pop();
	return (True);
}
static Bool
imlib_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data)
{
	Imlib_Image image;
	ButtonImage *bi;
	DATA32 *pixels;
	unsigned i, z;

	imlib_context_push(ds->context);
	image = imlib_create_image(w, h);
	if (!image) {
		EPRINTF("could not create image %ux%u\n", w, h);
		imlib_context_pop();
		return (False);
	}
	imlib_context_set_image(image);
	imlib_context_set_mask(None);
	pixels = imlib_image_get_data();
	if (!pixels) {
		EPRINTF("could not get image data\n");
		imlib_free_image();
		imlib_context_pop();
		return (False);
	}
	for (i = 0, z = w * h; i < z; i++)
		pixels[i] = data[i];
	imlib_image_put_back_data(pixels);
	imlib_image_set_has_alpha(1);
	if (h > ds->style.titleheight) {
		unsigned hs = ds->style.titleheight, ws = hs;

		Imlib_Image scaled =
		    imlib_create_cropped_scaled_image(0, 0, w, h, ws, hs);

		imlib_free_image();
		if (!scaled) {
			EPRINTF("could not scale image %ux%u to %dx%d\n", w, h, ws, hs);
			imlib_context_pop();
			return (False);
		}
		imlib_context_set_image(scaled);
		imlib_context_set_mask(None);
		image = scaled;
		w = ws;
		h = hs;
	}
	bi = &c->iconbtn;
	bi->x = bi->y = bi->b = 0;
	bi->d = ds->depth;
	bi->w = w;
	bi->h = h;
	if (bi->pixmap.draw) {
		imlib_free_pixmap_and_mask(bi->pixmap.draw);
		bi->pixmap.draw = None;
		bi->pixmap.mask = None;
	}
	imlib_render_pixmaps_for_whole_image(&bi->pixmap.draw, &bi->pixmap.mask);
	imlib_free_image();
	bi->present = True;
	imlib_context_pop();
	return True;
}
#endif				/* defined IMLIB2 && defined USE_IMLIB2 */

#if defined PIXBUF && defined USE_PIXBUF
static Bool
gdk_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask)
{
	GdkPixbuf *gicon = NULL, *gmask = NULL;
	Window root;
	int x, y;
	unsigned int w, h, b, d;
	ButtonImage *bi;

	if (!c || !icon)
		return (False);
	if (!XGetGeometry(dpy, icon, &root, &x, &y, &w, &h, &b, &d))
		return (False);
	if (icon)
		gicon = gdk_pixbuf_xlib_get_from_drawable(NULL, icon,
							 DefaultColormap(dpy, ds->screen),
							 DefaultVisual(dpy, ds->screen),
							 0, 0, 0, 0, w, h);
	if (!gicon) {
		EPRINTF("could not get pixbuf from drawable\n");
		return (False);
	}
	if (mask)
		gmask = gdk_pixbuf_xlib_get_from_drawable(NULL, mask,
							 DefaultColormap(dpy, ds->screen),
							 DefaultVisual(dpy, ds->screen),
							 0, 0, 0, 0, w, h);
	if (gmask) {
		GdkPixbuf *alpha;
		guchar *src, *dst;
		int src_stride, dst_stride, i, j;

		alpha = gdk_pixbuf_add_alpha(gicon, FALSE, 0, 0, 0);
		dst = gdk_pixbuf_get_pixels(alpha);
		src = gdk_pixbuf_get_pixels(gmask);
		dst_stride = gdk_pixbuf_get_rowstride(alpha);
		src_stride = gdk_pixbuf_get_rowstride(gmask);
		for (i = 0; i < h; i++) {
			for (j = 0; j < w; j++) {
				guchar *s = src + i * src_stride + j * 3;
				guchar *d = dst + i * dst_stride + j * 4;

				if (s[0] == 0)
					d[3] = 0;	/* transparent */
				else
					d[3] = 255;	/* opaque */
			}
		}
		g_object_unref(G_OBJECT(gicon));
		gicon = alpha;
		g_object_unref(G_OBJECT(gmask));
		gmask = NULL;
		if (!gicon) {
			EPRINTF("could not mask pixbuf\n");
			return (False);
		}
	}
	if (h > ds->style.titleheight + 2) {
		GdkPixbuf *scaled;

		w = ds->style.titleheight;
		h = ds->style.titleheight;
		scaled = gdk_pixbuf_scale_simple(gicon, w, h, GDK_INTERP_BILINEAR);
		g_object_unref(G_OBJECT(gicon));
		gicon = scaled;
		if (!gicon) {
			EPRINTF("could not scale pixbuf\n");
			return (False);
		}
	}
	bi = &c->iconbtn;
	bi->x = bi->y = bi->b = 0;
	bi->d = ds->depth;
	bi->w = w;
	bi->h = h;
	if (bi->h > ds->style.titleheight) {
		/* read lower down into image to clip top and bottom by same
		   amount */
		bi->y += (bi->h - ds->style.titleheight) / 2;
		bi->h = ds->style.titleheight;
	}
	if (bi->pixmap.draw) {
		XFreePixmap(dpy, bi->pixmap.draw);
		bi->pixmap.draw = None;
	}
	if (bi->pixmap.mask) {
		XFreePixmap(dpy, bi->pixmap.mask);
		bi->pixmap.mask = None;
	}
	gdk_pixbuf_xlib_render_pixmap_and_mask(gicon,
					       &bi->pixmap.draw,
					       &bi->pixmap.mask, 128);
	if (!bi->pixmap.draw) {
		EPRINTF("could not render pixbuf\n");
		return (False);
	}
	bi->present = True;
	return True;
}
static Bool
gdk_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data)
{
	return (False);
}
#endif				/* defined PIXBUF && defined USE_PIXBUF */

#if defined RENDER && defined USE_RENDER
static Bool
xrender_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h)
{
	XImage *xicon = NULL, *xmask = NULL, *alpha;
	ButtonImage *bi;
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

	bi = &c->iconbtn;
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
	return (True);
}
static Bool
xrender_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h, unsigned d)
{
	XImage *xicon = NULL, *xmask = NULL, *alpha;
	ButtonImage *bi;
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

	bi = &c->iconbtn;
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
	return (True);
}
static Bool
xrender_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data)
{
	XImage *alpha;
	ButtonImage *bi;
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

	bi = &c->iconbtn;
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
	return (True);
}
#endif				/* defined RENDER && defined USE_RENDER */

#if 1
static Bool
ximage_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h)
{
	XImage *xicon = NULL, *xmask = NULL, *ximage = NULL;
	ButtonImage *bi;
	unsigned i, j, k, l, d = ds->depth, th = ds->style.titleheight;
	unsigned long pixel;
	double *chanls = NULL; unsigned m; /* we all float down here... */
	double *counts = NULL; unsigned n;
	double *colors = NULL;

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
						XPutPixel(ximage, i, j, 0xFFFFFFFF);
					} else {
						XPutPixel(ximage, i, j, 0xFF000000);
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

	bi = &c->iconbtn;
	bi->x = bi->y = bi->b = 0;
	bi->d = ds->depth;
	bi->w = w;
	bi->h = h;
	if (bi->bitmap.ximage) {
		XDestroyImage(bi->bitmap.ximage);
		bi->bitmap.ximage = NULL;
	}
	XPRINTF(__CFMTS(c) "assigning ximage %p\n", __CARGS(c), ximage);
	bi->bitmap.ximage = ximage;
	bi->present = True;
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
static Bool
ximage_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h, unsigned d)
{
	XImage *xicon = NULL, *xmask = NULL, *ximage = NULL;
	ButtonImage *bi;
	unsigned i, j, k, l, th = ds->style.titleheight;
	unsigned long pixel;
	double *chanls = NULL; unsigned m;
	double *counts = NULL; unsigned n;
	double *colors = NULL;

	if (ds->style.outline)
		th--;

	if (!(xicon = XGetImage(dpy, icon, 0, 0, w, h, 0xffffffff, ZPixmap))) {
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

	bi = &c->iconbtn;
	bi->x = bi->y = bi->b = 0;
	bi->d = ds->depth;
	bi->w = w;
	bi->h = h;
	if (bi->pixmap.ximage) {
		XDestroyImage(bi->pixmap.ximage);
		bi->pixmap.ximage = NULL;
	}
	XPRINTF(__CFMTS(c) "assigning ximage %p\n", __CARGS(c), ximage);
	bi->pixmap.ximage = ximage;
	bi->present = True;
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
static Bool
ximage_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data)
{
	XImage *ximage = NULL;
	ButtonImage *bi;
	unsigned i, j, k, l, d = ds->depth, th = ds->style.titleheight;
	unsigned long pixel;
	double *chanls = NULL; unsigned m;
	double *counts = NULL; unsigned n;
	double *colors = NULL;
	long *p, *copy = NULL;

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

	bi = &c->iconbtn;
	bi->x = bi->y = bi->b = 0;
	bi->d = d;
	bi->w = w;
	bi->h = h;
	if (bi->pixmap.ximage) {
		XDestroyImage(bi->pixmap.ximage);
		bi->pixmap.ximage = NULL;
	}
	XPRINTF(__CFMTS(c) "assigning ximage %p (%dx%dx%d+%d+%d)\n", __CARGS(c), ximage,
			bi->w, bi->h, bi->d, bi->x, bi->y);
	bi->pixmap.ximage = ximage;
	bi->present = True;
	return (True);

      error:
	free(copy);
	free(chanls);
	free(counts);
	free(colors);
	if (ximage)
		XDestroyImage(ximage);
      return (False);
}
#else
static Bool
xlib_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h)
{
	XImage *xicon = NULL, *xmask = NULL, *alpha = NULL;
	ButtonImage *bi;
	unsigned i, j, k, l, d = ds->depth, th = ds->style.titleheight;
	unsigned long pixel, fg, bg;
	Pixmap pixmap;
	double *chanls = NULL; unsigned m; /* we all float down here... */
	double *counts = NULL; unsigned n;
	double *colors = NULL;

	/* for now */
	fg = ds->style.color.norm[ColFG].pixel;
	bg = ds->style.color.norm[ColBG].pixel;

	if (!(xicon = XGetImage(dpy, icon, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", icon, w, h);
		goto error;
	}
	if (mask && !(xmask = XGetImage(dpy, mask, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", mask, w, h);
		goto error;
	}
	if (h > th) {
		double scale = (double) th / (double) h;

		unsigned sh = lround(h * scale);
		unsigned sw = lround(w * scale);

		XPRINTF(__CFMTS(c) "scaling bitmap 0x%lx from %ux%u to %ux%u\n", __CARGS(c), icon, w, h, sw, sh);

		if (!(alpha = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, sw, sh, 8, 0))) {
			EPRINTF("could not create image %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(alpha->data = emallocz(alpha->bytes_per_line * alpha->height))) {
			EPRINTF("could not allocate image data %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(chanls = ecalloc(alpha->bytes_per_line * alpha->height, 4 * sizeof(*chanls)))) {
			EPRINTF("could not allocate chanls %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(counts = ecalloc(alpha->bytes_per_line * alpha->height, sizeof(*counts)))) {
			EPRINTF("could not allocate counts %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(colors = ecalloc(alpha->bytes_per_line * alpha->height, sizeof(*colors)))) {
			EPRINTF("could not allocate colors %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		for (j = 0; j < h; j++) {
			l = trunc(j * scale);
			for (i = 0; i < w; i++) {
				k = trunc(i * scale);
				n = l * sw + k;
				m = n << 2;
				counts[n]++;
				if (!xmask || XGetPixel(xmask, i, j)) {
					colors[n]++;
					pixel = XGetPixel(xicon, i, j) ? fg : bg;
					chanls[m+0] += 255;
					chanls[m+1] += (pixel >> 16) & 0xff;
					chanls[m+2] += (pixel >>  8) & 0xff;
					chanls[m+3] += (pixel >>  0) & 0xff;
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
				XPutPixel(alpha, k, l, pixel);
				amax = max(amax, ((pixel >> 24) & 0xff));
			}
		}
		if (!amax) {
			/* no opacity at all! */
			for (l = 0; l < sh; l++) {
				for (k = 0; k < sw; k++) {
					XPutPixel(alpha, k, l, XGetPixel(alpha, k, l) | 0xff000000);
				}
			}
		} else if (amax < 255) {
			/* something has to be opaque, bump the alpha */
			double bump = (double) 255 / (double) amax;
			for (l = 0; l < sh; l++) {
				for (k = 0; k < sw; k++) {
					pixel = XGetPixel(alpha, k, l);
					amax = (pixel >> 24) & 0xff;
					amax = lround(amax * bump);
					if (amax > 255)
						amax = 255;
					pixel = (pixel & 0x00ffffff) | (amax << 24);
					XPutPixel(alpha, k, l, pixel);
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
		if (!(alpha = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, w, h, 8, 0))) {
			EPRINTF("could not create image %ux%ux%u\n", w, h, d);
			goto error;
		}
		if (!(alpha->data = emallocz(alpha->bytes_per_line * alpha->height))) {
			EPRINTF("could not allocate image data %ux%ux%u\n", w, h, d);
			goto error;
		}
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				if (!xmask || XGetPixel(xmask, i, j)) {
					if (XGetPixel(xicon, i, j)) {
						XPutPixel(alpha, i, j, fg | 0xFF000000);
					} else {
						XPutPixel(alpha, i, j, bg | 0xFF000000);
					}
				} else {
					XPutPixel(alpha, i, j, 0x00000000);
				}
			}
		}
	}
	XDestroyImage(xicon);
	if (xmask)
		XDestroyImage(xmask);
	pixmap = XCreatePixmap(dpy, ds->drawable, w, h, ds->depth);
	XPutImage(dpy, pixmap, ds->dc.gc, alpha, 0, 0, 0, 0, w, h);
	XDestroyImage(alpha);

	bi = &c->iconbtn;
	bi->x = bi->y = bi->b = 0;
	bi->d = ds->depth;
	bi->w = w;
	bi->h = h;
	if (bi->pixmap.draw) {
		XFreePixmap(dpy, bi->pixmap.draw);
		bi->pixmap.draw = None;
	}
	if (bi->pixmap.mask) {
		XFreePixmap(dpy, bi->pixmap.mask);
		bi->pixmap.mask = None;
	}
	XPRINTF(__CFMTS(c) "assigning pixmap 0x%lx\n", __CARGS(c), pixmap);
	bi->pixmap.draw = pixmap;
	bi->present = True;
	return (True);

      error:
	free(chanls);
	free(counts);
	free(colors);
	if (alpha)
		XDestroyImage(alpha);
	if (xmask)
		XDestroyImage(xmask);
	if (xicon)
		XDestroyImage(xicon);
      return (False);
}
static Bool
xlib_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h, unsigned d)
{
	XImage *xicon = NULL, *xmask = NULL, *alpha = NULL;
	ButtonImage *bi;
	unsigned i, j, k, l, th = ds->style.titleheight;
	unsigned long pixel, bg;
	Pixmap pixmap;
	double *chanls = NULL; unsigned m;
	double *counts = NULL; unsigned n;
	double *colors = NULL;

	/* for now */
	bg = ds->style.color.norm[ColBG].pixel;

	if (!(xicon = XGetImage(dpy, icon, 0, 0, w, h, 0xffffffff, ZPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", icon, w, h);
		goto error;
	}
	if (mask && !(xmask = XGetImage(dpy, mask, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", mask, w, h);
		goto error;
	}
	d = ds->depth;
	if (h > th) {
		double scale = (double) th / (double) h;

		unsigned sh = lround(h * scale);
		unsigned sw = lround(w * scale);

		XPRINTF(__CFMTS(c) "scaling pixmap 0x%lx from %ux%u to %ux%u\n", __CARGS(c), icon, w, h, sw, sh);

		if (!(alpha = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, sw, sh, 8, 0))) {
			EPRINTF("could not create image %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(alpha->data = emallocz(alpha->bytes_per_line * alpha->height))) {
			EPRINTF("could not allocate image data %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(chanls = ecalloc(alpha->bytes_per_line * alpha->height, 4 * sizeof(*chanls)))) {
			EPRINTF("could not allocate chanls %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(counts = ecalloc(alpha->bytes_per_line * alpha->height, sizeof(*counts)))) {
			EPRINTF("could not allocate counts %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(colors = ecalloc(alpha->bytes_per_line * alpha->height, sizeof(*colors)))) {
			EPRINTF("could not allocate colors %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		for (j = 0; j < h; j++) {
			l = trunc(j * scale);
			for (i = 0; i < w; i++) {
				k = trunc(i * scale);
				n = l * sw + k;
				m = n << 2;
				counts[n]++;
				if (!xmask || XGetPixel(xmask, i, j)) {
					colors[n]++;
					pixel = XGetPixel(xicon, i, j);
					chanls[m+0] += 255;
					chanls[m+1] += (pixel >> 16) & 0xff;
					chanls[m+2] += (pixel >>  8) & 0xff;
					chanls[m+3] += (pixel >>  0) & 0xff;
				} else {
					/* should be conditional on not using XRender */
					colors[n]++;
					pixel = bg;
					chanls[m+0] += 0;
					chanls[m+1] += (pixel >> 16) & 0xff;
					chanls[m+2] += (pixel >>  8) & 0xff;
					chanls[m+3] += (pixel >>  0) & 0xff;
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
				XPutPixel(alpha, k, l, pixel);
				amax = max(amax, ((pixel >> 24) & 0xff));
			}
		}
		if (!amax) {
			/* no opacity at all! */
			for (l = 0; l < sh; l++) {
				for (k = 0; k < sw; k++) {
					XPutPixel(alpha, k, l, XGetPixel(alpha, k, l) | 0xff000000);
				}
			}
		} else if (amax < 255) {
			/* something has to be opaque, bump the alpha */
			double bump = (double) 255 / (double) amax;
			for (l = 0; l < sh; l++) {
				for (k = 0; k < sw; k++) {
					pixel = XGetPixel(alpha, k, l);
					amax = (pixel >> 24) & 0xff;
					amax = lround(amax * bump);
					if (amax > 255)
						amax = 255;
					pixel = (pixel & 0x00ffffff) | (amax << 24);
					XPutPixel(alpha, k, l, pixel);
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
		if (!(alpha = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, w, h, 8, 0))) {
			EPRINTF("could not create image %ux%ux%u\n", w, h, d);
			goto error;
		}
		if (!(alpha->data = emallocz(alpha->bytes_per_line * alpha->height))) {
			EPRINTF("could not allocate image data %ux%ux%u\n", w, h, d);
			goto error;
		}
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				if (!xmask || XGetPixel(xmask, i, j)) {
					XPutPixel(alpha, i, j, XGetPixel(xicon, i, j) | 0xFF000000);
				} else {
					XPutPixel(alpha, i, j, XGetPixel(xicon, i, j) & 0x00FFFFFF);
				}
			}
		}
	}
	XDestroyImage(xicon);
	if (xmask)
		XDestroyImage(xmask);
	pixmap = XCreatePixmap(dpy, ds->drawable, w, h, ds->depth);
	XPutImage(dpy, pixmap, ds->dc.gc, alpha, 0, 0, 0, 0, w, h);
	XDestroyImage(alpha);

	bi = &c->iconbtn;
	bi->x = bi->y = bi->b = 0;
	bi->d = ds->depth;
	bi->w = w;
	bi->h = h;
	if (bi->pixmap.draw) {
		XFreePixmap(dpy, bi->pixmap.draw);
		bi->pixmap.draw = None;
	}
	if (bi->pixmap.mask) {
		XFreePixmap(dpy, bi->pixmap.mask);
		bi->pixmap.mask = None;
	}
	XPRINTF(__CFMTS(c) "assigning pixmap 0x%lx\n", __CARGS(c), pixmap);
	bi->pixmap.draw = pixmap;
	bi->present = True;
	return (True);

      error:
	free(chanls);
	free(counts);
	free(colors);
	if (alpha)
		XDestroyImage(alpha);
	if (xmask)
		XDestroyImage(xmask);
	if (xicon)
		XDestroyImage(xicon);
      return (False);
}
static Bool
xlib_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data)
{
	XImage *alpha;
	ButtonImage *bi;
	unsigned i, j, k, l, d = ds->depth, th = ds->style.titleheight;
	unsigned long pixel, bg;
	Pixmap pixmap;
	double *chanls = NULL; unsigned m;
	double *counts = NULL; unsigned n;
	double *colors = NULL;
	long *p;

	/* for now */
	bg = ds->style.color.norm[ColBG].pixel;

	if (h > th) {
		double scale = (double) th / (double) h;

		unsigned sh = lround(h * scale);
		unsigned sw = lround(w * scale);

		XPRINTF(__CFMTS(c) "scaling data %p from %ux%u to %ux%u\n", __CARGS(c), data, w, h, sw, sh);

		if (!(alpha = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, sw, sh, 8, 0))) {
			EPRINTF("could not create image %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(alpha->data = emallocz(alpha->bytes_per_line * alpha->height))) {
			EPRINTF("could not allocate image data %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(chanls = ecalloc(alpha->bytes_per_line * alpha->height, 4 * sizeof(*chanls)))) {
			EPRINTF("could not allocate chanls %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(counts = ecalloc(alpha->bytes_per_line * alpha->height, sizeof(*counts)))) {
			EPRINTF("could not allocate counts %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		if (!(colors = ecalloc(alpha->bytes_per_line * alpha->height, sizeof(*colors)))) {
			EPRINTF("could not allocate colors %ux%ux%u\n", sw, sh, d);
			goto error;
		}
		unsigned rb = (bg >> 16) & 0xff;
		unsigned gb = (bg >>  8) & 0xff;
		unsigned bb = (bg >>  0) & 0xff;

		for (p = data, j = 0; j < h; j++) {
			l = trunc(j * scale);
			for (i = 0; i < w; i++, p++) {
				k = trunc(i * scale);
				n = l * sw + k;
				m = n << 2;
				pixel = *p;
				counts[n]++;
				{
					unsigned a = (pixel >> 24) & 0xff;
					unsigned r = (pixel >> 16) & 0xff;
					unsigned g = (pixel >>  8) & 0xff;
					unsigned b = (pixel >>  0) & 0xff;

					/* should only be when not using XRender */
					r = ((r * a) + (rb * (255 - a))) / 255;
					g = ((g * a) + (gb * (255 - a))) / 255;
					b = ((b * a) + (bb * (255 - a))) / 255;

					colors[n]++;
					chanls[m+0] += a;
					chanls[m+1] += r;
					chanls[m+2] += g;
					chanls[m+3] += b;
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
				XPutPixel(alpha, k, l, pixel);
				amax = max(amax, ((pixel >> 24) & 0xff));
			}
		}
		if (!amax) {
			/* no opacity at all! */
			for (l = 0; l < sh; l++) {
				for (k = 0; k < sw; k++) {
					XPutPixel(alpha, k, l, XGetPixel(alpha, k, l) | 0xff000000);
				}
			}
		} else if (amax < 255) {
			/* something has to be opaque, bump the alpha */
			double bump = (double) 255 / (double) amax;
			for (l = 0; l < sh; l++) {
				for (k = 0; k < sw; k++) {
					pixel = XGetPixel(alpha, k, l);
					amax = (pixel >> 24) & 0xff;
					amax = lround(amax * bump);
					if (amax > 255)
						amax = 255;
					pixel = (pixel & 0x00ffffff) | (amax << 24);
					XPutPixel(alpha, k, l, pixel);
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
		if (!(alpha = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, w, h, 8, 0))) {
			EPRINTF("could not create image %ux%ux%u\n", w, h, d);
			goto error;
		}
		if (!(alpha->data = emallocz(alpha->bytes_per_line * alpha->height))) {
			EPRINTF("could not allocate image data %ux%ux%u\n", w, h, d);
			goto error;
		}
	}
	pixmap = XCreatePixmap(dpy, ds->drawable, w, h, d);
	XPutImage(dpy, pixmap, ds->dc.gc, alpha, 0, 0, 0, 0, w, h);
	XDestroyImage(alpha);

	bi = &c->iconbtn;
	bi->x = bi->y = bi->b = 0;
	bi->d = d;
	bi->w = w;
	bi->h = h;
	if (bi->pixmap.draw) {
		XFreePixmap(dpy, bi->pixmap.draw);
		bi->pixmap.draw = None;
	}
	if (bi->pixmap.mask) {
		XFreePixmap(dpy, bi->pixmap.mask);
		bi->pixmap.mask = None;
	}
	XPRINTF(__CFMTS(c) "assigning pixmap 0x%lx\n", __CARGS(c), pixmap);
	bi->pixmap.draw = pixmap;
	bi->present = True;
	return (True);

      error:
	free(chanls);
	free(counts);
	free(colors);
	if (alpha)
		XDestroyImage(alpha);
      return (False);
}
#endif

#if defined IMLIB2 && defined USE_IMLIB2
#define createbitmapicon(args...) imlib_createbitmapicon(args)
#define createpixmapicon(args...) imlib_createpixmapicon(args)
#define createdataicon(args...)   imlib_createdataicon(args)
#else				/* !defined IMLIB2 || !defined USE_IMLIB2 */
#if defined PIXBUF && defined USE_PIXBUF
#define createbitmapicon(args...) gdk_createbitmapicon(args)
#define createpixmapicon(args...) gdk_createpixmapicon(args)
#define createdataicon(args...)   gdk_createdataicon(args)
#else				/* !defined PIXBUF || !defined USE_PIXBUF */
#if defined RENDER && defined USE_RENDER
#define createbitmapicon(args...) xrender_createbitmapicon(args)
#define createpixmapicon(args...) xrender_createpixmapicon(args)
#define createdataicon(args...)   xrender_createdataicon(args)
#else				/* !defined RENDER || !defined USE_RENDER */
#if 1
#define createbitmapicon(args...) ximage_createbitmapicon(args)
#define createpixmapicon(args...) ximage_createpixmapicon(args)
#define createdataicon(args...)   ximage_createdataicon(args)
#else
#define createbitmapicon(args...) xlib_createbitmapicon(args)
#define createpixmapicon(args...) xlib_createpixmapicon(args)
#define createdataicon(args...)   xlib_createdataicon(args)
#endif
#endif				/* !defined RENDER || !defined USE_RENDER */
#endif				/* !defined PIXBUF || !defined USE_PIXBUF */
#endif				/* !defined IMLIB2 || !defined USE_IMLIB2 */

Bool
createwmicon(Client *c)
{
	Window root;
	int x, y;
	unsigned int w, h, b, d;
	Pixmap icon = c->wmh.icon_pixmap;
	Pixmap mask = c->wmh.icon_mask;

	if (!c || !icon)
		return (False);
	if (!XGetGeometry(dpy, icon, &root, &x, &y, &w, &h, &b, &d))
		return (False);
	/* Watch out for the depth of the pixmap, old apps like xeyes will use a bitmap
	   (1-deep pixmap) for the icon.  More modern applications (like xterm) will use
	   a pixmap with default visual and depth. */
	if (d == 1)
		return createbitmapicon(scr, c, icon, mask, w, h);
	if (d > 1)
		return createpixmapicon(scr, c, icon, mask, w, h, d);
	return (False);
}

Bool
createkwmicon(Client *c, Pixmap *data, unsigned long n)
{
	Window root;
	int x, y;
	unsigned b;
	struct {
		Pixmap icon, mask;
		unsigned w, h, d;
	} *icons = NULL, *best;
	unsigned long rem;
	unsigned hdiff, dbest;
	Bool status = False;
	Pixmap *icon;
	int i, m;

	/* scan icons */
	for (m = 0, icon = data, rem = n; rem >= 2; m++, icon += 2, rem -= 2) {
		if (!icon[0])
			break;
		if (rem < 2)
			break;
		if (!(icons = reallocarray(icons, m + 1, sizeof(*icons))))
			break;
		icons[m].icon = icon[0];
		icons[m].mask = icon[1];
		if (!XGetGeometry(dpy, icon[0], &root, &x, &y,
					&icons[m].w, &icons[m].h, &b,
					&icons[m].d))
			break;
	}
	for (hdiff = -1U, dbest = 0, best = NULL, i = 0; i < m; i++) {
		if (icons[i].h <= scr->style.titleheight)
			b = scr->style.titleheight - icons[i].h;
		else
			b = icons[i].h - scr->style.titleheight;

		if (best && b == hdiff) {
			if (icons[i].d > dbest) {
				best = &icons[i];
				dbest = icons[i].d;
			}
		}
		if (b < hdiff) {
			best = &icons[i];
			hdiff = b;
			dbest = icons[i].d;
		}
	}
	if (best) {
		if (best->d == 1)
			status = createbitmapicon(scr, c, best->icon, best->mask, best->w, best->h);
		else if (best->d > 1)
			status = createpixmapicon(scr, c, best->icon, best->mask, best->w, best->h, best->d);
	}
	free(icons);
	XFree(data);
	return (status);
}

Bool
createneticon(Client *c, long *data, unsigned long n)
{
	struct {
		unsigned w, h;
		long *icon;
	} *icons = NULL, *best;
	unsigned long rem, z;
	unsigned hdiff, d;
	Bool status = False;
	long *icon;
	int i, m;

	/* scan icons */
	for (m = 0, icon = data, rem = n; rem > 2; m++, icon += z, rem -= z) {
		if (!icon[0] || !icon[1])
			break;
		z = 2 + icon[0] * icon[1];
		if (rem < z)
			break;
		if (!(icons = reallocarray(icons, m + 1, sizeof(*icons))))
			break;
		icons[m].w = icon[0];
		icons[m].h = icon[1];
		icons[m].icon = icon + 2;
	}
	for (hdiff = -1U, best = NULL, i = 0; i < m; i++) {
		if (icons[i].h <= scr->style.titleheight)
			d = scr->style.titleheight - icons[i].h;
		else
			d = icons[i].h - scr->style.titleheight;

		if (d < hdiff) {
			best = &icons[i];
			hdiff = d;
		}
	}
	if (best)
		status = createdataicon(scr, c, best->w, best->h, best->icon);
	free(icons);
	XFree(data);
	return (status);
}

enum { Normal, Focused, Selected };
enum { AlignLeft, AlignCenter, AlignRight };	/* title position */

static unsigned int
textnw(AScreen *ds, const char *text, unsigned int len, int hilite)
{
	XftTextExtentsUtf8(dpy, ds->style.font[hilite],
			   (const unsigned char *) text, len,
			   ds->dc.font[hilite].extents);
	return ds->dc.font[hilite].extents->xOff;
}

static unsigned int
textw(AScreen *ds, const char *text, int hilite)
{
	return textnw(ds, text, strlen(text), hilite) + ds->dc.font[hilite].height;
}

static ButtonImage *
buttonimage(AScreen *ds, Client *c, ElementType type)
{
	Bool pressed, hovered, focused, enabled, present, toggled;
	ButtonImage *image;
	ElementClient *ec;
	Element *e;
	int which;
	const char *name = NULL;

	(void) name;
	switch (type) {
	case MenuBtn:
		name = "menu";
		present = True;
		toggled = False;
		enabled = True;
		break;
	case IconifyBtn:
		name = "iconify";
		present = c->has.but.min;
		toggled = False;
		enabled = c->can.min;
		break;
	case MaximizeBtn:
		name = "maximize";
		present = c->has.but.max;
		toggled = c->is.max || c->is.maxv || c->is.maxh || c->is.full;
		enabled = c->can.max || c->can.maxv || c->can.maxh || c->can.full;
		break;
	case CloseBtn:
		name = "close";
		present = c->has.but.close;
		toggled = c->is.closing || c->is.killing;
		enabled = c->can.close;
		break;
	case ShadeBtn:
		name = "shade";
		present = c->has.but.shade;
		toggled = c->is.shaded;
		enabled = c->can.shade;
		break;
	case StickBtn:
		name = "stick";
		present = c->has.but.stick;
		toggled = c->is.sticky;
		enabled = c->can.stick;
		break;
	case LHalfBtn:
		name = "lhalf";
		present = c->has.but.half;
		toggled = c->is.lhalf;
		enabled = c->can.size && c->can.move;
		break;
	case RHalfBtn:
		name = "rhalf";
		present = c->has.but.half;
		toggled = c->is.rhalf;
		enabled = c->can.size && c->can.move;
		break;
	case FillBtn:
		name = "fill";
		present = c->has.but.fill;
		toggled = c->is.fill;
		enabled = c->can.fill || c->can.fillh || c->can.fillv;
		break;
	case FloatBtn:
		name = "float";
		present = c->has.but.floats;
		toggled = c->is.floater || c->skip.arrange;
		enabled = c->can.floats || c->can.arrange;
		break;
	case SizeBtn:
		name = "resize";
		present = c->has.but.size;
		toggled = c->is.moveresize;
		enabled = c->can.size || c->can.sizev || c->can.sizeh;
		break;
	case IconBtn:
		name = "icon";
		present = True;
		toggled = False;
		enabled = True;
		break;
	default:
		name = "unknown";
		present = False;
		toggled = False;
		enabled = False;
		break;
	}
	XPRINTF("Getting button image for '%s'\n", name);
	if (!present) {
		XPRINTF("button %s is not present!\n", name);
		return NULL;
	}

	ec = &c->element[type];
	pressed = ec->pressed;
	hovered = ec->hovered;
	focused = (c == sel);

	e = &ds->element[type];
	if (!e->action) {
		XPRINTF("button %s has no actions!\n", name);
		return NULL;
	}

	if (type == IconBtn && c->iconbtn.present)
		return &c->iconbtn;

	image = e->image;

	if (pressed == 0x1 && toggled && image[ButtonImageToggledPressedB1].present) {
		XPRINTF("button %s assigned toggled.pressed.b1 image\n", name);
		return &image[ButtonImageToggledPressedB1];
	}
	if (pressed == 0x2 && toggled && image[ButtonImageToggledPressedB2].present) {
		XPRINTF("button %s assigned toggled.pressed.b2 image\n", name);
		return &image[ButtonImageToggledPressedB2];
	}
	if (pressed == 0x4 && toggled && image[ButtonImageToggledPressedB3].present) {
		XPRINTF("button %s assigned toggled.pressed.b3 image\n", name);
		return &image[ButtonImageToggledPressedB3];
	}
	if (pressed && toggled && image[ButtonImageToggledPressed].present) {
		XPRINTF("button %s assigned toggled.pressed image\n", name);
		return &image[ButtonImageToggledPressed];
	}
	if (pressed == 0x1 && image[ButtonImagePressedB1].present) {
		XPRINTF("button %s assigned pressed.b1 image\n", name);
		return &image[ButtonImagePressedB1];
	}
	if (pressed == 0x2 && image[ButtonImagePressedB2].present) {
		XPRINTF("button %s assigned pressed.b2 image\n", name);
		return &image[ButtonImagePressedB2];
	}
	if (pressed == 0x4 && image[ButtonImagePressedB3].present) {
		XPRINTF("button %s assigned pressed.b3 image\n", name);
		return &image[ButtonImagePressedB3];
	}
	if (pressed && image[ButtonImagePressed].present) {
		XPRINTF("button %s assigned pressed image\n", name);
		return &image[ButtonImagePressed];
	}
	which = ButtonImageHover + (hovered ? 0 : (focused ? 1 : 2));
	which += toggled ? 3 : 0;
	which += enabled ? 0 : 6;
	if (hovered && !image[which].present) {
		XPRINTF("button %s has no hovered image %d, using %s instead\n", name,
			which, focused ? "focused" : "unfocus");
		which += focused ? 1 : 2;
	}
	if (!focused && !image[which].present) {
		XPRINTF("button %s has no unfocus image %d, using focused instead\n",
			name, which);
		which -= 1;
	}
	if (toggled && !image[which].present) {
		XPRINTF("button %s has no toggled image %d, using untoggled instead\n",
			name, which);
		which -= 3;
	}
	if (!enabled && !image[which].present) {
		XPRINTF("button %s missing disabled image %d, skipping button\n", name,
			which);
		return NULL;
	}
	if (image[which].present) {
		XPRINTF("button %s going with chosen image %d\n", name, which);
		return &image[which];
	}
	if (image[ButtonImageDefault].present) {
		XPRINTF("button %s missing chosen image %d, going with default\n", name,
			which);
		return &image[ButtonImageDefault];
	}
	XPRINTF("button %s missing default image, skipping button\n", name);
	return NULL;
}

static int
buttonw(AScreen *ds, Client *c, ElementType type)
{
	ButtonImage *bi;

	if (!(bi = buttonimage(ds, c, type)) || !bi->present)
		return 0;
	return bi->w + 2 * bi->b;
}

static ElementType
elementtype(char which)
{
	switch (which) {
	case 'E':
		return MenuBtn;
	case 'I':
		return IconifyBtn;
	case 'M':
		return MaximizeBtn;
	case 'C':
		return CloseBtn;
	case 'S':
		return ShadeBtn;
	case 'A':
		return StickBtn;
	case 'L':
		return LHalfBtn;
	case 'R':
		return RHalfBtn;
	case 'X':
		return FillBtn;
	case 'F':
		return FloatBtn;
	case 'Z':
		return SizeBtn;
	case 'P':
		return IconBtn;
	case 'T':
		return TitleTags;
	case 'N':
		return TitleName;
	case '|':
		return TitleSep;
	default:
		return LastElement;
	}
}

static int
elementw(AScreen *ds, Client *c, char which)
{
	int w = 0;
	ElementType type = elementtype(which);
	int hilite = (c == sel) ? Selected : (c->is.focused ? Focused : Normal);

	if (0 > type || type >= LastElement)
		return 0;

	switch (type) {
		unsigned int j;

	case TitleTags:
		for (j = 0; j < ds->ntags; j++) {
			if (c->tags & (1ULL << j))
				w += textw(ds, ds->tags[j].name, hilite);
		}
		break;
	case TitleName:
		w = textw(ds, c->name, hilite);
		break;
	case TitleSep:
		w = ds->dc.h / 2;
		break;
	default:
		if (0 <= type && type < LastBtn)
			w = buttonw(ds, c, type);
		break;
	}
	return w;
}

static int
drawtext(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw,
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
			XftTextRenderUtf8(dpy, PictOpSrc, shd, font, dst, 0, 0, x + drop,
					  y + drop, (FcChar8 *) buf, len);
		XftTextRenderUtf8(dpy, PictOpSrc, src, font, dst, 0, 0, x + drop,
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

static int
drawbutton(AScreen *ds, Client *c, ElementType type, XftColor *col, int x)
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

		XRenderFillRectangle(dpy, PictOpSrc, dst, &bg->color, g.x, g.y, g.w, g.h);
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

static int
drawelement(AScreen *ds, char which, int x, int position, Client *c)
{
	int w = 0;
	XftColor *color = c == sel ? ds->style.color.sele : ds->style.color.norm;
	int hilite = (c == sel) ? Selected : (c->is.focused ? Focused : Normal);
	ElementType type = elementtype(which);
	ElementClient *ec = &c->element[type];

	if (0 > type || type >= LastElement)
		return 0;

	XPRINTF("drawing element '%c' for client %s\n", which, c->name);

	ec->present = False;
	/* element client geometry used to detect element under pointer */
	ec->eg.x = ds->dc.x;
	ec->eg.y = 0;
	ec->eg.w = 0;
	ec->eg.h = ds->style.titleheight;
	ec->eg.b = 0;		/* later */

	switch (type) {
		unsigned int j;
		int x, tw;

	case TitleTags:
		for (j = 0, x = ds->dc.x; j < ds->ntags; j++, w += tw, x += tw)
			tw = (c->tags & (1ULL << j)) ? drawtext(ds, ds->tags[j].name,
								ds->dc.draw.pixmap,
								ds->dc.draw.xft, color,
								hilite, x, ds->dc.y,
								ds->dc.w) : 0;
		ec->eg.w = w;
		break;
	case TitleName:
		w = drawtext(ds, c->name, ds->dc.draw.pixmap, ds->dc.draw.xft, color,
			     hilite, ds->dc.x, ds->dc.y, ds->dc.w);
		ec->eg.w = w;
		break;
	case TitleSep:
		XSetForeground(dpy, ds->dc.gc, color[ColBorder].pixel);
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x + ds->dc.h / 4, 0,
			  ds->dc.x + ds->dc.h / 4, ds->dc.h);
		w = ds->dc.h / 2;
		ec->eg.w = w;
		break;
	default:
		if (0 <= type && type < LastBtn)
			w = drawbutton(ds, c, type, color, ds->dc.x);
		break;
	}
	if (w) {
		ec->present = True;
		XPRINTF("element '%c' at %dx%d+%d+%d:%d for client '%s'\n",
			which, ec->eg.w, ec->eg.h, ec->eg.x, ec->eg.y, ec->eg.b, c->name);
	} else
		XPRINTF("missing element '%c' for client %s\n", which, c->name);
	return w;
}

static void
drawdockapp(Client *c, AScreen *ds)
{
	int status;
	unsigned long pixel;

	pixel = (c == sel) ? ds->style.color.sele[ColBG].pixel : ds->style.color.norm[ColBG].pixel;
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
	status =
	    XFillRectangle(dpy, c->frame, ds->gc, ds->dc.x, ds->dc.y, ds->dc.w,
			   ds->dc.h);
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
drawclient(Client *c)
{
	size_t i;
	AScreen *ds;
	int status;

	/* might be drawing a client that is not on the current screen */
	if (!(ds = getscreen(c->win, True))) {
		XPRINTF("What? no screen for window 0x%lx???\n", c->win);
		return;
	}
	if (c == sel && !ds->colormapnotified)
		installcolormaps(ds, c, c->cmapwins);
	setopacity(c, (c == sel) ? OPAQUE : ds->style.opacity);
	if (!isvisible(c, NULL))
		return;
	if (c->is.dockapp)
		return drawdockapp(c, ds);
	if (!c->title && !c->grips)
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
	XSetForeground(dpy, ds->dc.gc,
		       c == sel ? ds->style.color.sele[ColBG].pixel : ds->style.color.norm[ColBG].pixel);
	XSetLineAttributes(dpy, ds->dc.gc, ds->style.border, LineSolid, CapNotLast,
			   JoinMiter);
	XSetFillStyle(dpy, ds->dc.gc, FillSolid);
	status =
	    XFillRectangle(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x, ds->dc.y,
			   ds->dc.w, ds->dc.h);
	if (!status)
		XPRINTF("Could not fill rectangle, error %d\n", status);
	/* Don't know about this... */
	if (ds->dc.w < textw(ds, c->name, (c == sel) ? Selected : (c->is.focused ? Focused : Normal))) {
		ds->dc.w -= elementw(ds, c, CloseBtn);
		drawtext(ds, c->name, ds->dc.draw.pixmap, ds->dc.draw.xft,
			 c == sel ? ds->style.color.sele : ds->style.color.norm,
			 c == sel ? 1 : 0, ds->dc.x, ds->dc.y, ds->dc.w);
		drawbutton(ds, c, CloseBtn,
			   c == sel ? ds->style.color.sele : ds->style.color.norm,
			   ds->dc.w);
		goto end;
	}
	/* Left */
	ds->dc.x +=
	    (ds->style.spacing >
	     ds->style.border) ? ds->style.spacing - ds->style.border : 0;
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
	ds->dc.x -=
	    (ds->style.spacing >
	     ds->style.border) ? ds->style.spacing - ds->style.border : 0;
	for (i = strlen(ds->style.titlelayout); i--;) {
		if (ds->style.titlelayout[i] == ' ' || ds->style.titlelayout[i] == '-')
			break;
		ds->dc.x -= elementw(ds, c, ds->style.titlelayout[i]);
		drawelement(ds, ds->style.titlelayout[i], 0, AlignRight, c);
		ds->dc.x -= ds->style.spacing;
	}
      end:
	if (ds->style.outline) {
		XSetForeground(dpy, ds->dc.gc,
			       c == sel ? ds->style.color.sele[ColBorder].pixel :
			       ds->style.color.norm[ColBorder].pixel);
		XPRINTF(__CFMTS(c) "drawing line from +%d+%d to +%d+%d\n", __CARGS(c),
				0, ds->dc.h - 1, ds->dc.w, ds->dc.h - 1);
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, 0, ds->dc.h - 1, ds->dc.w,
			  ds->dc.h - 1);
	}
	if (c->title) {
		XPRINTF(__CFMTS(c) "copying title pixmap to %dx%d+%d+%d to +%d+%d\n", __CARGS(c),
				c->c.w, ds->dc.h, 0, 0, 0, 0);
		XCopyArea(dpy, ds->dc.draw.pixmap, c->title, ds->dc.gc, 0, 0, c->c.w,
			  ds->dc.h, 0, 0);
	}
	ds->dc.x = ds->dc.y = 0;
	ds->dc.w = c->c.w;
	ds->dc.h = ds->style.gripsheight;
	XSetForeground(dpy, ds->dc.gc,
		       c == sel ? ds->style.color.sele[ColBG].pixel :
		       ds->style.color.norm[ColBG].pixel);
	XSetLineAttributes(dpy, ds->dc.gc, ds->style.border, LineSolid, CapNotLast,
			   JoinMiter);
	XSetFillStyle(dpy, ds->dc.gc, FillSolid);
	status =
	    XFillRectangle(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x, ds->dc.y,
			   ds->dc.w, ds->dc.h);
	if (!status)
		XPRINTF("Could not fill rectangle, error %d\n", status);
	if (ds->style.outline) {
		XSetForeground(dpy, ds->dc.gc,
			       c == sel ? ds->style.color.sele[ColBorder].pixel :
			       ds->style.color.norm[ColBorder].pixel);
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

static Bool
alloccolor(const char *colstr, XftColor *color)
{
	Bool status;

	if (!colstr)
		return (False);
	status = XftColorAllocName(dpy, scr->visual, scr->colormap, colstr, color);
	if (!status)
		eprint("error, cannot allocate color '%s'\n", colstr);
	return (status);
}

static void
freepixmap(ButtonImage *bi)
{
	if (bi->pixmap.draw) {
		XFreePixmap(dpy, bi->pixmap.draw);
		bi->pixmap.draw = None;
	}
}

#ifdef IMLIB2
const char *
imlib_error_string(Imlib_Load_Error error)
{
	switch (error) {
	case IMLIB_LOAD_ERROR_NONE:
		return ("IMLIB_LOAD_ERROR_NONE");
	case IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST:
		return ("IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST");
	case IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY:
		return ("IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY");
	case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ:
		return ("IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ");
	case IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT:
		return ("IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT");
	case IMLIB_LOAD_ERROR_PATH_TOO_LONG:
		return ("IMLIB_LOAD_ERROR_PATH_TOO_LONG");
	case IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT:
		return ("IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT");
	case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY:
		return ("IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY");
	case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE:
		return ("IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE");
	case IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS:
		return ("IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS");
	case IMLIB_LOAD_ERROR_OUT_OF_MEMORY:
		return ("IMLIB_LOAD_ERROR_OUT_OF_MEMORY");
	case IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS:
		return ("IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS");
	case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE:
		return ("IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE");
	case IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE:
		return ("IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE");
	case IMLIB_LOAD_ERROR_UNKNOWN:
		return ("IMLIB_LOAD_ERROR_UNKNOWN");
	}
	return ("IMLIB_LOAD_ERROR_UNKNOWN");
}
#endif

#ifdef XPM
#if defined RENDER
Bool
xrender_initxpm(char *path, ButtonImage *bi)
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
#endif				/* defined RENDER */

const char *
xpm_status_string(int status)
{
	static char buf[64] = { 0, };

	switch (status) {
	case XpmColorError:
		return ("color error");
	case XpmSuccess:
		return ("success");
	case XpmOpenFailed:
		return ("open failed");
	case XpmFileInvalid:
		return ("file invalid");
	case XpmNoMemory:
		return ("no memory");
	case XpmColorFailed:
		return ("color failed");
	default:
		snprintf(buf, sizeof(buf), "unknown %d", status);
		return (buf);
	}
}

static Bool
initxpm(char *path, ButtonImage *bi)
{
	Pixmap draw = None, mask = None;
	XpmAttributes xa = { 0, };
	int status;

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
	if (bi->pixmap.draw) {
		XFreePixmap(dpy, bi->pixmap.draw);
		bi->pixmap.draw = None;
	}
	if (bi->pixmap.mask) {
		XFreePixmap(dpy, bi->pixmap.mask);
		bi->pixmap.mask = None;
	}
	bi->pixmap.draw = draw;
	bi->pixmap.mask = mask;
	bi->w = xa.width;
	bi->h = xa.height;
	XPRINTF("%s pixmap geometry is %dx%dx%d+%d+%d\n", path, bi->w, bi->h, scr->depth, bi->x, bi->y);
	if (bi->h > scr->style.titleheight) {
		/* read lower down into image to clip top and bottom by same amount */
		bi->y += (bi->h - scr->style.titleheight) / 2;
		bi->h = scr->style.titleheight;
		XPRINTF("%s pixmap clipped to %dx%dx%d+%d+%d\n", path, bi->w, bi->h, scr->depth, bi->x, bi->y);
	}
	free(path);
	return True;
}
#endif

static Bool
initxbm(char *path, ButtonImage *bi)
{
	Pixmap draw = None, mask = None;
	int status;

	status =
	    XReadBitmapFile(dpy, scr->root, path, &bi->w, &bi->h, &draw, &bi->x, &bi->y);
	if (status != BitmapSuccess || !draw) {
		EPRINTF("could not load xbm file %s\n", path);
		return False;
	}
	if (bi->bitmap.draw)
		XFreePixmap(dpy, bi->bitmap.draw);
	if (bi->bitmap.mask)
		XFreePixmap(dpy, bi->bitmap.mask);
	bi->bitmap.draw = draw;
	bi->bitmap.mask = mask;
	/* don't care about hostspot: not a cursor */
	bi->x = bi->y = 0;
	if (bi->h > scr->style.titleheight) {
		/* read lower down into image to clip top and bottom by same amount */
		bi->y += (bi->h - scr->style.titleheight) / 2;
		bi->h = scr->style.titleheight;
	}
	free(path);
	return True;
}

static Bool
initext(char *path, ButtonImage *bi)
{
	Imlib_Image image;
	Imlib_Load_Error error;

	imlib_context_push(scr->context);

	image = imlib_load_image_with_error_return(path, &error);
	if (!image) {
		EPRINTF("%s: could not load image file %s\n",
			imlib_error_string(error), path);
		imlib_context_pop();
		return False;
	}
	imlib_context_set_image(image);
	imlib_context_set_mask(None);
	bi->w = imlib_image_get_width();
	bi->h = imlib_image_get_height();
	if (bi->h > scr->style.titleheight) {
		/* read lower down into image to clip top and bottom by same amount */
		bi->y += (bi->h - scr->style.titleheight) / 2;
		bi->h = scr->style.titleheight;
	}
	if (bi->pixmap.image) {
		imlib_context_set_image(bi->pixmap.image);
		imlib_context_set_mask(None);
		imlib_free_image();
	}
	bi->pixmap.image = image;
	free(path);
	return True;
}

static Bool
initbitmap(char *path, ButtonImage *bi)
{
	unsigned char *data = NULL;
	Imlib_Image image;
	DATA32 *pixels;
	int status;

	status = XReadBitmapFileData(path, &bi->w, &bi->h, &data, &bi->x, &bi->y);
	if (status != BitmapSuccess || !data) {
		EPRINTF("could not load image file %s\n", path);
		return False;
	}
	/* don't care about hotspot: not a cursor */
	bi->x = bi->y = 0;
	imlib_context_push(scr->context);
	image = imlib_create_image(bi->w, bi->h);
	if (!image) {
		EPRINTF("Cannot create image %d x %d\n", bi->w, bi->h);
		imlib_context_pop();
		XFree(data);
		return False;
	}
	imlib_context_set_image(image);
	imlib_context_set_mask(None);
	pixels = imlib_image_get_data();
	if (!pixels) {
		EPRINTF("Cannot get image data\n");
		imlib_free_image();
		imlib_context_pop();
		XFree(data);
		return False;
	}
	{
		unsigned int i, j, stride = (bi->w + 7) >> 3;
		DATA32 *p = pixels;

		for (i = 0; i < bi->h; i++) {
			unsigned char *row = &data[i * stride];

			for (j = 0; j < bi->w; j++) {
				unsigned char *byte = &row[j >> 3];
				unsigned bit = j % 8;

				if ((*byte >> bit) & 0x1) {
					/* it is a one */
					*p++ = 0xffffffff;
				} else {
					/* it is a zero */
					*p++ = 0x00000000;
				}
			}
		}
	}
	imlib_image_put_back_data(pixels);
	bi->w = imlib_image_get_width();
	bi->h = imlib_image_get_height();
	if (bi->h > scr->style.titleheight) {
		/* read lower down into image to clip top and bottom by same amount */
		bi->y += (bi->h - scr->style.titleheight) / 2;
		bi->h = scr->style.titleheight;
	}
	if (bi->bitmap.image) {
		imlib_context_set_image(bi->bitmap.image);
		imlib_context_set_mask(None);
		imlib_free_image();
	}
	bi->bitmap.image = image;
	imlib_context_pop();
	bi->x = bi->y = bi->b = 0;
	bi->d = 1;
	XFree(data);
	free(path);
	return True;
}

static Bool
initpixmap(const char *file, ButtonImage *bi)
{
	char *path;

	if (!file || !(path = findrcpath(file)))
		return False;
#ifdef XPM
	/* use libxpm for xpms because it works best */
	if (strstr(path, ".xpm") && strlen(strstr(path, ".xpm")) == 4)
		if (initxpm(path, bi))
			return True;
#endif
	if (strstr(path, ".xbm") && strlen(strstr(path, ".xbm")) == 4)
		if (initxbm(path, bi))
			return True;
#ifdef IMLIB2
	if (!strstr(path, ".xbm") || strlen(strstr(path, ".xbm")) != 4) {
		if (initext(path, bi))
			return True;
	} else {
		if (initbitmap(path, bi))
			return True;
	}
#endif
	EPRINTF("could not load button element from %s\n", path);
	if (bi->pixmap.draw) {
		XFreePixmap(dpy, bi->pixmap.draw);
		bi->pixmap.draw = None;
	}
	if (bi->pixmap.mask) {
		XFreePixmap(dpy, bi->pixmap.mask);
		bi->pixmap.mask = None;
	}
	if (bi->bitmap.draw) {
		XFreePixmap(dpy, bi->bitmap.draw);
		bi->bitmap.draw = None;
	}
	if (bi->bitmap.mask) {
		XFreePixmap(dpy, bi->bitmap.mask);
		bi->bitmap.mask = None;
	}
#ifdef IMLIB2
	imlib_context_push(scr->context);
	if (bi->pixmap.image) {
		imlib_context_set_image(bi->pixmap.image);
		imlib_context_set_mask(None);
		imlib_free_image();
		bi->pixmap.image = NULL;
	}
	if (bi->bitmap.image) {
		imlib_context_set_image(bi->bitmap.image);
		imlib_context_set_mask(None);
		imlib_free_image();
		bi->bitmap.image = NULL;
	}
	imlib_context_pop();
#endif
	free(path);
	return False;
}

static Bool
b_menu(Client *c, XEvent *ev)
{
	spawn(scr->options.menucommand);
	return True;
}

static Bool
b_min(Client *c, XEvent *ev)
{
	if (!c->can.min)
		return False;
	switch (ev->xbutton.button) {
	case Button1:
		iconify(c);
		break;
	case Button2:
		iconify(c);	/* reserved for hide window */
		break;
	case Button3:
		iconify(c);	/* reserved for withdraw window */
		break;
	default:
		return False;
	}
	return True;
}

static Bool
b_max(Client *c, XEvent *ev)
{
	switch (ev->xbutton.button) {
	case Button1:
		if (c->can.max || (c->can.size && c->can.move))
			togglemax(c);
		break;
	case Button2:
		if (c->can.maxv || ((c->can.size || c->can.sizev) && c->can.move))
			togglemaxv(c);
		break;
	case Button3:
		if (c->can.maxh || ((c->can.size || c->can.sizeh) && c->can.move))
			togglemaxh(c);
		break;
	default:
		return False;
	}
	return True;
}

static Bool
b_close(Client *c, XEvent *ev)
{
	if (!c->can.close)
		return False;
	switch (ev->xbutton.button) {
	case Button1:
		killclient(c);
		break;
	case Button2:
		killproc(c);
		break;
	case Button3:
		killxclient(c);
		break;
	default:
		return False;
	}
	return True;
}

static Bool
b_shade(Client *c, XEvent *ev)
{
	if (!c->can.shade)
		return False;
	switch (ev->xbutton.button) {
	case Button1:
		break;
	case Button2:
		if (c->is.shaded)
			return False;
		break;
	case Button3:
		if (!c->is.shaded)
			return False;
		break;
	default:
		return False;
	}
	toggleshade(c);
	return True;
}

static Bool
b_stick(Client *c, XEvent *ev)
{
	if (!c->can.stick)
		return False;
	switch (ev->xbutton.button) {
	case Button1:
		break;
	case Button2:
		if (c->is.sticky)
			return False;
		break;
	case Button3:
		if (!c->is.sticky)
			return False;
		break;
	default:
		return False;
	}
	togglesticky(c);
	return True;
}

static Bool
b_lhalf(Client *c, XEvent *ev)
{
	if (!c->can.move || !c->can.size)
		return False;
	switch (ev->xbutton.button) {
	case Button1:
		break;
	case Button2:
		if (c->is.lhalf)
			return False;
		break;
	case Button3:
		if (!c->is.lhalf)
			return False;
		break;
	default:
		return False;
	}
	togglelhalf(c);
	return True;
}

static Bool
b_rhalf(Client *c, XEvent *ev)
{
	if (!c->can.move || !c->can.size)
		return False;
	switch (ev->xbutton.button) {
	case Button1:
		break;
	case Button2:
		if (c->is.rhalf)
			return False;
		break;
	case Button3:
		if (!c->is.rhalf)
			return False;
		break;
	default:
		return False;
	}
	togglerhalf(c);
	return True;
}

static Bool
b_fill(Client *c, XEvent *ev)
{
	if (!c->can.fill)
		return False;
	switch (ev->xbutton.button) {
	case Button1:
		break;
	case Button2:
		if (c->is.fill)
			return False;
		break;
	case Button3:
		if (!c->is.fill)
			return False;
		break;
	default:
		return False;
	}
	togglefill(c);
	return True;
}

static Bool
b_float(Client *c, XEvent *ev)
{
	switch (ev->xbutton.button) {
	case Button1:
		break;
	case Button2:
		if (c->skip.arrange)
			return False;
		break;
	case Button3:
		if (!c->skip.arrange)
			return False;
		break;
	default:
		return False;
	}
	togglefloating(c);
	return True;
}

static Bool
b_resize(Client *c, XEvent *ev)
{
	if (!c->can.size)
		return False;
	return m_resize(c, ev);
}

static void
freeelement(ElementType type)
{
	int i;
	Element *e = &scr->element[type];

	if (e->image && type < LastBtn) {
		for (i = 0; i < LastButtonImageType; i++) {
			if (e->image[i].present)
				freepixmap(&e->image[i]);
		}
		free(e->image);
		e->image = NULL;
	}
}

static void
initelement(ElementType type, const char *name, const char *def,
	    Bool (**action) (Client *, XEvent *))
{
	char res[128] = { 0, }, buf[128] = { 0, }, *bgdef;

	static const char *kind[LastButtonImageType] = {
		[ButtonImageDefault] = "",
		[ButtonImagePressed] = ".pressed",
		[ButtonImagePressedB1] = ".pressed.b1",
		[ButtonImagePressedB2] = ".pressed.b2",
		[ButtonImagePressedB3] = ".pressed.b3",
		[ButtonImageToggledPressed] = ".toggled.pressed",
		[ButtonImageToggledPressedB1] = ".toggled.pressed.b1",
		[ButtonImageToggledPressedB2] = ".toggled.pressed.b2",
		[ButtonImageToggledPressedB3] = ".toggled.pressed.b3",
		[ButtonImageHover] = ".hovered",
		[ButtonImageFocus] = ".focused",
		[ButtonImageUnfocus] = ".unfocus",
		[ButtonImageToggledHover] = ".toggled.hovered",
		[ButtonImageToggledFocus] = ".toggled.focused",
		[ButtonImageToggledUnfocus] = ".toggled.unfocus",
		[ButtonImageDisabledHover] = ".disabled.hovered",
		[ButtonImageDisabledFocus] = ".disabled.focused",
		[ButtonImageDisabledUnfocus] = ".disabled.unfocus",
		[ButtonImageToggledDisabledHover] = ".toggled.disabled.hovered",
		[ButtonImageToggledDisabledFocus] = ".toggled.disabled.focused",
		[ButtonImageToggledDisabledUnfocus] = ".toggled.disabled.unfocus",
	};
	int i;
	Element *e = &scr->element[type];

	if (type < LastBtn) {
		if (!name)
			return;
		e->action = NULL;
		free(e->image);
		e->image = ecalloc(LastButtonImageType, sizeof(*e->image));
		for (i = 0; i < LastButtonImageType; i++) {
			if (i == 0 || !e->image[0].bg.pixel) {
				bgdef = NULL;
			} else {
				snprintf(buf, sizeof(buf), "#%02x%02x%02x",
						e->image[0].bg.color.red,
						e->image[0].bg.color.green,
						e->image[0].bg.color.blue);
				bgdef = buf;
			}
			snprintf(res, sizeof(res), "%s%s.bg", name, kind[i]);
			alloccolor(getscreenres(res, bgdef), &e->image[i].bg);
			bgdef = NULL;

			snprintf(res, sizeof(res), "%s%s.pixmap", name, kind[i]);
			if ((e->image[i].present =
			     initpixmap(getscreenres(res, def), &e->image[i])))
				e->action = action;
			else
				XPRINTF("could not load pixmap for %s\n", res);
			def = NULL;
		}
	} else {
		scr->element[type].action = action;
	}
}

static void
freebuttons()
{
	int i;

	for (i = 0; i < LastElement; i++)
		freeelement(i);
}

static void
initbuttons()
{
	int i;
	static struct {
		const char *name;
		const char *def;
		Bool (*action[Button5-Button1+1][2]) (Client *, XEvent *);
	} setup[LastElement] = {
		/* *INDENT-OFF* */
		[MenuBtn]	= { "button.menu",	MENUPIXMAP,	{
			[Button1-Button1] = { NULL,		b_menu		},
			[Button2-Button1] = { NULL,		b_menu		},
			[Button3-Button1] = { NULL,		b_menu		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[IconifyBtn]	= { "button.iconify",	ICONPIXMAP,	{
					    /* ButtonPress	ButtonRelease	*/
			[Button1-Button1] = { NULL,		b_min		},
			[Button2-Button1] = { NULL,		b_min		},
			[Button3-Button1] = { NULL,		b_min		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[MaximizeBtn]	= { "button.maximize",	MAXPIXMAP,	{
			[Button1-Button1] = { NULL,		b_max		},
			[Button2-Button1] = { NULL,		b_max		},
			[Button3-Button1] = { NULL,		b_max		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[CloseBtn]	= { "button.close",	CLOSEPIXMAP,	{
			[Button1-Button1] = { NULL,		b_close		},
			[Button2-Button1] = { NULL,		b_close		},
			[Button3-Button1] = { NULL,		b_close		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[ShadeBtn]	= { "button.shade",	SHADEPIXMAP,	{
			[Button1-Button1] = { NULL,		b_shade		},
			[Button2-Button1] = { NULL,		b_shade		},
			[Button3-Button1] = { NULL,		b_shade		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[StickBtn]	= { "button.stick",	STICKPIXMAP,	{
			[Button1-Button1] = { NULL,		b_stick		},
			[Button2-Button1] = { NULL,		b_stick		},
			[Button3-Button1] = { NULL,		b_stick		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[LHalfBtn]	= { "button.lhalf",	LHALFPIXMAP,	{
			[Button1-Button1] = { NULL,		b_lhalf		},
			[Button2-Button1] = { NULL,		b_lhalf		},
			[Button3-Button1] = { NULL,		b_lhalf	},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[RHalfBtn]	= { "button.rhalf",	RHALFPIXMAP,	{
			[Button1-Button1] = { NULL,		b_rhalf		},
			[Button2-Button1] = { NULL,		b_rhalf		},
			[Button3-Button1] = { NULL,		b_rhalf		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[FillBtn]	= { "button.fill",	FILLPIXMAP,	{
			[Button1-Button1] = { NULL,		b_fill		},
			[Button2-Button1] = { NULL,		b_fill		},
			[Button3-Button1] = { NULL,		b_fill		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[FloatBtn]	= { "button.float",	FLOATPIXMAP,	{
			[Button1-Button1] = { NULL,		b_float		},
			[Button2-Button1] = { NULL,		b_float		},
			[Button3-Button1] = { NULL,		b_float		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[SizeBtn]	= { "button.resize",	SIZEPIXMAP,	{
			[Button1-Button1] = { b_resize,		NULL		},
			[Button2-Button1] = { b_resize,		NULL		},
			[Button3-Button1] = { b_resize,		NULL		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[IconBtn]	= { "button.icon",	WINPIXMAP,	{
			[Button1-Button1] = { NULL,		b_menu		},
			[Button2-Button1] = { NULL,		b_menu		},
			[Button3-Button1] = { NULL,		b_menu		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[TitleTags]	= { "title.tags",	NULL,		{
			[Button1-Button1] = { NULL,		NULL		},
			[Button2-Button1] = { NULL,		NULL		},
			[Button3-Button1] = { NULL,		NULL		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[TitleName]	= { "title.name",	NULL,		{
			[Button1-Button1] = { m_move,		NULL		},
			[Button2-Button1] = { NULL,		NULL		},
			[Button3-Button1] = { m_resize,	NULL		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		[TitleSep]	= { "title.separator",	NULL,		{
			[Button1-Button1] = { NULL,		NULL		},
			[Button2-Button1] = { NULL,		NULL		},
			[Button3-Button1] = { NULL,		NULL		},
			[Button4-Button1] = { NULL,		NULL		},
			[Button5-Button1] = { NULL,		NULL		},
		} },
		/* *INDENT-ON* */
	};

	XSetForeground(dpy, scr->dc.gc, scr->style.color.norm[ColButton].pixel);
	XSetBackground(dpy, scr->dc.gc, scr->style.color.norm[ColBG].pixel);

	for (i = 0; i < LastElement; i++)
		initelement(i, setup[i].name, setup[i].def, &setup[i].action[0][0]);
}

static void
freefont(int hilite)
{
	if (scr->style.font[hilite]) {
		XftFontClose(dpy, scr->style.font[hilite]);
		scr->style.font[hilite] = NULL;
	}
	if (scr->dc.font[hilite].extents) {
		free(scr->dc.font[hilite].extents);
		scr->dc.font[hilite].extents = NULL;
	}
}

static void
initfont(const char *fontstr, int hilite)
{
	scr->style.font[hilite] = NULL;
	scr->style.font[hilite] = XftFontOpenXlfd(dpy, scr->screen, fontstr);
	if (!scr->style.font[hilite])
		scr->style.font[hilite] = XftFontOpenName(dpy, scr->screen, fontstr);
	if (!scr->style.font[hilite])
		eprint("error, cannot load font: '%s'\n", fontstr);
	scr->dc.font[hilite].extents = emallocz(sizeof(XGlyphInfo));
	XftTextExtentsUtf8(dpy, scr->style.font[hilite],
			   (const unsigned char *) fontstr, strlen(fontstr),
			   scr->dc.font[hilite].extents);
	scr->dc.font[hilite].height =
	    scr->style.font[hilite]->ascent + scr->style.font[hilite]->descent + 1;
	scr->dc.font[hilite].ascent = scr->style.font[hilite]->ascent;
	scr->dc.font[hilite].descent = scr->style.font[hilite]->descent;
}

static void
freecolor(XftColor *color)
{
	if (color->pixel)
		XftColorFree(dpy, scr->visual, scr->colormap, color);
}

void
freestyle()
{
	int i, j;

	freebuttons();
	for (i = 0; i <= Selected; i++) {
		for (j = 0; j < ColLast; j++)
			freecolor(&scr->style.color.hue[i][j]);
		freefont(i);
	}
	if (scr->dc.gc) {
		XFreeGC(dpy, scr->dc.gc);
		scr->dc.gc = None;
	}
	if (scr->dc.draw.xft) {
		XftDrawDestroy(scr->dc.draw.xft);
		scr->dc.draw.xft = NULL;
	}
	if (scr->dc.draw.pixmap) {
		XFreePixmap(dpy, scr->dc.draw.pixmap);
		scr->dc.draw.pixmap = None;
	}
}

void
initstyle(Bool reload)
{
	Client *c;

	freestyle();

	alloccolor(getscreenres("selected.border", SELBORDERCOLOR), &scr->style.color.sele[ColBorder]);
	alloccolor(getscreenres("selected.bg", SELBGCOLOR), &scr->style.color.sele[ColBG]);
	alloccolor(getscreenres("selected.fg", SELFGCOLOR), &scr->style.color.sele[ColFG]);
	alloccolor(getscreenres("selected.button", SELBUTTONCOLOR), &scr->style.color.sele[ColButton]);

	alloccolor(getscreenres("focused.border", FOCBORDERCOLOR), &scr->style.color.focu[ColBorder]);
	alloccolor(getscreenres("focused.bg", FOCBGCOLOR), &scr->style.color.focu[ColBG]);
	alloccolor(getscreenres("focused.fg", FOCFGCOLOR), &scr->style.color.focu[ColFG]);
	alloccolor(getscreenres("focused.button", FOCBUTTONCOLOR), &scr->style.color.focu[ColButton]);

	alloccolor(getscreenres("normal.border", NORMBORDERCOLOR), &scr->style.color.norm[ColBorder]);
	alloccolor(getscreenres("normal.bg", NORMBGCOLOR), &scr->style.color.norm[ColBG]);
	alloccolor(getscreenres("normal.fg", NORMFGCOLOR), &scr->style.color.norm[ColFG]);
	alloccolor(getscreenres("normal.button", NORMBUTTONCOLOR), &scr->style.color.norm[ColButton]);

	if ((scr->style.drop[Selected] = atoi(getscreenres("selected.drop", "0"))))
		alloccolor(getscreenres("selected.shadow", SELBORDERCOLOR), &scr->style.color.sele[ColShadow]);
	if ((scr->style.drop[Focused] = atoi(getscreenres("focused.drop", "0"))))
		alloccolor(getscreenres("focused.shadow", FOCBORDERCOLOR), &scr->style.color.focu[ColShadow]);
	if ((scr->style.drop[Normal] = atoi(getscreenres("normal.drop", "0"))))
		alloccolor(getscreenres("normal.shadow", NORMBORDERCOLOR), &scr->style.color.norm[ColShadow]);

	initfont(getscreenres("selected.font", getscreenres("font", FONT)), Selected);
	initfont(getscreenres("focused.font", getscreenres("font", FONT)), Focused);
	initfont(getscreenres("normal.font", getscreenres("font", FONT)), Normal);
	scr->style.border = atoi(getscreenres("border", STR(BORDERPX)));
	scr->style.margin = atoi(getscreenres("margin", STR(MARGINPX)));
	scr->style.opacity = (int) ((double) OPAQUE * atof(getscreenres("opacity", STR(NF_OPACITY))));
	scr->style.outline = atoi(getscreenres("outline", "0")) ? 1 : 0;
	scr->style.spacing = atoi(getscreenres("spacing", "1"));
	strncpy(scr->style.titlelayout, getscreenres("titlelayout", "N  IMC"),
		LENGTH(scr->style.titlelayout));
	scr->style.gripsheight = atoi(getscreenres("grips", STR(GRIPHEIGHT)));
	scr->style.titlelayout[LENGTH(scr->style.titlelayout) - 1] = '\0';
	scr->style.titleheight = atoi(getscreenres("title", STR(TITLEHEIGHT)));
	if (!scr->style.titleheight)
		scr->style.titleheight =
		    max(scr->dc.font[Selected].height + scr->style.drop[Selected],
		    max(scr->dc.font[Focused].height + scr->style.drop[Focused],
			scr->dc.font[Normal].height + scr->style.drop[Normal])) + 2;
	scr->dc.gc = XCreateGC(dpy, scr->drawable, 0, 0);
	scr->dc.draw.w = DisplayWidth(dpy, scr->screen);
	scr->dc.draw.h = max(scr->style.titleheight, scr->style.gripsheight);
	if (scr->dc.draw.h) {
		scr->dc.draw.pixmap =
		    XCreatePixmap(dpy, scr->drawable, scr->dc.draw.w, scr->dc.draw.h,
				  scr->depth);
		scr->dc.draw.xft =
		    XftDrawCreate(dpy, scr->dc.draw.pixmap,
				  scr->visual,
				  scr->colormap);
	}
	/* GC for dock apps */
	scr->gc = XCreateGC(dpy, scr->root, 0, 0);
	initbuttons();
	/* redraw all existing clients */
	for (c = scr->clients; c ; c = c->next) {
		drawclient(c);
		ewmh_update_net_window_extents(c);
	}
}
