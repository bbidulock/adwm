/* See COPYING file for copyright and license details. */
#include <unistd.h>
#include <errno.h>
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
 * functions for mimicing the behaviour and styles of blackbox.  This module
 * reads the blackbox configuration file to determine configuration information,
 * and accesses a blackbox(1) style file.  It also reads the bbkeys(1)
 * configuration file to find key bindings.
 *
 * This module is really just intended to access a large volume of blackbox
 * styles.  Blackbox style files are similar to fluxbox(1), openbox(1) and
 * waimea(1).
 */

typedef struct {
	struct {
		AdwmBitmap iconify, maximize, restore, close;
		struct {
			Texture focus;	/* default white, opposite black */
			Texture unfocus;	/* default black, opposite white */
			unsigned marginWidth;	/* => 2 */
		} title, label;
		struct {
			Texture focus;	/* default white, opposite black */
			Texture unfocus;	/* default black, opposite white */
			Texture pressed;	/* default black, opposite white */
			unsigned marginWidth;	/* => 2 */
		} button;
		struct {
			Texture focus;	/* default white, opposite black */
			Texture unfocus;	/* default black, opposite white */
		} handle, grip;
		struct {
			struct {
				XColor borderColor;	/* => white, black */
			} focus, unfocus;
			unsigned borderWidth;	/* => 1 */
		} frame;
		AdwmFont font;	/* no default? */
		const char *alignment;	/* => 'left' */
		unsigned handleHeight;	/* => 6 */
	} window;
	struct {
		/* anonymous Texture */
		struct {
			Appearance appearance;	/* default 'flat solid' */
			XColor color;	/* color1 => color => white */
			XColor colorTo;	/* color2 => colorTo => white */
			XColor picColor;	/* => color => white */
			Pixmap pixmap;
			XftColor textColor;	/* unused */
			XColor borderColor;	/* => black */
			unsigned borderWidth;	/* => 1 */
			XColor backgroundColor;	/* => color => white */
			XColor foregroundColor;	/* unused */
		};
		unsigned marginWidth;	/* => 2 */
	} slit;
	XColor borderColor;		/* => black */
	unsigned borderWidth;		/* => 1 */
	unsigned bevelWidth;		/* => 3 */
	unsigned frameWidth;		/* => bevelWidth => 3 */
	unsigned handleWidth;		/* => 6 */
	const char *rootCommand;
	/* calculated */
	unsigned buttonWidth;
	unsigned labelHeight;
	unsigned titleHeight;
	unsigned gripWidth;
	unsigned handleHeight;
} BlackboxStyle;

typedef enum {
	PlaceTopLeft,
	PlaceTopCenter,
	PlaceTopRight,
	PlaceCenterRight,
	PlaceBottomRight,
	PlaceBottomCenter,
	PlaceBottomLeft,
	PlaceCenterLeft,
} BlackboxPlace;

typedef enum {
	DirectionHorizontal,
	DirectionVertical,
} BlackboxDirection;

typedef enum {
	FocusSloppy,			/* default */
	FocusClickToFocus
} BlackboxFocus;

typedef enum {
	PlacementRowSmart,		/* default */
	PlacementColSmart,
	PlacementCascade,
	PlacementMinOverlap
} BlackboxPlacement;

typedef enum {
	PlaceLeftToRight,
	PlaceRightToLeft,
	PlaceTopToBottom = PlaceLeftToRight,
	PlaceBottomToTop = PlaceRightToLeft,
} BlackboxPlaceDir;

typedef struct {
	struct {
		BlackboxPlace placement;
		BlackboxDirection direction;
		Bool onTop;
		Bool autoHide;
	} slit;
	struct {
		Bool onTop;
		Bool autoHide;
		BlackboxPlace placement;
		unsigned widthPercent;	/* 1-100 */
	} toolbar;
	Bool enableToolbar;
	unsigned workspaces;
	const char *workspaceNames;
	const char *strftimeFormat;
	Bool dateFormat;		/* False - American, True European */
	Bool clockFormat;		/* False - 12hr, True 24hr */

	Bool fullMaximization;
	Bool disableBindingsWithScrollLock;
	unsigned edgeSnapThreshold;
	BlackboxPlaceDir rowPlacementDirection;
	BlackboxFocus focusModel;
	Bool autoRaise;
	Bool clickRaise;
	BlackboxPlaceDir colPlacementDirection;
	Bool focusNewWindows;
	Bool focusLastWindow;
	BlackboxPlacement windowPlacement;
} BlackboxScreen;

