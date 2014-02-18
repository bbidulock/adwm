#include <regex.h>
#include <ctype.h>
#include <assert.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include "adwm.h"
#include "config.h"

static void
k_zoom(XEvent *e, Key *k) {
	if (sel) zoom(sel);
}

static void
k_focusnext(XEvent *e, Key *k) {
	if (sel) focusnext(sel);
}

static void
k_focusprev(XEvent *e, Key *k) {
	if (sel) focusprev(sel);
}

static void
k_killclient(XEvent *e, Key *k) {
	if (sel) killclient(sel);
}

static void
k_moveresizekb(XEvent *e, Key *k) {
	if (sel) {
		int dw = 0, dh = 0, dx = 0, dy = 0;

		sscanf(k->arg, "%d %d %d %d", &dx, &dy, &dw, &dh);
		moveresizekb(sel, dx, dy, dw, dh);
	}
}

static void
k_rotateview(XEvent *e, Key *k) {
	rotateview(sel);
}

static void
k_unrotateview(XEvent *e, Key *k) {
	unrotateview(sel);
}

static void
k_rotatezone(XEvent *e, Key *k) {
	rotatezone(sel);
}

static void
k_unrotatezone(XEvent *e, Key *k) {
	unrotatezone(sel);
}

static void
k_rotatewins(XEvent *e, Key *k) {
	rotatewins(sel);
}

static void
k_unrotatewins(XEvent *e, Key *k) {
	unrotatewins(sel);
}

static void
k_focusicon(XEvent *e, Key *k) {
	focusicon();
}

static void
k_viewprevtag(XEvent *e, Key *k) {
	viewprevtag();
}

static void
k_viewlefttag(XEvent *e, Key *k) {
	viewlefttag();
}

static void
k_viewrighttag(XEvent *e, Key *k) {
	viewrighttag();
}

static void
k_togglemonitor(XEvent *e, Key *k) {
	togglemonitor();
}

static void
k_appendtag(XEvent *e, Key *k) {
	appendtag();
}

static void
k_rmlasttag(XEvent *e, Key *k) {
	rmlasttag();
}

static void
k_raise(XEvent *e, Key *k)
{
	if (sel) raiseclient(sel);
}

static void
k_lower(XEvent *e, Key *k)
{
	if (sel) lowerclient(sel);
}

static void
k_raiselower(XEvent *e, Key *k)
{
	if (sel) raiselower(sel);
}

static void
k_quit(XEvent *e, Key *k)
{
	quit(k->arg);
}

static void
k_restart(XEvent *e, Key *k)
{
	restart(k->arg);
}

typedef struct {
	const char *name;
	void (*action) (XEvent *e, Key *k);
} KeyItem;

