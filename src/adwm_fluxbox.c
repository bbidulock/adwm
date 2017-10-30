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
#include "resource.h"
#include "actions.h"
#include "parse.h"

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

typedef enum {
	PlaceBottomLeft,
	PlaceBottomCenter,
	PlaceBottomRight,
	PlaceLeftBottom,
	PlaceLeftCenter,
	PlaceLeftTop,
	PlaceRightBottom,
	PlaceRightCenter,
	PlaceRightTop,
	PlaceTopLeft,
	PlaceTopCenter,
	PlaceTopRight,
} FluxboxPlace;

typedef enum {
	AlignLeft,
	AlignRelative,
	AlignRight,
} FluxboxAlign;

typedef enum {
	PlacementRowSmart,
	PlacementColSmart,
	PlacementCascade,
	PlacementUnderMouse,
} FluxboxPlacement;

typedef enum {
	PlaceLeftToRight,
	PlaceRightToLeft,
	PlaceTopToBottom = PlaceLeftToRight,
	PlaceBottomToTop = PlaceRightToLeft,
} FluxboxPlaceDir;

typedef enum {
	AttachAreaWindow,
	AttachAreaTitlebar,
} FluxboxAttachArea;

typedef enum {
	DirectionHorizontal,
	DirectionVertical,
} FluxboxDirection;

typedef enum {
	FocusModelClickToFocus,
	FocusModelMouseFocus,
	FocusModelStrictMouseFocus,
} FluxboxFocusModel;

typedef enum {
	LayerAboveDock,
	LayerDock,
	LayerTop,
	LayerNormal,
	LayerBottom,
	LayerDesktop,
} FluxboxLayer;

typedef enum {
	DecoNormal,
	DecoNone,
	DecoBorder,
	DecoTab,
	DecoTiny,
	DecoTool,
} FluxboxDeco;

typedef struct {
	struct {
		struct {
			unsigned alpha;	/* 255 */
		} focus, unfocus;
	} window;
	struct {
		unsigned alpha;		/* 255 *//* undocumented */
	} menu;
	struct {
		Bool autoHide;		/* False */
		FluxboxLayer layer;	/* Dock */
		FluxboxPlace placement;	/* RightBottom */
		Bool maxOver;		/* False */
		unsigned onhead;	/* 0 */
		Bool acceptKdeDockapps;	/* undocumented */
		unsigned alpha;		/* 255 *//* undocumented */
		FluxboxDirection direction;	/* undocumented */
		Bool onTop;		/* undocumented */
	} slit;
	struct {
		unsigned alpha;		/* 255 */
		Bool autoHide;		/* False */
		FluxboxLayer layer;	/* Dock */
		FluxboxPlace placement;	/* BottomCenter */
		Bool maxOver;		/* False */
		unsigned height;	/* 0 */
		Bool visible;		/* True */
		unsigned widthPercent;	/* 100 */
		const char *tools;	/* workspacename, prevworkspace, nextworkspace,
					   iconbar, prevwindow, nextwindow, systemtray,
					   clock */
		unsigned onhead;	/* 1 */
		Bool onTop;
	} toolbar;
	struct {
		Bool maxOver;		/* False */
		Bool intitlebar;	/* True */
		Bool usePixmap;
	} tabs;
	struct {
		const char *pattern;
		const char *mode;	/* {static groups} (workspace) */
		Bool usePixmap;		/* True */
		unsigned iconTextPadding;	/* 10 */
		FluxboxAlign alignment;	/* Relative */
		unsigned iconWidth;	/* 128 */
		const char *wheelMode;	/* Screen *//* undocumented */
	} iconbar;
	struct {
		const char *capStyle;	/* undocumented */
		const char *joinStyle;	/* undocumented */
		const char *lineStyle;	/* undocumented */
		const char *lineWidth;	/* undocumented */
	} overlay;
	struct {
		FluxboxPlace placement;	/* TopLeft */
		unsigned width;		/* 64 */
		unsigned height;
	} tab;
	struct {
		const char *left;	/* mis-documented as session resource */
		const char *right;	/* mis-documented as session resource */
	} titlebar;
	const char *strftimeFormat;	/* %k:%M */
	Bool autoRaise;			/* True */
	Bool clickRaises;		/* True */
	Bool workspacewarping;		/* True */
	Bool showwindowposition;	/* False */
	FluxboxDeco defaultDeco;	/* NORMAL */
	unsigned menuDelay;		/* 200 */
	unsigned menuDelayClose;	/* undocumented */
	const char *menuMode;		/* undocumented */
	Bool focusNewWindows;		/* True */
	const char *workspaceNames;	/* Workspace 1, Workspace 2, Workspace 3,
					   Workspace 4 */
	unsigned edgeSnapThreshold;	/* 10 */
	FluxboxPlacement windowPlacement;	/* RowSmartPlacement */
	FluxboxPlaceDir rowPlacementDirection;	/* LeftToRight */
	FluxboxPlaceDir colPlacementDirection;	/* TopToBottom */
	Bool fullMaximization;		/* False */
	Bool opaqueMove;		/* True */
	unsigned workspaces;		/* 4 */
	const char *windowMenu;		/* ~/.fluxbox/windowmenu */
	/* not documented */
	Bool allowRemoteActions;	/* False */
	struct {
		Bool usePixmap;		/* True *//* undocumented */
	} clientMenu;
	Bool decorateTransient;		/* True */
	unsigned demandsAttentionTimeout;	/* 500 */
	Bool desktopwheeling;		/* True */
	Bool focusLastWindow;		/* True */
	FluxboxFocusModel focusModel;	/* ClickToFocus */
	Bool focusSameHead;		/* False */
	const char *followModel;
	Bool imageDither;		/* True *//* undocumented */
	Bool maxDisableMove;		/* False *//* undocumented */
	Bool maxDisableResize;		/* False *//* undocumented */
	Bool maxIgnoreIncrement;	/* False *//* undocumented */
	unsigned noFocusWhileTypingDelay;	/* 0 *//* undocumented */
	const char *resizeMode;		/* Bottom *//* undocumented */
	Bool reversewheeling;		/* False *//* undocumented */
	const char *rootCommand;	/* "" *//* undocumented */
	const char *tabFocusModel;	/* SloppyTabFocus *//* undocumented */
	unsigned tooltipDelay;		/* 500 *//* undocumented */
	const char *userFollowModel;	/* Follow *//* undocumented */
	const char *windowScrollAction;	/* "" *//* undocumented */
	Bool windowScrollReverse;	/* False *//* undocumented */
} FluxboxScreen;

