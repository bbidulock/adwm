/* See COPYING file for copyright and license details.  */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <regex.h>
#include <signal.h>
#include <math.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/XF86keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xft/Xft.h>
#ifdef XRANDR
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/randr.h>
#endif
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#ifdef SYNC
#include <X11/extensions/sync.h>
#endif
#ifdef STARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>
#endif
#include "adwm.h"
#include "layout.h"
#include "draw.h"
#include "ewmh.h"
#include "config.h"

#define MWFACT		(1<<0)	/* adjust master factor */
#define NMASTER		(1<<1)	/* adjust number of masters */
#define	ZOOM		(1<<2)	/* adjust zoom */
#define	OVERLAP		(1<<3)	/* floating layout */
#define NCOLUMNS	(1<<4)	/* adjust number of columns */
#define ROTL		(1<<5)	/* adjust rotation */
#define MMOVE		(1<<6)	/* shuffle tile position with mouse */

#define FEATURES(_layout, _which) (!(!((_layout)->features & (_which))))
#define M2LT(_mon) ((_mon)->curview->layout)
#define MFEATURES(_monitor, _which) ((_monitor) && FEATURES(M2LT(_monitor), (_which)))

/*
 * This file contains layout-specific functions that must ultimately move into layout modules.
 */

static Bool
validlist()
{
	Client *p, *c;

	if (!scr->clients)
		return True;
	for (p = NULL, c = scr->clients; c; p = c, c = c->next) {
		if (c->prev != p)
			return False;
		if (p && p->next != c)
			return False;
	}
	return True;
}

static void
attach(Client *c, Bool attachaside)
{
	assert(!c->prev && !c->next);
	if (attachaside) {
		if (scr->clients) {
			Client *lastClient = scr->clients;

			while (lastClient->next)
				lastClient = lastClient->next;
			c->prev = lastClient;
			lastClient->next = c;
		} else
			scr->clients = c;
	} else {
		if (scr->clients)
			scr->clients->prev = c;
		c->next = scr->clients;
		scr->clients = c;
	}
#ifdef DEBUG
	assert(validlist());
#endif
}

static void
attachclist(Client *c)
{
	Client **cp;

	assert(c->cnext == NULL);
	for (cp = &scr->clist; *cp; cp = &(*cp)->cnext) ;
	*cp = c;
	c->cnext = NULL;
}

static void
attachflist(Client *c, Bool front)
{
	Client *s;

	assert(c->fnext == NULL);
	if (front || !sel || sel == c) {
		c->fnext = scr->flist;
		scr->flist = c;
	} else {
		for (s = scr->flist; s && s != sel; s = s->fnext) ;
		assert(s == sel);
		c->fnext = s->fnext;
		s->fnext = c;
	}
}

static void
attachstack(Client *c, Bool front)
{
	if (front) {
		assert(c && c->snext == NULL);
		c->snext = scr->stack;
		scr->stack = c;
	} else {
		Client **cp;

		assert(c && c->snext == NULL);
		for (cp = &scr->stack; *cp; cp = &(*cp)->snext) ;
		c->snext = NULL;
		*cp = c;
	}
}

static void
detach(Client *c)
{
	if (c->prev)
		c->prev->next = c->next;
	if (c->next)
		c->next->prev = c->prev;
	if (c == scr->clients)
		scr->clients = c->next;
	c->next = c->prev = NULL;
#ifdef DEBUG
	assert(validlist());
#endif
}

static void
detachclist(Client *c)
{
	Client **cp;

	for (cp = &scr->clist; *cp && *cp != c; cp = &(*cp)->cnext) ;
	assert(*cp == c);
	*cp = c->cnext;
	c->cnext = NULL;
}

static void
detachflist(Client *c)
{
	Client **cp;

	for (cp = &scr->flist; *cp && *cp != c; cp = &(*cp)->fnext) ;
	assert(*cp == c);
	*cp = c->fnext;
	c->fnext = NULL;
}

static void
detachstack(Client *c)
{
	Client **cp;

	for (cp = &scr->stack; *cp && *cp != c; cp = &(*cp)->snext) ;
	assert(*cp == c);
	*cp = c->snext;
	c->snext = NULL;
}

void
tookfocus(Client *next)
{
	Client *last = took;

	took = next;
	if (last && last != next) {
		if (last->is.focused) {
			last->is.focused = False;
			ewmh_update_net_window_state(last);
		}
	}
	if (next && next != last) {
		if (!next->is.focused) {
			next->is.focused = True;
			ewmh_update_net_window_state(next);
		}
		detachflist(next);
		attachflist(next, True);
	}
	gave = next;
}

static void
getgeometry(Client *c, Geometry *g, ClientGeometry * gc)
{
	*(Geometry *) gc = *g;
	gc->t = c->th;
	gc->g = c->gh;
}

/* can be static soon */
Bool
isfloating(Client *c, Monitor *m)
{
	if ((c->is.floater && !c->is.dockapp) || c->skip.arrange)
		return True;
	if (c->is.full)
		return True;
	if (m && MFEATURES(m, OVERLAP))
		return True;
	return False;
}

Bool
enterclient(XEvent *e, Client *c)
{
	if (c->is.bastard && !c->is.dockapp) {
		CPRINTF(c, "FOCUS: cannot focus bastards.\n");
		return True;
	}
	/* focus when switching monitors */
	if (!isvisible(sel, c->cmon)) {
		CPRINTF(c, "FOCUS: monitor switching focus\n");
		focus(c);
	}
	switch (scr->options.focus) {
	case Clk2Focus:
		break;
	case SloppyFloat:
		/* FIXME: incorporate isfloating() check into skip.sloppy setting */
		if (!c->skip.sloppy && isfloating(c, c->cmon)) {
			CPRINTF(c, "FOCUS: sloppy focus\n");
			focus(c);
		}
		break;
	case AllSloppy:
		if (!c->skip.sloppy) {
			CPRINTF(c, "FOCUS: sloppy focus\n");
			focus(c);
		}
		break;
	case SloppyRaise:
		if (!c->skip.sloppy) {
			CPRINTF(c, "FOCUS: sloppy focus\n");
			focus(c);
			raiseclient(c);
		}
		break;
	}
	return True;
}

static void
reconfigure_dockapp(Client *c, ClientGeometry * n)
{
	XWindowChanges wwc, fwc;
	unsigned wmask, fmask;

	wmask = fmask = 0;
	if (c->c.x != (fwc.x = n->x)) {
		c->c.x = n->x;
		fmask |= CWX;
	}
	if (c->c.y != (fwc.y = n->y)) {
		c->c.y = n->y;
		fmask |= CWY;
	}
	if (c->c.w != (fwc.width = n->w)) {
		c->c.w = n->w;
		fmask |= CWWidth;
	}
	if (c->c.h != (fwc.height = n->h)) {
		c->c.h = n->h;
		fmask |= CWHeight;
	}
	if (c->c.b != (fwc.border_width = n->b)) {
		c->c.b = n->b;
		fmask |= CWBorderWidth;
	}
	if (c->r.x != (wwc.x = (n->w - c->r.w) / 2)) {
		c->r.x = (n->w - c->r.w) / 2;
		wmask |= CWX;
	}
	if (c->r.y != (wwc.y = (n->h - c->r.h) / 2)) {
		c->r.y = (n->h - c->r.h) / 2;
		wmask |= CWY;
	}
	XMapWindow(dpy, c->frame);	/* not mapped for some reason... */
	wwc.width = c->r.w;
	wwc.height = c->r.h;
	wwc.border_width = c->r.b;
	if (fmask) {
		DPRINTF("frame wc = %ux%u+%d+%d:%d\n", fwc.width, fwc.height, fwc.x,
			fwc.y, fwc.border_width);
		XConfigureWindow(dpy, c->frame,
				 CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &fwc);
	}
	if (wmask) {
		DPRINTF("wind  wc = %ux%u+%d+%d:%d\n", wwc.width, wwc.height, wwc.x,
			wwc.y, wwc.border_width);
		XConfigureWindow(dpy, c->icon,
				 CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &wwc);
	}
	if ((fmask | wmask) && !(wmask & (CWWidth | CWHeight))) {
		XConfigureEvent ce;

		ce.type = ConfigureNotify;
		ce.display = dpy;
		ce.event = c->icon;
		ce.window = c->icon;
		ce.x = c->c.x + c->r.x;
		ce.y = c->c.y + c->r.y;
		ce.width = c->r.w;
		ce.height = c->r.h;
		ce.border_width = c->c.b;
		ce.above = None;
		ce.override_redirect = False;
		XSendEvent(dpy, c->icon, False, StructureNotifyMask, (XEvent *) &ce);
	}
	XSync(dpy, False);
	drawclient(c);
	if (fmask & (CWWidth | CWHeight))
		ewmh_update_net_window_extents(c);
	XSync(dpy, False);
}

/* FIXME: this does not handle moving the window across monitor
 * or desktop boundaries. */

static void
reconfigure(Client *c, ClientGeometry * n)
{
	XWindowChanges wwc, fwc;
	unsigned wmask, fmask;
	Bool tchange = False, gchange = False, shaded = False;

	if (n->w <= 0 || n->h <= 0) {
		CPRINTF(c, "zero width %d or height %d\n", n->w, n->h);
		return;
	}
	/* offscreen appearance fixes */
	if (n->x > DisplayWidth(dpy, scr->screen))
		n->x = DisplayWidth(dpy, scr->screen) - n->w - 2 * n->b;
	if (n->y > DisplayHeight(dpy, scr->screen))
		n->y = DisplayHeight(dpy, scr->screen) - n->h - 2 * n->b;
	DPRINTF("x = %d y = %d w = %d h = %d b = %d t = %d g = %d\n", n->x, n->y, n->w,
		n->h, n->b, n->t, n->g);

	if (c->is.dockapp)
		return reconfigure_dockapp(c, n);

	wmask = fmask = 0;
	if (c->c.x != (fwc.x = n->x)) {
		c->c.x = n->x;
		DPRINTF("frame wc.x = %d\n", fwc.x);
		fmask |= CWX;
	}
	if (c->c.y != (fwc.y = n->y)) {
		c->c.y = n->y;
		DPRINTF("frame wc.y = %d\n", fwc.y);
		fmask |= CWY;
	}
	if (c->c.w != (fwc.width = wwc.width = n->w)) {
		c->c.w = n->w;
		DPRINTF("frame wc.w = %u\n", fwc.width);
		fmask |= CWWidth;
		DPRINTF("wind  wc.w = %u\n", wwc.width);
		wmask |= CWWidth;
	}
	if (c->c.h - c->th - c->gh != (wwc.height = n->h - n->t - n->g)) {
		DPRINTF("wind  wc.h = %u\n", wwc.height);
		wmask |= CWHeight;
	}
	if (c->c.h != (fwc.height = n->h)) {
		c->c.h = n->h;
		DPRINTF("frame wc.h = %u\n", fwc.height);
		fmask |= CWHeight;
	}
	if (n->t && !c->title)
		n->t = 0;
	if (c->th != (wwc.y = n->t)) {
		c->th = n->t;
		DPRINTF("wind  wc.y = %d\n", wwc.y);
		wmask |= CWY;
		tchange = True;
	}
	if (n->g && !c->grips)
		n->g = 0;
	if (c->gh != n->g) {
		c->gh = n->g;
		gchange = True;
	}
	if (n->t && (c->is.shaded && (c != sel || !scr->options.autoroll))) {
		fwc.height = n->t;
		DPRINTF("frame wc.h = %u\n", fwc.height);
		fmask |= CWHeight;
		shaded = True;
	} else {
		fwc.height = n->h;
		DPRINTF("frame wc.h = %u\n", fwc.height);
		fmask |= CWHeight;
		shaded = False;
	}
	if (c->c.b != (fwc.border_width = n->b)) {
		c->c.b = n->b;
		DPRINTF("frame wc.b = %u\n", fwc.border_width);
		fmask |= CWBorderWidth;
	}
	if (fmask) {
		DPRINTF("frame wc = %ux%u+%d+%d:%d\n", fwc.width, fwc.height, fwc.x,
			fwc.y, fwc.border_width);
		XConfigureWindow(dpy, c->frame,
				 CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &fwc);
	}
	wwc.x = 0;
	wwc.border_width = 0;
	if ((wmask & (CWWidth | CWHeight))
	    && !newsize(c, wwc.width, wwc.height, CurrentTime))
		wmask &= ~(CWWidth | CWHeight);
	if (shaded)
		XUnmapWindow(dpy, c->win);
	else
		XMapWindow(dpy, c->win);
	if (wmask) {
		DPRINTF("wind  wc = %ux%u+%d+%d:%d\n", wwc.width, wwc.height, wwc.x,
			wwc.y, wwc.border_width);
		XConfigureWindow(dpy, c->win, wmask | CWX | CWY | CWBorderWidth, &wwc);
	}
	/* ICCCM 2.0 4.1.5 */
	if ((fmask | wmask) && !(wmask & (CWWidth | CWHeight)))
		send_configurenotify(c, None);
	XSync(dpy, False);
	if (c->title && (tchange || ((wmask | fmask) & (CWWidth)))) {
		if (tchange) {
			if (n->t)
				XMapWindow(dpy, c->title);
			else
				XUnmapWindow(dpy, c->title);
		}
		if (n->t && (tchange || ((wmask | fmask) & (CWWidth))))
			XMoveResizeWindow(dpy, c->title, 0, 0, wwc.width, n->t);
	}
	if (c->grips) {
		if (shaded || !n->g)
			XUnmapWindow(dpy, c->grips);
		else
			XMapWindow(dpy, c->grips);
		if (n->g && (gchange || ((wmask | fmask) & (CWWidth | CWHeight | CWY))))
			XMoveResizeWindow(dpy, c->grips, 0, n->h - n->g, wwc.width, n->g);
	}
	if (((c->title && n->t) || (c->grips && n->g)) &&
	    ((tchange && n->t) || (gchange && n->g) || (wmask & CWWidth)))
		drawclient(c);
	if (tchange || gchange || (fmask & CWBorderWidth))
		ewmh_update_net_window_extents(c);
	XSync(dpy, False);
}

static Bool
constrain(Client *c, ClientGeometry * g)
{
	int w = g->w, h = g->h;
	Bool ret = False;

	CPRINTF(c, "geometry before constraint: %dx%d+%d+%d:%d[%d,%d]\n",
		g->w, g->h, g->x, g->y, g->b, g->t, g->g);

	/* remove decoration */
	h -= g->t + g->g;

	/* set minimum possible */
	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;

	/* temporarily remove base dimensions */
	w -= c->basew;
	h -= c->baseh;

	/* adjust for aspect limits */
	if (c->minay > 0 && c->maxay > 0 && c->minax > 0 && c->maxax > 0) {
		if (w * c->maxay > h * c->maxax)
			w = h * c->maxax / c->maxay;
		else if (w * c->minay < h * c->minax)
			h = w * c->minay / c->minax;
	}

	/* adjust for increment value */
	if (c->incw)
		w -= w % c->incw;
	if (c->inch)
		h -= h % c->inch;

	/* restore base dimensions */
	w += c->basew;
	h += c->baseh;

	if (c->minw > 0 && w < c->minw)
		w = c->minw;
	if (c->minh > 0 && h < c->minh)
		h = c->minh;
	if (c->maxw > 0 && w > c->maxw)
		w = c->maxw;
	if (c->maxh > 0 && h > c->maxh)
		h = c->maxh;

	/* restore decoration */
	h += g->t + g->g;

	if (w <= 0 || h <= 0)
		return ret;
	if (w != g->w) {
		g->w = w;
		ret = True;
	}
	if (h != g->h) {
		g->h = h;
		ret = True;
	}
	CPRINTF(c, "geometry after constraints: %dx%d+%d+%d:%d[%d,%d]\n",
		g->w, g->h, g->x, g->y, g->b, g->t, g->g);
	return ret;
}

static void
save(Client *c)
{
	CPRINTF(c, "%dx%d+%d+%d:%d <= %dx%d+%d+%d:%d\n",
		c->r.w, c->r.h, c->r.x, c->r.y, c->r.b,
		c->c.w, c->c.h, c->c.x, c->c.y, c->c.b);
	c->r = c->c;
}

/* unused */
void
restore(Client *c)
{
	ClientGeometry g;

	getgeometry(c, &c->r, &g);
	DPRINTF("CALLING: constrain()\n");
	constrain(c, &g);
	DPRINTF("CALLING reconfigure()\n");
	reconfigure(c, &g);
}

