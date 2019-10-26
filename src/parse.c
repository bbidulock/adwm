/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "layout.h"
#include "tags.h"
#include "actions.h"
#include "parse.h" /* verification */

typedef struct {
	const char *name;
	void (*action) (XEvent *e, Key *k);
	char *arg;
} KeyItem;

static KeyItem KeyItems[] = {
	/* *INDENT-OFF* */
	{ "viewprevtag",	k_viewprevtag,	 NULL		},
	{ "quit",		k_quit,		 NULL		}, /* arg is new command */
	{ "restart",		k_restart,	 NULL		}, /* arg is new command */
	{ "reload",		k_reload,	 NULL		},
	{ "killclient",		k_killclient,	 NULL		},
	{ "focusmain",		k_focusmain,	 NULL		},
	{ "focusurgent",	k_focusurgent,	 NULL		},
	{ "zoom",		k_zoom,		 NULL		},
	{ "moveright",		k_moveresizekb,	 "+5 +0 +0 +0"	}, /* arg is delta geometry */
	{ "moveleft",		k_moveresizekb,	 "-5 +0 +0 +0"	}, /* arg is delta geometry */
	{ "moveup",		k_moveresizekb,	 "+0 -5 +0 +0"	}, /* arg is delta geometry */
	{ "movedown",		k_moveresizekb,	 "+0 +5 +0 +0"	}, /* arg is delta geometry */
	{ "resizedecx",		k_moveresizekb,	 "+3 +0 -6 +0"	}, /* arg is delta geometry */
	{ "resizeincx",		k_moveresizekb,	 "-3 +0 +6 +0"	}, /* arg is delta geometry */
	{ "resizedecy",		k_moveresizekb,	 "+0 +3 +0 -6"	}, /* arg is delta geometry */
	{ "resizeincy",		k_moveresizekb,	 "+0 -3 +0 +6"	}, /* arg is delta geometry */
	{ "togglemonitor",	k_togglemonitor, NULL		},
	{ "appendtag",		k_appendtag,	 NULL		},
	{ "rmlasttag",		k_rmlasttag,	 NULL		},
	{ "flipview",		k_flipview,	 NULL		},
	{ "rotateview",		k_rotateview,	 NULL		},
	{ "unrotateview",	k_unrotateview,	 NULL		},
	{ "flipzone",		k_flipzone,	 NULL		},
	{ "rotatezone",		k_rotatezone,	 NULL		},
	{ "unrotatezone",	k_unrotatezone,	 NULL		},
	{ "flipwins",		k_flipwins,	 NULL		},
	{ "rotatewins",		k_rotatewins,	 NULL		},
	{ "unrotatewins",	k_unrotatewins,	 NULL		},
	{ "raise",		k_raise,	 NULL		},
	{ "lower",		k_lower,	 NULL		},
	{ "raiselower",		k_raiselower,	 NULL		}
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
	{ "ncolumns",		k_setncolumns	 },
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

static const struct {
	const char *prefix;
	WindowPlacement plc;
} plc_prefix[] = {
	/* *INDENT-OFF* */
	{ "col",	ColSmartPlacement   },
	{ "row",	RowSmartPlacement   },
	{ "min",	MinOverlapPlacement },
	{ "und",	UnderMousePlacement },
	{ "cas",	CascadePlacement    },
	{ "rnd",	RandomPlacement     }
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
	{ "lhalf",		k_setlhalf	 },
	{ "rhalf",		k_setrhalf	 },
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
	{ "dectiled",		k_setdectiled	 },
	{ "sticky",		k_setsticky	 }
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
	{"taketo",	k_taketo	},
	{"sendto",	k_sendto	}
	/* *INDENT-ON* */
};

#define CLEANMASK(mask) (mask & ~(numlockmask | LockMask))

static const char *
showkey(Key *k)
{
	static char buf[256] = { 0, };
	unsigned long mod = k->mod;
	char *sym;

	buf[0] = '\0';
	if (mod & Mod1Mask)
		strcat(buf, "A");
	if (mod & ControlMask)
		strcat(buf, "C");
	if (mod & ShiftMask)
		strcat(buf, "S");
	if (mod & Mod4Mask)
		strcat(buf, "W");
	if (mod & (Mod1Mask|ControlMask|ShiftMask|Mod4Mask))
		strcat(buf, " + ");
	if ((sym = XKeysymToString(k->keysym)))
		strcat(buf, sym);
	else
		strcat(buf, "(unknown)");
	return (buf);
}

char *
showchain_r(Key *k)
{
	char *buf, *tmp;

	buf = calloc(1024, sizeof(*buf));
	strcat(buf, showkey(k));
	if (k->chain) {
		for (k = k->chain; k; k = k->cnext) {
			tmp = showchain_r(k);
			strcat(buf, " : ");
			strcat(buf, tmp);
			free(tmp);
			if (k->cnext)
				strcat(buf, ", ");
		}
	}
	return buf;
}

const char *
showchain(Key *k)
{
	static char buf[1025], *tmp;

	tmp = showchain_r(k);
	strncpy(buf, tmp, 1024);
	free(tmp);
	return buf;
}

static void
freekey(Key *k)
{
	if (k) {
		free(k->arg);
		k->arg = NULL;
		free(k);
	}
}

void
freechain(Key *k)
{
	Key *c, *cnext;

	if (k) {
		XPRINTF("Freeing chain: %p: %s\n", k, showchain(k));
		cnext = k->chain;
		k->chain = NULL;
		while ((c = cnext)) {
			cnext = c->cnext;
			c->cnext = NULL;
			freechain(c);
		}
		freekey(k);
	}
}

/*
 * (*kp) and k have the same mod and keysym.  Both have a non-null ->chain.
 * if there is a key in the (*kp)->chain list (*lp) that has the same mod and
 * keysm as k->chain, then free (*lp) and set its location to *kp->chain
 */
static void
mergechain(Key *c, Key *k)
{
	Key **lp;
	Key *next = k->chain;

	XPRINTF("Merging chain %s\n", showchain(k));
	XPRINTF("   into chain %s\n", showchain(c));
	k->chain = NULL;
	freekey(k);
	k = next;

	for (lp = &c->chain; *lp; lp = &(*lp)->cnext)
		if ((*lp)->mod == k->mod && (*lp)->keysym == k->keysym)
			break;
	if (*lp) {
		if ((*lp)->chain && k->chain)
			mergechain(*lp, k);
		else {
			XPRINTF("Overriding previous key alternate %s!\n", showchain(k));
			k->cnext = (*lp)->cnext;
			(*lp)->cnext = NULL;
			freechain(*lp);
			*lp = k;
		}
	} else {
		k->cnext = NULL;
		*lp = k;
	}
}

void
addchain(Key *k)
{
	Key **kp;

	XPRINTF("Adding chain: %s ...\n", showchain(k));
	for (kp = &scr->keylist; *kp; kp = &(*kp)->cnext)
		if ((*kp)->mod == k->mod && (*kp)->keysym == k->keysym)
			break;
	if (*kp) {
		XPRINTF("Overlapping key definition %s\n", XKeysymToString(k->keysym));
		if ((*kp)->chain && k->chain)
			mergechain(*kp, k);
		else {
			XPRINTF("Overriding previous key definition: %s!\n", showchain(k));
			k->cnext = (*kp)->cnext;
			(*kp)->cnext = NULL;
			freechain(*kp);
			*kp = k;
		}
		XPRINTF("... added chain: %s\n", showchain(*kp));
	} else {
		XPRINTF("Adding new key %s\n", XKeysymToString(k->keysym));
		k->cnext = scr->keylist;
		scr->keylist = k;
		XPRINTF("... added chain: %s\n", showchain(k));
	}
}

static unsigned long
parsemod(const char *s, const char *e)
{
	const char *p;
	unsigned long mod = 0;

	XPRINTF("Parsing mod keys from '%s'\n", s);
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
			XPRINTF("Unrecognized character '%c' in key spec\n", *p);
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
	XPRINTF("Parsing keysym from '%s'\n", s);
	if (s < e && (t = strndup(s, e - s))) {
		if ((sym = XStringToKeysym(t)) == NoSymbol)
			XPRINTF("Failed to parse symbol '%s'\n", t);
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
	XPRINTF("Parsing arg from '%s'\n", s);
	if (s < e)
		arg = strndup(s, e - s);
	return arg;
}

static Bool
parsekey(const char *s, const char *e, Key *k)
{
	const char *p, *q;

	XPRINTF("Parsing key from: '%s'\n", s);
	for (p = s; p < e && (isalnum(*p) || isblank(*p) || *p == '_'); p++) ;
	if (p < e && *p == '+') {
		k->mod = parsemod(s, p);
		p++;
	} else
		p = s;
	for (q = p; q < e && (isalnum(*q) || isblank(*q) || *q == '_'); q++) ;
	if (q < e && *q != '=')
		q = e;
	if ((k->keysym = parsesym(p, q)) == NoSymbol) {
		EPRINTF("Failed to parse symbol from '%s' to '%s'\n", p, q);
		return False;
	}
	if (q < e)
		k->arg = parsearg(q + 1, e);
	else if (k->arg)
		k->arg = strdup(k->arg);
	return True;
}

static Key *
parsechain(const char *s, const char *e, Key *spec)
{
	const char *p, *q;
	Key *chain = NULL, *last = NULL;

	XPRINTF("Parsing chain from: '%s'\n", s);

	for (p = s; p < e; p = (*q == ':' ? q + 1 : q)) {
		Key *k;

		for (q = p;
		     q < e && (isalnum(*q) || isblank(*q) || *q == '_' || *q == '+');
		     q++) ;
		if (q < e && *q != ':')
			q = e;
		k = ecalloc(1, sizeof(*k));
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
	if (chain)
		XPRINTF("Parsed chain: %s\n", showchain(chain));
	return chain;
}

void
parsekeys(const char *s, Key *spec)
{
	const char *p, *e;
	Key *k;

	XPRINTF("Parsing key: '%s'\n", s);
	for (p = s; *p != '\0'; p = (*e == ',' ? e + 1 : e)) {
		/* need to escape ',' in command */
		for (e = p; *e != '\0' && (*e != ',' || (e > p && *(e - 1) == '\\')); e++) ;
		if ((k = parsechain(p, e, spec))) {
			XPRINTF("Adding key: '%s'\n", s);
			addchain(k);
		}
	}
}

static void
initmodkey()
{
	char tmp[2];

	strncpy(tmp, getresource("modkey", "A"), 1);
	switch (tmp[0]) {
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
freekeys(void)
{
	Key *k, *knext;

	knext = scr->keylist;
	scr->keylist = NULL;
	while ((k = knext)) {
		knext = k->cnext;
		k->cnext = NULL;
		XPRINTF("Freeing key chain: %s\n", showchain(k));
		freechain(k);
	}
}

void
initkeys(Bool reload)
{
	unsigned int i, j, l;
	const char *res;
	char t[64];

	freekeys();
	initmodkey();
	/* global functions */
	for (i = 0; i < LENGTH(KeyItems); i++) {
		Key key = { 0, };

		snprintf(t, sizeof(t), "%s", KeyItems[i].name);
		XPRINTF("Check for key item '%s'\n", t);
		if (!(res = getresource(t, NULL)))
			continue;
		key.func = KeyItems[i].action;
		key.arg = KeyItems[i].arg;
		parsekeys(res, &key);
	}
	/* increment, decrement and set functions */
	for (j = 0; j < LENGTH(KeyItemsByAmt); j++) {
		for (i = 0; i < LENGTH(inc_prefix); i++) {
			Key key = { 0, };

			snprintf(t, sizeof(t), "%s%s", inc_prefix[i].prefix,
				 KeyItemsByAmt[j].name);
			XPRINTF("Check for key item '%s'\n", t);
			if (!(res = getresource(t, NULL)))
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

				snprintf(t, sizeof(t), "%s%s%s",
					 set_prefix[i].prefix, KeyItemsByState[j].name,
					 set_suffix[l].suffix);
				XPRINTF("Check for key item '%s'\n", t);
				if (!(res = getresource(t, NULL)))
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

			snprintf(t, sizeof(t), "%s%s", KeyItemsByDir[j].name,
				 rel_suffix[i].suffix);
			XPRINTF("Check for key item '%s'\n", t);
			if (!(res = getresource(t, NULL)))
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

			snprintf(t, sizeof(t), "%s%d", KeyItemsByTag[j].name, i);
			XPRINTF("Check for key item '%s'\n", t);
			if (!(res = getresource(t, NULL)))
				continue;
			key.func = KeyItemsByTag[j].action;
			key.arg = NULL;
			key.tag = i;
			key.dir = RelativeNone;
			parsekeys(res, &key);
		}
		for (i = 0; i < LENGTH(tag_suffix); i++) {
			Key key = { 0, };

			snprintf(t, sizeof(t), "%s%s", KeyItemsByTag[j].name,
				 tag_suffix[i].suffix);
			XPRINTF("Check for key item '%s'\n", t);
			if (!(res = getresource(t, NULL)))
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

			snprintf(t, sizeof(t), "%s%d", KeyItemsByList[j].name, i);
			XPRINTF("Check for key item '%s'\n", t);
			if (!(res = getresource(t, NULL)))
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

				snprintf(t, sizeof(t), "%s%s%s",
					 KeyItemsByList[j].name, lst_suffix[i].suffix,
					 list_which[l].which);
				XPRINTF("Check for key item '%s'\n", t);
				if (!(res = getresource(t, NULL)))
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

				snprintf(t, sizeof(t), "cycle%s%s%s",
					 KeyItemsByList[j].name, cyc_suffix[i].suffix,
					 list_which[l].which);
				XPRINTF("Check for key item '%s'\n", t);
				if (!(res = getresource(t, NULL)))
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
	/* placement setting */
	for (i = 0; i < LENGTH(plc_prefix); i++) {
		for (l = 0; l < LENGTH(set_suffix); l++) {
			Key key = { 0, };

			snprintf(t, sizeof(t), "%splacement%s",
					plc_prefix[i].prefix,
					set_suffix[l].suffix);
			XPRINTF("Check for key item '%s'\n", t);
			if (!(res = getresource(t, NULL)))
				continue;
			key.func = k_setplacement;
			key.arg = NULL;
			key.plc = plc_prefix[i].plc;
			key.any = set_suffix[l].any;
			parsekeys(res, &key);
		}
	}
	/* layout setting */
	for (i = 0; layouts[i].symbol != '\0'; i++) {
		Key key = { 0, };

		snprintf(t, sizeof(t), "setlayout%c", layouts[i].symbol);
		XPRINTF("Check for key item '%s'\n", t);
		if (!(res = getresource(t, NULL)))
			continue;
		key.func = k_setlayout;
		key.arg = t + 9;
		parsekeys(res, &key);
	}
	/* spawn */
	for (i = 0; i < 64; i++) {
		Key key = { 0, };

		snprintf(t, sizeof(t), "spawn%d", i);
		XPRINTF("Check for key item '%s'\n", t);
		if (!(res = getresource(t, NULL)))
			continue;
		key.func = k_spawn;
		key.arg = NULL;
		parsekeys(res, &key);
	}
}

char *skip_fields[32] = {
	"taskbar",
	"pager",
	"winlist",
	"cycle",
	"focus",
	"arrange",
	"sloppy",
	NULL,
};

SkipUnion skip_mask = {
	.taskbar = 1,
	.pager = 1,
	.winlist = 1,
	.cycle = 1,
	.focus = 1,
	.arrange = 1,
	.sloppy = 1,
};

char *has_fields[32] = {
	"border",
	"grips",
	"title",
	"but.menu",
	"but.min",
	"but.max",
	"but.close",
	"but.size",
	"but.shade",
	"but.stick",
	"but.fill",
	"but.floats",
	"but.half",
	NULL,
};

HasUnion has_mask = {
	.border = 1,
	.grips = 1,
	.title = 1,
	.but = {
		.menu = 1,
		.min = 1,
		.max = 1,
		.close = 1,
		.size = 1,
		.shade = 1,
		.stick = 1,
		.fill = 1,
		.floats = 1,
		.half = 1,
	},
};

char *is_fields[32] = {
	"transient",
	"grptrans",
	"banned",
	"max",
	"floater",
	"maxv",
	"maxh",
	"lhalf",
	"rhalf",
	"shaded",
	"icon",
	"fill",
	"modal",
	"above",
	"below",
	"attn",
	"sticky",
	"undec",
	"closing",
	"killing",
	"pinging",
	"hidden",
	"bastard",
	"full",
	"focused",
	"selected",
	"dockapp",
	"moveresize",
	"managed",
	"saving",
	NULL,
};

IsUnion is_mask = {
	.transient = 0,
	.grptrans = 0,
	.banned = 0,
	.max = 1,
	.floater = 1,
	.maxv = 1,
	.maxh = 1,
	.lhalf = 1,
	.rhalf = 1,
	.shaded = 1,
	.icon = 1,
	.fill = 1,
	.modal = 0,
	.above = 1,
	.below = 1,
	.attn = 0,
	.sticky = 1,
	.undec = 1,
	.closing = 0,
	.killing = 0,
	.pinging = 0,
	.hidden = 1,
	.bastard = 1,
	.full = 1,
	.focused = 0,
	.selected = 0,
	.dockapp = 1,
	.moveresize = 0,
	.managed = 0,
	.saving = 0,
};

char *can_fields[32] = {
	"move",
	"size",
	"sizev",
	"sizeh",
	"min",
	"max",
	"maxv",
	"maxh",
	"close",
	"shade",
	"stick",
	"full",
	"above",
	"below",
	"fill",
	"fillh",
	"fillv",
	"floats",
	"hide",
	"tag",
	"arrange",
	"undec",
	"select",
	"focus",
	NULL,
};

CanUnion can_mask = {
	.move = 1,
	.size = 1,
	.sizev = 1,
	.sizeh = 1,
	.min = 1,
	.max = 1,
	.maxv = 1,
	.maxh = 1,
	.close = 1,
	.shade = 1,
	.stick = 1,
	.full = 1,
	.above = 1,
	.below = 1,
	.fill = 1,
	.fillh = 1,
	.fillv = 1,
	.floats = 1,
	.hide = 1,
	.tag = 1,
	.arrange = 1,
	.undec = 1,
	.select = 1,
	.focus = 1,
};

char *with_fields[32] = {
	"struts",
	"time",
	"boundary",
	"clipping",
	"wshape",
	"bshape",
	NULL,
};

WithUnion with_mask = {
	.struts = 0,
	.time = 0,
	.boundary = 0,
	.clipping = 0,
	.wshape = 0,
	.bshape = 0,
};

static void
parserule(const char *s, Rule *r)
{
	Bool isfloating = False;
	Bool hastitle = False;

	r->prop = emallocz(128);
	r->tags = emallocz(64);
	sscanf(s, "%s %s %d %d", r->prop, r->tags, &isfloating, &hastitle);

	/* set current equivalents to old format */
	r->skip.skip.arrange = isfloating;
	r->skip.set.arrange = 1;
	r->has.has.title = hastitle;
	r->has.has.grips = hastitle;
	r->has.set.title = 1;
	r->has.set.grips = 1;
}

static void
compileregs(void)
{
	unsigned int i;
	regex_t *reg;

	for (i = 0; i < nrules; i++) {
		if (rules[i]->prop && strcmp(rules[i]->prop, "NULL")) {
			reg = emallocz(sizeof(regex_t));
			if (regcomp(reg, rules[i]->prop, REG_EXTENDED))
				free(reg);
			else
				rules[i]->propregex = reg;
		}
		if (rules[i]->tags && strcmp(rules[i]->tags, "NULL")) {
			reg = emallocz(sizeof(regex_t));
			if (regcomp(reg, rules[i]->tags, REG_EXTENDED))
				free(reg);
			else
				rules[i]->tagregex = reg;
		}
	}
}

static void
freerules(void)
{
	int i;

	if (!rules)
		return;
	for (i = 0; i < 64; i++) {
		if (!rules[i])
			continue;
		free(rules[i]->propregex);
		rules[i]->propregex = NULL;
		free(rules[i]->tagregex);
		rules[i]->tagregex = NULL;
		free(rules[i]->prop);
		rules[i]->prop = NULL;
		free(rules[i]->tags);
		rules[i]->tags = NULL;
		free(rules[i]);
		rules[i] = NULL;
	}
	free(rules);
	rules = NULL;
}

void
initrules(Bool reload)
{
	int i;
	char t[64];
	const char *res;

	freerules();
	rules = ecalloc(64, sizeof(Rule *));
	for (i = 0; i < 64; i++) {
		/* old format */
		snprintf(t, sizeof(t), "rule%d", i);
		if ((res = getresource(t, NULL))) {
			rules[nrules] = emallocz(sizeof(Rule));
			parserule(res, rules[nrules]);
			nrules++;
		} else {
			snprintf(t, sizeof(t), "rule%d.prop", i);
			if ((res = getresource(t, NULL))) {
				unsigned mask;
				Rule *r;
				int f;

				r = rules[nrules] = emallocz(sizeof(Rule));
				r->prop = strdup(res);
				snprintf(t, sizeof(t), "rule%d.tags", i);
				if ((res = getresource(t, NULL)))
					r->tags = strdup(res);
				for (f = 0, mask = 1; f < 32; f++, mask <<= 1) {
					if ((skip_mask.skip & mask) && !!skip_fields[f]) {
						snprintf(t, sizeof(t), "rule%d.skip.%s", i, skip_fields[f]);
						if ((res = getresource(t, NULL))) {
							r->skip.set.skip |= mask;
							if (atoi(res))
								r->skip.skip.skip |= mask;
						}
					}
				}
				for (f = 0, mask = 1; f < 32; f++, mask <<= 1) {
					if ((has_mask.has & mask) && !!has_fields[f]) {
						snprintf(t, sizeof(t), "rule%d.has.%s", i, has_fields[f]);
						if ((res = getresource(t, NULL))) {
							r->has.set.has |= mask;
							if (atoi(res))
								r->has.has.has |= mask;
						}
					}
				}
				for (f = 0, mask = 1; f < 32; f++, mask <<= 1) {
					if ((is_mask.is & mask) && !!is_fields[f]) {
						snprintf(t, sizeof(t), "rule%d.is.%s", i, is_fields[f]);
						if ((res = getresource(t, NULL))) {
							r->is.set.is |= mask;
							if (atoi(res))
								r->is.is.is |= mask;
						}
					}
				}
				for (f = 0, mask = 1; f < 32; f++, mask <<= 1) {
					if ((can_mask.can & mask) && !!can_fields[f]) {
						snprintf(t, sizeof(t), "rule%d.can.%s", i, can_fields[f]);
						if ((res = getresource(t, NULL))) {
							r->can.set.can |= mask;
							if (atoi(res))
								r->can.can.can |= mask;
						}
					}
				}
				nrules++;
			}
		}
	}
	// rules = erealloc(rules, nrules * sizeof(Rule *));
	compileregs();
}