typedef struct {
	const char *menuFile;
	unsigned maximumColors;		/* FIXME */
	const char *styleFile;
	Bool changeWorkspaceWithMouseWheel;
	Bool shadeWindowWithMouseWheel;
	Bool toolbarActionsWithMouseWheel;
	Bool imageDither;
	Bool opaqueMove;
	Bool placementIgnoresShaded;
	unsigned autoRaiseDelay;	/* default 250 */
	unsigned doubleClickInterval;	/* default 250 */
	Bool opaqueResize;		/* FIXME */
	unsigned windowSnapThreshold;	/* FIXME */

	unsigned colorsPerChannel;	/* 2 - 6, default 4 */
	unsigned cacheLife;		/* default 5 */
	unsigned cacheMax;		/* 200 Kilobytes */
	BlackboxScreen *screens;
} BlackboxSession;

/* blackbox(1) always uses these bitmaps for buttons.  The minimize button is
 * always on the left, the close button on the right followed by the
 * maximize/restore button (which depends on max/maxv/maxh state) */

static const int iconify_width = 9;
static const int iconify_height = 9;

static const unsigned char iconify_bits[] = {
	0x00, 0x00, 0x82, 0x00, 0xc6, 0x00, 0x6c, 0x00, 0x38,
	0x00, 0x10, 0x00, 0x00, 0x00, 0xff, 0x01, 0xff, 0x01
};

static const int maximize_width = 9;
static const int maximize_height = 9;

static const unsigned char maximize_bits[] = {
	0xff, 0x01, 0xff, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xff, 0x01
};

static const int restore_width = 9;
static const int restore_height = 9;

static const unsigned char restore_bits[] = {
	0xf8, 0x01, 0xf8, 0x01, 0x08, 0x01, 0x3f, 0x01, 0x3f,
	0x01, 0xe1, 0x01, 0x21, 0x00, 0x21, 0x00, 0x3f, 0x00
};

static const int close_width = 9;
static const int close_height = 9;

static const unsigned char close_bits[] = {
	0x83, 0x01, 0xc7, 0x01, 0xee, 0x00, 0x7c, 0x00, 0x38,
	0x00, 0x7c, 0x00, 0xee, 0x00, 0xc7, 0x01, 0x83, 0x01
};

