#include <regex.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include "adwm.h"
#include "layout.h"
#include "tags.h"
#include "actions.h"
#include "parse.h" /* verification */

typedef struct {
	const char *name;
	void (*action) (XEvent *e, Key *k);
} KeyItem;

static KeyItem KeyItems[] = {
	/* *INDENT-OFF* */
	{ "viewprevtag",	k_viewprevtag	 },
	{ "quit",		k_quit		 }, /* arg is new command */
	{ "restart", 		k_restart	 }, /* arg is new command */
	{ "killclient",		k_killclient	 },
	{ "zoom", 		k_zoom		 },
	{ "moveright", 		k_moveresizekb	 }, /* arg is delta geometry */
	{ "moveleft", 		k_moveresizekb	 }, /* arg is delta geometry */
	{ "moveup", 		k_moveresizekb	 }, /* arg is delta geometry */
	{ "movedown", 		k_moveresizekb	 }, /* arg is delta geometry */
	{ "resizedecx", 	k_moveresizekb	 }, /* arg is delta geometry */
	{ "resizeincx", 	k_moveresizekb	 }, /* arg is delta geometry */
	{ "resizedecy", 	k_moveresizekb	 }, /* arg is delta geometry */
	{ "resizeincy", 	k_moveresizekb	 }, /* arg is delta geometry */
	{ "togglemonitor", 	k_togglemonitor	 },
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
	{ "all",	AllClients	},	/* all on current monitor */
	{ "any",	AnyClient	},	/* clients on any monitor */
	{ "every",	EveryClient	}	/* clients on every workspace */
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
	{ "",		FocusClient	},	/* focusable, same monitor */
	{ "act",	ActiveClient	},	/* activatable (incl. icons), same monitor */
	{ "all",	AllClients	},	/* all clients (incl. icons), same mon */
	{ "any",	AnyClient	},	/* any client on any monitor */
	{ "every",	EveryClient	}	/* all clients, all desktops */
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
	{ "focus",	k_focus		},
	{ "client",	k_client	},
	{ "stack",	k_stack		},
	{ "group",	k_group		},
	{ "tab",	k_tab		},
	{ "panel",	k_panel		},
	{ "dock",	k_dock		},
	{ "swap",	k_swap		}
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
	{"view",	k_view		},
	{"toggleview",	k_toggleview	},
	{"focusview",	k_focusview	},
	{"tag",		k_tag		},
	{"toggletag",	k_toggletag	},
	{"taketo",	k_taketo	}
	/* *INDENT-ON* */
};

#define CLEANMASK(mask) (mask & ~(numlockmask | LockMask))

static void
freekey(Key *k)
{
	if (k) {
		free(k->arg);
		free(k);
	}
}

void
freechain(Key *chain)
{
	Key *k, *knext, *c, *cnext;

	for (knext = chain; (k = knext);) {
		knext = k->chain;
		for (cnext = k->cnext; (c = cnext);) {
			cnext = c->cnext;
			freekey(c);
		}
		freekey(k);
	}
}

static void
mergechain(Key **kp, Key *k)
{
	Key **lp;

	Key *next = k->chain;

	k->chain = NULL;
	freekey(k);
	k = next;

	for (lp = &(*kp)->chain; *lp; lp = &(*lp)->cnext)
		if ((*lp)->mod == k->mod && (*lp)->keysym == k->keysym)
			break;
	if (*lp) {
		if ((*lp)->chain && k->chain)
			mergechain(lp, k);
		else {
			DPRINTF("Overriding previous key alternate!\n");
			freechain(*lp);
			*lp = k;
		}
	} else
		*lp = k;
}

void
addchain(Key *k)
{
	Key **kp;
	int i;

	for (kp = scr->keys, i = 0; i < scr->nkeys; i++, kp++)
		if ((*kp)->mod == k->mod && (*kp)->keysym == k->keysym)
			break;
	if (i < scr->nkeys) {
		if ((*kp)->chain && k->chain)
			mergechain(kp, k);
		else {
			DPRINTF("Overriding previous key definition!\n");
			freechain(*kp);
			*kp = k;
		}
	} else {
		scr->keys = erealloc(scr->keys, (scr->nkeys + 1) * sizeof(*scr->keys));
		scr->keys[scr->nkeys] = k;
		scr->nkeys++;
	}
}

