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
#ifdef IMLIB2
#include <Imlib2.h>
#endif
#ifdef XPM
#include <X11/xpm.h>
#endif
#include "adwm.h"
#include "resource.h"
#include "parse.h"

/*
 * The purpose of this file is to provide a loadable module that provides
 * functions for providing the behaviour specified to adwm.  This module reads
 * the adwm configuration file to determine configuration information, and
 * access an adwm(1) style file.  It also reads the adwm(1) keys configuration
 * file to find key bindings.
 *
 * This module is really just intenteded to provide the default adwm(1)
 * behaviour as a separate loadable module (because it is not needed when
 * adwm(1) is mimicing the behaviour of another window manager).
 */

typedef struct {
	char *rcfile;			/* rcfile */
	char *udir;			/* user directory */
	char *pdir;			/* private directory */
	char *sdir;			/* system directory */
} AdwmConfig;

static AdwmConfig config;

static XrmDatabase xconfigdb;
static XrmDatabase xstyledb;
static XrmDatabase xkeysdb;

typedef struct {
	const char *layout;
	const char *name;
} AdwmView;

typedef struct {
	Bool attachaside;
	const char *command;
	Bool decoratetiled;
	Bool decoratemax;
	Bool hidebastards;
	Bool autoroll;
	Bool sloppy;
	unsigned snap;
	struct {
		unsigned position;
		unsigned orient;
	} dock;
	unsigned dragdistance;
	double mwfact;
	double mhfact;
	unsigned ncolumns;
	const char *deflayout;
	struct {
		unsigned number;
	} tags;
	AdwmView *views;
} AdwmSession;

static AdwmSession session;

typedef struct {
	struct {
		struct {
			XColor border;
			XColor bg;
			XColor fg;
			XColor button;
			XftColor font;
			XftColor shadow;
		} color;
		AdwmFont font;
		unsigned drop;

	} normal, selected;
	unsigned border;
	unsigned margin;
	unsigned opacity;
	Bool outline;
	unsigned spacing;
	const char *titlelayout;
	unsigned grips;
	unsigned title;
	Texture *elements;
} AdwmStyle;

static AdwmStyle *styles;

static void
initrcfile_ADWM(void)
{
	const char *home = getenv("HOME") ? : ".";
	const char *file = NULL;
	char *pos;
	int i, len;
	char *owd;
	struct stat st;

	/* init resource database */
	XrmInitialize();

	owd = calloc(PATH_MAX, sizeof(*owd));
	if (!getcwd(owd, PATH_MAX))
		strcpy(owd, "/");

	for (i = 0; i < cargc - 1; i++)
		if (!strcmp(cargv[i], "-f"))
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
		len = strlen(home) + strlen("/.adwm/adwmrc") + 1;
		config.rcfile = ecalloc(len, sizeof(*config.rcfile));
		strcpy(config.rcfile, home);
		strcat(config.rcfile, "/.adwm/adwmrc");
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
	config.udir = ecalloc(strlen(home) + strlen("/.adwm") + 1, sizeof(*config.udir));
	strcpy(config.udir, home);
	strcat(config.udir, "/.adwm");
	free(config.sdir);
	config.sdir = strdup("/usr/share/adwm");
	if (!strncmp(home, config.pdir, strlen(home))) {
		free(config.pdir);
		config.pdir = strdup(config.udir);
	}

	xconfigdb = XrmGetFileDatabase(config.rcfile);
	if (!xconfigdb) {
		free(config.rcfile);
		len = strlen(config.pdir) + strlen("/adwmrc") + 2;
		config.rcfile = ecalloc(len, sizeof(*config.rcfile));
		strcpy(config.rcfile, config.pdir);
		strcat(config.rcfile, "/adwmrc");
		xconfigdb = XrmGetFileDatabase(config.rcfile);
	}
	if (!xconfigdb) {
		free(config.rcfile);
		len = strlen(config.udir) + strlen("/adwmrc") + 2;
		config.rcfile = ecalloc(len, sizeof(*config.rcfile));
		strcpy(config.rcfile, config.udir);
		strcat(config.rcfile, "/adwmrc");
		xconfigdb = XrmGetFileDatabase(config.rcfile);
	}
	if (!xconfigdb) {
		free(config.rcfile);
		len = strlen(config.sdir) + strlen("/adwmrc") + 2;
		config.rcfile = ecalloc(len, sizeof(*config.rcfile));
		strcpy(config.rcfile, config.sdir);
		strcat(config.rcfile, "/adwmrc");
		xconfigdb = XrmGetFileDatabase(config.rcfile);
	}
	if (!xconfigdb) {
		eprint("Could not find usable database\n");
		exit(EXIT_FAILURE);
	}

	styles = calloc(nscr, sizeof(*styles));
}