static KeyItem KeyItems[] = {
	/* *INDENT-OFF* */
	{ "focusicon",		k_focusicon	 },
	{ "focusnext",		k_focusnext	 },
	{ "focusprev",		k_focusprev	 },
	{ "viewprevtag",	k_viewprevtag	 },
	{ "viewlefttag",	k_viewlefttag	 },
	{ "viewrighttag",	k_viewrighttag	 },
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

void
k_setmwfactor(XEvent *e, Key * k)
{
	Monitor *m;
	View *v;
	const char *arg;
	double factor;

	if (!(m = selmonitor()))
		return;
	v = scr->views + m->curtag;

	switch (k->act) {
	case IncCount:
		arg = k->arg ? : strdup("+5%");
		break;
	case DecCount:
		arg = k->arg ? : strdup("-5%");
		break;
	default:
	case SetCount:
		arg = k->arg ? : strdup(STR(DEFWMFACT));
		break;
	}
	if (sscanf(arg, "%lf", &factor) == 1) {
		if (strchr(arg, '%'))
			factor /= 100.0;
		if (arg[0] == '+' || arg[0] == '-' || k->act != SetCount) {
			switch (v->major) {
			case OrientTop:
			case OrientRight:
				if (k->act == DecCount)
					setmwfact(m, v, v->mwfact - factor);
				else
					setmwfact(m, v, v->mwfact + factor);
				break;
			case OrientBottom:
			case OrientLeft:
			case OrientLast:
				if (k->act == DecCount)
					setmwfact(m, v, v->mwfact + factor);
				else
					setmwfact(m, v, v->mwfact - factor);
				break;
			}
		} else
			setmwfact(m, v, factor);
	}
}

void
k_setnmaster(XEvent *e, Key * k)
{
	const char *arg;
	Monitor *m;
	View *v;
	int num;
	Bool master, column;

	if (!(m = selmonitor()))
		return;
	v = scr->views + m->curtag;
	master = FEATURES(v->layout, NMASTER) ? True : False;
	column = FEATURES(v->layout, NCOLUMNS) ? True : False;

	if (!master && !column)
		return;
	switch (k->act) {
	case IncCount:
		arg = k->arg ? : strdup("+1");
		break;
	case DecCount:
		arg = k->arg ? : strdup("-1");
		break;
	default:
	case SetCount:
		arg = k->arg ? : strdup(master ? STR(DEFNMASTER) : STR(DEFNCOLUMNS));
		break;
	}
	if (sscanf(arg, "%d", &num) == 1) {
		if (arg[0] == '+' || arg[0] == '-' || k->act != SetCount) {
			if (k->act == DecCount) {
				if (master)
					setnmaster(m, v, v->nmaster - num);
				else
					setnmaster(m, v, v->ncolumns - num);
			} else {
				if (master)
					setnmaster(m, v, v->nmaster + num);
				else
					setnmaster(m, v, v->ncolumns + num);
			}
		} else
			setnmaster(m, v, num);
	}
}

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
	{ "ncolumns",		k_setnmaster	 }
	/* *INDENT-ON* */
};

static void
k_setgeneric(XEvent *e, Key *k, void(*func)(XEvent *, Key *, Client *))
{
	switch (k->any) {
		Monitor *m;
		Client *c;
	case FocusClient:
		if (sel)
			func(e, k, sel);
		break;
	case AllClients:
		if (!(m = selmonitor()))
			return;
		for (c = scr->clients; c; c = c->next) {
			if (c->curmon != m)
				continue;
			func(e, k, c);
		}
		break;
	case AnyClient:
		for (c = scr->clients; c; c = c->next) {
			if (!c->curmon)
				continue;
			func(e, k, c);
		}
		break;
	case EveryClient:
		for (c = scr->clients; c; c = c->next) {
			func(e, k, c);
		}
		break;
	default:
		break;
	}
}

static void
c_setfloating(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->skip.arrange)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->skip.arrange)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglefloating(c);
}

static void
k_setfloating(XEvent *e, Key *k)
{
	return k_setgeneric(e, k, &c_setfloating);
}


static void
c_setfill(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->is.fill)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->is.fill)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglefill(c);
}

static void
k_setfill(XEvent *e, Key *k)
{
	return k_setgeneric(e, k, &c_setfill);
}

static void
c_setfull(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->is.full)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->is.full)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglefull(c);
}

static void
k_setfull(XEvent *e, Key *k)
{
	return k_setgeneric(e, k, &c_setfull);
}

static void
c_setmax(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->is.max)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->is.max)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglemax(c);
}

static void
k_setmax(XEvent *e, Key *k)
{
	return k_setgeneric(e, k, &c_setmax);
}

static void
c_setmaxv(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->is.maxv)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->is.maxv)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglemaxv(c);
}

static void
k_setmaxv(XEvent *e, Key *k)
{
	return k_setgeneric(e, k, &c_setmaxv);
}


static void
c_setmaxh(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->is.maxh)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->is.maxh)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglemaxh(c);
}

static void
k_setmaxh(XEvent *e, Key *k)
{
	return k_setgeneric(e, k, &c_setmaxh);
}

static void
c_setshade(XEvent *e, Key * k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->is.shaded)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->is.shaded)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	toggleshade(c);
}

