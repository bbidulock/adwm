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

static ButtonImage *
buttonimage(EScreen *ds, Client *c, ElementType type)
{
	Bool pressed, hovered, focused, enabled, present, toggled;
	ElementClient *ec;
	Element *e;
	int image;
	const char *name = NULL;

	(void) name;
	switch (type) {
	case IconifyBtn:
		name = "iconify";
		present = c->has.but.min;
		toggled = False;
		enabled = c->can.min;
		break;
	case MaximizeBtn:
		name = "maximize";
		present = c->has.but.max;
		toggled = c->is.max || c->is.maxv || c->is.maxh || c->is.fs;
		enabled = c->can.max || c->can.maxv || c->can.maxh || c->can.fs;
		break;
	case CloseBtn:
		name = "close";
		present = c->has.but.close;
		toggled = False;
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
		present = False;
		toggled = False;
		enabled = False;
		break;
	case RHalfBtn:
		name = "rhalf";
		present = False;
		toggled = False;
		enabled = False;
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
		toggled = False;
		enabled = c->can.size || c->can.sizev || c->can.sizeh;
		break;
	default:
		name = "unknown";
		present = False;
		toggled = False;
		enabled = False;
		break;
	}
	DPRINTF("Getting button image for '%s'\n", name);
	if (!present) {
		DPRINTF("button %s is not present!\n", name);
		return NULL;
	}

	ec = &c->element[type];
	pressed = ec->pressed ? True : False;
	hovered = ec->hovered;
	focused = (c == give) || (c == sel);

	e = &ds->element[type];
	if (!e->action) {
		DPRINTF("button %s has no actions!\n", name);
		return NULL;
	}

	if (pressed && toggled && e->image[ButtonImageToggledPressed].present) {
		DPRINTF("button %s assigned toggled.pressed image\n", name);
		return &e->image[ButtonImageToggledPressed];
	}
	if (pressed && e->image[ButtonImagePressed].present) {
		DPRINTF("button %s assigned pressed image\n", name);
		return &e->image[ButtonImagePressed];
	}
	image = ButtonImageHover + (hovered ? 0 : (focused ? 1 : 2));
	image += toggled ? 3 : 0;
	image += enabled ? 0 : 6;
	if (hovered && !e->image[image].present) {
		DPRINTF("button %s has no hovered image %d, using %s instead\n", name, image, focused ? "focused" : "unfocus");
		image += focused ? 1 : 2;
	}
	if (!focused && !e->image[image].present) {
		DPRINTF("button %s has no unfocus image %d, using focused instead\n", name, image);
		image -= 1;
	}
	if (toggled && !e->image[image].present) {
		DPRINTF("button %s has no toggled image %d, using untoggled instead\n", name, image);
		image -= 3;
	}
	if (!enabled && !e->image[image].present) {
		DPRINTF("button %s missing disabled image %d, skipping button\n", name, image);
		return NULL;
	}
	if (e->image[image].present) {
		DPRINTF("button %s going with chosen image %d\n", name, image);
		return &e->image[image];
	}
	if (e->image[ButtonImageDefault].present) {
		DPRINTF("button %s missing chosen image %d, going with default\n", name, image);
		return &e->image[ButtonImageDefault];
	}
	DPRINTF("button %s missing default image, skipping button\n", name);
	return NULL;
}

static int
drawbutton(EScreen *ds, Client *c, ElementType type, unsigned long col[ColLast], int x)
{
	ElementClient *ec = &c->element[type];
	Drawable d = c->drawable;
	ButtonImage *bi;

	if (!(bi = buttonimage(ds, c, type)) || !bi->present) {
		DPRINTF("button %d has no button image\n", type);
		return 0;
	}

	ec->g.x = x;
	ec->g.y = (ds->dc.h - bi->h) / 2;
	ec->g.w = bi->w;
	ec->g.h = bi->h;

#if defined IMLIB2 || defined XPM
	if (bi->pixmap) {
		DPRINTF("Copying pixmap 0x%lx with geom %dx%d+%d+%d to drawable 0x%lx\n",
			bi->pixmap, ec->g.w, ec->g.h, ec->g.x, ec->g.y, d);
		XCopyArea(dpy, bi->pixmap, d, ds->dc.gc, 0, bi->y, ec->g.w, ec->g.h, ec->g.x, ec->g.y);
		return ec->g.w;
	} else
#endif
	if (bi->bitmap) {
		XSetForeground(dpy, ds->dc.gc, col[ColBG]);
		XFillRectangle(dpy, d, ds->dc.gc, ec->g.x, 0, ds->dc.h, ds->dc.h);
		XSetForeground(dpy, ds->dc.gc, ec->pressed ? col[ColFG] : col[ColButton]);
		XSetBackground(dpy, ds->dc.gc, col[ColBG]);
		XCopyPlane(dpy, bi->bitmap, d, ds->dc.gc, 0, 0, ec->g.w, ec->g.h, ec->g.x, ec->g.y + bi->y, 1);
		return ds->dc.h;
	}
	DPRINTF("button %d has no pixmap or bitmap\n", type);
	return 0;
}

