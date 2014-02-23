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

/*
 * The purpose of this file is to provide a loadable module that provides
 * functions to mimic the behaviour and style of fluxbox.  This module reads the
 * fluxbox configuration file to determine configuration information and
 * accesses a fluxbox(1) style file.  It also reads the fluxbox(1) keys
 * configuration file to find key and mouse bindings.
 *
 * Note that fluxbox(1) does not permit its style to change anything regarding
 * key bindings or configuration settings: just styles.
 *
 * This module is really just intended to access a large volume of fluxbox
 * styles.  Fluxbox style files are similar to blackbox(1), openbox(1) and
 * waimea(1).
 */

typedef struct {
	Bool topleft;
	Bool topright;
	Bool botleft;
	Bool botright;
} FluxboxCorners;

typedef struct {
	XftFont *font;
	XGlyphInfo extents;
	unsigned ascent;
	unsigned descent;
	unsigned height;
	unsigned width;
} FluxboxFont;

typedef enum {
	EffectNone,
	EffectShadow,
	EffectHalo
} FluxboxEffect;

typedef struct {
	struct {
		struct {
			Texture focus, unfocus, pressed;
		} button;
		struct {
			AdwmPixmap pixmap;
			struct {
				AdwmPixmap pixmap;
			} pressed, unfocus;
		} close, iconify, maximize, shade, stick;
		struct {
			AdwmPixmap pixmap;
			struct {
				AdwmPixmap pixmap;
			} unfocus;
		} stuck, lhalf, rhalf;
		struct {
			Texture focus, unfocus;
		} grip, handle;
		struct {
			Texture focus, unfocus, active;
		} label;
		struct {
			Texture focus, unfocus;
			unsigned height;
		} title;
		struct {
			XColor focusColor;
			XColor unfocusColor;
		} frame;

		unsigned bevelWidth;
		Bool shaded;
		unsigned borderWidth;
		XColor borderColor;
		FluxboxCorners roundCorners;
		Justify justify;
		unsigned handleWidth;
		struct {
			AdwmFont font;
			FluxboxEffect effect;
			struct {
				XftColor color;
				unsigned x;
				unsigned y;
			} shadow;
			struct {
				XftColor color;
			} halo;
		};
	} window;
	/* fallbacks: obsolete */
	XColor borderColor;
	unsigned borderWidth;
	unsigned bevelWidth;
	unsigned frameWidth;
	unsigned handleWidth;
} FluxboxStyle;

static void
initkeys_FLUXBOX(void)
{
}

static FluxboxStyle *styles = NULL;

