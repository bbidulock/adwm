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

enum { Normal, Selected };
enum { AlignLeft, AlignCenter, AlignRight };	/* title position */

static unsigned int textnw(const char *text, unsigned int len);
static unsigned int textw(const char *text);

static int
drawtext(const char *text, Drawable drawable, XftDraw *xftdrawable,
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
	h = style.titleheight;
	y = scr->dc.h / 2 + scr->dc.font.ascent / 2 - 1 - style.outline;
	x += scr->dc.font.height / 2;
	/* shorten text if necessary */
	while (len && (w = textnw(buf, len)) > mw) {
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
		x = scr->dc.x++;
	XSetForeground(dpy, scr->dc.gc, col[ColBG]);
	XFillRectangle(dpy, drawable, scr->dc.gc, x - scr->dc.font.height / 2, 0,
	    w + scr->dc.font.height, h);
	XftDrawStringUtf8(xftdrawable,
	    (col == scr->color.norm) ? scr->color.font[Normal] : scr->color.font[Selected],
	    style.font, x, y, (unsigned char *) buf, len);
	return w + scr->dc.font.height;
}

static int
drawbutton(Drawable d, Button btn, unsigned long col[ColLast], int x, int y) {
	if (btn.action == NULL)
		return 0;
	XSetForeground(dpy, scr->dc.gc, col[ColBG]);
	XFillRectangle(dpy, d, scr->dc.gc, x, 0, scr->dc.h, scr->dc.h);
	XSetForeground(dpy, scr->dc.gc, btn.pressed ? col[ColFG] : col[ColButton]);
	XSetBackground(dpy, scr->dc.gc, col[ColBG]);
	XCopyPlane(dpy, btn.pm, d, scr->dc.gc, 0, 0, scr->button[Iconify].pw,
	    scr->button[Iconify].ph, x, y + scr->button[Iconify].py, 1);
	return scr->dc.h;
}

static int
drawelement(char which, int x, int position, Client *c) {
	int w;
	unsigned int j;
	unsigned long *color = c == sel ? scr->color.sel : scr->color.norm;

	switch (which) {
	case 'T':
		w = 0;
		for (j = 0; j < scr->ntags; j++) {
			if (c->tags[j])
				w += drawtext(scr->tags[j], c->drawable, c->xftdraw,
				    color, scr->dc.x, scr->dc.y, scr->dc.w);
		}
		break;
	case '|':
		XSetForeground(dpy, scr->dc.gc, color[ColBorder]);
		XDrawLine(dpy, c->drawable, scr->dc.gc, scr->dc.x + scr->dc.h / 4, 0,
		    scr->dc.x + scr->dc.h / 4, scr->dc.h);
		w = scr->dc.h / 2;
		break;
	case 'N':
		w = drawtext(c->name, c->drawable, c->xftdraw, color, scr->dc.x, scr->dc.y, scr->dc.w);
		break;
	case 'I':
		scr->button[Iconify].x = scr->dc.x;
		w = drawbutton(c->drawable, scr->button[Iconify], color,
		    scr->dc.x, scr->dc.h / 2 - scr->button[Iconify].ph / 2);
		break;
	case 'M':
		scr->button[Maximize].x = scr->dc.x;
		w = drawbutton(c->drawable, scr->button[Maximize], color,
		    scr->dc.x, scr->dc.h / 2 - scr->button[Maximize].ph / 2);
		break;
	case 'C':
		scr->button[Close].x = scr->dc.x;
		w = drawbutton(c->drawable, scr->button[Close], color, scr->dc.x,
		    scr->dc.h / 2 - scr->button[Maximize].ph / 2);
		break;
	default:
		w = 0;
		break;
	}
	return w;
}

static int
elementw(char which, Client *c) {
	int w;
	unsigned int j;

	switch (which) {
	case 'I':
	case 'M':
	case 'C':
		return scr->dc.h;
	case 'N':
		return textw(c->name);
	case 'T':
		w = 0;
		for (j = 0; j < scr->ntags; j++) {
			if (c->tags[j])
				w += textw(scr->tags[j]);
		}
		return w;
	case '|':
		return scr->dc.h / 2;
	}
	return 0;
}

void
drawclient(Client *c) {
	size_t i;

	if (style.opacity) {
		setopacity(c, c == sel ? OPAQUE : style.opacity);
	}
	if (!isvisible(c, NULL))
		return;
	if (!c->title)
		return;
	scr->dc.x = scr->dc.y = 0;
	scr->dc.w = c->w;
	scr->dc.h = style.titleheight;
	XftDrawChange(c->xftdraw, c->drawable);
	XSetForeground(dpy, scr->dc.gc, c == sel ? scr->color.sel[ColBG] : scr->color.norm[ColBG]);
	XSetLineAttributes(dpy, scr->dc.gc, style.border, LineSolid, CapNotLast, JoinMiter);
	XFillRectangle(dpy, c->drawable, scr->dc.gc, scr->dc.x, scr->dc.y, scr->dc.w, scr->dc.h);
	if (scr->dc.w < textw(c->name)) {
		scr->dc.w -= scr->dc.h;
		scr->button[Close].x = scr->dc.w;
		drawtext(c->name, c->drawable, c->xftdraw,
		    c == sel ? scr->color.sel : scr->color.norm, scr->dc.x, scr->dc.y, scr->dc.w);
		drawbutton(c->drawable, scr->button[Close],
		    c == sel ? scr->color.sel : scr->color.norm, scr->dc.w,
		    scr->dc.h / 2 - scr->button[Close].ph / 2);
		goto end;
	}
	/* Left */
	for (i = 0; i < strlen(style.titlelayout); i++) {
		if (style.titlelayout[i] == ' ' || style.titlelayout[i] == '-')
			break;
		scr->dc.x += drawelement(style.titlelayout[i], scr->dc.x, AlignLeft, c);
	}
	if (i == strlen(style.titlelayout) || scr->dc.x >= scr->dc.w)
		goto end;
	/* Center */
	scr->dc.x = scr->dc.w / 2;
	for (i++; i < strlen(style.titlelayout); i++) {
		if (style.titlelayout[i] == ' ' || style.titlelayout[i] == '-')
			break;
		scr->dc.x -= elementw(style.titlelayout[i], c) / 2;
		scr->dc.x += drawelement(style.titlelayout[i], 0, AlignCenter, c);
	}
	if (i == strlen(style.titlelayout) || scr->dc.x >= scr->dc.w)
		goto end;
	/* Right */
	scr->dc.x = scr->dc.w;
	for (i = strlen(style.titlelayout); i-- ; ) {
		if (style.titlelayout[i] == ' ' || style.titlelayout[i] == '-')
			break;
		scr->dc.x -= elementw(style.titlelayout[i], c);
		drawelement(style.titlelayout[i], 0, AlignRight, c);
	}
      end:
	if (style.outline) {
		XSetForeground(dpy, scr->dc.gc,
		    c == sel ? scr->color.sel[ColBorder] : scr->color.norm[ColBorder]);
		XDrawLine(dpy, c->drawable, scr->dc.gc, 0, scr->dc.h - 1, scr->dc.w, scr->dc.h - 1);
	}
	XCopyArea(dpy, c->drawable, c->title, scr->dc.gc, 0, 0, c->w, scr->dc.h, 0, 0);
}

static unsigned long
getcolor(const char *colstr) {
	XColor color, exact;

	if (!XAllocNamedColor(dpy, DefaultColormap(dpy, scr->screen), colstr, &color, &exact))
		eprint("error, cannot allocate color '%s'\n", colstr);
	return color.pixel;
}

static int
initpixmap(const char *file, Button *b) {
	b->pm = XCreatePixmap(dpy, scr->root, style.titleheight, style.titleheight, 1);
	if (BitmapSuccess == XReadBitmapFile(dpy, scr->root, file, &b->pw, &b->ph,
		&b->pm, &b->px, &b->py)) {
		if (b->px == -1 || b->py == -1)
			b->px = b->py = 0;
		return 0;
	} else
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
	XSetForeground(dpy, scr->dc.gc, scr->color.norm[ColButton]);
	XSetBackground(dpy, scr->dc.gc, scr->color.norm[ColBG]);
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
	style.font = NULL;
	style.font = XftFontOpenXlfd(dpy, scr->screen, fontstr);
	if (!style.font)
		style.font = XftFontOpenName(dpy, scr->screen, fontstr);
	if (!style.font)
		eprint("error, cannot load font: '%s'\n", fontstr);
	scr->dc.font.extents = emallocz(sizeof(XGlyphInfo));
	XftTextExtentsUtf8(dpy, style.font,
	    (const unsigned char *) fontstr, strlen(fontstr), scr->dc.font.extents);
	scr->dc.font.height = style.font->ascent + style.font->descent + 1;
	scr->dc.font.ascent = style.font->ascent;
	scr->dc.font.descent = style.font->descent;
}

void
initstyle() {
	scr->color.norm[ColBorder] = getcolor(getresource("normal.border", NORMBORDERCOLOR));
	scr->color.norm[ColBG] = getcolor(getresource("normal.bg", NORMBGCOLOR));
	scr->color.norm[ColFG] = getcolor(getresource("normal.fg", NORMFGCOLOR));
	scr->color.norm[ColButton] = getcolor(getresource("normal.button", NORMBUTTONCOLOR));

	scr->color.sel[ColBorder] = getcolor(getresource("selected.border", SELBORDERCOLOR));
	scr->color.sel[ColBG] = getcolor(getresource("selected.bg", SELBGCOLOR));
	scr->color.sel[ColFG] = getcolor(getresource("selected.fg", SELFGCOLOR));
	scr->color.sel[ColButton] = getcolor(getresource("selected.button", SELBUTTONCOLOR));

	scr->color.font[Selected] = emallocz(sizeof(XftColor));
	scr->color.font[Normal] = emallocz(sizeof(XftColor));
	XftColorAllocName(dpy, DefaultVisual(dpy, scr->screen), DefaultColormap(dpy,
				scr->screen), getresource("selected.fg", SELFGCOLOR), scr->color.font[Selected]);
	XftColorAllocName(dpy, DefaultVisual(dpy, scr->screen), DefaultColormap(dpy,
				scr->screen), getresource("normal.fg", NORMFGCOLOR), scr->color.font[Normal]);
	if (!scr->color.font[Normal] || !scr->color.font[Normal])
		eprint("error, cannot allocate colors\n");
	initfont(getresource("font", FONT));
	style.border = atoi(getresource("border", STR(BORDERPX)));
	style.margin = atoi(getresource("margin", STR(MARGINPX)));
	style.opacity = OPAQUE * atof(getresource("opacity", STR(NF_OPACITY)));
	style.outline = atoi(getresource("outline", "0"));
	strncpy(style.titlelayout, getresource("titlelayout", "N  IMC"),
	    LENGTH(style.titlelayout));
	style.titlelayout[LENGTH(style.titlelayout) - 1] = '\0';
	style.titleheight = atoi(getresource("title", STR(TITLEHEIGHT)));
	if (!style.titleheight)
		style.titleheight = scr->dc.font.height + 2;
	scr->dc.gc = XCreateGC(dpy, scr->root, 0, 0);
	initbuttons();
}

void
deinitstyle() {	
	/* XXX: more to do */
	XftColorFree(dpy, DefaultVisual(dpy, scr->screen), DefaultColormap(dpy,
		scr->screen), scr->color.font[Normal]);
	XftColorFree(dpy, DefaultVisual(dpy, scr->screen), DefaultColormap(dpy,
		scr->screen), scr->color.font[Selected]);
	XftFontClose(dpy, style.font);
	free(scr->dc.font.extents);
	XFreeGC(dpy, scr->dc.gc);
}

static unsigned int
textnw(const char *text, unsigned int len) {
	XftTextExtentsUtf8(dpy, style.font,
	    (const unsigned char *) text, len, scr->dc.font.extents);
	return scr->dc.font.extents->xOff;
}

static unsigned int
textw(const char *text) {
	return textnw(text, strlen(text)) + scr->dc.font.height;
}
