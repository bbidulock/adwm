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
 * The purpose of this file is to provide a loadable modules that provides
 * functions to mimic the behaviour and style of openbox.  This module reads the
 * openbox configuration file to determine configuration information, key and
 * mouse bindings.  It also reads the openbox(1) style file.
 *
 * Note that openbox(1) does not permit its style to change anything regarding
 * key or mouse bindings or configuration settings; just style.  Also,
 * openbox(1) does not have a keys file that is separate from the primary
 * configuration file.  This is a serious disadvantage.
 *
 * This module is really just intended to access a large volume of openbox
 * styles.  Openbox style files are similar to blackbox(1), fluxbox(1) and
 * waimea(1).
 */

typedef enum {
	JustifyLeft,
	JustifyRight,
	JustifyCenter
} OpenboxJustify;

typedef enum {
	CornersTop,
	CornersBottom,
	CornersLeft,
	CornersRight
} OpenboxCorners;

typedef struct {
	XftFont *font;
	XGlyphInfo extents;
	unsigned ascent;
	unsigned descent;
	unsigned height;
	unsigned width;
} OpenboxFont;

typedef enum {
	EffectNone,
	EffectShadow,
	EffectHalo
} OpenboxEffect;

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
		// Bool shade; /* XXX */
		unsigned borderWidth;
		XColor borderColor;
		OpenboxCorners roundCorners;
		OpenboxJustify justify;
		unsigned handleWidth;
		struct {
			OpenboxFont font;
			OpenboxEffect effect;
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
} OpenboxStyle;

static void
initkeys_OPENBOX(void)
{
}

static OpenboxStyle *styles = NULL;

static void
initstyle_OPENBOX(void)
{
	const char *res;
	OpenboxStyle *style;

	if (!styles)
		styles = ecalloc(nscr, sizeof(*styles));
	style = styles + scr->screen;
	(void) res;
	(void) style;
	/* FIXME: complete this */

}

static void
deinitstyle_OPENBOX(void)
{
}

static void
drawclient_OPENBOX(Client *c)
{
}

AdwmOperations adwm_ops = {
	.name = "fluxbox",
	.initkeys = &initkeys_OPENBOX,
	.initstyle = &initstyle_OPENBOX,
	.deinitstyle = &deinitstyle_OPENBOX,
	.drawclient = &drawclient_OPENBOX,
};

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
