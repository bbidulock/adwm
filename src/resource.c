/* See COPYING file for copyright and license details. */
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
#include "config.h"
#ifdef IMLIB2
#include <Imlib2.h>
#endif
#ifdef XPM
#include <X11/xpm.h>
#endif

XrmDatabase xresdb;

const char *readres(const char *name, const char *clas, const char *defval);
void getbitmap(const unsigned char *bits, int width, int height, AdwmBitmap * bitmap);
void getappearance(const char *descrip, Appearance *appear);
void getxcolor(const char *color, const char *defcol, XColor *xcol);
void getxftcolor(const char *color, const char *defcol, XftColor *xftcol);
void freexftcolor(XftColor *xftcol);
void getbool(const char *name, const char *clas, const char *true_val, Bool defval,
	     Bool *result);
void getfont(const char *font, const char *deffont, AdwmFont *afont);
void freefont(AdwmFont *afont);
void readtexture(const char *name, const char *clas, Texture *t, const char *defcol,
		 const char *oppcol);
void freetexture(Texture *t);
void getpixmap(const char *file, AdwmPixmap *p);
void getshadow(const char *descrip, TextShadow * shadow);

const char *
readres(const char *name, const char *clas, const char *defval)
{
	char *type;
	XrmValue value;

	XrmGetResource(xresdb, name, clas, &type, &value);
	if (value.addr)
		return value.addr;
	return defval;
}

void
getbitmap(const unsigned char *bits, int width, int height, AdwmBitmap * bitmap)
{
	bitmap->width = width;
	bitmap->height = height;
	bitmap->pixmap = XCreateBitmapFromData(dpy,
					       scr->root, (char *) bits, width, height);
	if (!bitmap->pixmap)
		eprint("ERROR: cannot create bitmap\n");
}

void
getappearance(const char *descrip, Appearance *appear)
{
	appear->pattern = PatternSolid;
	appear->gradient = GradientDiagonal;
	appear->relief = ReliefRaised;
	appear->interlaced = False;
	appear->border = False;

	if (strcasestr(descrip, "parentrelative"))
		appear->pattern = PatternParent;
	else if (strcasestr(descrip, "gradient")) {
		appear->pattern = PatternGradient;
		if (strcasestr(descrip, "crossdiagonal"))
			appear->gradient = GradientCrossDiagonal;
		else if (strcasestr(descrip, "rectangle"))
			appear->gradient = GradientRectangle;
		else if (strcasestr(descrip, "pyramid"))
			appear->gradient = GradientPyramid;
		else if (strcasestr(descrip, "pipecross"))
			appear->gradient = GradientPipeCross;
		else if (strcasestr(descrip, "elliptic"))
			appear->gradient = GradientElliptic;
		else if (strcasestr(descrip, "horizontal"))
			appear->gradient = GradientHorizontal;
		else if (strcasestr(descrip, "splitvertical"))
			appear->gradient = GradientSplitVertical;
		else if (strcasestr(descrip, "vertical"))
			appear->gradient = GradientVertical;
		else if (strcasestr(descrip, "diagonal"))
			appear->gradient = GradientDiagonal;
		else
			appear->gradient = GradientDiagonal;
	} else if (strcasestr(descrip, "solid"))
		appear->pattern = PatternSolid;
	else if (strcasestr(descrip, "pixmap"))
		appear->pattern = PatternPixmap;
	else
		appear->pattern = PatternSolid;
	if (strcasestr(descrip, "bevel2"))
		appear->bevel = Bevel2;
	else if (strcasestr(descrip, "bevel1"))
		appear->bevel = Bevel1;
	else
		appear->bevel = Bevel1;
	if (strcasestr(descrip, "sunken"))
		appear->relief = ReliefSunken;
	else if (strcasestr(descrip, "flat"))
		appear->relief = ReliefFlat;
	else if (strcasestr(descrip, "raised"))
		appear->relief = ReliefRaised;
	else
		appear->relief = ReliefRaised;

	appear->interlaced = strcasestr(descrip, "interlaced") ? True : False;
	appear->border = strcasestr(descrip, "border") ? True : False;
}

