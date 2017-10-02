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
#include "actions.h" /* verification */

void
m_move(Client *c, XEvent *e)
{
	DPRINT;
	focus(c);
	if (!mousemove(c, e, (e->xbutton.state & ControlMask) ? False : True))
		raiselower(c);
}

void
m_nexttag(Client *c, XEvent *e)
{
	viewrighttag();
}

void
m_prevtag(Client *c, XEvent *e)
{
	viewlefttag();
}

void
m_resize(Client *c, XEvent *e)
{
	focus(c);
	if (!mouseresize(c, e, (e->xbutton.state & ControlMask) ? False : True))
		raiselower(c);
}

void
m_shade(Client *c, XEvent *e)
{
	if (!c)
		return;
	switch (e->xbutton.button) {
	case Button4:
		/* up */
		if (!c->is.shaded)
			toggleshade(c);
		break;
	case Button5:
		/* down */
		if (c->is.shaded)
			toggleshade(c);
		break;
	}
}

void
m_spawn(Client *c, XEvent *e)
{
	spawn(scr->options.command);
}

void
m_spawn2(Client *c, XEvent *e)
{
	spawn(scr->options.command2);
}

void
m_spawn3(Client *c, XEvent *e)
{
	spawn(scr->options.command3);
}

void
m_zoom(Client *c, XEvent *e)
{
	focus(c);
	if (!mousemove(c, e, (e->xbutton.state & ControlMask) ? True : False))
		zoomfloat(c);
}

void
k_chain(XEvent *e, Key *key)
{
	Key *k = NULL;

	if (XGrabKeyboard
	    (dpy, scr->root, GrabModeSync, False, GrabModeAsync, e->xkey.time)) {
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

void
k_focusmain(XEvent *e, Key *k)
{
	if (sel)
		focusmain(sel);
}

void
k_focusurgent(XEvent *e, Key *k)
{
	if (sel)
		focusurgent(sel);
}

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
k_flipview(XEvent *e, Key *k)
{
	flipview(sel);
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
k_flipzone(XEvent *e, Key *k)
{
	flipzone(sel);
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
k_flipwins(XEvent *e, Key *k)
{
	flipwins(sel);
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

void
k_reload(XEvent *e, Key *k)
{
	reload();
}

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
k_setncolumns(XEvent *e, Key *k)
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
				decncolumns(v, num);
			else
				incncolumns(v, num);
		} else
			setncolumns(v, num);
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
c_setlhalf(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->is.lhalf)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->is.lhalf)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglelhalf(c);
}

void
k_setlhalf(XEvent *e, Key *k)
{
	return k_setgeneric(e, k, &c_setlhalf);
}

static void
c_setrhalf(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->is.rhalf)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->is.rhalf)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglerhalf(c);
}

void
k_setrhalf(XEvent *e, Key *k)
{
	return k_setgeneric(e, k, &c_setrhalf);
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
		AScreen *s;
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

static void
c_setsticky(XEvent *e, Key *k, Client *c)
{
	switch (k->set) {
	case SetFlagSetting:
		if (c->is.sticky)
			return;
		break;
	case UnsetFlagSetting:
		if (!c->is.sticky)
			return;
		break;
	default:
	case ToggleFlagSetting:
		break;
	}
	togglesticky(c);
}

void
k_setsticky(XEvent *e, Key *k)
{
	return k_setgeneric(e, k, &c_setsticky);
}


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
k_select_cl(View *cv, Key *k, CycleList * cl)
{
	Client *c;
	View *v;
	static IsUnion is = {.is = 0 };

	if (cl != k->where) {
		c = k->where->c;
		c->is.icon = is.icon;
		c->is.hidden = is.hidden;
		if (is.icon || is.hidden)
			arrange(c->cview);
		is.is = 0;
	}
	c = cl->c;
	if (!(v = c->cview) && !(v = clientview(c))) {
		if (!(v = onview(c))) {
			k_stop(NULL, k);
			return;
		}
		/* have to switch views on monitor v->curmon */
		view(cv, v->index);
	}
	if (cl != k->where) {
		is.is = 0;
		if (c->is.icon || c->is.hidden) {
			if ((is.icon = c->is.icon))
				c->is.icon = False;
			if ((is.hidden = c->is.hidden))
				c->is.hidden = False;
		}
	}
	/* raise floaters as we cycle */
	raisefloater(c);
	focus(c);
	if (k->cyc) {
		k->stop = k_stop;
		if (!XGrabKeyboard(dpy, scr->root, GrabModeSync, False,
				   GrabModeAsync, user_time))
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

void
k_sendto(XEvent *e, Key *k)
{
	if (sel)
		tagonly(sel, idxoftag(sel->cview, k));
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

