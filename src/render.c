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
#include "render.h" /* verification */

#undef ALPHAW32
#undef CMPALPHA
#undef CROPSCALE
#define FILTERPIC
#undef NEARESTFL
#undef DOWNSCALE
#define JUSTRENDER

#if defined RENDER
void
render_removepixmap(AdwmPixmap *p)
{
	if (p->pixmap.pict) {
		XRenderFreePicture(dpy, p->pixmap.pict);
		p->pixmap.pict = None;
	}
	if (p->bitmap.pict) {
		XRenderFreePicture(dpy, p->bitmap.pict);
		p->bitmap.pict = None;
	}
}

#ifdef JUSTRENDER
static Bool
createicon_bitmap(AScreen *ds, Client *c, Pixmap draw, Pixmap mask, unsigned w, unsigned h, Bool cropscale)
{
	ButtonImage **bis, *bi;
	AdwmPixmap *px;
	Picture pict = None;
	XRenderPictFormat *format;
	XRenderPictureAttributes pa = {
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
	};
	unsigned long pamask = CPComponentAlpha;
	unsigned th = titleheight(ds);
	Bool result = False;

	if (mask) {
		pa.clip_mask = mask;
		pa.clip_x_origin = 0;
		pa.clip_y_origin = 0;
		pamask |= CPClipMask | CPClipXOrigin | CPClipYOrigin;
	}
	format = XRenderFindStandardFormat(dpy, PictStandardA1);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		if ((pict = XRenderCreatePicture(dpy, draw, format, pamask, &pa))) {
			bi = *bis;
			px = &bi->px;
			px->x = px->y = px->b = 0;
			px->w = w;
			px->h = h;
			px->d = 1;
			if (h > th) {
				XPRINTF("transforming image from h = %u to %u\n", h, th);
				/* get XRender to scale the image for us */
				XDouble scale = (XDouble) th / (XDouble) h;
				/* *INDENT-OFF* */
				XTransform trans = {
					{ { XDoubleToFixed(1.0), 0, 0 },
					  { 0, XDoubleToFixed(1.0), 0 },
					  { 0, 0, XDoubleToFixed(scale) } }
				};
				/* *INDENT-ON* */
#ifdef FILTERPIC
#ifdef NEARESTFL
				XRenderSetPictureFilter(dpy, pict, FilterNearest, NULL, 0);
#else
				XRenderSetPictureFilter(dpy, pict, FilterBilinear, NULL, 0);
#endif
#endif				/* FILTERPIC */
				XRenderSetPictureTransform(dpy, pict, &trans);
				px->w = lround(w * scale);
				px->h = th;
			}
			if (px->bitmap.pict) {
				XRenderFreePicture(dpy, px->bitmap.pict);
				px->bitmap.pict = None;
			}
			px->bitmap.pict = pict;
			bi->present = True;
			result = True;
		} else
			EPRINTF("could not create picture\n");
	}
	return (result);
}
#else				/* JUSTRENDER */
static Bool
createicon_bitmap(AScreen *ds, Client *c, XImage *xdraw, XImage *xmask, Bool cropscale)
{
	XImage *ximage = NULL;
	ButtonImage **bis, *bi;
	AdwmPixmap *px;
	unsigned th = titleheight(ds);
	Pixmap draw = None;
	Picture pict = None;
	Bool result = False;
	XRenderPictFormat *format;
	GC gc = None;
	XRenderPictureAttributes pa = {
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
	};
	unsigned long pamask = CPComponentAlpha;

	if (!(ximage = combine_bitmap_and_mask(ds, xdraw, xmask))) {
		EPRINTF("could not combine draw and mask images\n");
		goto error;
	}
	if (!(draw = XCreatePixmap(dpy, ds->drawable,
				   ximage->width, ximage->height, ximage->depth))) {
		EPRINTF("could not create pixmap\n");
		goto error;
	}
	if (!(gc = XCreateGC(dpy, draw, 0UL, NULL))) {
		EPRINTF("could not create gc\n");
		goto error;
	}
	XPutImage(dpy, draw, gc, ximage, 0, 0, 0, 0, ximage->width, ximage->height);

	format = XRenderFindStandardFormat(dpy, PictStandardA8);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		if ((pict = XRenderCreatePicture(dpy, draw, format, pamask, &pa))) {
			bi = *bis;
			px = &bi->px;
			px->x = px->y = px->b = 0;
			px->d = ximage->depth;
			px->w = ximage->width;
			px->h = ximage->height;
			if (ximage->height > th) {
				XPRINTF("transforming image from h = %u to %u\n", ximage->height, th);
				/* get XRender to scale the image for us */
				XDouble scale = (XDouble) th / (XDouble) ximage->height;
				/* *INDENT-OFF* */
				XTransform trans = {
					{ { XDoubleToFixed(1.0), 0, 0 },
					  { 0, XDoubleToFixed(1.0), 0 },
					  { 0, 0, XDoubleToFixed(scale) } }
				};
				/* *INDENT-ON* */
#if 1
#ifdef FILTERPIC
#ifdef NEARESTFL
				XRenderSetPictureFilter(dpy, pict, FilterNearest, NULL, 0);
#else
				XRenderSetPictureFilter(dpy, pict, FilterBilinear, NULL, 0);
#endif
#endif				/* FILTERPIC */
#endif
				XRenderSetPictureTransform(dpy, pict, &trans);
				px->w = lround(ximage->width * scale);
				px->h = th;
			}
			if (px->bitmap.pict) {
				XRenderFreePicture(dpy, px->bitmap.pict);
				px->bitmap.pict = None;
			}
			px->bitmap.pict = pict;
			bi->present = True;
			result = True;
		} else
			EPRINTF("could not create picture\n");
	}
      error:
	if (gc)
		XFreeGC(dpy, gc);
	if (draw)
		XFreePixmap(dpy, draw);
	if (ximage)
		XDestroyImage(ximage);
	return (result);
}
#endif				/* JUSTRENDER */