void
getxcolor(const char *color, const char *defcol, XColor *xcol)
{
	XColor exact;

	if (XAllocNamedColor(dpy, DefaultColormap(dpy, scr->screen), color, xcol, &exact))
		return;
	if (XAllocNamedColor(dpy, DefaultColormap(dpy, scr->screen), color, xcol, &exact))
		return;
	eprint("ERROR: cannot allocate color '%s' or '%s'\n", color, defcol);
}

void
getxftcolor(const char *color, const char *defcol, XftColor *xftcol)
{
	if (XftColorAllocName(dpy, DefaultVisual(dpy, scr->screen),
			      DefaultColormap(dpy, scr->screen), color, xftcol))
		return;
	if (XftColorAllocName(dpy, DefaultVisual(dpy, scr->screen),
			      DefaultColormap(dpy, scr->screen), defcol, xftcol))
		return;
	eprint("ERROR: cannot allocate color '%s' or '%s'\n", color, defcol);
}

void
freexftcolor(XftColor *xftcol)
{
	XftColorFree(dpy, DefaultVisual(dpy, scr->screen),
		     DefaultColormap(dpy, scr->screen), xftcol);
}

void
getbool(const char *name, const char *clas, const char *true_val, Bool defval,
	Bool *result)
{
	const char *res;
	const char *tval = true_val ? : "true";

	if ((res = readres(name, clas, NULL)))
		*result = strcasestr(res, tval) ? True : False;
	else
		*result = defval;
}

void
getfont(const char *font, const char *deffont, AdwmFont *afont)
{
	const char *used;

	do {
		used = font;
		if ((afont->font = XftFontOpenXlfd(dpy, scr->screen, used)))
			break;
		if ((afont->font = XftFontOpenName(dpy, scr->screen, used)))
			break;
		used = deffont;
		if ((afont->font = XftFontOpenXlfd(dpy, scr->screen, used)))
			break;
		if ((afont->font = XftFontOpenName(dpy, scr->screen, used)))
			break;
		eprint("ERROR: cannot load font '%s' or '%s'\n", font, deffont);
	}
	while (0);

	XftTextExtentsUtf8(dpy, afont->font,
			   (const unsigned char *) used, strlen(used), &afont->extents);
	afont->height = afont->font->ascent + afont->font->descent + 1;
	afont->ascent = afont->font->ascent;
	afont->descent = afont->font->descent;
	afont->width = afont->font->max_advance_width;
}

void
freefont(AdwmFont *afont)
{
	if (afont->font)
		XftFontClose(dpy, afont->font);
	afont->font = NULL;
}

void
getpixmap(const char *file, AdwmPixmap *p)
{
	if (!file)
		goto done;
#ifdef XPM
	else if (strstr(file, ".xpm") && strlen(strstr(file, ".xpm")) == 4) {
		XpmAttributes xa = { 0, };

		if (XpmReadFileToPixmap(dpy, scr->root, file, &p->pixmap, &p->mask, &xa)
		    == Success) {
			p->file = file;
			p->depth = (xa.valuemask & XpmDepth) ? xa.depth : 0;
			p->width = xa.width;
			p->height = xa.height;
			p->x = xa.x_hotspot;
			p->y = xa.y_hotspot;
			return;
		}
		DPRINTF("could not load pixmap file %s\n", file);
		goto done;
	}
#endif
#ifdef IMLIB2
	else if (!strstr(file, ".xbm") || strlen(strstr(file, ".xbm")) != 4) {
		Imlib_Image image;

		imlib_context_push(scr->context);
		imlib_context_set_mask(None);

		if (!(image = imlib_load_image(file))) {
			if (file[0] != '/') {
				/* TODO: look for it else where */
			}
		}
		if (image) {
			imlib_context_set_image(image);
			imlib_context_set_mask(None);
			p->file = file;
			p->width = imlib_image_get_width();
			p->height = imlib_image_get_height();
			p->x = p->y = 0;
			image_render_pixmaps_for_whole_image(&p->pixmap, &p->mask);
			return;
		}
		DPRINTF("could not load image file %s\n", file);
		goto done;
	}
#endif
	else if (strstr(file, ".xbm") && strlen(strstr(file, ".xbm")) == 4) {
		if (BitmapSuccess == XReadBitmapFile(dpy, scr->root, file, &p->width,
						     &p->height, &p->pixmap, &p->x,
						     &p->y)) {
			p->file = file;
			p->depth = 2;
			return;
		}
		DPRINTF("could not load bitmap file %s\n", file);
		goto done;
	}
      done:
	p->file = NULL;
	p->pixmap = p->mask = None;
	p->depth = p->width = p->height = 0;
	p->x = p->y = 0;
}

