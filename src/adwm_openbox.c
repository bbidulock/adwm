/* See COPYING file for copyright and license details. */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
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

typedef struct {
	struct {
		XColor color;
		unsigned width;
	} border;
	struct {
		struct {
			XColor color;
			unsigned width;
		} border;
		struct {
			struct {
				Appearance bg;
				struct {
					struct {
						XftColor color;
					} text;
				} disabled;
				struct {
					XftColor color;
				} text;
			} active;
			Appearance bg;
			struct {
				struct {
					XftColor color;
				} text;
			} disabled;
			struct {
				XftColor color;
			} text;
			TextShadow font;
		} items;
		struct {
			unsigned x;
			unsigned y;
		} overlap;
		struct {
			XColor color;
			struct {
				unsigned height;
				unsigned width;
			} padding;
			unsigned width;
		} separator;
		struct {
			Appearance bg;
			struct {
				XftColor color;
				TextShadow font;
				Justify justify;
			} text;
		} title;
	} menu;
	struct {
		Appearance bg;
		struct {
			XColor color;
			unsigned width;
		} border;
		struct {
			Appearance bg;
			XColor color;	/* should be bg.color */
		} hilight, unhilight;
		struct {
			Appearance bg;
			struct {
				XftColor color;
				TextShadow font;
			} text;
		} label;
	} osd;
	struct {
		unsigned height;
		unsigned width;
	} padding;
	struct {			/* window */
		struct {		/* active, inactive */
			struct {	/* border, client */
				XColor color;
			} border, client;
			struct {	/* button */
				struct {	/* disabled, hover, pressed, unpressed */
					Appearance bg;
					struct {	/* image */
						XColor color;
					} image;
				} disabled, hover, pressed, unpressed;
				struct {	/* toggled */
					Appearance bg;
					struct {	/* image */
						XColor color;
					} image;
					struct {	/* hover, pressed, unpressed */
						Appearance bg;
						struct {
							XColor color;
						} image;
					} hover, pressed, unpressed;
				} toggled;
			} button;
			struct {	/* grip, handle */
				Appearance bg;
			} grip, handle;
			struct {	/* label */
				Appearance bg;
				struct {	/* text */
					XftColor color;
					TextShadow font;
				} text;
			} label;
			struct {	/* title */
				Appearance bg;
				struct {	/* separator */
					XColor color;
				} separator;
			} title;
		} active, inactive;
		struct {		/* client */
			struct {	/* padding */
				unsigned height;
				unsigned width;
			} padding;
		} client;
		struct {		/* handle */
			unsigned width;
		} handle;
		struct {		/* label */
			struct {
				Justify justify;
			} text;
		} label;
	} window;
	struct {
		struct {
			AdwmBitmap xbm;
			struct {
				AdwmBitmap xbm;
			} pressed, hover;
			struct {
				AdwmBitmap xbm;
				struct {
					AdwmBitmap xbm;
				} pressed, hover;
			} toggled;
		} max, desk, shade;
		struct {
			AdwmBitmap xbm;
			struct {
				AdwmBitmap xbm;
			} pressed, disabled, hover;
		} iconify, close;
		struct {
			AdwmBitmap xbm;
		} bullet;
	} image;
} OpenboxStyle;

typedef struct {
	char *rcfile;			/* rcfile */
	char *udir;			/* user directory */
	char *pdir;			/* private directory */
	char *sdir;			/* system directory */
} OpenboxConfig;