Bool
configureclient(XEvent *e, Client *c, int gravity)
{
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	Monitor *cm;

	/* XXX: if c->curmon is set, we are displayed on a monitor, but c->curview should 
	   also be set.  We only need the monitor for the layout.  When there is no
	   curmon, use one of the views to which the client is tagged? How meaningful is
	   it moving a window not in the current view? Perhaps we should treat it a just
	   moving the saved floating state.  This is the only function that calls
	   findcurmonitor(). */

	if (!(cm = c->cmon ? : selmonitor()))
		return False;
	if (!c->is.max && isfloating(c, cm)) {
		ClientGeometry g;
		int dx, dy, dw, dh;

		getgeometry(c, &c->c, &g);

		dx = ((ev->value_mask & CWX) && c->can.move) ? ev->x - g.x : 0;
		dy = ((ev->value_mask & CWY) && c->can.move) ? ev->y - g.y : 0;
		dw = ((ev->value_mask & CWWidth) && c->can.sizeh) ? ev->width - g.w : 0;
		dh = ((ev->value_mask & CWHeight)
		      && c->can.sizev) ? ev->height - (g.h - g.t - g.g) : 0;
		g.b = (ev->value_mask & CWBorderWidth) ? ev->border_width : g.b;

		moveresizekb(c, dx, dy, dw, dh, gravity);
		/* TODO: check _XA_WIN_CLIENT_MOVING and handle moves between monitors */
	} else {
		ClientGeometry g;

		getgeometry(c, &c->c, &g);

		g.b = (ev->value_mask & CWBorderWidth) ? ev->border_width : g.b;

		DPRINTF("CALLING reconfigure()\n");
		reconfigure(c, &g);
		if (ev->value_mask & (CWBorderWidth))
			c->s.b = g.b;
	}
	if (ev->value_mask & CWStackMode) {
		Client *s = NULL;

		if (ev->value_mask & CWSibling)
			if (!(s = getclient(ev->above, ClientAny)))
				return False;
		/* might want to make this optional */
		restack_client(c, ev->detail, s);
	}
	return True;
}

static Monitor *
findmonbynum(int num)
{
	Monitor *m;

	for (m = scr->monitors; m && m->num != num; m = m->next) ;
	return m;
}

Bool
configuremonitors(XEvent *e, Client *c)
{
	XClientMessageEvent *ev = &e->xclient;

	if (c->is.max) {
		Monitor *mt, *mb, *ml, *mr;
		int t, b, l, r;
		ClientGeometry g;

		mt = findmonbynum(ev->data.l[0]);
		mb = findmonbynum(ev->data.l[1]);
		ml = findmonbynum(ev->data.l[2]);
		mr = findmonbynum(ev->data.l[3]);
		if (!mt || !mb || !ml || !mr) {
			ewmh_update_net_window_fs_monitors(c);
			return False;
		}
		t = mt->sc.y;
		b = mb->sc.y + mb->sc.h;
		l = ml->sc.x;
		r = mr->sc.x + mr->sc.w;
		if (t >= b || r >= l) {
			ewmh_update_net_window_fs_monitors(c);
			return False;
		}
		g.x = l;
		g.y = t;
		g.w = r - l;
		g.h = b - t;
		g.b = 0;
		g.t = 0;
		g.g = 0;
		reconfigure(c, &g);
	}
	return True;
}

static void
calc_full(Client *c, Monitor *m, ClientGeometry * g)
{
	Monitor *fsmons[4] = { m, m, m, m };
	long *mons;
	unsigned long n = 0;
	int i;

	mons = getcard(c->win, _XA_NET_WM_FULLSCREEN_MONITORS, &n);
	if (n >= 4) {
		for (i = 0; i < 4; i++)
			if (!(fsmons[i] = findmonbynum(mons[i])))
				break;
		if (i < 4 || (fsmons[0]->sc.y >= fsmons[1]->sc.y + fsmons[1]->sc.h) ||
		    (fsmons[1]->sc.x >= fsmons[3]->sc.x + fsmons[3]->sc.w))
			fsmons[0] = fsmons[1] = fsmons[2] = fsmons[3] = m;
	}
	free(mons);
	g->x = fsmons[2]->sc.x;
	g->y = fsmons[0]->sc.y;
	g->w = fsmons[3]->sc.x + fsmons[3]->sc.w - g->x;
	g->h = fsmons[1]->sc.y + fsmons[1]->sc.h - g->y;
	g->b = 0;
	g->t = 0;
	g->g = 0;
}

static void
calc_fill(Client *c, Monitor *m, Workarea *wa, ClientGeometry * g)
{
	int x1, x2, y1, y2, w, h;
	Client *o;

	x1 = wa->x;
	x2 = wa->x + wa->w;
	y1 = wa->y;
	y2 = wa->y + wa->h;

	for (o = scr->clients; o; o = o->next) {
		if (!(isvisible(o, m)))
			continue;
		if (o == c)
			continue;
		if (o->is.bastard || o->is.dockapp)
			continue;
		if (o->c.y + o->c.h > g->y && o->c.y < g->y + g->h) {
			if (o->c.x < g->x)
				x1 = max(x1, o->c.x + o->c.w + scr->style.border);
			else
				x2 = min(x2, o->c.x - scr->style.border);
		}
		if (o->c.x + o->c.w > g->x && o->c.x < g->x + g->w) {
			if (o->c.y < g->y)
				y1 = max(y1, o->c.y + o->c.h + scr->style.border);
			else
				y2 = max(y2, o->c.y - scr->style.border);
		}
		XPRINTF("x1 = %d x2 = %d y1 = %d y2 = %d\n", x1, x2, y1, y2);
	}
	w = x2 - x1;
	h = y2 - y1;
	XPRINTF("x1 = %d w = %d y1 = %d h = %d\n", x1, w, y1, h);
	if (w > g->w) {
		g->x = x1;
		g->w = w;
		g->b = scr->style.border;
	}
	if (h > g->h) {
		g->y = y1;
		g->h = h;
		g->b = scr->style.border;
	}
}

static void
calc_max(Client *c, Workarea *wa, ClientGeometry * g)
{
	g->x = wa->x;
	g->y = wa->y;
	g->w = wa->w;
	g->h = wa->h;
	if (!scr->options.decmax)
		g->t = 0;
	g->b = 0;
}

static void
calc_maxv(Client *c, Workarea *wa, ClientGeometry * g)
{
	g->y = wa->y;
	g->h = wa->h;
	g->b = scr->style.border;
}

static void
calc_maxh(Client *c, Workarea *wa, ClientGeometry * g)
{
	g->x = wa->x;
	g->w = wa->w;
	g->b = scr->style.border;
}

static void
get_decor(Client *c, Monitor *m, ClientGeometry * g)
{
	int i;
	Bool decorate;

	if (c->is.dockapp) {
		g->t = 0;
		g->g = 0;
		return;
	}
	g->t = c->th;
	g->g = c->gh;
	if (c->is.max || !c->has.title)
		g->t = 0;
	if (c->is.max || !c->has.grips)
		g->g = 0;
	if (c->is.max || (!c->has.title && !c->has.grips))
		return;
	if (c->is.floater || c->skip.arrange)
		decorate = True;
	else if (c->is.shaded && (c != sel || !scr->options.autoroll))
		decorate = True;
	else if (!m && !(m = clientmonitor(c))) {
		decorate = False;
		for (i = 0; i < scr->ntags; i++) {
			if ((c->tags & (1ULL << i)) && (scr->views[i].dectiled ||
							FEATURES(scr->views[i].layout,
								 OVERLAP))) {
				decorate = True;
				break;
			}
		}
	} else {
		decorate = (m->curview->dectiled || MFEATURES(m, OVERLAP)) ?
		    True : False;
	}
	g->t = decorate ? ((c->title && c->has.title) ? scr->style.titleheight : 0) : 0;
	g->g = decorate ? ((c->grips && c->has.grips) ? scr->style.gripsheight : 0) : 0;
}

static void
discardenter()
{
	XEvent ev;

	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev)) ;
	if (sel)
		focus(sel);	/* put focus back where it was */
}

static void
updatefloat(Client *c, Monitor *m)
{
	ClientGeometry g;
	Workarea wa;

	if (c->is.dockapp)
		return;
	if (!m && !(m = c->cmon))
		return;
	if (!isfloating(c, m))
		return;
	getworkarea(m, &wa);
	CPRINTF(c, "r: %dx%d+%d+%d:%d\n", c->r.w, c->r.h, c->r.x, c->r.y, c->r.b);
	getgeometry(c, &c->r, &g);
	CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
	g.b = scr->style.border;
	CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
	get_decor(c, m, &g);
	CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
	if (c->is.full) {
		calc_full(c, m, &g);
		CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
	} else {
		if (c->is.max) {
			calc_max(c, &wa, &g);
			CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
		} else if (c->is.fill) {
			calc_fill(c, m, &wa, &g);
			CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
		} else {
			if (c->is.maxv) {
				calc_maxv(c, &wa, &g);
				CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y,
					g.b);
			}
			if (c->is.maxh) {
				calc_maxh(c, &wa, &g);
				CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y,
					g.b);
			}
		}
	}
	if (!c->is.max && !c->is.full) {
		/* TODO: more than just northwest gravity */
		DPRINTF("CALLING: constrain()\n");
		constrain(c, &g);
	}
	CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
	DPRINTF("CALLING reconfigure()\n");
	reconfigure(c, &g);
	if (c->is.max)
		ewmh_update_net_window_fs_monitors(c);
	discardenter();
}

static void
arrangefloats(Monitor *m)
{
	Client *c;

	for (c = scr->stack; c; c = c->snext)
		if (isvisible(c, m) && !c->is.bastard && !c->is.dockapp)
			/* XXX: can.move? can.tag? */
			updatefloat(c, m);
}

Client *
nextdockapp(Client *c, Monitor *m)
{
	for (; c && (!c->is.dockapp || !isvisible(c, m) || c->is.hidden); c = c->next) ;
	return c;
}

Client *
prevdockapp(Client *c, Monitor *m)
{
	for (; c && (!c->is.dockapp || !isvisible(c, m) || c->is.hidden); c = c->prev) ;
	return c;
}

static void
arrangedock(Monitor *m)
{
	Client *c = nextdockapp(scr->clients, m), *cn;
	View *v = m->curview;
	int i, num, rows, cols, margin = 4, max = 64, b = scr->style.border;
	ClientGeometry *g = NULL, n;
	DockPosition pos = m->dock.position;
	DockSide side;
	Bool overlap = FEATURES(v->layout, OVERLAP) ? True : False;

	updategeom(m);
	m->dock.wa = m->wa;

	if (!c || pos == DockNone)
		return;

	for (num = 0, c = nextdockapp(scr->clients, m); c;
	     c = nextdockapp(c->next, m), num++) ;

	DPRINTF("there are %d dock apps\n", num);

	switch (m->dock.orient) {
	case DockHorz:
		DPRINTF("Dock orientation Horz\n");
		break;
	case DockVert:
		DPRINTF("Dock orientation Vert\n");
		break;
	}

	switch (pos) {
	case DockNorth:
		side = DockSideNorth;
		DPRINTF("Dock position North\n");
		break;
	case DockEast:
		side = DockSideEast;
		DPRINTF("Dock position East\n");
		break;
	case DockSouth:
		side = DockSideSouth;
		DPRINTF("Dock position South\n");
		break;
	case DockWest:
		side = DockSideWest;
		DPRINTF("Dock position South\n");
		break;
	case DockNorthEast:
		side = (m->dock.orient == DockHorz) ? DockSideNorth : DockSideEast;
		DPRINTF("Dock position NorthEast\n");
		break;
	case DockNorthWest:
		side = (m->dock.orient == DockHorz) ? DockSideNorth : DockSideWest;
		DPRINTF("Dock position NorthWest\n");
		break;
	case DockSouthWest:
		side = (m->dock.orient == DockHorz) ? DockSideSouth : DockSideWest;
		DPRINTF("Dock position SouthWest\n");
		break;
	case DockSouthEast:
		side = (m->dock.orient == DockHorz) ? DockSideSouth : DockSideEast;
		DPRINTF("Dock position SouthEast\n");
		break;
	default:
		return;
	}

	switch (side) {
	case DockSideEast:
		DPRINTF("Dock side East\n");
		break;
	case DockSideWest:
		DPRINTF("Dock side West\n");
		break;
	case DockSideNorth:
		DPRINTF("Dock side North\n");
		break;
	case DockSideSouth:
		DPRINTF("Dock side South\n");
		break;
	}

	switch (side) {
	case DockSideEast:
	case DockSideWest:
		rows = (m->wa.h - b) / (max + b);
		cols = (num - 1) / rows + 1;
		DPRINTF("dock columns %d, rows %d\n", cols, rows);
		if (rows > num)
			rows = num;
		g = calloc(cols, sizeof(*g));
		for (i = 0; i < cols; i++) {
			g[i].b = b;
			g[i].w = max;
			g[i].h = (m->wa.h - b) / rows - b;
			g[i].y = m->wa.y;
			switch ((int) side) {
			case DockSideEast:
				g[i].x = m->wa.x + m->wa.w - (i + 1) * (g[i].w + b);
				break;
			case DockSideWest:
				g[i].x = m->wa.x + i * (g[i].w + b);
				break;
			}
			DPRINTF("geometry col %d is %dx%d+%d+%d:%d\n", i, g[i].w, g[i].h,
				g[i].x, g[i].y, g[i].b);
			num -= rows;
			if (rows > num)
				rows = num;
		}
		if (overlap) {
			int h;

			for (i = 0, cn = nextdockapp(scr->clients, m); (c = cn);) {
				cn = nextdockapp(c->next, m);

				h = c->r.h + 2 * margin;
				if (h > max)
					h = max;
				h += b;
				if (!cn || g[i].y + h + b <= m->wa.y + m->wa.h) {
					g[i].y += h + b;
					if (cn)
						continue;
				}
				switch ((int) pos) {
				case DockEast:
				case DockWest:
					g[i].y = m->wa.y + (m->wa.h -
							    (g[i].y + b - m->wa.y)) / 2;
					break;
				case DockNorthEast:
				case DockNorthWest:
					g[i].y = m->wa.y;
					break;
				case DockSouthEast:
				case DockSouthWest:
					g[i].y = m->wa.y + (m->wa.h -
							    (g[i].y + b - m->wa.y));
					break;
				}
				DPRINTF("geometry col %d is %dx%d+%d+%d:%d\n", i, g[i].w,
					g[i].h, g[i].x, g[i].y, g[i].b);
				i++;
			}
			cols = i;
		}
		switch ((int) side) {
		case DockSideEast:
			m->dock.wa.w -= cols * (max + b) + b;
			XResizeWindow(dpy, m->veil, m->dock.wa.w, m->dock.wa.h);
			break;
		case DockSideWest:
			m->dock.wa.w -= cols * (max + b) + b;
			XResizeWindow(dpy, m->veil, m->dock.wa.w, m->dock.wa.h);
			m->dock.wa.x += cols * (max + b) + b;
			XMoveWindow(dpy, m->veil, m->dock.wa.x, m->dock.wa.y);
			break;
		}
		break;
	case DockSideNorth:
	case DockSideSouth:
		cols = (m->wa.w - b) / (max + b);
		rows = (num - 1) / cols + 1;
		if (cols > num)
			cols = num;
		DPRINTF("dock columns %d, rows %d\n", cols, rows);
		g = calloc(rows, sizeof(*g));
		for (i = 0; i < rows; i++) {
			g[i].b = b;
			g[i].w = (m->wa.w - b) / cols - b;
			g[i].h = max;
			g[i].x = m->wa.x;
			switch ((int) side) {
			case DockSideNorth:
				g[i].y = m->wa.y + i * (g[i].h + b);
				break;
			case DockSideSouth:
				g[i].y = m->wa.y + m->wa.h - (i + 1) * (g[i].h + b);
				break;
			}
			DPRINTF("geometry row %d is %dx%d+%d+%d:%d\n", i, g[i].w, g[i].h,
				g[i].x, g[i].y, g[i].b);
			num -= cols;
			if (cols > num)
				cols = num;
		}
		if (overlap) {
			int w;

			for (i = 0, cn = nextdockapp(scr->clients, m); (c = cn);) {
				cn = nextdockapp(c->next, m);

				w = c->r.w + 2 * margin;
				if (w > max)
					w = max;
				w += b;
				if (!cn || g[i].x + w + b <= m->wa.x + m->wa.w) {
					g[i].x += w + b;
					if (cn)
						continue;
				}
				switch ((int) pos) {
				case DockNorth:
				case DockSouth:
					g[i].x = m->wa.x + (m->wa.w -
							    (g[i].x + b - m->wa.x)) / 2;
					break;
				case DockNorthWest:
				case DockSouthWest:
					g[i].x = m->wa.x;
					break;
				case DockNorthEast:
				case DockSouthEast:
					g[i].x = m->wa.x + (m->wa.w -
							    (g[i].x + b - m->wa.x));
					break;
				}
				DPRINTF("geometry row %d is %dx%d+%d+%d:%d\n", i, g[i].w,
					g[i].h, g[i].x, g[i].y, g[i].b);
				i++;
			}
			cols = i;
		}
		switch ((int) side) {
		case DockSideNorth:
			m->dock.wa.y += rows * (max + b) + b;
			XMoveWindow(dpy, m->veil, m->dock.wa.x, m->dock.wa.y);
			m->dock.wa.h -= rows * (max + b) + b;
			XResizeWindow(dpy, m->veil, m->dock.wa.w, m->dock.wa.h);
			break;
		case DockSideSouth:
			m->dock.wa.h -= rows * (max + b) + b;
			XResizeWindow(dpy, m->veil, m->dock.wa.w, m->dock.wa.h);
			break;
		}
		break;
	}
	for (i = 0, cn = nextdockapp(scr->clients, m); (c = cn);) {
		cn = nextdockapp(c->next, m);
		switch (m->dock.orient) {
		case DockVert:
			if (overlap) {
				g[i].h = c->r.h + 2 * margin;
				if (g[i].h > max)
					g[i].h = max;
				g[i].h += b;
				if (g[i].y + g[i].h + b > m->wa.y + m->wa.h) {
					g[i + 1].h = g[i].h;
					i++;
				}
				n = g[i];
				g[i].y += g[i].h + b;
			} else {
				n = g[i];
				g[i].y += g[i].h + b;
				if (!cn || g[i].y > m->wa.y + m->wa.h) {
					n.h = m->wa.y + m->wa.h - n.y - 2 * b;
					i++;
				}
			}
			break;
		case DockHorz:
			if (overlap) {
				g[i].w = c->r.w + 2 * margin;
				if (g[i].w > max)
					g[i].w = max;
				g[i].w += b;
				if (g[i].x + g[i].w + b > m->wa.x + m->wa.w) {
					g[i + 1].w = g[i].w;
					i++;
				}
				n = g[i];
				g[i].x += g[i].w + b;
			} else {
				n = g[i];
				g[i].x += g[i].w + b;
				if (!cn || g[i].x > m->wa.x + m->wa.w) {
					n.w = m->wa.x + m->wa.h - n.x - 2 * b;
					i++;
				}
			}
			break;
		}
		if (!c->is.moveresize) {
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &n);
		} else {
			ClientGeometry C = n;

			/* center it where it was before */
			C.x = (c->c.x + c->c.w / 2) - C.w / 2;
			C.y = (c->c.y + c->c.h / 2) - C.h / 2;
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &C);
		}
	}
	free(g);
}