void
getshadow(const char *descrip, TextShadow * s)
{
	s->shadow = False;
	if (descrip) {
		const char *p;

		s->shadow = strcasestr(descrip, "shadow=y") ? True : False;
		if (!s->shadow)
			return;
		if ((p = strcasestr(descrip, "shadowtint="))) {
			int tint;

			tint = atoi(p + 11);
			s->tranparency = abs(tint);
			if (tint < 0)
				getxftcolor("black", "black", &s->color);
			else
				getxftcolor("white", "white", &s->color);
		}
		if ((p = strcasestr(descrip, "shadowoffset="))) {
			s->offset = atoi(p + 13);
		}
		return;
	}
}

void
readtexture(const char *name, const char *clas, Texture *t, const char *defcol,
	    const char *oppcol)
{
	static char fname[256], fclas[256];
	const char *res;

	snprintf(fname, sizeof(fname), "%s", name);
	snprintf(fclas, sizeof(fclas), "%s", clas);
	res = readres(fname, fclas, "flat solid");
	snprintf(fname, sizeof(fname), "%s.appearance", name);
	snprintf(fclas, sizeof(fclas), "%s.Appearance", clas);
	res = readres(fname, fclas, res);
	getappearance(res, &t->appearance);

	snprintf(fname, sizeof(fname), "%s.color", name);
	snprintf(fclas, sizeof(fclas), "%s.Color", clas);
	res = readres(fname, fclas, defcol);
	snprintf(fname, sizeof(fname), "%s.color1", name);
	snprintf(fclas, sizeof(fclas), "%s.Color1", clas);
	res = readres(fname, fclas, res);
	getxcolor(res, defcol, &t->color);

	snprintf(fname, sizeof(fname), "%s.colorTo", name);
	snprintf(fclas, sizeof(fclas), "%s.ColorTo", clas);
	res = readres(fname, fclas, defcol);
	snprintf(fname, sizeof(fname), "%s.color2", name);
	snprintf(fclas, sizeof(fclas), "%s.Color2", clas);
	res = readres(fname, fclas, res);
	getxcolor(res, defcol, &t->colorTo);

	snprintf(fname, sizeof(fname), "%s.color", name);
	snprintf(fclas, sizeof(fclas), "%s.Color", clas);
	res = readres(fname, fclas, defcol);
	snprintf(fname, sizeof(fname), "%s.picColor", name);
	snprintf(fclas, sizeof(fclas), "%s.PicColor", clas);
	res = readres(fname, fclas, res);
	getxcolor(res, defcol, &t->picColor);

	snprintf(fname, sizeof(fname), "%s.textColor", name);
	snprintf(fclas, sizeof(fclas), "%s.TextColor", clas);
	res = readres(fname, fclas, oppcol);
	getxftcolor(res, oppcol, &t->textColor);

	snprintf(fname, sizeof(fname), "%s.textColor.opacity", name);
	snprintf(fclas, sizeof(fclas), "%s.TextColor.Opacity", clas);
	res = readres(fname, fclas, "0");
	t->textOpacity = strtoul(res, NULL, 0);
	if (t->textOpacity > 100)
		t->textOpacity = 100;

	snprintf(fname, sizeof(fname), "%s.textShadowColor", name);
	snrpintf(fclas, sizeof(fclas), "%s.TextShadwoColor", clas);
	res = readres(fname, fclas, defcol);
	getxftcolor(res, defcol, &t->textShadowColor);

	snprintf(fname, sizeof(fname), "%s.textShadowColor.opacity", name);
	snprintf(fclas, sizeof(fclas), "%s.TextShadowColor.Opacity", clas);
	res = readres(fname, fclas, "0");
	t->textOpacity = strtoul(res, NULL, 0);
	if (t->textShadowOpacity > 100)
		t->textShadowOpacity = 100;

	snprintf(fname, sizeof(fname), "%s.textShadowXOffset", name);
	snprintf(fclas, sizeof(fclas), "%s.TextShadowXOffset", clas);
	res = readres(fname, fclas, "0");
	t->textShadowXOffset = strtoul(res, NULL, 0);

	snprintf(fname, sizeof(fname), "%s.textShadowYOffset", name);
	snprintf(fclas, sizeof(fclas), "%s.TextShadowYOffset", clas);
	res = readres(fname, fclas, "0");
	t->textShadowYOffset = strtoul(res, NULL, 0);

	snprintf(fname, sizeof(fname), "%s.borderColor", name);
	snprintf(fclas, sizeof(fclas), "%s.BorderColor", clas);
	res = readres(fname, fclas, oppcol);
	getxcolor(res, oppcol, &t->borderColor);

	res = readres("borderWidth", "BorderWidth", "1");
	snprintf(fname, sizeof(fname), "%s.borderWidth", name);
	snprintf(fclas, sizeof(fclas), "%s.BorderWidth", clas);
	res = readres(fname, fclas, res);
	t->borderWidth = strtoul(res, NULL, 0);

	snprintf(fname, sizeof(fname), "%s.color", name);
	snprintf(fclas, sizeof(fclas), "%s.Color", clas);
	res = readres(fname, fclas, defcol);
	snprintf(fname, sizeof(fname), "%s.backgroundColor", name);
	snprintf(fclas, sizeof(fclas), "%s.BackgroundColor", clas);
	res = readres(fname, fclas, res);
	getxcolor(res, defcol, &t->backgroundColor);

	snprintf(fname, sizeof(fname), "%s.picColor", name);
	snprintf(fclas, sizeof(fclas), "%s.PicColor", clas);
	res = readres(fname, fclas, oppcol);
	snprintf(fname, sizeof(fname), "%s.foregroundColor", name);
	snprintf(fclas, sizeof(fclas), "%s.ForegroundColor", clas);
	res = readres(fname, fclas, res);
	getxcolor(res, oppcol, &t->foregroundColor);

	snprintf(fname, sizeof(fname), "%s.opacity", name);
	snprintf(fclas, sizeof(fclas), "%s.Opacity", clas);
	res = readres(fname, fcclas, "0");
	t->opacity = strtoul(res, NULL, 0);
	if (t->opacity > 100)
		t->opacity = 100;

	snprintf(fname, sizeof(fname), "%s.pixmap", name);
	snprintf(fclas, sizeof(fclas), "%s.Pixmap", clas);
	res = readres(fname, fclas, NULL);
	getpixmap(res, &t->pixmap);

	snprintf(fname, sizeof(fname), "%s.border", name);
	snprintf(fclas, sizeof(fclas), "%s.border", clas);
	res = readres(fname, fclas, "{ 0, 0, 0, 0 }");
	sscanf(res, "{ %u, %u, %u, %u }", &t->borders[0], &t->borders[1], &t->borders[2],
	       &t->borders[3]);
}

void
freetexture(Texture *t)
{
	freexftcolor(&t->textColor);
}

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
