/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "actions.h"
#include "config.h"
#include "icons.h"
#if defined IMLIB2 && defined USE_IMLIB2
#include "imlib.h"
#endif				/* defined IMLIB2 && defined USE_IMLIB2 */
#if defined PIXBUF && defined USE_PIXBUF
#include "pixbuf.h"
#endif				/* defined PIXBUF && defined USE_PIXBUF */
#if defined RENDER && defined USE_RENDER
#include "render.h"
#endif				/* defined RENDER && defined USE_RENDER */
#if 1
#include "ximage.h"
#else
#include "xlib.h"
#endif
#include "draw.h"		/* verification */

#if defined IMLIB2 && defined USE_IMLIB2
#define createbitmapicon(args...) imlib2_createbitmapicon(args)
#define createbitmapicon(args...) imlib2_createbitmapicon(args)
#define createpixmapicon(args...) imlib2_createpixmapicon(args)
#define createdataicon(args...)   imlib2_createdataicon(args)
#define createpngicon(args...)	  imlib2_createpngicon(args)
#define createjpgicon(args...)	  imlib2_createjpgicon(args)
#define createsvgicon(args...)	  imlib2_createsvgicon(args)
#define createxpmicon(args...)	  imlib2_createxpmicon(args)
#define createxbmicon(args...)	  imlib2_createxbmicon(args)
#define drawbutton(args...)	  imlib2_drawbutton(args)
#define drawtext(args...)	  imlib2_drawtext(args)
#define drawsep(args...)	  imlib2_drawsep(args)
#define drawdockapp(args...)	  imlib2_drawdockapp(args)
#define drawnormal(args...)	  imlib2_drawnormal(args)
#define initpng(args...)	  imlib2_initpng(args)
#define initjpg(args...)	  imlib2_initjpg(args)
#define initsvg(args...)	  imlib2_initsvg(args)
#define initxpm(args...)	  imlib2_initxpm(args)
#define initxbm(args...)	  imlib2_initxbm(args)
#else				/* !defined IMLIB2 || !defined USE_IMLIB2 */
#if defined PIXBUF && defined USE_PIXBUF
#define createbitmapicon(args...) pixbuf_createbitmapicon(args)
#define createpixmapicon(args...) pixbuf_createpixmapicon(args)
#define createdataicon(args...)   pixbuf_createdataicon(args)
#define createpngicon(args...)	  pixbuf_createpngicon(args)
#define createjpgicon(args...)	  pixbuf_createjpgicon(args)
#define createsvgicon(args...)	  pixbuf_createsvgicon(args)
#define createxpmicon(args...)	  pixbuf_createxpmicon(args)
#define createxbmicon(args...)	  pixbuf_createxbmicon(args)
#define drawbutton(args...)	  pixbuf_drawbutton(args)
#define drawtext(args...)	  pixbuf_drawtext(args)
#define drawsep(args...)	  pixbuf_drawsep(args)
#define drawdockapp(args...)	  pixbuf_drawdockapp(args)
#define drawnormal(args...)	  pixbuf_drawnormal(args)
#define initpng(args...)	  pixbuf_initpng(args)
#define initjpg(args...)	  pixbuf_initjpg(args)
#define initsvg(args...)	  pixbuf_initsvg(args)
#define initxpm(args...)	  pixbuf_initxpm(args)
#define initxbm(args...)	  pixbuf_initxbm(args)
#else				/* !defined PIXBUF || !defined USE_PIXBUF */
#if defined RENDER && defined USE_RENDER
#define createbitmapicon(args...) render_createbitmapicon(args)
#define createpixmapicon(args...) render_createpixmapicon(args)
#define createdataicon(args...)   render_createdataicon(args)
#define createpngicon(args...)	  render_createpngicon(args)
#define createjpgicon(args...)	  render_createjpgicon(args)
#define createsvgicon(args...)	  render_createsvgicon(args)
#define createxpmicon(args...)	  render_createxpmicon(args)
#define createxbmicon(args...)	  render_createxbmicon(args)
#define drawbutton(args...)	  render_drawbutton(args)
#define drawtext(args...)	  render_drawtext(args)
#define drawsep(args...)	  render_drawsep(args)
#define drawdockapp(args...)	  render_drawdockapp(args)
#define drawnormal(args...)	  render_drawnormal(args)
#define initpng(args...)	  render_initpng(args)
#define initjpg(args...)	  render_initjpg(args)
#define initsvg(args...)	  render_initsvg(args)
#define initxpm(args...)	  render_initxpm(args)
#define initxbm(args...)	  render_initxbm(args)
#else				/* !defined RENDER || !defined USE_RENDER */
#if defined XCAIRO && defined USE_XCAIRO
#define createpixmapicon(args...) xcairo_createpixmapicon(args)
#define createdataicon(args...)   xcairo_createdataicon(args)
#define createpngicon(args...)	  xcairo_createpngicon(args)
#define createjpgicon(args...)	  xcairo_createjpgicon(args)
#define createsvgicon(args...)	  xcairo_createsvgicon(args)
#define createxpmicon(args...)	  xcairo_createxpmicon(args)
#define createxbmicon(args...)	  xcairo_createxbmicon(args)
#define drawbutton(args...)	  xcairo_drawbutton(args)
#define drawtext(args...)	  xcairo_drawtext(args)
#define drawsep(args...)	  xcairo_drawsep(args)
#define drawdockapp(args...)	  xcairo_drawdockapp(args)
#define drawnormal(args...)	  xcairo_drawnormal(args)
#define initpng(args...)	  xcairo_initpng(args)
#define initjpg(args...)	  xcairo_initjpg(args)
#define initsvg(args...)	  xcairo_initsvg(args)
#define initxpm(args...)	  xcairo_initxpm(args)
#define initxbm(args...)	  xcairo_initxbm(args)
#else				/* !defined XCAIRO || !defined USE_XCAIRO */
#if 1
#define createbitmapicon(args...) ximage_createbitmapicon(args)
#define createpixmapicon(args...) ximage_createpixmapicon(args)
#define createdataicon(args...)   ximage_createdataicon(args)
#define createpngicon(args...)	  ximage_createpngicon(args)
#define createjpgicon(args...)	  ximage_createjpgicon(args)
#define createsvgicon(args...)	  ximage_createsvgicon(args)
#define createxpmicon(args...)	  ximage_createxpmicon(args)
#define createxbmicon(args...)	  ximage_createxbmicon(args)
#define drawbutton(args...)	  ximage_drawbutton(args)
#define drawtext(args...)	  ximage_drawtext(args)
#define drawsep(args...)	  ximage_drawsep(args)
#define drawdockapp(args...)	  ximage_drawdockapp(args)
#define drawnormal(args...)	  ximage_drawnormal(args)
#define initpng(args...)	  ximage_initpng(args)
#define initjpg(args...)	  ximage_initjpg(args)
#define initsvg(args...)	  ximage_initsvg(args)
#define initxpm(args...)	  ximage_initxpm(args)
#define initxbm(args...)	  ximage_initxbm(args)
#else
#define createbitmapicon(args...) xlib_createbitmapicon(args)
#define createpixmapicon(args...) xlib_createpixmapicon(args)
#define createdataicon(args...)   xlib_createdataicon(args)
#define createpngicon(args...)	  xlib_createpngicon(args)
#define createjpgicon(args...)	  xlib_createjpgicon(args)
#define createsvgicon(args...)	  xlib_createsvgicon(args)
#define createxpmicon(args...)	  xlib_createxpmicon(args)
#define createxbmicon(args...)	  xlib_createxbmicon(args)
#define drawbutton(args...)	  xlib_drawbutton(args)
#define drawtext(args...)	  xlib_drawtext(args)
#define drawsep(args...)	  xlib_drawsep(args)
#define drawdockapp(args...)	  xlib_drawdockapp(args)
#define drawnormal(args...)	  xlib_drawnormal(args)
#define initpng(args...)	  xlib_initpng(args)
#define initjpg(args...)	  xlib_initjpg(args)
#define initsvg(args...)	  xlib_initsvg(args)
#define initxpm(args...)	  xlib_initxpm(args)
#define initxbm(args...)	  xlib_initxbm(args)
#endif
#endif				/* !defined XCAIRO || !defined USE_XCAIRO */
#endif				/* !defined RENDER || !defined USE_RENDER */
#endif				/* !defined PIXBUF || !defined USE_PIXBUF */
#endif				/* !defined IMLIB2 || !defined USE_IMLIB2 */