Client *
nexttiled(Client *c, Monitor *m)
{
	for (; c && (c->is.dockapp || c->is.floater || c->skip.arrange || !isvisible(c, m)
		     || c->is.bastard || (c->is.icon || c->is.hidden)); c = c->next) ;
	return c;
}

Client *
prevtiled(Client *c, Monitor *m)
{
	for (; c && (c->is.dockapp || c->is.floater || c->skip.arrange || !isvisible(c, m)
		     || c->is.bastard || (c->is.icon || c->is.hidden)); c = c->prev) ;
	return c;
}

static void
tile(Monitor *cm)
{
	LayoutArgs wa, ma, sa;
	ClientGeometry n, m, s, g;
	Client *c, *mc;
	View *v;
	int i, overlap, th, gh, mg;
	Bool mtdec, stdec, mgdec, sgdec;

	if (!(c = nexttiled(scr->clients, cm)))
		return;

	th = scr->style.titleheight;
	gh = scr->style.gripsheight;
	mg = scr->style.margin;

	getworkarea(cm, (Workarea *) &wa);
	v = cm->curview;
	for (wa.n = wa.s = ma.s = sa.s = 0, c = nexttiled(scr->clients, cm);
	     c; c = nexttiled(c->next, cm), wa.n++) {
		if (c->is.shaded && (c != sel || !scr->options.autoroll)) {
			wa.s++;
			if (wa.n < v->nmaster)
				ma.s++;
			else
				sa.s++;
		}
	}

	/* window geoms */

	/* master & slave number */
	ma.n = (wa.n > v->nmaster) ? v->nmaster : wa.n;
	sa.n = (ma.n < wa.n) ? wa.n - ma.n : 0;
	DPRINTF("there are %d masters\n", ma.n);
	DPRINTF("there are %d slaves\n", sa.n);

	/* at least one unshaded */
	ma.s = (ma.s && ma.s == ma.n) ? ma.n - 1 : ma.s;
	sa.s = (sa.s && sa.s == sa.n) ? sa.n - 1 : sa.s;

	/* too confusing when unshaded windows are not decorated */
	mtdec = (v->dectiled || ma.s) ? True : False;
	stdec = (v->dectiled || sa.s) ? True : False;
	mgdec = v->dectiled ? True : False;
	sgdec = v->dectiled ? True : False;

	/* master and slave work area dimensions */
	switch (v->major) {
	case OrientTop:
	case OrientBottom:
		ma.w = wa.w;
		ma.h = (sa.n > 0) ? wa.h * v->mwfact : wa.h;
		sa.w = wa.w;
		sa.h = wa.h - ma.h;
		break;
	case OrientLeft:
	case OrientRight:
	default:
		ma.w = (sa.n > 0) ? v->mwfact * wa.w : wa.w;
		ma.h = wa.h;
		sa.w = wa.w - ma.w;
		sa.h = wa.h;
		break;
	}
	ma.b = (ma.n > 1 || mtdec) ? scr->style.border : 0;
	ma.g = mg > ma.b ? mg - ma.b : 0;
	ma.th = th + 2 * (ma.b + ma.g);

	sa.b = scr->style.border;
	sa.g = mg > sa.b ? mg - sa.b : 0;
	sa.th = th + 2 * (sa.b + sa.g);

	overlap = (sa.n > 0) ? ma.b : 0;
	DPRINTF("overlap is %d\n", overlap);

	/* master and slave work area position */
	switch (v->major) {
	case OrientBottom:
		DPRINTF("major orientation is masters bottom(%d)\n", v->major);
		ma.x = wa.x;
		ma.y = wa.y + sa.h;
		sa.x = wa.x;
		sa.y = wa.y;
		ma.y -= overlap;
		ma.h += overlap;
		break;
	case OrientRight:
	default:
		DPRINTF("major orientation is masters right(%d)\n", v->major);
		ma.x = wa.x + sa.w;
		ma.y = wa.y;
		sa.x = wa.x;
		sa.y = wa.y;
		ma.x -= overlap;
		ma.w += overlap;
		break;
	case OrientLeft:
		DPRINTF("major orientation is masters left(%d)\n", v->major);
		ma.x = wa.x;
		ma.y = wa.y;
		sa.x = wa.x + ma.w;
		sa.y = wa.y;
		ma.w += overlap;
		break;
	case OrientTop:
		DPRINTF("major orientation is masters top(%d)\n", v->major);
		ma.x = wa.x;
		ma.y = wa.y;
		sa.x = wa.x;
		sa.y = wa.y + ma.h;
		ma.h += overlap;
		break;
	}
	DPRINTF("master work area %dx%d+%d+%d:%d\n", ma.w, ma.h, ma.x, ma.y, ma.b);
	DPRINTF("slave  work area %dx%d+%d+%d:%d\n", sa.w, sa.h, sa.x, sa.y, sa.b);

	/* master tile dimensions */
	switch (v->minor) {
	case OrientTop:
	case OrientBottom:
		m.w = ma.w;
		m.h = ((ma.n > 0) ? (ma.h - ma.s * ma.th) / (ma.n - ma.s) : ma.h) + ma.b;
		break;
	case OrientLeft:
	case OrientRight:
	default:
		m.w = ((ma.n > 0) ? ma.w / ma.n : ma.w) + ma.b;
		m.h = ma.h;
		break;
	}
	m.b = ma.b;
	m.t = mtdec ? th : 0;
	m.g = mgdec ? gh : 0;

	/* slave tile dimensions */
	switch (v->major) {
	case OrientTop:
	case OrientBottom:
		s.w = ((sa.n > 0) ? sa.w / sa.n : 0) + sa.b;
		s.h = sa.h;
		break;
	case OrientLeft:
	case OrientRight:
	default:
		s.w = sa.w;
		s.h = ((sa.n > 0) ? (sa.h - sa.s * sa.th) / (sa.n - sa.s) : 0) + sa.b;
		break;
	}
	s.b = sa.b;
	s.t = stdec ? th : 0;
	s.g = sgdec ? gh : 0;

	/* position of first master */
	switch (v->minor) {
	case OrientTop:
		DPRINTF("minor orientation is top to bottom(%d)\n", v->minor);
		m.x = ma.x;
		m.y = ma.y;
		break;
	case OrientBottom:
		DPRINTF("minor orientation is bottom to top(%d)\n", v->minor);
		m.x = ma.x;
		m.y = ma.y + ma.h - m.h;
		break;
	case OrientLeft:
		DPRINTF("minor orientation is left to right(%d)\n", v->minor);
		m.x = ma.x;
		m.y = ma.y;
		break;
	case OrientRight:
	default:
		DPRINTF("minor orientation is right to left(%d)\n", v->minor);
		m.x = ma.x + ma.w - m.w;
		m.y = ma.y;
		break;
	}
	DPRINTF("initial master %dx%d+%d+%d:%d\n", m.w, m.h, m.x, m.y, m.b);

	i = 0;
	c = mc = nexttiled(scr->clients, cm);

	/* lay out the master area */
	n = m;

	for (; c && i < ma.n; c = nexttiled(c->next, cm)) {
		if (c->is.max) {
			c->is.max = False;
			ewmh_update_net_window_state(c);
		}
		g = n;
		g.t = c->has.title ? g.t : 0;
		g.g = c->has.grips ? g.g : 0;
		if ((c->was.shaded = c->is.shaded) && (c != sel || !scr->options.autoroll))
			if (!ma.s)
				c->is.shaded = False;
		g.x += ma.g;
		g.y += ma.g;
		g.w -= 2 * (ma.g + g.b);
		g.h -= 2 * (ma.g + g.b);
		if (!c->is.moveresize) {
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &g);
		} else {
			ClientGeometry C = g;

			/* center it where it was before */
			C.x = (c->c.x + c->c.w / 2) - C.w / 2;
			C.y = (c->c.y + c->c.h / 2) - C.h / 2;
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &C);
		}
		if (c->is.shaded && (c != sel || !scr->options.autoroll))
			if (ma.s)
				n.h = ma.th;
		i++;
		switch (v->minor) {
		case OrientTop:
			n.y += n.h - n.b;
			if (i == ma.n - 1)
				n.h = ma.h - (n.y - ma.y);
			break;
		case OrientBottom:
			n.y -= n.h - n.b;
			if (i == ma.n - 1) {
				n.h += (n.y - ma.y);
				n.y = ma.y;
			}
			break;
		case OrientLeft:
			n.x += n.w - n.b;
			if (i == ma.n - 1)
				n.w = ma.w - (n.x - ma.x);
			break;
		case OrientRight:
		default:
			n.x -= n.w - n.b;
			if (i == ma.n - 1) {
				n.w += (n.x - ma.x);
				n.x = ma.x;
			}
			break;
		}
		if (ma.s && c->is.shaded && (c != sel || !scr->options.autoroll)) {
			n.h = m.h;
			ma.s--;
		}
		c->is.shaded = c->was.shaded;
	}

	/* position of first slave */
	switch (v->major) {
	case OrientRight:
		DPRINTF("slave orientation is top to bottom(%d)\n", v->major);
		s.x = sa.x;
		s.y = sa.y;
		break;
	case OrientLeft:
		DPRINTF("slave orientation is bottom to top(%d)\n", v->major);
		s.x = sa.x;
		s.y = sa.y + sa.h - s.h;
		break;
	case OrientTop:
		DPRINTF("slave orientation is left to right(%d)\n", v->major);
		s.x = sa.x;
		s.y = sa.y;
		break;
	case OrientBottom:
	default:
		DPRINTF("slave orientation is right to left(%d)\n", v->major);
		s.x = sa.x + sa.w - s.w;
		s.y = sa.y;
		break;
	}
	DPRINTF("initial slave  %dx%d+%d+%d:%d\n", s.w, s.h, s.x, s.y, s.b);

	/* lay out the slave area - always top->bot, left->right */
	n = s;

	for (; c && i < wa.n; c = nexttiled(c->next, cm)) {
		if (c->is.max) {
			c->is.max = False;
			ewmh_update_net_window_state(c);
		}
		g = n;
		g.t = c->has.title ? g.t : 0;
		g.g = c->has.grips ? g.g : 0;
		if ((c->was.shaded = c->is.shaded) && (c != sel || !scr->options.autoroll))
			if (!sa.s)
				c->is.shaded = False;
		g.x += sa.g;
		g.y += sa.g;
		g.w -= 2 * (sa.g + g.b);
		g.h -= 2 * (sa.g + g.b);
		if (!c->is.moveresize) {
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &g);
		} else {
			ClientGeometry C = g;

			/* center it where it was before */
			C.x = (c->c.x + c->c.w / 2) - C.w / 2;
			C.y = (c->c.y + c->c.h / 2) - C.h / 2;
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &C);
		}
		if (c->is.shaded && (c != sel || !scr->options.autoroll))
			if (sa.s)
				n.h = sa.th;
		i++;
		switch (v->major) {
		case OrientTop:
			/* left to right */
			n.x += n.w - n.b;
			if (i == wa.n - 1)
				n.w = sa.w - (n.x - sa.x);
			break;
		case OrientBottom:
			/* right to left */
			n.x -= n.w - n.b;
			if (i == wa.n - 1) {
				n.w += (n.x - sa.x);
				n.x = sa.x;
			}
			break;
		case OrientLeft:
			/* bottom to top */
			n.y -= n.h - n.b;
			if (i == wa.n - 1) {
				n.h += (n.y - sa.y);
				n.y = sa.y;
			}
			break;
		case OrientRight:
		default:
			/* top to bottom */
			n.y += n.h - n.b;
			if (i == wa.n - 1)
				n.h = sa.h - (n.y - sa.y);
			break;
		}
		if (sa.s && c->is.shaded && (c != sel || !scr->options.autoroll)) {
			n.h = s.h;
			sa.s--;
		}
		c->is.shaded = c->was.shaded;
	}
}

static void
arrange_tile(Monitor *m)
{
	arrangedock(m);
	tile(m);
	arrangefloats(m);
}

static void
grid(Monitor *m)
{
	Client *c;
	Workarea wa;
	View *v;
	int rows, cols, n, i, *rc, *rh, col, row;
	int cw, cl, *rl;
	int gap;

	if (!(c = nexttiled(scr->clients, m)))
		return;

	v = m->curview;

	getworkarea(m, &wa);

	for (n = 0, c = nexttiled(scr->clients, m); c; c = nexttiled(c->next, m), n++) ;

	for (cols = 1; cols < n && cols < v->ncolumns; cols++) ;

	for (rows = 1; (rows * cols) < n; rows++) ;

	/* number of rows per column */
	rc = ecalloc(cols, sizeof(*rc));
	for (col = 0, i = 0; i < n; rc[col]++, i++, col = i % cols) ;

	/* average height per client in column */
	rh = ecalloc(cols, sizeof(*rh));
	for (col = 0; col < cols; rh[col] = wa.h / rc[col], col++) ;

	/* height of last client in column */
	rl = ecalloc(cols, sizeof(*rl));
	for (col = 0; col < cols; rl[col] = wa.h - (rc[col] - 1) * rh[col], col++) ;

	/* average width of column */
	cw = wa.w / cols;

	/* width of last column */
	cl = wa.w - (cols - 1) * cw;

	gap =
	    scr->style.margin >
	    scr->style.border ? scr->style.margin - scr->style.border : 0;

	for (i = 0, col = 0, row = 0, c = nexttiled(scr->clients, m); c && i < n;
	     c = nexttiled(c->next, m), i++, col = i % cols, row = i / cols) {
		ClientGeometry n;

		n.x = gap + wa.x + (col * cw);
		n.y = gap + wa.y + (row * rh[col]);
		n.w = (col == cols - 1) ? cl : cw;
		n.h = (row == rows - 1) ? rl[col] : rh[col];
		n.b = scr->style.border;
		if (col > 0) {
			n.w += n.b;
			n.x -= n.b;
		}
		if (row > 0) {
			n.h += n.b;
			n.y -= n.b;
		}
		n.w -= 2 * (gap + n.b);
		n.h -= 2 * (gap + n.b);
		n.t = (v->dectiled && c->has.title) ? scr->style.titleheight : 0;
		n.g = (v->dectiled && c->has.grips) ? scr->style.gripsheight : 0;
		if (!c->is.moveresize) {
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &n);
		} else {
			ClientGeometry C = n;

			/* center it where it was before */
			C.x = (c->c.x + c->c.w / 2) - C.w / 2;
			C.y = (c->c.y + c->c.h / 2) - C.h / 2;
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &C);
		}
	}
	free(rc);
	free(rh);
	free(rl);
}

static void
arrange_grid(Monitor *m)
{
	arrangedock(m);
	grid(m);
	arrangefloats(m);
}

static void
monocle(Monitor *m)
{
	Client *c;
	Workarea w;
	View *v;

	getworkarea(m, &w);
	v = m->curview;
	for (c = nexttiled(scr->clients, m); c; c = nexttiled(c->next, m)) {
		ClientGeometry g;

		memcpy(&g, &w, sizeof(w));
		g.t = (v->dectiled && c->has.title) ? scr->style.titleheight : 0;
		g.g = (v->dectiled && c->has.grips) ? scr->style.gripsheight : 0;
		g.b = (g.t || g.g) ? scr->style.border : 0;
		g.w -= 2 * g.b;
		g.h -= 2 * g.b;

		DPRINTF("CALLING reconfigure()\n");
		reconfigure(c, &g);
	}
}

static void
arrange_monocle(Monitor *m)
{
	arrangedock(m);
	monocle(m);
	arrangefloats(m);
}