#ifdef JUSTRENDER
static Bool
createicon_pixmap(AScreen *ds, Client *c, Pixmap draw, Pixmap mask, unsigned w, unsigned h, unsigned d, Bool cropscale)
{
	ButtonImage **bis, *bi;
	AdwmPixmap *px;
	Picture pict = None;
#if 0
	Picture alph = None;
#endif
	XRenderPictFormat *format;
	XRenderPictureAttributes pa = {
#ifdef ALPHAW32
		.component_alpha = d == 32 ? True : False,
#else				/* ALPHAW32 */
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
#endif				/* ALPHAW32 */
	};
	unsigned long pamask = CPComponentAlpha;
	unsigned th = titleheight(ds);
	Bool result = False;

	if (mask) {
#if 0
		format = XRenderFindStandardFormat(dpy, PictStandardA1);
		if (!(alph = XRenderCreatePicture(dpy, mask, format, pamask, &pa))) {
			EPRINTF("cannot create alpha picture\n");
			goto error;
		}
		pa.alpha_map = alph;
		pa.alpha_x_origin = 0;
		pa.alpha_y_origin = 0;
		pamask |= CPAlphaMap | CPAlphaXOrigin | CPAlphaYOrigin;
#else
		pa.clip_mask = mask;
		pa.clip_x_origin = 0;
		pa.clip_y_origin = 0;
		pamask |= CPClipMask | CPClipXOrigin | CPClipYOrigin;
#endif
	}
#if 0
	format = XRenderFindStandardFormat(dpy, PictStandardARGB32);
#else
	if (d == 32)
		format = XRenderFindStandardFormat(dpy, PictStandardARGB32);
	else
		format = XRenderFindStandardFormat(dpy, PictStandardRGB24);
#endif

	for (bis = getbuttons(c); bis && *bis; bis++) {
		if ((pict = XRenderCreatePicture(dpy, draw, format, pamask, &pa))) {
			bi = *bis;
			px = &bi->px;
			px->x = px->y = px->b = 0;
			px->w = w;
			px->h = h;
			px->d = d;
			if (h > th) {
				XPRINTF("transforming image from h = %u to %u\n", h, th);
				/* get XRender to scale the image for us */
				XDouble scale = (XDouble) th / (XDouble) h;
				/* *INDENT-OFF* */
				XTransform trans = {
					{ { XDoubleToFixed(1.0), 0, 0 },
					  { 0, XDoubleToFixed(1.0), 0 },
					  { 0, 0, XDoubleToFixed(scale) } }
				};
				/* *INDENT-ON* */
#ifdef FILTERPIC
#ifdef NEARESTFL
				XRenderSetPictureFilter(dpy, pict, FilterNearest, NULL, 0);
#else
				XRenderSetPictureFilter(dpy, pict, FilterBilinear, NULL, 0);
#endif
#endif				/* FILTERPIC */
				XRenderSetPictureTransform(dpy, pict, &trans);
				px->w = lround(w * scale);
				px->h = th;
			}
			if (px->pixmap.pict) {
				XRenderFreePicture(dpy, px->pixmap.pict);
				px->pixmap.pict = None;
			}
			px->pixmap.pict = pict;
			bi->present = True;
			result = True;
		} else
			EPRINTF("could not create picture\n");
	}
#if 0
      error:
	if (alph)
		XRenderFreePicture(dpy, alph);
#endif
	return (result);
}
#else				/* JUSTRENDER */
static Bool
createicon_pixmap(AScreen *ds, Client *c, XImage *xdraw, XImage *xmask, Bool cropscale)
{
	XImage *ximage = NULL;
	ButtonImage **bis, *bi;
	AdwmPixmap *px;
	unsigned th = titleheight(ds);
	Pixmap draw = None;
	Picture pict = None;
	Bool result = False;
	XRenderPictureAttributes pa = {
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
	};
	unsigned long pamask = CPComponentAlpha;

	if (!(ximage = combine_pixmap_and_mask(ds, xdraw, xmask))) {
		EPRINTF("could not combine draw and mask images\n");
		goto error;
	}
#ifdef CROPSCALE
	if (ximage->height > th)
		ximage = crop_image(ds, ximage);
#endif				/* CROPSCALE */
#ifdef DOWNSCALE
	if (ximage->height > th) {
		unsigned tw = floor((double) ximage->width * (double) th / (double) ximage->height);
		XPRINTF("scaling image %ux%ux%u to %ux%u\n", ximage->width, ximage->height, ximage->depth, tw, th);
		ximage = dn_scale_image(ds, ximage, tw, th, False);
		XPRINTF("scaled image to %ux%ux%u\n",  ximage->width, ximage->height, ximage->depth);
	}
#endif				/* DOWNSCALE */
	if (!(draw = XCreatePixmap(dpy, ds->drawable, ximage->width, ximage->height, ximage->depth))) {
		EPRINTF("could not create pixmap\n");
		goto error;
	}
	XPutImage(dpy, draw, ds->dc.gc, ximage, 0, 0, 0, 0, ximage->width, ximage->height);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		if ((pict = XRenderCreatePicture(dpy, draw, ds->format, pamask, &pa))) {
			bi = *bis;
			px = &bi->px;
			px->x = px->y = px->b = 0;
			px->d = ximage->depth;
			px->w = ximage->width;
			px->h = ximage->height;
			if (ximage->height > th) {
				XPRINTF("transforming image from h = %u to %u\n", ximage->height, th);
				/* get XRender to scale the image for us */
				XDouble scale =  (XDouble) th / (XDouble) ximage->height;
				/* *INDENT-OFF* */
				XTransform trans = {
					{ { XDoubleToFixed(1.0), 0, 0 },
					  { 0, XDoubleToFixed(1.0), 0 },
					  { 0, 0, XDoubleToFixed(scale) } }
				};
				/* *INDENT-ON* */
#ifdef FILTERPIC
#ifdef NEARESTFL
				XRenderSetPictureFilter(dpy, pict, FilterNearest, NULL, 0);
#else
				XRenderSetPictureFilter(dpy, pict, FilterBilinear, NULL, 0);
#endif
#endif				/* FILTERPIC */
				XRenderSetPictureTransform(dpy, pict, &trans);
				px->w = lround(ximage->width * scale);
				px->h = th;
			}
			if (px->pixmap.pict) {
				XRenderFreePicture(dpy, px->pixmap.pict);
				px->pixmap.pict = None;
			}
			px->pixmap.pict = pict;
			bi->present = True;
			result = True;
		} else
			EPRINTF("could not create picture\n");
	}
      error:
	if (draw)
		XFreePixmap(dpy, draw);
	if (ximage)
		XDestroyImage(ximage);
	return (result);
}
#endif				/* JUSTRENDER */