Bool
drawdamage(Client *c, XDamageNotifyEvent *ev)
{
	Bool status = False;

#if defined IMLIB2 && defined USE_IMLIB2
	status |= imlib2_drawdamage(c, ev);
#endif				/* defined IMLIB2 && defined USE_IMLIB2 */
#if defined PIXBUF && defined USE_PIXBUF
	status |= pixbuf_drawdamage(c, ev);
#endif				/* defined PIXBUF && defined USE_PIXBUF */
#if defined RENDER && defined USE_RENDER
	status |= render_drawdamage(c, ev);
#endif				/* defined RENDER && defined USE_RENDER */
#if 1
	status |= ximage_drawdamage(c, ev);
#else
	status |= xlib_drawdamage(c, ev);
#endif
	return (status);
}

void
removebutton(ButtonImage *bi)
{
	free(bi->px.file);
	bi->px.file = NULL;

#if defined IMLIB2 && defined USE_IMLIB2
	imlib2_removepixmap(&bi->px);
#endif				/* defined IMLIB2 && defined USE_IMLIB2 */
#if defined PIXBUF && defined USE_PIXBUF
	pixbuf_removepixmap(&bi->px);
#endif				/* defined PIXBUF && defined USE_PIXBUF */
#if defined RENDER && defined USE_RENDER
	render_removepixmap(&bi->px);
#endif				/* defined RENDER && defined USE_RENDER */
#if 1
	ximage_removepixmap(&bi->px);
#else
	xlib_removepixmap(&bi->px);
#endif
	bi->present = False;
}

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
		if (icons[i].h <= (unsigned) scr->style.titleheight)
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
		if (icons[i].h <= (unsigned) scr->style.titleheight)
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