static void
k_setshade(XEvent *e, Key * k)
{
	k_setgeneric(e, k, &c_setshade);
}

static void
c_sethidden(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->is.hidden)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->is.hidden)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglehidden(c);
}

static void
k_sethidden(XEvent *e, Key *k)
{
	k_setgeneric(e, k, &c_sethidden);
}

static void
c_setmin(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->is.icon)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->is.icon)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglemin(c);
}

static void
k_setmin(XEvent *e, Key *k)
{
	k_setgeneric(e, k, &c_setmin);
}

static void
c_setabove(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->is.above)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->is.above)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	toggleabove(c);
}

static void
k_setabove(XEvent *e, Key *k)
{
	k_setgeneric(e, k, &c_setabove);
}

static void
c_setbelow(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->is.below)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->is.below)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglebelow(c);
}

static void
k_setbelow(XEvent *e, Key *k)
{
	k_setgeneric(e, k, &c_setbelow);
}

static void
c_setpager(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (!c->skip.pager)
			return;
		break;
	case UnsetFlagSetting:
		if (c->skip.pager)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglepager(c);
}

static void
k_setpager(XEvent *e, Key *k)
{
	k_setgeneric(e, k, &c_setpager);
}

static void
c_settaskbar(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (!c->skip.taskbar)
			return;
		break;
	case UnsetFlagSetting:
		if (c->skip.taskbar)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	toggletaskbar(c);
}

static void
k_settaskbar(XEvent *e, Key *k)
{
	k_setgeneric(e, k, &c_settaskbar);
}

static void
k_setscrgeneric(XEvent *e, Key *k, void(*func)(XEvent *, Key *, AScreen *))
{
	switch (k->any) {
		AScreen *s = NULL;
		Window w, proot;
		unsigned int mask;
		int d;

	case FocusClient:
		if (sel && (s = getscreen(sel->win)))
			func(e, k, s);
		break;
	case PointerClient:
		XQueryPointer(dpy, scr->root, &proot, &w, &d, &d, &d, &d, &mask);
		if ((s = getscreen(proot)))
			func(e, k, s);
		break;
	case AllClients:
	case AnyClient:
		func(e, k, scr);
		break;
	case EveryClient:
		for (s = screens; s < screens + nscr; scr++)
			if (s->managed)
				func(e, k, s);
		break;
	default:
		break;
	}
}