static ElementType
elementtype(char which)
{
	switch (which) {
	case 'I': return IconifyBtn;
	case 'M': return MaximizeBtn;
	case 'C': return CloseBtn;
	case 'S': return ShadeBtn;
	case 'A': return StickBtn;
	case 'L': return LHalfBtn;
	case 'R': return RHalfBtn;
	case 'X': return FillBtn;
	case 'F': return FloatBtn;
	case 'Z': return SizeBtn;
	case 'T': return TitleTags;
	case 'N': return TitleName;
	case '|': return TitleSep;
	default:  return LastElement;
	}
}

static int
drawelement(EScreen *ds, char which, int x, int position, Client *c)
{
	int w = 0;
	unsigned long *color = c == sel ? ds->style.color.sel : ds->style.color.norm;
	ElementType type = elementtype(which);
	ElementClient *ec = &c->element[type];

	if (0 > type || type >= LastElement)
		return 0;

	DPRINTF("drawing element '%c' for client %s\n", which, c->name);

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
			tw = c->tags[j] ? drawtext(ds, ds->tags[j], c->drawable,
						   c->xftdraw, color, x, ds->dc.y,
						   ds->dc.w) : 0;
		break;
	case TitleSep:
		XSetForeground(dpy, ds->dc.gc, color[ColBorder]);
		XDrawLine(dpy, c->drawable, ds->dc.gc, ds->dc.x + ds->dc.h / 4, 0,
			  ds->dc.x + ds->dc.h / 4, ds->dc.h);
		w = ds->dc.h / 2;
		break;
	case TitleName:
		w = drawtext(ds, c->name, c->drawable, c->xftdraw, color,
			     ds->dc.x, ds->dc.y, ds->dc.w);
		break;
	default:
		if (0 <= type && type < LastBtn)
			w = drawbutton(ds, c, type, color, ds->dc.x);
		break;
	}
	if (w) {
		ec->present = True;
		ec->g.w = w;
		DPRINTF("element '%c' at %dx%d+%d+%d:%d for client '%s'\n",
				which, ec->g.w, ec->g.h, ec->g.x, ec->g.y,
				ec->g.b, c->name);
	} else
		DPRINTF("missing element '%c' for client %s\n", which, c->name);
	return w;
}

static int
buttonw(EScreen *ds, Client *c, ElementType type)
{
	ButtonImage *bi;

	if (!(bi = buttonimage(ds, c, type)) || !bi->present)
		return 0;
	return bi->w + 2 * bi->b;
}