Bool
createappicon(Client *c)
{
	const char *names[4] = { NULL, };
	const char *exts[5] = { NULL, };
	char *file, *p;
	int i = 0, j = 0;
	Bool result = False;

#ifdef STARTUP_NOTIFICATION
	if (c->seq)
		names[i++] = sn_startup_sequence_get_icon_name(c->seq);
#endif
	if (c->ch.res_class)
		names[i++] = c->ch.res_class;
	if (c->ch.res_name)
		names[i++] = c->ch.res_name;
	names[i] = NULL;

#if defined LIBPNG || defined IMLIB2 || defined PIXBUF
	exts[j++] = "png";
#endif
#if defined LIBRSVG
	exts[j++] = "svg";
#endif
#if defined XPM || defined IMLIB2 || defined PIXBUF
	exts[j++] = "xpm";
#endif
	exts[j++] = "xbm";
	exts[j] = NULL;

	if (!(file = FindBestIcon(names, scr->style.titleheight, exts)))
		return (result);
	if ((p = strrchr(file, '.'))) {
		p++;
		if (!strcmp(p, "xbm"))
			result = createxbmicon(scr, c, file);
#if defined XPM || defined IMLIB2 || defined PIXBUF
		else
		if (!strcmp(p, "xpm"))
			result = createxpmicon(scr, c, file);
#endif
#if defined LIBPNG || defined IMLIB2 || defined PIXBUF
		else
		if (!strcmp(p, "png"))
			result = createpngicon(scr, c, file);
#endif
#if defined LIBRSVG
		else
		if (!strcmp(p, "svg"))
			result = createsvgicon(scr, c, file);
#endif
	}
	free(file);
	return (result);
}

int
gethilite(Client *c)
{
	if (c == sel)
		return Selected;
	if (c == gave || c == took)
		return Focused;
	return Normal;
}

XftColor *
gethues(AScreen *ds, Client *c)
{
	if (c == sel)
		return ds->style.color.sele;
	if (c == gave || c == took)
		return ds->style.color.focu;
	return ds->style.color.norm;
}

XftColor *
getcolor(AScreen *ds, Client *c, int type)
{
	XftColor *col = gethues(ds, c);
	return &col[type];
}

unsigned long
getpixel(AScreen *ds, Client *c, int type)
{
	XftColor *col = getcolor(ds, c, type);
	return col->pixel;
}

unsigned int
textnw(AScreen *ds, const char *text, unsigned int len, int hilite)
{
	XftTextExtentsUtf8(dpy, ds->style.font[hilite],
			   (const unsigned char *) text, len,
			   ds->dc.font[hilite].extents);
	return ds->dc.font[hilite].extents->xOff;
}

unsigned int
textw(AScreen *ds, const char *text, int hilite)
{
	return textnw(ds, text, strlen(text), hilite) + ds->dc.font[hilite].height;
}

ButtonImage *
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

	if (type == IconBtn && (image = getbutton(c)))
		return (image);

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
	return bi->px.w + 2 * bi->px.b;
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