static void
initconfig_ADWM(void)
{
	const char *res;
	char name[256], clas[256], *n, *c;
	size_t nlen, clen;
	int i;
	unsigned num;

	static const char *def[] = {
		"1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
		"11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
		"21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
		"31", "32", "33", "34", "35", "36", "37", "38", "39", "40",
		"41", "42", "43", "44", "45", "46", "47", "48", "49", "50",
		"51", "52", "53", "54", "55", "56", "57", "58", "59", "60",
		"61", "62", "63", "64"
	};

	xresdb = xconfigdb;
	if (!session.views) {
		getbool("adwm.session.attachaside", "Adwm.Session.AttachASide", "1", True,
			&session.attachaside);
		res = readres("adwm.session.command", "Adwm.Session.Command", COMMAND);
		session.command = res;
		getbool("adwm.session.decoratetiled", "Adwm.Session.DecorateTiled", "1",
			False, &session.decoratetiled);
		getbool("adwm.session.decoratemax", "Adwm.Session.DecorateMax", "1", True,
			&session.decoratemax);
		getbool("adwm.session.hidebastards", "Adwm.Session.HideBastards", "1",
			False, &session.hidebastards);
		getbool("adwm.session.autoroll", "Adwm.Session.AutoRoll", "1", False,
			&session.autoroll);
		getbool("adwm.session.sloppy", "Adwm.Session.Sloppy", "1", False,
			&session.sloppy);
		res = readres("adwm.session.snap", "Adwm.Session.Snap", STR(SNAP));
		session.snap = strtoul(res, NULL, 0);
		res = readres("adwm.session.dock.position", "Adwm.Session.Dock.Position",
			      "1");
		session.dock.position = strtoul(res, NULL, 0);
		res = readres("adwm.session.dock.orient", "Adwm.Session.Dock.Orient",
			      "1");
		session.dock.orient = strtoul(res, NULL, 0);
		res = readres("adwm.session.dragdistance", "Adwm.Session.DragDistance",
			      "5");
		session.dragdistance = strtoul(res, NULL, 0);
		res = readres("adwm.session.mwfact", "Adwm.Session.Mwfact",
			      STR(DEFMWFACT));
		session.mwfact = strtod(res, NULL);
		res = readres("adwm.session.mhfact", "Adwm.Session.Mhfact",
			      STR(DEFMHFACT));
		session.mhfact = strtod(res, NULL);
		res = readres("adwm.session.ncolumns", "Adwm.Session.NColumns",
			      STR(DEFNCOLUMNS));
		session.ncolumns = strtoul(res, NULL, 0);
		res = readres("adwm.session.deflayout", "Adwm.Session.DefLayout", "i");
		session.deflayout = res;
		res =
		    readres("adwm.session.tags.number", "Adwm.Session.Tags.number", "5");
		session.tags.number = strtoul(res, NULL, 0);

		res =
		    readres("adwm.session.tags.number", "Adwm.Session.Tags.number", "5");
		num = strtoul(res, NULL, 0);
		if (num < 1)
			num = 1;
		if (num > MAXTAGS)
			num = MAXTAGS;
		session.tags.number = num;

		strncpy(name, "adwm.session.tags.", sizeof(name));
		nlen = strlen(name);
		n = name + nlen;
		nlen = sizeof(name) - nlen;
		strncpy(clas, "Adwm.Session.Tags.", sizeof(clas));
		clen = strlen(clas);
		c = clas + clen;
		clen = sizeof(clas) - clen;

		num = MAXTAGS;
		session.views = calloc(num, sizeof(*session.views));

		for (i = 0; i < num; i++) {
			snprintf(n, nlen, "layout%d", i);
			snprintf(c, clen, "Layout%d", i);
			res = readres(name, clas, session.deflayout);
			session.views[i].layout = res;

			snprintf(n, nlen, "name%d", i);
			snprintf(n, clen, "Name%d", i);
			res = readres(name, clas, def[i]);
			session.views[i].name = res;
		}
	}
}

typedef struct {
	const char *name;
	void (*action) (XEvent *e, Key *k);
} KeyItem;

