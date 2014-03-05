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
#include "parse.h" /* verification */

void
k_zoom(XEvent *e, Key *k)
{
	if (sel)
		zoom(sel);
}

void
k_killclient(XEvent *e, Key *k)
{
	if (sel)
		killclient(sel);
}

void
k_moveresizekb(XEvent *e, Key *k)
{
	if (sel) {
		int dw = 0, dh = 0, dx = 0, dy = 0;

		sscanf(k->arg, "%d %d %d %d", &dx, &dy, &dw, &dh);
		moveresizekb(sel, dx, dy, dw, dh, sel->gravity);
	}
}

void
k_rotateview(XEvent *e, Key *k)
{
	rotateview(sel);
}

void
k_unrotateview(XEvent *e, Key *k)
{
	unrotateview(sel);
}

void
k_rotatezone(XEvent *e, Key *k)
{
	rotatezone(sel);
}

void
k_unrotatezone(XEvent *e, Key *k)
{
	unrotatezone(sel);
}

void
k_rotatewins(XEvent *e, Key *k)
{
	rotatewins(sel);
}

void
k_unrotatewins(XEvent *e, Key *k)
{
	unrotatewins(sel);
}

void
k_viewprevtag(XEvent *e, Key *k)
{
	viewprevtag();
}

void
k_togglemonitor(XEvent *e, Key *k)
{
	togglemonitor();
}

void
k_appendtag(XEvent *e, Key *k)
{
	appendtag();
}

void
k_rmlasttag(XEvent *e, Key *k)
{
	rmlasttag();
}

void
k_raise(XEvent *e, Key *k)
{
	if (sel)
		raiseclient(sel);
}

void
k_lower(XEvent *e, Key *k)
{
	if (sel)
		lowerclient(sel);
}

void
k_raiselower(XEvent *e, Key *k)
{
	if (sel)
		raiselower(sel);
}

void
k_quit(XEvent *e, Key *k)
{
	quit(k->arg);
}

void
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

void
k_setmwfactor(XEvent *e, Key *k)
{
	View *v;
	const char *arg;
	double factor;

	if (!(v = selview()))
		return;

	switch (k->act) {
	case IncCount:
		arg = k->arg ? : strdup("+5%");
		break;
	case DecCount:
		arg = k->arg ? : strdup("+5%");
		break;
	default:
	case SetCount:
		arg = k->arg ? : strdup(STR(DEFWMFACT));
		break;
	}
	if (sscanf(arg, "%lf", &factor) == 1) {
		if (strchr(arg, '%'))
			factor /= 100.0;
		if (*arg == '+' || *arg == '-' || k->act != SetCount) {
			switch (v->major) {
			case OrientTop:
			case OrientRight:
				if (k->act == DecCount)
					setmwfact(v, v->mwfact + factor);
				else
					setmwfact(v, v->mwfact - factor);
				break;
			case OrientBottom:
			case OrientLeft:
			case OrientLast:
				if (k->act == DecCount)
					setmwfact(v, v->mwfact - factor);
				else
					setmwfact(v, v->mwfact + factor);
				break;
			}
		} else
			setmwfact(v, factor);
	}
}

void
k_setnmaster(XEvent *e, Key *k)
{
	const char *arg;
	View *v;
	int num;

	if (!(v = selview()))
		return;

	switch (k->act) {
	case IncCount:
		arg = k->arg ? : "+1";
		break;
	case DecCount:
		arg = k->arg ? : "+1";
		break;
	default:
	case SetCount:
		arg = k->arg ? : "0";
		break;
	}
	if (sscanf(arg, "%d", &num) == 1) {
		if (arg[0] == '+' || arg[0] == '-' || k->act != SetCount) {
			if (k->act == DecCount)
				decnmaster(v, num);
			else
				incnmaster(v, num);
		} else
			setnmaster(v, num);
	}
}

