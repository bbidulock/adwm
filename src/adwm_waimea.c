/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "resource.h"
#include "parse.h"

typedef enum {
	ButtonStateNone,
	ButtonStateMaximized,
	ButtonStateMinimized,
	ButtonStateShaded,
	ButtonStateSticky,
	ButtonStateAlwaysOnTop,
	ButtonStateAlwaysOnBottom,
	ButtonStateDecorTitle,
	ButtonStateDecorHandle,
	ButtonStateDecorBorder,
	ButtonStateDecorAll,
	ButtonStateFullscreen,
	ButtonStateClose,
} WaimeaButtonState;

typedef enum {
	AutoplaceFalse,
	AutoplaceLeft,
	AutoplaceRight,
} WaimeaAutoplace;

typedef struct {
	Texture frame;
	unsigned borderWidth;
	XColor borderColor;
} WaimeaDockStyle;

typedef struct {
	Bool foreground;		/* ? */
	WaimeaButtonState state;	/* None */
	WaimeaAutoplace autoplace;	/* False */
	int position;			/* */
	struct {
		Texture focus, unfocus, pressed;
	} true, false;

} WaimeaButtonStyle;

typedef struct {
	struct {
		struct {
			Texture focus, unfocus;
			unsigned height;
		} title, label, handle, button, grip;
		Justify justify;
		AdwmFont font;
		WaimeaButtonStyle buttons[12];
	} window;
	struct {
		struct {
			Texture appearance;
			Justify justify;
			AdwmFont font;
			unsigned height;
		} title, frame, hilite;
		struct {
			const char *look;
		} bullet;
		struct {
			struct {
				const char *look;
			} true, false;
		} checkbox;
		unsigned borderWidth;
		struct {
			unsigned height;
		} item;
	} menu;
	struct {
		WaimeaDockStyle dock[4];
	} dockappholder;
	unsigned borderWidth;
	XColor borderColor;
	XColor outlineColor;
	const char *rootCommand;
} WaimeaStyle;

typedef enum {
	StackingTypeNormal,
	StackingTypeAlwaysOnTop,
	StackingTypeAlwaysOnBottom,
} WaimeaStackingType;

typedef enum {
	RevertTypeWindow,
} WaimeaRevertType;

typedef enum {
	DirectionHorizontal,
	DirectionVertical,
} WaimeaDirection;

typedef struct {
	const char *geometry;
	const char *order;
	const char *desktopMask;	/* All */
	WaimeaDirection direction;	/* ? */
	Bool centered;
	unsigned gridSpace;
	WaimeaStackingType stacking;
	Bool inworkspace;		/* False */
} WaimeaDock;

typedef struct {
	const char *styleFile;		/* /usr/share/waimea/styles/Default */
	const char *menuFile;		/* /usr/share/waimea/menu */
	const char *actionFile;		/* /usr/share/waimea/actions/action */
	unsigned numberOfDesktops;	/* 4 */
	const char *desktopNames;	/* */
	Bool doubleBufferedText;	/* True */
	Bool lazyTransparency;		/* False */
	unsigned colorsPerChannel;	/* 4 */
	unsigned cacheMax;		/* 200 */
	Bool imageDither;		/* True */
	const char *virtualSize;	/* 3x3 */
	WaimeaStackingType menuStacking;	/* Normal */
	Bool transientAbove;		/* True */
	WaimeaRevertType focusRevertTo;	/* Window */
	WaimeaDock *dock;
} WaimeaScreen;

typedef struct {
	const char *screenMask;
	const char *scriptDir;
	unsigned doubleClickInterval;
	WaimeaScreen *screens;
} WaimeaSession;

typedef struct {
	char *rcfile;			/* rcfile */
	char *udir;			/* user directory */
	char *pdir;			/* private directory */
	char *sdir;			/* system directory */
} WaimeaConfig;

static WaimeaStyle *styles = NULL;
static WaimeaConfig config;