#ifdef JUSTRENDER
Bool
render_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w,
			unsigned h)
{
	return createicon_bitmap(ds, c, icon, mask, w, h, True);
}
#else				/* JUSTRENDER */
Bool
render_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w,
			unsigned h)
{
	XImage *xdraw = NULL, *xmask = NULL, *ximage = NULL;
	ButtonImage *bi, **bis;
	AdwmPixmap *px;
	unsigned th = titleheight(ds);
	Pixmap draw = None;
	Picture pict = None;
	Bool result = False;
	XRenderPictureAttributes pa = {
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
	};
	unsigned long pamask = CPComponentAlpha;

	if (!(xdraw = XGetImage(dpy, icon, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", icon, w, h);
		goto error;
	}
	if (mask && !(xmask = XGetImage(dpy, mask, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", mask, w, h);
		goto error;
	}
	if (!(ximage = combine_pixmap_and_mask(ds, xdraw, xmask))) {
		EPRINTF("could not combine draw and mask images\n");
		goto error;
	}
	if (!(draw = XCreatePixmap(dpy, ds->drawable, ximage->width, ximage->height, ximage->depth))) {
		EPRINTF("could not create pixmap\n");
		goto error;
	}
	XPutImage(dpy, draw, ds->dc.gc, ximage, 0, 0, 0, 0, w, h);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		if ((pict = XRenderCreatePicture(dpy, draw, ds->format, pamask, &pa))) {
			bi = *bis;
			px = &bi->px;
			px->x = px->y = px->b = 0;
			px->d = ds->depth;
			px->w = w;
			px->h = h;
			if (h > th) {
				XPRINTF("transforming image from h = %u to %u\n", ximage->height, th);
				/* get XRender to scale the image for us */
				XDouble scale = (XDouble) th / (XDouble) h;
				/* *INDENT-OFF* */
				XTransform trans = {
					{ { XDoubleToFixed(1.0), 0, 0 },
					  { 0, XDoubleToFixed(1.0), 0 },
					  { 0, 0, XDoubleToFixed(scale) } }
				};
				/* *INDENT-ON* */
#ifdef FILTERPIC
#ifdef NEARESTFL
				XRenderSetPictureFilter(dpy, pict, FilterNearest, NULL, 0);
#else
				XRenderSetPictureFilter(dpy, pict, FilterBilinear, NULL, 0);
#endif
#endif				/* FILTERPIC */
				XRenderSetPictureTransform(dpy, pict, &trans);
				px->w = lround(w * scale);
				px->h = th;
			}
			if (px->bitmap.pict) {
				XRenderFreePicture(dpy, px->bitmap.pict);
				px->bitmap.pict = None;
			}
			px->bitmap.pict = pict;
			bi->present = True;
			result = True;
		} else
			EPRINTF("could not create picture\n");
	}
      error:
	if (draw)
		XFreePixmap(dpy, draw);
	if (xdraw)
		XDestroyImage(xdraw);
	if (xmask)
		XDestroyImage(xmask);
	if (ximage)
		XDestroyImage(ximage);
	return (result);
}
#endif				/* JUSTRENDER */

#ifdef JUSTRENDER
Bool
render_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w,
			unsigned h, unsigned d)
{
	return createicon_pixmap(ds, c, icon, mask, w, h, d, True);
}
#else				/* JUSTRENDER */
Bool
render_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w,
			unsigned h, unsigned d)
{
	XImage *xdraw = NULL, *xmask = NULL, *ximage = NULL;
	ButtonImage *bi, **bis;
	AdwmPixmap *px;
	unsigned th = titleheight(ds);
	Pixmap draw = None;
	Picture pict = None;
	Bool result = False;
	XRenderPictureAttributes pa = {
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
	};
	unsigned long pamask = CPComponentAlpha;

	if (!(xdraw = XGetImage(dpy, icon, 0, 0, w, h, AllPlanes, ZPixmap))) {
		EPRINTF("could not get pixmap 0x%lx %ux%u\n", icon, w, h);
		goto error;
	}
	if (mask && !(xmask = XGetImage(dpy, mask, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", mask, w, h);
		goto error;
	}
	if (!(ximage = combine_pixmap_and_mask(ds, xdraw, xmask))) {
		EPRINTF("could not combine ximages\n");
		goto error;
	}
#ifdef CROPSCALE
	if (ximage->height > th)
		ximage = crop_image(ds, ximage);
#endif				/* CROPSCALE */
#ifdef DOWNSCALE
	if (ximage->height > th) {
		unsigned tw = floor((double) ximage->width * (double) th / (double) ximage->height);
		XPRINTF("scaling image %ux%ux%u to %ux%u\n", ximage->width, ximage->height, ximage->depth, tw, th);
		ximage = dn_scale_image(ds, ximage, tw, th, False);
		XPRINTF("scaled image to %ux%ux%u\n",  ximage->width, ximage->height, ximage->depth);
	}
#endif				/* DOWNSCALE */
	if (!(draw = XCreatePixmap(dpy, ds->drawable, ximage->width, ximage->height, ximage->depth))) {
		EPRINTF("could not create pixmap\n");
		goto error;
	}
	XPutImage(dpy, draw, ds->dc.gc, ximage, 0, 0, 0, 0, ximage->width, ximage->height);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		if ((pict = XRenderCreatePicture(dpy, draw, ds->format, pamask, &pa))) {
			bi = *bis;
			px = &bi->px;
			px->x = px->y = px->b = 0;
			px->d = ximage->depth;
			px->w = ximage->width;
			px->h = ximage->height;
			if (ximage->height > th) {
				XPRINTF("transforming image from h = %u to %u\n", ximage->height, th);
				/* get XRender to scale the image for us */
				XDouble scale = (XDouble) th / (XDouble) ximage->height;
				/* *INDENT-OFF* */
				XTransform trans = {
					{ { XDoubleToFixed(1.0), 0, 0 },
					  { 0, XDoubleToFixed(1.0), 0 },
					  { 0, 0, XDoubleToFixed(scale) } }
				};
				/* *INDENT-ON* */
#ifdef FILTERPIC
#ifdef NEARESTFL
				XRenderSetPictureFilter(dpy, pict, FilterNearest, NULL, 0);
#else
				XRenderSetPictureFilter(dpy, pict, FilterBilinear, NULL, 0);
#endif
#endif				/* FILTERPIC */
				XRenderSetPictureTransform(dpy, pict, &trans);
				px->w = lround(ximage->width * scale);
				px->h = th;
			}
			if (px->pixmap.pict) {
				XRenderFreePicture(dpy, px->pixmap.pict);
				px->pixmap.pict = None;
			}
			px->pixmap.pict = pict;
			bi->present = True;
			result = True;
		} else
			EPRINTF("could not create picture\n");
	}
      error:
	if (draw)
		XFreePixmap(dpy, draw);
	if (xdraw)
		XDestroyImage(xdraw);
	if (xmask)
		XDestroyImage(xmask);
	if (ximage)
		XDestroyImage(ximage);
	return (result);
}
#endif				/* JUSTRENDER */

Bool
render_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data)
{
	XImage *ximage = NULL;
	ButtonImage *bi, **bis;
	AdwmPixmap *px;
	unsigned i, j, th = titleheight(ds);
	long *p;
	Pixmap draw = None;
	Picture pict = None;
	Bool result = False;
	XRenderPictureAttributes pa = {
#ifdef ALPHAW32
		.component_alpha = True,
#else				/* ALPHAW32 */
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
#endif				/* ALPHAW32 */
	};
	unsigned long pamask = CPComponentAlpha;
	unsigned long alpha = 0;

	if (!data) {
		EPRINTF("ERROR: pass null pointer!\n");
		return (False);
	}
	if (!(ximage = XCreateImage(dpy, ds->visual, ds->depth, ZPixmap, 0, NULL, w, h, 32, 0))) {
		EPRINTF("could not create ximage %ux%ux%u\n", w, h, ds->depth);
		goto error;
	}
	if (!(ximage->data = calloc(ximage->bytes_per_line, ximage->height))) {
		EPRINTF("could not allocate data for ximage %ux%ux%u\n", w, h, ds->depth);
		goto error;
	}
	for (p = data, j = 0; j < ximage->height; j++) {
		for (i = 0; i < ximage->width; i++, p++) {
			unsigned long pixel = *p & 0xffffffffUL;
#if 0
			if (!(pixel & 0xff000000))
				pixel = 0;
#endif
			alpha |= pixel & 0xff000000UL;
			XPutPixel(ximage, i, j, pixel);
		}
	}
	if (alpha == 0UL) {
		XPRINTF("data icon has no alpha!\n");
		for (p = data, j = 0; j < ximage->height; j++)
			for (i = 0; i < ximage->width; i++, p++)
				XPutPixel(ximage, i, j, *p | 0xff000000UL);
	}
#ifdef CROPSCALE
	if (ximage->height > th)
		ximage = crop_image(ds, ximage);
#endif				/* CROPSCALE */
#ifdef DOWNSCALE
	if (ximage->height > th) {
		unsigned tw = floor((double) ximage->width * (double) th / (double) ximage->height);
		XPRINTF("scaling image %ux%ux%u to %ux%u\n", ximage->width, ximage->height, ximage->depth, tw, th);
		ximage = dn_scale_image(ds, ximage, tw, th, False);
		XPRINTF("scaled image to %ux%ux%u\n",  ximage->width, ximage->height, ximage->depth);
	}
#endif				/* DOWNSCALE */
	if (!(draw = XCreatePixmap(dpy, ds->drawable, ximage->width, ximage->height, ximage->depth))) {
		EPRINTF("could not create pixmap\n");
		goto error;
	}
	XPutImage(dpy, draw, ds->dc.gc, ximage, 0, 0, 0, 0, ximage->width, ximage->height);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		if ((pict = XRenderCreatePicture(dpy, draw, ds->format, pamask, &pa))) {
			bi = *bis;
			px = &bi->px;
			px->x = px->y = px->b = 0;
			px->d = ximage->depth;
			px->w = ximage->width;
			px->h = ximage->height;
			if (ximage->height > th) {
				XPRINTF("transforming image from h = %u to %u\n", ximage->height, th);
				/* get XRender to scale the image for us */
				XDouble scale = (XDouble) th / (XDouble) ximage->height;
				/* *INDENT-OFF* */
				XTransform trans = {
					{ { XDoubleToFixed(1.0), 0, 0 },
					  { 0, XDoubleToFixed(1.0), 0 },
					  { 0, 0, XDoubleToFixed(scale) } }
				};
				/* *INDENT-ON* */
#ifdef FILTERPIC
#ifdef NEARESTFL
				XRenderSetPictureFilter(dpy, pict, FilterNearest, NULL, 0);
#else
				XRenderSetPictureFilter(dpy, pict, FilterBilinear, NULL, 0);
#endif
#endif				/* FILTERPIC */
				XRenderSetPictureTransform(dpy, pict, &trans);
				px->w = lround(ximage->width * scale);
				px->h = th;
			}
			if (px->pixmap.pict) {
				XRenderFreePicture(dpy, px->pixmap.pict);
				px->pixmap.pict = None;
			}
			px->pixmap.pict = pict;
			bi->present = True;
			result = True;
		} else
			EPRINTF("could not create picture\n");
	}
      error:
	if (draw)
		XFreePixmap(dpy, draw);
	if (ximage)
		XDestroyImage(ximage);
	return (result);
}

