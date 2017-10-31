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

enum { Normal, Selected };
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
	ElementClient *ec;
	Element *e;
	int image;
	const char *name = NULL;

	(void) name;
	switch (type) {
	case MenuBtn:
		name = "menu";
		present = True;
		toggled = False;
		enabled = True;
	case IconifyBtn:
		name = "iconify";
		present = c->has.but.min;
		toggled = False;
		enabled = c->user.min;
		break;
	case MaximizeBtn:
		name = "maximize";
		present = c->has.but.max;
		toggled = c->is.max || c->is.maxv || c->is.maxh || c->is.full;
		enabled = c->user.max || c->user.maxv || c->user.maxh || c->user.full;
		break;
	case CloseBtn:
		name = "close";
		present = c->has.but.close;
		toggled = c->is.closing || c->is.killing;
		enabled = c->user.close;
		break;
	case ShadeBtn:
		name = "shade";
		present = c->has.but.shade;
		toggled = c->is.shaded;
		enabled = c->user.shade;
		break;
	case StickBtn:
		name = "stick";
		present = c->has.but.stick;
		toggled = c->is.sticky;
		enabled = c->user.stick;
		break;
	case LHalfBtn:
		name = "lhalf";
		present = c->has.but.half;
		toggled = c->is.lhalf;
		enabled = c->user.size && c->user.move;
		break;
	case RHalfBtn:
		name = "rhalf";
		present = c->has.but.half;
		toggled = c->is.rhalf;
		enabled = c->user.size && c->user.move;
		break;
	case FillBtn:
		name = "fill";
		present = c->has.but.fill;
		toggled = c->is.fill;
		enabled = c->user.fill || c->user.fillh || c->user.fillv;
		break;
	case FloatBtn:
		name = "float";
		present = c->has.but.floats;
		toggled = c->is.floater || c->skip.arrange;
		enabled = c->user.floats || c->user.arrange;
		break;
	case SizeBtn:
		name = "resize";
		present = c->has.but.size;
		toggled = c->is.moveresize;
		enabled = c->user.size || c->user.sizev || c->user.sizeh;
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

	if (pressed == 0x1 && toggled && e->image[ButtonImageToggledPressedB1].present) {
		XPRINTF("button %s assigned toggled.pressed.b1 image\n", name);
		return &e->image[ButtonImageToggledPressedB1];
	}
	if (pressed == 0x2 && toggled && e->image[ButtonImageToggledPressedB2].present) {
		XPRINTF("button %s assigned toggled.pressed.b2 image\n", name);
		return &e->image[ButtonImageToggledPressedB2];
	}
	if (pressed == 0x4 && toggled && e->image[ButtonImageToggledPressedB3].present) {
		XPRINTF("button %s assigned toggled.pressed.b3 image\n", name);
		return &e->image[ButtonImageToggledPressedB3];
	}
	if (pressed && toggled && e->image[ButtonImageToggledPressed].present) {
		XPRINTF("button %s assigned toggled.pressed image\n", name);
		return &e->image[ButtonImageToggledPressed];
	}
	if (pressed == 0x1 && e->image[ButtonImagePressedB1].present) {
		XPRINTF("button %s assigned pressed.b1 image\n", name);
		return &e->image[ButtonImagePressedB1];
	}
	if (pressed == 0x2 && e->image[ButtonImagePressedB2].present) {
		XPRINTF("button %s assigned pressed.b2 image\n", name);
		return &e->image[ButtonImagePressedB2];
	}
	if (pressed == 0x4 && e->image[ButtonImagePressedB3].present) {
		XPRINTF("button %s assigned pressed.b3 image\n", name);
		return &e->image[ButtonImagePressedB3];
	}
	if (pressed && e->image[ButtonImagePressed].present) {
		XPRINTF("button %s assigned pressed image\n", name);
		return &e->image[ButtonImagePressed];
	}
	image = ButtonImageHover + (hovered ? 0 : (focused ? 1 : 2));
	image += toggled ? 3 : 0;
	image += enabled ? 0 : 6;
	if (hovered && !e->image[image].present) {
		XPRINTF("button %s has no hovered image %d, using %s instead\n", name,
			image, focused ? "focused" : "unfocus");
		image += focused ? 1 : 2;
	}
	if (!focused && !e->image[image].present) {
		XPRINTF("button %s has no unfocus image %d, using focused instead\n",
			name, image);
		image -= 1;
	}
	if (toggled && !e->image[image].present) {
		XPRINTF("button %s has no toggled image %d, using untoggled instead\n",
			name, image);
		image -= 3;
	}
	if (!enabled && !e->image[image].present) {
		XPRINTF("button %s missing disabled image %d, skipping button\n", name,
			image);
		return NULL;
	}
	if (e->image[image].present) {
		XPRINTF("button %s going with chosen image %d\n", name, image);
		return &e->image[image];
	}
	if (e->image[ButtonImageDefault].present) {
		XPRINTF("button %s missing chosen image %d, going with default\n", name,
			image);
		return &e->image[ButtonImageDefault];
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
	int hilite = (c == sel) ? Selected : Normal;

	if (0 > type || type >= LastElement)
		return 0;

	switch (type) {
		unsigned int j;

	case TitleName:
		w = textw(ds, c->name, hilite);
		break;
	case TitleTags:
		for (j = 0; j < ds->ntags; j++) {
			if (c->tags & (1ULL << j))
				w += textw(ds, ds->tags[j].name, hilite);
		}
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
drawtext(AScreen *ds, const char *text, Drawable drawable, XftDraw * xftdrawable,
	 unsigned long col[ColLast], int hilite, int x, int y, int mw)
{
	int w, h;
	char buf[256];
	unsigned int len, olen;
	int drop, status;

	if (!text)
		return 0;
	olen = len = strlen(text);
	w = 0;
	if (len >= sizeof buf)
		len = sizeof buf - 1;
	memcpy(buf, text, len);
	buf[len] = 0;
	h = ds->style.titleheight;
	y = ds->dc.h / 2 + ds->dc.font[hilite].ascent / 2 - 1 - ds->style.outline;
	x += ds->dc.font[hilite].height / 2;
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
	XSetForeground(dpy, ds->dc.gc, col[ColBG]);
	XSetFillStyle(dpy, ds->dc.gc, FillSolid);
	status =
	    XFillRectangle(dpy, drawable, ds->dc.gc, x - ds->dc.font[hilite].height / 2,
			   0, w + ds->dc.font[hilite].height, h);
	if (!status)
		DPRINTF("Could not fill rectangle, error %d\n", status);
	if ((drop = ds->style.drop[hilite]))
		XftDrawStringUtf8(xftdrawable, ds->style.color.shadow[hilite],
				  ds->style.font[hilite], x + drop, y + drop,
				  (unsigned char *) buf, len);
	XftDrawStringUtf8(xftdrawable, ds->style.color.font[hilite],
			  ds->style.font[hilite], x, y, (unsigned char *) buf, len);
	return w + ds->dc.font[hilite].height;
}

static int
drawbutton(AScreen *ds, Client *c, ElementType type, unsigned long col[ColLast], int x)
{
	ElementClient *ec = &c->element[type];
	Drawable d = ds->dc.draw.pixmap;
	ButtonImage *bi;
	int status;

	if (!(bi = buttonimage(ds, c, type)) || !bi->present) {
		XPRINTF("button %d has no button image\n", type);
		return 0;
	}

	ec->g.x = x;
	ec->g.y = (ds->dc.h - bi->h) / 2;
	ec->g.w = bi->w;
	ec->g.h = bi->h;

#if defined IMLIB2 || defined XPM
	if (bi->pixmap) {
		XPRINTF("Copying pixmap 0x%lx with geom %dx%d+%d+%d to drawable 0x%lx\n",
			bi->pixmap, ec->g.w, ec->g.h, ec->g.x, ec->g.y, d);
		XCopyArea(dpy, bi->pixmap, d, ds->dc.gc, 0, bi->y, ec->g.w, ec->g.h,
			  ec->g.x, ec->g.y);
		return ec->g.w;
	} else
#endif
	if (bi->bitmap) {
		XSetForeground(dpy, ds->dc.gc, col[ColBG]);
		XSetFillStyle(dpy, ds->dc.gc, FillSolid);
		status =
		    XFillRectangle(dpy, d, ds->dc.gc, ec->g.x, 0, ds->dc.h, ds->dc.h);
		if (!status)
			DPRINTF("Could not fill rectangle, error %d\n", status);
		XSetForeground(dpy, ds->dc.gc, ec->pressed ? col[ColFG] : col[ColButton]);
		XSetBackground(dpy, ds->dc.gc, col[ColBG]);
		XCopyPlane(dpy, bi->bitmap, d, ds->dc.gc, 0, 0, ec->g.w, ec->g.h, ec->g.x,
			   ec->g.y + bi->y, 1);
		return ds->dc.h;
	}
	XPRINTF("button %d has no pixmap or bitmap\n", type);
	return 0;
}

static int
drawelement(AScreen *ds, char which, int x, int position, Client *c)
{
	int w = 0;
	unsigned long *color = c == sel ? ds->style.color.sel : ds->style.color.norm;
	int hilite = (c == sel) ? Selected : Normal;
	ElementType type = elementtype(which);
	ElementClient *ec = &c->element[type];

	if (0 > type || type >= LastElement)
		return 0;

	XPRINTF("drawing element '%c' for client %s\n", which, c->name);

	ec->present = False;
	ec->g.x = ds->dc.x;
	ec->g.y = 0;
	ec->g.w = 0;
	ec->g.h = ds->style.titleheight;
	ec->g.b = 0;		/* later */

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
		break;
	case TitleSep:
		XSetForeground(dpy, ds->dc.gc, color[ColBorder]);
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x + ds->dc.h / 4, 0,
			  ds->dc.x + ds->dc.h / 4, ds->dc.h);
		w = ds->dc.h / 2;
		break;
	case TitleName:
		w = drawtext(ds, c->name, ds->dc.draw.pixmap, ds->dc.draw.xft, color,
			     hilite, ds->dc.x, ds->dc.y, ds->dc.w);
		break;
	default:
		if (0 <= type && type < LastBtn)
			w = drawbutton(ds, c, type, color, ds->dc.x);
		break;
	}
	if (w) {
		ec->present = True;
		ec->g.w = w;
		XPRINTF("element '%c' at %dx%d+%d+%d:%d for client '%s'\n",
			which, ec->g.w, ec->g.h, ec->g.x, ec->g.y, ec->g.b, c->name);
	} else
		XPRINTF("missing element '%c' for client %s\n", which, c->name);
	return w;
}

static void
drawdockapp(Client *c, AScreen *ds)
{
	int status;
	unsigned long pixel;

	pixel = (c == sel) ? ds->style.color.sel[ColBG] : ds->style.color.norm[ColBG];
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
		DPRINTF("Could not fill rectangle, error %d\n", status);
	CPRINTF(c, "Filled dockapp frame %dx%d+%d+%d\n", ds->dc.w, ds->dc.h, ds->dc.x,
		ds->dc.y);
	/* note that ParentRelative dockapps need the background set to the foregroudn */
	XSetWindowBackground(dpy, c->frame, pixel);
	/* following are quite disruptive - many dockapps ignore expose events and simply
	   update on timer */
	// XClearWindow(dpy, c->icon ? : c->win);
	// XClearArea(dpy, c->icon ? : c->win, 0, 0, 0, 0, True);
#if 0
	/* doesn't work */
	{
		XEvent ev;

		ev.xexpose.type = Expose;
		ev.xexpose.serial = 0;
		ev.xexpose.send_event = False;
		ev.xexpose.display = dpy;
		ev.xexpose.window = c->icon ? : c->win;
		ev.xexpose.x = 0;
		ev.xexpose.y = 0;
		ev.xexpose.width = c->r.w;
		ev.xexpose.height = c->r.h;
		ev.xexpose.count = 0;

		XSendEvent(dpy, c->icon ? : c->win, False, NoEventMask, &ev);

		ev.xconfigure.type = ConfigureNotify;
		ev.xconfigure.serial = 0;
		ev.xconfigure.send_event = True;
		ev.xconfigure.display = dpy;
		ev.xconfigure.event = c->icon ? : c->win;
		ev.xconfigure.window = c->icon ? : c->win;
		ev.xconfigure.x = c->c.x + c->c.b + c->r.x;
		ev.xconfigure.y = c->c.y + c->c.b + c->r.y;
		ev.xconfigure.width = c->r.w;
		ev.xconfigure.height = c->r.h;
		ev.xconfigure.border_width = c->r.b;
		ev.xconfigure.above = None;
		ev.xconfigure.override_redirect = False;

		XSendEvent(dpy, c->icon ? : c->win, False, NoEventMask, &ev);
	}
#endif
}

void
drawclient(Client *c)
{
	size_t i;
	AScreen *ds;
	int status;

	/* might be drawing a client that is not on the current screen */
	if (!(ds = getscreen(c->win, True))) {
		DPRINTF("What? no screen for window 0x%lx???\n", c->win);
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
		ds->dc.draw.pixmap = XCreatePixmap(dpy, ds->root, ds->dc.w,
						   ds->dc.draw.h, ds->depth);
		XftDrawChange(ds->dc.draw.xft, ds->dc.draw.pixmap);
	}
	XSetForeground(dpy, ds->dc.gc,
		       c ==
		       sel ? ds->style.color.sel[ColBG] : ds->style.color.norm[ColBG]);
	XSetLineAttributes(dpy, ds->dc.gc, ds->style.border, LineSolid, CapNotLast,
			   JoinMiter);
	XSetFillStyle(dpy, ds->dc.gc, FillSolid);
	status =
	    XFillRectangle(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x, ds->dc.y,
			   ds->dc.w, ds->dc.h);
	if (!status)
		DPRINTF("Could not fill rectangle, error %d\n", status);
	/* Don't know about this... */
	if (ds->dc.w < textw(ds, c->name, (c == sel) ? Selected : Normal)) {
		ds->dc.w -= elementw(ds, c, CloseBtn);
		drawtext(ds, c->name, ds->dc.draw.pixmap, ds->dc.draw.xft,
			 c == sel ? ds->style.color.sel : ds->style.color.norm,
			 c == sel ? 1 : 0, ds->dc.x, ds->dc.y, ds->dc.w);
		drawbutton(ds, c, CloseBtn,
			   c == sel ? ds->style.color.sel : ds->style.color.norm,
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
			       c ==
			       sel ? ds->style.color.sel[ColBorder] : ds->style.color.
			       norm[ColBorder]);
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, 0, ds->dc.h - 1, ds->dc.w,
			  ds->dc.h - 1);
	}
	if (c->title)
		XCopyArea(dpy, ds->dc.draw.pixmap, c->title, ds->dc.gc, 0, 0, c->c.w,
			  ds->dc.h, 0, 0);
	ds->dc.x = ds->dc.y = 0;
	ds->dc.w = c->c.w;
	ds->dc.h = ds->style.gripsheight;
	XSetForeground(dpy, ds->dc.gc,
		       c ==
		       sel ? ds->style.color.sel[ColBG] : ds->style.color.norm[ColBG]);
	XSetLineAttributes(dpy, ds->dc.gc, ds->style.border, LineSolid, CapNotLast,
			   JoinMiter);
	XSetFillStyle(dpy, ds->dc.gc, FillSolid);
	status =
	    XFillRectangle(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x, ds->dc.y,
			   ds->dc.w, ds->dc.h);
	if (!status)
		DPRINTF("Could not fill rectangle, error %d\n", status);
	if (ds->style.outline) {
		XSetForeground(dpy, ds->dc.gc,
			       c ==
			       sel ? ds->style.color.sel[ColBorder] : ds->style.color.
			       norm[ColBorder]);
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

static unsigned long
getcolor(const char *colstr)
{
	XColor color, exact;

	if (!XAllocNamedColor(dpy, scr->colormap, colstr,
			      &color, &exact))
		eprint("error, cannot allocate color '%s'\n", colstr);
	return color.pixel;
}

static void
freepixmap(ButtonImage *bi)
{
	if (bi->pixmap) {
		XFreePixmap(dpy, bi->pixmap);
		bi->pixmap = None;
	}
}

static int
initpixmap(const char *file, ButtonImage *bi)
{
	char *path;

	if (!file || !(path = findrcpath(file)))
		return 1;
#ifdef XPM
	if (strstr(path, ".xpm") && strlen(strstr(path, ".xpm")) == 4) {
		XpmAttributes xa = { 0, };

		xa.visual = scr->visual;
		xa.valuemask |= XpmVisual;
		xa.colormap = scr->colormap;
		xa.valuemask |= XpmColormap;
		xa.depth = scr->depth;
		xa.valuemask |= XpmDepth;

		if (XpmReadFileToPixmap(dpy, scr->drawable, path, &bi->pixmap, &bi->mask, &xa)
		    == Success) {
			if ((bi->w = xa.width) && (bi->h = xa.height)) {
				if (bi->h > scr->style.titleheight) {
					bi->y += (bi->h - scr->style.titleheight) / 2;
					bi->h = scr->style.titleheight;
				}
				free(path);
				return 0;
			}
		}
	}
#endif
#ifdef IMLIB2
	if (!strstr(path, ".xbm") || strlen(strstr(path, ".xbm")) != 4) {
		Imlib_Image image;

		imlib_context_push(scr->context);
		imlib_context_set_mask(None);

		if ((image = imlib_load_image(path))) {
			imlib_context_set_image(image);
			imlib_context_set_mask(None);
			bi->w = imlib_image_get_width();
			bi->h = imlib_image_get_height();
			if (bi->h > scr->style.titleheight)
				bi->h = scr->style.titleheight;
			if (bi->w > 2 * scr->style.titleheight)
				bi->w = 2 * scr->style.titleheight;
			imlib_render_pixmaps_for_whole_image_at_size
			    (&bi->pixmap, &bi->mask, bi->w, bi->h);
		} else {
			DPRINTF("could not load image file %s\n", path);
		}
		imlib_context_pop();
		if (image) {
			free(path);
			return 0;
		}
	}
#endif
	if (strstr(path, ".xbm") && strlen(strstr(path, ".xbm")) == 4) {
		bi->bitmap =
		    XCreatePixmap(dpy, scr->root, scr->style.titleheight,
				  scr->style.titleheight, 1);
		if (BitmapSuccess ==
		    XReadBitmapFile(dpy, scr->root, path, &bi->w, &bi->h, &bi->bitmap,
				    &bi->x, &bi->y)) {
			if (bi->x == -1 || bi->y == -1)
				bi->x = bi->y = 0;
			free(path);
			return 0;
		}
	}
	free(path);
	return 1;
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
	if (!c->user.min)
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
		if (c->user.max || (c->user.size && c->user.move))
			togglemax(c);
		break;
	case Button2:
		if (c->user.maxv || ((c->user.size || c->user.sizev) && c->user.move))
			togglemaxv(c);
		break;
	case Button3:
		if (c->user.maxh || ((c->user.size || c->user.sizeh) && c->user.move))
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
	if (!c->user.close)
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
	if (!c->user.shade)
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
	if (!c->user.stick)
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
	if (!c->user.move || !c->user.size)
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
	if (!c->user.move || !c->user.size)
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
	if (!c->user.fill)
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
	if (!c->user.size)
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
	char res[128];

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
			snprintf(res, sizeof(res), "%s%s.pixmap", name, kind[i]);
			if ((e->image[i].present =
			     !initpixmap(getresource(res, def), &e->image[i])))
				e->action = action;
			else
				DPRINTF("could not load pixmap for %s\n", res);
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

	XSetForeground(dpy, scr->dc.gc, scr->style.color.norm[ColButton]);
	XSetBackground(dpy, scr->dc.gc, scr->style.color.norm[ColBG]);

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

void
freestyle()
{
	freebuttons();
	if (scr->style.color.font[Normal]) {
		XftColorFree(dpy, scr->visual,
			     scr->colormap,
			     scr->style.color.font[Normal]);
		free(scr->style.color.font[Normal]);
		scr->style.color.font[Normal] = NULL;
	}
	if (scr->style.color.font[Selected]) {
		XftColorFree(dpy, scr->visual,
			     scr->colormap,
			     scr->style.color.font[Selected]);
		free(scr->style.color.font[Selected]);
		scr->style.color.font[Selected] = NULL;
	}
	if (scr->style.color.shadow[Normal]) {
		XftColorFree(dpy, scr->visual,
			     scr->colormap,
			     scr->style.color.shadow[Normal]);
		free(scr->style.color.shadow[Normal]);
		scr->style.color.shadow[Normal] = NULL;
	}
	if (scr->style.color.shadow[Selected]) {
		XftColorFree(dpy, scr->visual,
			     scr->colormap,
			     scr->style.color.shadow[Selected]);
		free(scr->style.color.shadow[Selected]);
		scr->style.color.shadow[Selected] = NULL;
	}
	freefont(Normal);
	freefont(Selected);
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
	scr->style.color.norm[ColBorder] =
	    getcolor(getresource("normal.border", NORMBORDERCOLOR));
	scr->style.color.norm[ColBG] = getcolor(getresource("normal.bg", NORMBGCOLOR));
	scr->style.color.norm[ColFG] = getcolor(getresource("normal.fg", NORMFGCOLOR));
	scr->style.color.norm[ColButton] =
	    getcolor(getresource("normal.button", NORMBUTTONCOLOR));

	scr->style.color.sel[ColBorder] =
	    getcolor(getresource("selected.border", SELBORDERCOLOR));
	scr->style.color.sel[ColBG] = getcolor(getresource("selected.bg", SELBGCOLOR));
	scr->style.color.sel[ColFG] = getcolor(getresource("selected.fg", SELFGCOLOR));
	scr->style.color.sel[ColButton] =
	    getcolor(getresource("selected.button", SELBUTTONCOLOR));

	scr->style.color.font[Selected] = emallocz(sizeof(XftColor));
	scr->style.color.font[Normal] = emallocz(sizeof(XftColor));
	XftColorAllocName(dpy, scr->visual,
			  scr->colormap,
			  getresource("selected.fg", SELFGCOLOR),
			  scr->style.color.font[Selected]);
	XftColorAllocName(dpy, scr->visual,
			  scr->colormap,
			  getresource("normal.fg", NORMFGCOLOR),
			  scr->style.color.font[Normal]);
	if (!scr->style.color.font[Selected] || !scr->style.color.font[Normal])
		eprint("error, cannot allocate colors\n");

	if ((scr->style.drop[Selected] = atoi(getresource("selected.drop", "0")))) {
		scr->style.color.shadow[Selected] = emallocz(sizeof(XftColor));
		XftColorAllocName(dpy, scr->visual,
				  scr->colormap,
				  getresource("selected.shadow", SELBORDERCOLOR),
				  scr->style.color.shadow[Selected]);
		if (!scr->style.color.shadow[Selected])
			eprint("error, cannot allocate colors\n");
	}
	if ((scr->style.drop[Normal] = atoi(getresource("normal.drop", "0")))) {
		scr->style.color.shadow[Normal] = emallocz(sizeof(XftColor));
		XftColorAllocName(dpy, scr->visual,
				  scr->colormap,
				  getresource("normal.shadow", NORMBORDERCOLOR),
				  scr->style.color.shadow[Normal]);
		if (!scr->style.color.shadow[Normal])
			eprint("error, cannot allocate colors\n");
	}
	initfont(getresource("normal.font", getresource("font", FONT)), Normal);
	initfont(getresource("selected.font", getresource("font", FONT)), Selected);
	scr->style.border = atoi(getresource("border", STR(BORDERPX)));
	scr->style.margin = atoi(getresource("margin", STR(MARGINPX)));
	scr->style.opacity = (int) ((double) OPAQUE * atof(getresource("opacity", STR(NF_OPACITY))));
	scr->style.outline = atoi(getresource("outline", "0")) ? 1 : 0;
	scr->style.spacing = atoi(getresource("spacing", "1"));
	strncpy(scr->style.titlelayout, getresource("titlelayout", "N  IMC"),
		LENGTH(scr->style.titlelayout));
	scr->style.gripsheight = atoi(getresource("grips", STR(GRIPHEIGHT)));
	scr->style.titlelayout[LENGTH(scr->style.titlelayout) - 1] = '\0';
	scr->style.titleheight = atoi(getresource("title", STR(TITLEHEIGHT)));
	if (!scr->style.titleheight)
		scr->style.titleheight =
		    max(scr->dc.font[Selected].height + scr->style.drop[Selected],
			scr->dc.font[Normal].height + scr->style.drop[Normal]) + 2;
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
	for (c = scr->clients; c ; c = c->next)
		drawclient(c);
}