int
elementw(AScreen *ds, Client *c, char which)
{
	int w = 0;
	ElementType type = elementtype(which);
	int hilite = (c == sel) ? Selected : (c->is.focused ? Focused : Normal);

	if (0 > type || type >= LastElement)
		return 0;

	switch (type) {
		unsigned j;

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

unsigned
titleheight(AScreen *ds)
{
	unsigned th = ds->style.titleheight;
	if (ds->style.outline && ds->style.border < th)
		th -= ds->style.border;
	return th;
}

int
drawelement(AScreen *ds, char which, int x, int position, Client *c)
{
	int w = 0;
	XftColor *color = c == sel ? ds->style.color.sele : ds->style.color.norm;
	int hilite = (c == sel) ? Selected : (c->is.focused ? Focused : Normal);
	ElementType type = elementtype(which);
	ElementClient *ec = &c->element[type];

	(void) x;		/* XXX */
	(void) position;	/* XXX */

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
		int x, tw;
		unsigned j;

	case TitleTags:
		for (j = 0, x = ds->dc.x; j < ds->ntags; j++, w += tw, x += tw)
			tw = (c->tags & (1ULL << j)) ?
			    drawtext(ds, ds->tags[j].name, ds->dc.draw.pixmap,
				     ds->dc.draw.xft, color, hilite, x, ds->dc.y,
				     ds->dc.w) : 0;
		ec->eg.w = w;
		break;
	case TitleName:
		w = drawtext(ds, c->name, ds->dc.draw.pixmap, ds->dc.draw.xft, color,
			     hilite, ds->dc.x, ds->dc.y, ds->dc.w);
		ec->eg.w = w;
		break;
	case TitleSep:
		w = drawsep(ds, "|", ds->dc.draw.pixmap, ds->dc.draw.xft, color,
			    hilite, ds->dc.x, ds->dc.y, ds->dc.w);
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

void
drawclient(Client *c)
{
	AScreen *ds;

	/* might be drawing a client that is not on the current screen */
	if (!(ds = getscreen(c->win, True))) {
		XPRINTF("What? no screen for window 0x%lx???\n", c->win);
		return;
	}
	if (c == sel && !ds->colormapnotified)
		installcolormaps(ds, c, c->cmapwins);
	setopacity(c, (c == sel) ? OPAQUE : (unsigned) ds->style.opacity);
	if (!isvisible(c, NULL))
		return;
	if (c->is.dockapp)
		return drawdockapp(ds, c);
	if (!c->title && !c->grips)
		return;
	drawnormal(ds, c);
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

static Bool
initpixmap(const char *file, ButtonImage *bi)
{
	char *path, *p;

	if (!file || !(path = findrcpath(file)))
		return False;
	if ((p = strstr(path, ".xpm")) && strlen(p) == 4)
		if ((bi->present = initxpm(path, &bi->px)))
			return True;
	if ((p = strstr(path, ".xbm")) && strlen(p) == 4)
		if ((bi->present = initxbm(path, &bi->px)))
			return True;
	if ((p = strstr(path, ".png")) && strlen(p) == 4)
		if ((bi->present = initpng(path, &bi->px)))
			return True;
	if ((p = strstr(path, ".svg")) && strlen(p) == 4)
		if ((bi->present = initsvg(path, &bi->px)))
			return True;
	EPRINTF("could not load button element from %s\n", path);
	removebutton(bi);
	free(path);
	return False;
}

static void
freeelement(ElementType type)
{
	int i;
	Element *e = &scr->element[type];

	if (e->image && type < LastBtn) {
		for (i = 0; i < LastButtonImageType; i++)
			removebutton(&e->image[i]);
		free(e->image);
		e->image = NULL;
	}
}

void
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

Bool (*buttons[LastElement][Button5-Button1+1][2]) (Client *, XEvent *) = {
	/* *INDENT-OFF* */
	/* ElementType */
	[MenuBtn]	= {
				    /* ButtonPress	ButtonRelease	*/
		[Button1-Button1] = { NULL,		b_menu		},
		[Button2-Button1] = { NULL,		b_menu		},
		[Button3-Button1] = { NULL,		b_menu		},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	},
	[IconifyBtn]	= {
		[Button1-Button1] = { NULL,		b_iconify	},
		[Button2-Button1] = { NULL,		b_hide		},
		[Button3-Button1] = { NULL,		b_withdraw	},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	},
	[MaximizeBtn]	= {
		[Button1-Button1] = { NULL,		b_maxb		},
		[Button2-Button1] = { NULL,		b_maxv		},
		[Button3-Button1] = { NULL,		b_maxh		},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	},
	[CloseBtn]	= {
		[Button1-Button1] = { NULL,		b_close		},
		[Button2-Button1] = { NULL,		b_kill		},
		[Button3-Button1] = { NULL,		b_xkill		},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	},
	[ShadeBtn]	= {
		[Button1-Button1] = { NULL,		b_reshade	},
		[Button2-Button1] = { NULL,		b_shade		},
		[Button3-Button1] = { NULL,		b_unshade	},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	},
	[StickBtn]	= {
		[Button1-Button1] = { NULL,		b_restick	},
		[Button2-Button1] = { NULL,		b_stick		},
		[Button3-Button1] = { NULL,		b_unstick	},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	},
	[LHalfBtn]	= {
		[Button1-Button1] = { NULL,		b_relhalf	},
		[Button2-Button1] = { NULL,		b_lhalf		},
		[Button3-Button1] = { NULL,		b_unlhalf	},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	},
	[RHalfBtn]	= {
		[Button1-Button1] = { NULL,		b_rerhalf	},
		[Button2-Button1] = { NULL,		b_rhalf		},
		[Button3-Button1] = { NULL,		b_unrhalf	},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	},
	[FillBtn]	= {
		[Button1-Button1] = { NULL,		b_refill	},
		[Button2-Button1] = { NULL,		b_fill		},
		[Button3-Button1] = { NULL,		b_unfill	},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	},
	[FloatBtn]	= {
		[Button1-Button1] = { NULL,		b_refloat	},
		[Button2-Button1] = { NULL,		b_float		},
		[Button3-Button1] = { NULL,		b_tile		},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	},
	[SizeBtn]	= {
		[Button1-Button1] = { b_resize,		NULL		},
		[Button2-Button1] = { b_resize,		NULL		},
		[Button3-Button1] = { b_resize,		NULL		},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	},
	[IconBtn]	= {
		[Button1-Button1] = { NULL,		b_menu		},
		[Button2-Button1] = { NULL,		b_menu		},
		[Button3-Button1] = { NULL,		b_menu		},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	},
	[TitleTags]	= {
		[Button1-Button1] = { NULL,		NULL		},
		[Button2-Button1] = { NULL,		NULL		},
		[Button3-Button1] = { NULL,		NULL		},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	},
	[TitleName]	= {
		[Button1-Button1] = { b_move,		b_restack	},
		[Button2-Button1] = { b_move,		b_zoom		},
		[Button3-Button1] = { b_resize,		b_restack	},
		[Button4-Button1] = { b_shade,		NULL		},
		[Button5-Button1] = { b_unshade,	NULL		},
	},
	[TitleSep]	= {
		[Button1-Button1] = { NULL,		NULL		},
		[Button2-Button1] = { NULL,		NULL		},
		[Button3-Button1] = { NULL,		NULL		},
		[Button4-Button1] = { NULL,		NULL		},
		[Button5-Button1] = { NULL,		NULL		},
	}
	/* *INDENT-ON* */
};

static void
initbuttons()
{
	int i;
	static struct {
		const char *name;
		const char *def;
	} setup[LastElement] = {
		/* *INDENT-OFF* */
		[MenuBtn]	= { "button.menu",	MENUPIXMAP	},
		[IconifyBtn]	= { "button.iconify",	ICONPIXMAP,	},
		[MaximizeBtn]	= { "button.maximize",	MAXPIXMAP,	},
		[CloseBtn]	= { "button.close",	CLOSEPIXMAP,	},
		[ShadeBtn]	= { "button.shade",	SHADEPIXMAP,	},
		[StickBtn]	= { "button.stick",	STICKPIXMAP,	},
		[LHalfBtn]	= { "button.lhalf",	LHALFPIXMAP,	},
		[RHalfBtn]	= { "button.rhalf",	RHALFPIXMAP,	},
		[FillBtn]	= { "button.fill",	FILLPIXMAP,	},
		[FloatBtn]	= { "button.float",	FLOATPIXMAP,	},
		[SizeBtn]	= { "button.resize",	SIZEPIXMAP,	},
		[IconBtn]	= { "button.icon",	WINPIXMAP,	},
		[TitleTags]	= { "title.tags",	NULL,		},
		[TitleName]	= { "title.name",	NULL,		},
		[TitleSep]	= { "title.separator",	NULL,		},
		/* *INDENT-ON* */
	};

	XSetForeground(dpy, scr->dc.gc, scr->style.color.norm[ColButton].pixel);
	XSetBackground(dpy, scr->dc.gc, scr->style.color.norm[ColBG].pixel);

	for (i = 0; i < LastElement; i++)
		initelement(i, setup[i].name, setup[i].def, &buttons[i][0][0]);
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
initstyle(Bool reload __attribute__((unused)))
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
		LENGTH(scr->style.titlelayout)-1);
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