#ifdef JUSTRENDER
Bool
render_createpngicon(AScreen *ds, Client *c, const char *file)
{
	Bool result = False;
#ifdef LIBPNG
	Pixmap draw = None;
	XImage *ximage;
	unsigned th = titleheight(ds);

	ximage = png_read_file_to_ximage(dpy, ds->visual, file);
	if (!ximage) {
		EPRINTF("could not read png file %s\n", file);
		goto error;
	}
	(void) th;
#ifdef CROPSCALE
	if (ximage->height > th)
		ximage = crop_image(ds, ximage);
#endif				/* CROPSCALE */
#ifdef DOWNSCALE
	if (ximage->height > th) {
		unsigned tw = floor((double) ximage->width * (double) th / (double) ximage->height);
		XPRINTF("scaling image %ux%ux%u to %ux%u\n", ximage->width, ximage->height, ximage->depth, tw, th);
		ximage = dn_scale_image(ds, ximage, tw, th, False);
		XPRINTF("scaled image to %ux%ux%u\n",  ximage->width, ximage->height, ximage->depth);
	}
#endif				/* DOWNSCALE */
	if (!(draw = XCreatePixmap(dpy, ds->drawable, ximage->width, ximage->height, ximage->depth))) {
		EPRINTF("could not create pixmap\n");
		goto error;
	}
	XPutImage(dpy, draw, ds->dc.gc, ximage, 0, 0, 0, 0, ximage->width, ximage->height);
	result = createicon_pixmap(ds, c, draw, None, ximage->width, ximage->height, ximage->depth, True);
	if (result)
		XPRINTF("created icon from %s\n", file);
      error:
	if (ximage)
		XDestroyImage(ximage);
	if (draw)
		XFreePixmap(dpy, draw);
#endif				/* LIBPNG */
	return (result);
}
#else				/* JUSTRENDER */
Bool
render_createpngicon(AScreen *ds, Client *c, const char *file)
{
	Bool result = False;
#ifdef LIBPNG
	XImage *xdraw = NULL;

	if (!(xdraw = png_read_file_to_ximage(dpy, ds->visual, file))) {
		EPRINTF("could not read png file %s\n", file);
		goto error;
	}
	result = createicon_pixmap(ds, c, xdraw, NULL, True);
	if (result)
		XPRINTF("created icon from %s\n", file);
      error:
	if (xdraw)
		XDestroyImage(xdraw);
#endif				/* LIBPNG */
	return (result);
}
#endif				/* JUSTRENDER */