static void
s_setshowing(XEvent *e, Key *k, AScreen * s)
{
	AScreen *old;

	switch (k->set) {
	case SetFlagSetting:
		if (s->showing_desktop)
			return;
		break;
	case UnsetFlagSetting:
		if (!s->showing_desktop)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	/* have to juggle the global */
	old = scr;
	scr = s;
	toggleshowing();
	scr = old;
}

static void
k_setshowing(XEvent *e, Key *k)
{
	return k_setscrgeneric(e, k, &s_setshowing);
}

static void
k_setlaygeneric(XEvent *e, Key *k, void (*func) (XEvent *, Key *, Monitor *, View *))
{
	switch (k->any) {
		Monitor *m;
		View *v;

	case FocusClient:
		if (!(m = selmonitor()))
			return;
		v = scr->views + m->curtag;
		func(e, k, m, v);
		break;
	case PointerClient:
		if (!(m = curmonitor()))
			m = nearmonitor();
		v = scr->views + m->curtag;
		func(e, k, m, v);
		break;
	case AllClients:
		for (m = scr->monitors; m; m = m->next) {
			v = scr->views + m->curtag;
			func(e, k, m, v);
		}
		break;
	case AnyClient:
		if (!(m = selmonitor()))
			m = nearmonitor();
		v = scr->views + m->curtag;
		func(e, k, m, v);
		break;
	case EveryClient:
		for (v = scr->views; v < scr->views + scr->ntags; v++) {
			for (m = scr->monitors; m; m = m->next)
				if (v == scr->views + m->curtag)
					break;
			func(e, k, m, v);
		}
		break;
	default:
		break;
	}
}

static void
v_setstruts(XEvent *e, Key *k, Monitor *m, View *v)
{
	switch (k->set) {
	case SetFlagSetting:
		if (v->barpos == StrutsOn)
			return;
		break;
	case UnsetFlagSetting:
		if (v->barpos != StrutsOn)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglestruts(m, v);
}

static void
k_setstruts(XEvent *e, Key *k)
{
	return k_setlaygeneric(e, k, &v_setstruts);
}

static void
v_setdectiled(XEvent *e, Key *k, Monitor *m, View *v)
{
	switch (k->set) {
	case SetFlagSetting:
		if (v->dectiled)
			return;
		break;
	case UnsetFlagSetting:
		if (!v->dectiled)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	toggledectiled(m, v);
}

static void
k_setdectiled(XEvent *e, Key *k)
{
	return k_setlaygeneric(e, k, &v_setdectiled);
}



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
	{ "",		FocusClient	    },
	{ "ptr",	PointerClient	    },
	{ "all",	AllClients	    }, /* all on current monitor */
	{ "any",	AnyClient	    }, /* clients on any monitor */
	{ "every",	EveryClient	    }  /* clients on every workspace */
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

static void
k_moveto(XEvent *e, Key *k) {
	if (sel)
		moveto(sel, k->dir);
}

static void
k_snapto(XEvent *e, Key *k) {
	if (sel)
		snapto(sel, k->dir);
}

static void
k_edgeto(XEvent *e, Key *k) {
	if (sel)
		edgeto(sel, k->dir);
}

static void
k_moveby(XEvent *e, Key *k) {
	if (sel) {
		int amount = 1;
		if (k->arg)
			sscanf(k->arg, "%d", &amount);
		moveby(sel, k->dir, amount);
	}
}

static const struct {
	const char *suffix;
	RelativeDirection dir;
} lst_suffix[] = {
	/* *INDENT-OFF* */
	{ "next",	RelativeNext	},
	{ "prev",	RelativePrev	},
	{ "last",	RelativeLast	}
	/* *INDENT-ON* */
};

static const struct {
	const char *prefix;
	WhichList list;
} lst_prefix[] = {
	/* *INDENT-OFF* */
	{ "focus",	ListFocus	},
	{ "active",	ListActive	},
	{ "group",	ListGroup	},
	{ "client",	ListClient	},
	{ "tab",	ListTab		}
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

static View *
getviewrandc(int r, int c)
{
	int idx;
	View *v;

	for (v = scr->views, idx = 0; idx < scr->ntags; idx++, v++)
		if (v->row == r && v->col == c)
			break;
	if (idx >= scr->ntags)
		v = NULL;
	return v;
}

static int
_getviewrandc(View *v, int dr, int dc)
{
	int r, c, idx, iter;

	r = v->row;
	c = v->col;
	idx = v->index;
	iter = 0;
	assert(scr->d.rows > 0);
	assert(scr->d.cols > 0);
	do {
		r += dr;
		while (r < 0)
			r += scr->d.rows;
		r = r % scr->d.rows;
		c += dc;
		while (c < 0)
			c += scr->d.rows;
		c = c % scr->d.cols;
	} while (!(v = getviewrandc(r, c)) && ++iter < scr->ntags);
	if (v)
		idx = v->index;
	else
		DPRINTF("ERROR: could not find view idx = %d, dr = %d, dc = %d\n", idx,
			dr, dc);
	return idx;
}

static int
idxoftag(View *v, Key *k)
{
	const char *arg;
	const char *one = "1";
	int d, dr, dc, idx;
	RelativeDirection dir;

	idx = v->index;

	assert(scr->ntags > 0);

	arg = k->arg ? : strdup(one);
	dir = k->dir;
	if (sscanf(arg, "%d", &d) == 1) {
		if ((arg[0] == '+' || arg[0] == '-') && dir == RelativeNone)
			dir = RelativeNext;
	} else
		d = 1;
	switch (dir) {
	case RelativeNone:
		idx = k->tag;
		goto done;
	case RelativeNext:
		idx += d;
		goto done;
	case RelativePrev:
		idx -= d;
		goto done;
	case RelativeLast:
		if (0 <= scr->last && scr->last < scr->ntags)
			idx = scr->last;
		goto done;
	case RelativeNorthWest:
		switch (scr->layout.start) {
		default:
		case _NET_WM_TOPLEFT:
			dr = -abs(d);
			dc = -abs(d);
			break;
		case _NET_WM_TOPRIGHT:
			dr = -abs(d);
			dc = abs(d);
			break;
		case _NET_WM_BOTTOMRIGHT:
			dr = abs(d);
			dc = abs(d);
			break;
		case _NET_WM_BOTTOMLEFT:
			dr = abs(d);
			dc = -abs(d);
			break;
		}
		break;
	case RelativeNorth:
		switch (scr->layout.start) {
		default:
		case _NET_WM_TOPLEFT:
		case _NET_WM_TOPRIGHT:
			dr = -abs(d);
			dc = 0;
			break;
		case _NET_WM_BOTTOMRIGHT:
		case _NET_WM_BOTTOMLEFT:
			dr = abs(d);
			dc = 0;
			break;
		}
		break;
	case RelativeNorthEast:
		switch (scr->layout.start) {
		default:
		case _NET_WM_TOPLEFT:
			dr = -abs(d);
			dc = abs(d);
			break;
		case _NET_WM_TOPRIGHT:
			dr = -abs(d);
			dc = -abs(d);
			break;
		case _NET_WM_BOTTOMRIGHT:
			dr = abs(d);
			dc = -abs(d);
			break;
		case _NET_WM_BOTTOMLEFT:
			dr = abs(d);
			dc = abs(d);
			break;
		}
		break;
	case RelativeWest:
		switch (scr->layout.start) {
		default:
		case _NET_WM_TOPLEFT:
		case _NET_WM_BOTTOMLEFT:
			dr = 0;
			dc = -abs(d);
			break;
		case _NET_WM_TOPRIGHT:
		case _NET_WM_BOTTOMRIGHT:
			dr = 0;
			dc = abs(d);
			break;
		}
		break;
	case RelativeEast:
		switch (scr->layout.start) {
		default:
		case _NET_WM_TOPLEFT:
		case _NET_WM_BOTTOMLEFT:
			dr = 0;
			dc = abs(d);
			break;
		case _NET_WM_TOPRIGHT:
		case _NET_WM_BOTTOMRIGHT:
			dr = 0;
			dc = -abs(d);
			break;
		}
		break;
	case RelativeSouthWest:
		switch (scr->layout.start) {
		default:
		case _NET_WM_TOPLEFT:
			dr = abs(d);
			dc = -abs(d);
			break;
		case _NET_WM_TOPRIGHT:
			dr = abs(d);
			dc = abs(d);
			break;
		case _NET_WM_BOTTOMRIGHT:
			dr = -abs(d);
			dc = abs(d);
			break;
		case _NET_WM_BOTTOMLEFT:
			dr = -abs(d);
			dc = -abs(d);
			break;
		}
		break;
	case RelativeSouth:
		switch (scr->layout.start) {
		default:
		case _NET_WM_TOPLEFT:
		case _NET_WM_TOPRIGHT:
			dr = abs(d);
			dc = 0;
			break;
		case _NET_WM_BOTTOMRIGHT:
		case _NET_WM_BOTTOMLEFT:
			dr = -abs(d);
			dc = 0;
			break;
		}
		break;
	case RelativeSouthEast:
		switch (scr->layout.start) {
		default:
		case _NET_WM_TOPLEFT:
			dr = abs(d);
			dc = abs(d);
			break;
		case _NET_WM_TOPRIGHT:
			dr = abs(d);
			dc = -abs(d);
			break;
		case _NET_WM_BOTTOMRIGHT:
			dr = -abs(d);
			dc = -abs(d);
			break;
		case _NET_WM_BOTTOMLEFT:
			dr = -abs(d);
			dc = abs(d);
			break;
		}
		break;
	default:
		goto done;
	}
	idx = _getviewrandc(v, dr, dc);
      done:
	while (idx < 0)
		idx += scr->ntags;
	idx = idx % scr->ntags;
	return idx;
}

static void
k_toggletag(XEvent *e, Key *k)
{
	if (sel) {
		Monitor *m;
		View *v;

		if (!(m = sel->curmon))
			return;
		v = scr->views + m->curtag;
		toggletag(sel, idxoftag(v, k));
	}
}

static void
k_tag(XEvent *e, Key *k)
{
	if (sel) {
		Monitor *m;
		View *v;

		if (!(m = sel->curmon))
			return;
		v = scr->views + m->curtag;
		tag(sel, idxoftag(v, k));
	}
}

static void
k_focusview(XEvent *e, Key *k)
{
	Monitor *cm;
	View *v;

	if (!(cm = selmonitor()))
		cm = nearmonitor();
	v = scr->views + cm->curtag;
	focusview(cm, idxoftag(v, k));
}

static void
k_toggleview(XEvent *e, Key *k)
{
	Monitor *cm;
	View *v;

	if (!(cm = selmonitor()))
		cm = nearmonitor();
	v = scr->views + cm->curtag;
	toggleview(cm, idxoftag(v, k));
}

static void
k_view(XEvent *e, Key *k)
{
	Monitor *cm;
	View *v;

	if (!(cm = selmonitor()))
		cm = nearmonitor();
	v = scr->views + cm->curtag;
	view(idxoftag(v, k));
}

static void
k_taketo(XEvent *e, Key *k)
{
	if (sel) {
		Monitor *m;
		View *v;

		if (!(m = sel->curmon))
			return;
		v = scr->views + m->curtag;
		taketo(sel, idxoftag(v, k));
	}
}

static const struct {
	const char *suffix;
	RelativeDirection dir;
} tag_suffix[] = {
	/* *INDENT-OFF* */
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

static KeyItem KeyItemsByTag[] = {
	{ "view",		k_view		},
	{ "toggleview",		k_toggleview	},
	{ "focusview",		k_focusview	},
	{ "tag", 		k_tag		},
	{ "toggletag", 		k_toggletag	},
	{ "taketo",		k_taketo	}
};

#define CLEANMASK(mask) (mask & ~(numlockmask | LockMask))

static void
k_chain(XEvent *e, Key *key)
{
	XEvent ev;
	KeySym keysym;
	Key *k;

	XMaskEvent(dpy, KeyPressMask, &ev);

	keysym = XkbKeycodeToKeysym(dpy, (KeyCode) ev.xkey.keycode, 0, 0);

	for (k = key->cnext; k; k = k->cnext)
		if (keysym == k->keysym && CLEANMASK(ev.xkey.state) == k->mod) {
			if (k->func)
				k->func(&ev, k);
			return;
		}
}

static void
freekey(Key *k)
{
	if (k) {
		free(k->arg);
		free(k);
	}
}

static void
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

static void
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
	for (; e > s && isblank(*(e - 1)); e--);
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

static void
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

static void
k_setlayout(XEvent *e, Key *k)
{
	setlayout(k->arg);
}

static void
k_spawn(XEvent *e, Key *k)
{
	spawn(k->arg);
}

int
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
		for (i = 0; i < 32; i++) {
			Key key = { 0, };

			snprintf(t, sizeof(t), "%s%d", KeyItemsByTag[j].name, i);
			DPRINTF("Check for key item '%s'\n", t);
			tmp = getresource(t, NULL);
			if (!tmp)
				continue;
			key.func = KeyItemsByTag[j].action;
			key.arg = NULL;
			key.dir = RelativeNone;
			key.tag = i;
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
			key.dir = tag_suffix[i].dir;
			key.tag = 0;
			parsekeys(tmp, &key);
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
	// rules = erealloc(rules, nrules * sizeof(Rule *));
	compileregs();
}