static void
arrange_float(Monitor *m)
{
	arrangedock(m);
	arrangefloats(m);
}

static void
arrangemon(Monitor *m)
{
	Client *c;
	int struts;

	if (m->curview->layout->arrange)
		m->curview->layout->arrange(m);
	struts = m->curview->barpos;
	for (c = scr->stack; c; c = c->snext) {
		if ((clientmonitor(c) == m) &&
		    ((!c->is.bastard && !c->is.dockapp && !(c->is.icon || c->is.hidden))
		     || ((c->is.bastard || c->is.dockapp) && struts == StrutsOn))) {
			unban(c, m);
		}
	}

	for (c = scr->stack; c; c = c->snext) {
		if ((clientmonitor(c) == NULL)
		    || (!c->is.bastard && !c->is.dockapp && (c->is.icon || c->is.hidden))
		    || ((c->is.bastard || c->is.dockapp) && struts == StrutsHide)) {
			ban(c);
		}
	}
	for (c = scr->stack; c; c = c->snext)
		ewmh_update_net_window_state(c);
}

static void
restack()
{
	Client *c, **ol, **cl, **sl;
	Window *wl;
	int i, j, n;

	for (n = 0, c = scr->stack; c; c = c->snext, n++) ;
	if (!n) {
		ewmh_update_net_client_list();
		return;
	}
	wl = ecalloc(n, sizeof(*wl));
	ol = ecalloc(n, sizeof(*ol));
	cl = ecalloc(n, sizeof(*cl));
	sl = ecalloc(n, sizeof(*sl));
	/* 
	 * EWMH WM SPEC 1.5 Draft 1:
	 *
	 * Stacking order
	 *
	 * To obtain good interoperability betweeen different Desktop
	 * Environments, the following layerd stacking order is
	 * recommended, from the bottom:
	 *
	 * - windows of type _NET_WM_TYPE_DESKTOP
	 * - windows having state _NET_WM_STATE_BELOW
	 * - windows not belonging in any other layer
	 * - windows of type _NET_WM_TYPE_DOCK (unless they have state
	 *   _NET_WM_TYPE_BELOW) and windows having state
	 *   _NET_WM_STATE_ABOVE
	 * - focused windows having state _NET_WM_STATE_FULLSCREEN
	 */
	for (i = 0, j = 0, c = scr->stack; c; ol[i] = cl[i] = c, i++, c = c->snext) ;
	/* focused windows having state _NET_WM_STATE_FULLSCREEN */
	XPRINTF("%s", "LAYER: focused windows having state _NET_WM_STATE_FULLSCREEN:\n");
	for (i = 0; i < n; i++) {
		if (!(c = cl[i]))
			continue;
		if (sel == c && c->is.max) {
			cl[i] = NULL;
			wl[j] = c->frame;
			sl[j] = c;
			j++;
			XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		}
	}
	/* windows of type _NET_WM_TYPE_DOCK (unless they have state _NET_WM_STATE_BELOW) 
	   and windows having state _NET_WM_STATE_ABOVE. */
	XPRINTF("%s",
		"LAYER: _NET_WINDOW_TYPE_DOCK not _NET_WM_STATE_BELOW and _NET_WM_STATE_ABOVE:\n");
	for (i = 0; i < n; i++) {
		if (!(c = cl[i]))
			continue;
		if ((WTCHECK(c, WindowTypeDock) && !c->is.below) || c->is.above) {
			cl[i] = NULL;
			wl[j] = c->frame;
			sl[j] = c;
			j++;
			XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		}
	}
	/* windows not belonging in any other layer (but we put floating above special
	   above tiled) */
	XPRINTF("%s", "LAYER: floaters:\n");
	for (i = 0; i < n; i++) {
		if (!(c = cl[i]))
			continue;
		if (!c->is.bastard && (c->is.floater || c->skip.arrange) && !c->is.below
		    && !WTCHECK(c, WindowTypeDesk)) {
			cl[i] = NULL;
			wl[j] = c->frame;
			sl[j] = c;
			j++;
			XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		}
	}
	XPRINTF("%s", "LAYER: bastards:\n");
	for (i = 0; i < n; i++) {
		if (!(c = cl[i]))
			continue;
		if (c->is.bastard && !c->is.below && !WTCHECK(c, WindowTypeDesk)) {
			cl[i] = NULL;
			wl[j] = c->frame;
			sl[j] = c;
			j++;
			XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		}
	}
	XPRINTF("%s", "LAYER: tiled:\n");
	for (i = 0; i < n; i++) {
		if (!(c = cl[i]))
			continue;
		if (!c->is.bastard && !(c->is.floater || c->skip.arrange) && !c->is.below
		    && !WTCHECK(c, WindowTypeDesk)) {
			cl[i] = NULL;
			wl[j] = c->frame;
			sl[j] = c;
			j++;
			XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		}
	}
	/* windows having state _NET_WM_STATE_BELOW */
	XPRINTF("%s", "LAYER: windows having _NET_WM_STATE_BELOW:\n");
	for (i = 0; i < n; i++) {
		if (!(c = cl[i]))
			continue;
		if (c->is.below && !WTCHECK(c, WindowTypeDesk)) {
			cl[i] = NULL;
			wl[j] = c->frame;
			sl[j] = c;
			j++;
			XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		}
	}
	/* windows of type _NET_WM_TYPE_DESKTOP */
	XPRINTF("%s", "LAYER: windows having _NET_WINDOW_TYPE_DESKTOP:\n");
	for (i = 0; i < n; i++) {
		if (!(c = cl[i]))
			continue;
		if (WTCHECK(c, WindowTypeDesk)) {
			cl[i] = NULL;
			wl[j] = c->frame;
			sl[j] = c;
			j++;
			XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		}
	}
	assert(j == n);
	free(cl);

	if (bcmp(ol, sl, n * sizeof(*ol))) {
		XPRINTF("%s", "Old stacking order:\n");
		for (c = scr->stack; c; c = c->snext)
			XPRINTF("client frame 0x%08lx win 0x%08lx name %s%s\n",
				c->frame, c->win, c->name,
				c->is.bastard ? " (bastard)" : "");
		scr->stack = sl[0];
		for (i = 0; i < n - 1; i++)
			sl[i]->snext = sl[i + 1];
		sl[i]->snext = NULL;
		XPRINTF("%s", "New stacking order:\n");
		for (c = scr->stack; c; c = c->snext)
			XPRINTF("client frame 0x%08lx win 0x%08lx name %s%s\n",
				c->frame, c->win, c->name,
				c->is.bastard ? " (bastard)" : "");
	} else {
		XPRINTF("%s", "No new stacking order\n");
	}
	free(ol);
	free(sl);

	if (!window_stack.members || (window_stack.count != n) ||
	    bcmp(window_stack.members, wl, n * sizeof(*wl))) {
		free(window_stack.members);
		window_stack.members = wl;
		window_stack.count = n;

		XRestackWindows(dpy, wl, n);

		ewmh_update_net_client_list();
	} else {
		XPRINTF("%s", "No new stacking order\n");
		free(wl);
	}
	discardenter();
}

static int
segm_overlap(int min1, int max1, int min2, int max2)
{
	int tmp, res = 0;

	if (min1 > max1) {
		tmp = min1;
		min1 = max1;
		max1 = tmp;
	}
	if (min2 > max2) {
		tmp = min2;
		min2 = max2;
		max2 = tmp;
	}
	if (min1 <= min2 && max1 >= min2)
		// min1 min2 (max2?) max1 (max2?)
		res = (max2 <= max1) ? max2 - min2 : max1 - min2;
	else if (min1 <= max2 && max1 >= max2)
		// (min2?) min1 (min2?) max2 max1
		res = (min2 >= min1) ? max2 - min2 : max2 - min1;
	else if (min2 <= min1 && max2 >= min1)
		// min2 min1 (max1?) max2 (max1?)
		res = (max1 <= max2) ? max1 - min1 : max2 - min1;
	else if (min2 <= max1 && max2 >= max1)
		// (min1?) min2 (min1?) max1 max2
		res = (min1 <= min2) ? max1 - min2 : max1 - min1;
	return res;
}

static Bool
wind_overlap(int min1, int max1, int min2, int max2)
{
	return segm_overlap(min1, max1, min2, max2) ? True : False;
}

static Bool
place_overlap(Geometry *c, Geometry *o)
{
	Bool ret = False;

	if (wind_overlap(c->x, c->x + c->w, o->x, o->x + o->w) &&
	    wind_overlap(c->y, c->y + c->h, o->y, o->y + o->h))
		ret = True;
	DPRINTF("%dx%d+%d+%d and %dx%d+%d+%d %s\n", c->w, c->h, c->x, c->y,
		o->w, o->h, o->x, o->y, ret ? "overlap" : "disjoint");
	return ret;
}

static Bool
client_overlap(Client *c, Client *o)
{
	if (wind_overlap(c->c.x, c->c.x + c->c.w, o->c.x, o->c.x + o->c.w) &&
	    wind_overlap(c->c.y, c->c.y + c->c.h, o->c.y, o->c.y + o->c.h))
		return True;
	return False;
}

static Bool
client_occludes(Client *c, Client *o)
{
	Client *s;

	if (!client_overlap(c, o))
		return False;
	for (s = c->snext; s && s != o; s = s->snext) ;
	return (s ? True : False);
}

static Bool
client_occludes_any(Client *c)
{
	Client *s;

	for (s = c->snext; s; s = s->snext)
		if (client_overlap(c, s))
			return True;
	return False;
}

static Bool
client_occluded_any(Client *c)
{
	Client *s;

	for (s = scr->stack; s && s != c; s = s->next)
		if (client_overlap(c, s))
			return True;
	return False;
}

void
restack_client(Client *c, int stack_mode, Client *o)
{
	Client *s, **cp, **op, **lp;

	for (lp = &scr->stack, s = scr->stack; s; lp = &s->snext, s = *lp) ;
	for (cp = &scr->stack, s = scr->stack; s && s != c; cp = &s->snext, s = *cp) ;
	assert(s == c);
	for (op = &scr->stack, s = scr->stack; s && s != o; op = &s->snext, s = *op) ;
	assert(s == o);

	switch (stack_mode) {
	case Above:
		if (o) {
			/* just above sibling */
			*cp = c->snext;
			*op = c;
			c->snext = o;
		} else {
			/* top of stack */
			*cp = c->snext;
			c->snext = scr->stack;
			scr->stack = c;
		}
		break;
	case Below:
		if (o) {
			/* just below sibling */
			*cp = c->snext;
			c->snext = o->snext;
			o->snext = c;
		} else {
			/* bottom of stack */
			*cp = c->snext;
			c->snext = *lp;
			*lp = c;
		}
		break;
	case TopIf:
		if (o) {
			if (!client_occludes(o, c))
				return;
		} else {
			if (!client_occluded_any(c))
				return;
		}
		/* top of stack */
		*cp = c->snext;
		c->snext = scr->stack;
		scr->stack = c;
		break;
	case BottomIf:
		if (o) {
			if (!client_occludes(c, o))
				return;
		} else {
			if (!client_occludes_any(c))
				return;
		}
		/* bottom of stack */
		*cp = c->snext;
		c->snext = *lp;
		*lp = c;
		break;
	case Opposite:
		if (o) {
			if (client_occludes(o, c)) {
				/* top of stack */
				*cp = c->snext;
				c->snext = scr->stack;
				scr->stack = c;
			} else if (client_occludes(c, o)) {
				/* bottom of stack */
				*cp = c->snext;
				c->snext = *lp;
				*lp = c;
			} else
				return;
		} else {
			if (client_occluded_any(c)) {
				/* top of stack */
				*cp = c->snext;
				c->snext = scr->stack;
				scr->stack = c;
			} else if (client_occludes_any(c)) {
				/* bottom of stack */
				*cp = c->snext;
				c->snext = *lp;
				*lp = c;
			} else
				return;
		}
		break;
	default:
		return;
	}
	restack();
}

void
restack_belowif(Client *c, Client *o)
{
	if (o && client_occludes(c, o))
		restack_client(c, Below, o);
}

void
toggleabove(Client *c)
{
	if (!c || (!c->can.above && c->is.managed))
		return;
	c->is.above = !c->is.above;
	if (c->is.managed) {
		restack();
		ewmh_update_net_window_state(c);
	}
}

void
togglebelow(Client *c)
{
	if (!c || (!c->can.below && c->is.managed))
		return;
	c->is.below = !c->is.below;
	if (c->is.managed) {
		restack();
		ewmh_update_net_window_state(c);
	}
}

void
arrange(Monitor *om)
{
	Monitor *m;

	if (!om)
		for (m = scr->monitors; m; m = m->next)
			XMapRaised(dpy, m->veil);
	else
		XMapRaised(dpy, om->veil);
	if (!om)
		for (m = scr->monitors; m; m = m->next)
			arrangemon(m);
	else
		arrangemon(om);
	restack();
	if (!om)
		for (m = scr->monitors; m; m = m->next)
			XUnmapWindow(dpy, m->veil);
	else
		XUnmapWindow(dpy, om->veil);
	discardenter();
}

Layout layouts[] = {
	/* *INDENT-OFF* */
	/* function		symbol	features				major		minor		placement		*/
	{  arrange_float,	'i',	OVERLAP,				0,		0,		ColSmartPlacement	},
	{  arrange_tile,	't',	MWFACT | NMASTER | ZOOM | ROTL | MMOVE,	OrientLeft,	OrientBottom,	ColSmartPlacement	},
	{  arrange_tile,	'b',	MWFACT | NMASTER | ZOOM | ROTL | MMOVE,	OrientBottom,	OrientLeft,	ColSmartPlacement	},
	{  arrange_tile,	'u',	MWFACT | NMASTER | ZOOM | ROTL | MMOVE,	OrientTop,	OrientRight,	ColSmartPlacement	},
	{  arrange_tile,	'l',	MWFACT | NMASTER | ZOOM | ROTL | MMOVE,	OrientRight,	OrientTop,	ColSmartPlacement	},
	{  arrange_monocle,	'm',	0,					0,		0,		ColSmartPlacement	},
	{  arrange_float,	'f',	OVERLAP,				0,		0,		ColSmartPlacement	},
	{  arrange_grid,	'g',	NCOLUMNS | ROTL | MMOVE,		OrientLeft,	OrientTop,	ColSmartPlacement	},
	{  NULL,		'\0',	0,					0,		0,		0			}
	/* *INDENT-ON* */
};

void
setlayout(const char *arg)
{
	Monitor *cm;

	if (!(cm = selmonitor()))
		return;
	if (arg) {
		View *v;
		Layout *l;

		for (l = layouts; l->symbol; l++)
			if (*arg == l->symbol)
				break;
		if (!l->symbol)
			return;
		v = cm->curview;
		v->layout = l;
		v->major = l->major;
		v->minor = l->minor;
		v->placement = l->placement;
	}
	arrange(cm);
	ewmh_update_net_desktop_modes();
}

void
raiseclient(Client *c)
{
	detachstack(c);
	attachstack(c, True);
	restack();
}

void
lowerclient(Client *c)
{
	detachstack(c);
	attachstack(c, False);
	restack();
}

void
raiselower(Client *c)
{
	if (c == scr->stack)
		lowerclient(c);
	else
		raiseclient(c);
}

void
raisetiled(Client *c)
{
	if (c->is.bastard || c->is.dockapp || !isfloating(c, c->cmon)) {
		CPRINTF(c, "raising non-floating client on focus\n");
		if (c->is.bastard || c->is.dockapp) {
			c->is.below = False;
			c->is.above = True;
		}
		raiseclient(c);
	}
}

void
lowertiled(Client *c)
{
	if (c->is.bastard || c->is.dockapp || !isfloating(c, c->cmon)) {
		CPRINTF(c, "lowering non-floating client on focus\n");
		if (c->is.bastard || c->is.dockapp) {
			c->is.below = True;
			c->is.above = False;
		}
		lowerclient(c);
	}
}

void
setmwfact(Monitor *m, View *v, double factor)
{
	if (factor < 0.1)
		factor = 0.1;
	if (factor > 0.9)
		factor = 0.9;
	if (v->mwfact != factor) {
		v->mwfact = factor;
		arrange(m);
	}
}

void
setnmaster(Monitor *m, View *v, int n)
{
	Bool master, column;
	int length;

	master = FEATURES(v->layout, NMASTER) ? True : False;
	column = FEATURES(v->layout, NCOLUMNS) ? True : False;

	if (!master && !column)
		return;

	if (n < 1) {
		if (master)
			n = DEFNMASTER;
		else
			n = DEFNCOLUMNS;
	}
	switch (v->major) {
	case OrientTop:
	case OrientBottom:
		length = m->wa.h;
		if (master) {
			switch (v->minor) {
			case OrientTop:
			case OrientBottom:
				length = (double) m->wa.h * v->mwfact;
				break;
			default:
				break;
			}
		}
		if (length / n < 2 * (scr->style.titleheight + scr->style.border))
			return;
		break;
	case OrientLeft:
	case OrientRight:
		length = m->wa.w;
		if (master) {
			switch (v->minor) {
			case OrientLeft:
			case OrientRight:
				length = (double) m->wa.w * v->mwfact;
				break;
			default:
				break;
			}
		}
		if (length / n < 2 * (6 * scr->style.titleheight + scr->style.border))
			return;
		break;
	case OrientLast:
		return;
	}
	if (master) {
		if (v->nmaster != n) {
			v->nmaster = n;
			arrange(m);
		}
	} else {
		if (v->ncolumns != n) {
			v->ncolumns = n;
			arrange(m);
		}
	}
}