/*
 *
 * Geometry:
 *
 * border.width
 * menu.border.width
 * menu.separator.width
 * menu.separator.padding.width
 * menu.separator.padding.height
 * osd.border.width
 * window.client.padding.width
 * window.client.padding.height
 * window.handle.width
 * padding.width
 * padding.height
 * menu.overlap.x
 * menu.overlap.y
 * menu.overlap
 *
 * Border colors:
 *
 * window.active.border.color
 * window.active.title.separator.color
 * window.inactive.border.color
 * window.inactive.title.separator.color
 * border.color
 * window.active.client.color
 * window.inactive.client.color
 * menu.border.color
 * osd.border.color
 *
 * Titlebar colors:
 *
 * window.active.label.text.color
 * window.inactive.lable.text.color
 * window.active.button.unpressed.image.color
 * window.active.button.pressed.image.color
 * window.active.button.disabled.image.color
 * window.active.button.hover.image.color
 * window.active.button.toggled.unpressed.image.color
 * window.active.button.toggled.pressed.image.color
 * window.active.button.toggled.hover.image.color
 * window.active.button.toggled.image.color
 * window.inactive.button.unpressed.image.color
 * window.inactive.button.pressed.image.color
 * window.inactive.button.disabled.image.color
 * window.inactive.button.hover.image.color
 * window.inactive.button.toggled.unpressed.image.color
 * window.inactive.button.toggled.pressed.image.color
 * window.inactive.button.toggled.hover.image.color
 * window.inactive.button.toggled.image.color
 *
 * Window textures:
 *
 * window.active.title.bg
 * window.active.label.bg
 * window.active.handle.bg
 * window.active.grip.bg
 * window.active.button.unpressed.bg
 * window.active.button.pressed.bg
 * window.active.button.hover.bg
 * window.active.button.disabled.bg
 * window.active.button.toggled.unpressed.bg
 * window.active.button.toggled.pressed.bg
 * window.active.button.toggled.hover.bg
 * window.active.button.toggled.bg
 * window.inactive.title.bg
 * window.inactive.label.bg
 * window.inactive.handle.bg
 * window.inactive.grip.bg
 * window.inactive.button.unpressed.bg
 * window.inactive.button.pressed.bg
 * window.inactive.button.hover.bg
 * window.inactive.button.disabled.bg
 * window.inactive.button.toggled.unpressed.bg
 * window.inactive.button.toggled.pressed.bg
 * window.inactive.button.toggled.hover.bg
 * window.inactive.button.toggled.bg
 *
 * Menu colors:
 *
 * menu.title.text.color
 * menu.items.text.color
 * menu.items.disabled.text.color
 * menu.items.active.text.color
 * menu.items.active.disabled.text.color
 * menu.separator.color
 *
 * Menu textures:
 *
 * menu.items.bg
 * menu.items.active.bg
 * menu.title.bg
 *
 * OSD colors:
 *
 * osd.label.text.color
 * osd.hilight.bg.color
 * osd.unhilight.bg.color
 *
 * OSD textures:
 *
 * osd.bg
 * osd.label.bg
 * osd.hilight.bg
 * osd.unhilight.bg
 *
 * Text justification:
 *
 * window.label.text.justify
 * menu.title.text.justify
 *
 * Text shadows:
 *
 * window.active.label.text.font
 * window.inactive.label.text.font
 * menu.items.font
 * menu.title.text.font
 * osd.label.text.font
 *
 * Button images:
 *
 * max.xbm
 * max_toggled.xbm
 * max_pressed.xbm
 * max_hover.xbm
 * max_toggled_pressed.xbm
 * max_toggled_hover.xbm
 *
 * iconify.xbm
 * iconify_pressed.xbm
 * iconify_disabled.xbm
 * iconify_hover.xbm
 *
 * close.xbm
 * close_pressed.xbm
 * close_disabled.xbm
 * close_hover.xbm
 *
 * desk.xbm
 * desk_toggled.xbm
 * desk_pressed.xbm
 * desk_disabled.xbm
 * desk_hover.xbm
 * desk_toggled_pressed.xbm
 * desk_toggled_hover.xbm
 *
 * shade.xbm
 * shade_toggled.xbm
 * shade_pressed.xbm
 * shade_disabled.xbm
 * shade_hover.xbm
 * shade_toggled_pressed.xbm
 * shade_toggled_hover.xbm
 *
 * Submenu bullet:
 *
 * bullet.xbm
 *
 *
 */

#define bullet_width 4
#define bullet_height 7
static unsigned char bullet_bits[] = {
	0x01, 0x03, 0x07, 0x0f, 0x07, 0x03, 0x01
};

#define close_width 6
#define close_height 6
static unsigned char close_bits[] = {
	0x33, 0x3f, 0x1e, 0x1e, 0x3f, 0x33
};

#define desk_width 6
#define desk_height 6
static unsigned char desk_bits[] = {
	0x33, 0x33, 0x00, 0x00, 0x33, 0x33
};

#define desk_toggled_width 6
#define desk_toggled_height 6
static unsigned char desk_toggled_bits[] = {
	0x00, 0x1e, 0x1a, 0x16, 0x1e, 0x00
};

#define iconify_width 6
#define iconify_height 6
static unsigned char iconify_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x3f, 0x3f
};

#define max_width 6
#define max_height 6
static unsigned char max_bits[] = {
	0x3f, 0x3f, 0x21, 0x21, 0x21, 0x3f
};

