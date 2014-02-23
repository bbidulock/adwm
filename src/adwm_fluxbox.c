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

typedef enum {
	JustifyLeft,
	JustifyRight,
	JustifyCenter
} FluxboxJustify;

typedef enum {
	CornersTop,
	CornersBottom,
	CornersLeft,
	CornersRight
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
			Pixmap pixmap;
			struct {
				Pixmap pixmap;
			} pressed, unfocus;
		} close, iconify, maximize, shade, stick;
		struct {
			Pixmap pixmap;
			struct {
				Pixmap pixmap;
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
		FluxboxJustify justify;
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
	(void) style;
	(void) res;
	/* FIXME: complete this */
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