static void
initstyle_FLUXBOX(void)
{
	const char *res;
	FluxboxStyle *style;

	if (!styles)
		styles = ecalloc(nscr, sizeof(*styles));
	style = styles + scr->screen;

	readtexture("window.button.focus", "Window.Button.Focus",
		    &style->window.button.focus, "white", "black");
	readtexture("window.button.unfocus", "Window.Button.Unfocus",
		    &style->window.button.unfocus, "black", "white");
	readtexture("window.button.pressed", "Window.Button.Pressed",
		    &style->window.button.pressed, "white", "black");

	res = readres("window.close.pixmap", "Window.Close.Pixmap", NULL);
	getpixmap(res, &style->window.close.pixmap);
	res = readres("window.close.pressed.pixmap", "Window.Close.Pressed.Pixmap", NULL);
	getpixmap(res, &style->window.close.pressed.pixmap);
	res = readres("window.close.unfocus.pixmap", "Window.Close.Unfocus.Pixmap", NULL);
	getpixmap(res, &style->window.close.unfocus.pixmap);

	res = readres("window.iconify.pixmap", "Window.Close.Pixmap", NULL);
	getpixmap(res, &style->window.iconify.pixmap);
	res = readres("window.iconify.pressed.pixmap",
		      "Window.Close.Pressed.Pixmap", NULL);
	getpixmap(res, &style->window.iconify.pressed.pixmap);
	res = readres("window.iconify.unfocus.pixmap",
		      "Window.Close.Unfocus.Pixmap", NULL);
	getpixmap(res, &style->window.iconify.unfocus.pixmap);

	res = readres("window.maximize.pixmap", "Window.Close.Pixmap", NULL);
	getpixmap(res, &style->window.maximize.pixmap);
	res = readres("window.maximize.pressed.pixmap",
		      "Window.Close.Pressed.Pixmap", NULL);
	getpixmap(res, &style->window.maximize.pressed.pixmap);
	res = readres("window.maximize.unfocus.pixmap",
		      "Window.Close.Unfocus.Pixmap", NULL);
	getpixmap(res, &style->window.maximize.unfocus.pixmap);

	res = readres("window.shade.pixmap", "Window.Close.Pixmap", NULL);
	getpixmap(res, &style->window.shade.pixmap);
	res = readres("window.shade.pressed.pixmap", "Window.Close.Pressed.Pixmap", NULL);
	getpixmap(res, &style->window.shade.pressed.pixmap);
	res = readres("window.shade.unfocus.pixmap", "Window.Close.Unfocus.Pixmap", NULL);
	getpixmap(res, &style->window.shade.unfocus.pixmap);

	res = readres("window.stick.pixmap", "Window.Close.Pixmap", NULL);
	getpixmap(res, &style->window.stick.pixmap);
	res = readres("window.stick.pressed.pixmap", "Window.Close.Pressed.Pixmap", NULL);
	getpixmap(res, &style->window.stick.pressed.pixmap);
	res = readres("window.stick.unfocus.pixmap", "Window.Close.Unfocus.Pixmap", NULL);
	getpixmap(res, &style->window.stick.unfocus.pixmap);

	res = readres("window.stuck.pixmap", "Window.Close.Pixmap", NULL);
	getpixmap(res, &style->window.stuck.pixmap);
	res = readres("window.stuck.unfocus.pixmap", "Window.Close.Unfocus.Pixmap", NULL);
	getpixmap(res, &style->window.stuck.unfocus.pixmap);

	res = readres("window.lhalf.pixmap", "Window.Close.Pixmap", NULL);
	getpixmap(res, &style->window.lhalf.pixmap);
	res = readres("window.lhalf.unfocus.pixmap", "Window.Close.Unfocus.Pixmap", NULL);
	getpixmap(res, &style->window.lhalf.unfocus.pixmap);

	res = readres("window.rhalf.pixmap", "Window.Close.Pixmap", NULL);
	getpixmap(res, &style->window.rhalf.pixmap);
	res = readres("window.rhalf.unfocus.pixmap", "Window.Close.Unfocus.Pixmap", NULL);
	getpixmap(res, &style->window.rhalf.unfocus.pixmap);

	readtexture("window.grip.focus", "Window.Grip.Focus",
		    &style->window.grip.focus, "white", "black");
	readtexture("window.grip.unfocus", "Window.Grip.Unfocus",
		    &style->window.grip.unfocus, "black", "white");
	readtexture("window.handle.focus", "Window.Handle.Focus",
		    &style->window.handle.focus, "white", "black");
	readtexture("window.handle.unfocus", "Window.Handle.Unfocus",
		    &style->window.handle.unfocus, "black", "white");
	readtexture("window.label.focus", "Window.Label.Focus",
		    &style->window.label.focus, "white", "black");
	readtexture("window.label.unfocus", "Window.Label.Unfocus",
		    &style->window.label.unfocus, "black", "white");
	readtexture("window.label.active", "Window.Label.Active",
		    &style->window.label.active, "white", "black");
	readtexture("window.title.focus", "Window.Title.Focus",
		    &style->window.title.focus, "white", "black");
	readtexture("window.title.unfocus", "Window.Title.Unfocus",
		    &style->window.title.unfocus, "black", "white");
	res = readres("window.title.height", "Window.Title.Height", "12");
	style->window.title.height = strtoul(res, NULL, 0);

	res = readres("window.frame.focusColor", "Window.Frame.FocusColor", "black");
	getxcolor(res, "black", &style->window.frame.focusColor);
	res = readres("window.frame.unfocusColor", "Window.Frame.UnfocusColor", "white");
	getxcolor(res, "white", &style->window.frame.unfocusColor);

	res = readres("window.bevelWidth", "Window.BevelWidth", "1");
	style->window.bevelWidth = strtoul(res, NULL, 0);
	res = readres("window.shade", "Window.Shade", "False");
	style->window.shaded = strcasestr(res, "true") ? True : False;
	res = readres("window.borderWidth", "Window.BorderWidth", "1");
	style->window.borderWidth = strtoul(res, NULL, 0);
	res = readres("window.borderColor", "Window.BorderColor", "black");
	getxcolor(res, "black", &style->window.borderColor);

	res = readres("window.roundCorners", "Window.RoundCorners", "");
	style->window.roundCorners.topleft = strcasestr(res, "topleft") ? True : False;
	style->window.roundCorners.topright = strcasestr(res, "topright") ? True : False;
	style->window.roundCorners.botleft = strcasestr(res, "botleft") ? True : False;
	style->window.roundCorners.botright = strcasestr(res, "botright") ? True : False;

	res = readres("window.justify", "Window.Justify", "left");
	style->window.justify = JustifyLeft;
	if (strcasestr(res, "left"))
		style->window.justify = JustifyLeft;
	if (strcasestr(res, "center"))
		style->window.justify = JustifyCenter;
	if (strcasestr(res, "right"))
		style->window.justify = JustifyRight;

	res = readres("window.handleWidth", "Window.HandleWidth", "2");
	style->window.handleWidth = strtoul(res, NULL, 0);

	res = readres("window.font", "Window.Font", "sans 8");
	getfont(res, "sans 8", &style->window.font);

	res = readres("window.effect", "Window.Effect", "");
	style->window.effect = EffectNone;
	if (strcasestr(res, "shadow"))
		style->window.effect = EffectShadow;
	if (strcasestr(res, "halo"))
		style->window.effect = EffectHalo;

	res = readres("window.shadow.color", "Window.Shadow.Color", "black");
	getxftcolor(res, "black", &style->window.shadow.color);
	res = readres("window.shadow.x", "Window.Shadow.X", "0");
	style->window.shadow.x = strtoul(res, NULL, 0);
	res = readres("window.shadow.y", "Window.Shadow.Y", "0");
	style->window.shadow.y = strtoul(res, NULL, 0);

	res = readres("window.halo.color", "Window.Halo.Color", "black");
	getxftcolor(res, "black", &style->window.halo.color);

	res = readres("borderColor", "BorderColor", "black");
	getxcolor(res, "black", &style->borderColor);
	res = readres("borderWidth", "BorderWidth", "1");
	style->borderWidth = strtoul(res, NULL, 0);
	res = readres("bevelWidth", "BevelWidth", "0");
	style->bevelWidth = strtoul(res, NULL, 0);
	res = readres("frameWidth", "FrameWidth", "1");
	style->frameWidth = strtoul(res, NULL, 0);
	res = readres("handleWidth", "HandleWidth", "2");
	style->handleWidth = strtoul(res, NULL, 0);
}

static void
deinitstyle_FLUXBOX(void)
{
}

static void
drawclient_FLUXBOX(Client *c)
{
}

AdwmOperations adwm_ops = {
	.name = "fluxbox",
	.initkeys = &initkeys_FLUXBOX,
	.initstyle = &initstyle_FLUXBOX,
	.deinitstyle = &deinitstyle_FLUXBOX,
	.drawclient = &drawclient_FLUXBOX,
};

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