static unsigned long
parsemod(const char *s, const char *e)
{
	const char *p;
	unsigned long mod = 0;

	DPRINTF("Parsing mod keys from '%s'\n", s);
	for (p = s; p < e; p++)
		switch (*p) {
		case 'A':
			mod |= Mod1Mask;
			break;
		case 'S':
			mod |= ShiftMask;
			break;
		case 'C':
			mod |= ControlMask;
			break;
		case 'W':
			mod |= Mod4Mask;
			break;
		case 'M':
			mod |= modkey;
			break;
		case 'N':
			mod |= (modkey == Mod1Mask) ? Mod4Mask : 0;
			mod |= (modkey == Mod4Mask) ? Mod1Mask : 0;
			break;
		default:
			if (isblank(*p))
				break;
			DPRINTF("Unrecognized character '%c' in key spec\n", *p);
			break;
		}
	return mod;
}

static KeySym
parsesym(const char *s, const char *e)
{
	char *t;
	KeySym sym = NoSymbol;

	for (; s < e && isblank(*s); s++) ;
	for (; e > s && isblank(*(e - 1)); e--) ;
	DPRINTF("Parsing keysym from '%s'\n", s);
	if (s < e && (t = strndup(s, e - s))) {
		sym = XStringToKeysym(t);
		free(t);
	}
	return sym;
}

static char *
parsearg(const char *s, const char *e)
{
	char *arg = NULL;

	for (; s < e && isblank(*s); s++) ;
	for (; e > s && isblank(*(e - 1)); e--) ;
	DPRINTF("Parsing arg from '%s'\n", s);
	if (s < e)
		arg = strndup(s, e - s);
	return arg;
}

static Bool
parsekey(const char *s, const char *e, Key *k)
{
	const char *p, *q;

	DPRINTF("Parsing key from: '%s'\n", s);
	if ((p = strchr(s, '+')) && p < e) {
		k->mod = parsemod(s, p);
		p++;
	} else
		p = s;
	q = ((q = strchr(p, '=') ? : e) < e) ? q : e;
	if ((k->keysym = parsesym(p, q)) == NoSymbol) {
		DPRINTF("Failed to parse symbol from '%s'\n", p);
		return False;
	}
	if (q < e) {
		free(k->arg);
		k->arg = parsearg(q + 1, e);
	}
	return True;
}

static Key *
parsechain(const char *s, const char *e, Key *spec)
{
	const char *p, *q;
	Key *chain = NULL, *last = NULL;

	DPRINTF("Parsing chain from: '%s'\n", s);

	for (p = s, q = ((q = strchr(p, ':') ? : e) > e) ? e : q;
	     *p != '\0' && *p != ',';
	     p = (*q == ':' ? q + 1 : q), q = ((q = strchr(p, ':') ? : e) > e) ? e : q) {
		Key *k = ecalloc(1, sizeof(*k));

		*k = *spec;
		if (!parsekey(p, q, k)) {
			freekey(k);
			freechain(chain);
			chain = last = NULL;
			break;
		}
		chain = chain ? : k;
		if (last) {
			last->func = &k_chain;
			last->chain = k;
		}
		last = k;
	}
	return chain;
}

void
parsekeys(const char *s, Key *spec)
{
	const char *p, *e;
	Key *k;

	DPRINTF("Parsing key: '%s'\n", s);
	for (p = s, e = strchrnul(p, ','); *p != '\0';
	     p = (*e == ',' ? e + 1 : e), e = strchrnul(p, ','))
		if ((k = parsechain(p, e, spec))) {
			DPRINTF("Adding key: '%s'\n", s);
			addchain(k);
		}
}