void
decnmaster(Monitor *m, View *v, int n)
{
	Bool master, column;

	master = FEATURES(v->layout, NMASTER) ? True : False;
	column = FEATURES(v->layout, NCOLUMNS) ? True : False;

	if (!master && !column)
		return;
	if (master)
		setnmaster(m, v, v->nmaster - n);
	else
		setnmaster(m, v, v->ncolumns - n);
}

void
incnmaster(Monitor *m, View *v, int n)
{
	Bool master, column;

	master = FEATURES(v->layout, NMASTER) ? True : False;
	column = FEATURES(v->layout, NCOLUMNS) ? True : False;

	if (!master && !column)
		return;
	if (master)
		setnmaster(m, v, v->nmaster + n);
	else
		setnmaster(m, v, v->ncolumns + n);
}

static void
swapstacked(Client *c, Client *s)
{
	Client *cn = c->next;
	Client *cp = c->prev;
	Client *sn = s->next;
	Client *sp = s->prev;

	assert(s != c);
	assert(cn || sn);	/* only one tail */
	assert(cp || sp);	/* only one head */

	if (scr->clients == c)
		scr->clients = s;
	else if (scr->clients == s)
		scr->clients = c;

	if (cn == s)
		cn = c;
	if (cp == s)
		cp = c;
	if (sn == c)
		sn = s;
	if (sp == c)
		sp = s;

	s->next = cn;
	s->prev = cp;
	c->next = sn;
	c->prev = sp;

	if (cn)
		cn->prev = s;
	if (cp)
		cp->next = s;
	if (sn)
		sn->prev = c;
	if (sp)
		sp->next = c;

#ifdef DEBUG
	assert(validlist());
#endif
}

static Client *
ontiled(Client *c, Monitor *m, int cx, int cy)
{
	Client *s;

	for (s = nexttiled(scr->clients, m);
	     s && (s == c ||
		   s->c.x > cx || cx > s->c.x + s->c.w ||
		   s->c.y > cy || cy > s->c.y + s->c.h); s = nexttiled(s->next, m)) ;
	return s;
}

static Client *
ondockapp(Client *c, Monitor *m, int cx, int cy)
{
	Client *s;

	for (s = nextdockapp(scr->clients, m);
	     s && (s == c ||
		   s->c.x > cx || cx > s->c.x + s->c.w ||
		   s->c.y > cy || cy > s->c.y + s->c.h); s = nextdockapp(s->next, m)) ;
	return s;
}

static Client *
onstacked(Client *c, Monitor *m, int cx, int cy)
{
	return (c->is.dockapp ? ondockapp(c, m, cx, cy) : ontiled(c, m, cx, cy));
}

static Bool
ismoveevent(Display *display, XEvent *event, XPointer arg)
{
	switch (event->type) {
	case ButtonPress:
	case ButtonRelease:
	case ClientMessage:
	case ConfigureRequest:
	case Expose:
	case MapRequest:
	case MotionNotify:
		return True;
	default:
		if (event->type == XSyncAlarmNotify + ebase[XsyncBase])
			return True;
		return False;
	}
}

static Bool
move_begin(Client *c, Monitor *m, Bool toggle, int move)
{
	Bool isfloater;

	if (!c->can.move)
		return False;

	/* regrab pointer with move cursor */
	XGrabPointer(dpy, scr->root, False, MOUSEMASK, GrabModeAsync,
		     GrabModeAsync, None, cursor[move], CurrentTime);

	isfloater = isfloating(c, NULL) ? True : False;

	c->is.moveresize = True;
	c->was.is = 0;
	if (toggle || isfloater) {
		if ((c->was.full = c->is.full))
			c->is.full = False;
		if ((c->was.max = c->is.max))
			c->is.max = False;
		if ((c->was.maxv = c->is.maxv))
			c->is.maxv = False;
		if ((c->was.maxh = c->is.maxh))
			c->is.maxh = False;
		if ((c->was.fill = c->is.fill))
			c->is.fill = False;
		if ((c->was.shaded = c->is.shaded))
			c->is.shaded = False;
		if (!isfloater) {
			save(c);	/* tear out at current geometry */
			/* XXX: could this not just be raiseclient(c) ??? */
			detachstack(c);
			attachstack(c, True);
			togglefloating(c);
		} else if (c->was.is) {
			c->was.floater = isfloater;
			updatefloat(c, m);
			raiseclient(c);
		} else
			raiseclient(c);
	} else {
		/* can't move tiled in monocle mode */
		if (!c->is.dockapp && !MFEATURES(m, MMOVE)) {
			c->is.moveresize = False;
			return False;
		}
	}
	return True;
}

static Bool
move_cancel(Client *c, Monitor *m, ClientGeometry * orig)
{
	Bool wasfloating;

	if (!c->is.moveresize)
		return False;	/* nothing to cancel */

	c->is.moveresize = False;
	wasfloating = c->was.floater;
	c->was.floater = False;

	if (isfloating(c, NULL)) {
		if (c->was.is) {
			c->is.full = c->was.full;
			c->is.max = c->was.max;
			c->is.maxv = c->was.maxv;
			c->is.maxh = c->was.maxh;
			c->is.fill = c->was.fill;
			c->is.shaded = c->was.shaded;
		}
		if (wasfloating) {
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, orig);
			save(c);
			updatefloat(c, m);
		} else
			togglefloating(c);
	} else
		arrange(m);
	return True;
}

static Bool
move_finish(Client *c, Monitor *m)
{
	Bool wasfloating;

	if (!c->is.moveresize)
		return False;	/* didn't start or was cancelled */

	c->is.moveresize = False;
	wasfloating = c->was.floater;
	c->was.floater = False;

	if (isfloating(c, m)) {
		if (c->was.is) {
			c->is.full = c->was.full;
			c->is.max = c->was.max;
			c->is.maxv = c->was.maxv;
			c->is.maxh = c->was.maxh;
			c->is.fill = c->was.fill;
			c->is.shaded = c->was.shaded;
		}
		if (wasfloating)
			updatefloat(c, m);
		else
			arrange(m);
	} else
		arrange(m);
	return True;
}

/* TODO: handle movement across EWMH desktops */

Bool
mousemove(Client *c, XEvent *e, Bool toggle)
{
	int dx, dy;
	int x_root, y_root;
	Monitor *m, *nm;
	ClientGeometry n, o;
	Bool moved = False, isfloater;
	int move = CurMove;

	x_root = e->xbutton.x_root;
	y_root = e->xbutton.y_root;

	/* The monitor that we clicked in the window from and the monitor in which the
	   window was last laid out can be two different places for floating windows
	   (i.e. a window that overlaps the boundary between two monitors.  Because we
	   handle changing monitors and even changing screens here, we should use the
	   monitor to which it was last associated instead of where it was clicked.  This 
	   is actually important because the monitor it is on might be floating and the
	   monitor the edge was clicked on might be tiled. */

	if (!(m = getmonitor(x_root, y_root)) || (c->cmon && m != c->cmon))
		if (!(m = c->cmon)) {
			CPRINTF(c, "No monitor to move from!\n");
			XUngrabPointer(dpy, CurrentTime);
			return moved;
		}

	if (!c->can.floats || c->is.dockapp)
		toggle = False;

	isfloater = (toggle || isfloating(c, NULL)) ? True : False;

	if (XGrabPointer(dpy, scr->root, False, MOUSEMASK, GrabModeAsync,
			 GrabModeAsync, None, None, CurrentTime) != GrabSuccess) {
		CPRINTF(c, "Couldn't grab pointer!\n");
		XUngrabPointer(dpy, CurrentTime);
		return moved;
	}

	getgeometry(c, &c->c, &n);
	getgeometry(c, &c->c, &o);

	for (;;) {
		Client *s;
		XEvent ev;

		XIfEvent(dpy, &ev, &ismoveevent, (XPointer) c);
		geteventscr(&ev);

		switch (ev.type) {
		case ButtonRelease:
			break;
		case ClientMessage:
			if (ev.xclient.message_type == _XA_NET_WM_MOVERESIZE) {
				if (ev.xclient.data.l[2] == 11) {
					/* _NET_WM_MOVERESIZE_CANCEL */
					CPRINTF(c, "Move cancelled!\n");
					moved = move_cancel(c, m, &o);
					break;
				}
				continue;
			}
			/* fall through */
		case ConfigureRequest:
		case Expose:
		case MapRequest:
		default:
			scr = event_scr;
			handle_event(&ev);
			continue;
		case MotionNotify:
			XSync(dpy, False);
			dx = (ev.xmotion.x_root - x_root);
			dy = (ev.xmotion.y_root - y_root);
			pushtime(ev.xmotion.time);
			if (!moved) {
				if (abs(dx) < scr->options.dragdist
				    && abs(dy) < scr->options.dragdist)
					continue;
				if (!(moved = move_begin(c, m, toggle, move))) {
					CPRINTF(c, "Couldn't move client!\n");
					break;
				}
				getgeometry(c, &c->c, &n);
			}
			nm = getmonitor(ev.xmotion.x_root, ev.xmotion.y_root);
			if (c->is.dockapp || !isfloater) {
				Client *s;

				/* cannot move off monitor when shuffling tiled */
				if (event_scr != scr || nm != m) {
					CPRINTF(c, "Cannot move off monitor!\n");
					continue;
				}
				if ((s =
				     onstacked(c, m, ev.xmotion.x_root,
					       ev.xmotion.y_root))) {
					swapstacked(c, s);
					arrange(m);
				}
				/* move center to new position */
				getgeometry(c, &c->c, &n);
				n.x = ev.xmotion.x_root - n.w / 2;
				n.y = ev.xmotion.y_root - n.h / 2;
				DPRINTF("CALLING reconfigure()\n");
				reconfigure(c, &n);
				continue;
			}
			n.x = o.x + dx;
			n.y = o.y + dy;
			if (nm && scr->options.snap && !(ev.xmotion.state & ControlMask)) {
				Workarea w;
				int nx2 = n.x + c->c.w + 2 * c->c.b;
				int ny2 = n.y + c->c.h + 2 * c->c.b;

				getworkarea(nm, &w);
				if (abs(n.x - w.x) < scr->options.snap)
					n.x += w.x - n.x;
				else if (abs(nx2 - (w.x + w.w)) < scr->options.snap)
					n.x += (w.x + w.w) - nx2;
				else
					for (s = event_scr->stack; s; s = s->snext) {
						int sx = s->c.x;
						int sy = s->c.y;
						int sx2 = s->c.x + s->c.w + 2 * s->c.b;
						int sy2 = s->c.y + s->c.h + 2 * s->c.b;

						if (wind_overlap(n.y, ny2, sy, sy2)) {
							if (abs(n.x - sx) < scr->options.snap)
								n.x += sx - n.x;
							else if (abs(nx2 - sx2) <
								 scr->options.snap)
								n.x += sx2 - nx2;
							else
								continue;
							break;
						}
					}
				if (abs(n.y - w.y) < scr->options.snap)
					n.y += w.y - n.y;
				else if (abs(ny2 - (w.y + w.h)) < scr->options.snap)
					n.y += (w.y + w.h) - ny2;
				else
					for (s = event_scr->stack; s; s = s->snext) {
						int sx = s->c.x;
						int sy = s->c.y;
						int sx2 = s->c.x + s->c.w + 2 * s->c.b;
						int sy2 = s->c.y + s->c.h + 2 * s->c.b;

						if (wind_overlap(n.x, nx2, sx, sx2)) {
							if (abs(n.y - sy) < scr->options.snap)
								n.y += sy - n.y;
							else if (abs(ny2 - sy2) <
								 scr->options.snap)
								n.y += sy2 - ny2;
							else
								continue;
							break;
						}
					}
			}
			if (event_scr != scr)
				reparentclient(c, event_scr, n.x, n.y);
			if (nm && m != nm) {
				c->tags = nm->curview->seltags;
				ewmh_update_net_window_desktop(c);
				drawclient(c);
				arrange(NULL);
				m = nm;
			}
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &n);
			save(c);
			continue;
		}
		XUngrabPointer(dpy, CurrentTime);
		break;
	}
	if (move_finish(c, m))
		moved = True;
	discardenter();
	ewmh_update_net_window_state(c);
	return moved;
}

static int
findcorner(Client *c, int x_root, int y_root)
{
	int cx, cy, from;
	float dx, dy;

	/* TODO: if we are on the resize handles we need to consider grip width */
	cx = c->c.x + c->c.w / 2;
	cy = c->c.y + c->c.h / 2;
	dx = (float) abs(cx - x_root) / (float) c->c.w;
	dy = (float) abs(cy - y_root) / (float) c->c.h;

	if (y_root < cy) {
		/* top */
		if (x_root < cx) {
			/* top-left */
			if (!c->can.sizev)
				from = CurResizeLeft;
			else if (!c->can.sizeh)
				from = CurResizeTop;
			else {
				if (dx < dy * 0.4) {
					from = CurResizeTop;
				} else if (dy < dx * 0.4) {
					from = CurResizeLeft;
				} else {
					from = CurResizeTopLeft;
				}
			}
		} else {
			/* top-right */
			if (!c->can.sizev)
				from = CurResizeRight;
			else if (!c->can.sizeh)
				from = CurResizeTop;
			else {
				if (dx < dy * 0.4) {
					from = CurResizeTop;
				} else if (dy < dx * 0.4) {
					from = CurResizeRight;
				} else {
					from = CurResizeTopRight;
				}
			}
		}
	} else {
		/* bottom */
		if (x_root < cx) {
			/* bottom-left */
			if (!c->can.sizev)
				from = CurResizeLeft;
			else if (!c->can.sizeh)
				from = CurResizeBottom;
			else {
				if (dx < dy * 0.4) {
					from = CurResizeBottom;
				} else if (dy < dx * 0.4) {
					from = CurResizeLeft;
				} else {
					from = CurResizeBottomLeft;
				}
			}
		} else {
			/* bottom-right */
			if (!c->can.sizev)
				from = CurResizeRight;
			else if (!c->can.sizeh)
				from = CurResizeBottom;
			else {
				if (dx < dy * 0.4) {
					from = CurResizeBottom;
				} else if (dy < dx * 0.4) {
					from = CurResizeRight;
				} else {
					from = CurResizeBottomRight;
				}
			}
		}
	}
	return from;
}

static Bool
isresizeevent(Display *display, XEvent *event, XPointer arg)
{
	switch (event->type) {
	case ButtonPress:
	case ButtonRelease:
	case ClientMessage:
	case ConfigureRequest:
	case Expose:
	case MapRequest:
	case MotionNotify:
		return True;
	default:
		if (event->type == XSyncAlarmNotify + ebase[XsyncBase])
			return True;
		return False;
	}
}

static Bool
resize_begin(Client *c, Monitor *m, Bool toggle, int from)
{
	Bool isfloater;

	if (!c->can.size)
		return False;

	switch (from) {
	case CurResizeTop:
	case CurResizeBottom:
		if (!c->can.sizev)
			return False;
		break;
	case CurResizeLeft:
	case CurResizeRight:
		if (!c->can.sizeh)
			return False;
		break;
	default:
		break;
	}

	/* regrab pointer with resize cursor */
	XGrabPointer(dpy, scr->root, False, MOUSEMASK, GrabModeAsync,
		     GrabModeAsync, None, cursor[from], CurrentTime);

	isfloater = isfloating(c, m) ? True : False;

	c->is.moveresize = True;
	c->was.is = 0;
	if (toggle || isfloater) {
		if ((c->was.full = c->is.full))
			c->is.full = False;
		if ((c->was.max = c->is.max))
			c->is.max = False;
		if ((c->was.maxv = c->is.maxv))
			c->is.maxv = False;
		if ((c->was.maxh = c->is.maxh))
			c->is.maxh = False;
		if ((c->was.fill = c->is.fill))
			c->is.fill = False;
		if ((c->was.shaded = c->is.shaded))
			c->is.shaded = False;
		if (!isfloater) {
			save(c);	/* tear out at current geometry */
			/* XXX: could this not just be raiseclient(c) ??? */
			detachstack(c);
			attachstack(c, True);
			togglefloating(c);
		} else if (c->was.is) {
			c->was.floater = isfloater;
			updatefloat(c, m);
			raiseclient(c);
		} else
			raiseclient(c);
	} else {
		/* can't resize tiled yet... */
		c->is.moveresize = False;
		return False;
	}
	return True;
}

static Bool
resize_cancel(Client *c, Monitor *m, ClientGeometry * orig)
{
	Bool wasfloating;

	if (!c->is.moveresize)
		return False;	/* nothing to cancel */

	c->is.moveresize = False;
	wasfloating = c->was.floater;
	c->was.floater = False;

	if (isfloating(c, m)) {
		if (c->was.is) {
			c->is.full = c->was.full;
			c->is.max = c->was.max;
			c->is.maxv = c->was.maxv;
			c->is.maxh = c->was.maxh;
			c->is.fill = c->was.fill;
			c->is.shaded = c->was.shaded;
		}
		if (wasfloating) {
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, orig);
			save(c);
			updatefloat(c, m);
		} else
			togglefloating(c);
	} else
		arrange(m);
	return True;
}

