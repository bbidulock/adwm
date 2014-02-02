/*
 *  echinus wm written by Alexander Polakov <polachok@gmail.com>
 *  this file contains code related to drawing
 */
#include <regex.h>
#include <ctype.h>
#include <assert.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include "echinus.h"
#include "config.h"
#ifdef IMLIB2
#include "Imlib2.h"
#endif
#ifdef XPM
#include <X11/xpm.h>
#endif

enum { Normal, Selected };
enum { AlignLeft, AlignCenter, AlignRight };	/* title position */

static unsigned int textnw(EScreen *ds, const char *text, unsigned int len);
static unsigned int textw(EScreen *ds, const char *text);

static int
drawtext(EScreen *ds, const char *text, Drawable drawable, XftDraw *xftdrawable,
    unsigned long col[ColLast], int x, int y, int mw) {
	int w, h;
	char buf[256];
	unsigned int len, olen;

	if (!text)
		return 0;
	olen = len = strlen(text);
	w = 0;
	if (len >= sizeof buf)
		len = sizeof buf - 1;
	memcpy(buf, text, len);
	buf[len] = 0;
	h = ds->style.titleheight;
	y = ds->dc.h / 2 + ds->dc.font.ascent / 2 - 1 - ds->style.outline;
	x += ds->dc.font.height / 2;
	/* shorten text if necessary */
	while (len && (w = textnw(ds, buf, len)) > mw) {
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
	XSetForeground(dpy, ds->dc.gc, col[ColBG]);
	XFillRectangle(dpy, drawable, ds->dc.gc, x - ds->dc.font.height / 2, 0,
	    w + ds->dc.font.height, h);
	XftDrawStringUtf8(xftdrawable,
	    (col == ds->style.color.norm) ? ds->style.color.font[Normal] : ds->style.color.font[Selected],
	    ds->style.font, x, y, (unsigned char *) buf, len);
	return w + ds->dc.font.height;
}

static int
drawbutton(EScreen *ds, Drawable d, Button btn, unsigned long col[ColLast], int x, int y)
{
	if (!btn.action)
		return 0;
#if defined IMLIB2 || defined XPM
	if (btn.pixmap) {
		DPRINTF("Copying pixmap 0x%lx with geom %dx%d+%d+%d to drawable 0x%lx\n",
			btn.pixmap, btn.pw, btn.ph, x, y + btn.py, d);
		XCopyArea(dpy, btn.pixmap, d, ds->dc.gc, 0, btn.po, btn.pw, btn.ph, x, y + btn.py);
	} else
#endif
	if (btn.bitmap) {
		XSetForeground(dpy, ds->dc.gc, col[ColBG]);
		XFillRectangle(dpy, d, ds->dc.gc, x, 0, ds->dc.h, ds->dc.h);
		XSetForeground(dpy, ds->dc.gc, btn.pressed ? col[ColFG] : col[ColButton]);
		XSetBackground(dpy, ds->dc.gc, col[ColBG]);
		XCopyPlane(dpy, btn.bitmap, d, ds->dc.gc, 0, 0, btn.pw, btn.ph, x, y + btn.py, 1);
		return ds->dc.h;
	}
	return 0;
}

static int
drawelement(EScreen *ds, char which, int x, int position, Client *c) {
	int w;
	unsigned int j;
	unsigned long *color = c == sel ? ds->style.color.sel : ds->style.color.norm;

	switch (which) {
	case 'T':
		w = 0;
		for (j = 0; j < ds->ntags; j++) {
			if (c->tags[j])
				w += drawtext(ds, ds->tags[j], c->drawable, c->xftdraw,
				    color, ds->dc.x, ds->dc.y, ds->dc.w);
		}
		break;
	case '|':
		XSetForeground(dpy, ds->dc.gc, color[ColBorder]);
		XDrawLine(dpy, c->drawable, ds->dc.gc, ds->dc.x + ds->dc.h / 4, 0,
		    ds->dc.x + ds->dc.h / 4, ds->dc.h);
		w = ds->dc.h / 2;
		break;
	case 'N':
		w = drawtext(ds, c->name, c->drawable, c->xftdraw, color, ds->dc.x, ds->dc.y, ds->dc.w);
		break;
	case 'I':
		ds->button[Iconify].x = ds->dc.x;
		w = drawbutton(ds, c->drawable, ds->button[Iconify], color,
		    ds->dc.x, ds->dc.h / 2 - ds->button[Iconify].ph / 2);
		break;
	case 'M':
		ds->button[Maximize].x = ds->dc.x;
		w = drawbutton(ds, c->drawable, ds->button[Maximize], color,
		    ds->dc.x, ds->dc.h / 2 - ds->button[Maximize].ph / 2);
		break;
	case 'C':
		ds->button[Close].x = ds->dc.x;
		w = drawbutton(ds, c->drawable, ds->button[Close], color, ds->dc.x,
		    ds->dc.h / 2 - ds->button[Maximize].ph / 2);
		break;
	default:
		w = 0;
		break;
	}
	return w;
}

static int
elementw(EScreen *ds, char which, Client *c) {
	int w;
	unsigned int j;

	switch (which) {
	case 'I':
		return ds->button[Iconify].pw;
	case 'M':
		return ds->button[Maximize].pw;
	case 'C':
		return ds->button[Iconify].pw;
	case 'N':
		return textw(ds, c->name);
	case 'T':
		w = 0;
		for (j = 0; j < ds->ntags; j++) {
			if (c->tags[j])
				w += textw(ds, ds->tags[j]);
		}
		return w;
	case '|':
		return ds->dc.h / 2;
	}
	return 0;
}

void
drawclient(Client *c) {
	size_t i;
	EScreen *ds;

	/* might be drawing a client that is not on the current screen */
	if (!(ds = getscreen(c->win))) {
		DPRINTF("What? no screen for window 0x%lx???\n", c->win);
		return;
	}
	if (ds->style.opacity) {
		setopacity(c, c == sel ? OPAQUE : ds->style.opacity);
	}
	if (!isvisible(c, NULL))
		return;
	if (!c->title)
		return;
	ds->dc.x = ds->dc.y = 0;
	ds->dc.w = c->c.w;
	ds->dc.h = ds->style.titleheight;
	XftDrawChange(c->xftdraw, c->drawable);
	XSetForeground(dpy, ds->dc.gc, c == sel ? ds->style.color.sel[ColBG] : ds->style.color.norm[ColBG]);
	XSetLineAttributes(dpy, ds->dc.gc, ds->style.border, LineSolid, CapNotLast, JoinMiter);
	XFillRectangle(dpy, c->drawable, ds->dc.gc, ds->dc.x, ds->dc.y, ds->dc.w, ds->dc.h);
	if (ds->dc.w < textw(ds, c->name)) {
		ds->dc.w -= ds->dc.h;
		ds->button[Close].x = ds->dc.w;
		drawtext(ds, c->name, c->drawable, c->xftdraw,
		    c == sel ? ds->style.color.sel : ds->style.color.norm, ds->dc.x, ds->dc.y, ds->dc.w);
		drawbutton(ds, c->drawable, ds->button[Close],
		    c == sel ? ds->style.color.sel : ds->style.color.norm, ds->dc.w,
		    ds->dc.h / 2 - ds->button[Close].ph / 2);
		goto end;
	}
	/* Left */
	for (i = 0; i < strlen(ds->style.titlelayout); i++) {
		if (ds->style.titlelayout[i] == ' ' || ds->style.titlelayout[i] == '-')
			break;
		ds->dc.x += drawelement(ds, ds->style.titlelayout[i], ds->dc.x, AlignLeft, c);
	}
	if (i == strlen(ds->style.titlelayout) || ds->dc.x >= ds->dc.w)
		goto end;
	/* Center */
	ds->dc.x = ds->dc.w / 2;
	for (i++; i < strlen(ds->style.titlelayout); i++) {
		if (ds->style.titlelayout[i] == ' ' || ds->style.titlelayout[i] == '-')
			break;
		ds->dc.x -= elementw(ds, ds->style.titlelayout[i], c) / 2;
		ds->dc.x += drawelement(ds, ds->style.titlelayout[i], 0, AlignCenter, c);
	}
	if (i == strlen(ds->style.titlelayout) || ds->dc.x >= ds->dc.w)
		goto end;
	/* Right */
	ds->dc.x = ds->dc.w;
	for (i = strlen(ds->style.titlelayout); i-- ; ) {
		if (ds->style.titlelayout[i] == ' ' || ds->style.titlelayout[i] == '-')
			break;
		ds->dc.x -= elementw(ds, ds->style.titlelayout[i], c);
		drawelement(ds, ds->style.titlelayout[i], 0, AlignRight, c);
	}
      end:
	if (ds->style.outline) {
		XSetForeground(dpy, ds->dc.gc,
		    c == sel ? ds->style.color.sel[ColBorder] : ds->style.color.norm[ColBorder]);
		XDrawLine(dpy, c->drawable, ds->dc.gc, 0, ds->dc.h - 1, ds->dc.w, ds->dc.h - 1);
	}
	XCopyArea(dpy, c->drawable, c->title, ds->dc.gc, 0, 0, c->c.w, ds->dc.h, 0, 0);
}

static unsigned long
getcolor(const char *colstr) {
	XColor color, exact;

	if (!XAllocNamedColor(dpy, DefaultColormap(dpy, scr->screen), colstr, &color, &exact))
		eprint("error, cannot allocate color '%s'\n", colstr);
	return color.pixel;
}

static int
initpixmap(const char *file, Button * b)
{
#ifdef XPM
	if (strstr(file, ".xpm") && strlen(strstr(file, ".xpm")) == 4) {
		XpmAttributes xa = { 0, };

		if (XpmReadFileToPixmap(dpy, scr->root, file, &b->pixmap, &b->mask, &xa) == Success) {
			if ((b->pw = xa.width) && (b->ph = xa.height)) {
				if (b->ph > scr->style.titleheight) {
					b->po = b->ph / 2 - scr->style.titleheight / 2;
					b->ph = scr->style.titleheight;
				}
				return 0;
			}
		}
	}
#endif
#ifdef IMILIB2
	if (!strstr(file, ".xbm") || strlen(strstr(file, ".xbm")) != 4) {
		imlib_context_push(scr->context);
		imlib_context_set_mask(None);

		if (!(b->image = imlib_load_image(file))) {
			if (file[0] != '/') {
				/* TODO: look for it else where */
			}
		}
		if (b->image) {
			imlib_context_set_mask(None);
			imlib_context_set_image(b->image);
			b->pw = imlib_image_get_width();
			b->ph = imlib_image_get_height();
			if (b->ph > scr->style.titleheight)
				b->ph = scr->style.titleheight;
			if (b->pw > 2*scr->style.titleheight)
				b->pw = 2*scr->style.titleheight;
			imlib_render_pixmaps_for_whole_image_at_size
			    (&b->pixmap, &b->mask, b->pw, b->ph);
		} else {
			DPRINTF("could not load image file %s\n", file);
		}
		imlib_context_pop();
		if (b->image)
			return 0;
	}
#endif
	if (strstr(file, ".xbm") && strlen(strstr(file, ".xbm")) == 4) {
		b->bitmap =
		    XCreatePixmap(dpy, scr->root, scr->style.titleheight,
				  scr->style.titleheight, 1);
		if (BitmapSuccess ==
		    XReadBitmapFile(dpy, scr->root, file, &b->pw, &b->ph, &b->bitmap,
				    &b->px, &b->py)) {
			if (b->px == -1 || b->py == -1)
				b->px = b->py = 0;
			return 0;
		}
	}
	return 1;
}

static void
_iconify(const char *arg) {
	if (sel) iconify(sel);
}
static void
_togglemax(const char *arg) {
	if (sel) togglemax(sel);
}
static void
_killclient(const char *arg) {
	if (sel) killclient(sel);
}

static void
initbuttons() {
	scr->button[Iconify].action = _iconify;
	scr->button[Maximize].action = _togglemax;
	scr->button[Close].action = _killclient;
	scr->button[Iconify].x = scr->button[Close].x = scr->button[Maximize].x = -1;
	XSetForeground(dpy, scr->dc.gc, scr->style.color.norm[ColButton]);
	XSetBackground(dpy, scr->dc.gc, scr->style.color.norm[ColBG]);
	if (initpixmap(getresource("button.iconify.pixmap", ICONPIXMAP),
	    &scr->button[Iconify]))
		scr->button[Iconify].action = NULL;
	if (initpixmap(getresource("button.maximize.pixmap", MAXPIXMAP),
	    &scr->button[Maximize]))
		scr->button[Maximize].action = NULL;
	if (initpixmap(getresource("button.close.pixmap", CLOSEPIXMAP),
	    &scr->button[Close]))
		scr->button[Close].action = NULL;
}

static void
initfont(const char *fontstr) {
	scr->style.font = NULL;
	scr->style.font = XftFontOpenXlfd(dpy, scr->screen, fontstr);
	if (!scr->style.font)
		scr->style.font = XftFontOpenName(dpy, scr->screen, fontstr);
	if (!scr->style.font)
		eprint("error, cannot load font: '%s'\n", fontstr);
	scr->dc.font.extents = emallocz(sizeof(XGlyphInfo));
	XftTextExtentsUtf8(dpy, scr->style.font,
	    (const unsigned char *) fontstr, strlen(fontstr), scr->dc.font.extents);
	scr->dc.font.height = scr->style.font->ascent + scr->style.font->descent + 1;
	scr->dc.font.ascent = scr->style.font->ascent;
	scr->dc.font.descent = scr->style.font->descent;
}

void
initstyle() {
	scr->style.color.norm[ColBorder] = getcolor(getresource("normal.border", NORMBORDERCOLOR));
	scr->style.color.norm[ColBG] = getcolor(getresource("normal.bg", NORMBGCOLOR));
	scr->style.color.norm[ColFG] = getcolor(getresource("normal.fg", NORMFGCOLOR));
	scr->style.color.norm[ColButton] = getcolor(getresource("normal.button", NORMBUTTONCOLOR));

	scr->style.color.sel[ColBorder] = getcolor(getresource("selected.border", SELBORDERCOLOR));
	scr->style.color.sel[ColBG] = getcolor(getresource("selected.bg", SELBGCOLOR));
	scr->style.color.sel[ColFG] = getcolor(getresource("selected.fg", SELFGCOLOR));
	scr->style.color.sel[ColButton] = getcolor(getresource("selected.button", SELBUTTONCOLOR));

	scr->style.color.font[Selected] = emallocz(sizeof(XftColor));
	scr->style.color.font[Normal] = emallocz(sizeof(XftColor));
	XftColorAllocName(dpy, DefaultVisual(dpy, scr->screen), DefaultColormap(dpy,
				scr->screen), getresource("selected.fg", SELFGCOLOR), scr->style.color.font[Selected]);
	XftColorAllocName(dpy, DefaultVisual(dpy, scr->screen), DefaultColormap(dpy,
				scr->screen), getresource("normal.fg", NORMFGCOLOR), scr->style.color.font[Normal]);
	if (!scr->style.color.font[Normal] || !scr->style.color.font[Normal])
		eprint("error, cannot allocate colors\n");
	initfont(getresource("font", FONT));
	scr->style.border = atoi(getresource("border", STR(BORDERPX)));
	scr->style.margin = atoi(getresource("margin", STR(MARGINPX)));
	scr->style.opacity = OPAQUE * atof(getresource("opacity", STR(NF_OPACITY)));
	scr->style.outline = atoi(getresource("outline", "0"));
	strncpy(scr->style.titlelayout, getresource("titlelayout", "N  IMC"),
	    LENGTH(scr->style.titlelayout));
	scr->style.titlelayout[LENGTH(scr->style.titlelayout) - 1] = '\0';
	scr->style.titleheight = atoi(getresource("title", STR(TITLEHEIGHT)));
	if (!scr->style.titleheight)
		scr->style.titleheight = scr->dc.font.height + 2;
	scr->dc.gc = XCreateGC(dpy, scr->root, 0, 0);
	initbuttons();
}

void
deinitstyle() {	
	/* XXX: more to do */
	XftColorFree(dpy, DefaultVisual(dpy, scr->screen), DefaultColormap(dpy,
		scr->screen), scr->style.color.font[Normal]);
	XftColorFree(dpy, DefaultVisual(dpy, scr->screen), DefaultColormap(dpy,
		scr->screen), scr->style.color.font[Selected]);
	XftFontClose(dpy, scr->style.font);
	free(scr->dc.font.extents);
	XFreeGC(dpy, scr->dc.gc);
}

static unsigned int
textnw(EScreen *ds, const char *text, unsigned int len) {
	XftTextExtentsUtf8(dpy, ds->style.font,
	    (const unsigned char *) text, len, ds->dc.font.extents);
	return ds->dc.font.extents->xOff;
}

static unsigned int
textw(EScreen *ds, const char *text) {
	return textnw(ds, text, strlen(text)) + ds->dc.font.height;
}