Bool
render_createjpgicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use RENDER to create JPG icon %s\n", file);
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

#ifdef JUSTRENDER
Bool
render_createxpmicon(AScreen *ds, Client *c, const char *file)
{
	Bool result = False;
#ifdef XPM
	Pixmap draw = None, mask = None;
	XpmAttributes xa = {
		.visual = DefaultVisual(dpy, ds->screen),
		.colormap = DefaultColormap(dpy, ds->screen),
		.depth = DefaultDepth(dpy, ds->screen),
		.valuemask = XpmDepth | XpmVisual | XpmColormap,
	};
	int status;

	status = XpmReadFileToPixmap(dpy, ds->drawable, file, &draw, &mask, &xa);
	if (status != XpmSuccess || !draw) {
		EPRINTF("could not load xpm file %s\n", file);
		goto error;
	}
	XpmFreeAttributes(&xa);
	result = createicon_pixmap(ds, c, draw, mask, xa.width, xa.height, xa.depth, True);
	if (result)
		XPRINTF("created icon from %s\n", file);
      error:
	if (mask)
		XFreePixmap(dpy, mask);
	if (draw)
		XFreePixmap(dpy, draw);
#endif				/* XPM */
	return (result);
}
#else				/* JUSTRENDER */
Bool
render_createxpmicon(AScreen *ds, Client *c, const char *file)
{
	Bool result = False;
#ifdef XPM
	XImage *xdraw = NULL, *xmask = NULL;
	int status;

	XpmAttributes xa = {
		.visual = DefaultVisual(dpy, ds->screen),
		.colormap = DefaultColormap(dpy, ds->screen),
		.depth = DefaultDepth(dpy, ds->screen),
		.valuemask = XpmDepth | XpmVisual | XpmColormap,
	};

	status = XpmReadFileToImage(dpy, file, &xdraw, &xmask, &xa);
	if (status != XpmSuccess || !xdraw) {
		EPRINTF("could not load xpm file: %s on %s\n",
			xpm_status_string(status), file);
		goto error;
	}
	XpmFreeAttributes(&xa);
	result = createicon_pixmap(ds, c, xdraw, xmask, True);
	if (result)
		XPRINTF("created icon from %s\n", file);
      error:
	if (xmask)
		XDestroyImage(xmask);
	if (xdraw)
		XDestroyImage(xdraw);
#endif				/* XPM */
	return (result);
}
#endif				/* JUSTRENDER */