static Bool
resize_finish(Client *c, Monitor *m)
{
	Bool wasfloating;

	if (!c->is.moveresize)
		return False;	/* didn't start or was cancelled */

	c->is.moveresize = False;
	wasfloating = c->was.floater;
	c->was.floater = False;

	if (isfloating(c, m)) {
		if (c->was.is) {
			c->is.shaded = c->was.shaded;
		}
		if (wasfloating)
			updatefloat(c, m);
		else
			arrange(m);
	} else
		arrange(m);
	return True;
}

Bool
mouseresize_from(Client *c, int from, XEvent *e, Bool toggle)
{
	int dx, dy;
	int x_root, y_root;
	Monitor *m, *nm;
	ClientGeometry n, o;
	Bool resized = False;

	x_root = e->xbutton.x_root;
	y_root = e->xbutton.y_root;

	if (!(m = getmonitor(x_root, y_root)) || (c->cmon && m != c->cmon))
		if (!(m = c->cmon)) {
			XUngrabPointer(dpy, CurrentTime);
			return resized;
		}

	if (!c->can.floats || c->is.dockapp)
		toggle = False;

	if (XGrabPointer(dpy, scr->root, False, MOUSEMASK, GrabModeAsync,
			 GrabModeAsync, None, None, CurrentTime) != GrabSuccess) {
		XUngrabPointer(dpy, CurrentTime);
		return resized;
	}

	getgeometry(c, &c->c, &o);
	getgeometry(c, &c->c, &n);

	for (;;) {
		Workarea w;
		int rx, ry, nx2, ny2;
		unsigned int snap;
		Client *s;
		XEvent ev;

		XIfEvent(dpy, &ev, &isresizeevent, (XPointer) c);
		geteventscr(&ev);

		switch (ev.type) {
		case ButtonRelease:
			break;
		case ClientMessage:
			if (ev.xclient.message_type == _XA_NET_WM_MOVERESIZE) {
				if (ev.xclient.data.l[2] == 11) {
					/* _NET_WM_MOVERESIZE_CANCEL */
					resized = resize_cancel(c, m, &o);
					break;
				}
				continue;
			}
			/* fall through */
		case ConfigureRequest:
		case Expose:
		case MapRequest:
		default:
			scr = event_scr;
			handle_event(&ev);
			continue;
		case MotionNotify:
			if (event_scr != scr)
				continue;
			XSync(dpy, False);
			dx = (x_root - ev.xmotion.x_root);
			dy = (y_root - ev.xmotion.y_root);
			pushtime(ev.xmotion.time);
			if (!resized) {
				if (abs(dx) < scr->options.dragdist
				    && abs(dy) < scr->options.dragdist)
					continue;
				if (!(resized = resize_begin(c, m, toggle, from)))
					break;
				getgeometry(c, &c->c, &n);
			}
			switch (from) {
			case CurResizeTopLeft:
				n.w = o.w + dx;
				n.h = o.h + dy;
				constrain(c, &n);
				n.x = o.x + o.w - n.w;
				n.y = o.y + o.h - n.h;
				nx2 = n.x + n.w + 2 * c->c.b;
				ny2 = n.y + n.h + 2 * c->c.b;
				rx = n.x;
				ry = n.y;
				break;
			case CurResizeTop:
				n.w = o.w;
				n.h = o.h + dy;
				constrain(c, &n);
				n.x = o.x;
				n.y = o.y + o.h - n.h;
				nx2 = n.x + n.w + 2 * c->c.b;
				ny2 = n.y + n.h + 2 * c->c.b;
				rx = n.x + n.w / 2 + c->c.b;
				ry = n.y;
				break;
			case CurResizeTopRight:
				n.w = o.w - dx;
				n.h = o.h + dy;
				constrain(c, &n);
				n.x = o.x;
				n.y = o.y + o.h - n.h;
				nx2 = n.x + n.w + 2 * c->c.b;
				ny2 = n.y + n.h + 2 * c->c.b;
				rx = n.x + n.w + 2 * c->c.b;
				ry = n.y;
				break;
			case CurResizeRight:
				n.w = o.w - dx;
				n.h = o.h;
				constrain(c, &n);
				n.x = o.x;
				n.y = o.y;
				nx2 = n.x + n.w + 2 * c->c.b;
				ny2 = n.y + n.h + 2 * c->c.b;
				rx = n.x + n.w + 2 * c->c.b;
				ry = n.y + n.h / 2 + c->c.b;
				break;
			default:
			case CurResizeBottomRight:
				n.w = o.w - dx;
				n.h = o.h - dy;
				constrain(c, &n);
				n.x = o.x;
				n.y = o.y;
				nx2 = n.x + n.w + 2 * c->c.b;
				ny2 = n.y + n.h + 2 * c->c.b;
				rx = n.x + n.w + 2 * c->c.b;
				ry = n.y + n.h + 2 * c->c.b;
				break;
			case CurResizeBottom:
				n.w = o.w;
				n.h = o.h - dy;
				constrain(c, &n);
				n.x = o.x;
				n.y = o.y;
				nx2 = n.x + n.w + 2 * c->c.b;
				ny2 = n.y + n.h + 2 * c->c.b;
				rx = n.x + n.w + 2 * c->c.b;
				ry = n.y + n.h + 2 * c->c.b;
				break;
			case CurResizeBottomLeft:
				n.w = o.w + dx;
				n.h = o.h - dy;
				constrain(c, &n);
				n.x = o.x + o.w - n.w;
				n.y = o.y;
				nx2 = n.x + n.w + 2 * c->c.b;
				ny2 = n.y + n.h + 2 * c->c.b;
				rx = n.x;
				ry = n.y + n.h + 2 * c->c.b;
				break;
			case CurResizeLeft:
				n.w = o.w + dx;
				n.h = o.h;
				constrain(c, &n);
				n.x = o.x + o.w - n.w;
				n.y = o.y;
				nx2 = n.x + n.w + 2 * c->c.b;
				ny2 = n.y + n.h + 2 * c->c.b;
				rx = n.x;
				ry = n.y + n.h / 2 + c->c.b;
				break;
			}
			if ((snap = (ev.xmotion.state & ControlMask) ? 0 : scr->options.snap)) {
				if ((nm = getmonitor(rx, ry))) {
					getworkarea(nm, &w);
					if (abs(rx - w.x) < snap)
						n.x += w.x - rx;
					else
						for (s = scr->stack; s; s = s->next) {
							int sx = s->c.x;
							int sy = s->c.y;
							int sx2 =
							    s->c.x + s->c.w + 2 * s->c.b;
							int sy2 =
							    s->c.y + s->c.h + 2 * s->c.b;
							if (wind_overlap
							    (n.y, ny2, sy, sy2)) {
								if (abs(rx - sx) < snap)
									n.x += sx - rx;
								else if (abs(rx - sx2) <
									 snap)
									n.x += sx2 - rx;
								else
									continue;
								break;
							}
						}
					if (abs(ry - w.y) < snap)
						n.y += w.y - ry;
					else
						for (s = scr->stack; s; s = s->next) {
							int sx = s->c.x;
							int sy = s->c.y;
							int sx2 =
							    s->c.x + s->c.w + 2 * s->c.b;
							int sy2 =
							    s->c.y + s->c.h + 2 * s->c.b;
							if (wind_overlap
							    (n.x, nx2, sx, sx2)) {
								if (abs(ry - sy) < snap)
									n.y += sy - ry;
								else if (abs(ry - sy2) <
									 snap)
									n.y += sy2 - ry;
								else
									continue;
								break;
							}
						}
				}
			}
			if (n.w < MINWIDTH)
				n.w = MINWIDTH;
			if (n.h < MINHEIGHT)
				n.h = MINHEIGHT;
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &n);
			if (isfloating(c, m))
				save(c);
			continue;
		}
		XUngrabPointer(dpy, CurrentTime);
		break;
	}
	if (resize_finish(c, m))
		resized = True;
	discardenter();
	ewmh_update_net_window_state(c);
	return resized;
}

Bool
mouseresize(Client *c, XEvent *e, Bool toggle)
{
	int from;

	if (!c->can.size || (!c->can.sizeh && !c->can.sizev)) {
		XUngrabPointer(dpy, CurrentTime);
		return False;
	}
	from = findcorner(c, e->xbutton.x_root, e->xbutton.y_root);
	return mouseresize_from(c, from, e, toggle);
}

static void
getreference(int *xr, int *yr, Geometry *g, int gravity)
{
	switch (gravity) {
	case UnmapGravity:
	case NorthWestGravity:
	default:
		*xr = g->x;
		*yr = g->y;
		break;
	case NorthGravity:
		*xr = g->x + g->b + g->w / 2;
		*yr = g->y;
		break;
	case NorthEastGravity:
		*xr = g->x + 2 * g->b + g->w;
		*yr = g->y;
		break;
	case WestGravity:
		*xr = g->x;
		*yr = g->y + g->b + g->h / 2;
		break;
	case CenterGravity:
		*xr = g->x + g->b + g->w / 2;
		*yr = g->y + g->b + g->h / 2;
		break;
	case EastGravity:
		*xr = g->x + 2 * g->b + g->w;
		*yr = g->y + g->b + g->h / 2;
		break;
	case SouthWestGravity:
		*xr = g->x;
		*yr = g->y + 2 * g->b + g->h;
		break;
	case SouthGravity:
		*xr = g->x + g->b + g->w / 2;
		*yr = g->y + 2 * g->b + g->h;
		break;
	case SouthEastGravity:
		*xr = g->x + 2 * g->b + g->w;
		*yr = g->y + 2 * g->b + g->h;
		break;
	case StaticGravity:
		*xr = g->x + g->b;
		*yr = g->y + g->b;
		break;
	}
}

static void
putreference(int xr, int yr, Geometry *g, int gravity)
{
	switch (gravity) {
	case UnmapGravity:
	case NorthWestGravity:
	default:
		g->x = xr;
		g->y = yr;
		break;
	case NorthGravity:
		g->x = xr - g->b - g->w / 2;
		g->y = yr;
		break;
	case NorthEastGravity:
		g->x = xr - 2 * g->b - g->w;
		g->y = yr;
		break;
	case WestGravity:
		g->x = xr;
		g->y = yr - g->b - g->h / 2;
		break;
	case CenterGravity:
		g->x = xr - g->b - g->w / 2;
		g->y = yr - g->b - g->h / 2;
		break;
	case EastGravity:
		g->x = xr - 2 * g->b - g->w;
		g->y = yr - g->b - g->h / 2;
		break;
	case SouthWestGravity:
		g->x = xr;
		g->y = yr - 2 * g->b - g->h;
		break;
	case SouthGravity:
		g->x = xr - g->b - g->w / 2;
		g->y = yr - 2 * g->b - g->h;
		break;
	case SouthEastGravity:
		g->x = xr - 2 * g->b - g->w;
		g->y = yr - 2 * g->b - g->h;
		break;
	case StaticGravity:
		g->x = xr - g->b;
		g->y = yr - g->b;
		break;
	}
}

void
moveresizekb(Client *c, int dx, int dy, int dw, int dh, int gravity)
{
	Monitor *m;

	/* FIXME: this just resizes and moves the window, it does handle changing
	   monitors */
	if (!c)
		return;
	if (!c->can.move) {
		dx = 0;
		dy = 0;
	}
	if (!c->can.sizeh)
		dw = 0;
	if (!c->can.sizev)
		dh = 0;
	if (!(dx || dy || dw || dh))
		return;
	if (!(m = clientmonitor(c)))
		return;
	if (!isfloating(c, m))
		return;
	if (dw || dh) {
		ClientGeometry g;
		int xr, yr;

		getgeometry(c, &c->c, &g);
		getreference(&xr, &yr, (Geometry *) &g, gravity);

		if (dw && (dw < c->incw))
			dw = (dw / abs(dw)) * c->incw;
		if (dh && (dh < c->inch))
			dh = (dh / abs(dh)) * c->inch;
		g.w += dw;
		g.h += dh;
		if (c->gravity != StaticGravity) {
			DPRINTF("CALLING: constrain()\n");
			constrain(c, &g);
		}
		putreference(xr, yr, (Geometry *) &g, gravity);
		if (g.w != c->c.w || g.h != c->c.h) {
			c->is.max = False;
			c->is.maxv = False;
			c->is.maxh = False;
			c->is.fill = False;
			ewmh_update_net_window_state(c);
		}
		c->r.x = g.x;
		c->r.y = g.y;
		c->r.w = g.w;
		c->r.h = g.h;
	}
	if (dx || dy) {
		c->r.x += dx;
		c->r.y += dy;
	}
	updatefloat(c, NULL);
}

static void
getplace(Client *c, ClientGeometry * g)
{
	long *s = NULL;
	unsigned long n;

	s = getcard(c->win, _XA_WIN_EXPANDED_SIZE, &n);
	if (n >= 4) {
		g->x = s[0];
		g->y = s[1];
		g->w = s[2];
		g->h = s[3];
	} else {
		/* original static geometry */
		g->x = c->s.x;
		g->y = c->s.y;
		g->w = c->s.w;
		g->h = c->s.h;
	}
	g->b = c->c.b;
	g->t = c->th;
	g->g = c->gh;
}

static int
area_overlap(int xmin1, int ymin1, int xmax1, int ymax1,
	     int xmin2, int ymin2, int xmax2, int ymax2)
{
	int w = 0, h = 0;

	w = segm_overlap(xmin1, xmax1, xmin2, xmax2);
	h = segm_overlap(ymin1, ymax1, ymin2, ymax2);

	return (w && h) ? (w * h) : 0;
}

static int
total_overlap(Client *c, Monitor *m, Geometry *g)
{
	Client *o;
	int x1, x2, y1, y2, a;

	x1 = g->x;
	x2 = g->x + g->w + 2 * g->b;
	y1 = g->y;
	y2 = g->y + g->h + 2 * g->b + c->th + c->gh;
	a = 0;

	for (o = scr->clients; o; o = o->next)
		if (o != c && !o->is.bastard && !c->is.dockapp && o->can.floats
		    && isvisible(o, m))
			a += area_overlap(x1, y1, x2, y2, o->r.x, o->r.y,
					  o->r.x + o->r.w + 2 * o->r.b,
					  o->r.y + o->r.h + 2 * o->r.b);
	return a;
}

static Client *
nextplaced(Client *x, Client *c, Monitor *m)
{
	for (; c && (c == x || c->is.bastard || c->is.dockapp || !c->can.floats
		     || !isvisible(c, m)); c = c->snext) ;
	return c;
}

static int
qsort_t2b_l2r(const void *a, const void *b)
{
	Geometry *ga = *(typeof(ga) *) a;
	Geometry *gb = *(typeof(ga) *) b;
	int ret;

	if (!(ret = ga->y - gb->y))
		ret = ga->x - gb->x;
	return ret;
}

static int
qsort_l2r_t2b(const void *a, const void *b)
{
	Geometry *ga = *(typeof(ga) *) a;
	Geometry *gb = *(typeof(ga) *) b;
	int ret;

	if (!(ret = ga->x - gb->x))
		ret = ga->y - gb->y;
	return ret;
}

static int
qsort_cascade(const void *a, const void *b)
{
	Geometry *ga = *(typeof(ga) *) a;
	Geometry *gb = *(typeof(ga) *) b;
	int ret;

	if ((ret = ga->x - gb->x) < 0 || (ret = ga->y - gb->y) < 0)
		return -1;
	return 1;
}

static Geometry *
place_geom(Client *c)
{
	if (c->is.max || c->is.maxv || c->is.maxh || c->is.fill || c->is.full)
		return &c->c;
	return &c->r;
}