static void
initmodkey()
{
	char tmp;

	strncpy(&tmp, getresource("modkey", "A"), 1);
	switch (tmp) {
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

void
initkeys()
{
	unsigned int i, j, l;
	const char *tmp;
	char t[64];

	initmodkey();
	scr->keys = ecalloc(LENGTH(KeyItems), sizeof(Key *));
	/* global functions */
	for (i = 0; i < LENGTH(KeyItems); i++) {
		Key key = { 0, };

		snprintf(t, sizeof(t), "%s", KeyItems[i].name);
		DPRINTF("Check for key item '%s'\n", t);
		tmp = getresource(t, NULL);
		if (!tmp)
			continue;
		key.func = KeyItems[i].action;
		key.arg = NULL;
		parsekeys(tmp, &key);
	}
	/* increment, decrement and set functions */
	for (j = 0; j < LENGTH(KeyItemsByAmt); j++) {
		for (i = 0; i < LENGTH(inc_prefix); i++) {
			Key key = { 0, };

			snprintf(t, sizeof(t), "%s%s", inc_prefix[i].prefix,
				 KeyItemsByAmt[j].name);
			DPRINTF("Check for key item '%s'\n", t);
			tmp = getresource(t, NULL);
			if (!tmp)
				continue;
			key.func = KeyItemsByAmt[j].action;
			key.arg = NULL;
			key.act = inc_prefix[i].act;
			parsekeys(tmp, &key);
		}
	}
	/* client or screen state set, unset and toggle functions */
	for (j = 0; j < LENGTH(KeyItemsByState); j++) {
		for (i = 0; i < LENGTH(set_prefix); i++) {
			for (l = 0; l < LENGTH(set_suffix); l++) {
				Key key = { 0, };

				snprintf(t, sizeof(t), "%s%s%s", set_prefix[i].prefix,
					 KeyItemsByState[j].name, set_suffix[l].suffix);
				DPRINTF("Check for key item '%s'\n", t);
				tmp = getresource(t, NULL);
				if (!tmp)
					continue;
				key.func = KeyItemsByState[j].action;
				key.arg = NULL;
				key.set = set_prefix[i].set;
				key.any = set_suffix[l].any;
				parsekeys(tmp, &key);
			}
		}
	}
	/* functions with a relative direction */
	for (j = 0; j < LENGTH(KeyItemsByDir); j++) {
		for (i = 0; i < LENGTH(rel_suffix); i++) {
			Key key = { 0, };

			snprintf(t, sizeof(t), "%s%s", KeyItemsByDir[j].name,
				 rel_suffix[i].suffix);
			DPRINTF("Check for key item '%s'\n", t);
			tmp = getresource(t, NULL);
			if (!tmp)
				continue;
			key.func = KeyItemsByDir[j].action;
			key.arg = NULL;
			key.dir = rel_suffix[i].dir;
			parsekeys(tmp, &key);
		}
	}
	/* per tag functions */
	for (j = 0; j < LENGTH(KeyItemsByTag); j++) {
		for (i = 0; i < MAXTAGS; i++) {
			Key key = { 0, };

			snprintf(t, sizeof(t), "%s%d", KeyItemsByTag[j].name, i);
			DPRINTF("Check for key item '%s'\n", t);
			tmp = getresource(t, NULL);
			if (!tmp)
				continue;
			key.func = KeyItemsByTag[j].action;
			key.arg = NULL;
			key.tag = i;
			key.dir = RelativeNone;
			parsekeys(tmp, &key);
		}
		for (i = 0; i < LENGTH(tag_suffix); i++) {
			Key key = { 0, };

			snprintf(t, sizeof(t), "%s%s", KeyItemsByTag[j].name,
				 tag_suffix[i].suffix);
			DPRINTF("Check for key item '%s'\n", t);
			tmp = getresource(t, NULL);
			if (!tmp)
				continue;
			key.func = KeyItemsByTag[j].action;
			key.arg = NULL;
			key.tag = 0;
			key.dir = tag_suffix[i].dir;
			key.wrap = tag_suffix[i].wrap;
			parsekeys(tmp, &key);
		}
	}
	/* list settings */
	for (j = 0; j < LENGTH(KeyItemsByList); j++) {
		for (i = 0; i < 32; i++) {
			Key key = { 0, };

			snprintf(t, sizeof(t), "%s%d", KeyItemsByList[j].name, i);
			DPRINTF("Check for key item '%s'\n", t);
			tmp = getresource(t, NULL);
			if (!tmp)
				continue;
			key.func = KeyItemsByList[j].action;
			key.arg = NULL;
			key.tag = i;
			key.any = AllClients;
			key.dir = RelativeNone;
			key.cyc = False;
			parsekeys(tmp, &key);
		}
		for (i = 0; i < LENGTH(lst_suffix); i++) {
			for (l = 0; l < LENGTH(list_which); l++) {
				Key key = { 0, };

				snprintf(t, sizeof(t), "%s%s%s",
					 KeyItemsByList[j].name,
					 lst_suffix[i].suffix, list_which[l].which);
				DPRINTF("Check for key item '%s'\n", t);
				tmp = getresource(t, NULL);
				if (!tmp)
					continue;
				key.func = KeyItemsByList[j].action;
				key.arg = NULL;
				key.tag = 0;
				key.any = list_which[l].any;
				key.dir = lst_suffix[i].dir;
				key.cyc = False;
				parsekeys(tmp, &key);
			}
		}
		for (i = 0; i < LENGTH(cyc_suffix); i++) {
			for (l = 0; l < LENGTH(list_which); l++) {
				Key key = { 0, };

				snprintf(t, sizeof(t), "cycle%s%s%s",
					 KeyItemsByList[j].name,
					 cyc_suffix[i].suffix, list_which[l].which);
				DPRINTF("Check for key item '%s'\n", t);
				tmp = getresource(t, NULL);
				if (!tmp)
					continue;
				key.func = KeyItemsByList[j].action;
				key.arg = NULL;
				key.tag = 0;
				key.any = list_which[l].any;
				key.dir = cyc_suffix[i].dir;
				key.cyc = True;
				parsekeys(tmp, &key);
			}
		}
	}
	/* layout setting */
	for (i = 0; layouts[i].symbol != '\0'; i++) {
		Key key = { 0, };

		snprintf(t, sizeof(t), "setlayout%c", layouts[i].symbol);
		DPRINTF("Check for key item '%s'\n", t);
		tmp = getresource(t, NULL);
		if (!tmp)
			continue;
		key.func = k_setlayout;
		key.arg = strdup(&layouts[i].symbol);
		parsekeys(tmp, &key);
	}
	/* spawn */
	for (i = 0; i < 64; i++) {
		Key key = { 0, };

		snprintf(t, sizeof(t), "spawn%d", i);
		DPRINTF("Check for key item '%s'\n", t);
		tmp = getresource(t, NULL);
		if (!tmp)
			continue;
		key.func = k_spawn;
		key.arg = NULL;
		parsekeys(tmp, &key);
	}
}

static void
parserule(const char *s, Rule * r)
{
	r->prop = emallocz(128);
	r->tags = emallocz(64);
	sscanf(s, "%s %s %d %d", r->prop, r->tags, &r->isfloating, &r->hastitle);
}

static void
compileregs(void)
{
	unsigned int i;
	regex_t *reg;

	for (i = 0; i < nrules; i++) {
		if (rules[i]->prop) {
			reg = emallocz(sizeof(regex_t));
			if (regcomp(reg, rules[i]->prop, REG_EXTENDED))
				free(reg);
			else
				rules[i]->propregex = reg;
		}
		if (rules[i]->tags) {
			reg = emallocz(sizeof(regex_t));
			if (regcomp(reg, rules[i]->tags, REG_EXTENDED))
				free(reg);
			else
				rules[i]->tagregex = reg;
		}
	}
}

void
initrules()
{
	int i;
	char t[64];
	const char *tmp;

	rules = ecalloc(64, sizeof(Rule *));
	for (i = 0; i < 64; i++) {
		snprintf(t, sizeof(t), "rule%d", i);
		tmp = getresource(t, NULL);
		if (!tmp)
			continue;
		rules[nrules] = emallocz(sizeof(Rule));
		parserule(tmp, rules[nrules]);
		nrules++;
	}
	// rules = erealloc(rules, nrules * sizeof(Rule *));
	compileregs();
}