/*

session.screen0.allowRemoteActions:	true
session.screen0.autoRaise:	false
session.screen0.clickRaises:	true
session.screen0.clientMenu.usePixmap:	true
session.screen0.colPlacementDirection:	TopToBottom
session.screen0.decorateTransient:	true
session.screen0.defaultDeco:	NORMAL
session.screen0.demandsAttentionTimeout:	500
session.screen0.desktopwheeling:	true
session.screen0.edgeSnapThreshold:	0
session.screen0.focusLastWindow:	True
session.screen0.focusModel:	StrictMouseFocus
session.screen0.focusNewWindows:	true
session.screen0.focusSameHead:	false
session.screen0.followModel:	Ignore
session.screen0.fullMaximization:	false
session.screen0.iconbar.alignment:	Relative
session.screen0.iconbar.iconTextPadding:	10
session.screen0.iconbar.iconWidth:	70
session.screen0.iconbar.mode:	{static groups}
session.screen0.iconbar.usePixmap:	true
session.screen0.iconbar.wheelMode:	Screen
session.screen0.imageDither:	true
session.screen0.maxDisableMove:	false
session.screen0.maxDisableResize:	false
session.screen0.maxIgnoreIncrement:	false
session.screen0.menu.alpha:	224
session.screen0.menuDelay:	0
session.screen0.menuDelayClose:	0
session.screen0.menuMode:	Delay
session.screen0.noFocusWhileTypingDelay:	0
session.screen0.opaqueMove:	true
session.screen0.overlay.capStyle:	CapNotLast
session.screen0.overlay.joinStyle:	JoinMiter
session.screen0.overlay.lineStyle:	LineSolid
session.screen0.overlay.lineWidth:	1
session.screen0.resizeMode:	Bottom
session.screen0.reversewheeling:	false
session.screen0.rootCommand:	xprop -root -format _BLACKBOX_PID 32c -set _BLACKBOX_PID $$
session.screen0.rowPlacementDirection:	LeftToRight
session.screen0.showwindowposition:	true
session.screen0.slit.acceptKdeDockapps:	true
session.screen0.slit.alpha:	255
session.screen0.slit.autoHide:	true
session.screen0.slit.direction:	Vertical
session.screen0.slit.layer:	AboveDock
session.screen0.slit.maxOver:	false
session.screen0.slit.onhead:	0
session.screen0.slit.onTop:	False
session.screen0.slit.placement:	RightCenter
session.screen0.strftimeFormat:	%Y-%m-%d %H:%M
session.screen0.tabFocusModel:	SloppyTabFocus
session.screen0.tab.height:	16
session.screen0.tab.placement:	TopLeft
session.screen0.tabs.intitlebar:	true
session.screen0.tabs.maxOver:	true
session.screen0.tabs.usePixmap:	true
session.screen0.tab.width:	120
session.screen0.titlebar.left:	MenuIcon Stick Shade LHalf
session.screen0.titlebar.right:	RHalf Minimize Maximize Close
session.screen0.toolbar.alpha:	255
session.screen0.toolbar.autoHide:	false
session.screen0.toolbar.height:	0
session.screen0.toolbar.layer:	Dock
session.screen0.toolbar.maxOver:	false
session.screen0.toolbar.onhead:	0
session.screen0.toolbar.onTop:	False
session.screen0.toolbar.placement:	BottomCenter
session.screen0.toolbar.tools:	systemtray, workspacename, prevworkspace, nextworkspace, iconbar, prevwindow, nextwindow, clock
session.screen0.toolbar.visible:	true
session.screen0.toolbar.widthPercent:	100
session.screen0.tooltipDelay:	500
session.screen0.userFollowModel:	Follow
session.screen0.window.focus.alpha:	255
session.screen0.windowMenu:
session.screen0.windowPlacement:	RowSmartPlacement
session.screen0.windowScrollAction:
session.screen0.windowScrollReverse:	false
session.screen0.window.unfocus.alpha:	237
session.screen0.workspaceNames:	1,2,3,4,5,6,7,8,
session.screen0.workspaces:	8
session.screen0.workspacewarping:	true

 */

typedef struct {
	unsigned autoRaiseDelay;	/* 250 */
	unsigned cacheLife;		/* 5 */
	unsigned cacheMax;		/* 200 */
	unsigned colorsPerChannel;	/* 4 */
	unsigned doubleClickInterval;	/* 250 */
	Bool forcePseudoTransparency;	/* False */
	Bool ignoreBorder;		/* False */
	unsigned tabPadding;		/* 0 */
	FluxboxAttachArea tabsAttachArea;	/* Window */
	const char *appsFile;		/* ~/.fluxbox/apps */
	const char *groupFile;		/* ~/.fluxbox/groups */
	const char *keyFile;		/* ~/.fluxbox/keys */
	const char *menuFile;		/* ~/.fluxbox/menu */
	const char *slitlistFile;	/* ~/.fluxbox/slitlist */
	const char *styleFile;		/* */
	const char *styleOverlay;	/* ~/.fluxbox/overlay */

	unsigned modKey;		/* Mod1 *//* undocumented */
	Bool imageDither;		/* ?? *//* undocumented */
	const char *configVersion;	/* 13 *//* undocumented */
	Bool opaqueMove;		/* True *//* undocumented *//* screen resource? */
	FluxboxScreen *screens;
} FluxboxSession;

/*

session.appsFile:	~/.fluxbox/apps
session.autoRaiseDelay:	250
session.cacheLife:	5
session.cacheMax:	200
session.colorsPerChannel:	4
session.configVersion:	13
session.doubleClickInterval:	250
session.forcePseudoTransparency:	false
session.groupFile:	~/.fluxbox/groups
session.ignoreBorder:	false
session.imageDither:	True
session.keyFile:	~/.fluxbox/keys
session.menuFile:	~/.fluxbox/menu
session.modKey:	Mod1
session.opaqueMove:	False
session.slitlistFile:	~/.fluxbox/slitlist
session.styleFile:	/usr/share/fluxbox/styles/Penguins
session.styleOverlay:	~/.fluxbox/overlay
session.tabPadding:	0
session.tabsAttachArea:	Window

 */

typedef struct {
	char *rcfile;			/* rcfile */
	char *udir;			/* user directory */
	char *pdir;			/* private directory */
	char *sdir;			/* system directory */
} FluxboxConfig;

static FluxboxStyle *styles = NULL;
static FluxboxConfig config;

static void
initrcfile_FLUXBOX(const char *conf, Bool reload)
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
		len = strlen(home) + strlen("/.fluxbox/init") + 1;
		config.rcfile = ecalloc(len, sizeof(*config.rcfile));
		strcpy(config.rcfile, home);
		strcat(config.rcfile, "/.fluxbox/init");
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
	    ecalloc(strlen(home) + strlen("/.fluxbox") + 1, sizeof(*config.udir));
	strcpy(config.udir, home);
	strcat(config.udir, "/.fluxbox");
	free(config.sdir);
	config.sdir = strdup("/usr/share/fluxbox");
	if (!strncmp(home, config.pdir, strlen(home))) {
		free(config.pdir);
		config.pdir = strdup(config.udir);
	}

	styles = ecalloc(nscr, sizeof(*styles));
}

static XrmDatabase xconfigdb;
static XrmDatabase xstyledb;

static FluxboxSession session;