#define max_toggled_width 6
#define max_toggled_height 6
static unsigned char max_toggled_bits[] = {
	0x3e, 0x22, 0x2f, 0x29, 0x39, 0x0f
};

#define shade_width 6
#define shade_height 6
static unsigned char shade_bits[] = {
	0x3f, 0x3f, 0x00, 0x00, 0x00, 0x00
};

#define shade_toggled_width 6
#define shade_toggled_height 6
static unsigned char shade_toggled_bits[] = {
	0x3f, 0x3f, 0x00, 0x00, 0x00, 0x00
};

static void
initkeys_OPENBOX(void)
{
}

static OpenboxStyle *styles = NULL;
static OpenboxConfig config;

/** @brief Initialize configuration ala openbox(1).
  *
  * Openbox takes a command such as:
  *
  *   openbox [--config-file RCFILE]
  *
  * When RCFILE is not specified, $XDG_CONFIG_HOME/openbox/rc.xml is used.  The
  * locations of other openbox configuration files are specified by the initial
  * configuration file, but are typically placed under ~/.config/optnbox.
  * System files are placed under /usr/share/openbox.  Themes are located under
  * /usr/share/themes.
  */
static void
initrcfile_OPENBOX()
{
	const char *home = getenv("HOME") ? : ".";
	const char *cnfg = getenv("XDG_CONFIG_HOME");
	const char *sufx = "/openbox/rc.xml";
	const char *cdir = "/.config";
	const char *file = NULL;
	char *pos;
	unsigned int i, len;

	for (i = 0; i < cargc - 1; i++)
		if (!strcmp(cargv[i], "--config-file"))
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
		if (cnfg) {
			len = strlen(cnfg) + strlen(sufx) + 1;
			config.rcfile = ecalloc(len, sizeof(*config.rcfile));
			strcpy(config.rcfile, cnfg);
		} else {
			len = strlen(home) + strlen(cdir) + strlen(sufx) + 1;
			config.rcfile = ecalloc(len, sizeof(*config.rcfile));
			strcpy(config.rcfile, home);
			strcat(config.rcfile, cdir);
		}
		strcat(config.rcfile, sufx);
	}
	free(config.pdir);
	config.pdir = strdup(config.rcfile);
	if ((pos = strrchr(config.pdir, '/')))
		*pos = '\0';
	free(config.udir);
	if (cnfg) {
		len = strlen(cnfg) + strlen(sufx) + 1;
		config.udir = ecalloc(len, sizeof(*config.udir));
		strcpy(config.udir, cnfg);
	} else {
		len = strlen(home) + strlen(cdir) + strlen(sufx) + 1;
		config.udir = ecalloc(len, sizeof(*config.udir));
		strcpy(config.udir, home);
		strcat(config.udir, cdir);
	}
	strcat(config.udir, sufx);
	*strrchr(config.udir, '/') = '\0';
	free(config.sdir);
	config.sdir = strdup("/usr/share/openbox");
}

static void
initconfig_OPENBOX(void)
{
}

static void
initstyles(const char *stylename)
{
	const char *themes = "/themes/";
	const char *suffix = "/openbox-3/themerc";
	const char *hdir, *ddir, *home;
	char *dirs, *dir, *end;

	if ((ddir = getenv("XDG_DATA_DIRS")))
		ddir = "/usr/local/share:/usr/share";
	if ((hdir = getenv("XDG_DATA_HOME"))) {
		dirs = ecalloc(strlen(hdir) + strlen(ddir) + 2, sizeof(*dirs));
		strcpy(dirs, hdir);
	} else {
		home = getenv("HOME");
		hdir = "/.local/share";
		dirs = ecalloc(strlen(home) + strlen(hdir) + strlen(ddir) + 2, sizeof(*dirs));
		strcpy(dirs, home);
		strcat(dirs, hdir);
	}
	strcat(dirs, ":");
	strcat(dirs, ddir);
	end = dirs + strlen(dirs);

	for (dir = dirs; dir && dir < end; dir += strlen(dir) + 1) {
		size_t plen;
		struct stat st;
		char *path;

		*strchrnul(dir, ':') = '\0';
		plen =
		    strlen(dir) + strlen(themes) + 12 + strlen(stylename) +
		    strlen(suffix) + 2;
		path = ecalloc(plen, sizeof(*path));
		strcpy(path, dir);
		strcat(path, themes);
		strcat(path, stylename);
		strcat(path, suffix);
		if (stat(path, &st)) {
			DPRINTF("%s: %s\n", strerror(errno), path);
			free(path);
			continue;
		}
		if (!S_ISREG(st.st_mode)) {
			DPRINTF("not regular file: %s\n", path);
			free(path);
			continue;
		}
		if (access(path, R_OK)) {
			DPRINTF("cannot access: %s\n", path);
			free(path);
			continue;
		}
		if (!(xresdb = XrmGetFileDatabase(path))) {
			DPRINTF("cannot get file database: %s\n", path);
			free(path);
			continue;
		}
	}

	styles = ecalloc(nscr, sizeof(*styles));
}