static KeyItem KeyItems[] = {
	/* *INDENT-OFF* */
	{ "viewprevtag",	k_viewprevtag	 },
	{ "quit",		k_quit		 }, /* arg is new command */
	{ "restart",		k_restart	 }, /* arg is new command */
	{ "killclient",		k_killclient	 },
	{ "zoom",		k_zoom		 },
	{ "moveright",		k_moveresizekb	 }, /* arg is delta geometry */
	{ "moveleft",		k_moveresizekb	 }, /* arg is delta geometry */
	{ "moveup",		k_moveresizekb	 }, /* arg is delta geometry */
	{ "movedown",		k_moveresizekb	 }, /* arg is delta geometry */
	{ "resizedecx",		k_moveresizekb	 }, /* arg is delta geometry */
	{ "resizeincx",		k_moveresizekb	 }, /* arg is delta geometry */
	{ "resizedecy",		k_moveresizekb	 }, /* arg is delta geometry */
	{ "resizeincy",		k_moveresizekb	 }, /* arg is delta geometry */
	{ "togglemonitor",	k_togglemonitor	 },
	{ "appendtag",		k_appendtag	 },
	{ "rmlasttag",		k_rmlasttag	 },
	{ "rotateview",		k_rotateview	 },
	{ "unrotateview",	k_unrotateview	 },
	{ "rotatezone",		k_rotatezone	 },
	{ "unrotatezone",	k_unrotatezone	 },
	{ "rotatewins",		k_rotatewins	 },
	{ "unrotatewins",	k_unrotatewins	 },
	{ "raise",		k_raise		 },
	{ "lower",		k_lower		 },
	{ "raiselower",		k_raiselower	 }
	/* *INDENT-ON* */
};

static const struct {
	const char *prefix;
	ActionCount act;
} inc_prefix[] = {
	/* *INDENT-OFF* */
	{ "set", SetCount},	/* set count to arg or reset no arg */
	{ "inc", IncCount},	/* increment count by arg (or 1) */
	{ "dec", DecCount}	/* decrement count by arg (or 1) */
	/* *INDENT-ON* */
};

static KeyItem KeyItemsByAmt[] = {
	/* *INDENT-OFF* */
	{ "mwfact",		k_setmwfactor	 },
	{ "nmaster",		k_setnmaster	 },
	{ "ncolumns",		k_setnmaster	 },
	{ "margin",		k_setmargin	 },
	{ "border",		k_setborder	 }
	/* *INDENT-ON* */
};

static const struct {
	const char *prefix;
	FlagSetting set;
} set_prefix[] = {
	/* *INDENT-OFF* */
	{ "",		SetFlagSetting	    },
	{ "set",	SetFlagSetting	    },
	{ "un",		UnsetFlagSetting    },
	{ "de",		UnsetFlagSetting    },
	{ "unset",	UnsetFlagSetting    },
	{ "toggle",	ToggleFlagSetting   }
	/* *INDENT-ON* */
};

static const struct {
	const char *suffix;
	WhichClient any;
} set_suffix[] = {
	/* *INDENT-OFF* */
	{ "",		FocusClient	},
	{ "sel",	ActiveClient	},
	{ "ptr",	PointerClient	},
	{ "all",	AllClients	}, /* all on current monitor */
	{ "any",	AnyClient	}, /* clients on any monitor */
	{ "every",	EveryClient	}  /* clients on every workspace */
	/* *INDENT-ON* */
};

static KeyItem KeyItemsByState[] = {
	/* *INDENT-OFF* */
	{ "floating",		k_setfloating	 },
	{ "fill",		k_setfill	 },
	{ "full",		k_setfull	 },
	{ "max",		k_setmax	 },
	{ "maxv",		k_setmaxv	 },
	{ "maxh",		k_setmaxh	 },
	{ "shade",		k_setshade	 },
	{ "shaded",		k_setshade	 },
	{ "hide",		k_sethidden	 },
	{ "hidden",		k_sethidden	 },
	{ "iconify",		k_setmin	 },
	{ "min",		k_setmin	 },
	{ "above",		k_setabove	 },
	{ "below",		k_setbelow	 },
	{ "pager",		k_setpager	 },
	{ "taskbar",		k_settaskbar	 },
	{ "showing",		k_setshowing	 },
	{ "struts",		k_setstruts	 },
	{ "dectiled",		k_setdectiled	 }
	/* *INDENT-ON* */
};