static void
place_smart(Client *c, WindowPlacement p, ClientGeometry * g, Monitor *m, Workarea *w)
{
	Client *s;
	Geometry **stack = NULL, **unobs, *e, *o;
	unsigned int num, i, j;

	/* XXX: this algorithm is smarter than the old place_smart: it first determines
	   the top layer of windows by determining which windows are not obscured by any
	   other.  Then, for cascade placement it simply cascades over the northwest
	   unobscured window (if possible) and starts a new cascade in the northwest
	   otherwise.  For column placement, it attempts to place the window to the right 
	   of unobscured windows (if possible) and then works its way right and then down 
	   and right again.  For row placement, it attempts to place windows below
	   unobscured windows (if possible) and then works its way right and then down
	   again.  If either row or column fail it reverts to cascade. */

	/* find all placeable windows in stacking order */
	for (num = 0, s = nextplaced(c, scr->stack, m); s;
	     num++, s = nextplaced(c, s->snext, m)) {
		stack = erealloc(stack, (num + 1) * sizeof(*stack));
		stack[num] = place_geom(s);
	}
	DPRINTF("There are %d stacked windows\n", num);

	unobs = ecalloc(num, sizeof(*unobs));
	memcpy(unobs, stack, num * sizeof(*unobs));

	/* nulls all occluded windows */
	for (i = 0; i < num; i++)
		if ((e = stack[i]))
			for (j = i + 1; j < num; j++)
				if ((o = unobs[j]) && place_overlap(e, o))
					unobs[j] = NULL;

	/* collect non-nulls to the start */
	for (i = 0, j = 0; i < num; i++) {
		if (!(e = unobs[i]))
			break;
		stack[j++] = e;
	}

	free(unobs);
	unobs = NULL;

	DPRINTF("There are %d unoccluded windows\n", j);

	assert(j > 0 || num == 0);	/* first window always unoccluded */
	num = j;

	g->x = w->x;
	g->y = w->y;

	/* if northwest placement works, go with it */
	for (i = 0; i < num && !place_overlap((Geometry *) g, stack[i]); i++) ;
	if (i == num) {
		free(stack);
		return;
	}

	switch (p) {
	case RowSmartPlacement:
		/* sort top to bottom, left to right */
		DPRINTF("sorting top to bottom, left to right\n");
		qsort(stack, num, sizeof(*stack), &qsort_t2b_l2r);
		break;
	case ColSmartPlacement:
		/* sort left to right, top to bottom */
		DPRINTF("sorting left to right, top to bottom\n");
		qsort(stack, num, sizeof(*stack), &qsort_l2r_t2b);
		break;
	case CascadePlacement:
	default:
		DPRINTF("sorting top left to bottom right\n");
		qsort(stack, num, sizeof(*stack), &qsort_cascade);
		break;
	}
	DPRINTF("Sort result:\n");
	for (i = 0; i < num; i++) {
		e = stack[i];
		DPRINTF("%d: %dx%d+%d+%d\n", i, e->w, e->h, e->x, e->y);
	}

	switch (p) {
	case RowSmartPlacement:
		DPRINTF("trying RowSmartPlacement\n");
		/* try below, right each window */
		/* below */
		for (i = 0; i < num; i++) {
			e = stack[i];
			DPRINTF("below: %dx%d+%d+%d this\n", e->w, e->h, e->x, e->y);
			g->x = e->x;
			g->y = e->y + e->h;
			if (g->x < w->x || g->y < w->y || g->x + g->w > w->x + w->w
			    || g->y + g->h > w->y + w->h) {
				DPRINTF("below: %dx%d+%d+%d outside\n", g->w, g->h, g->x,
					g->y);
				continue;
			}
			for (j = 0; j < num && !place_overlap((Geometry *) g, stack[j]);
			     j++) ;
			if (j == num) {
				DPRINTF("below: %dx%d+%d+%d good\n", g->w, g->h, g->x,
					g->y);
				free(stack);
				return;
			}
			DPRINTF("below: %dx%d+%d+%d no good\n", g->w, g->h, g->x, g->y);
		}
		/* right */
		for (i = 0; i < num; i++) {
			e = stack[i];
			DPRINTF("right: %dx%d+%d+%d this\n", e->w, e->h, e->x, e->y);
			g->x = e->x + e->w;
			g->y = e->y;
			if (g->x < w->x || g->y < w->y || g->x + g->w > w->x + w->w
			    || g->y + g->h > w->y + w->h) {
				DPRINTF("right: %dx%d+%d+%d outside\n", g->w, g->h, g->x,
					g->y);
				continue;
			}
			for (j = 0; j < num && !place_overlap((Geometry *) g, stack[j]);
			     j++) ;
			if (j == num) {
				DPRINTF("right: %dx%d+%d+%d good\n", g->w, g->h, g->x,
					g->y);
				free(stack);
				return;
			}
			DPRINTF("right: %dx%d+%d+%d no good\n", g->w, g->h, g->x, g->y);
		}
		DPRINTF("defaulting to CascadePlacement\n");
		break;
	case ColSmartPlacement:
		DPRINTF("trying ColSmartPlacement\n");
		/* try right, below each window */
		/* right */
		for (i = 0; i < num; i++) {
			e = stack[i];
			DPRINTF("right: %dx%d+%d+%d this\n", e->w, e->h, e->x, e->y);
			g->x = e->x + e->w;
			g->y = e->y;
			if (g->x < w->x || g->y < w->y || g->x + g->w > w->x + w->w
			    || g->y + g->h > w->y + w->h) {
				DPRINTF("right: %dx%d+%d+%d outside\n", g->w, g->h, g->x,
					g->y);
				continue;
			}
			for (j = 0; j < num && !place_overlap((Geometry *) g, stack[j]);
			     j++) ;
			if (j == num) {
				DPRINTF("right: %dx%d+%d+%d good\n", g->w, g->h, g->x,
					g->y);
				free(stack);
				return;
			}
			DPRINTF("right: %dx%d+%d+%d no good\n", g->w, g->h, g->x, g->y);
		}
		/* below */
		for (i = 0; i < num; i++) {
			e = stack[i];
			DPRINTF("below: %dx%d+%d+%d this\n", e->w, e->h, e->x, e->y);
			g->x = e->x;
			g->y = e->y + e->h;
			if (g->x < w->x || g->y < w->y || g->x + g->w > w->x + w->w
			    || g->y + g->h > w->y + w->h) {
				DPRINTF("below: %dx%d+%d+%d outside\n", g->w, g->h, g->x,
					g->y);
				continue;
			}
			for (j = 0; j < num && !place_overlap((Geometry *) g, stack[j]);
			     j++) ;
			if (j == num) {
				DPRINTF("below: %dx%d+%d+%d good\n", g->w, g->h, g->x,
					g->y);
				free(stack);
				return;
			}
			DPRINTF("below: %dx%d+%d+%d no good\n", g->w, g->h, g->x, g->y);
		}
		DPRINTF("defaulting to CascadePlacement\n");
		break;
	default:
		DPRINTF("trying CascadePlacement\n");
		break;
	}

	/* default to cascade placement */
	e = stack[0];
	g->x = e->x + scr->style.titleheight;
	g->y = e->y + scr->style.titleheight;

	/* keep window on screen if possible */
	if (g->y + g->h > w->y + w->h && g->y > w->y)
		g->y = w->y;
	if (g->x + g->w > w->x + w->w && g->x > w->x)
		g->x = w->x;

	free(stack);
	return;
}

/* unused */
void
old_place_smart(Client *c, WindowPlacement p, ClientGeometry * g, Monitor *m, Workarea *w)
{
	int t_b, l_r, xl, xr, yt, yb, x_beg, x_end, xd, y_beg, y_end, yd;
	int best_x, best_y, best_a;
	int d = scr->style.titleheight;

	/* XXX: this algorithm (borrowed from fluxbox) calculates an insane number of
	   calculations: it basically tests every possible fully on screen position
	   against every other window. Simulated anealing would work better, but, like a
	   bubble sort, it is simple. */

	t_b = 1;		/* top to bottom or bottom to top */
	l_r = 1;		/* left to right or right to left */

	xl = w->x;
	xr = w->x + w->w - (g->w + 2 * g->b);
	yt = w->y;
	yb = w->y + w->h - (g->h + 2 * g->b + g->t + g->g);
	DPRINTF("boundaries: xl %d xr %x yt %d yb %d\n", xl, xr, yt, yb);

	if (l_r) {
		x_beg = xl;
		x_end = xr;
		if (p == CascadePlacement) {
			x_end -= x_end % d;
			xd = d;
		} else
			xd = 1;
	} else {
		x_beg = xr;
		x_end = xl;
		if (p == CascadePlacement) {
			x_end += x_end % d;
			xd = -d;
		} else
			xd = -1;
	}
	DPRINTF("x range: x_beg %d x_end %d xd %d\n", x_beg, x_end, xd);

	if (t_b) {
		y_beg = yt;
		y_end = yb;
		if (p == CascadePlacement) {
			y_end -= y_end % d;
			yd = d;
		} else
			yd = 1;
	} else {
		y_beg = yb;
		y_end = yt;
		if (p == CascadePlacement) {
			y_end += y_end % d;
			yd = -d;
		} else
			yd = -1;
	}
	DPRINTF("y range: y_beg %d y_end %d yd %d\n", y_beg, y_end, yd);

	best_x = best_y = 0;
	best_a = INT_MAX;

	if (p == ColSmartPlacement) {
		for (g->x = x_beg; l_r ? (g->x < x_end) : (g->x > x_end); g->x += xd) {
			for (g->y = y_beg; t_b ? (g->y < y_end) : (g->y > y_end);
			     g->y += yd) {
				int a;

				if ((a = total_overlap(c, m, (Geometry *) g)) == 0)
					return;
				if (a < best_a) {
					best_x = g->x;
					best_y = g->y;
					best_a = a;
				}
			}
		}
	} else {
		for (g->y = y_beg; t_b ? (g->y < y_end) : (g->y > y_end); g->y += yd) {
			for (g->x = x_beg; l_r ? (g->x < x_end) : (g->x > x_end);
			     g->x += xd) {
				int a;

				if ((a = total_overlap(c, m, (Geometry *) g)) == 0)
					return;
				if (a < best_a) {
					best_x = g->x;
					best_y = g->y;
					best_a = a;
				}
			}
		}
	}
	g->x = best_x;
	g->y = best_y;
}

static void
place_minoverlap(Client *c, WindowPlacement p, ClientGeometry * g, Monitor *m,
		 Workarea *w)
{
	/* TODO: write this */
}

static void
place_undermouse(Client *c, WindowPlacement p, ClientGeometry * g, Monitor *m,
		 Workarea *w)
{
	int mx, my;

	/* pick a different monitor than the default */
	getpointer(&g->x, &g->y);
	if (!(m = getmonitor(g->x, g->y))) {
		m = closestmonitor(g->x, g->y);
		g->x = m->mx;
		g->y = m->my;
	}
	getworkarea(m, w);

	putreference(g->x, g->y, (Geometry *) g, c->gravity);

	/* keep center of window inside work area, otherwise wnck task bars figure its on 
	   a different monitor */

	mx = g->x + g->w / 2 + g->b;
	my = g->y + g->h / 2 + g->b;

	if (mx < w->x)
		g->x += w->x - mx;
	if (mx > w->x + w->w)
		g->x -= mx - (w->x + w->w);
	if (my < w->y)
		g->y += w->y - my;
	if (my > w->y + w->h)
		g->y -= my - (w->y + w->h);
}

static void
place_random(Client *c, WindowPlacement p, ClientGeometry * g, Monitor *m, Workarea *w)
{
	int x_min, x_max, y_min, y_max, x_off, y_off;
	int d = scr->style.titleheight;

	x_min = w->x;
	x_max = w->x + w->w - (g->x + g->w + 2 * g->b);
	x_max -= x_max % d;
	y_min = w->y;
	y_max = w->y + w->h - (g->y + g->h + 2 * g->b);
	y_max -= y_max % d;

	x_off = rand() % (x_max - x_min);
	x_off -= x_off % d;
	y_off = rand() % (y_max - y_min);
	y_off -= y_off % d;

	g->x = x_min + x_off;
	g->y = y_min + y_off;
}

static void
place(Client *c, WindowPlacement p)
{
	ClientGeometry g;
	Workarea w;
	Monitor *m;

	getplace(c, &g);

	/* FIXME: use initial location considering gravity */
	if (!(m = clientmonitor(c)))
		if (!(m = curmonitor()))
			m = nearmonitor();
	g.x += m->sc.x;
	g.y += m->sc.y;

	getworkarea(m, &w);

	switch (p) {
	case ColSmartPlacement:
		place_smart(c, p, &g, m, &w);
		break;
	case RowSmartPlacement:
		place_smart(c, p, &g, m, &w);
		break;
	case MinOverlapPlacement:
		place_minoverlap(c, p, &g, m, &w);
		break;
	case UnderMousePlacement:
		place_undermouse(c, p, &g, m, &w);
		break;
	case CascadePlacement:
		place_smart(c, p, &g, m, &w);
		break;
	case RandomPlacement:
		place_random(c, p, &g, m, &w);
		break;
	}
	c->r.x = g.x;
	c->r.y = g.y;
}

void
addclient(Client *c, Bool focusme, Bool raiseme)
{
	CPRINTF(c, "initial geometry c: %dx%d+%d+%d:%d t %d g %d\n",
		c->c.w, c->c.h, c->c.x, c->c.y, c->c.b, c->th, c->gh);
	CPRINTF(c, "initial geometry r: %dx%d+%d+%d:%d t %d g %d\n",
		c->r.w, c->r.h, c->r.x, c->r.y, c->r.b, c->th, c->gh);
	CPRINTF(c, "initial geometry s: %dx%d+%d+%d:%d t %d g %d\n",
		c->s.w, c->s.h, c->s.x, c->s.y, c->s.b, c->th, c->gh);

	if (!c->s.x && !c->s.y && c->can.move && !c->is.dockapp) {
		/* put it on the monitor startup notification requested if not already
		   placed with its group */
		if (c->monitor && !clientmonitor(c))
			c->tags = scr->monitors[c->monitor - 1].curview->seltags;
		place(c, ColSmartPlacement);
	}

	CPRINTF(c, "placed geometry c: %dx%d+%d+%d:%d t %d g %d\n",
		c->c.w, c->c.h, c->c.x, c->c.y, c->c.b, c->th, c->gh);
	CPRINTF(c, "placed geometry r: %dx%d+%d+%d:%d t %d g %d\n",
		c->r.w, c->r.h, c->r.x, c->r.y, c->r.b, c->th, c->gh);
	CPRINTF(c, "placed geometry s: %dx%d+%d+%d:%d t %d g %d\n",
		c->s.w, c->s.h, c->s.x, c->s.y, c->s.b, c->th, c->gh);

	if (!c->can.move) {
		int mx, my;
		Monitor *cm;

		/* wnck task bars figure window is on monitor containing center of window 
		 */
		mx = c->s.x + c->s.w / 2 + c->s.b;
		my = c->s.y + c->s.h / 2 + c->s.b;

		if (!(cm = getmonitor(mx, my)))
			cm = closestmonitor(mx, my);
		c->tags = cm->curview->seltags;
	}

	c->cscr = scr;
	attach(c, scr->options.attachaside);
	attachclist(c);
	attachflist(c, focusme);
	attachstack(c, raiseme);
	ewmh_update_net_client_list();
	if (c->is.managed)
		ewmh_update_net_window_desktop(c);
}

void
delclient(Client *c)
{
	detach(c);
	detachclist(c);
	detachflist(c);
	detachstack(c);
	c->cscr = NULL;
}

void
moveto(Client *c, RelativeDirection position)
{
	Monitor *m;
	Workarea w;
	ClientGeometry g;

	if (!c || !c->can.move || !(m = c->cmon))
		return;
	if (!isfloating(c, m))
		return;		/* for now */

	getworkarea(m, &w);
	getgeometry(c, &c->c, &g);

	switch (position) {
	case RelativeNone:
		if (c->gravity == ForgetGravity)
			return;
		return moveto(c, c->gravity);
	case RelativeNorthWest:
		g.x = w.x;
		g.y = w.y;
		break;
	case RelativeNorth:
		g.x = w.x + w.w / 2 - (g.w + 2 * g.b) / 2;
		g.y = w.y;
		break;
	case RelativeNorthEast:
		g.x = w.x + w.w - (g.w + 2 * g.b);
		g.y = w.y;
		break;
	case RelativeWest:
		g.x = w.x;
		g.y = w.y + w.h / 2 - (g.h + 2 * g.b) / 2;
		break;
	case RelativeCenter:
		g.x = w.x + w.w / 2 - (g.w + 2 * g.b) / 2;
		g.y = w.y + w.h / 2 - (g.h + 2 * g.b) / 2;
		break;
	case RelativeEast:
		g.x = w.x + w.w - (g.w + 2 * g.b);
		g.y = w.y + w.h / 2 - (g.h + 2 * g.b) / 2;
		break;
	case RelativeSouthWest:
		g.x = w.x;
		g.y = w.y + w.h - (g.h + 2 * g.b);
		break;
	case RelativeSouth:
		g.x = w.x + w.w / 2 - (g.w + 2 * g.b) / 2;
		g.y = w.y + w.h - (g.h + 2 * g.b);
		break;
	case RelativeSouthEast:
		g.x = w.x + w.w - (g.w + 2 * g.b);
		g.y = w.y + w.h - (g.h + 2 * g.b);
		break;
	case RelativeStatic:
		g.x = c->s.x;
		g.y = c->s.y;
		break;
	default:
		return;
	}
	reconfigure(c, &g);
	save(c);
	discardenter();
}