/*
 * window.title.focus.appearance => window.title.focus => 'flat solid', white
 * window.title.focus.color1 => window.title.focus.color => white
 * window.title.focus.color2 => window.title.focus.colorTo => white
 * window.title.focus.backgroundColor => window.title.focus.color => white
 * window.title.focus.borderColor => black
 * window.title.focus.borderWidth => 1
 * window.title.focus.textColor <= unused
 * window.title.focus.foregroundColor <= unused
 *
 * window.title.unfocus.appearance => window.title.unfocus => 'flat solid', black
 * window.title.unfocus.color1 => window.title.unfocus.color => black
 * window.title.unfocus.color2 => window.title.unfocus.colorTo => black
 * window.title.unfocus.backgroundColor => window.title.unfocus.color => black
 * window.title.unfocus.borderColor => black
 * window.title.unfocus.borderWidth => 1
 * window.title.unfocus.textColor <= unused
 * window.title.unfocus.foregroundColor <= unused
 *
 * window.title.marginWidth => 2
 *
 * window.label.focus.appearance => window.label.focus => 'flat solid', white
 * window.label.focus.color1 => window.label.focus.color => white
 * window.label.focus.color2 => window.label.focus.colorTo => white
 * window.label.focus.backgroundColor => window.label.focus.color => white
 * window.label.focus.borderColor => black
 * window.label.focus.borderWidth => 1
 * window.label.focus.textColor => black
 * window.label.focus.foregroundColor <= unused
 *
 * window.label.unfocus.appearance => window.label.unfocus => 'flat solid', black
 * window.label.unfocus.color1 => window.label.unfocus.color => black
 * window.label.unfocus.color2 => window.label.unfocus.colorTo => black
 * window.label.unfocus.backgroundColor => window.label.unfocus.color => black
 * window.label.unfocus.borderColor => black
 * window.label.unfocus.borderWidth => 1
 * window.label.unfocus.textColor => white
 * window.label.unfocus.foregroundColor <= unused
 *
 * window.label.marginWidth => 2
 *
 * window.button.focus.appearance => window.button.focus => 'flat solid', white
 * window.button.focus.color1 => window.button.focus.color => white
 * window.button.focus.color2 => window.button.focus.colorTo => white
 * window.button.focus.backgroundColor => window.button.focus.color => white
 * window.button.focus.borderColor => black
 * window.button.focus.borderWidth => 1
 * window.button.focus.textColor <= unused
 * window.button.focus.foregroundColor => window.button.focus.picColor => black
 *
 * window.button.unfocus.appearance => window.button.unfocus => 'flat solid', black
 * window.button.unfocus.color1 => window.button.unfocus.color => black
 * window.button.unfocus.color2 => window.button.unfocus.colorTo => black
 * window.button.unfocus.backgroundColor => window.button.unfocus.color => black
 * window.button.unfocus.borderColor => black
 * window.button.unfocus.borderWidth => 1
 * window.button.unfocus.textColor <= unused
 * window.button.unfocus.foregroundColor => window.button.unfocus.picColor => white
 *
 * window.button.pressed.appearance => window.button.pressed => 'flat solid', black
 * window.button.pressed.color1 => window.button.pressed.color => black
 * window.button.pressed.color2 => window.button.pressed.colorTo => black
 * window.button.pressed.backgroundColor => window.button.pressed.color => black
 * window.button.pressed.borderColor => black
 * window.button.pressed.borderWidth => 1
 * window.button.pressed.textColor <= unused
 * window.button.pressed.foregroundColor <= unused
 *
 * window.button.marginWidth => 2
 *
 * window.handle.focus.appearance => window.handle.focus => 'flat solid', white
 * window.handle.focus.color1 => window.handle.focus.color => white
 * window.handle.focus.color2 => window.handle.focus.colorTo => white
 * window.handle.focus.backgroundColor => window.handle.focus.color => white
 * window.handle.focus.borderColor => black
 * window.handle.focus.borderWidth => 1
 * window.handle.focus.textColor <= unused
 * window.handle.focus.foregroundColor <= unused
 *
 * window.handle.unfocus.appearance => window.handle.unfocus => 'flat solid', black
 * window.handle.unfocus.color1 => window.handle.unfocus.color => black
 * window.handle.unfocus.color2 => window.handle.unfocus.colorTo => black
 * window.handle.unfocus.backgroundColor => window.handle.unfocus.color => black
 * window.handle.unfocus.borderColor => black
 * window.handle.unfocus.borderWidth => 1
 * window.handle.unfocus.textColor <= unused
 * window.handle.unfocus.foregroundColor <= unused
 *
 * window.grip.focus.appearance => window.grip.focus => 'flat solid', white
 * window.grip.focus.color1 => window.grip.focus.color => white
 * window.grip.focus.color2 => window.grip.focus.colorTo => white
 * window.grip.focus.backgroundColor => window.grip.focus.color => white
 * window.grip.focus.borderColor => black
 * window.grip.focus.borderWidth => 1
 * window.grip.focus.textColor <= unused
 * window.grip.focus.foregroundColor <= unused
 *
 * window.grip.unfocus.appearance => window.grip.unfocus => 'flat solid', black
 * window.grip.unfocus.color1 => window.grip.unfocus.color => black
 * window.grip.unfocus.color2 => window.grip.unfocus.colorTo => black
 * window.grip.unfocus.backgroundColor => window.grip.unfocus.color => black
 * window.grip.unfocus.borderColor => black
 * window.grip.unfocus.borderWidth => 1
 * window.grip.unfocus.textColor <= unused
 * window.grip.unfocus.foregroundColor <= unused
 *
 * window.frame.focus.borderColor => white
 * window.frame.unfocus.borderColor => black
 * window.frame.borderWidth => 1
 *
 * window.font
 * window.alignment => left
 * window.handleHeight => 6
 *
 * slit.appearance => slit => 'flat solid' white
 * slit.color1 => slit.color => white
 * slit.color2 => slit.colorTo => white
 * slit.backgroundColor => slit.color => white
 * slit.borderColor => black
 * slit.borderWidth => 1
 * slit.textColor <= unused
 * slit.foregroundColor <= unused
 * slit.marginWidth => 2
 *
 * Old Style:
 *
 * borderColor => black
 * borderWidth => 1
 * bevelWidth => 3
 * frameWidth => bevelWidth => 3
 * handleWidth => 6
 * rootCommand (from config file) => rootCommand (from style file)
 *
 * Texture descriptions can be:
 *
 * parentrelative, or:
 *
 * solid
 * gradient crossdiagonal
 * gradient rectangle
 * gradient pyramid
 * gradient pipecross
 * gradient elliptic
 * gradient horizontal
 * gradient splitvertical
 * gradient vertical
 *
 * + sunken, flat or raised (default)
 *
 * +? interlaced
 *
 * +? border
 *
 * For gradient there are two colors, for others there is one
 *
 *
 */