static void
initrcfile_WAIMEA(const char *conf, Bool reload)
{
}

static XrmDatabase xconfigdb;
static XrmDatabase xstyledb;

static void
initconfig_WAIMEA(Bool reload)
{
	(void) config;
	(void) xconfigdb;
}

static void
initkeys_WAIMEA(Bool reload)
{
}

static void
initstyle_WAIMEA(Bool reload)
{
	const char *res;
	WaimeaStyle *style;
	int i;
	char name[256], clas[256];

	xresdb = xstyledb;
	if (!styles)
		styles = ecalloc(nscr, sizeof(*styles));
	style = styles + scr->screen;

	res = readres("window.title.height", "Window.Title.Height", "0");
	style->window.title.height = strtoul(res, NULL, 0);

	readtexture("window.title.focus", "Window.Title.Focus",
		    &style->window.title.focus, "white", "black");
	readtexture("window.title.unfocus", "Window.Title.Unfocus",
		    &style->window.title.unfocus, "black", "white");
	readtexture("window.label.focus", "Window.Label.Focus",
		    &style->window.label.focus, "white", "black");
	readtexture("window.label.unfocus", "Window.Label.Unfocus",
		    &style->window.label.unfocus, "black", "white");
	readtexture("window.handle.focus", "Window.Label.Focus",
		    &style->window.handle.focus, "white", "black");
	readtexture("window.handle.unfocus", "Window.Label.Unfocus",
		    &style->window.handle.unfocus, "black", "white");
	readtexture("window.button.focus", "Window.Label.Focus",
		    &style->window.button.focus, "white", "black");
	readtexture("window.button.unfocus", "Window.Label.Unfocus",
		    &style->window.button.unfocus, "black", "white");
	readtexture("window.grip.focus", "Window.Label.Focus",
		    &style->window.grip.focus, "white", "black");
	readtexture("window.grip.unfocus", "Window.Label.Unfocus",
		    &style->window.grip.unfocus, "black", "white");

	res = readres("window.justify", "Window.Justify", "left");
	style->window.justify = JustifyLeft;
	if (strcasestr(res, "left"))
		style->window.justify = JustifyLeft;
	else if (strcasestr(res, "right"))
		style->window.justify = JustifyRight;
	else if (strcasestr(res, "center"))
		style->window.justify = JustifyCenter;

	res = readres("window.font", "Window.Font", "sans 8");
	getfont(res, "sans 8", &style->window.font);

	for (i = 0; i < 12; i++) {
		snprintf(name, sizeof(name), "window.button%d.foreground", i);
		snprintf(clas, sizeof(clas), "Window.Button%d.Foreground", i);
		getbool(name, clas, NULL, False, &style->window.buttons[i].foreground);
		snprintf(name, sizeof(name), "window.button%d.state", i);
		snprintf(clas, sizeof(clas), "Window.Button%d.State", i);
		res = readres(name, clas, "None");
		style->window.buttons[i].state = ButtonStateNone;
		if (strcasestr(res, "maximized"))
			style->window.buttons[i].state = ButtonStateMaximized;
		else if (strcasestr(res, "minimized"))
			style->window.buttons[i].state = ButtonStateMinimized;
		else if (strcasestr(res, "shaded"))
			style->window.buttons[i].state = ButtonStateShaded;
		else if (strcasestr(res, "sticky"))
			style->window.buttons[i].state = ButtonStateSticky;
		else if (strcasestr(res, "alwaysontop"))
			style->window.buttons[i].state = ButtonStateAlwaysOnTop;
		else if (strcasestr(res, "alwaysonbottom"))
			style->window.buttons[i].state = ButtonStateAlwaysOnBottom;
		else if (strcasestr(res, "decortitle"))
			style->window.buttons[i].state = ButtonStateDecorTitle;
		else if (strcasestr(res, "decorhandle"))
			style->window.buttons[i].state = ButtonStateDecorHandle;
		else if (strcasestr(res, "decorborder"))
			style->window.buttons[i].state = ButtonStateDecorBorder;
		else if (strcasestr(res, "decorall"))
			style->window.buttons[i].state = ButtonStateDecorAll;
		else if (strcasestr(res, "fullscreen"))
			style->window.buttons[i].state = ButtonStateFullscreen;
		else if (strcasestr(res, "close"))
			style->window.buttons[i].state = ButtonStateClose;
		snprintf(name, sizeof(name), "window.button%d.autoplace", i);
		snprintf(clas, sizeof(clas), "Window.Button%d.Autoplace", i);
		res = readres(name, clas, "False");
		style->window.buttons[i].autoplace = AutoplaceFalse;
		if (strcasestr(res, "false"))
			style->window.buttons[i].autoplace = AutoplaceFalse;
		else if (strcasestr(res, "left"))
			style->window.buttons[i].autoplace = AutoplaceLeft;
		else if (strcasestr(res, "right"))
			style->window.buttons[i].autoplace = AutoplaceRight;
		snprintf(name, sizeof(name), "window.button%d.position", i);
		snprintf(clas, sizeof(clas), "Window.Button%d.Position", i);
		res = readres(name, clas, "0");
		style->window.buttons[i].position = strtol(res, NULL, 0);
		snprintf(name, sizeof(name), "window.button%d.true.focus", i);
		snprintf(clas, sizeof(clas), "Window.Button%d.True.Focus", i);
		readtexture(name, clas, &style->window.buttons[i].true.focus, "white",
			    "black");
		snprintf(name, sizeof(name), "window.button%d.false.focus", i);
		snprintf(clas, sizeof(clas), "Window.Button%d.False.Focus", i);
		readtexture(name, clas, &style->window.buttons[i].false.focus, "white",
			    "black");
		snprintf(name, sizeof(name), "window.button%d.true.unfocus", i);
		snprintf(clas, sizeof(clas), "Window.Button%d.True.Unfocus", i);
		readtexture(name, clas, &style->window.buttons[i].true.unfocus, "white",
			    "black");
		snprintf(name, sizeof(name), "window.button%d.false.unfocus", i);
		snprintf(clas, sizeof(clas), "Window.Button%d.False.Unfocus", i);
		readtexture(name, clas, &style->window.buttons[i].false.unfocus, "white",
			    "black");
		snprintf(name, sizeof(name), "window.button%d.true.pressed", i);
		snprintf(clas, sizeof(clas), "Window.Button%d.True.Pressed", i);
		readtexture(name, clas, &style->window.buttons[i].true.pressed, "white",
			    "black");
		snprintf(name, sizeof(name), "window.button%d.false.pressed", i);
		snprintf(clas, sizeof(clas), "Window.Button%d.False.Pressed", i);
		readtexture(name, clas, &style->window.buttons[i].false.pressed, "white",
			    "black");
	}
	readtexture("menu.title", "Menu.Title", &style->menu.title.appearance, "white",
		    "black");
	res = readres("menu.title.justify", "Menu.Title.Justify", "center");
	style->menu.title.justify = JustifyCenter;
	if (strcasestr(res, "center"))
		style->menu.title.justify = JustifyCenter;
	else if (strcasestr(res, "left"))
		style->menu.title.justify = JustifyLeft;
	else if (strcasestr(res, "right"))
		style->menu.title.justify = JustifyRight;
	res = readres("menu.title.font", "Menu.Title.Font", "sans 8");
	getfont(res, "sans 8", &style->menu.title.font);
	res = readres("menu.title.height", "Menu.Title.Height", "0");
	style->menu.title.height = strtoul(res, NULL, 0);

	readtexture("menu.frame", "Menu.Frame", &style->menu.frame.appearance, "white",
		    "black");
	res = readres("menu.frame.justify", "Menu.Frame.Justify", "center");
	style->menu.frame.justify = JustifyCenter;
	if (strcasestr(res, "center"))
		style->menu.frame.justify = JustifyCenter;
	else if (strcasestr(res, "left"))
		style->menu.frame.justify = JustifyLeft;
	else if (strcasestr(res, "right"))
		style->menu.frame.justify = JustifyRight;
	res = readres("menu.frame.font", "Menu.Frame.Font", "sans 8");
	getfont(res, "sans 8", &style->menu.frame.font);
	res = readres("menu.frame.height", "Menu.Frame.Height", "0");
	style->menu.frame.height = strtoul(res, NULL, 0);

	readtexture("menu.hilite", "Menu.Hilite", &style->menu.hilite.appearance, "white",
		    "black");
	res = readres("menu.hilite.justify", "Menu.Hilite.Justify", "center");
	style->menu.hilite.justify = JustifyCenter;
	if (strcasestr(res, "center"))
		style->menu.hilite.justify = JustifyCenter;
	else if (strcasestr(res, "left"))
		style->menu.hilite.justify = JustifyLeft;
	else if (strcasestr(res, "right"))
		style->menu.hilite.justify = JustifyRight;
	res = readres("menu.hilite.font", "Menu.Hilite.Font", "sans 8");
	getfont(res, "sans 8", &style->menu.hilite.font);
	res = readres("menu.hilite.height", "Menu.Hilite.Height", "0");
	style->menu.hilite.height = strtoul(res, NULL, 0);

	res = readres("menu.bullet.look", "Menu.Bullet.Look", ">");
	style->menu.bullet.look = res;
	res = readres("menu.checkbox.true.look", "Menu.Checkbox.True.Look", "X");
	style->menu.checkbox.true.look = res;
	res = readres("menu.checkbox.false.look", "Menu.Checkbox.False.Look", "_");
	style->menu.checkbox.false.look = res;
	res = readres("menu.borderWidth", "Menu.BorderWidth", "0");
	style->menu.borderWidth = strtoul(res, NULL, 0);
	res = readres("menu.item.height", "Menu.Item.Height", "0");
	style->menu.item.height = strtoul(res, NULL, 0);

	for (i = 0; i < 4; i++) {
		snprintf(name, sizeof(name), "dockappholder.dock%d.frame", i);
		snprintf(clas, sizeof(clas), "Dockappholder.Dock%d.Frame", i);
		readtexture(name, clas, &style->dockappholder.dock[i].frame, "white",
			    "black");
		snprintf(name, sizeof(name), "dockappholder.dock%d.borderWidth", i);
		snprintf(clas, sizeof(clas), "Dockappholder.Dock%d.BorderWidth", i);
		res = readres(name, clas, "0");
		style->dockappholder.dock[i].borderWidth = strtoul(res, NULL, 0);
		snprintf(name, sizeof(name), "dockappholder.dock%d.borderColor", i);
		snprintf(clas, sizeof(clas), "Dockappholder.Dock%d.borderColor", i);
		res = readres(name, clas, "black");
		getxcolor(res, "black", &style->dockappholder.dock[i].borderColor);
	}

	res = readres("borderWidth", "BorderWidth", "0");
	style->borderWidth = strtoul(res, NULL, 0);
	res = readres("borderColor", "BorderColor", "black");
	getxcolor(res, "black", &style->borderColor);
	res = readres("outlineColor", "OutlineColor", "black");
	getxcolor(res, "black", &style->outlineColor);
	res = readres("rootCommand", "RootCommand", NULL);
	style->rootCommand = res;
}

static void
deinitstyle_WAIMEA(void)
{
}

static void
drawclient_WAIMEA(Client *c)
{
}

AdwmOperations adwm_ops = {
	.name = "waimea",
	.clas = "Waimea",
	.initrcfile = &initrcfile_WAIMEA,
	.initconfig = &initconfig_WAIMEA,
	.initkeys = &initkeys_WAIMEA,
	.initstyle = &initstyle_WAIMEA,
	.deinitstyle = &deinitstyle_WAIMEA,
	.drawclient = &drawclient_WAIMEA,
};

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