static void
initconfig_FLUXBOX(Bool reload)
{
	const char *res;
	FluxboxScreen *screen;
	char name[256], clas[256], *n, *c;
	size_t nlen, clen;

	/* Note: called once for each managed screen */
	xresdb = xconfigdb;

	if (!session.screens) {
		res = readres("session.autoRaiseDelay", "Session.AutoRaiseDelay", "250");
		session.autoRaiseDelay = strtoul(res, NULL, 0);
		res = readres("session.cacheLife", "Session.CacheLife", "5");
		session.cacheLife = strtoul(res, NULL, 0);
		res = readres("session.cacheMax", "Session.CacheMax", "200");
		session.cacheMax = strtoul(res, NULL, 0);
		res =
		    readres("session.colorsPerChannel", "Session.ColorsPerChannel", "4");
		session.colorsPerChannel = strtoul(res, NULL, 0);
		res =
		    readres("session.doubleClickInterval", "Session.DoubleClickInterval",
			    "250");
		session.doubleClickInterval = strtoul(res, NULL, 0);
		getbool("session.forcePseudoTransparency",
			"Session.ForcePseudoTransparency", NULL, False,
			&session.forcePseudoTransparency);
		getbool("session.ignoreBorder", "Session.IgnoreBorder", NULL,
			False, &session.ignoreBorder);
		res = readres("session.tabPadding", "Session.TabPadding", "0");
		session.tabPadding = strtoul(res, NULL, 0);
		res =
		    readres("session.tabsAttachArea", "Session.TabsAttachArea", "Window");
		session.tabsAttachArea = AttachAreaWindow;
		if (strcasestr(res, "window"))
			session.tabsAttachArea = AttachAreaWindow;
		if (strcasestr(res, "titlebar"))
			session.tabsAttachArea = AttachAreaTitlebar;
#if 0
		/* TODO: do more with these */
		res = readres("session.titlebar.left", "Session.Titlebar.Left", "stick");
		session.titlebar.left = res;
		res = readres("session.titlebar.right",
			      "Session.Titlebar.Right", "shade minimize maximize close");
		session.titlebar.right = res;
#endif
		res = readres("session.appsFile", "Session.AppsFile", "~/.fluxbox/apps");
		session.appsFile = res;
		res =
		    readres("session.groupFile", "Session.GroupFile",
			    "~/.fluxbox/groups");
		session.groupFile = res;
		res = readres("session.keyFile", "Session.KeyFile", "~/.fluxbox/keys");
		session.keyFile = res;
		res = readres("session.menuFile", "Session.MenuFile", "~/.fluxbox/menu");
		session.menuFile = res;
		res =
		    readres("session.slitlistFile", "Session.SlitlistFile",
			    "~/.fluxbox/slitlist");
		session.slitlistFile = res;
		res =
		    readres("session.styleFile", "Session.StyleFile",
			    "/usr/share/fluxbox/styles/bloe");
		session.styleFile = res;
		res =
		    readres("session.styleOverlay", "Session.StyleOverlay",
			    "~/.fluxbox/overlay");
		session.styleOverlay = res;

		res = readres("session.modKey", "Session.ModKey", "Mod1");
		session.modKey = Mod1Mask;
		if (!strcasecmp(res, "mod1"))
			session.modKey = Mod1Mask;
		if (!strcasecmp(res, "mod4"))
			session.modKey = Mod4Mask;
		if (!strcasecmp(res, "shift"))
			session.modKey = ShiftMask;
		if (!strcasecmp(res, "control"))
			session.modKey = ControlMask;
		getbool("session.imageDither", "Session.ImageDither", NULL, False,
			&session.imageDither);
		res = readres("session.configVersion", "Session.ConfigVersion", "13");
		session.configVersion = res;
		getbool("session.opaqueMove", "Session.OpaqueMove", NULL, True,
			&session.opaqueMove);
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

	snprintf(n, nlen, "window.focus.alpha");
	snprintf(c, clen, "Window.Focus.Alpha");
	res = readres(n, c, "255");
	screen->window.focus.alpha = strtoul(res, NULL, 0);

	snprintf(n, nlen, "window.unfocus.alpha");
	snprintf(c, clen, "Window.Unfocus.Alpha");
	res = readres(n, c, "255");
	screen->window.unfocus.alpha = strtoul(res, NULL, 0);

	snprintf(n, nlen, "menu.alpha");
	snprintf(c, clen, "Menu.Alpha");
	res = readres(n, c, "255");
	screen->menu.alpha = strtoul(res, NULL, 0);

	snprintf(n, nlen, "slit.autoHide");
	snprintf(c, clen, "Slit.AutoHide");
	getbool(n, c, NULL, False, &screen->slit.autoHide);

	snprintf(n, nlen, "slit.layer");
	snprintf(c, clen, "Slit.Layer");
	res = readres(n, c, "Dock");
	screen->slit.layer = LayerDock;
	if (!strcasecmp(res, "above dock"))
		screen->slit.layer = LayerAboveDock;
	if (!strcasecmp(res, "dock"))
		screen->slit.layer = LayerDock;
	if (!strcasecmp(res, "top"))
		screen->slit.layer = LayerTop;
	if (!strcasecmp(res, "normal"))
		screen->slit.layer = LayerNormal;
	if (!strcasecmp(res, "bottom"))
		screen->slit.layer = LayerBottom;
	if (!strcasecmp(res, "desktop"))
		screen->slit.layer = LayerDesktop;

	snprintf(n, nlen, "slit.placement");
	snprintf(c, clen, "Slit.Placement");
	res = readres(n, c, "RightBottom");
	screen->slit.placement = PlaceRightBottom;
	if (strcasestr(res, "bottomleft"))
		screen->slit.placement = PlaceBottomLeft;
	if (strcasestr(res, "bottomcenter"))
		screen->slit.placement = PlaceBottomCenter;
	if (strcasestr(res, "bottomright"))
		screen->slit.placement = PlaceBottomRight;
	if (strcasestr(res, "leftbottom"))
		screen->slit.placement = PlaceLeftBottom;
	if (strcasestr(res, "leftcenter"))
		screen->slit.placement = PlaceLeftCenter;
	if (strcasestr(res, "lefttop"))
		screen->slit.placement = PlaceLeftTop;
	if (strcasestr(res, "rightbottom"))
		screen->slit.placement = PlaceRightBottom;
	if (strcasestr(res, "rightcenter"))
		screen->slit.placement = PlaceRightCenter;
	if (strcasestr(res, "righttop"))
		screen->slit.placement = PlaceRightTop;
	if (strcasestr(res, "topleft"))
		screen->slit.placement = PlaceTopLeft;
	if (strcasestr(res, "topcenter"))
		screen->slit.placement = PlaceTopCenter;
	if (strcasestr(res, "topright"))
		screen->slit.placement = PlaceTopRight;

	snprintf(n, nlen, "slit.maxOver");
	snprintf(c, clen, "Slit.MaxOver");
	getbool(n, c, NULL, False, &screen->slit.maxOver);

	snprintf(n, nlen, "slit.onhead");
	snprintf(c, clen, "Slit.Onhead");
	res = readres(n, c, "0");
	screen->slit.onhead = strtoul(res, NULL, 0);

	snprintf(n, nlen, "slit.acceptKdeDockapps");
	snprintf(c, clen, "Slit.AcceptKdeDockapps");
	getbool(n, c, NULL, False, &screen->slit.acceptKdeDockapps);

	snprintf(n, nlen, "slit.alpha");
	snprintf(c, clen, "Slit.Alpha");
	res = readres(n, c, "255");
	screen->slit.alpha = strtoul(res, NULL, 0);

	snprintf(n, nlen, "slit.direction");
	snprintf(c, clen, "Slit.Direction");
	res = readres(n, c, "Vertical");
	screen->slit.direction = DirectionVertical;
	if (strcasestr(res, "vertical"))
		screen->slit.direction = DirectionVertical;
	if (strcasestr(res, "horizontal"))
		screen->slit.direction = DirectionHorizontal;

	snprintf(n, nlen, "slit.onTop");
	snprintf(c, clen, "Slit.OnTop");
	getbool(n, c, NULL, False, &screen->slit.onTop);

	snprintf(n, nlen, "toolbar.alpha");
	snprintf(c, clen, "Toolbar.Alpha");
	res = readres(n, c, "255");
	screen->toolbar.alpha = strtoul(res, NULL, 0);

	snprintf(n, nlen, "toolbar.autoHide");
	snprintf(c, clen, "Toolbar.AutoHide");
	getbool(n, c, NULL, False, &screen->toolbar.autoHide);

	snprintf(n, nlen, "toolbar.layer");
	snprintf(c, clen, "Toolbar.Layer");
	res = readres(n, c, "Dock");
	screen->toolbar.layer = LayerDock;
	if (!strcasecmp(res, "above dock"))
		screen->toolbar.layer = LayerAboveDock;
	if (!strcasecmp(res, "dock"))
		screen->toolbar.layer = LayerDock;
	if (!strcasecmp(res, "top"))
		screen->toolbar.layer = LayerTop;
	if (!strcasecmp(res, "normal"))
		screen->toolbar.layer = LayerNormal;
	if (!strcasecmp(res, "bottom"))
		screen->toolbar.layer = LayerBottom;
	if (!strcasecmp(res, "desktop"))
		screen->toolbar.layer = LayerDesktop;

	snprintf(n, nlen, "toolbar.placement");
	snprintf(c, clen, "Toolbar.Placement");
	res = readres(n, c, "BottomCenter");
	screen->toolbar.placement = PlaceBottomCenter;
	if (strcasestr(res, "bottomleft"))
		screen->toolbar.placement = PlaceBottomLeft;
	if (strcasestr(res, "bottomcenter"))
		screen->toolbar.placement = PlaceBottomCenter;
	if (strcasestr(res, "bottomright"))
		screen->toolbar.placement = PlaceBottomRight;
	if (strcasestr(res, "leftbottom"))
		screen->toolbar.placement = PlaceLeftBottom;
	if (strcasestr(res, "leftcenter"))
		screen->toolbar.placement = PlaceLeftCenter;
	if (strcasestr(res, "lefttop"))
		screen->toolbar.placement = PlaceLeftTop;
	if (strcasestr(res, "rightbottom"))
		screen->toolbar.placement = PlaceRightBottom;
	if (strcasestr(res, "rightcenter"))
		screen->toolbar.placement = PlaceRightCenter;
	if (strcasestr(res, "righttop"))
		screen->toolbar.placement = PlaceRightTop;
	if (strcasestr(res, "topleft"))
		screen->toolbar.placement = PlaceTopLeft;
	if (strcasestr(res, "topcenter"))
		screen->toolbar.placement = PlaceTopCenter;
	if (strcasestr(res, "topright"))
		screen->toolbar.placement = PlaceTopRight;

	snprintf(n, nlen, "toolbar.maxOver");
	snprintf(c, clen, "Toolbar.MaxOver");
	getbool(n, c, NULL, False, &screen->toolbar.maxOver);

	snprintf(n, nlen, "toolbar.height");
	snprintf(c, clen, "Toolbar.Height");
	res = readres(n, c, "0");
	screen->toolbar.height = strtoul(res, NULL, 0);

	snprintf(n, nlen, "toolbar.visible");
	snprintf(c, clen, "Toolbar.Visible");
	getbool(n, c, NULL, True, &screen->toolbar.visible);

	snprintf(n, nlen, "toolbar.widthPercent");
	snprintf(c, clen, "Toolbar.WidthPercent");
	res = readres(n, c, "100");
	screen->toolbar.widthPercent = strtoul(res, NULL, 0);

	snprintf(n, nlen, "toolbar.tools");
	snprintf(c, clen, "Toolbar.Tools");
	res =
	    readres(n, c,
		    "workspacename, prevworkspace, nextworkspace, iconbar, prevwindow, nextwindow, systemtray, clock");
	screen->toolbar.tools = res;

	snprintf(n, nlen, "toolbar.onhead");
	snprintf(c, clen, "Toolbar.Onhead");
	res = readres(n, c, "1");
	screen->toolbar.onhead = strtoul(res, NULL, 0);

	snprintf(n, nlen, "toolbar.onTop");
	snprintf(c, clen, "Toolbar.OnTop");
	getbool(n, c, NULL, True, &screen->toolbar.onTop);

	snprintf(n, nlen, "tabs.maxOver");
	snprintf(c, clen, "Tabs.MaxOver");
	getbool(n, c, NULL, True, &screen->tabs.maxOver);

	snprintf(n, nlen, "tabs.intitlebar");
	snprintf(c, clen, "Tabs.Intitlebar");
	getbool(n, c, NULL, True, &screen->tabs.intitlebar);

	snprintf(n, nlen, "tabs.usePixmap");
	snprintf(c, clen, "Tabs.UsePixmap");
	getbool(n, c, NULL, True, &screen->tabs.usePixmap);

	snprintf(n, nlen, "iconbar.pattern");
	snprintf(c, clen, "Iconbar.Pattern");
	res = readres(n, c, "");
	screen->iconbar.pattern = res;

	snprintf(n, nlen, "iconbar.mode");
	snprintf(c, clen, "Iconbar.Mode");
	res = readres(n, c, "{static groups} (workspace)");
	screen->iconbar.mode = res;

	snprintf(n, nlen, "iconbar.usePixmap");
	snprintf(c, clen, "Iconbar.UsePixmap");
	getbool(n, c, NULL, True, &screen->iconbar.usePixmap);

	snprintf(n, nlen, "iconbar.iconTextPadding");
	snprintf(c, clen, "Iconbar.IconTextPadding");
	res = readres(n, c, "10");
	screen->iconbar.iconTextPadding = strtoul(res, NULL, 0);

	snprintf(n, nlen, "iconbar.alignment");
	snprintf(c, clen, "Iconbar.Alignment");
	res = readres(n, c, "AlignLeft");
	screen->iconbar.alignment = AlignRelative;
	if (strcasestr(res, "left"))
		screen->iconbar.alignment = AlignLeft;
	if (strcasestr(res, "relative"))
		screen->iconbar.alignment = AlignRelative;
	if (strcasestr(res, "right"))
		screen->iconbar.alignment = AlignRight;

	snprintf(n, nlen, "iconbar.iconWidth");
	snprintf(c, clen, "Iconbar.IconWidth");
	res = readres(n, c, "128");
	screen->iconbar.iconWidth = strtoul(res, NULL, 0);

	snprintf(n, nlen, "iconbar.wheelMode");
	snprintf(c, clen, "Iconbar.WheelMode");
	res = readres(n, c, "Screen");
	screen->iconbar.wheelMode = res;

	snprintf(n, nlen, "overlay.capStyle");
	snprintf(c, clen, "Overlay.CapStyle");
	res = readres(n, c, "");
	screen->overlay.capStyle = res;

	snprintf(n, nlen, "overlay.joinStyle");
	snprintf(c, clen, "Overlay.JoinStyle");
	res = readres(n, c, "");
	screen->overlay.joinStyle = res;

	snprintf(n, nlen, "overlay.lineStyle");
	snprintf(c, clen, "Overlay.LineStyle");
	res = readres(n, c, "");
	screen->overlay.lineStyle = res;

	snprintf(n, nlen, "overlay.lineWidth");
	snprintf(c, clen, "Overlay.LineWidth");
	res = readres(n, c, "");
	screen->overlay.lineWidth = res;

	snprintf(n, nlen, "tab.placement");
	snprintf(c, clen, "Tab.Placement");
	res = readres(n, c, "TopLeft");
	screen->tab.placement = PlaceTopLeft;
	if (strcasestr(res, "bottomleft"))
		screen->tab.placement = PlaceBottomLeft;
	if (strcasestr(res, "bottomcenter"))
		screen->tab.placement = PlaceBottomCenter;
	if (strcasestr(res, "bottomright"))
		screen->tab.placement = PlaceBottomRight;
	if (strcasestr(res, "leftbottom"))
		screen->tab.placement = PlaceLeftBottom;
	if (strcasestr(res, "leftcenter"))
		screen->tab.placement = PlaceLeftCenter;
	if (strcasestr(res, "lefttop"))
		screen->tab.placement = PlaceLeftTop;
	if (strcasestr(res, "rightbottom"))
		screen->tab.placement = PlaceRightBottom;
	if (strcasestr(res, "rightcenter"))
		screen->tab.placement = PlaceRightCenter;
	if (strcasestr(res, "righttop"))
		screen->tab.placement = PlaceRightTop;
	if (strcasestr(res, "topleft"))
		screen->tab.placement = PlaceTopLeft;
	if (strcasestr(res, "topcenter"))
		screen->tab.placement = PlaceTopCenter;
	if (strcasestr(res, "topright"))
		screen->tab.placement = PlaceTopRight;

	snprintf(n, nlen, "tab.width");
	snprintf(c, clen, "Tab.Width");
	res = readres(n, c, "64");
	screen->tab.width = strtoul(res, NULL, 0);

	snprintf(n, nlen, "tab.height");
	snprintf(c, clen, "Tab.Height");
	res = readres(n, c, "0");
	screen->tab.height = strtoul(res, NULL, 0);

	snprintf(n, nlen, "titlebar.left");
	snprintf(c, clen, "Titlebar.Left");
	res = readres(n, c, "FIXME");
	screen->titlebar.left = res;

	snprintf(n, nlen, "titlebar.right");
	snprintf(c, clen, "Titlebar.Right");
	res = readres(n, c, "FIXME");
	screen->titlebar.right = res;

	snprintf(n, nlen, "strftimeFormat");
	snprintf(c, clen, "StrftimeFormat");
	res = readres(n, c, "%k:%M");
	screen->strftimeFormat = res;

	snprintf(n, nlen, "autoRaise");
	snprintf(c, clen, "AutoRaise");
	getbool(n, c, NULL, True, &screen->autoRaise);

	snprintf(n, nlen, "clickRaises");
	snprintf(c, clen, "ClickRaises");
	getbool(n, c, NULL, True, &screen->clickRaises);

	snprintf(n, nlen, "workspacewarping");
	snprintf(c, clen, "Workspacewarping");
	getbool(n, c, NULL, True, &screen->workspacewarping);

	snprintf(n, nlen, "showwindowposition");
	snprintf(c, clen, "Showwindowposition");
	getbool(n, c, NULL, False, &screen->showwindowposition);

	snprintf(n, nlen, "defaultDeco");
	snprintf(c, clen, "DefaultDeco");
	res = readres(n, c, "NORMAL");
	screen->defaultDeco = DecoNormal;
	if (!strcasecmp(res, "normal"))
		screen->defaultDeco = DecoNormal;
	if (!strcasecmp(res, "none"))
		screen->defaultDeco = DecoNone;
	if (!strcasecmp(res, "border"))
		screen->defaultDeco = DecoBorder;
	if (!strcasecmp(res, "tab"))
		screen->defaultDeco = DecoTab;
	if (!strcasecmp(res, "tiny"))
		screen->defaultDeco = DecoTiny;
	if (!strcasecmp(res, "tool"))
		screen->defaultDeco = DecoTool;

	snprintf(n, nlen, "menuDelay");
	snprintf(c, clen, "MenuDelay");
	res = readres(n, c, "200");
	screen->menuDelay = strtoul(res, NULL, 0);

	snprintf(n, nlen, "menuDelayClose");
	snprintf(c, clen, "MenuDelayClose");
	res = readres(n, c, NULL);
	if (res)
		screen->menuDelayClose = strtoul(res, NULL, 0);
	else
		screen->menuDelayClose = screen->menuDelay;

	snprintf(n, nlen, "menuMode");
	snprintf(c, clen, "MenuMode");
	res = readres(n, c, "FIXME");
	screen->menuMode = res;

	snprintf(n, nlen, "focusNewWindows");
	snprintf(c, clen, "FocusNewWindows");
	getbool(n, c, NULL, True, &screen->focusNewWindows);

	snprintf(n, nlen, "workspaceNames");
	snprintf(c, clen, "WorkspaceNames");
	res = readres(n, c, "Workspace 1, Workspace 2, Workspace 3, Workspace 4");
	screen->workspaceNames = res;

	snprintf(n, nlen, "edgeSnapThreshold");
	snprintf(c, clen, "EdgeSnapThreshold");
	res = readres(n, c, "10");
	screen->edgeSnapThreshold = strtoul(res, NULL, 0);

	snprintf(n, nlen, "windowPlacement");
	snprintf(c, clen, "WindowPlacement");
	res = readres(n, c, "RowSmartPlacement");
	screen->windowPlacement = PlacementRowSmart;
	if (strcasestr(res, "rowsmartplacement"))
		screen->windowPlacement = PlacementRowSmart;
	if (strcasestr(res, "colsmartplacement"))
		screen->windowPlacement = PlacementColSmart;
	if (strcasestr(res, "cascadeplacement"))
		screen->windowPlacement = PlacementCascade;
	if (strcasestr(res, "undermouseplacement"))
		screen->windowPlacement = PlacementUnderMouse;

	snprintf(n, nlen, "rowPlacementDirection");
	snprintf(c, clen, "RowPlacementDirection");
	res = readres(n, c, "LeftToRight");
	screen->rowPlacementDirection = PlaceLeftToRight;
	if (strcasestr(res, "lefttoright"))
		screen->rowPlacementDirection = PlaceLeftToRight;
	if (strcasestr(res, "righttoleft"))
		screen->rowPlacementDirection = PlaceRightToLeft;

	snprintf(n, nlen, "colPlacementDirection");
	snprintf(c, clen, "ColPlacementDirection");
	res = readres(n, c, "TopToBottom");
	screen->colPlacementDirection = PlaceTopToBottom;
	if (strcasestr(res, "toptobottom"))
		screen->colPlacementDirection = PlaceTopToBottom;
	if (strcasestr(res, "bottomtotop"))
		screen->colPlacementDirection = PlaceBottomToTop;

	snprintf(n, nlen, "fullMaximization");
	snprintf(c, clen, "FullMaximization");
	getbool(n, c, NULL, False, &screen->fullMaximization);

	snprintf(n, nlen, "opaqueMove");
	snprintf(c, clen, "OpaqueMove");
	getbool(n, c, NULL, True, &screen->opaqueMove);

	snprintf(n, nlen, "workspaces");
	snprintf(c, clen, "Workspaces");
	res = readres(n, c, "4");
	screen->workspaces = strtoul(res, NULL, 0);

	snprintf(n, nlen, "windowMenu");
	snprintf(c, clen, "WindowMenu");
	res = readres(n, c, "~/.fluxbox/windowmenu");
	screen->windowMenu = res;

	snprintf(n, nlen, "allowRemoteActions");
	snprintf(c, clen, "AllowRemoteActions");
	getbool(n, c, NULL, False, &screen->allowRemoteActions);

	snprintf(n, nlen, "clientMenu.usePixmap");
	snprintf(c, clen, "ClientMenu.UsePixmap");
	getbool(n, c, NULL, True, &screen->clientMenu.usePixmap);

	snprintf(n, nlen, "decorateTransient");
	snprintf(c, clen, "DecorateTransient");
	getbool(n, c, NULL, True, &screen->decorateTransient);

	snprintf(n, nlen, "demandsAttentionTimeout");
	snprintf(c, clen, "DemandsAttentionTimeout");
	res = readres(n, c, "500");
	screen->demandsAttentionTimeout = strtoul(res, NULL, 0);

	snprintf(n, nlen, "desktopwheeling");
	snprintf(c, clen, "Desktopwheeling");
	getbool(n, c, NULL, True, &screen->desktopwheeling);

	snprintf(n, nlen, "focusLastWindow");
	snprintf(c, clen, "FocusLastWindow");
	getbool(n, c, NULL, True, &screen->focusLastWindow);

	snprintf(n, nlen, "focusModel");
	snprintf(c, clen, "FocusModel");
	res = readres(n, c, "ClickToFocus");
	screen->focusModel = FocusModelClickToFocus;
	if (strcasestr(res, "clicktofocus"))
		screen->focusModel = FocusModelClickToFocus;
	if (strcasestr(res, "mousefocus"))
		screen->focusModel = FocusModelMouseFocus;
	if (strcasestr(res, "strictmousefocus"))
		screen->focusModel = FocusModelStrictMouseFocus;

	snprintf(n, nlen, "focusSameHead");
	snprintf(c, clen, "FocusSameHead");
	getbool(n, c, NULL, False, &screen->focusSameHead);

	snprintf(n, nlen, "imageDither");
	snprintf(c, clen, "ImageDither");
	getbool(n, c, NULL, True, &screen->imageDither);

	snprintf(n, nlen, "maxDisableMove");
	snprintf(c, clen, "MaxDisableMove");
	getbool(n, c, NULL, False, &screen->maxDisableMove);

	snprintf(n, nlen, "maxDisableResize");
	snprintf(c, clen, "MaxDisableResize");
	getbool(n, c, NULL, False, &screen->maxDisableResize);

	snprintf(n, nlen, "maxIgnoreIncrement");
	snprintf(c, clen, "MaxIgnoreIncrement");
	getbool(n, c, NULL, False, &screen->maxIgnoreIncrement);

	snprintf(n, nlen, "noFocusWhileTypingDelay");
	snprintf(c, clen, "NoFocusWhileTypingDelay");
	res = readres(n, c, "0");
	screen->noFocusWhileTypingDelay = strtoul(res, NULL, 0);

	snprintf(n, nlen, "resizeMode");
	snprintf(c, clen, "ResizeMode");
	res = readres(n, c, "Bottom");
	screen->resizeMode = res;

	snprintf(n, nlen, "reversewheeling");
	snprintf(c, clen, "Reversewheeling");
	getbool(n, c, NULL, False, &screen->reversewheeling);

	snprintf(n, nlen, "rootCommand");
	snprintf(c, clen, "RootCommand");
	res = readres(n, c, "");
	screen->rootCommand = res;

	snprintf(n, nlen, "tabFocusModel");
	snprintf(c, clen, "TabFocusModel");
	res = readres(n, c, "SloppyTabFocus");
	screen->tabFocusModel = res;

	snprintf(n, nlen, "tooltipDelay");
	snprintf(c, clen, "TooltipDelay");
	res = readres(n, c, "500");
	screen->tooltipDelay = strtoul(res, NULL, 0);

	snprintf(n, nlen, "userFollowModel");
	snprintf(c, clen, "UserFollowModel");
	res = readres(n, c, "Follow");
	screen->userFollowModel = res;

	snprintf(n, nlen, "windowScrollAction");
	snprintf(c, clen, "WindowScrollAction");
	res = readres(n, c, "");
	screen->windowScrollAction = res;

	snprintf(n, nlen, "windowScrollReverse");
	snprintf(c, clen, "WindowScrollReverse");
	getbool(n, c, NULL, False, &screen->windowScrollReverse);

}

typedef struct {
	FILE *f;
	char *buff;
	char *func;
	char *keys;
	char *args;
	Key *leaf;
} ParserContext;

ParserContext *pctx;

typedef struct _KeyItem KeyItem;
struct _KeyItem {
	const char *name;
	void (*parse) (Key *);
};

static void
p_min(Key *k)
{
	k->func = k_setmin;
	k->set = ToggleFlagSetting;
	k->any = FocusClient;
}

static void
p_max(Key *k)
{
	k->func = k_setmax;
	k->set = ToggleFlagSetting;
	k->any = FocusClient;
}

static void
p_maxh(Key *k)
{
	k->func = k_setmaxh;
	k->set = ToggleFlagSetting;
	k->any = FocusClient;
}

static void
p_maxv(Key *k)
{
	k->func = k_setmaxv;
	k->set = ToggleFlagSetting;
	k->any = FocusClient;
}

static void
p_full(Key *k)
{
	k->func = k_setfull;
	k->set = ToggleFlagSetting;
	k->any = FocusClient;
}

static void
p_raise(Key *k)
{
	k->func = k_raise;
}

static void
p_lower(Key *k)
{
	k->func = k_lower;
}

static void
p_raiselayer(Key *k)
{
	/* FIXME: todo */
}

static void
p_lowerlayer(Key *k)
{
	/* FIXME: todo */
}

static void
p_setlayer(Key *k)
{
	/* FIXME: todo */
}

static void
p_close(Key *k)
{
	k->func = k_killclient;
}

static void
p_kill(Key *k)
{
	k->func = k_killclient;
}

static void
p_shade(Key *k)
{
	k->func = k_setshade;
	k->set = ToggleFlagSetting;
	k->any = FocusClient;
}

static void
p_stick(Key *k)
{
	k->func = k_toggletag;
	k->dir = RelativeNone;
	k->tag = -1U;
}

static void
p_setdecor(Key *k)
{
	/* FIXME: todo */
}

static void
p_toggledecor(Key *k)
{
	/* FIXME: todo */
}

static void
p_tabnext(Key *k)
{
	k->func = k_tab;
	k->any = FocusClient;
	k->dir = RelativeNext;
}

static void
p_tabprev(Key *k)
{
	k->func = k_tab;
	k->any = FocusClient;
	k->dir = RelativePrev;
}

static void
p_tab(Key *k)
{
	if (sscanf(pctx->args, "%u", &k->tag) < 1)
		return;
	if (k->tag)
		k->tag--;
	k->func = k_tab;
	k->any = FocusClient;
	k->dir = RelativeNone;
}

static void
p_movetabright(Key *k)
{
}

static void
p_movetableft(Key *k)
{
}

static void
p_detach(Key *k)
{
}

static void
p_resizeto(Key *k)
{
	unsigned w, h;
	char *p = pctx->args, *e = NULL;
	char arg[64];

	for (; p < e && *p && isblank(*p); p++) ;
	w = strtoul(p, &e, 0);
	if (!w && e == p)
		return;
	if (*e == '%') {
		if (w > 100)
			w = 100;
		if (w < 5)
			w = 5;
		w = (DisplayWidth(dpy, scr->screen) * w) / 100;
		p = e + 1;
	} else if (isblank(*e)) {
		if (w < 10)
			w = 10;
		if (w > DisplayWidth(dpy, scr->screen))
			w = DisplayWidth(dpy, scr->screen);
		p = e;
	} else
		return;
	for (; p < e && *p && isblank(*p); p++) ;
	h = strtoul(p, &e, 0);
	if (!h && e == p)
		return;
	if (*e == '%') {
		if (h > 100)
			h = 100;
		if (h < 5)
			h = 5;
		h = (DisplayHeight(dpy, scr->screen) * h) / 100;
		p = e + 1;
	} else if (!*e || isblank(*e) || *e == '\n') {
		if (h < 10)
			h = 10;
		if (h > DisplayHeight(dpy, scr->screen))
			h = DisplayHeight(dpy, scr->screen);
	} else
		return;
	snprintf(arg, sizeof(arg), "0 0 %u %u", w, h);
	k->func = k_moveresizekb;
	k->arg = strdup(arg);
}

static void
p_resize(Key *k)
{
	int dw, dh;
	char *p = pctx->args, *e = NULL;
	char arg[64];

	for (; p < e && *p && isblank(*p); p++) ;
	dw = strtol(p, &e, 0);
	if (!dw && e == p)
		return;
	if (*e == '%') {
		if (dw > 100)
			dw = 100;
		if (dw < 1)
			dw = 1;
		dw = (DisplayWidth(dpy, scr->screen) * dw) / 100;
		p = e + 1;
	} else if (isblank(*e)) {
		if (dw < 1)
			dw = 1;
		if (dw > DisplayWidth(dpy, scr->screen))
			dw = DisplayWidth(dpy, scr->screen);
		p = e;
	} else
		return;
	for (; p < e && *p && isblank(*p); p++) ;
	dh = strtol(p, &e, 0);
	if (!dh && e == p)
		return;
	if (*e == '%') {
		if (dh > 100)
			dh = 100;
		if (dh < 1)
			dh = 1;
		dh = (DisplayHeight(dpy, scr->screen) * dh) / 100;
		p = e + 1;
	} else if (!*e || isblank(*e) || *e == '\n') {
		if (dh < 1)
			dh = 1;
		if (dh > DisplayHeight(dpy, scr->screen))
			dh = DisplayHeight(dpy, scr->screen);
	} else
		return;
	snprintf(arg, sizeof(arg), "0 0 %s%u %s%u",
		 dw < 0 ? "-" : (dw > 0 ? "+" : ""), (unsigned) abs(dw),
		 dh < 0 ? "-" : (dh > 0 ? "+" : ""), (unsigned) abs(dh));
	k->func = k_moveresizekb;
	k->arg = strdup(arg);
}

static void
p_resizeh(Key *k)
{
	int dw;
	char *p = pctx->args, *e = NULL;
	char arg[64];

	for (; p < e && *p && isblank(*p); p++) ;
	dw = strtol(p, &e, 0);
	if (!dw && e == p)
		return;
	if (*e == '%') {
		if (dw > 100)
			dw = 100;
		if (dw < 1)
			dw = 1;
		dw = (DisplayWidth(dpy, scr->screen) * dw) / 100;
	} else if (!*e || isblank(*e) || *e == '\n') {
		if (dw < 1)
			dw = 1;
		if (dw > DisplayWidth(dpy, scr->screen))
			dw = DisplayWidth(dpy, scr->screen);
	} else
		return;
	snprintf(arg, sizeof(arg), "0 0 %s%u 0",
		 dw < 0 ? "-" : (dw > 0 ? "+" : ""), (unsigned) abs(dw));
	k->func = k_moveresizekb;
	k->arg = strdup(arg);
}

static void
p_resizev(Key *k)
{
	int dh;
	char *p = pctx->args, *e = NULL;
	char arg[64];

	for (; p < e && *p && isblank(*p); p++) ;
	dh = strtol(p, &e, 0);
	if (!dh && e == p)
		return;
	if (*e == '%') {
		if (dh > 100)
			dh = 100;
		if (dh < 1)
			dh = 1;
		dh = (DisplayHeight(dpy, scr->screen) * dh) / 100;
	} else if (!*e || isblank(*e) || *e == '\n') {
		if (dh < 1)
			dh = 1;
		if (dh > DisplayHeight(dpy, scr->screen))
			dh = DisplayHeight(dpy, scr->screen);
	} else
		return;
	snprintf(arg, sizeof(arg), "0 0 0 %s%u", dh < 0 ? "-" : "+", (unsigned) abs(dh));
	k->func = k_moveresizekb;
	k->arg = strdup(arg);
}

static void
p_moveto(Key *k)
{
}

static void
p_move(Key *k)
{
	int dx, dy;
	char *p = pctx->args, *e = NULL;
	char arg[64];

	for (; p < e && *p && isblank(*p); p++) ;
	dx = strtol(p, &e, 0);
	if (!dx && e == p)
		return;
	if (!isblank(*e))
		return;
	for (p = e; p < e && *p && isblank(*p); p++) ;
	dy = strtol(p, &e, 0);
	if (!dy && e == p)
		return;
	if (*p && !isblank(*p))
		return;
	snprintf(arg, sizeof(arg), "%s%u %s%u 0 0",
		 dx < 0 ? "-" : "+", (unsigned) abs(dx),
		 dy < 0 ? "-" : "+", (unsigned) abs(dy));
	k->func = k_moveresizekb;
	k->arg = strdup(arg);
}

static void
p_moveright(Key *k)
{
	k->func = k_moveby;
	k->dir = RelativeEast;
	k->arg = strdup(pctx->args);
}

static void
p_moveleft(Key *k)
{
	k->func = k_moveby;
	k->dir = RelativeWest;
	k->arg = strdup(pctx->args);
}

static void
p_moveup(Key *k)
{
	k->func = k_moveby;
	k->dir = RelativeNorth;
	k->arg = strdup(pctx->args);
}

static void
p_movedown(Key *k)
{
	k->func = k_moveby;
	k->dir = RelativeSouth;
	k->arg = strdup(pctx->args);
}

static void
p_taketo(Key *k)
{
	if (sscanf(pctx->args, "%u", &k->tag) < 1)
		return;
	if (k->tag)
		k->tag--;
	k->func = k_taketo;
	k->dir = RelativeNone;
}

static void
p_sendto(Key *k)
{
	if (sscanf(pctx->args, "%u", &k->tag) < 1)
		return;
	if (k->tag)
		k->tag--;
	k->func = k_tag;
	k->dir = RelativeNone;
}

static void
p_takenext(Key *k)
{
	k->func = k_taketo;
	k->dir = RelativeNext;
}

static void
p_takeprev(Key *k)
{
	k->func = k_taketo;
	k->dir = RelativePrev;
}

static void
p_sendnext(Key *k)
{
	k->func = k_tag;
	k->dir = RelativeNext;
}

static void
p_sendprev(Key *k)
{
	k->func = k_tag;
	k->dir = RelativePrev;
}

static void
p_setalpha(Key *k)
{
}

static void
p_sethead(Key *k)
{
}

static void
p_headnext(Key *k)
{
}

static void
p_headprev(Key *k)
{
}

static void
p_setxprop(Key *k)
{
}

static void
p_deskadd(Key *k)
{
	k->func = k_appendtag;
}

static void
p_deskrem(Key *k)
{
	k->func = k_rmlasttag;
}

static void
p_desknext(Key *k)
{
	k->func = k_view;
	k->dir = RelativeNext;
	k->wrap = True;
}

static void
p_deskprev(Key *k)
{
	k->func = k_view;
	k->dir = RelativePrev;
	k->wrap = True;
}

static void
p_deskright(Key *k)
{
	k->func = k_view;
	k->dir = RelativeNext;
	k->wrap = False;
}

static void
p_deskleft(Key *k)
{
	k->func = k_view;
	k->dir = RelativePrev;
	k->wrap = False;
}

static void
p_desk(Key *k)
{
	if (sscanf(pctx->args, "%u", &k->tag) < 1)
		return;
	if (k->tag)
		k->tag--;
	k->func = k_view;
	k->dir = RelativeNone;
	k->wrap = True;
}

static void
p_next(Key *k)
{
}

static void
p_prev(Key *k)
{
}

static void
p_groupnext(Key *k)
{
}

static void
p_groupprev(Key *k)
{
}

static void
p_window(Key *k)
{
}

static void
p_activate(Key *k)
{
}

static void
p_focus(Key *k)
{
}

static void
p_attach(Key *k)
{
}

static void
p_focusleft(Key *k)
{
}

static void
p_focusright(Key *k)
{
}

static void
p_focusup(Key *k)
{
}

static void
p_focusdown(Key *k)
{
}

static void
p_arrange(Key *k)
{
}

static void
p_arrangev(Key *k)
{
}

static void
p_arrangeh(Key *k)
{
}

static void
p_showing(Key *k)
{
	k->func = k_setshowing;
	k->set = ToggleFlagSetting;
	k->any = FocusClient;
}

static void
p_deiconify(Key *k)
{
	const char *p = pctx->args;

	k->any = FocusClient;
	if (strcasestr(p, "allworkspace")) {
		k->any = AllClients;
	} else if (strcasestr(p, "all")) {
		k->any = AnyClient;
	} else if (strcasestr(p, "lastworkspace")) {
		k->any = FocusClient;
	} else if (strcasestr(p, "last")) {
		k->any = ActiveClient;
	}
	if (strcasestr(p, "originquiet")) {
	} else if (strcasestr(p, "current")) {
	}
	k->func = k_setmin;
	k->set = UnsetFlagSetting;
	k->dir = RelativeCenter;
	k->ico = OnlyIcons;
}

static void
p_setdeskname(Key *k)
{
}

static void
p_setdesknamedialog(Key *k)
{
}

static void
p_closeall(Key *k)
{
}

static void
p_rootmenu(Key *k)
{
}

static void
p_deskmenu(Key *k)
{
}

static void
p_windowmenu(Key *k)
{
}

static void
p_clientmenu(Key *k)
{
}

static void
p_custommenu(Key *k)
{
}

static void
p_hidemenus(Key *k)
{
}

static void
p_restart(Key *k)
{
	char *p = pctx->args, *e = p + strlen(p);

	for (; p < e && *p && isblank(*p); p++) ;
	for (; e > p && (!*e || isspace(*e)); e--) ;
	k->func = k_restart;
	if (p < e)
		k->arg = strndup(p, e - p);
}

static void
p_quit(Key *k)
{
	k->func = k_quit;
}

static void
p_reconfig(Key *k)
{
}

static void
p_setstyle(Key *k)
{
}

static void
p_reloadstyle(Key *k)
{
}

static void
p_exec(Key *k)
{
	char *p = pctx->args, *e = p + strlen(p);

	for (; p < e && *p && isblank(*p); p++) ;
	for (; e > p && (!*e || isspace(*e)); e--) ;
	k->func = k_spawn;
	if (p < e)
		k->arg = strndup(p, e - p);
}

static void
p_command(Key *k)
{
}

static void
p_setenv(Key *k)
{
}

static void
p_setres(Key *k)
{
}

static void
p_setresdialog(Key *k)
{
}

static void
p_macrocmd(Key *k)
{
}

const KeyItem KeyItems[] = {
	/* *INDENT-OFF* */
	{ "minimize",		    &p_min		},
	{ "minimizewindow",	    &p_min		},
	{ "iconify",		    &p_min		},
	{ "maximize",		    &p_max		},
	{ "maximizewindow",	    &p_max		},
	{ "maximizehorizontal",	    &p_maxh		},
	{ "maximizevertical",	    &p_maxv		},
	{ "fullscreen",		    &p_full		},
	{ "raise",		    &p_raise		},
	{ "lower",		    &p_lower		},
	{ "raiselayer",		    &p_raiselayer	},
	{ "lowerlayer",		    &p_lowerlayer	},
	{ "setlayer",		    &p_setlayer		},
	{ "close",		    &p_close		},
	{ "kill",		    &p_kill		},
	{ "killwindow",		    &p_kill		},
	{ "shade",		    &p_shade		},
	{ "shadewindow",	    &p_shade		},
	{ "stick",		    &p_stick		},
	{ "stickwindow",	    &p_stick		},
	{ "setdecor",		    &p_setdecor		},
	{ "toggledecor",	    &p_toggledecor	},
	{ "nexttab",		    &p_tabnext		},
	{ "prevtab",		    &p_tabprev		},
	{ "tab",		    &p_tab		},
	{ "movetabright",	    &p_movetabright	},
	{ "movetableft",	    &p_movetableft	},
	{ "detachclient",	    &p_detach		},
	{ "resizeto",		    &p_resizeto		},
	{ "resize",		    &p_resize		},
	{ "resizehorizontal",	    &p_resizeh		},
	{ "resizevertical",	    &p_resizev		},
	{ "moveto",		    &p_moveto		},
	{ "move",		    &p_move		},
	{ "moveright",		    &p_moveright	},
	{ "moveleft",		    &p_moveleft		},
	{ "moveup",		    &p_moveup		},
	{ "movedown",		    &p_movedown		},
	{ "taketoworkspace",	    &p_taketo		},
	{ "sendtoworkspace",	    &p_sendto		},
	{ "taketonextworkspace",    &p_takenext		},
	{ "taketoprevworkspace",    &p_takeprev		},
	{ "sendtonextworkspace",    &p_sendnext		},
	{ "sendtoprevworkspace",    &p_sendprev		},
	{ "setalpha",		    &p_setalpha		},
	{ "sethead",		    &p_sethead		},
	{ "sendtonexthead",	    &p_headnext		},
	{ "sendtoprevhead",	    &p_headprev		},
	{ "setxprop",		    &p_setxprop		},

	{ "addworkspace",	    &p_deskadd		},
	{ "removelastworkspace",    &p_deskrem		},
	{ "nextworkspace",	    &p_desknext		},
	{ "prevworkspace",	    &p_deskprev		},
	{ "rightworkspace",	    &p_deskright	},
	{ "leftworkspace",	    &p_deskleft		},
	{ "workspace",		    &p_desk		},
	{ "nextwindow",		    &p_next		},
	{ "prevwindow",		    &p_prev		},
	{ "nextgroup",		    &p_groupnext	},
	{ "prevgroup",		    &p_groupprev	},
	{ "gotowindow",		    &p_window		},
	{ "activate",		    &p_activate		},
	{ "focus",		    &p_focus		},
	{ "attach",		    &p_attach		},
	{ "focusleft",		    &p_focusleft	},
	{ "focusright",		    &p_focusright	},
	{ "focusup",		    &p_focusup		},
	{ "focusdown",		    &p_focusdown	},
	{ "arrangewindows",	    &p_arrange		},
	{ "arrangewindowsvertical", &p_arrangev		},
	{ "arrangewindowshorizontal",&p_arrangeh	},
	{ "showdesktop",	    &p_showing		},
	{ "deiconify",		    &p_deiconify	},
	{ "setworkspacename",	    &p_setdeskname	},
	{ "setworkspacenamedialog", &p_setdesknamedialog},
	{ "closeallwindows",	    &p_closeall		},

	{ "rootmenu",		    &p_rootmenu		},
	{ "workspacemenu",	    &p_deskmenu		},
	{ "windowmenu",		    &p_windowmenu	},
	{ "clienmenu",		    &p_clientmenu	},
	{ "custommenu",		    &p_custommenu	},
	{ "hidemenus",		    &p_hidemenus	},

	{ "restart",		    &p_restart		},
	{ "quit",		    &p_quit		},
	{ "exit",		    &p_quit		},
	{ "reconfig",		    &p_reconfig		},
	{ "reconfigure",	    &p_reconfig		},
	{ "setstyle",		    &p_setstyle		},
	{ "reloadstyle",	    &p_reloadstyle	},
	{ "execcommand",	    &p_exec		},
	{ "exec",		    &p_exec		},
	{ "execute",		    &p_exec		},
	{ "commanddialog",	    &p_command		},
	{ "setenv",		    &p_setenv		},
	{ "export",		    &p_setenv		},
	{ "setresourcevalue",	    &p_setres		},
	{ "setresourcevaluedialog", &p_setresdialog	},

	{ "macrocmd",		    &p_macrocmd		},

	{ NULL, NULL }
	/* *INDENT-ON* */
};

static char *
p_getline()
{
	return fgets(pctx->buff, PATH_MAX, pctx->f);
}

static unsigned long
p_mod(const char **b, const char *e)
{
	unsigned long mods = 0;
	const char *p = *b;

	while (p < e) {
		for (; *p && isblank(*p); p++) ;
		if (!strncasecmp("control", p, sizeof("control"))) {
			mods |= ControlMask;
			p += sizeof("control");
		} else if (!strncasecmp("shift", p, sizeof("shift"))) {
			mods |= ShiftMask;
			p += sizeof("shift");
		} else if (!strncasecmp("mod1", p, sizeof("mod1"))) {
			mods |= Mod1Mask;
			p += sizeof("mod1");
		} else if (!strncasecmp("mod4", p, sizeof("mod4"))) {
			mods |= Mod4Mask;
			p += sizeof("mod4");
		}
	}
	*b = p;
	return mods;
}

static KeySym
p_sym(const char **b, const char *e)
{
	KeySym sym = NoSymbol;
	const char *p, *f;
	char *t;

	for (p = *b; p < e && *p && isblank(*p); p++) ;
	for (f = p; f < e && *f && !isblank(*f); f++) ;
	DPRINTF("Parsing keysym from '%s'\n", p);
	if (p < f && (t = strndup(p, f - p))) {
		sym = XStringToKeysym(t);
		free(t);
	}
	for (; f < e && *f && isblank(*f); f++) ;
	*b = f;
	return sym;
}

static Key *
p_key(const char *keys)
{
	unsigned long mod;
	KeySym sym = NoSymbol;
	const char *p = keys;
	const char *e = p + strlen(keys);
	Key *k, *chain = NULL, *leaf = NULL;

	while (p < e) {
		DPRINTF("Parsing key from '%s'\n", keys);
		mod = p_mod(&p, e);
		if ((sym = p_sym(&p, e)) == NoSymbol) {
			DPRINTF("Failed to parse symbol from '%s'\n", keys);
			if (chain)
				freechain(chain);
			return NULL;
		}
		k = ecalloc(1, sizeof(*k));
		k->mod = mod;
		k->keysym = sym;
		if (leaf) {
			leaf->chain = k;
			leaf->func = &k_chain;
		} else {
			chain = leaf = k;
		}
	}
	pctx->leaf = leaf;
	return chain;
}

static int
p_line()
{
	const char *b = pctx->buff;
	const char *p;
	char *f = pctx->func;
	char *k = pctx->keys;
	char *a = pctx->args;
	char *q;
	int items = 0;
	size_t len;

	while (*b && isblank(*b))
		b++;
	if (!*b || *b == '#')
		return items;
	*f = *k = *a = '\0';
	for (p = b; isalnum(*p); p++) ;
	if (*p == ':')
		/* can't handle keymodes yet */
		return items;
	if (!(p = strchr(b, ':')))
		return items;
	if ((len = p - b - 1) <= 0)
		return items;
	strncpy(k, b, len);
	for (q = k + len - 1; q >= k && isblank(*q); *q-- = '\0') ;
	p++;
	if (!isalnum(*p))
		return items;
	for (q = f; *p && isalnum(*p); *q++ = *p++) ;
	*q = '\0';
	for (; *p && isblank(*p); p++) ;
	for (q = q; *p && *p != '\n'; *q++ = *p++) ;
	*q = '\0';
	if (*k)
		items++;
	if (*f)
		items++;
	if (*a)
		items++;
	return items;
}

static void
p_file()
{
	while (p_getline()) {
		const KeyItem *item;
		Key *k;

		if (p_line() < 2)
			continue;
		for (item = KeyItems; item->name; item++)
			if (strcasecmp(pctx->func, item->name))
				break;
		if (!item->name || !item->parse)
			continue;
		if ((k = p_key(pctx->keys))) {
			(*item->parse) (pctx->leaf);
			addchain(k);
		}
	}
}

static void
initkeys_FLUXBOX(Bool reload)
{
	ParserContext ctx;
	const char *keyFile, *home = getenv("HOME") ? : "/";
	char *file = NULL;
	size_t len;

	keyFile = (session.keyFile && *session.keyFile) ? session.keyFile : "keys";
	if (!ctx.f) {
		if (*keyFile == '~') {
			len = strlen(home) + strlen(keyFile);
			file = ecalloc(len, sizeof(*file));
			strcpy(file, home);
			strcat(file, keyFile + 1);
			if (!(ctx.f = fopen(file, "r"))) {
				DPRINTF("%s: %s\n", file, strerror(errno));
				free(file);
				file = NULL;
			}
		}
	}
	if (!ctx.f) {
		if (*keyFile == '/') {
			len = strlen(keyFile) + 1;
			file = ecalloc(len, sizeof(*file));
			strcpy(file, keyFile);
			if (!(ctx.f = fopen(file, "r"))) {
				DPRINTF("%s: %s\n", file, strerror(errno));
				free(file);
				file = NULL;
			}
		}
	}
	if (!ctx.f) {
		len = strlen(config.pdir) + 1 + strlen(keyFile) + 1;
		file = ecalloc(len, sizeof(*file));
		strcpy(file, config.pdir);
		strcat(file, "/");
		strcat(file, keyFile);
		if (!(ctx.f = fopen(file, "r"))) {
			DPRINTF("%s: %s\n", file, strerror(errno));
			free(file);
			file = NULL;
		}
	}

	if (!ctx.f) {
		len = strlen(config.udir) + 1 + strlen(keyFile) + 1;
		file = ecalloc(len, sizeof(*file));
		strcpy(file, config.udir);
		strcat(file, "/");
		strcat(file, keyFile);
		if (!(ctx.f = fopen(file, "r"))) {
			DPRINTF("%s: %s\n", file, strerror(errno));
			free(file);
			file = NULL;
		}
	}
	if (!ctx.f) {
		len = strlen(config.sdir) + 1 + strlen(keyFile) + 1;
		file = ecalloc(len, sizeof(*file));
		strcpy(file, config.sdir);
		strcat(file, "/");
		strcat(file, keyFile);
		if (!(ctx.f = fopen(file, "r"))) {
			DPRINTF("%s: %s\n", file, strerror(errno));
			free(file);
			return;
		}
	}
	{
		ctx.buff = ecalloc(4 * (PATH_MAX + 1), sizeof(*ctx.buff));
		ctx.func = ctx.buff + PATH_MAX + 1;
		ctx.keys = ctx.func + PATH_MAX + 1;
		ctx.args = ctx.keys + PATH_MAX + 1;

		pctx = &ctx;

		p_file();

		pctx = NULL;

		free(ctx.buff);
	}
	fclose(ctx.f);
	free(file);
}

static void
initstyle_FLUXBOX(Bool reload)
{
	const char *res;
	FluxboxStyle *style;

	xresdb = xstyledb;
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
	.clas = "Fluxbox",
	.initrcfile = &initrcfile_FLUXBOX,
	.initconfig = &initconfig_FLUXBOX,
	.initkeys = &initkeys_FLUXBOX,
	.initstyle = &initstyle_FLUXBOX,
	.deinitstyle = &deinitstyle_FLUXBOX,
	.drawclient = &drawclient_FLUXBOX,
};

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