static void
initstyle_OPENBOX()
{
	const char *res;
	OpenboxStyle *style;

	/* Note: called once per managed screen */

	if (!styles)
		/* FIXME: look up style */
		initstyles("Squared-blue");
	style = styles + scr->screen;

	getbitmap(bullet_bits, bullet_width, bullet_height, &style->image.bullet.xbm);
	getbitmap(close_bits, close_width, close_height, &style->image.close.xbm);
	getbitmap(desk_bits, desk_width, desk_height, &style->image.desk.xbm);
	getbitmap(desk_toggled_bits, desk_toggled_width, desk_toggled_height,
		  &style->image.desk.toggled.xbm);
	getbitmap(iconify_bits, iconify_width, iconify_height, &style->image.iconify.xbm);
	getbitmap(max_bits, max_width, max_height, &style->image.max.xbm);
	getbitmap(max_toggled_bits, max_toggled_width, max_toggled_height,
		  &style->image.max.toggled.xbm);
	getbitmap(shade_bits, shade_width, shade_height, &style->image.shade.xbm);
	getbitmap(shade_toggled_bits, shade_toggled_width, shade_toggled_height,
		  &style->image.shade.toggled.xbm);

	res = readres("border.color", "Border.Color", "black");
	getxcolor(res, "black", &style->border.color);
	res = readres("border.width", "Border.Width", "1");
	style->border.width = strtoul(res, NULL, 0);

	res = readres("padding.height", "Padding.Height", "0");
	style->padding.height = strtoul(res, NULL, 0);
	res = readres("padding.width", "Padding.Width", "0");
	style->padding.width = strtoul(res, NULL, 0);

	res = readres("window.active.border.color",
		      "Window.Active.Border.Color", "black");
	getxcolor(res, "black", &style->window.active.border.color);
	res = readres("window.active.client.color",
		      "Window.Active.Client.Color", "black");
	getxcolor(res, "black", &style->window.active.client.color);

	res = readres("window.active.button.disabled.bg",
		      "Window.Active.Button.Disabled.Bg", "flat solid");
	getappearance(res, &style->window.active.button.disabled.bg);
	res = readres("window.active.button.disabled.image.color",
		      "Window.Active.Button.Disabled.Image.Color", "gray");
	getxcolor(res, "gray", &style->window.active.button.disabled.image.color);

	res = readres("window.active.button.hover.bg",
		      "Window.Active.Button.Hover.Bg", "flat solid");
	getappearance(res, &style->window.active.button.hover.bg);
	res = readres("window.active.button.hover.image.color",
		      "Window.Active.Button.Hover.Image.Color", "gray");
	getxcolor(res, "gray", &style->window.active.button.hover.image.color);

	res = readres("window.active.button.pressed.bg",
		      "Window.Active.Button.Pressed.Bg", "flat solid");
	getappearance(res, &style->window.active.button.pressed.bg);
	res = readres("window.active.button.pressed.image.color",
		      "Window.Active.Button.Pressed.Image.Color", "gray");
	getxcolor(res, "gray", &style->window.active.button.pressed.image.color);

	res = readres("window.active.button.unpressed.bg",
		      "Window.Active.Button.Unpressed.Bg", "flat solid");
	getappearance(res, &style->window.active.button.unpressed.bg);
	res = readres("window.active.button.unpressed.image.color",
		      "Window.Active.Button.Unpressed.Image.Color", "gray");
	getxcolor(res, "gray", &style->window.active.button.unpressed.image.color);

	res = readres("window.active.button.toggled.bg",
		      "Window.Active.Button.Toggled.Bg", "flat solid");
	getappearance(res, &style->window.active.button.toggled.bg);
	res = readres("window.active.button.toggled.image.color",
		      "Window.Active.Button.Toggled.Image.Color", "white");
	getxcolor(res, "white", &style->window.active.button.toggled.image.color);

	res = readres("window.active.button.toggled.hover.bg",
		      "Window.Active.Button.Toggled.Hover.Bg", "flat solid");
	getappearance(res, &style->window.active.button.toggled.hover.bg);
	res = readres("window.active.button.toggled.hover.image.color",
		      "Window.Active.Button.Toggled.Hover.Image.Color", "white");
	getxcolor(res, "white", &style->window.active.button.toggled.hover.image.color);

	res = readres("window.active.button.toggled.pressed.bg",
		      "Window.Active.Button.Toggled.Pressed.Bg", "flat solid");
	getappearance(res, &style->window.active.button.toggled.pressed.bg);
	res = readres("window.active.button.toggled.pressed.image.color",
		      "Window.Active.Button.Toggled.Pressed.Image.Color", "white");
	getxcolor(res, "white", &style->window.active.button.toggled.pressed.image.color);

	res = readres("window.active.button.toggled.unpressed.bg",
		      "Window.Active.Button.Toggled.Unpressed.Bg", "flat solid");
	getappearance(res, &style->window.active.button.toggled.unpressed.bg);
	res = readres("window.active.button.toggled.unpressed.image.color",
		      "Window.Active.Button.Toggled.Unpressed.Image.Color", "white");
	getxcolor(res, "white",
		  &style->window.active.button.toggled.unpressed.image.color);

	res = readres("window.active.grip.bg", "Window.Active.Grip.Bg", "flat solid");
	getappearance(res, &style->window.active.grip.bg);
	res = readres("window.active.handle.bg", "Window.Active.Handle.Bg", "flat solid");
	getappearance(res, &style->window.active.handle.bg);

	res = readres("window.active.label.bg", "Window.Active.Label.Bg", "flat solid");
	getappearance(res, &style->window.active.label.bg);
	res = readres("window.active.label.text.color",
		      "Window.Active.Label.Text.Color", "white");
	getxftcolor(res, "white", &style->window.active.label.text.color);
	res = readres("window.active.label.text.font",
		      "Window.Active.Label.Text.Font", "");
	getshadow(res, &style->window.active.label.text.font);

	res = readres("window.active.title.bg", "Window.Active.Title.Bg", "flat solid");
	getappearance(res, &style->window.active.title.bg);
	res = readres("window.active.title.separator.color",
		      "Window.Active.Title.Separator.Color", "white");
	getxcolor(res, "white", &style->window.active.title.separator.color);

	res = readres("window.inactive.border.color",
		      "Window.Inactive.Border.Color", "black");
	getxcolor(res, "black", &style->window.inactive.border.color);
	res = readres("window.inactive.client.color",
		      "Window.Inactive.Client.Color", "black");
	getxcolor(res, "black", &style->window.inactive.client.color);

	res = readres("window.inactive.button.disabled.bg",
		      "Window.Inactive.Button.Disabled.Bg", "flat solid");
	getappearance(res, &style->window.inactive.button.disabled.bg);
	res = readres("window.inactive.button.disabled.image.color",
		      "Window.Inactive.Button.Disabled.Image.Color", "gray");
	getxcolor(res, "gray", &style->window.inactive.button.disabled.image.color);

	res = readres("window.inactive.button.hover.bg",
		      "Window.Inactive.Button.Hover.Bg", "flat solid");
	getappearance(res, &style->window.inactive.button.hover.bg);
	res = readres("window.inactive.button.hover.image.color",
		      "Window.Inactive.Button.Hover.Image.Color", "gray");
	getxcolor(res, "gray", &style->window.inactive.button.hover.image.color);

	res = readres("window.inactive.button.pressed.bg",
		      "Window.Inactive.Button.Pressed.Bg", "flat solid");
	getappearance(res, &style->window.inactive.button.pressed.bg);
	res = readres("window.inactive.button.pressed.image.color",
		      "Window.Inactive.Button.Pressed.Image.Color", "gray");
	getxcolor(res, "gray", &style->window.inactive.button.pressed.image.color);

	res = readres("window.inactive.button.unpressed.bg",
		      "Window.Inactive.Button.Unpressed.Bg", "flat solid");
	getappearance(res, &style->window.inactive.button.unpressed.bg);
	res = readres("window.inactive.button.unpressed.image.color",
		      "Window.Inactive.Button.Unpressed.Image.Color", "gray");
	getxcolor(res, "gray", &style->window.inactive.button.unpressed.image.color);

	res = readres("window.inactive.button.toggled.bg",
		      "Window.Inactive.Button.Toggled.Bg", "flat solid");
	getappearance(res, &style->window.inactive.button.toggled.bg);
	res = readres("window.inactive.button.toggled.image.color",
		      "Window.Inactive.Button.Toggled.Image.Color", "white");
	getxcolor(res, "white", &style->window.inactive.button.toggled.image.color);

	res = readres("window.inactive.button.toggled.hover.bg",
		      "Window.Inactive.Button.Toggled.Hover.Bg", "flat solid");
	getappearance(res, &style->window.inactive.button.toggled.hover.bg);
	res = readres("window.inactive.button.toggled.hover.image.color",
		      "Window.Inactive.Button.Toggled.Hover.Image.Color", "white");
	getxcolor(res, "white", &style->window.inactive.button.toggled.hover.image.color);

	res = readres("window.inactive.button.toggled.pressed.bg",
		      "Window.Inactive.Button.Toggled.Pressed.Bg", "flat solid");
	getappearance(res, &style->window.inactive.button.toggled.pressed.bg);
	res = readres("window.inactive.button.toggled.pressed.image.color",
		      "Window.Inactive.Button.Toggled.Pressed.Image.Color", "white");
	getxcolor(res, "white",
		  &style->window.inactive.button.toggled.pressed.image.color);

	res = readres("window.inactive.button.toggled.unpressed.bg",
		      "Window.Inactive.Button.Toggled.Unpressed.Bg", "flat solid");
	getappearance(res, &style->window.inactive.button.toggled.unpressed.bg);
	res = readres("window.inactive.button.toggled.unpressed.image.color",
		      "Window.Inactive.Button.Toggled.Unpressed.Image.Color", "white");
	getxcolor(res, "white",
		  &style->window.inactive.button.toggled.unpressed.image.color);

	res = readres("window.inactive.grip.bg", "Window.Inactive.Grip.Bg", "flat solid");
	getappearance(res, &style->window.inactive.grip.bg);
	res = readres("window.inactive.handle.bg",
		      "Window.Inactive.Handle.Bg", "flat solid");
	getappearance(res, &style->window.inactive.handle.bg);

	res = readres("window.inactive.label.bg",
		      "Window.Inactive.Label.Bg", "flat solid");
	getappearance(res, &style->window.inactive.label.bg);
	res = readres("window.inactive.label.text.color",
		      "Window.Inactive.Label.Text.Color", "white");
	getxftcolor(res, "white", &style->window.inactive.label.text.color);
	res = readres("window.inactive.label.text.font",
		      "Window.Inactive.Label.Text.Font", "");
	getshadow(res, &style->window.inactive.label.text.font);

	res = readres("window.inactive.title.bg",
		      "Window.Inactive.Title.Bg", "flat solid");
	getappearance(res, &style->window.inactive.title.bg);
	res = readres("window.inactive.title.separator.color",
		      "Window.Inactive.Title.Separator.Color", "white");
	getxcolor(res, "white", &style->window.inactive.title.separator.color);

	res = readres("window.client.padding.height",
		      "Window.Client.Padding.Height", "0");
	style->window.client.padding.height = strtoul(res, NULL, 0);
	res = readres("window.client.padding.width", "Window.Client.Padding.Width", "0");
	style->window.client.padding.width = strtoul(res, NULL, 0);
	res = readres("window.handle.width", "Window.Handle.Width", "2");
	style->window.handle.width = strtoul(res, NULL, 0);
	res = readres("window.label.text.justify", "Window.Label.Text.Justify", "left");
	style->window.label.text.justify = JustifyLeft;
	if (strcasestr(res, "left"))
		style->window.label.text.justify = JustifyLeft;
	if (strcasestr(res, "center"))
		style->window.label.text.justify = JustifyCenter;
	if (strcasestr(res, "right"))
		style->window.label.text.justify = JustifyRight;

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
	.name = "openbox",
	.clas = "Openbox",
	.initrcfile = &initrcfile_OPENBOX,
	.initconfig = &initconfig_OPENBOX,
	.initkeys = &initkeys_OPENBOX,
	.initstyle = &initstyle_OPENBOX,
	.deinitstyle = &deinitstyle_OPENBOX,
	.drawclient = &drawclient_OPENBOX,
};

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