void
moveby(Client *c, RelativeDirection direction, int amount)
{
	Monitor *m;
	Workarea w;
	ClientGeometry g;

	if (!c || !c->can.move || !(m = c->cmon))
		return;
	if (!isfloating(c, m))
		return;		/* for now */

	getworkarea(m, &w);
	getgeometry(c, &c->c, &g);
	switch (direction) {
		int dx, dy;

	case RelativeNone:
		if (c->gravity == ForgetGravity)
			return;
		return moveby(c, c->gravity, amount);
	case RelativeNorthWest:
		g.x -= amount;
		g.y -= amount;
		break;
	case RelativeNorth:
		g.y -= amount;
		break;
	case RelativeNorthEast:
		g.x += amount;
		g.y -= amount;
		break;
	case RelativeWest:
		g.x -= amount;
		break;
	case RelativeCenter:
		dx = w.x + w.w / 2 - (g.x + g.w / 2 + g.b);
		dy = w.y + w.h / 2 - (g.y + g.h / 2 + g.b);
		if (amount < 0) {
			/* move away from center */
			dx = dx / abs(dx) * amount;
			dy = dy / abs(dx) * amount;
		} else {
			/* move toward center */
			dx = dx / abs(dx) * min(abs(dx), abs(amount));
			dy = dy / abs(dy) * min(abs(dy), abs(amount));
		}
		g.x -= dx;
		g.y -= dy;
		break;
	case RelativeEast:
		g.x += amount;
		break;
	case RelativeSouthWest:
		g.x -= amount;
		g.y += amount;
		break;
	case RelativeSouth:
		g.y += amount;
		break;
	case RelativeSouthEast:
		g.x += amount;
		g.y += amount;
		break;
	case RelativeStatic:
		dx = c->s.x + c->s.w / 2 - (g.x + g.w / 2 + g.b);
		dy = c->s.y + c->s.h / 2 - (g.y + g.h / 2 + g.b);
		if (amount < 0) {
			/* move away from static */
			dx = dx / abs(dx) * amount;
			dy = dy / abs(dx) * amount;
		} else {
			/* move toward static */
			dx = dx / abs(dx) * min(abs(dx), abs(amount));
			dy = dy / abs(dy) * min(abs(dy), abs(amount));
		}
		g.x -= dx;
		g.y -= dy;
		break;
	default:
		return;
	}
	/* keep onscreen */
	if (g.x >= m->sc.x + m->sc.w)
		g.x = m->sc.x + m->sc.w - 1;
	if (g.x + g.w + g.b < m->sc.x)
		g.x = m->sc.x - (g.w + g.b);
	if (g.y >= m->sc.y + m->sc.h)
		g.y = m->sc.y + m->sc.h - 1;
	if (g.y + g.h + g.b < m->sc.y)
		g.y = m->sc.y - (g.h + g.b);
	reconfigure(c, &g);
	save(c);
	discardenter();
}

void
snapto(Client *c, RelativeDirection direction)
{
	Monitor *m;
	Workarea w;
	ClientGeometry g;
	Client *s, *ox, *oy;
	int min1, max1, edge;

	if (!c || !c->can.move || !(m = c->cmon))
		return;
	if (!isfloating(c, m))
		return;		/* for now */

	getworkarea(m, &w);
	getgeometry(c, &c->c, &g);

	switch (direction) {
	case RelativeNone:
		if (c->gravity == ForgetGravity)
			return;
		return snapto(c, c->gravity);
	case RelativeNorthWest:
		min1 = g.y + g.b;
		max1 = g.y + g.b + g.h;
		edge = g.x;
		for (ox = NULL, s = scr->clients; s; s = s->next) {
			if (s->cmon != m)
				continue;
			if (s->c.x + s->c.b + s->c.w >= edge)
				continue;
			if (segm_overlap(min1, max1,
					 s->c.y + s->c.b, s->c.y + s->c.b + s->c.h))
				if (!ox || ox->c.x + ox->c.w + 2 * ox->c.b >
				    s->c.x + s->c.w + 2 * s->c.b)
					ox = s;
		}
		min1 = g.x + g.b;
		max1 = g.x + g.b + g.w;
		edge = g.y;
		for (oy = NULL, s = scr->clients; s; s = s->next) {
			if (s->cmon != m)
				continue;
			if (s->c.y + s->c.b + s->c.h >= edge)
				continue;
			if (segm_overlap(min1, max1,
					 s->c.x + s->c.b, s->c.x + s->c.b + s->c.w))
				if (!oy || oy->c.y + oy->c.h + 2 * oy->c.b >
				    s->c.y + s->c.h + 2 * s->c.b)
					oy = s;
		}
		g.x = ox ? ox->c.x + ox->c.b + ox->c.w : w.x;
		g.y = oy ? oy->c.y + oy->c.b + oy->c.h : w.y;
		break;
	case RelativeNorth:
		min1 = g.x + g.b;
		max1 = g.x + g.b + g.w;
		edge = g.y;
		for (oy = NULL, s = scr->clients; s; s = s->next) {
			if (s->cmon != m)
				continue;
			if (s->c.y + s->c.b + s->c.h >= edge)
				continue;
			if (segm_overlap(min1, max1,
					 s->c.x + s->c.b, s->c.x + s->c.b + s->c.w))
				if (!oy || oy->c.y + oy->c.h + 2 * oy->c.b >
				    s->c.y + s->c.h + 2 * s->c.b)
					oy = s;
		}
		g.y = oy ? oy->c.y + oy->c.b + oy->c.h : w.y;
		break;
	case RelativeNorthEast:
		min1 = g.y + g.b;
		max1 = g.y + g.b + g.h;
		edge = g.x + g.b + g.w;
		for (ox = NULL, s = scr->clients; s; s = s->next) {
			if (s->cmon != m)
				continue;
			if (s->c.x < edge)
				continue;
			if (segm_overlap(min1, max1,
					 s->c.y + s->c.b, s->c.y + s->c.b + s->c.h))
				if (!ox || ox->c.x >= s->c.x)
					ox = s;
		}
		min1 = g.x + g.b;
		max1 = g.x + g.b + g.w;
		edge = g.y;
		for (oy = NULL, s = scr->clients; s; s = s->next) {
			if (s->cmon != m)
				continue;
			if (s->c.y + s->c.b + s->c.h >= edge)
				continue;
			if (segm_overlap(min1, max1,
					 s->c.x + s->c.b, s->c.x + s->c.b + s->c.w))
				if (!oy || oy->c.y + oy->c.h + 2 * oy->c.b >
				    s->c.y + s->c.h + 2 * s->c.b)
					oy = s;
		}
		g.x = (ox ? ox->c.x : w.x + w.w - g.b) - (g.w + g.b);
		g.y = oy ? oy->c.y + oy->c.b + oy->c.h : w.y;
		break;
	case RelativeWest:
		min1 = g.y + g.b;
		max1 = g.y + g.b + g.h;
		edge = g.x;
		for (ox = NULL, s = scr->clients; s; s = s->next) {
			if (s->cmon != m)
				continue;
			if (s->c.x + s->c.b + s->c.w >= edge)
				continue;
			if (segm_overlap(min1, max1,
					 s->c.y + s->c.b, s->c.y + s->c.b + s->c.h))
				if (!ox || ox->c.x + ox->c.w + 2 * ox->c.b >
				    s->c.x + s->c.w + 2 * s->c.b)
					ox = s;
		}
		g.x = ox ? ox->c.x + ox->c.b + ox->c.w : w.x;
		break;
	case RelativeCenter:
		return;
	case RelativeEast:
		min1 = g.y + g.b;
		max1 = g.y + g.b + g.h;
		edge = g.x + g.b + g.w;
		for (ox = NULL, s = scr->clients; s; s = s->next) {
			if (s->cmon != m)
				continue;
			if (s->c.x < edge)
				continue;
			if (segm_overlap(min1, max1,
					 s->c.y + s->c.b, s->c.y + s->c.b + s->c.h))
				if (!ox || ox->c.x >= s->c.x)
					ox = s;
		}
		g.x = (ox ? ox->c.x : w.x + w.w - g.b) - (g.w + g.b);
		break;
	case RelativeSouthWest:
		min1 = g.y + g.b;
		max1 = g.y + g.b + g.h;
		edge = g.x;
		for (ox = NULL, s = scr->clients; s; s = s->next) {
			if (s->cmon != m)
				continue;
			if (s->c.x + s->c.b + s->c.w >= edge)
				continue;
			if (segm_overlap(min1, max1,
					 s->c.y + s->c.b, s->c.y + s->c.b + s->c.h))
				if (!ox || ox->c.x + ox->c.w + 2 * ox->c.b >
				    s->c.x + s->c.w + 2 * s->c.b)
					ox = s;
		}
		min1 = g.x + g.b;
		max1 = g.x + g.b + g.w;
		edge = g.y + g.b + g.h;
		for (oy = NULL, s = scr->clients; s; s = s->next) {
			if (s->cmon != m)
				continue;
			if (s->c.y < edge)
				continue;
			if (segm_overlap(min1, max1,
					 s->c.x + s->c.b, s->c.x + s->c.b + s->c.w))
				if (!oy || oy->c.y >= s->c.y)
					oy = s;
		}
		g.x = ox ? ox->c.x + ox->c.b + ox->c.w : w.x;
		g.y = (oy ? oy->c.y : w.y + w.h - g.b) - (g.h + g.b);
		break;
	case RelativeSouth:
		min1 = g.x + g.b;
		max1 = g.x + g.b + g.w;
		edge = g.y + g.b + g.h;
		for (oy = NULL, s = scr->clients; s; s = s->next) {
			if (s->cmon != m)
				continue;
			if (s->c.y < edge)
				continue;
			if (segm_overlap(min1, max1,
					 s->c.x + s->c.b, s->c.x + s->c.b + s->c.w))
				if (!oy || oy->c.y >= s->c.y)
					oy = s;
		}
		g.y = (oy ? oy->c.y : w.y + w.h - g.b) - (g.h + g.b);
		break;
	case RelativeSouthEast:
		min1 = g.y + g.b;
		max1 = g.y + g.b + g.h;
		edge = g.x + g.b + g.w;
		for (ox = NULL, s = scr->clients; s; s = s->next) {
			if (s->cmon != m)
				continue;
			if (s->c.x < edge)
				continue;
			if (segm_overlap(min1, max1,
					 s->c.y + s->c.b, s->c.y + s->c.b + s->c.h))
				if (!ox || ox->c.x >= s->c.x)
					ox = s;
		}
		min1 = g.x + g.b;
		max1 = g.x + g.b + g.w;
		edge = g.y + g.b + g.h;
		for (oy = NULL, s = scr->clients; s; s = s->next) {
			if (s->cmon != m)
				continue;
			if (s->c.y < edge)
				continue;
			if (segm_overlap(min1, max1,
					 s->c.x + s->c.b, s->c.x + s->c.b + s->c.w))
				if (!oy || oy->c.y >= s->c.y)
					oy = s;
		}
		g.x = (ox ? ox->c.x : w.x + w.w - g.b) - (g.w + g.b);
		g.y = (oy ? oy->c.y : w.y + w.h - g.b) - (g.h + g.b);
		break;
	case RelativeStatic:
		return;
	default:
		return;
	}
	reconfigure(c, &g);
	save(c);
	discardenter();
}

void
edgeto(Client *c, int direction)
{
	Monitor *m;
	Workarea w;
	ClientGeometry g;

	if (!c || !c->can.move || !(m = c->cmon))
		return;
	if (!isfloating(c, m))
		return;		/* for now */

	getworkarea(m, &w);
	getgeometry(c, &c->c, &g);

	switch (direction) {
	case RelativeNone:
		if (c->gravity == ForgetGravity)
			return;
		return edgeto(c, c->gravity);
	case RelativeNorthWest:
		g.x = w.x;
		g.y = w.y;
		break;
	case RelativeNorth:
		g.y = w.y;
		break;
	case RelativeNorthEast:
		g.x = w.x + w.w - (g.w + 2 * g.b);
		g.y = w.y;
		break;
	case RelativeWest:
		g.x = w.x;
		break;
	case RelativeCenter:
		g.x = w.x + w.w / 2 - (g.w / 2 + g.b);
		g.y = w.y + w.h / 2 - (g.h / 2 + g.b);
		break;
	case RelativeEast:
		g.x = w.x + w.w - (g.w + 2 * g.b);
		break;
	case RelativeSouthWest:
		g.x = w.x;
		g.y = w.y + w.h - (g.h + 2 * g.b);
		break;
	case RelativeSouth:
		g.y = w.y + w.h - (g.h + 2 * g.b);
		break;
	case RelativeSouthEast:
		g.x = w.x + w.w - (g.w + 2 * g.b);
		g.y = w.y + w.h - (g.h + 2 * g.b);
		break;
	case RelativeStatic:
		g.x = c->s.x;
		g.y = c->s.y;
		break;
	default:
		return;
	}
	reconfigure(c, &g);
	save(c);
	discardenter();
}

void
rotateview(Client *c)
{
	Monitor *m;
	View *v;

	if (!c || !(m = c->cmon))
		return;
	v = m->curview;
	if (!FEATURES(v->layout, ROTL))
		return;
	v->major = (v->major + 1) % OrientLast;
	v->minor = (v->minor + 1) % OrientLast;
	arrange(m);
}

void
unrotateview(Client *c)
{
	Monitor *m;
	View *v;

	if (!c || !(m = c->cmon))
		return;
	v = m->curview;
	if (!FEATURES(v->layout, ROTL))
		return;
	v->major = (v->major + OrientLast - 1) % OrientLast;
	v->minor = (v->minor + OrientLast - 1) % OrientLast;
	arrange(m);
}

void
rotatezone(Client *c)
{
	Monitor *m;
	View *v;

	if (!c || !(m = c->cmon))
		return;
	v = m->curview;
	if (!FEATURES(v->layout, ROTL) || !FEATURES(v->layout, NMASTER))
		return;
	v->minor = (v->minor + 1) % OrientLast;
	arrange(m);
}

void
unrotatezone(Client *c)
{
	Monitor *m;
	View *v;

	if (!c || !(m = c->cmon))
		return;
	v = m->curview;
	if (!FEATURES(v->layout, ROTL) || !FEATURES(v->layout, NMASTER))
		return;
	v->minor = (v->minor + OrientLast - 1) % OrientLast;
	arrange(m);
}

void
rotatewins(Client *c)
{
	Monitor *m;
	View *v;
	Client *s;

	if (!c || !(m = c->cmon))
		return;
	v = m->curview;
	if (!FEATURES(v->layout, ROTL))
		return;
	if ((s = nexttiled(scr->clients, m))) {
		detach(s);
		attach(s, True);
		arrange(m);
		focus(s);
	}
}

void
unrotatewins(Client *c)
{
	Monitor *m;
	View *v;
	Client *last, *s;

	if (!c || !(m = c->cmon))
		return;
	v = m->curview;
	if (!FEATURES(v->layout, ROTL) || !FEATURES(v->layout, NMASTER))
		return;
	for (last = scr->clients; last && last->next; last = last->next) ;
	if ((s = prevtiled(last, m))) {
		detach(s);
		attach(s, False);
		arrange(m);
		focus(s);
	}
}

void
togglefloating(Client *c)
{
	Monitor *m;

	if (!c || c->is.floater || !c->can.floats || !(m = c->cmon))
		return;
	c->skip.arrange = !c->skip.arrange;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, m);
		arrange(m);
	}
}

void
togglefill(Client *c)
{
	Monitor *m;

	if (!c || (!c->can.fill && c->is.managed) || !(m = c->cmon))
		return;
	c->is.fill = !c->is.fill;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, m);
	}

}

void
togglefull(Client *c)
{
	Monitor *m;

	if (!c || (!c->can.full && c->is.managed) || !(m = c->cmon))
		return;
	if (!c->is.full) {
		c->wasfloating = c->skip.arrange;
		if (!c->skip.arrange)
			c->skip.arrange = True;
	} else {
		if (!c->wasfloating && c->skip.arrange)
			c->skip.arrange = False;
	}
	c->is.full = !c->is.full;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, m);
		restack();
	}
}

void
togglemax(Client *c)
{
	Monitor *m;

	if (!c || (!c->can.max && c->is.managed) || !(m = c->cmon))
		return;
	c->is.max = !c->is.max;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, m);
		restack();
	}
}

void
togglemaxv(Client *c)
{
	Monitor *m;

	if (!c || (!c->can.maxv && c->is.managed) || !(m = c->cmon))
		return;
	c->is.maxv = !c->is.maxv;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, m);
	}
}

void
togglemaxh(Client *c)
{
	Monitor *m;

	if (!c || (!c->can.maxh && c->is.managed) || !(m = c->cmon))
		return;
	c->is.maxh = !c->is.maxh;
	ewmh_update_net_window_state(c);
	updatefloat(c, m);
}

void
toggleshade(Client *c)
{
	Monitor *m;

	if (!c || (!c->can.shade && c->is.managed) || !(m = c->cmon))
		return;
	c->was.shaded = c->is.shaded;
	c->is.shaded = !c->is.shaded;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, m);
	}
}

void
toggledectiled(Monitor *m, View *v)
{
	v = m->curview;
	v->dectiled = v->dectiled ? False : True;
	if (m)
		arrange(m);
}

void
zoom(Client *c)
{
	Monitor *m;

	if (!c)
		return;
	if (!(m = c->cmon))
		return;
	if (!MFEATURES(m, ZOOM) || c->skip.arrange)
		return;
	if (c == nexttiled(scr->clients, m))
		if (!(c = nexttiled(c->next, m)))
			return;
	detach(c);
	attach(c, False);
	arrange(m);
	focus(c);
}

void
zoomfloat(Client *c)
{
	Monitor *m;

	if (!(m = c->cmon))
		return;
	if (isfloating(c, m))
		togglefloating(c);
	else
		zoom(c);
}

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