typedef struct {
	char *rcfile;			/* rcfile */
	char *udir;			/* user directory */
	char *pdir;			/* private directory */
	char *sdir;			/* system directory */
} BlackboxConfig;

static BlackboxStyle *styles = NULL;
static BlackboxConfig config;

static void
initkeys_BLACKBOX(void)
{
}

BlackboxSession session;

static XrmDatabase xconfigdb;
static XrmDatabase xstyledb;

static void
initconfig_BLACKBOX(void)
{
	const char *res;
	BlackboxScreen *screen;
	char name[256], clas[256], *n, *c;
	size_t nlen, clen;

	/* NOTE: called once for each managed screen */
	xresdb = xconfigdb;

	if (!session.screens) {
		getbool("session.imageDither", "Session.ImageDither", NULL, False,
			&session.imageDither);
		getbool("session.opaqueMove", "Session.OpaqueMove", NULL, False,
			&session.opaqueMove);
		session.menuFile = readres("session.menuFile", "Session.MenuFile", NULL);
		session.styleFile =
		    readres("session.styleFile", "Session.StyleFile", NULL);
		res = readres("session.colorsPerChannel", "Session.ColorsPerChannel",
			      "4");
		session.colorsPerChannel = strtoul(res, NULL, 0);
		res = readres("session.doubleClickInterval",
			      "Session.DoubleClickInterval", "250");
		session.doubleClickInterval = strtoul(res, NULL, 0);
		res = readres("session.autoRaiseDelay", "Session.AutoRaiseDelay", "250");
		session.autoRaiseDelay = strtoul(res, NULL, 0);
		res = readres("session.cacheLife", "Session.CacheLife", "5");
		session.cacheLife = strtoul(res, NULL, 0);
		res = readres("session.cacheMax", "Session.CacheMax", "200");
		session.cacheMax = strtoul(res, NULL, 0);
		getbool("session.changeWorkspaceWithMouseWheel",
			"Session.ChangeWorkspaceWithMouseWheel", NULL, False,
			&session.changeWorkspaceWithMouseWheel);
		getbool("session.shadeWindowWithMouseWheel",
			"Session.ShadeWindowWithMouseWheel", NULL, False,
			&session.shadeWindowWithMouseWheel);
		getbool("session.toolbarActionsWithMouseWheel",
			"Session.ToolbarActionsWithMouseWheel", NULL, False,
			&session.toolbarActionsWithMouseWheel);
		getbool("session.placementIgnoresShaded",
			"Session.PlacementIgnoresShaded", NULL, False,
			&session.placementIgnoresShaded);
	}

	session.screens =
	    erealloc(session.screens, (scr->screen + 1) * sizeof(*session.screens));
	screen = session.screens + scr->screen;
	memset(screen, 0, sizeof(*screen));
	snprintf(name, sizeof(name), "session.screen%d.", scr->screen);
	n = name + strlen(name);
	nlen = sizeof(name) - strlen(name);
	snprintf(clas, sizeof(clas), "Session.Screen%d.", scr->screen);
	c = clas + strlen(clas);
	clen = sizeof(clas) - strlen(clas);

	snprintf(n, nlen, "slit.placement");
	snprintf(c, clen, "Slit.Placement");
	res = readres(n, c, "CenterLeft");
	screen->slit.placement = PlaceCenterLeft;
	if (strcasestr(res, "topleft"))
		screen->slit.placement = PlaceTopLeft;
	else if (strcasestr(res, "topcenter"))
		screen->slit.placement = PlaceTopCenter;
	else if (strcasestr(res, "topright"))
		screen->slit.placement = PlaceTopRight;
	else if (strcasestr(res, "centerright"))
		screen->slit.placement = PlaceCenterRight;
	else if (strcasestr(res, "bottomright"))
		screen->slit.placement = PlaceBottomRight;
	else if (strcasestr(res, "bottomcenter"))
		screen->slit.placement = PlaceBottomCenter;
	else if (strcasestr(res, "bottomleft"))
		screen->slit.placement = PlaceBottomLeft;
	else if (strcasestr(res, "centerleft"))
		screen->slit.placement = PlaceCenterLeft;

	snprintf(n, nlen, "slit.direction");
	snprintf(c, clen, "Slit.Direction");
	res = readres(n, c, "Vertical");
	screen->slit.direction = DirectionVertical;
	if (strcasestr(res, "vertical"))
		screen->slit.direction = DirectionVertical;
	else if (strcasestr(res, "horizontal"))
		screen->slit.direction = DirectionHorizontal;

	snprintf(n, nlen, "slit.onTop");
	snprintf(c, clen, "Slit.OnTop");
	getbool(n, c, NULL, False, &screen->slit.onTop);

	snprintf(n, nlen, "slit.autoHide");
	snprintf(c, clen, "Slit.AutoHide");
	getbool(n, c, NULL, False, &screen->slit.autoHide);

	snprintf(n, nlen, "toolbar.placement");
	snprintf(c, clen, "Toolbar.Placement");
	res = readres(n, c, "BottomCenter");
	screen->toolbar.placement = PlaceBottomCenter;
	if (strcasestr(res, "topleft"))
		screen->toolbar.placement = PlaceTopLeft;
	else if (strcasestr(res, "topcenter"))
		screen->toolbar.placement = PlaceTopCenter;
	else if (strcasestr(res, "topright"))
		screen->toolbar.placement = PlaceTopRight;
	else if (strcasestr(res, "bottomright"))
		screen->toolbar.placement = PlaceBottomRight;
	else if (strcasestr(res, "bottomcenter"))
		screen->toolbar.placement = PlaceBottomCenter;
	else if (strcasestr(res, "bottomleft"))
		screen->toolbar.placement = PlaceBottomLeft;

	snprintf(n, nlen, "toolbar.onTop");
	snprintf(c, clen, "Toolbar.OnTop");
	getbool(n, c, NULL, False, &screen->toolbar.onTop);

	snprintf(n, nlen, "toolbar.autoHide");
	snprintf(c, clen, "Toolbar.AutoHide");
	getbool(n, c, NULL, False, &screen->toolbar.autoHide);

	snprintf(n, nlen, "toolbar.widthPercent");
	snprintf(c, clen, "Toolbar.WidthPercent");
	res = readres(n, c, "66");
	screen->toolbar.widthPercent = strtoul(res, NULL, 0);
	if (screen->toolbar.widthPercent < 1)
		screen->toolbar.widthPercent = 1;
	if (screen->toolbar.widthPercent > 100)
		screen->toolbar.widthPercent = 100;

	snprintf(n, nlen, "enableToolbar");
	snprintf(c, clen, "EnableToolbar");
	getbool(n, c, NULL, False, &screen->enableToolbar);

	snprintf(n, nlen, "focusModel");
	snprintf(c, clen, "FocusModel");
	res = readres(n, c, "SloppyFocus");
	screen->focusModel = FocusSloppy;
	screen->autoRaise = False;
	screen->clickRaise = False;
	if (strcasestr(res, "sloppyfocus")) {
		screen->focusModel = FocusSloppy;
		screen->autoRaise = strcasestr(res, "autoraise") ? True : False;
		screen->clickRaise = strcasestr(res, "clickraise") ? True : False;
	} else if (strcasestr(res, "clicktofocus"))
		screen->focusModel = FocusClickToFocus;

	snprintf(n, nlen, "windowPlacement");
	snprintf(c, clen, "WindowPlacement");
	res = readres(n, c, "RowSmartPlacement");
	screen->windowPlacement = PlacementRowSmart;
	if (strcasestr(res, "rowsmartplacement"))
		screen->windowPlacement = PlacementRowSmart;
	else if (strcasestr(res, "colsmartplacement"))
		screen->windowPlacement = PlacementColSmart;
	else if (strcasestr(res, "cascadeplacement"))
		screen->windowPlacement = PlacementCascade;
	else if (strcasestr(res, "minoverlap"))
		screen->windowPlacement = PlacementMinOverlap;

	snprintf(n, nlen, "rowPlacementDirection");
	snprintf(c, clen, "RowPlacementDirection");
	res = readres(n, c, "LeftToRight");
	screen->rowPlacementDirection = PlaceLeftToRight;
	if (strcasestr(res, "lefttoright"))
		screen->rowPlacementDirection = PlaceLeftToRight;
	else if (strcasestr(res, "righttoleft"))
		screen->rowPlacementDirection = PlaceRightToLeft;

	snprintf(n, nlen, "colPlacementDirection");
	snprintf(c, clen, "ColPlacementDirection");
	res = readres(n, c, "TopToBottom");
	screen->colPlacementDirection = PlaceTopToBottom;
	if (strcasestr(res, "toptobottom"))
		screen->colPlacementDirection = PlaceTopToBottom;
	else if (strcasestr(res, "bottomtotop"))
		screen->colPlacementDirection = PlaceBottomToTop;

	snprintf(n, nlen, "fullMaximization");
	snprintf(c, clen, "FullMaximization");
	getbool(n, c, NULL, False, &screen->fullMaximization);

	snprintf(n, nlen, "focusNewWindows");
	snprintf(c, clen, "FocusNewWindows");
	getbool(n, c, NULL, False, &screen->focusNewWindows);

	snprintf(n, nlen, "focusLastWindow");
	snprintf(c, clen, "FocusLastWindow");
	getbool(n, c, NULL, False, &screen->focusLastWindow);

	snprintf(n, nlen, "disableBindingsWithScrollLock");
	snprintf(c, clen, "DisableBindingsWithScrollLock");
	getbool(n, c, NULL, False, &screen->disableBindingsWithScrollLock);

	snprintf(n, nlen, "workspaces");
	snprintf(c, clen, "Workspaces");
	res = readres(n, c, "1");
	screen->workspaces = strtoul(res, NULL, 0);
	if (screen->workspaces < 1)
		screen->workspaces = 1;
	if (screen->workspaces > sizeof(unsigned long long) * 8)
		screen->workspaces = sizeof(unsigned long long) * 8;

	snprintf(n, nlen, "workspaceNames");
	snprintf(c, clen, "WorkspaceNames");
	screen->workspaceNames = readres(n, c, NULL);

	snprintf(n, nlen, "strftimeFormat");
	snprintf(c, clen, "StrftimeFormat");
	screen->strftimeFormat = readres(n, c, "%I:%M %p");

	snprintf(n, nlen, "dateFormat");
	snprintf(c, clen, "DateFormat");
	getbool(n, c, "european", False, &screen->dateFormat);

	snprintf(n, nlen, "clockFormat");
	snprintf(c, clen, "ClockFormat");
	getbool(n, c, "24", False, &screen->clockFormat);

	snprintf(n, nlen, "edgeSnapThreshold");
	snprintf(c, clen, "EdgeSnapThreshold");
	res = readres(n, c, "0");
	screen->edgeSnapThreshold = strtoul(res, NULL, 0);
}

