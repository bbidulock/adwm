#include <regex.h>
#include <ctype.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include "adwm.h"
#include "config.h"

static void
k_togglefloating(const char *arg) {
	if (sel) togglefloating(sel);
}

static void
k_togglefill(const char *arg) {
	if (sel) togglefill(sel);
}

static void
k_togglemaxv(const char *arg) {
	if (sel) togglemaxv(sel);
}

static void
k_togglemaxh(const char *arg) {
	if (sel) togglemaxh(sel);
}

static void
k_toggleshade(const char *arg) {
	if (sel) toggleshade(sel);
}

static void
k_togglehidden(const char *arg) {
	if (sel) togglehidden(sel);
}

static void
k_zoom(const char *arg) {
	if (sel) zoom(sel);
}

static void
k_focusnext(const char *arg) {
	if (sel) focusnext(sel);
}

static void
k_iconify(const char *arg) {
	if (sel) iconify(sel);
}

static void
k_focusprev(const char *arg) {
	if (sel) focusprev(sel);
}

static void
k_killclient(const char *arg) {
	if (sel) killclient(sel);
}

static void
k_moveresizekb(const char *arg) {
	if (sel) {
		int dw = 0, dh = 0, dx = 0, dy = 0;

		sscanf(arg, "%d %d %d %d", &dx, &dy, &dw, &dh);
		moveresizekb(sel, dx, dy, dw, dh);
	}
}

static void
k_rotateview(const char *arg) {
	rotateview(sel);
}

static void
k_unrotateview(const char *arg) {
	unrotateview(sel);
}

static void
k_rotatezone(const char *arg) {
	rotatezone(sel);
}

static void
k_unrotatezone(const char *arg) {
	unrotatezone(sel);
}

static void
k_rotatewins(const char *arg) {
	rotatewins(sel);
}

static void
k_unrotatewins(const char *arg) {
	unrotatewins(sel);
}

static void
k_togglestruts(const char *arg) {
	togglestruts();
}

static void
k_toggledectiled(const char *arg) {
	toggledectiled();
}

static void
k_focusicon(const char *arg) {
	focusicon();
}

static void
k_viewprevtag(const char *arg) {
	viewprevtag();
}

static void
k_viewlefttag(const char *arg) {
	viewlefttag();
}

static void
k_viewrighttag(const char *arg) {
	viewrighttag();
}

static void
k_togglemonitor(const char *arg) {
	togglemonitor();
}

static void
k_appendtag(const char *arg) {
	appendtag();
}

static void
k_rmlasttag(const char *arg) {
	rmlasttag();
}

typedef struct {
	const char *name;
	void (*action) (const char *arg);
} KeyItem;

static KeyItem KeyItems[] = {
	{ "togglestruts",	k_togglestruts	 },
	{ "toggledectiled",	k_toggledectiled },
	{ "focusicon",		k_focusicon	 },
	{ "focusnext",		k_focusnext	 },
	{ "focusprev",		k_focusprev	 },
	{ "viewprevtag",	k_viewprevtag	 },
	{ "viewlefttag",	k_viewlefttag	 },
	{ "viewrighttag",	k_viewrighttag	 },
	{ "quit",		quit		 }, /* arg is new command */
	{ "restart", 		restart		 }, /* arg is new command */
	{ "killclient",		k_killclient	 },
	{ "togglefloating", 	k_togglefloating },
	{ "decmwfact", 		setmwfact	 }, /* arg is delta or factor */
	{ "incmwfact", 		setmwfact	 }, /* arg is delta or factor */
	{ "incnmaster", 	incnmaster	 }, /* arg is delta */
	{ "decnmaster", 	incnmaster	 }, /* arg is delta */
	{ "iconify", 		k_iconify	 },
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
	{ "togglefill", 	k_togglefill	 },
	{ "togglemaxv", 	k_togglemaxv	 },
	{ "togglemaxh", 	k_togglemaxh	 },
	{ "toggleshade", 	k_toggleshade	 },
	{ "togglehidden", 	k_togglehidden	 },
	{ "appendtag",		k_appendtag	 },
	{ "rmlasttag",		k_rmlasttag	 },
	{ "rotateview",		k_rotateview	 },
	{ "unrotateview",	k_unrotateview	 },
	{ "rotatezone",		k_rotatezone	 },
	{ "unrotatezone",	k_unrotatezone	 },
	{ "rotatewins",		k_rotatewins	 },
	{ "unrotatewins",	k_unrotatewins	 },
};

static int
idxoftag(const char *tag) {
	unsigned int i;

	if (tag == NULL)
		return -1;
	for (i = 0; (i < scr->ntags) && strcmp(tag, scr->tags[i]); i++);
	return i < scr->ntags ? i : 0;
}

static void
k_toggletag(const char *arg) {
	if (sel) toggletag(sel, idxoftag(arg));
}

static void
k_tag(const char *arg) {
	if (sel) tag(sel, idxoftag(arg));
}

static void
k_focusview(const char *arg) {
	focusview(idxoftag(arg));
}

static void
k_toggleview(const char *arg) {
	toggleview(idxoftag(arg));
}