static const struct {
	const char *suffix;
	RelativeDirection dir;
} rel_suffix[] = {
	/* *INDENT-OFF* */
	{ "NW",	RelativeNorthWest	},
	{ "N",	RelativeNorth		},
	{ "NE",	RelativeNorthEast	},
	{ "W",	RelativeWest		},
	{ "C",	RelativeCenter		},
	{ "E",	RelativeEast		},
	{ "SW",	RelativeSouthWest	},
	{ "S",	RelativeSouth		},
	{ "SE",	RelativeSouthEast	},
	{ "R",	RelativeStatic		},
	{ "L",	RelativeLast		}
	/* *INDENT-ON* */
};

static KeyItem KeyItemsByDir[] = {
	/* *INDENT-OFF* */
	{ "moveto",		k_moveto	}, /* arg is position */
	{ "snapto",		k_snapto	}, /* arg is direction */
	{ "edgeto",		k_edgeto	}, /* arg is direction */
	{ "moveby",		k_moveby	}  /* arg is direction and amount */
	/* *INDENT-ON* */
};

static const struct {
	const char *which;
	WhichClient any;
} list_which[] = {
	/* *INDENT-OFF* */
	{ "",		FocusClient	}, /* focusable, same monitor */
	{ "act",	ActiveClient	}, /* activatable (incl. icons), same monitor */
	{ "all",	AllClients	}, /* all clients (incl. icons), same mon */
	{ "any",	AnyClient	}, /* any client on any monitor */
	{ "every",	EveryClient	}  /* all clients, all desktops */
	/* *INDENT-ON* */
};

static const struct {
	const char *suffix;
	RelativeDirection dir;
} lst_suffix[] = {
	/* *INDENT-OFF* */
	{ "",		RelativeNone		},
	{ "icon",	RelativeCenter		},
	{ "next",	RelativeNext		},
	{ "prev",	RelativePrev		},
	{ "last",	RelativeLast		},
	{ "up",		RelativeNorth		},
	{ "down",	RelativeSouth		},
	{ "left",	RelativeWest		},
	{ "right",	RelativeEast		},
	{ "NW",		RelativeNorthWest	},
	{ "N",		RelativeNorth		},
	{ "NE",		RelativeNorthEast	},
	{ "W",		RelativeWest		},
	{ "E",		RelativeEast		},
	{ "SW",		RelativeSouthWest	},
	{ "S",		RelativeSouth		},
	{ "SE",		RelativeSouthEast	}
	/* *INDENT-ON* */
};

static const struct {
	const char *suffix;
	RelativeDirection dir;
} cyc_suffix[] = {
	/* *INDENT-OFF* */
	{ "icon",	RelativeCenter		},
	{ "next",	RelativeNext		},
	{ "prev",	RelativePrev		}
	/* *INDENT-ON* */
};

static KeyItem KeyItemsByList[] = {
	/* *INDENT-OFF* */
	{ "focus",		k_focus		},
	{ "client",		k_client	},
	{ "stack",		k_stack		},
	{ "group",		k_group		},
	{ "tab",		k_tab		},
	{ "panel",		k_panel		},
	{ "dock",		k_dock		},
	{ "swap",		k_swap		}
	/* *INDENT-ON* */
};

static const struct {
	const char *suffix;
	RelativeDirection dir;
	Bool wrap;
} tag_suffix[] = {
	/* *INDENT-OFF* */
	{ "",		RelativeNone,		False	},
	{ "next",	RelativeNext,		True	},
	{ "prev",	RelativePrev,		True	},
	{ "last",	RelativeLast,		False	},
	{ "up",		RelativeNorth,		False	},
	{ "down",	RelativeSouth,		False	},
	{ "left",	RelativeWest,		False	},
	{ "right",	RelativeEast,		False	},
	{ "NW",		RelativeNorthWest,	True	},
	{ "N",		RelativeNorth,		True	},
	{ "NE",		RelativeNorthEast,	True	},
	{ "W",		RelativeWest,		True	},
	{ "E",		RelativeEast,		True	},
	{ "SW",		RelativeSouthWest,	True	},
	{ "S",		RelativeSouth,		True	},
	{ "SE",		RelativeSouthEast,	True	}
	/* *INDENT-ON* */
};

static KeyItem KeyItemsByTag[] = {
	/* *INDENT-OFF* */
	{"view",		k_view		},
	{"toggleview",		k_toggleview	},
	{"focusview",		k_focusview	},
	{"tag",			k_tag		},
	{"toggletag",		k_toggletag	},
	{"taketo",		k_taketo	}
	/* *INDENT-ON* */
};