#ifdef JUSTRENDER
Bool
render_createxbmicon(AScreen *ds, Client *c, const char *file)
{
	Pixmap draw = None;
	int x, y, status;
	unsigned w, h;
	Bool result = False;

	status = XReadBitmapFile(dpy, scr->root, file, &w, &h, &draw, &x, &y);
	if (status != BitmapSuccess || !draw) {
		EPRINTF("could not load xbm file %s\n", file);
		goto error;
	}
	result = createicon_bitmap(ds, c, draw, None, w, h, True);
	if (result)
		XPRINTF("created icon from %s\n", file);
      error:
	if (draw)
		XFreePixmap(dpy, draw);
	return (result);
}
#else				/* JUSTRENDER */
Bool
render_createxbmicon(AScreen *ds, Client *c, const char *file)
{
	XImage *xdraw = NULL;
	unsigned w, h;
	int x, y, status;
	Bool result = False;

	status = XReadBitmapFileImage(dpy, ds->visual, file, &w, &h, &xdraw, &x, &y);
	if (status != BitmapSuccess || !xdraw) {
		EPRINTF("could not load xbm file: %s on %s\n", xbm_status_string(status), file);
		goto error;
	}
	result = createicon_bitmap(ds, c, xdraw, NULL, True);
	if (result)
		XPRINTF("created icon from %s\n", file);
      error:
	if (xdraw)
		XDestroyImage(xdraw);
	return (result);
}
#endif				/* JUSTRENDER */

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
	Picture dst = ds->dc.draw.pict;
	XftColor *fg, *bg;
	Geometry g = { 0, };
	ButtonImage *bi;
	AdwmPixmap *px;
	int th = titleheight(ds);

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
	(void) bg;

	if (px->pixmap.pict) {
		XRenderComposite(dpy, PictOpOver, px->pixmap.pict, px->pixmap.alph, dst,
				 0, 0, 0, 0, ec->eg.x, ec->eg.y, ec->eg.w, ec->eg.h);
		return g.w;
	} else if (px->bitmap.pict) {
		Picture fill = XRenderCreateSolidFill(dpy, &fg->color);

#if 0
		XRenderPictureAttributes pa = {
			.alpha_map = px->bitmap.pict,
			.alpha_x_origin = 0,
			.alpha_y_origin = 0,
		};
		unsigned long pamask = CPAlphaMap | CPAlphaXOrigin | CPAlphaYOrigin;
		XRenderChangePicture(dpy, fill, pamask, &pa);
#endif
		XRenderComposite(dpy, PictOpOver, fill, px->bitmap.pict, dst, 0, 0, 0, 0,
				 ec->eg.x, ec->eg.y, ec->eg.w, ec->eg.h);
		XRenderFreePicture(dpy, fill);
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
	int gap;
	XftFont *font = ds->style.font[hilite];
	struct _FontInfo *info = &ds->dc.font[hilite];
	XftColor *fcol = ds->style.color.hue[hilite];
	int drop = ds->style.drop[hilite];
	Picture src, dst;

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

	dst = ds->dc.draw.pict;
	xtrap_push(True,NULL);
	XRenderFillRectangle(dpy, PictOpSrc, dst, &col[ColBG].color, x - gap, 0,
			     w + gap * 2, h);
	xtrap_pop();
	if (drop) {
		src = XftDrawSrcPicture(xftdraw, &fcol[ColShadow]);
		XftTextRenderUtf8(dpy, PictOpOver, src, font, dst, 0, 0,
				  x + drop, y + drop, (FcChar8 *) buf, len);
	}
	src = XftDrawSrcPicture(xftdraw, &fcol[ColFG]);
	XftTextRenderUtf8(dpy, PictOpOver, src, font, dst, 0, 0,
			  x + drop, y + drop, (FcChar8 *) buf, len);
	return w + gap * 2;
}

int
render_drawsep(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw,
	       XftColor *col, int hilite, int x, int y, int mw)
{
	Geometry g;
	int b, th = titleheight(ds);

	g.x = x + th / 4;
	g.y = 0;
	g.w = th / 4 + th / 4;
	g.h = th;

	if (g.w > mw)
		return (0);

	if ((b = ds->style.border)) {
		xtrap_push(True,NULL);
		XRenderFillRectangle(dpy, PictOpOver, ds->dc.draw.pict, &col->color, g.x, g.y, b, g.h);
		xtrap_pop();
	}

	return (g.w);
}

void
render_drawdockapp(AScreen *ds, Client *c)
{
	XftColor *col;
	Picture dst, src, fill;
#if 1
	XRenderPictFormat *format = XRenderFindStandardFormat(dpy, PictStandardRGB24);
#else
	XRenderPictFormat *format = XRenderFindStandardFormat(dpy, PictStandardARGB32);
#endif

	XRenderPictureAttributes pa = {
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
		.subwindow_mode = IncludeInferiors,
	};
	unsigned long pamask = CPComponentAlpha | CPSubwindowMode;

	col = getcolor(ds, c, ColBG);

	ds->dc.x = ds->dc.y = 0;
	if (!(ds->dc.w = c->c.w))
		goto error;
	if (!(ds->dc.h = c->c.h))
		goto error;
	/* create a new picture for the dock app each time */
	if (!(dst = XRenderCreatePicture(dpy, c->frame, format, pamask, &pa)))
		goto error;
	if (c->pict.frame)
		XRenderFreePicture(dpy, c->pict.frame);
	c->pict.frame = dst;
	if (!(src = c->pict.frame) && !(src = XRenderCreatePicture(dpy, c->icon, format, pamask, &pa)))
		goto error;
	c->pict.icon = src;
	fill = XRenderCreateSolidFill(dpy, &col->color);
	XRenderComposite(dpy, PictOpSrc, fill, None, dst, 0, 0, 0, 0, 0, 0, c->c.w, c->c.h);
	XRenderComposite(dpy, PictOpOver, src, None, dst, 0, 0, 0, 0, c->r.x, c->r.y, c->r.w, c->r.h);
	XRenderFreePicture(dpy, fill);
      error:
	/* note that ParentRelative dockapps need the background set to the foreground */
	XSetWindowBackground(dpy, c->frame, col->pixel);
}