static void
initrcfile_BLACKBOX()
{
	const char *home = getenv("HOME") ? : ".";
	const char *file = NULL;
	char *pos;
	int i, len;
	struct stat st;

	for (i = 0; i < cargc - 1; i++)
		if (!strcmp(cargv[i], "-rc"))
			file = cargv[i + 1];
	free(config.rcfile);
	if (file) {
		if (*file == '/')
			config.rcfile = strdup(file);
		else {
			len = strlen(home) + strlen(file) + 2;
			config.rcfile = ecalloc(len, sizeof(*config.rcfile));
			strcpy(config.rcfile, home);
			strcat(config.rcfile, "/");
			strcat(config.rcfile, file);
		}
	} else {
		len = strlen(home) + strlen("/.blackboxrc") + 1;
		config.rcfile = ecalloc(len, sizeof(*config.rcfile));
		strcpy(config.rcfile, home);
		strcat(config.rcfile, "/.blackboxrc");
		if (!lstat(config.rcfile, &st) && S_ISLNK(st.st_mode)) {
			char *buf = ecalloc(PATH_MAX + 1, sizeof(*buf));

			if (readlink(config.rcfile, buf, PATH_MAX) == -1)
				eprint("%s: %s\n", config.rcfile, strerror(errno));
			if (*buf == '/') {
				free(config.rcfile);
				config.rcfile = strdup(buf);
			} else if (*buf) {
				free(config.rcfile);
				len = strlen(home) + strlen(buf) + 2;
				config.rcfile = ecalloc(len, sizeof(*config.rcfile));
				strcpy(config.rcfile, home);
				strcat(config.rcfile, "/");
				strcat(config.rcfile, buf);
			}
			free(buf);
		}
	}
	free(config.pdir);
	config.pdir = strdup(config.rcfile);
	if ((pos = strrchr(config.pdir, '/')))
		*pos = '\0';
	free(config.udir);
	config.udir =
	    ecalloc(strlen(home) + strlen("/.blackbox") + 1, sizeof(*config.udir));
	strcpy(config.udir, home);
	strcat(config.udir, "/.blackbox");
	free(config.sdir);
	config.sdir = strdup("/usr/share/blackbox");
	if (!strncmp(home, config.pdir, strlen(home))) {
		free(config.pdir);
		config.pdir = strdup(config.udir);
	}
}