static void
initmodkey()
{
	const char *res;

	res = readres("adwm.modkey", "Adwm.Modkey", "A");
	switch (*res) {
	case 'S':
		modkey = ShiftMask;
		break;
	case 'C':
		modkey = ControlMask;
		break;
	case 'W':
		modkey = Mod4Mask;
		break;
	default:
		modkey = Mod1Mask;
	}
}

static char *
capclass(char *clas)
{
	char *p;

	for (p = clas; *p != '\0'; p = strchrnul(p, '.'), p += ((p == '\0') ? 0 : 1))
		*p = toupper(*p);
	return clas;
}

static void
initkeys_ADWM(void)
{
	unsigned int i, j, l;
	const char *res;
	char name[256], clas[256];

	xresdb = xkeysdb;

	initmodkey();
	scr->keys = ecalloc(LENGTH(KeyItems), sizeof(Key *));
	/* global functions */
	for (i = 0; i < LENGTH(KeyItems); i++) {
		Key key = { 0, };

		snprintf(name, sizeof(name), "%s", KeyItems[i].name);
		snprintf(clas, sizeof(clas), "%s", KeyItems[i].name);
		DPRINTF("Check for key item '%s'\n", name);
		if (!(res = readres(name, capclass(clas), NULL)))
			continue;
		key.func = KeyItems[i].action;
		key.arg = NULL;
		parsekeys(res, &key);
	}
	/* increment, decrement and set functions */
	for (j = 0; j < LENGTH(KeyItemsByAmt); j++) {
		for (i = 0; i < LENGTH(inc_prefix); i++) {
			Key key = { 0, };

			snprintf(name, sizeof(name), "%s%s", inc_prefix[i].prefix,
				 KeyItemsByAmt[j].name);
			snprintf(clas, sizeof(clas), "%s%s", inc_prefix[i].prefix,
				 KeyItemsByAmt[j].name);
			DPRINTF("Check for key item '%s'\n", name);
			if (!(res = readres(name, capclass(clas), NULL)))
				continue;
			key.func = KeyItemsByAmt[j].action;
			key.arg = NULL;
			key.act = inc_prefix[i].act;
			parsekeys(res, &key);
		}
	}
	/* client or screen state set, unset and toggle functions */
	for (j = 0; j < LENGTH(KeyItemsByState); j++) {
		for (i = 0; i < LENGTH(set_prefix); i++) {
			for (l = 0; l < LENGTH(set_suffix); l++) {
				Key key = { 0, };

				snprintf(name, sizeof(name), "%s%s%s",
					 set_prefix[i].prefix, KeyItemsByState[j].name,
					 set_suffix[l].suffix);
				snprintf(clas, sizeof(clas), "%s%s%s",
					 set_prefix[i].prefix, KeyItemsByState[j].name,
					 set_suffix[l].suffix);
				DPRINTF("Check for key item '%s'\n", name);
				if (!(res = readres(name, capclass(clas), NULL)))
					continue;
				key.func = KeyItemsByState[j].action;
				key.arg = NULL;
				key.set = set_prefix[i].set;
				key.any = set_suffix[l].any;
				parsekeys(res, &key);
			}
		}
	}
	/* functions with a relative direction */
	for (j = 0; j < LENGTH(KeyItemsByDir); j++) {
		for (i = 0; i < LENGTH(rel_suffix); i++) {
			Key key = { 0, };

			snprintf(name, sizeof(name), "%s%s", KeyItemsByDir[j].name,
				 rel_suffix[i].suffix);
			snprintf(clas, sizeof(clas), "%s%s", KeyItemsByDir[j].name,
				 rel_suffix[i].suffix);
			DPRINTF("Check for key item '%s'\n", name);
			if (!(res = readres(name, capclass(clas), NULL)))
				continue;
			key.func = KeyItemsByDir[j].action;
			key.arg = NULL;
			key.dir = rel_suffix[i].dir;
			parsekeys(res, &key);
		}
	}
	/* per tag functions */
	for (j = 0; j < LENGTH(KeyItemsByTag); j++) {
		for (i = 0; i < MAXTAGS; i++) {
			Key key = { 0, };

			snprintf(name, sizeof(name), "%s%d", KeyItemsByTag[j].name, i);
			snprintf(clas, sizeof(clas), "%s%d", KeyItemsByTag[j].name, i);
			DPRINTF("Check for key item '%s'\n", name);
			if (!(res = readres(name, capclass(clas), NULL)))
				continue;
			key.func = KeyItemsByTag[j].action;
			key.arg = NULL;
			key.tag = i;
			key.dir = RelativeNone;
			parsekeys(res, &key);
		}
		for (i = 0; i < LENGTH(tag_suffix); i++) {
			Key key = { 0, };

			snprintf(name, sizeof(name), "%s%s", KeyItemsByTag[j].name,
				 tag_suffix[i].suffix);
			snprintf(clas, sizeof(clas), "%s%s", KeyItemsByTag[j].name,
				 tag_suffix[i].suffix);
			DPRINTF("Check for key item '%s'\n", name);
			if (!(res = readres(name, capclass(clas), NULL)))
				continue;
			key.func = KeyItemsByTag[j].action;
			key.arg = NULL;
			key.tag = 0;
			key.dir = tag_suffix[i].dir;
			key.wrap = tag_suffix[i].wrap;
			parsekeys(res, &key);
		}
	}
	/* list settings */
	for (j = 0; j < LENGTH(KeyItemsByList); j++) {
		for (i = 0; i < 32; i++) {
			Key key = { 0, };

			snprintf(name, sizeof(name), "%s%d", KeyItemsByList[j].name, i);
			snprintf(clas, sizeof(clas), "%s%d", KeyItemsByList[j].name, i);
			DPRINTF("Check for key item '%s'\n", name);
			if (!(res = readres(name, capclass(clas), NULL)))
				continue;
			key.func = KeyItemsByList[j].action;
			key.arg = NULL;
			key.tag = i;
			key.any = AllClients;
			key.dir = RelativeNone;
			key.cyc = False;
			parsekeys(res, &key);
		}
		for (i = 0; i < LENGTH(lst_suffix); i++) {
			for (l = 0; l < LENGTH(list_which); l++) {
				Key key = { 0, };

				snprintf(name, sizeof(name), "%s%s%s",
					 KeyItemsByList[j].name, lst_suffix[i].suffix,
					 list_which[l].which);
				snprintf(clas, sizeof(clas), "%s%s%s",
					 KeyItemsByList[j].name, lst_suffix[i].suffix,
					 list_which[l].which);
				DPRINTF("Check for key item '%s'\n", name);
				if (!(res = readres(name, capclass(clas), NULL)))
					continue;
				key.func = KeyItemsByList[j].action;
				key.arg = NULL;
				key.tag = 0;
				key.any = list_which[l].any;
				key.dir = lst_suffix[i].dir;
				key.cyc = False;
				parsekeys(res, &key);
			}
		}
		for (i = 0; i < LENGTH(cyc_suffix); i++) {
			for (l = 0; l < LENGTH(list_which); l++) {
				Key key = { 0, };

				snprintf(name, sizeof(name), "cycle%s%s%s",
					 KeyItemsByList[j].name, cyc_suffix[i].suffix,
					 list_which[l].which);
				snprintf(clas, sizeof(clas), "cycle%s%s%s",
					 KeyItemsByList[j].name, cyc_suffix[i].suffix,
					 list_which[l].which);
				DPRINTF("Check for key item '%s'\n", name);
				if (!(res = readres(name, capclass(clas), NULL)))
					continue;
				key.func = KeyItemsByList[j].action;
				key.arg = NULL;
				key.tag = 0;
				key.any = list_which[l].any;
				key.dir = cyc_suffix[i].dir;
				key.cyc = True;
				parsekeys(res, &key);
			}
		}
	}
	/* layout setting */
	for (i = 0; layouts[i].symbol != '\0'; i++) {
		Key key = { 0, };

		snprintf(name, sizeof(name), "setlayout%c", layouts[i].symbol);
		snprintf(clas, sizeof(clas), "setlayout%c", layouts[i].symbol);
		DPRINTF("Check for key item '%s'\n", name);
		if (!(res = readres(name, capclass(clas), NULL)))
			continue;
		key.func = k_setlayout;
		key.arg = strdup(&layouts[i].symbol);
		parsekeys(res, &key);
	}
	/* spawn */
	for (i = 0; i < 64; i++) {
		Key key = { 0, };

		snprintf(name, sizeof(name), "spawn%d", i);
		snprintf(clas, sizeof(clas), "spawn%d", i);
		DPRINTF("Check for key item '%s'\n", name);
		if (!(res = readres(name, capclass(clas), NULL)))
			continue;
		key.func = k_spawn;
		key.arg = NULL;
		parsekeys(res, &key);
	}
}