void
render_drawnormal(AScreen *ds, Client *c)
{
	size_t i;
	Picture dst;
	XRenderColor *bg, *bc;
	XRenderPictureAttributes pa = {
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
	};
	unsigned long pamask = CPComponentAlpha;

	if (!c->title)
		return;

	ds->dc.x = ds->dc.y = 0;
	ds->dc.w = c->c.w;
	ds->dc.h = ds->style.titleheight;
	if (ds->dc.draw.w < ds->dc.w) {
		ds->dc.draw.w = ds->dc.w;
	}
	/* try freeing the old picture before creating the new one! */
	if (c->pict.title) {
		XRenderFreePicture(dpy, c->pict.title);
		c->pict.title = None;
	}
	if (!(dst = XRenderCreatePicture(dpy, c->title, ds->format, pamask, &pa))) {
		EPRINTF("could not create title picture\n");
		return;
	}
	c->pict.title = dst;
	ds->dc.draw.pict = dst;

	bg = &getcolor(ds, c, ColBG)->color;
	bc = &getcolor(ds, c, ColBorder)->color;

	{
	xtrap_push(True,NULL);
	XRenderFillRectangle(dpy, PictOpSrc, dst, bg, ds->dc.x, ds->dc.y, ds->dc.w, ds->dc.h);
	xtrap_pop();
	}

	/* Don't know about this... */
	if (ds->dc.w < textw(ds, c->name, gethilite(c))) {
		ds->dc.w -= elementw(ds, c, CloseBtn);
		render_drawtext(ds, c->name, dst, ds->dc.draw.xft, gethues(ds, c), gethilite(c), ds->dc.x, ds->dc.y, ds->dc.w);
		render_drawbutton(ds, c, CloseBtn, gethues(ds, c), ds->dc.w);
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
		XPRINTF(__CFMTS(c) "drawing line from +%d+%d to +%d+%d\n", __CARGS(c), 0, ds->dc.h - 1, ds->dc.w, ds->dc.h - 1);
		xtrap_push(True,NULL);
		XRenderFillRectangle(dpy, PictOpOver, dst, bc, 0, ds->dc.h - ds->style.border, ds->dc.w, ds->style.border);
		xtrap_pop();
	}
	if (!c->grips)
		return;

	ds->dc.x = ds->dc.y = 0;
	ds->dc.w = c->c.w;
	ds->dc.h = ds->style.gripsheight;
	/* try freeing the old picture before creating the new one! */
	if (c->pict.grips) {
		XRenderFreePicture(dpy, c->pict.grips);
		c->pict.grips = None;
	}
	if (!(dst = XRenderCreatePicture(dpy, c->grips, ds->format, pamask, &pa))) {
		EPRINTF("could not create grips picture\n");
		return;
	}
	c->pict.grips = dst;

	{
	xtrap_push(True,NULL);
	XRenderFillRectangle(dpy, PictOpSrc, dst, bg, ds->dc.x, ds->dc.y, ds->dc.w, ds->dc.h);
	xtrap_pop();
	}

	if (ds->style.outline) {
		xtrap_push(True,NULL);
		XRenderFillRectangle(dpy, PictOpOver, dst, bc, 0, 0, ds->dc.w, ds->style.border);
		/* needs to be adjusted to do ds->style.gripswidth instead */
		ds->dc.x = ds->dc.w / 2 - ds->dc.w / 5;
		XRenderFillRectangle(dpy, PictOpOver, dst, bc, ds->dc.x, 0, ds->style.border, ds->dc.h);
		ds->dc.x = ds->dc.w / 2 + ds->dc.w / 5;
		XRenderFillRectangle(dpy, PictOpOver, dst, bc, ds->dc.x, 0, ds->style.border, ds->dc.h);
		xtrap_pop();
	}
}

Bool
render_initpng(char *path, AdwmPixmap *px)
{
#ifdef LIBPNG
	Pixmap draw = None;
	Picture pict = None;
	XRenderPictureAttributes pa = {
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
	};
	unsigned long pamask = CPComponentAlpha;

	XImage *ximage;

	ximage = png_read_file_to_ximage(dpy, scr->visual, path);
	if (!ximage) {
		EPRINTF("could not read png file %s\n", path);
		goto error;
	}

	if (!(draw = XCreatePixmap(dpy, scr->drawable, ximage->width, ximage->height, ximage->depth))) {
		EPRINTF("could not create pixmap\n");
		goto error;
	}
	XPutImage(dpy, draw, scr->dc.gc, ximage, 0, 0, 0, 0, ximage->width, ximage->height);

	if (!(pict = XRenderCreatePicture(dpy, draw, scr->format, pamask, &pa))) {
		EPRINTF("could not create picture\n");
		goto error;
	}
	if (px->pixmap.pict) {
		XRenderFreePicture(dpy, px->pixmap.pict);
		px->pixmap.pict = None;
	}
	if (px->file) {
		free(px->file);
		px->file = NULL;
	}
	px->pixmap.pict = pict;
	px->file = path;
	px->x = px->y = px->b = 0;
	px->w = ximage->width;
	px->h = ximage->height;
	px->d = ximage->depth;
#if 0
	unsigned th = titleheight(scr);
	if (px->h > th) {
		/* read lower down into image to clip top and bottom by same amount */
		px->y += (px->h - th) / 2;
		px->h = th;
	}
#endif
      error:
	if (ximage)
		XDestroyImage(ximage);
	if (draw)
		XFreePixmap(dpy, draw);
	return (pict ? True : False);
#else
	return (False);
#endif
}

Bool
render_initjpg(char *path, AdwmPixmap *px)
{
	return (False);
}

Bool
render_initsvg(char *path, AdwmPixmap *px)
{
	return (False);
}