static void
k_view(const char *arg) {
	view(idxoftag(arg));
}

static KeyItem KeyItemsByTag[] = {
	{ "view",		k_view		},
	{ "toggleview",		k_toggleview	},
	{ "focusview",		k_focusview	},
	{ "tag", 		k_tag		},
	{ "toggletag", 		k_toggletag	},
};

static void
parsekey(const char *s, Key *k) {
	int l = strlen(s);
	unsigned long modmask = 0;
	char *pos, *opos;
	const char *stmp;
	char *tmp;
	int i;

	pos = strchr(s, '+');
	if ((pos - s) && pos) {
		for (i = 0, stmp = s; stmp < pos; i++, stmp++) {
			switch(*stmp) {
			case 'A':
				modmask = modmask | Mod1Mask;
				break;
			case 'S':
				modmask = modmask | ShiftMask;
				break;
			case 'C':
				modmask = modmask | ControlMask;
				break;
			case 'W':
				modmask = modmask | Mod4Mask;
				break;
			}
		}
	} else
		pos = (char *) s;
	opos = pos;
	k->mod = modmask;
	pos = strchr(s, '=');
	if (pos) {
		if (pos - opos < 0)
			opos = (char *) s;
		tmp = emallocz(pos - opos);
		for (; !isalnum(opos[0]); opos++);
		strncpy(tmp, opos, pos - opos - 1);
		k->keysym = XStringToKeysym(tmp);
		free(tmp);
		tmp = emallocz((s + l - pos + 1));
		for (pos++; !isgraph(pos[0]); pos++);
		strncpy(tmp, pos, s + l - pos);
		k->arg = tmp;
	} else {
		tmp = emallocz((s + l - opos));
		for (opos++; !isalnum(opos[0]); opos++);
		strncpy(tmp, opos, s + l - opos);
		k->keysym = XStringToKeysym(tmp);
		free(tmp);
	}
}

static void
initmodkey() {
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

int
initkeys() {
	unsigned int i, j;
	const char *tmp;
	char t[64];

	initmodkey();
	scr->keys = malloc(sizeof(Key *) * LENGTH(KeyItems));
	/* global functions */
	for (i = 0; i < LENGTH(KeyItems); i++) {
		tmp = getresource(KeyItems[i].name, NULL);
		if (!tmp)
			continue;
		scr->keys[scr->nkeys] = malloc(sizeof(Key));
		scr->keys[scr->nkeys]->func = KeyItems[i].action;
		scr->keys[scr->nkeys]->arg = NULL;
		parsekey(tmp, scr->keys[scr->nkeys]);
		scr->nkeys++;
	}
	/* per tag functions */
	for (j = 0; j < LENGTH(KeyItemsByTag); j++) {
		for (i = 0; i < scr->ntags; i++) {
			snprintf(t, sizeof(t), "%s%d", KeyItemsByTag[j].name, i);
			tmp = getresource(t, NULL);
			if (!tmp)
				continue;
			scr->keys = realloc(scr->keys, sizeof(Key *) * (scr->nkeys + 1));
			scr->keys[scr->nkeys] = malloc(sizeof(Key));
			scr->keys[scr->nkeys]->func = KeyItemsByTag[j].action;
			scr->keys[scr->nkeys]->arg = scr->tags[i];
			parsekey(tmp, scr->keys[scr->nkeys]);
			scr->nkeys++;
		}
	}
	/* layout setting */
	for (i = 0; layouts[i].symbol != '\0'; i++) {
		snprintf(t, sizeof(t), "setlayout%c", layouts[i].symbol);
		tmp = getresource(t, NULL);
		if (!tmp)
			continue;
		scr->keys = realloc(scr->keys, sizeof(Key *) * (scr->nkeys + 1));
		scr->keys[scr->nkeys] = malloc(sizeof(Key));
		scr->keys[scr->nkeys]->func = setlayout;
		scr->keys[scr->nkeys]->arg = &layouts[i].symbol;
		parsekey(tmp, scr->keys[scr->nkeys]);
		scr->nkeys++;
	}
	/* spawn */
	for (i = 0; i < 64; i++) {
		snprintf(t, sizeof(t), "spawn%d", i);
		tmp = getresource(t, NULL);
		if (!tmp)
			continue;
		scr->keys = realloc(scr->keys, sizeof(Key *) * (scr->nkeys + 1));
		scr->keys[scr->nkeys] = malloc(sizeof(Key));
		scr->keys[scr->nkeys]->func = spawn;
		scr->keys[scr->nkeys]->arg = NULL;
		parsekey(tmp, scr->keys[scr->nkeys]);
		scr->nkeys++;
	}
	return 0;
}

static void
parserule(const char *s, Rule *r) {
	r->prop = emallocz(128);
	r->tags = emallocz(64);
	sscanf(s, "%s %s %d %d", r->prop, r->tags, &r->isfloating, &r->hastitle);
}

static void
compileregs(void) {
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
initrules() {
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
	rules = realloc(rules, nrules * sizeof(Rule *));
	compileregs();
}