static void
initstyle_ADWM(void)
{
	const char *res;
	AdwmStyle *style;

	xresdb = xstyledb;
	if (!styles)
		styles = ecalloc(nscr, sizeof(*styles));
	style = styles + scr->screen;

	res = readres("adwm.style.normal.border", "Adwm.Style.Normal.Border",
		      NORMBORDERCOLOR);
	getxcolor(res, NORMBORDERCOLOR, &style->normal.color.border);
	res = readres("adwm.style.normal.bg", "Adwm.Style.Normal.Bg", NORMBGCOLOR);
	getxcolor(res, NORMBGCOLOR, &style->normal.color.bg);
	res = readres("adwm.style.normal.fg", "Adwm.Style.Normal.Fg", NORMFGCOLOR);
	getxcolor(res, NORMFGCOLOR, &style->normal.color.fg);
	res = readres("adwm.style.normal.button", "Adwm.Style.Normal.Button",
		      NORMBUTTONCOLOR);
	getxcolor(res, NORMBUTTONCOLOR, &style->normal.color.button);

	res = readres("adwm.style.selected.border", "Adwm.Style.Selected.Border",
		      SELBORDERCOLOR);
	getxcolor(res, SELBORDERCOLOR, &style->selected.color.border);
	res = readres("adwm.style.selected.bg", "Adwm.Style.Selected.Bg", SELBGCOLOR);
	getxcolor(res, SELBGCOLOR, &style->selected.color.bg);
	res = readres("adwm.style.selected.fg", "Adwm.Style.Selected.Fg", SELFGCOLOR);
	getxcolor(res, SELFGCOLOR, &style->selected.color.fg);
	res = readres("adwm.style.selected.button", "Adwm.Style.Selected.Button",
		      SELBUTTONCOLOR);
	getxcolor(res, SELBUTTONCOLOR, &style->selected.color.button);

	res = readres("adwm.style.normal.font", "Adwm.Style.Normal.Font", FONT);
	getfont(res, FONT, &style->normal.font);
	res = readres("adwm.style.normal.fg", "Adwm.Style.Normal.Fg", NORMFGCOLOR);
	getxftcolor(res, NORMFGCOLOR, &style->normal.color.font);
	res = readres("adwm.style.normal.drop", "Adwm.Style.Normal.Drop", "0");
	style->normal.drop = strtoul(res, NULL, 0);
	res = readres("adwm.style.normal.shadow", "Adwm.Style.Normal.Shadow",
		      NORMBORDERCOLOR);
	getxftcolor(res, NORMBORDERCOLOR, &style->normal.color.shadow);

	res = readres("adwm.style.selected.font", "Adwm.Style.Selected.Font", FONT);
	getfont(res, FONT, &style->selected.font);
	res = readres("adwm.style.selected.fg", "Adwm.Style.Selected.Fg", SELFGCOLOR);
	getxftcolor(res, SELFGCOLOR, &style->selected.color.font);
	res = readres("adwm.style.selected.drop", "Adwm.Style.Selected.Drop", "0");
	style->selected.drop = strtoul(res, NULL, 0);
	res = readres("adwm.style.selected.shadow", "Adwm.Style.Selected.Shadow",
		      SELBORDERCOLOR);
	getxftcolor(res, SELBORDERCOLOR, &style->selected.color.shadow);

	res = readres("adwm.style.border", "Adwm.Style.Border", STR(BORDERPX));
	style->border = strtoul(res, NULL, 0);
	res = readres("adwm.style.margin", "Adwm.Style.Margin", STR(MARGINPX));
	style->margin = strtoul(res, NULL, 0);
	res = readres("adwm.style.opacity", "Adwm.Style.Opacity", STR(NF_OPACITY));
	style->opacity = strtod(res, NULL) * OPAQUE;
	getbool("adwm.style.outline", "Adwm.Style.Outline", "1", False, &style->outline);
	res = readres("adwm.style.spacing", "Adwm.Style.Spacing", "1");
	style->spacing = strtoul(res, NULL, 0);
	res = readres("adwm.style.titlelayout", "Adwm.Style.TitleLayout", "N  IMC");
	style->titlelayout = res;
	res = readres("adwm.style.grips", "Adwm.Style.Grips", STR(GRIPHEIGHT));
	style->grips = strtoul(res, NULL, 0);
	res = readres("adwm.style.title", "Adwm.Style.Title", STR(TITLEHEIGHT));
	style->title = strtoul(res, NULL, 0);

	{
		static const struct {
			const char *name;
			const char *clas;
			const char *def;
		} elements[LastElement] = {
			/* *INDENT-OFF* */
			[IconifyBtn]	= { "button.iconify",	ICONPIXMAP  },
			[MaximizeBtn]	= { "button.maximize",	MAXPIXMAP   },
			[CloseBtn]	= { "button.close",	CLOSEPIXMAP },
			[ShadeBtn]	= { "button.shade",	SHADEPIXMAP },
			[StickBtn]	= { "button.stick",	STICKPIXMAP },
			[LHalfBtn]	= { "button.lhalf",	LHALFPIXMAP },
			[RHalfBtn]	= { "button.rhalf",	RHALFPIXMAP },
			[FillBtn]	= { "button.fill",	FILLPIXMAP  },
			[FloatBtn]	= { "button.float",	FLOATPIXMAP },
			[SizeBtn]	= { "button.resize",	SIZEPIXMAP  },
			[TitleTags]	= { "title.tags",	NULL	    },
			[TitleName]	= { "title.name",	NULL	    },
			[TitleSep]	= { "title.sep",	NULL	    },
			/* *INDENT-ON* */
		};
		static const struct {
			const char *name;
			const char *clas;
		} kind[LastButtonImageType] = {
			/* *INDENT-OFF* */
			[ButtonImageDefault]		    = { "",				""			    },
			[ButtonImagePressed]		    = { ".pressed",			".Pressed"		    },
			[ButtonImageToggledPressed]	    = { ".toggled.pressed",		".Toggled.Pressed"	    },
			[ButtonImageHover]		    = { ".hovered",			".Hovered"		    },
			[ButtonImageFocus]		    = { ".focused",			".Focused"		    },
			[ButtonImageUnfocus]		    = { ".unfocus",			".Unfocus"		    },
			[ButtonImageToggledHover]	    = { ".toggled.hovered",		".Toggled.Hovered"	    },
			[ButtonImageToggledFocus]	    = { ".toggled.focused",		".Toggled.Focused"	    },
			[ButtonImageToggledUnfocus]	    = { ".toggled.unfocus",		".Toggled.Unfocus"	    },
			[ButtonImageDisabledHover]	    = { ".disabled.hovered",		".Disabled.Hovered"	    },
			[ButtonImageDisabledFocus]	    = { ".disabled.focused",		".Disabled.Focused"	    },
			[ButtonImageDisabledUnfocus]	    = { ".disabled.unfocus",		".Disabled.Unfocus"	    },
			[ButtonImageToggledDisabledHover]   = { ".toggled.disabled.hovered",	".Toggled.Disabled.Hovered" },
			[ButtonImageToggledDisabledFocus]   = { ".toggled.disabled.focused",	".Toggled.Disabled.Focused" },
			[ButtonImageToggledDisabledUnfocus] = { ".toggled.disabled.unfocus",	".Toggled.Disabled.Unfocus" },
			/* *INDENT-ON* */
		};
		int i, j, n;
		char name[256], clas[256];

		n = LastElement * LastButtonImageType;
		free(style->elements);
		style->elements = calloc(n, sizeof(*style->elements));

		for (n = 0, i = 0; i < LastElement; i++) {
			for (j = 0; j < LastButtonImageType; j++, n++) {
				snprintf(name, sizeof(name), "adwm.style.%s%s",
					 elements[i].name, kind[j].name);
				snprintf(clas, sizeof(clas), "Adwm.Style.%s%s",
					 elements[i].clas, kind[j].clas);
				readtexture(name, clas, &style->elements[n], "black",
					    "white");
				if (!style->elements[n].pixmap.file && elements[i].def)
					getpixmap(elements[i].def,
						  &style->elements[i].pixmap);
			}
		}
	}
}

static void
deinitstyle_ADWM(void)
{
}

static void
drawclient_ADWM(Client *c)
{
}

AdwmOperations adwm_ops = {
	.name = "adwm",
	.clas = "Adwm",
	.initrcfile = &initrcfile_ADWM,
	.initconfig = &initconfig_ADWM,
	.initkeys = &initkeys_ADWM,
	.initstyle = &initstyle_ADWM,
	.deinitstyle = &deinitstyle_ADWM,
	.drawclient = &drawclient_ADWM,
};

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