static int
elementw(EScreen *ds, Client *c, char which)
{
	int w = 0;
	ElementType type = elementtype(which);

	if (0 > type || type >= LastElement)
		return 0;

	switch (type) {
		unsigned int j;

	case TitleName:
		w = textw(ds, c->name);
		break;
	case TitleTags:
		for (j = 0; j < ds->ntags; j++) {
			if (c->tags[j])
				w += textw(ds, ds->tags[j]);
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
		ds->dc.w -= elementw(ds, c, CloseBtn);
		drawtext(ds, c->name, c->drawable, c->xftdraw,
		    c == sel ? ds->style.color.sel : ds->style.color.norm, ds->dc.x, ds->dc.y, ds->dc.w);
		drawbutton(ds, c, CloseBtn,
		    c == sel ? ds->style.color.sel : ds->style.color.norm, ds->dc.w);
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
		ds->dc.x -= elementw(ds, c, ds->style.titlelayout[i]) / 2;
		ds->dc.x += drawelement(ds, ds->style.titlelayout[i], 0, AlignCenter, c);
	}
	if (i == strlen(ds->style.titlelayout) || ds->dc.x >= ds->dc.w)
		goto end;
	/* Right */
	ds->dc.x = ds->dc.w;
	for (i = strlen(ds->style.titlelayout); i-- ; ) {
		if (ds->style.titlelayout[i] == ' ' || ds->style.titlelayout[i] == '-')
			break;
		ds->dc.x -= elementw(ds, c, ds->style.titlelayout[i]);
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
initpixmap(const char *file, ButtonImage *bi)
{
	if (!file)
		return 1;
#ifdef XPM
	if (strstr(file, ".xpm") && strlen(strstr(file, ".xpm")) == 4) {
		XpmAttributes xa = { 0, };

		if (XpmReadFileToPixmap(dpy, scr->root, file, &bi->pixmap, &bi->mask, &xa)
		    == Success) {
			if ((bi->w = xa.width) && (bi->h = xa.height)) {
				if (bi->h > scr->style.titleheight) {
					bi->y += (bi->h - scr->style.titleheight) / 2;
					bi->h = scr->style.titleheight;
				}
				return 0;
			}
		}
	}
#endif
#ifdef IMLIB2
	if (!strstr(file, ".xbm") || strlen(strstr(file, ".xbm")) != 4) {
		Imlib_Image image;

		imlib_context_push(scr->context);
		imlib_context_set_mask(None);

		if (!(image = imlib_load_image(file))) {
			if (file[0] != '/') {
				/* TODO: look for it else where */
			}
		}
		if (image) {
			imlib_context_set_mask(None);
			imlib_context_set_image(image);
			bi->w = imlib_image_get_width();
			bi->h = imlib_image_get_height();
			if (bi->h > scr->style.titleheight)
				bi->h = scr->style.titleheight;
			if (bi->w > 2 * scr->style.titleheight)
				bi->w = 2 * scr->style.titleheight;
			imlib_render_pixmaps_for_whole_image_at_size
			    (&bi->pixmap, &bi->mask, bi->w, bi->h);
		} else {
			DPRINTF("could not load image file %s\n", file);
		}
		imlib_context_pop();
		if (image)
			return 0;
	}
#endif
	if (strstr(file, ".xbm") && strlen(strstr(file, ".xbm")) == 4) {
		bi->bitmap =
		    XCreatePixmap(dpy, scr->root, scr->style.titleheight,
				  scr->style.titleheight, 1);
		if (BitmapSuccess ==
		    XReadBitmapFile(dpy, scr->root, file, &bi->w, &bi->h, &bi->bitmap,
				    &bi->x, &bi->y)) {
			if (bi->x == -1 || bi->y == -1)
				bi->x = bi->y = 0;
			return 0;
		}
	}
	return 1;
}

static void
b_min(Client *c, XEvent *ev)
{
	iconify(c);
}

static void
b_max(Client *c, XEvent *ev)
{
	switch (ev->xbutton.button) {
	case Button1:
		togglemax(c);
		break;
	case Button2:
		togglemaxv(c);
		break;
	case Button3:
		togglemaxh(c);
		break;
	}
}

static void
b_close(Client *c, XEvent *ev)
{
	killclient(c);
}

static void
b_shade(Client *c, XEvent *ev)
{
	switch (ev->xbutton.button) {
	case Button1:
		toggleshade(c);
		break;
	case Button2:
		if (!c->is.shaded)
			toggleshade(c);
		break;
	case Button3:
		if (c->is.shaded)
			toggleshade(c);
		break;
	}
}

static void
b_stick(Client *c, XEvent *ev)
{
	togglesticky(c);
}

static void
b_lhalf(Client *c, XEvent *ev)
{
}

static void
b_rhalf(Client *c, XEvent *ev)
{
}

static void
b_fill(Client *c, XEvent *ev)
{
	switch (ev->xbutton.button) {
	case Button1:
		togglefill(c);
		break;
	case Button2:
		if (!c->is.fill)
			togglefill(c);
		break;
	case Button3:
		if (c->is.fill)
			togglefill(c);
		break;
	}
}

static void
b_float(Client *c, XEvent *ev)
{
	switch (ev->xbutton.button) {
	case Button1:
		togglefloating(c);
		break;
	case Button2:
		if (!c->skip.arrange)
			togglefloating(c);
		break;
	case Button3:
		if (c->skip.arrange)
			togglefloating(c);
		break;
	}
}

static void
b_resize(Client *c, XEvent *ev)
{
	m_resize(c, ev);
}

static void
initelement(ElementType type, const char *name, const char *def, void (**action)(Client *, XEvent *)) {
	char res[128];
	static const char *kind[LastButtonImageType] = {
		[ButtonImageDefault] = "",
		[ButtonImagePressed] = ".pressed",
		[ButtonImageToggledPressed] = ".toggled.pressed",
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
			if ((e->image[i].present = !initpixmap(getresource(res, def), &e->image[i])))
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
initbuttons() {
	int i;
	static struct {
		const char *name;
		const char *def;
		void (*action[Button5][2])(Client *, XEvent *);
	} setup[LastElement] = {
		/* *INDENT-OFF* */
		[IconifyBtn]	= { "button.iconify",	ICONPIXMAP,	{
				     /* ButtonPress	ButtonRelease	*/
			[Button1-1] = { NULL,		b_min		},
			[Button2-1] = { NULL,		b_min		},
			[Button3-1] = { NULL,		b_min		},
			[Button4-1] = { NULL,		NULL		},
			[Button5-1] = { NULL,		NULL		},
		} },
		[MaximizeBtn]	= { "button.maximize",	MAXPIXMAP,	{
			[Button1-1] = { NULL,		b_max		},
			[Button2-1] = { NULL,		b_max		},
			[Button3-1] = { NULL,		b_max		},
			[Button4-1] = { NULL,		NULL		},
			[Button5-1] = { NULL,		NULL		},
		} },
		[CloseBtn]	= { "button.close",	CLOSEPIXMAP,	{
			[Button1-1] = { NULL,		b_close		},
			[Button2-1] = { NULL,		NULL		},
			[Button3-1] = { NULL,		NULL		},
			[Button4-1] = { NULL,		NULL		},
			[Button5-1] = { NULL,		NULL		},
		} },
		[ShadeBtn]	= { "button.shade",	SHADEPIXMAP,	{
			[Button1-1] = { NULL,		b_shade		},
			[Button2-1] = { NULL,		b_shade		},
			[Button3-1] = { NULL,		b_shade		},
			[Button4-1] = { NULL,		NULL		},
			[Button5-1] = { NULL,		NULL		},
		} },
		[StickBtn]	= { "button.stick",	STICKPIXMAP,	{
			[Button1-1] = { NULL,		b_stick		},
			[Button2-1] = { NULL,		b_stick		},
			[Button3-1] = { NULL,		b_stick		},
			[Button4-1] = { NULL,		NULL		},
			[Button5-1] = { NULL,		NULL		},
		} },
		[LHalfBtn]	= { "button.lhalf",	LHALFPIXMAP,	{
			[Button1-1] = { NULL,		b_lhalf		},
			[Button2-1] = { NULL,		b_lhalf		},
			[Button3-1] = { NULL,		b_lhalf	},
			[Button4-1] = { NULL,		NULL		},
			[Button5-1] = { NULL,		NULL		},
		} },
		[RHalfBtn]	= { "button.rhalf",	RHALFPIXMAP,	{
			[Button1-1] = { NULL,		b_rhalf		},
			[Button2-1] = { NULL,		b_rhalf		},
			[Button3-1] = { NULL,		b_rhalf		},
			[Button4-1] = { NULL,		NULL		},
			[Button5-1] = { NULL,		NULL		},
		} },
		[FillBtn]	= { "button.fill",	FILLPIXMAP,	{
			[Button1-1] = { NULL,		b_fill		},
			[Button2-1] = { NULL,		b_fill		},
			[Button3-1] = { NULL,		b_fill		},
			[Button4-1] = { NULL,		NULL		},
			[Button5-1] = { NULL,		NULL		},
		} },
		[FloatBtn]	= { "button.float",	FLOATPIXMAP,	{
			[Button1-1] = { NULL,		b_float		},
			[Button2-1] = { NULL,		b_float		},
			[Button3-1] = { NULL,		b_float		},
			[Button4-1] = { NULL,		NULL		},
			[Button5-1] = { NULL,		NULL		},
		} },
		[SizeBtn]	= { "button.resize",	SIZEPIXMAP,	{
			[Button1-1] = { b_resize,	NULL		},
			[Button2-1] = { b_resize,	NULL		},
			[Button3-1] = { b_resize,	NULL		},
			[Button4-1] = { NULL,		NULL		},
			[Button5-1] = { NULL,		NULL		},
		} },
		[TitleTags]	= { "title.tags",	NULL,		{
			[Button1-1] = { NULL,		NULL		},
			[Button2-1] = { NULL,		NULL		},
			[Button3-1] = { NULL,		NULL		},
			[Button4-1] = { NULL,		NULL		},
			[Button5-1] = { NULL,		NULL		},
		} },
		[TitleName]	= { "title.name",	NULL,		{
			[Button1-1] = { mousemove,	NULL		},
			[Button2-1] = { NULL,		NULL		},
			[Button3-1] = { mouseresize,	NULL		},
			[Button4-1] = { NULL,		NULL		},
			[Button5-1] = { NULL,		NULL		},
		} },
		[TitleSep]	= { "title.separator",	NULL,		{
			[Button1-1] = { NULL,		NULL		},
			[Button2-1] = { NULL,		NULL		},
			[Button3-1] = { NULL,		NULL		},
			[Button4-1] = { NULL,		NULL		},
			[Button5-1] = { NULL,		NULL		},
		} },
		/* *INDENT-ON* */

	};

	XSetForeground(dpy, scr->dc.gc, scr->style.color.norm[ColButton]);
	XSetBackground(dpy, scr->dc.gc, scr->style.color.norm[ColBG]);

	for (i = 0; i < LastElement; i++)
		initelement(i, setup[i].name, setup[i].def, &setup[i].action[0][0]);
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