#ifdef JUSTRENDER
Bool
render_initxpm(char *path, AdwmPixmap *px)
{
#ifdef XPM
	Pixmap draw = None, mask = None;
	Picture pict = None;
	int status;
	XRenderPictFormat *format;
	XpmAttributes xa = {
		.visual = DefaultVisual(dpy, scr->screen),
		.colormap = DefaultColormap(dpy, scr->screen),
		.depth = DefaultDepth(dpy, scr->screen),
		.valuemask = XpmVisual| XpmColormap| XpmDepth,
	};
	XRenderPictureAttributes pa = {
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
	};
	unsigned long pamask = CPComponentAlpha;

	status = XpmReadFileToPixmap(dpy, scr->drawable, path, &draw, &mask, &xa);
	if (status != XpmSuccess || !draw) {
		EPRINTF("could not load xpm file %s\n", path);
		goto error;
	}
	XpmFreeAttributes(&xa);
	if (mask) {
		pa.clip_mask = mask;
		pa.clip_x_origin = 0;
		pa.clip_y_origin = 0;
		pamask |= CPClipMask | CPClipXOrigin | CPClipYOrigin;
	}

	format = XRenderFindStandardFormat(dpy, PictStandardRGB24);

	if (!(pict = XRenderCreatePicture(dpy, draw, format, pamask, &pa))) {
		EPRINTF("could not create picture\n");
		goto error;
	}
	if (px->pixmap.pict) {
		XRenderFreePicture(dpy, px->pixmap.pict);
		px->pixmap.pict = None;
	}
	if (px->file) {
		free(px->file);
		px->file = NULL;
	}
	px->pixmap.pict = pict;
	px->file = path;
	px->x = px->y = px->b = 0;
	px->w = xa.width;
	px->h = xa.height;
	px->d = xa.depth;
#if 0
	unsigned th = titleheight(scr);
	if (px->h > th) {
		/* read lower down into image to clip top and bottom by same amount */
		px->y += (px->h - th) / 2;
		px->h = th;
	}
#endif
      error:
	if (draw)
		XFreePixmap(dpy, draw);
	if (mask)
		XFreePixmap(dpy, mask);
	return (pict ? True : False);
#else
	return (False);
#endif				/* !XPM */
}
#else				/* JUSTRENDER */
Bool
render_initxpm(char *path, AdwmPixmap *px)
{
#ifdef XPM
	Pixmap draw = None;
	Picture pict = None;
	XRenderPictureAttributes pa = {
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
	};
	unsigned long pamask = CPComponentAlpha;

	XImage *xdraw = NULL, *xmask = NULL, *ximage = NULL;
	unsigned w, h, d = scr->depth;
	int status;

	XpmAttributes xa = {
		.visual = scr->visual,
		.colormap = scr->colormap,
		.depth = scr->depth,
		.valuemask = XpmVisual| XpmColormap| XpmDepth,
	};

	status = XpmReadFileToImage(dpy, path, &xdraw, &xmask, &xa);
	if (status != XpmSuccess || !xdraw) {
		EPRINTF("could not load xpm file %s\n", path);
		goto error;
	}
	XpmFreeAttributes(&xa);
	if (!(ximage = combine_pixmap_and_mask(scr, xdraw, xmask))) {
		EPRINTF("could not combine images\n");
		goto error;
	}
	w = ximage->width;
	h = ximage->height;

	if (!(draw = XCreatePixmap(dpy, scr->drawable, w, h, d))) {
		EPRINTF("could not create pixmap\n");
		goto error;
	}
	XPutImage(dpy, draw, scr->dc.gc, ximage, 0, 0, 0, 0, w, h);

	if (!(pict = XRenderCreatePicture(dpy, draw, scr->format, pamask, &pa))) {
		EPRINTF("could not create picture\n");
		goto error;
	}
	if (px->pixmap.pict) {
		XRenderFreePicture(dpy, px->pixmap.pict);
		px->pixmap.pict = None;
	}
	if (px->file) {
		free(px->file);
		px->file = NULL;
	}
	px->pixmap.pict = pict;
	px->file = path;
	px->x = px->y = px->b = 0;
	px->w = w;
	px->h = h;
#if 0
	unsigned th = titleheight(scr);
	if (px->h > th) {
		/* read lower down into image to clip top and bottom by same amount */
		px->y += (px->h - th) / 2;
		px->h = th;
	}
#endif
      error:
	if (xdraw)
		XDestroyImage(xdraw);
	if (xmask)
		XDestroyImage(xmask);
	if (ximage)
		XDestroyImage(ximage);
	if (draw)
		XFreePixmap(dpy, draw);
	return (pict ? True : False);
#else
	return (False);
#endif				/* !XPM */
}
#endif				/* JUSTRENDER */

Bool
render_initxbm(char *path, AdwmPixmap *px)
{
	Pixmap draw;
	Picture pict = None;
	XRenderPictFormat *format;
	int status;
	XRenderPictureAttributes pa = {
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
	};
	unsigned long pamask = CPComponentAlpha;

	status = XReadBitmapFile(dpy, scr->root, path, &px->w, &px->h, &draw, &px->x, &px->y);
	if (status != BitmapSuccess || !draw) {
		EPRINTF("could not load xbm file %s\n", path);
		goto error;
	}
	format = XRenderFindStandardFormat(dpy, PictStandardA1);

	if (!(pict = XRenderCreatePicture(dpy, draw, format, pamask, &pa))) {
		EPRINTF("could not create picture\n");
		goto error;
	}
	if (px->bitmap.pict) {
		XRenderFreePicture(dpy, px->bitmap.pict);
		px->bitmap.pict = None;
	}
	if (px->file) {
		free(px->file);
		px->file = NULL;
	}
	px->bitmap.pict = pict;
	px->file = path;
	px->x = px->y = px->b = 0;
	px->w = px->w;
	px->h = px->h;
      error:
	if (draw)
		XFreePixmap(dpy, draw);
	return (pict ? True : False);
}

Bool
render_initxbmdata(const unsigned char *bits, int w, int h, AdwmPixmap *px)
{
	Pixmap draw;
	Picture pict = None;
	XRenderPictFormat *format;
	XRenderPictureAttributes pa = {
#ifdef CMPALPHA
		.component_alpha = True,
#else				/* CMPALPHA */
		.component_alpha = False,
#endif				/* CMPALPHA */
	};
	unsigned long pamask = CPComponentAlpha;

	if (!(draw = XCreateBitmapFromData(dpy, scr->drawable, (char *) bits, w, h))) {
		EPRINTF("could not load xbm data\n");
		goto error;
	}
	format = XRenderFindStandardFormat(dpy, PictStandardA1);

	if (!(pict = XRenderCreatePicture(dpy, draw, format, pamask, &pa))) {
		EPRINTF("could not create picture\n");
		goto error;
	}
	if (px->bitmap.pict) {
		XRenderFreePicture(dpy, px->bitmap.pict);
		px->bitmap.pict = None;
	}
	if (px->file) {
		free(px->file);
		px->file = NULL;
	}
	px->bitmap.pict = pict;
	px->x = px->y = px->b = 0;
	px->w = w;
	px->h = h;
      error:
	if (draw)
		XFreePixmap(dpy, draw);
	return (pict ? True : False);
}

#endif				/* defined RENDER */