void
k_setmargin(XEvent *e, Key *k)
{
	const char *arg;
	int num;

	switch (k->act) {
	case IncCount:
		arg = k->arg ? : strdup("+1");
		break;
	case DecCount:
		arg = k->arg ? : strdup("+1");
		break;
	default:
	case SetCount:
		arg = k->arg ? : strdup(STR(MARGINPX));
		break;
	}
	if (sscanf(arg, "%d", &num) == 1) {
		if (*arg == '+' || *arg == '-' || k->act != SetCount) {
			if (k->act == DecCount)
				decmargin(num);
			else
				incmargin(num);
		} else
			setmargin(num);
	}
}

void
k_setborder(XEvent *e, Key *k)
{
	const char *arg;
	int num;

	switch (k->act) {
	case IncCount:
		arg = k->arg ? : strdup("+1");
		break;
	case DecCount:
		arg = k->arg ? : strdup("+1");
		break;
	default:
	case SetCount:
		arg = k->arg ? : strdup(STR(BORDERPX));
		break;
	}
	if (sscanf(arg, "%d", &num) == 1) {
		if (*arg == '+' || *arg == '-' || k->act != SetCount) {
			if (k->act == DecCount)
				decborder(num);
			else
				incborder(num);
		} else
			setborder(num);
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
	{ "ncolumns",		k_setnmaster	 },
	{ "margin",		k_setmargin	 },
	{ "border",		k_setborder	 }
	/* *INDENT-ON* */
};

static void
k_setgeneric(XEvent *e, Key *k, void (*func) (XEvent *, Key *, Client *))
{
	switch (k->any) {
		View *v;
		Client *c;

	case FocusClient:
		if (sel)
			func(e, k, sel);
		break;
	case AllClients:
		if (!(v = selview()))
			return;
		for (c = scr->clients; c; c = c->next) {
			if (!c->cview || c->cview != v)
				continue;
			switch (k->ico) {
			case IncludeIcons:
				break;
			case ExcludeIcons:
				if (c->is.icon || c->is.hidden)
					continue;
				break;
			case OnlyIcons:
				if (c->is.icon || c->is.hidden)
					break;
				continue;
			}
			func(e, k, c);
		}
		break;
	case AnyClient:
		for (c = scr->clients; c; c = c->next) {
			if (!c->cview || !c->cview->curmon)
				continue;
			switch (k->ico) {
			case IncludeIcons:
				break;
			case ExcludeIcons:
				if (c->is.icon || c->is.hidden)
					continue;
				break;
			case OnlyIcons:
				if (c->is.icon || c->is.hidden)
					break;
				continue;
			}
			func(e, k, c);
		}
		break;
	case EveryClient:
		for (c = scr->clients; c; c = c->next) {
			switch (k->ico) {
			case IncludeIcons:
				break;
			case ExcludeIcons:
				if (c->is.icon || c->is.hidden)
					continue;
				break;
			case OnlyIcons:
				if (c->is.icon || c->is.hidden)
					break;
				continue;
			}
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

void
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

void
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

void
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

void
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

void
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

void
k_setmaxh(XEvent *e, Key *k)
{
	return k_setgeneric(e, k, &c_setmaxh);
}

static void
c_setshade(XEvent *e, Key *k, Client *c)
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

void
k_setshade(XEvent *e, Key *k)
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

void
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

void
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

void
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

void
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

void
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

void
k_settaskbar(XEvent *e, Key *k)
{
	k_setgeneric(e, k, &c_settaskbar);
}

static void
k_setscrgeneric(XEvent *e, Key *k, void (*func) (XEvent *, Key *, AScreen *))
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
s_setshowing(XEvent *e, Key *k, AScreen *s)
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

void
k_setshowing(XEvent *e, Key *k)
{
	return k_setscrgeneric(e, k, &s_setshowing);
}

static void
k_setlaygeneric(XEvent *e, Key *k, void (*func) (XEvent *, Key *, View *))
{
	switch (k->any) {
		Monitor *m;
		View *v;

	case FocusClient:
		if (!(v = selview()))
			return;
		func(e, k, v);
		break;
	case PointerClient:
		if (!(v = nearview()))
			return;
		func(e, k, v);
		break;
	case AllClients:
		for (m = scr->monitors; m; m = m->next)
			func(e, k, m->curview);
		break;
	case AnyClient:
		if (!(v = selview()))
			return;
		if (!v)
			return;
		func(e, k, v);
		break;
	case EveryClient:
		for (v = scr->views; v < scr->views + scr->ntags; v++) {
			for (m = scr->monitors; m; m = m->next)
				if (v == m->curview)
					break;
			func(e, k, v);
		}
		break;
	default:
		break;
	}
}

static void
v_setstruts(XEvent *e, Key *k, View *v)
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
	togglestruts(v);
}

void
k_setstruts(XEvent *e, Key *k)
{
	return k_setlaygeneric(e, k, &v_setstruts);
}

static void
v_setdectiled(XEvent *e, Key *k, View *v)
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
	toggledectiled(v);
}

void
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

void
k_moveto(XEvent *e, Key *k)
{
	if (sel)
		moveto(sel, k->dir);
}

void
k_snapto(XEvent *e, Key *k)
{
	if (sel)
		snapto(sel, k->dir);
}

void
k_edgeto(XEvent *e, Key *k)
{
	if (sel)
		edgeto(sel, k->dir);
}

void
k_moveby(XEvent *e, Key *k)
{
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

Bool canfocus(Client *c);

static Bool
k_focusable(Client *c, View *v, WhichClient any, RelativeDirection dir,
	    IconsIncluded ico)
{
	if (dir == RelativeCenter)
		if (!c->is.icon && !c->is.hidden)
			return False;

	switch (any) {
	case PointerClient:
		return False;
	case FocusClient:
		if (!canfocus(c))
			return False;
		if (c->skip.focus)
			return False;
		if (dir != RelativeCenter)
			if (c->is.icon || c->is.hidden)
				return False;
		if (!isvisible(c, v))
			return False;
		switch (ico) {
		case IncludeIcons:
			break;
		case ExcludeIcons:
			if (c->is.icon || c->is.hidden)
				return False;
			break;
		case OnlyIcons:
			if (!c->is.icon && !c->is.hidden)
				return False;
			break;
		}
		break;
	case ActiveClient:
		if (!isvisible(c, v))
			return False;
		switch (ico) {
		case IncludeIcons:
			break;
		case ExcludeIcons:
			if (c->is.icon || c->is.hidden)
				return False;
			break;
		case OnlyIcons:
			if (!c->is.icon && !c->is.hidden)
				return False;
			break;
		}
		break;
	case AllClients:
		if (c->skip.focus)
			return False;
		if (!isvisible(c, v))
			return False;
		switch (ico) {
		case IncludeIcons:
			break;
		case ExcludeIcons:
			if (c->is.icon || c->is.hidden)
				return False;
			break;
		case OnlyIcons:
			if (!c->is.icon && !c->is.hidden)
				return False;
			break;
		}
		break;
	case AnyClient:
		if (c->skip.focus)
			return False;
		if (!isvisible(c, NULL))
			return False;
		switch (ico) {
		case IncludeIcons:
			break;
		case ExcludeIcons:
			if (c->is.icon || c->is.hidden)
				return False;
			break;
		case OnlyIcons:
			if (!c->is.icon && !c->is.hidden)
				return False;
			break;
		}
		break;
	case EveryClient:
		break;
	}
	return True;
}

void
k_stop(XEvent *e, Key *k)
{
	free(k->cycle);
	k->cycle = k->where = NULL;
	k->num = 0;
	k->stop = NULL;
}

static float
howfar(Client *c, Client *o, RelativeDirection *dir)
{
	int dx, dy;
	int mxc, myc, mxo, myo;

	mxc = c->c.x + c->c.b + c->c.w / 2;
	myc = c->c.y + c->c.b + c->c.h / 2;
	mxo = o->c.x + o->c.b + o->c.w / 2;
	myo = o->c.y + o->c.b + o->c.h / 2;
	dx = mxo - mxc;
	dy = myo - myc;
	if (!dx && !dy)
		*dir = RelativeCenter;
	else if (abs(dx) < abs(dy) / 6)
		*dir = (dy < 0) ? RelativeNorth : RelativeSouth;
	else if (abs(dy) < abs(dx) / 6)
		*dir = (dx < 0) ? RelativeWest : RelativeEast;
	else if (dx < 0 && dy < 0)
		*dir = RelativeNorthWest;
	else if (dx < 0 && dy > 0)
		*dir = RelativeSouthWest;
	else if (dx > 0 && dy < 0)
		*dir = RelativeNorthEast;
	else
		*dir = RelativeSouthEast;
	return hypotf(dx, dy);
}

static CycleList *
findclosest(Key *k, RelativeDirection dir)
{
	CycleList *cl, *which = NULL;
	float mindist = 0;

	if (!k->where)
		k->where = k->cycle;

	for (cl = k->where->next; cl != k->where; cl = cl->next) {
		RelativeDirection where;
		float res;

		res = howfar(k->where->c, cl->c, &where);
		if (where == dir && (!which || res < mindist)) {
			which = cl;
			mindist = res;
		}
	}
	return which;
}

static void
k_select_cl(View *v, Key *k, CycleList * cl)
{
	Client *c;
	int i;

	if (cl != k->where) {
		c = k->where->c;
		c->is.icon = c->was.icon;
		c->is.hidden = c->was.hidden;
		if (c->was.icon || c->was.hidden)
			arrange(c->cview);
		c->was.is = 0;
	}
	c = cl->c;
	if (!(v = c->cview) && !(v = clientview(c))) {
		if (!(c->tags & ((1ULL << scr->ntags) - 1))) {
			k_stop(NULL, k);
			return;
		}
		/* have to switch views on monitor v->curmon */
		for (i = 0; i < scr->ntags; i++)
			if (c->tags & (1ULL << i))
				break;
		view(v, i);
	}
	if (cl != k->where) {
		c->was.is = 0;
		if (c->is.icon || c->is.hidden) {
			if ((c->was.icon = c->is.icon))
				c->is.icon = False;
			if ((c->was.hidden = c->is.hidden))
				c->is.hidden = False;
		}
	}
	focus(c);
	if (k->cyc) {
		k->stop = k_stop;
		if (!XGrabKeyboard(dpy, scr->root, GrabModeSync, False,
				   GrabModeAsync, CurrentTime))
			return;
		DPRINTF("Could not grab keyboard\n");
	}
	k_stop(NULL, k);
}

static void
k_select_dir(View *v, Key *k, RelativeDirection dir)
{
	CycleList *cl;

	if (!(cl = findclosest(k, dir))) {
		k_stop(NULL, k);
		return;
	}
	k_select_cl(v, k, cl);
}

static void
k_select_lst(View *v, Key *k, RelativeDirection dir)
{
	const char *arg = k->arg;
	CycleList *cl;
	int d, i, j, idx;

	if (arg && (sscanf(arg, "%d", &d) == 1)) {
		if ((*arg == '+' || *arg == '-') && dir == RelativeNone)
			dir = RelativeNext;
	} else
		d = 1;

	switch (dir) {
	case RelativeNone:
		idx = k->tag;
		XPRINTF("Absolute index is %d\n", idx);
		break;
	case RelativeNext:
	case RelativeCenter:
		if (k->where) {
			i = ((char *) k->where - (char *) k->cycle) / sizeof(*k->where);
			XPRINTF("Index of selected is %d\n", i);
			idx = i + d;
		} else {
			k->where = k->cycle;
			idx = 0;
		}
		XPRINTF("Next index is %d\n", idx);
		break;
	case RelativePrev:
		if (k->where) {
			i = ((char *) k->where - (char *) k->cycle) / sizeof(*k->where);
			XPRINTF("Index of selected is %d\n", i);
			idx = i - d;
		} else {
			k->where = k->cycle;
			idx = 0;
		}
		XPRINTF("Previous index is %d\n", idx);
		break;
	case RelativeLast:
		if (k->where) {
			i = ((char *) k->where - (char *) k->cycle) / sizeof(*k->where);
			XPRINTF("Index of selected is %d\n", i);
			idx = (i == 0) ? 1 : 0;
		} else {
			k->where = k->cycle;
			idx = 0;
		}
		XPRINTF("Last index is %d\n", idx);
		break;
	default:
		k_stop(NULL, k);
		return;
	}
	cl = k->cycle;
	j = idx;
	for (; j < 0; cl = cl->prev, j++) ;
	for (; j > 0; cl = cl->next, j--) ;
	k_select_cl(v, k, cl);
}

static void
k_select(View *v, Key *k)
{
	switch (k->dir) {
	case RelativeNone:
	case RelativeNext:
	case RelativePrev:
	case RelativeLast:
	case RelativeCenter:
		k_select_lst(v, k, k->dir);
		break;
	case RelativeNorthWest:
	case RelativeNorth:
	case RelativeNorthEast:
	case RelativeWest:
	case RelativeEast:
	case RelativeSouthWest:
	case RelativeSouth:
	case RelativeSouthEast:
		k_select_dir(v, k, k->dir);
		break;
	default:
		k_stop(NULL, k);
		break;
	}
}

void
k_focus(XEvent *e, Key *k)
{
	View *v;

	if (!(v = selview()))
		return;
	/* active order */
	if (!k->cycle) {
		Client *c;
		int i, n;
		CycleList *cl;

		k->where = NULL;
		for (n = 0, c = scr->clients; c; c = c->next)
			if (k_focusable(c, v, k->any, k->dir, k->ico))
				n++;
		if (!n)
			return;
		cl = k->cycle = ecalloc(n, sizeof(*k->cycle));
		for (i = 0, c = scr->clients; c && i < n; c = c->next) {
			if (!k_focusable(c, v, k->any, k->dir, k->ico))
				continue;
			if (c == sel)
				k->where = cl;
			cl->c = c;
			cl->next = cl + 1;
			cl->prev = cl - 1;
			i++;
			cl++;
		}
		k->cycle[0].prev = &k->cycle[n - 1];
		k->cycle[n - 1].next = &k->cycle[0];
		k->num = n;
	}
	k_select(v, k);
}

void
k_client(XEvent *e, Key *k)
{
	View *v;

	if (!(v = selview()))
		return;
	/* client order */
	if (!k->cycle) {
		Client *c;
		int i, n;
		CycleList *cl;

		k->where = NULL;
		for (n = 0, c = scr->clist; c; c = c->cnext)
			if (k_focusable(c, v, k->any, k->dir, k->ico))
				n++;
		if (!n)
			return;
		cl = k->cycle = ecalloc(n, sizeof(*k->cycle));
		for (i = 0, c = scr->clist; c && i < n; c = c->cnext) {
			if (!k_focusable(c, v, k->any, k->dir, k->ico))
				continue;
			if (c == sel)
				k->where = cl;
			cl->c = c;
			cl->next = cl + 1;
			cl->prev = cl - 1;
			i++;
			cl++;
		}
		k->cycle[0].prev = &k->cycle[n - 1];
		k->cycle[n - 1].next = &k->cycle[0];
		k->num = n;
	}
	k_select(v, k);
}

void
k_stack(XEvent *e, Key *k)
{
	View *v;

	if (!(v = selview()))
		return;
	/* stacking order */
	if (!k->cycle) {
		Client *c;
		int i, n;
		CycleList *cl;

		k->where = NULL;
		for (n = 0, c = scr->stack; c; c = c->snext)
			if (k_focusable(c, v, k->any, k->dir, k->ico))
				n++;
		if (!n)
			return;
		cl = k->cycle = ecalloc(n, sizeof(*k->cycle));
		for (i = 0, c = scr->stack; c && i < n; c = c->snext) {
			if (!k_focusable(c, v, k->any, k->dir, k->ico))
				continue;
			if (c == sel)
				k->where = cl;
			cl->c = c;
			cl->next = cl + 1;
			cl->prev = cl - 1;
			i++;
			cl++;
		}
		k->cycle[0].prev = &k->cycle[n - 1];
		k->cycle[n - 1].next = &k->cycle[0];
		k->num = n;
	}
	k_select(v, k);
}

void
k_group(XEvent *e, Key *k)
{
	View *v;

	if (!(v = selview()))
		return;
	/* client order same WM_CLASS */
	if (!k->cycle) {
		Client *c;
		int i, n;
		CycleList *cl;

		/* FIXME: just stick within the same resource class */
		k->where = NULL;
		for (n = 0, c = scr->clist; c; c = c->cnext)
			if (k_focusable(c, v, k->any, k->dir, k->ico))
				n++;
		if (!n)
			return;
		cl = k->cycle = ecalloc(n, sizeof(*k->cycle));
		for (i = 0, c = scr->clist; c && i < n; c = c->cnext) {
			if (!k_focusable(c, v, k->any, k->dir, k->ico))
				continue;
			if (c == sel)
				k->where = cl;
			cl->c = c;
			cl->next = cl + 1;
			cl->prev = cl - 1;
			i++;
			cl++;
		}
		k->cycle[0].prev = &k->cycle[n - 1];
		k->cycle[n - 1].next = &k->cycle[0];
		k->num = n;
	}
	k_select(v, k);
}

void
k_tab(XEvent *e, Key *k)
{
	/* tag within tab group */
	/* TODO */
	return;
}

void
k_panel(XEvent *e, Key *k)
{
	View *v;

	if (!(v = selview()))
		return;
	/* panel (app with struts) */
	if (!k->cycle) {
		Client *c;
		int i, n;
		CycleList *cl;

		k->where = NULL;
		for (n = 0, c = scr->clist; c; c = c->cnext)
			if (WTCHECK(c, WindowTypeDock))
				if (k_focusable(c, v, EveryClient, k->dir, k->ico))
					n++;
		if (!n)
			return;
		cl = k->cycle = ecalloc(n, sizeof(*k->cycle));
		for (i = 0, c = scr->clist; c && i < n; c = c->cnext) {
			if (!WTCHECK(c, WindowTypeDock))
				continue;
			if (!k_focusable(c, v, EveryClient, k->dir, k->ico))
				continue;
			if (c == sel)
				k->where = cl;
			cl->c = c;
			cl->next = cl + 1;
			cl->prev = cl - 1;
			i++;
			cl++;
		}
		k->cycle[0].prev = &k->cycle[n - 1];
		k->cycle[n - 1].next = &k->cycle[0];
		k->num = n;
	}
	k_select(v, k);
}

void
k_dock(XEvent *e, Key *k)
{
	View *v;

	if (!(v = selview()))
		return;
	/* dock apps */
	if (!k->cycle) {
		Client *c;
		int i, n;
		CycleList *cl;

		k->where = NULL;
		for (n = 0, c = scr->clist; c; c = c->cnext)
			if (c->is.dockapp)
				if (k_focusable(c, v, EveryClient, k->dir, k->ico))
					n++;
		if (!n)
			return;
		cl = k->cycle = ecalloc(n, sizeof(*k->cycle));
		for (i = 0, c = scr->clist; c && i < n; c = c->cnext) {
			if (!c->is.dockapp)
				continue;
			if (!k_focusable(c, v, EveryClient, k->dir, k->ico))
				continue;
			if (c == sel)
				k->where = cl;
			cl->c = c;
			cl->next = cl + 1;
			cl->prev = cl - 1;
			i++;
			cl++;
		}
		k->cycle[0].prev = &k->cycle[n - 1];
		k->cycle[n - 1].next = &k->cycle[0];
		k->num = n;
	}
	k_select(v, k);
}

void
k_swap(XEvent *e, Key *k)
{
	/* TODO: swap windows */
	return;
}

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
_getviewrandc(View *v, Key *k, int dr, int dc)
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
		if (k->wrap) {
			while (r < 0)
				r += scr->d.rows;
			r = r % scr->d.rows;
		} else {
			if (r < 0)
				r = 0;
			if (r >= scr->d.rows)
				r = scr->d.rows - 1;
		}
		c += dc;
		if (k->wrap) {
			while (c < 0)
				c += scr->d.rows;
			c = c % scr->d.cols;
		} else {
			if (c < 0)
				c = 0;
			if (c > scr->d.cols)
				c = scr->d.cols - 1;
		}
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
	int d, dr, dc, idx;
	RelativeDirection dir;

	idx = v->index;

	assert(scr->ntags > 0);

	arg = k->arg;
	dir = k->dir;
	if (arg && (sscanf(arg, "%d", &d) == 1)) {
		if ((*arg == '+' || *arg == '-') && dir == RelativeNone)
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
	idx = _getviewrandc(v, k, dr, dc);
      done:
	if (k->wrap) {
		while (idx < 0)
			idx += scr->ntags;
		idx = idx % scr->ntags;
	} else {
		if (idx < 0)
			idx = 0;
		if (idx >= scr->ntags)
			idx = scr->ntags - 1;
	}
	return idx;
}

void
k_toggletag(XEvent *e, Key *k)
{
	if (sel)
		toggletag(sel, idxoftag(sel->cview, k));
	else
		DPRINTF("WARNING: no selected client\n");
}

void
k_tag(XEvent *e, Key *k)
{
	if (sel)
		tag(sel, idxoftag(sel->cview, k));
	else
		DPRINTF("WARNING: no selected client\n");
}

void
k_focusview(XEvent *e, Key *k)
{
	View *v;

	if ((v = selview()))
		focusview(v, idxoftag(v, k));
	else
		DPRINTF("WARNING: no selected view\n");
}

void
k_toggleview(XEvent *e, Key *k)
{
	View *v;

	if ((v = selview()))
		toggleview(v, idxoftag(v, k));
	else
		DPRINTF("WARNING: no selected view\n");
}

void
k_view(XEvent *e, Key *k)
{
	View *v;

	if ((v = selview()))
		view(v, idxoftag(v, k));
	else
		DPRINTF("WARNING: no selected view\n");
}

void
k_taketo(XEvent *e, Key *k)
{
	if (sel)
		taketo(sel, idxoftag(sel->cview, k));
}

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

void
k_chain(XEvent *e, Key *key)
{
	Key *k = NULL;

	if (XGrabKeyboard
	    (dpy, scr->root, GrabModeSync, False, GrabModeAsync, CurrentTime)) {
		DPRINTF("Could not grab keyboard\n");
		return;
	}

	for (;;) {
		XEvent ev;
		KeySym keysym;
		unsigned long mod;

		XMaskEvent(dpy, KeyPressMask | KeyReleaseMask, &ev);
		pushtime(ev.xkey.time);
		keysym = XkbKeycodeToKeysym(dpy, (KeyCode) ev.xkey.keycode, 0, 0);
		mod = CLEANMASK(ev.xkey.state);

		switch (ev.type) {
			Key *kp;

		case KeyRelease:
			/* a key release other than the active key is a release of a
			   modifier indicating a stop */
			if (k && k->keysym != keysym) {
				if (k->stop)
					k->stop(&ev, k);
				return;
			}
			break;
		case KeyPress:
			/* a press of a different key, even a modifier, or a press of the 
			   same key with a different modifier mask indicates a stop of
			   the current sequence and the potential start of a new one */
			if (k && (k->keysym != keysym || k->mod != mod)) {
				if (k->stop)
					k->stop(&ev, k);
				return;
			}
			for (kp = key->cnext; !k && kp; kp = kp->cnext)
				if (kp->keysym == keysym && kp->mod == mod)
					k = kp;
			if (k) {
				if (k->func)
					k->func(&ev, k);
				if (k->chain || !k->stop)
					return;
				break;
			}
			/* Use the Escape key without modifiers to escape from a key
			   chain. */
			if (keysym == XK_Escape && !mod)
				return;
			/* unrecognized key presses must be ignored because they may just 
			   be a modifier being pressed */
			break;
		}
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
k_setlayout(XEvent *e, Key *k)
{
	setlayout(k->arg);
}

void
k_spawn(XEvent *e, Key *k)
{
	spawn(k->arg);
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