static void
initstyle_BLACKBOX(void)
{
	const char *res;
	BlackboxStyle *style;

	/* NOTE: called once for each managed screen */
	xresdb = xstyledb;
	if (!styles)
		styles = ecalloc(nscr, sizeof(*styles));
	style = styles + scr->screen;

	getbitmap(iconify_bits, iconify_width, iconify_height, &style->window.iconify);
	getbitmap(maximize_bits, maximize_width, maximize_height,
		  &style->window.maximize);
	getbitmap(restore_bits, restore_width, restore_height, &style->window.restore);
	getbitmap(close_bits, close_width, close_height, &style->window.close);

	readtexture("window.title.focus", "Window.Title.Focus",
		    &style->window.title.focus, "white", "black");
	readtexture("window.title.unfocus", "Window.Title.Unfocus",
		    &style->window.title.unfocus, "black", "white");
	res = readres("window.title.marginWidth", "Window.Title.MarginWidth", "2");
	style->window.title.marginWidth = strtoul(res, NULL, 0);

	readtexture("window.label.focus", "Window.Label.Focus",
		    &style->window.label.focus, "white", "black");
	readtexture("window.label.unfocus", "Window.Label.Unfocus",
		    &style->window.label.unfocus, "black", "white");
	res = readres("window.label.marginWidth", "Window.Label.MarginWidth", "2");
	style->window.label.marginWidth = strtoul(res, NULL, 0);

	readtexture("window.button.focus", "Window.Button.Focus",
		    &style->window.button.focus, "white", "black");
	readtexture("window.button.unfocus", "Window.Button.Unfocus",
		    &style->window.button.unfocus, "black", "white");
	readtexture("window.button.pressed", "Window.Button.Pressed",
		    &style->window.button.pressed, "black", "white");
	res = readres("window.button.marginWidth", "Window.Button.MarginWidth", "2");
	style->window.button.marginWidth = strtoul(res, NULL, 0);

	readtexture("window.handle.focus", "Window.Handle.Focus",
		    &style->window.handle.focus, "white", "black");
	readtexture("window.handle.unfocus", "Window.Handle.Unfocus",
		    &style->window.handle.unfocus, "black", "white");

	readtexture("window.grip.focus", "Window.Grip.Focus", &style->window.grip.focus,
		    "white", "black");
	readtexture("window.grip.unfocus", "Window.Grip.Unfocus",
		    &style->window.grip.unfocus, "black", "white");

	res = readres("window.frame.focus.borderColor",
		      "Window.Frame.Focus.BorderColor", "white");
	getxcolor(res, "white", &style->window.frame.focus.borderColor);
	res = readres("window.frame.unfocus.borderColor",
		      "Window.Frame.Unfocus.BorderColor", "black");
	getxcolor(res, "black", &style->window.frame.unfocus.borderColor);
	res = readres("borderWidth", "BorderWidth", "1");
	res = readres("window.frame.borderWidth", "Window.Frame.BorderWidth", res);
	style->borderWidth = style->window.frame.borderWidth = strtoul(res, NULL, 0);

	res = readres("window.font", "Window.Font", "sans 8");
	getfont(res, "sans 8", &style->window.font);
	style->window.alignment = readres("window.alignment", "Window.Alignment", "left");
	res = readres("handleWidth", "HandleWidth", "6");
	res = readres("window.handleHeight", "Window.HandleHeight", res);
	style->handleWidth = style->window.handleHeight = strtoul(res, NULL, 0);

	readtexture("slit", "Slit", (Texture *) &style->slit, "white", "black");
	res = readres("slit.marginWidth", "Slit.MarginWidth", "2");
	style->slit.marginWidth = strtoul(res, NULL, 0);

	res = readres("borderColor", "BorderColor", "black");
	getxcolor(res, "black", &style->borderColor);
	res = readres("bevelWidth", "BevelWidth", "3");
	style->bevelWidth = strtoul(res, NULL, 0);
	res = readres("frameWidth", "FrameWidth", res);
	style->frameWidth = strtoul(res, NULL, 0);
	style->rootCommand = readres("rootCommand", "RootCommand", NULL);

	/* calculated values */
	style->buttonWidth =
	    max(max(max(style->window.iconify.width, style->window.iconify.height),
		    max(style->window.maximize.width, style->window.maximize.height)),
		max(max(style->window.restore.width, style->window.restore.height),
		    max(style->window.close.width, style->window.close.height))) +
	    ((max(style->window.button.focus.borderWidth,
		  style->window.button.unfocus.borderWidth) +
	      style->window.button.marginWidth) * 2);
	style->labelHeight =
	    max(style->window.font.height +
		((max(style->window.label.focus.borderWidth,
		      style->window.label.unfocus.borderWidth) +
		  style->window.label.marginWidth) * 2), style->buttonWidth);
	style->buttonWidth = max(style->buttonWidth, style->labelHeight);
	style->titleHeight =
	    style->labelHeight +
	    ((max(style->window.title.focus.borderWidth,
		  style->window.title.unfocus.borderWidth) +
	      style->window.title.marginWidth) * 2);
	style->gripWidth = (style->buttonWidth * 2);
	style->handleHeight =
	    style->window.handleHeight +
	    (max(style->window.handle.focus.borderWidth,
		 style->window.handle.unfocus.borderWidth) * 2);

	/* 
	 * ADWM style options that must be set:
	 * scr->style.border
	 * scr->style.titleheight
	 * scr->style.color.norm[ColBorder]
	 * scr->style.color.sel[ColBorder]
	 * scr->style.margin
	 * scr->style.gripsheight
	 * scr->style.gripswidth <= need this
	 */
	scr->style.border = style->borderWidth;
	scr->style.titleheight = style->titleHeight;
	scr->style.color.norm[ColBorder] = style->window.frame.unfocus.borderColor.pixel;
	scr->style.color.sel[ColBorder] = style->window.frame.focus.borderColor.pixel;
	scr->style.margin = 0;
	scr->style.gripsheight = style->handleHeight;
	scr->style.gripswidth = style->gripWidth;

}

static void
deinitstyle_BLACKBOX(void)
{
	BlackboxStyle *style;

	if (!styles)
		return;
	style = styles + scr->screen;

	freetexture(&style->window.title.focus);
	freetexture(&style->window.title.unfocus);
	freetexture(&style->window.label.focus);
	freetexture(&style->window.label.unfocus);
	freetexture(&style->window.button.focus);
	freetexture(&style->window.button.unfocus);
	freetexture(&style->window.button.pressed);
	freetexture(&style->window.handle.focus);
	freetexture(&style->window.handle.unfocus);
	freetexture(&style->window.grip.focus);
	freetexture(&style->window.grip.unfocus);
	freetexture((Texture *) &style->slit);
	freefont(&style->window.font);
}

static void
drawclient_BLACKBOX(Client *c)
{
}

AdwmOperations adwm_ops = {
	.name = "blackbox",
	.clas = "Blackbox",
	.initrcfile = &initrcfile_BLACKBOX,
	.initconfig = &initconfig_BLACKBOX,
	.initkeys = &initkeys_BLACKBOX,
	.initstyle = &initstyle_BLACKBOX,
	.deinitstyle = &deinitstyle_BLACKBOX,
	.drawclient = &drawclient_BLACKBOX,
};

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
