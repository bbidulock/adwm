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
#include "layout.h"		/* verification */

#define MWFACT		(1<<0)	/* adjust master factor */
#define NMASTER		(1<<1)	/* adjust number of masters */
#define	ZOOM		(1<<2)	/* adjust zoom */
#define	OVERLAP		(1<<3)	/* floating layout */
#define NCOLUMNS	(1<<4)	/* adjust number of columns */
#define ROTL		(1<<5)	/* adjust rotation */
#define MMOVE		(1<<6)	/* shuffle tile position with mouse */

#define FEATURES(_layout, _which) (!(!((_layout)->features & (_which))))
#define VFEATURES(_view, _which) FEATURES(((_view)->layout),(_which))

/*
 * This file contains layout-specific functions that must ultimately move into layout modules.
 */


/*
 * Some basic tree manipulation operations.
 */

void
setfocused(Client *c)
{
	Leaf *l;
	Container *cp, *cc;

	if (c) {
		CPRINTF(c, "setting as focused\n");
		for (l = c->leaves; l; l = l->client.next)
			for (cc = (Container *)l; (cp = cc->parent); cc = cp)
				cp->node.children.focused = cc;
	}
}

void
setselected(Client *c)
{
	Leaf *l;
	Container *cp, *cc;

	if (c) {
		CPRINTF(c, "setting as selected\n");
		for (l = c->leaves; l; l = l->client.next)
			for (cc = (Container *)l; (cp = cc->parent); cc = cp)
				cp->node.children.selected = cc;
	}
}

void
delleaf(Leaf *l, Bool active)
{
	Term *t;
	Container *cp;

	if (!l) {
		DPRINTF("ERROR: no leaf\n");
		assert(l != NULL);
	}
	if (l->type != TreeTypeLeaf) {
		DPRINTF("ERROR: not a leaf\n");
		assert(l->type == TreeTypeLeaf);
	}
	if (!(t = l->parent)) {
		DPRINTF("WARNING: attempting to delete detached leaf\n");
		return;
	}
	l->parent = NULL;
	if ((t->children.head == l))
		t->children.head = l->next;
	if ((t->children.tail == l))
		t->children.tail = l->prev;
	if (l->prev)
		l->prev->next = l->next;
	if (l->next)
		l->next->prev = l->prev;
	l->prev = l->next = NULL;
	if (t->children.focused == l)
		t->children.focused = NULL;
	if (t->children.selected == l)
		t->children.selected = NULL;
	t->children.number--;

	if (active)
		for (cp = (Container *)t; cp; cp = cp->parent)
			cp->node.children.active -= 1;
}

void
appleaf(Container *cp, Leaf *l, Bool active)
{
	Leaf *after = NULL;

	/* append leaf l into bottom container of cp */
	/* when cp is a leaf, append after cp */
	if (l->type != TreeTypeLeaf) {
		DPRINTF("ERROR: l is not a leaf\n");
		assert(l->type == TreeTypeLeaf);
	}
	if (l->parent) {
		DPRINTF("WARNING: attempting to append attached leaf\n");
		delleaf(l, active);
	}
	if (cp && cp->type == TreeTypeLeaf) {
		after = &cp->leaf;
		cp = cp->parent;
	} else {
		while ((cp && cp->node.children.tail &&
			cp->node.children.tail->type != TreeTypeLeaf))
			cp = cp->node.children.tail;
	}
	if (!cp) {
		DPRINTF("ERROR: no parent node\n");
		assert(cp != NULL);
	}
	l->next = l->prev = NULL; /* safety */
	l->parent = &cp->term;
	if (!after)
		after = cp->term.children.tail;
	if (cp->term.children.tail == after)
		cp->term.children.tail = l;
	if ((l->next = (after ? after->next : NULL)))
		l->next->prev = l;
	if ((l->prev = after))
		l->prev->next = l;
	else
		cp->term.children.head = l;
	cp->term.children.number++;
	if (active)
		for (; cp; cp = cp->parent)
			cp->node.children.active += 1;
	if (l->client.client) {
		if (l->client.client == sel)
			setselected(sel);
		if (l->client.client == took)
			setfocused(took);
	}
}

void
insleaf(Container *cp, Leaf *l, Bool active)
{
	Leaf *before = NULL;

	/* insert leaf l into bottom container of cp */
	/* when cp is a leaf, insert before cp */
	if (l->type != TreeTypeLeaf) {
		DPRINTF("ERROR: l is not a leaf\n");
		assert(l->type == TreeTypeLeaf);
	}
	if (l->parent) {
		DPRINTF("WARNING: attempting to insert attached leaf\n");
		delleaf(l, active);
	}
	if (cp && cp->type == TreeTypeLeaf) {
		before = &cp->leaf;
		cp = cp->parent;
	} else {
		while ((cp && cp->node.children.head &&
			cp->node.children.head->type != TreeTypeLeaf))
			cp = cp->node.children.head;
	}
	if (!cp) {
		DPRINTF("ERROR: no parent node\n");
		assert(cp != NULL);
	}
	l->next = l->prev = NULL;	/* safety */
	l->parent = &cp->term;
	if (!before)
		before = cp->term.children.head;
	if (cp->term.children.head == before)
		cp->term.children.head = l;
	if ((l->prev = (before ? before->prev : NULL)))
		l->prev->next = l;
	if ((l->next = before))
		l->next->prev = l;
	else
		cp->term.children.tail = l;
	cp->term.children.number++;
	if (active)
		for (; cp; cp = cp->parent)
			cp->node.children.active += 1;
	if (l->client.client) {
		if (l->client.client == sel)
			setselected(sel);
		if (l->client.client == took)
			setfocused(took);
	}
}

void
delnode(Container *cc)
{
	Container *cp;

	if (cc->type != TreeTypeNode) {
		DPRINTF("ERROR: attempting to delete non-node\n");
		assert(cc->type == TreeTypeNode);
	}
	if (!(cp = cc->parent)) {
		DPRINTF("WARNING: attempting to delete detached node\n");
		return;
	}
	cc->parent = NULL;
	if ((cp->node.children.head == cc))
		cp->node.children.head = cc->next;
	if ((cp->node.children.tail == cc))
		cp->node.children.tail = cc->prev;
	if (cc->prev)
		cc->prev->next = cc->next;
	if (cc->next)
		cc->next->prev = cc->prev;
	cc->prev = cc->next = NULL;
	if (cp->node.children.focused == cc)
		cp->node.children.focused = NULL;
	if (cp->node.children.selected == cc)
		cp->node.children.selected = NULL;
	cp->node.children.number--;

	for (;cp;cp = cp->parent)
		cp->node.children.active -= cc->node.children.active;
}

void
appnode(Container *cp, Container *cc)
{
	/* append Container cc into container cp, cc must be extracted from tree */
	if (cc->parent) {
		DPRINTF("WARNING: attempting to append attached node\n");
		delnode(cc);
	}
#if 0
	if (cp->type == TreeTypeLeaf)
		cp = cp->parent;
	/* find bottom of tree */
	while ((cp && cp->node.children.tail &&
		cp->node.children.tail->type != TreeTypeLeaf))
		cp = cp->node.children.tail;
	if (!cp) {
		DPRINTF("ERROR: no parent node\n");
		assert(cp != NULL);
	}
#endif
	cc->next = cc->prev = NULL;	/* safety */
	cc->parent = cp;
	if ((cc->prev = cp->node.children.tail))
		cc->prev->next = cc;
	else
		cp->node.children.head = cc;
	cp->node.children.tail = cc;
	cp->node.children.number++;

	for (; cp; cp = cp->parent)
		cp->node.children.active += cc->node.children.active;
}

void
insnode(Container *cp, Container *cc)
{
	/* insert Container cc into container cp, cc must be extracted from tree */
	if (cc->parent) {
		DPRINTF("WARNING: attempting to append attached node\n");
		delnode(cc);
	}
#if 0
	if (cp->type == TreeTypeLeaf)
		cp = cp->parent;
	/* find bottom of tree */
	while ((cp && cp->node.children.head &&
		cp->node.children.head->type != TreeTypeLeaf))
		cp = cp->node.children.tail;
	if (!cp) {
		DPRINTF("ERROR: no parent node\n");
		assert(cp != NULL);
	}
#endif
	cc->next = cc->prev = NULL;	/* safety */
	cc->parent = cp;
	if ((cc->next = cp->node.children.head))
		cc->next->prev = cc;
	else
		cp->node.children.tail = cc;
	cp->node.children.head = cc;
	cp->node.children.number++;

	for (; cp; cp = cp->parent)
		cp->node.children.active += cc->node.children.active;
}

#ifdef DEBUG
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
#endif

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

static void
reattach(Client *c, Bool attachside)
{
	detach(c);
	attach(c, attachside);
}

void
reattachclist(Client *c)
{
	detachclist(c);
	attachclist(c);
}

static void
reattachflist(Client *c, Bool front)
{
	detachflist(c);
	attachflist(c, front);
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
		reattachflist(next, True);
	}
	gave = next;
	setfocused(took);
}

static Bool
isfloating(Client *c, View *v)
{
	if ((c->is.floater && !c->is.dockapp) || c->skip.arrange)
		return True;
	if (c->is.full)
		return True;
	if (v && VFEATURES(v, OVERLAP))
		return True;
	return False;
}

Bool
enterclient(XEvent *e, Client *c)
{
	if (!c || !canfocus(c)) {
		CPRINTF(c, "FOCUS: cannot focus client.\n");
		return True;
	}
	/* focus when switching monitors */
	if (!isvisible(sel, c->cview)) {
		CPRINTF(c, "FOCUS: monitor switching focus\n");
		focus(c);
	}
	switch (scr->options.focus) {
	case Clk2Focus:
		break;
	case SloppyFloat:
		/* FIXME: incorporate isfloating() check into skip.sloppy setting */
		if (!c->skip.sloppy && isfloating(c, c->cview)) {
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
reconfigure_dockapp(Client *c, ClientGeometry *n, Bool force)
{
	XWindowChanges wwc, fwc;
	unsigned wmask, fmask;

	GPRINTF(&c->r, "initial c->r geometry\n");
	GPRINTF(&c->c, "initial c->c geometry\n");
	GPRINTF(n,     "initial n    geometry\n");
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
	GPRINTF(&c->r, "final   c->r geometry\n");
	GPRINTF(&c->c, "final   c->c geometry\n");
	GPRINTF(n,     "final   n    geometry\n");
	if (fmask) {
		DPRINTF("frame wc = %ux%u+%d+%d:%d\n", fwc.width, fwc.height, fwc.x,
			fwc.y, fwc.border_width);
		XConfigureWindow(dpy, c->frame, fmask, &fwc);
	}
	if (wmask) {
		DPRINTF("wind  wc = %ux%u+%d+%d:%d\n", wwc.width, wwc.height, wwc.x,
			wwc.y, wwc.border_width);
		XConfigureWindow(dpy, c->icon, wmask, &wwc);
	}
	if (force || ((fmask | wmask) && !(wmask & (CWWidth | CWHeight)))) {
		XConfigureEvent ce;

		ce.type = ConfigureNotify;
		ce.display = dpy;
		ce.event = c->icon;
		ce.window = c->icon;
		ce.x = c->c.x + c->c.b + c->r.x;
		ce.y = c->c.y + c->c.b + c->r.y;
		ce.width = c->r.w;
		ce.height = c->r.h;
		ce.border_width = c->r.b;
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

static Bool
check_unmapnotify(Display *dpy, XEvent *ev, XPointer arg)
{
	Client *c = (typeof(c)) arg;

	return (ev->type == UnmapNotify && !ev->xunmap.send_event
		&& ev->xunmap.window == c->win && ev->xunmap.event == c->frame);
}

static void
reconfigure(Client *c, ClientGeometry *n, Bool force)
{
	XWindowChanges wwc, fwc;
	unsigned wmask, fmask;
	Bool tchange = False, gchange = False, hchange = False, shaded = False;

	if (n->w <= 0 || n->h <= 0) {
		CPRINTF(c, "zero width %d or height %d\n", n->w, n->h);
		return;
	}
	/* offscreen appearance fixes */
	if (n->x > DisplayWidth(dpy, scr->screen))
		n->x = DisplayWidth(dpy, scr->screen) - n->w - 2 * n->b;
	if (n->y > DisplayHeight(dpy, scr->screen))
		n->y = DisplayHeight(dpy, scr->screen) - n->h - 2 * n->b;
	DPRINTF("x = %d y = %d w = %d h = %d b = %d t = %d g = %d v = %d\n", n->x, n->y,
		n->w, n->h, n->b, n->t, n->g, n->v);

	if (c->is.dockapp)
		return reconfigure_dockapp(c, n, force);

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
	if (c->c.w - 2 * c->c.v != (wwc.width = n->w - 2 * n->v)) {
		DPRINTF("wind  wc.w = %u\n", wwc.width);
		wmask |= CWWidth;
	}
	if (c->c.w != (fwc.width = n->w)) {
		c->c.w = n->w;
		DPRINTF("frame wc.w = %u\n", fwc.width);
		fmask |= CWWidth;
	}
	if (c->c.h - c->c.t - c->c.g - c->c.v != (wwc.height = n->h - n->t - n->g - n->v)) {
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
	if (c->c.t != (wwc.y = n->t)) {
		c->c.t = n->t;
		DPRINTF("wind  wc.y = %d\n", wwc.y);
		wmask |= CWY;
		tchange = True;
	}
	if (n->g && !c->grips)
		n->g = 0;
	if (c->c.g != n->g) {
		c->c.g = n->g;
		gchange = True;
	}
	if (n->v && !c->grips)
		n->v = 0;
	if (c->c.v != n->v) {
		c->c.v = n->v;
		hchange = True;
	}
	if ((n->t || n->v) && (c->is.shaded && (c != sel || !scr->options.autoroll))) {
		fwc.height = n->t + 2 * n->v;
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
	if (shaded) {
		XEvent ev;

		XUnmapWindow(dpy, c->win);
		XSync(dpy, False);
		XCheckIfEvent(dpy, &ev, &check_unmapnotify, (XPointer) c);
	} else
		XMapWindow(dpy, c->win);
	if (wmask) {
		DPRINTF("wind  wc = %ux%u+%d+%d:%d\n", wwc.width, wwc.height, wwc.x,
			wwc.y, wwc.border_width);
		XConfigureWindow(dpy, c->win, wmask | CWX | CWY | CWBorderWidth, &wwc);
	}
	/* ICCCM 2.0 4.1.5 */
	if (force || ((fmask | wmask) && !(wmask & (CWWidth | CWHeight))))
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
	    ((tchange && n->t) || (gchange && n->g) || (hchange && n->v)
	     || (wmask & CWWidth)))
		drawclient(c);
	if (tchange || gchange || hchange || (fmask & CWBorderWidth))
		ewmh_update_net_window_extents(c);
	XSync(dpy, False);
}

static Bool
constrain(Client *c, ClientGeometry *g)
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

	g = c->r;
	DPRINTF("CALLING: constrain()\n");
	constrain(c, &g);
	DPRINTF("CALLING reconfigure()\n");
	reconfigure(c, &g, False);
}

Bool
configureclient(XEvent *e, Client *c, int gravity)
{
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	View *v;

	/* XXX: if c->cmon is set, we are displayed on a monitor, but c->curview should
	   also be set.  We only need the monitor for the layout.  When there is no cmon, 
	   use one of the views to which the client is tagged? How meaningful is it
	   moving a window not in the current view? Perhaps we should treat it a just
	   moving the saved floating state.  This is the only function that calls
	   findcurmonitor(). */

	/* This is not quite correct anymore.  The client requests reconfiguration of its 
	   interior window and uses the border width specified or last specified and the
	   specified gravity as though it was never reparented and has no decorative
	   border.  We need to move and resize the frame so that the reference points are 
	   intact. */

	if (!(v = c->cview ? : selview()))
		return False;
	if ((!c->is.max && isfloating(c, v)) || c->is.bastard) {
		ClientGeometry g;
		int dx, dy, dw, dh;

		g = c->c;

		/* we should really refuse to do anything at this point... */
		dx = (ev->value_mask & CWX) ? ev->x - g.x : 0;
		dy = (ev->value_mask & CWY) ? ev->y - g.y : 0;
		dw = (ev->value_mask & CWWidth) ? ev->width - g.w : 0;
		dh = (ev->value_mask & CWHeight) ? ev->height - (g.h - g.t - g.g) : 0;
		g.b = (ev->value_mask & CWBorderWidth) ? ev->border_width : g.b;

		moveresizeclient(c, dx, dy, dw, dh, gravity);
		/* TODO: check _XA_WIN_CLIENT_MOVING and handle moves between monitors */
	} else {
		ClientGeometry g;

		CPRINTF(c, "refusing to reconfigure client\n");
		g = c->c;

		g.b = (ev->value_mask & CWBorderWidth) ? ev->border_width : g.b;

		/* When a client requests a different configuration and it is in tiling
		   mode or maximized, we need to refuse whatever it asked for and
		   generate configure notifies for the tiled arrangment. Unfortunately,
		   reconfigure() only does this when the configuration changes, so we
		   need to ask it to do a forced reconfiguration. */
		DPRINTF("CALLING reconfigure()\n");
		reconfigure(c, &g, True);
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
		ClientGeometry g = { 0, };

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
		g.v = 0;
		reconfigure(c, &g, False);
	}
	return True;
}

static void
calc_full(Client *c, View *v, ClientGeometry *g)
{
	Monitor *m = v->curmon;
	Monitor *fsmons[4] = { m, m, m, m };
	long *mons;
	unsigned long n = 0;
	int i;

	mons = getcard(c->win, _XA_NET_WM_FULLSCREEN_MONITORS, &n);
	if (n >= 4) {
		Monitor *fsm[4] = { NULL, NULL, NULL, NULL };

		for (i = 0; i < 4; i++)
			if (!(fsm[i] = findmonbynum(mons[i])))
				break;
		if (i == 4 &&
		    fsm[0]->sc.y < fsm[1]->sc.y + fsm[1]->sc.h &&
		    fsm[2]->sc.x < fsm[3]->sc.x + fsm[3]->sc.w) {
			fsmons[0] = fsm[0];
			fsmons[1] = fsm[1];
			fsmons[2] = fsm[2];
			fsmons[3] = fsm[3];
		}
		XFree(mons);
	}
	g->x = fsmons[2]->sc.x;
	g->y = fsmons[0]->sc.y;
	g->w = fsmons[3]->sc.x + fsmons[3]->sc.w - g->x;
	g->h = fsmons[1]->sc.y + fsmons[1]->sc.h - g->y;
	g->b = 0;
	g->t = 0;
	g->g = 0;
}

static void
calc_fill(Client *c, View *v, Workarea *wa, ClientGeometry *g)
{
	int x1, x2, y1, y2, w, h;
	Client *o;

	x1 = wa->x;
	x2 = wa->x + wa->w;
	y1 = wa->y;
	y2 = wa->y + wa->h;

	for (o = scr->clients; o; o = o->next) {
		if (!(isvisible(o, v)))
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
calc_lhalf(Client *c, Workarea *wa, ClientGeometry *g)
{
	g->x = wa->x;
	g->y = wa->y;
	g->w = wa->w >> 1;
	g->h = wa->h;
	g->b = scr->style.border;
}

static void
calc_rhalf(Client *c, Workarea *wa, ClientGeometry *g)
{
	g->x = wa->x + (wa->w >> 1);
	g->y = wa->y;
	g->w = wa->w >> 1;
	g->h = wa->h;
	g->b = scr->style.border;
}

static void
calc_max(Client *c, Workarea *wa, ClientGeometry *g)
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
calc_maxv(Client *c, Workarea *wa, ClientGeometry *g)
{
	g->y = wa->y;
	g->h = wa->h;
	g->b = scr->style.border;
}

static void
calc_maxh(Client *c, Workarea *wa, ClientGeometry *g)
{
	g->x = wa->x;
	g->w = wa->w;
	g->b = scr->style.border;
}

static void
get_decor(Client *c, View *v, ClientGeometry *g)
{
	Bool decorate;

	if (c->is.dockapp) {
		g->t = 0;
		g->g = 0;
		return;
	}
	g->t = c->c.t;
	g->g = c->c.g;
	g->v = c->c.v;
	if (c->is.max || !c->has.title)
		g->t = 0;
	if (c->is.max || !c->has.grips)
		g->g = 0;
	if (c->is.max || !c->has.grips)
		g->v = 0;
	if (c->is.max || (!c->has.title && !c->has.grips))
		return;
	if (c->is.floater || c->skip.arrange)
		decorate = True;
	else if (c->is.shaded && (c != sel || !scr->options.autoroll))
		decorate = True;
	else if (!v && !(v = clientview(c))) {
		/* should never happen */
		decorate = ((v = onview(c)) && (v->dectiled || VFEATURES(v, OVERLAP))) ?
		    True : False;
	} else {
		decorate = (v->dectiled || VFEATURES(v, OVERLAP)) ? True : False;
	}
	g->t = decorate ? ((c->title && c->has.title) ? scr->style.titleheight : 0) : 0;
	g->g = decorate ? ((c->grips && c->has.grips) ? scr->style.gripsheight : 0) : 0;
	g->v = decorate ? ((c->grips && c->has.grips && scr->style.fullgrips) ?
			   g->g : 0) : 0;
}

static void
discardcrossing()
{
	XEvent ev;

	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask|LeaveWindowMask, &ev)) ;
}

static void
getworkarea(Monitor *m, Workarea *w)
{
	Workarea *wa;

	switch (m->curview->barpos) {
	case StrutsOn:
	default:
		if (m->dock.position != DockNone)
			wa = &m->dock.wa;
		else
			wa = &m->wa;
		break;
	case StrutsHide:
		wa = &m->wa;
		break;
	case StrutsOff:
		wa = &m->sc;
		break;
	}
	w->x = max(wa->x, 1);
	w->y = max(wa->y, 1);
	w->w = min(wa->x + wa->w, DisplayWidth(dpy, scr->screen) - 1) - w->x;
	w->h = min(wa->y + wa->h, DisplayHeight(dpy, scr->screen) - 1) - w->y;
}

static void
updatefloat(Client *c, View *v)
{
	ClientGeometry g = { 0, };
	Workarea wa;

	if (c->is.dockapp)
		return;
	if ((!v && !(v = c->cview)) || !v->curmon)
		return;
	if (!isfloating(c, v))
		return;
	getworkarea(v->curmon, &wa);
	CPRINTF(c, "r: %dx%d+%d+%d:%d\n", c->r.w, c->r.h, c->r.x, c->r.y, c->r.b);
	g = c->r;
	CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
	g.b = scr->style.border;
	CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
	get_decor(c, v, &g);
	CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
	if (c->is.full) {
		calc_full(c, v, &g);
		CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
	} else {
		if (c->is.max) {
			calc_max(c, &wa, &g);
			CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
		} else if (c->is.lhalf) {
			calc_lhalf(c, &wa, &g);
			CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
		} else if (c->is.rhalf) {
			calc_rhalf(c, &wa, &g);
			CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
		} else if (c->is.fill) {
			calc_fill(c, v, &wa, &g);
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
	reconfigure(c, &g, False);
	if (c->is.max)
		ewmh_update_net_window_fs_monitors(c);
	discardcrossing();
}

static void
arrangefloats(View *v)
{
	Client *c;

	for (c = scr->stack; c; c = c->snext)
		if (isvisible(c, v) && !c->is.bastard && !c->is.dockapp)
			/* XXX: can.move? can.tag? */
			updatefloat(c, v);
}

Client *
nextdockapp(Client *c, View *v)
{
	for (; c && (!c->is.dockapp || !isvisible(c, v) || c->is.hidden); c = c->next) ;
	return c;
}

Client *
prevdockapp(Client *c, View *v)
{
	for (; c && (!c->is.dockapp || !isvisible(c, v) || c->is.hidden); c = c->prev) ;
	return c;
}

static unsigned
countdockapps(Container *t)
{
	unsigned active = 0, act, num;
	Container *c;

	switch (t->type) {
	case TreeTypeNode:
	case TreeTypeTerm:
		for (num = 0, act = 0, c = t->node.children.head; c; c = c->next, num++)
			act += countdockapps(c);
		t->node.children.number = num;
		t->node.children.active = act;
		active += act;
		break;
	case TreeTypeLeaf:
		if (t->leaf.client.client && !t->leaf.is.hidden)
			active++;
		break;
	}
	return active;
}

static void
pushleaf(Term *n)
{
	Leaf *l;
	Container *nnew, *nold;
	Bool active = False;

	/* push leaf from the node after this one and appeand to this one */
	if (!(nnew = (Container *) n)) {
		DPRINTF("ERROR: no node!\n");
		assert(nnew != NULL);
	}
	if (!(nold = nnew->next)) {
		DPRINTF("ERROR: no next node!\n");
		assert(nold != NULL);
	}
	if (!(l = nold->term.children.head)) {
		DPRINTF("ERROR: no leaf node!\n");
		assert(l != NULL);
	}
	if (l->type != TreeTypeLeaf) {
		DPRINTF("ERROR: not a leaf node!\n");
		assert(l->type == TreeTypeLeaf);
	}
	if (l->client.client && !l->is.hidden)
		active = True;
	delleaf(l, active);	/* optional really */
	appleaf(nnew, l, active);
}

static void
popleaf(Term *n)
{
	Leaf *l;
	Container *nnew, *nold;
	Bool active = False;

	/* pop leaf from this node and insert onto the one after this one */
	if (!(nold = (Container *) n)) {
		DPRINTF("ERROR: no node!\n");
		assert(nold != NULL);
	}
	if (!(nnew = nold->next)) {
		DPRINTF("ERROR: no next node!\n");
		assert(nnew != NULL);
	}
	if (!(l = nold->term.children.tail)) {
		DPRINTF("ERROR: no leaf node!\n");
		assert(l != NULL);
	}
	if (l->type != TreeTypeLeaf) {
		DPRINTF("ERROR: not a leaf node!\n");
		assert(l->type == TreeTypeLeaf);
	}
	if (l->client.client && !l->is.hidden)
		active = True;
	delleaf(l, active);	/* optional really */
	insleaf(nnew, l, active);
}

static void
arrangedock(View *v)
{
	Leaf *l;
	Monitor *m;
	Container *t, *n;
	int i, j, num, major, minor, count, b;
	static const unsigned max = 64;
	Workarea wa;
	Bool floating = VFEATURES(v, OVERLAP) ? True : False;
	Bool hide = (v->barpos == StrutsOff) && (!sel || !sel->is.dockapp);
	unsigned min = hide ? 2 : max;

	if (!(m = v->curmon))
		return;

	updategeom(m);
	wa = m->dock.wa = m->wa;

	if (m->dock.position == DockNone)
		return;
	if (!(t = scr->dock.tree))
		return;
	n = (Container *) t->node.children.head;

	b = hide ? scr->style.border : 0;
	t->t.x = t->f.x = wa.x;
	t->t.y = t->f.y = wa.y;
	t->t.w = t->f.w = wa.w;
	t->t.h = t->f.h = wa.h;
	t->t.b = t->f.b = b;
	t->t.t = t->f.t = 0;
	t->t.g = t->f.g = 0;
	t->t.v = t->f.v = 0;
	t->c = floating ? t->f : t->t;

	num = countdockapps((Container *) t);
	DPRINTF("there are %d dock apps\n", num);
	if (!num)
		return;

	switch (t->node.children.ori) {
	case OrientLeft:
	case OrientRight:
		minor = (wa.h - b) / (max + b);
		major = (num - 1) / minor + 1;
		break;
	case OrientTop:
	case OrientBottom:
		minor = (wa.w - b) / (max + b);
		major = (num - 1) / minor + 1;
		break;
	default:
		DPRINTF("ERROR: invalid child orientation\n");
		return;
	}
	while (t->node.children.number < major)
		n = adddocknode(t);
	count = num;
	for (i = 0, n = (Container *) t->node.children.head; i < major; i++, n = n->next) {
		ClientGeometry gt, gf;
		double pt, dt;
		double pf, df;

		if (minor > count)
			minor = count;
		while (n->term.children.active < minor)
			pushleaf(&n->term);
		while (n->term.children.active > minor)
			popleaf(&n->term);
		gf = t->f;
		gt = t->t;
		switch (t->node.children.ori) {
		case OrientLeft:
		case OrientRight:
			wa.w -= min + b + (i ? 0 : b);
			pt = wa.y;
			gf.w = gt.w = min + 2 * b;
			dt = wa.h;
			df = minor * (max + b) + b;
			switch ((int) t->node.children.ori) {
			default:
			case OrientLeft:
				gf.x = gt.x = wa.x;
				switch ((int) t->node.children.pos) {
				case PositionNorthWest:
					pf = wa.y;
					break;
				default:
				case PositionWest:
					pf = wa.y + (wa.h - df) / 2.0;
					break;
				case PositionSouthWest:
					pf = wa.y + (wa.h - df);
					break;
				}
				wa.x += min + b;
				break;
			case OrientRight:
				gf.x = gt.x = wa.x + wa.w;
				switch ((int) t->node.children.pos) {
				case PositionNorthEast:
					pf = wa.y;
					break;
				default:
				case PositionEast:
					pf = wa.y + (wa.h - df) / 2.0;
					break;
				case PositionSouthEast:
					pf = wa.y + (wa.h - df);
					break;
				}
				break;
			}
			gt.y = pt;
			gf.y = pf;
			gt.h = dt;
			gf.h = df;
			break;
		case OrientTop:
		case OrientBottom:
			wa.h -= min + b + (i ? 0 : b);
			pt = wa.x;
			gf.h = gt.h = min + 2 * b;
			dt = wa.w;
			df = minor * (max + b) + b;
			switch ((int) t->node.children.ori) {
			default:
			case OrientTop:
				gf.y = gt.y = wa.y;
				switch ((int) t->node.children.pos) {
				case PositionNorthWest:
					pf = wa.x;
					break;
				default:
				case PositionNorth:
					pf = wa.x + (wa.w - df) / 2.0;
					break;
				case PositionNorthEast:
					pf = wa.x + (wa.w - df);
					break;
				}
				wa.y += min + b;
				break;
			case OrientBottom:
				gf.y = gt.y = wa.y + wa.h;
				switch ((int) t->node.children.pos) {
				case PositionSouthWest:
					pf = wa.x;
					break;
				default:
				case PositionSouth:
					pf = wa.x + (wa.w - df) / 2.0;
					break;
				case PositionSouthEast:
					pf = wa.x + (wa.w - df);
					break;
				}
				break;
			}
			gt.x = pt;
			gf.x = pf;
			gt.w = dt;
			gf.w = df;
			break;
		default:
			DPRINTF("ERROR: invalid childe orientation\n");
			return;
		}
		n->t = gt;
		n->f = gf;
		n->c = floating ? gf : gt;
		switch (n->term.children.ori) {
		case OrientLeft:
			/* left to right */
			dt = ((double)(dt - b)) /((double) minor) + b;
			df = ((double)(df - b)) /((double) minor) + b;
			pt = pt;
			pf = pf;
			gt.w = dt;
			gf.w = df;
			gt.x = pt;
			gf.x = pf;
			break;
		default:
		case OrientRight:
			/* right to left */
			dt = ((double)(dt - b)) /((double) minor) + b;
			df = ((double)(df - b)) /((double) minor) + b;
			pt = pt + n->t.w - dt;
			pf = pf + n->f.w - df;
			gt.w = dt;
			gf.w = df;
			gt.x = pt;
			gf.x = pf;
			break;
		case OrientTop:
			/* top to bottom */
			dt = ((double)(dt - b)) /((double) minor) + b;
			df = ((double)(df - b)) /((double) minor) + b;
			pt = pt;
			pf = pf;
			gt.h = dt;
			gf.h = df;
			gt.y = pt;
			gf.y = pf;
			break;
		case OrientBottom:
			/* bottom to top */
			dt = ((double)(dt - b)) /((double) minor) + b;
			df = ((double)(df - b)) /((double) minor) + b;
			pt = pt + n->t.h - dt;
			pf = pf + n->f.h - df;
			gt.h = dt;
			gf.h = df;
			gt.y = pt;
			gf.y = pf;
			break;
		}
		for (j = 1, l = n->term.children.head; l && j <= minor; l = l->next) {
			Client *c;

			if (!(c = l->client.client))
				continue;
			if (l->is.hidden || c->is.hidden) {
				ban(c);
				continue;
			}
			if (j == minor) {
				switch (n->term.children.ori) {
				case OrientLeft:
					/* left to right */
					gt.w = n->t.x + n->t.w - gt.x;
					gf.w = n->f.x + n->f.w - gf.x;
					break;
				case OrientRight:
					/* right to left */
					gt.w += gt.x - n->t.x;
					gt.x = n->t.x;
					gf.w += gf.x - n->f.x;
					gf.x = n->f.x;
					break;
				case OrientTop:
					/* top to bottom */
					gt.h = n->t.y + n->t.h - gt.y;
					gf.h = n->f.y + n->f.h - gf.y;
					break;
				case OrientBottom:
					/* bottom to top */
					gt.h += gt.y - n->t.y;
					gt.y = n->t.y;
					gf.h += gf.y - n->f.y;
					gf.y = n->f.y;
					break;
				}
			}
			l->t = gt;
			l->f = gf;
			l->c = floating ? gf : gt;
			{
				ClientGeometry g = l->c;

				if (!floating && c->is.moveresize) {
					/* center it where it already is located */
					g.x = (c->c.x + c->c.w / 2) - g.w / 2;
					g.y = (c->c.y + c->c.h / 2) - g.h / 2;
				}
				g.w -= 2 * g.b;
				g.h -= 2 * g.b;
				DPRINTF("CALLING reconfigure()\n");
				reconfigure(c, &g, False);
			}
			unban(c, v);
			switch (n->term.children.ori) {
			case OrientLeft:
				/* left to right */
				pt += dt - b;
				pf += df - b;
				gt.w = pt - gt.x + b;
				gf.w = pf - gf.x + b;
				gt.x = pt;
				gf.x = pf;
				break;
			case OrientRight:
				/* right to left */
				pt -= dt - b;
				pf -= df - b;
				gt.w = gt.x - pt + b;
				gf.w = gf.x - pf + b;
				gt.x = pt;
				gf.x = pf;
				break;
			case OrientTop:
				/* top to bottom */
				pt += dt - b;
				pf += df - b;
				gt.h = pt - gt.y + b;
				gf.h = pf - gf.y + b;
				gt.y = pt;
				gf.y = pf;
				break;
			case OrientBottom:
				/* bottom to top */
				pt -= dt - b;
				pf -= df - b;
				gt.h = gt.y - pt + b;
				gf.h = gf.y - pf + b;
				gt.y = pt;
				gf.y = pf;
				break;
			}
			j++;
		}
		count -= minor;
	}
	switch (t->node.children.ori) {
	default:
	case OrientRight:
		wa.w += b;
		break;
	case OrientLeft:
		wa.w += b;
		wa.x -= b;
		break;
	case OrientBottom:
		wa.h += b;
		break;
	case OrientTop:
		wa.h += b;
		wa.y -= b;
		break;
	}
	m->dock.wa = wa;
	assert(count == 0);
	for (; i < t->node.children.number; i++, n = n->next) {
		/* unused rows/cols at end */
		assert(n->term.children.active == 0);
		n->t.x = n->f.x = n->c.x = 0;
		n->t.y = n->f.y = n->c.y = 0;
		n->t.w = n->f.w = n->c.w = 0;
		n->t.h = n->f.h = n->c.h = 0;
	}
}

Client *
nexttiled(Client *c, View *v)
{
	for (; c && (c->is.dockapp || c->is.floater || c->skip.arrange || !isvisible(c, v)
		     || c->is.bastard || (c->is.icon || c->is.hidden)); c = c->next) ;
	return c;
}

Client *
prevtiled(Client *c, View *v)
{
	for (; c && (c->is.dockapp || c->is.floater || c->skip.arrange || !isvisible(c, v)
		     || c->is.bastard || (c->is.icon || c->is.hidden)); c = c->prev) ;
	return c;
}

static void
tile(View *v)
{
	LayoutArgs wa, ma, sa;
	ClientGeometry n = { 0, }, m = { 0, }, s = { 0, }, g = { 0, };
	Client *c, *mc;
	Monitor *cm = v->curmon;
	int i, overlap, th, gh, hh, mg;
	Bool mtdec, stdec, mgdec, sgdec, mvdec, svdec;

	if (!(c = nexttiled(scr->clients, v)))
		return;

	th = scr->style.titleheight;
	gh = scr->style.gripsheight;
	hh = scr->style.fullgrips ? gh : 0;
	mg = scr->style.margin;

	getworkarea(cm, (Workarea *) &wa);
	for (wa.n = wa.s = ma.s = sa.s = 0, c = nexttiled(scr->clients, v);
	     c; c = nexttiled(c->next, v), wa.n++) {
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
	mvdec = v->dectiled ? True : False;
	svdec = v->dectiled ? True : False;

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
	m.v = mvdec ? hh : 0;

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
	s.v = svdec ? hh : 0;

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
	c = mc = nexttiled(scr->clients, v);

	/* lay out the master area */
	n = m;

	for (; c && i < ma.n; c = nexttiled(c->next, v)) {
		IsUnion is = {.is = 0 };

		if (c->is.max) {
			c->is.max = False;
			ewmh_update_net_window_state(c);
		}
		g = n;
		g.t = c->has.title ? g.t : 0;
		g.g = c->has.grips ? g.g : 0;
		if ((is.shaded = c->is.shaded) && (c != sel || !scr->options.autoroll))
			if (!ma.s)
				c->is.shaded = False;
		g.x += ma.g;
		g.y += ma.g;
		g.w -= 2 * (ma.g + g.b);
		g.h -= 2 * (ma.g + g.b);
		if (!c->is.moveresize) {
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &g, False);
		} else {
			ClientGeometry C = g;

			/* center it where it was before */
			C.x = (c->c.x + c->c.w / 2) - C.w / 2;
			C.y = (c->c.y + c->c.h / 2) - C.h / 2;
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &C, False);
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
		c->is.shaded = is.shaded;
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

	for (; c && i < wa.n; c = nexttiled(c->next, v)) {
		IsUnion is = {.is = 0 };

		if (c->is.max) {
			c->is.max = False;
			ewmh_update_net_window_state(c);
		}
		g = n;
		g.t = c->has.title ? g.t : 0;
		g.g = c->has.grips ? g.g : 0;
		if ((is.shaded = c->is.shaded) && (c != sel || !scr->options.autoroll))
			if (!sa.s)
				c->is.shaded = False;
		g.x += sa.g;
		g.y += sa.g;
		g.w -= 2 * (sa.g + g.b);
		g.h -= 2 * (sa.g + g.b);
		if (!c->is.moveresize) {
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &g, False);
		} else {
			ClientGeometry C = g;

			/* center it where it was before */
			C.x = (c->c.x + c->c.w / 2) - C.w / 2;
			C.y = (c->c.y + c->c.h / 2) - C.h / 2;
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &C, False);
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
		c->is.shaded = is.shaded;
	}
}

static void
arrange_tile(View *v)
{
	arrangedock(v);
	tile(v);
	arrangefloats(v);
}

static void
create_tile(View *v)
{
}

static void
convert_tile(View *v)
{
}

static void
initlayout_tile(View *v)
{
	if (v->tree)
		convert_tile(v);
	else
		create_tile(v);
}

static void
grid(View *v)
{
	Client *c;
	Workarea wa;
	int rows, cols, n, i, *rc, *rh, col, row;
	int cw, cl, *rl;
	int gap;

	if (!(c = nexttiled(scr->clients, v)))
		return;

	getworkarea(v->curmon, &wa);

	for (n = 0, c = nexttiled(scr->clients, v); c; c = nexttiled(c->next, v), n++) ;

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

	for (i = 0, col = 0, row = 0, c = nexttiled(scr->clients, v); c && i < n;
	     c = nexttiled(c->next, v), i++, col = i % cols, row = i / cols) {
		ClientGeometry n = { 0, };

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
			reconfigure(c, &n, False);
		} else {
			ClientGeometry C = n;

			/* center it where it was before */
			C.x = (c->c.x + c->c.w / 2) - C.w / 2;
			C.y = (c->c.y + c->c.h / 2) - C.h / 2;
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &C, False);
		}
	}
	free(rc);
	free(rh);
	free(rl);
}

static void
arrange_grid(View *v)
{
	arrangedock(v);
	grid(v);
	arrangefloats(v);
}

static void
create_grid(View *v)
{
}

static void
convert_grid(View *v)
{
}

static void
initlayout_grid(View *v)
{
	if (v->tree)
		convert_grid(v);
	else
		create_grid(v);
}

static void
monocle(View *v)
{
	Client *c;
	Workarea w;

	getworkarea(v->curmon, &w);
	DPRINTF("work area is %dx%d+%d+%d\n", w.w, w.h, w.x, w.y);
	for (c = nexttiled(scr->clients, v); c; c = nexttiled(c->next, v)) {
		ClientGeometry g = { 0, };

		g.x = w.x;
		g.y = w.y;
		g.w = w.w;
		g.h = w.h;
		g.t = (v->dectiled && c->has.title) ? scr->style.titleheight : 0;
		g.g = (v->dectiled && c->has.grips) ? scr->style.gripsheight : 0;
		g.v = scr->style.fullgrips ? g.g : 0;
		g.b = (g.t || g.g || g.v) ? scr->style.border : 0;
		g.w -= 2 * g.b;
		g.h -= 2 * g.b;

		DPRINTF("CALLING reconfigure()\n");
		reconfigure(c, &g, False);
	}
}

static void
arrange_monocle(View *v)
{
	arrangedock(v);
	monocle(v);
	arrangefloats(v);
}

static void
create_monocle(View *v)
{
}

static void
convert_monocle(View *v)
{
}

static void
initlayout_monocle(View *v)
{
	if (v->tree)
		convert_monocle(v);
	else
		create_monocle(v);
}

static void
arrange_float(View *v)
{
	arrangedock(v);
	arrangefloats(v);
}

static void
create_float(View *v)
{
}

static void
convert_float(View *v)
{
}

static void
initlayout_float(View *v)
{
	if (v->tree)
		convert_float(v);
	else
		create_float(v);
}

static void
arrangeview(View *v)
{
	Client *c;
	int struts;

	if (v->layout && v->layout->arrange && v->layout->arrange->arrange)
		v->layout->arrange->arrange(v);
	struts = v->barpos;
	for (c = scr->stack; c; c = c->snext) {
		if ((clientview(c) == v) &&
		    ((!c->is.bastard && !c->is.dockapp && !(c->is.icon || c->is.hidden))
		     || ((c->is.bastard || c->is.dockapp) && struts != StrutsHide))) {
			unban(c, v);
		}
	}

	for (c = scr->stack; c; c = c->snext) {
		if ((clientview(c) == NULL)
		    || (!c->is.bastard && !c->is.dockapp && (c->is.icon || c->is.hidden))
		    || ((c->is.bastard || c->is.dockapp) && struts == StrutsHide)) {
			ban(c);
		}
	}
	for (c = scr->stack; c; c = c->snext)
		ewmh_update_net_window_state(c);
}

static Bool
isfloater(Client *c)
{
	if (c->is.floater || c->skip.arrange)
		return True;
	if (c->is.full)
		return True;
	if (c->cview && VFEATURES(c->cview, OVERLAP))
		return True;
	return False;
}

typedef struct StackContext {
	int n;				/* number of clients in stack */
	int i;				/* current client being considered */
	int j;				/* current stack position */
	Window *wl;			/* frame window list */
	Client **ol;			/* original client list */
	Client **cl;			/* unstacked client list */
	Client **sl;			/* stacked client list */
} StackContext;

void
stack_client(StackContext *s, Client *c)
{
	if (c->breadcrumb)
		return;
	s->cl[s->i] = NULL;
	s->sl[s->j] = c;
	s->wl[s->j] = c->frame;
	c->breadcrumb++;
	s->j++;
	XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
}

/** @brief - stack clients
  *
  * General rules that must also be applied:
  *
  * 1. Global modal windows must always go on top.
  * 2. Group modal windows must always go above their group.  When top-down
  *    stacking the group leader, group modal windows must be stacked first.
  * 3. Transient windows should go above the windows for which they are
  *    transient.  When stacking a window, all its transients must be stacked
  *    first.
  */
void
stack_clients(StackContext * s, Client *c)
{
	Client *m;
	Group *g;
	Window w;
	int k;

	/* always stack modal windows */
	if (c->is.modal)
		return stack_client(s, c);
	/* need to ensure that group modals are stacked first */
	if (!c->nonmodal)
		goto no_modal;
	if (c->is.grptrans && (w = c->leader) && (g = getleader(w, ClientLeader)))
		for (k = 0; k < g->count; k++)
			if ((w = g->members[k]) != c->win &&
			    (m = getclient(w, ClientWindow)) && m->is.modal)
				stack_client(s, m);
	if (c->is.transient && (w = c->transfor) && (g = getleader(w, ClientTransFor)))
		for (k = 0; k < g->count; k++)
			if ((w = g->members[k]) != c->win &&
			    (m = getclient(w, ClientWindow)) && m->is.modal)
				stack_client(s, m);
      no_modal:
	/* stack transients above windows they are transient for */
	if ((g = getleader(c->win, ClientLeader)))
		for (k = 0; k < g->count; k++)
			if ((w = g->members[k]) != c->win &&
			    (m = getclient(w, ClientWindow)) &&
			    m->is.grptrans && m->leader == c->win)
				stack_client(s, m);
	if ((g = getleader(c->win, ClientTransFor)))
		for (k = 0; k < g->count; k++)
			if ((w = g->members[k]) != c->win &&
			    (m = getclient(w, ClientWindow)) &&
			    m->is.transient && m->transfor == c->win)
				stack_client(s, m);
	/* stack the window itself */
	return stack_client(s, c);
}

/** @brief - restack windows
  *
  * The rationale is as follows: (from top to bottom)
  *
  * 1. (12) (WIN_LAYER_MENU      ) Global modal windows (if any).
  * 2. (10) (WIN_LAYER_ABOVE_DOCK) Focused windows with state _NET_WM_STATE_FULLSCREEN  (but not type Desk)
  * 3. ( 8) (WIN_LAYER_DOCK      ) Dockapps when a dockapp is selected and docks when selected.
  * 4. ( 6) (WIN_LAYER_ONTOP     ) Window with type Dock and not state Below and windows with state Above.
  * 5. ( 4) (WIN_LAYER_NORMAL    ) Other windows without state Below.
  * 6. ( 2) (WIN_LAYER_BELOW     ) Windows with state Below and Dock not already stacked
  * 7. ( 0) (WIN_LAYER_DESKTOP   ) Windows with type Desk.
  *
  * General rules that must also be applied:
  *
  * 4. Focused windows with both state Above and Below are treated as though
  *    Below was not set.
  * 5. Unfocused windows with both state Above and Below are treated as though
  *    Above was not set.
  */
static void
restack()
{
	StackContext s = { 0, };
	Client *c;

	XPRINTF("%s\n", "RESTACKING: -------------------------------------");
	for (s.n = 0, c = scr->stack; c; c = c->snext, s.n++)
		c->breadcrumb = 0;
	if (!s.n) {
		ewmh_update_net_client_list_stacking();
		return;
	}
	s.ol = ecalloc(s.n, sizeof(*s.ol));
	s.cl = ecalloc(s.n, sizeof(*s.cl));
	s.sl = ecalloc(s.n, sizeof(*s.sl));
	s.wl = ecalloc(s.n, sizeof(*s.wl));

	for (s.i = 0, s.j = 0, c = scr->stack; c;
	     s.ol[s.i] = s.cl[s.i] = c, s.i++, c = c->snext) ;

	/* 1. Global modal windows (if any). */
	if (window_stack.modal_transients) {
		for (s.i = 0; s.i < s.n; s.i++) {
			if (!(c = s.cl[s.i]))
				continue;
			if (c->is.modal == ModalSystem)
				stack_clients(&s, c);
		}
	}
	/* 2. Focused windows with state _NET_WM_STATE_FULLSCREEN  (but not type Desk) */
	for (s.i = 0; s.i < s.n; s.i++) {
		if (!(c = s.cl[s.i]))
			continue;
		if (WTCHECK(c, WindowTypeDesk))
			continue;
		if (sel == c && c->is.full)
			stack_clients(&s, c);
	}
	/* 3. Dockapps when a dockapp is selected and docks when selected. */
	if (sel && (WTCHECK(sel, WindowTypeDock) || sel->is.dockapp)) {
		if (sel->is.dockapp) {
			for (s.i = 0; s.i < s.n; s.i++) {
				if (!(c = s.cl[s.i]))
					continue;
				if (c->is.dockapp)
					stack_clients(&s, c);
			}
		} else {
			for (s.i = 0; s.i < s.n; s.i++) {
				if (!(c = s.cl[s.i]))
					continue;
				if (sel == c)
					stack_clients(&s, c);
			}
		}
	}
	/* 4. Window with type Dock and not state Below and windows with state Above. */
	for (s.i = 0; s.i < s.n; s.i++) {
		if (!(c = s.cl[s.i]))
			continue;
		if (WTCHECK(c, WindowTypeDesk))
			continue;
		if ((WTCHECK(c, WindowTypeDock) && !c->is.dockapp && !c->is.below && !c->is.banned) || c->is.above)
			stack_clients(&s, c);
	}
	/* 5. Windows (other than Desk or Dock) without state Below. */
	/* floating above */
	for (s.i = 0; s.i < s.n; s.i++) {
		if (!(c = s.cl[s.i]))
			continue;
		if (WTCHECK(c, WindowTypeDesk))
			continue;
		if ((WTCHECK(c, WindowTypeDock) || c->is.dockapp))
			continue;
		if (c->is.below)
			continue;
		if (isfloater(c))
			stack_clients(&s, c);
	}
	/* tiled below */
	for (s.i = 0; s.i < s.n; s.i++) {
		if (!(c = s.cl[s.i]))
			continue;
		if (WTCHECK(c, WindowTypeDesk))
			continue;
		if ((WTCHECK(c, WindowTypeDock) || c->is.dockapp))
			continue;
		if (c->is.below)
			continue;
		if (!isfloater(c))
			stack_clients(&s, c);
	}
	/** 6. Windows with state Below (but not type Desk) and Dock not already stacked */
	for (s.i = 0; s.i < s.n; s.i++) {
		if (!(c = s.cl[s.i]))
			continue;
		if (WTCHECK(c, WindowTypeDesk))
			continue;
		if ((WTCHECK(c, WindowTypeDock) || c->is.dockapp) || c->is.below)
			stack_clients(&s, c);
	}
	/** 7. Windows with type Desk. **/
	for (s.i = 0; s.i < s.n; s.i++) {
		if (!(c = s.cl[s.i]))
			continue;
		if (WTCHECK(c, WindowTypeDesk))
			stack_clients(&s, c);
	}
	assert(s.j == s.n);
	free(s.cl); s.cl = NULL;

	if (bcmp(s.ol, s.sl, s.n * sizeof(*s.ol))) {
		XPRINTF("%s", "Old stacking order:\n");
		for (c = scr->stack; c; c = c->snext)
			XPRINTF("client frame 0x%08lx win 0x%08lx name %s%s\n",
				c->frame, c->win, c->name,
				c->is.bastard ? " (bastard)" : "");
		scr->stack = s.sl[0];
		for (s.i = 0; s.i < s.n - 1; s.i++)
			s.sl[s.i]->snext = s.sl[s.i + 1];
		s.sl[s.i]->snext = NULL;
		XPRINTF("%s", "New stacking order:\n");
		for (c = scr->stack; c; c = c->snext)
			XPRINTF("client frame 0x%08lx win 0x%08lx name %s%s\n",
				c->frame, c->win, c->name,
				c->is.bastard ? " (bastard)" : "");
	} else {
		XPRINTF("%s", "No new stacking order\n");
	}
	free(s.ol); s.ol = NULL;
	free(s.sl); s.sl = NULL;

	if (!window_stack.members || (window_stack.count != s.n) ||
	    bcmp(window_stack.members, s.wl, s.n * sizeof(*s.wl))) {
		free(window_stack.members);
		window_stack.members = s.wl;
		window_stack.count = s.n;

		XRestackWindows(dpy, s.wl, s.n);

		ewmh_update_net_client_list_stacking();
	} else {
		XPRINTF("%s", "No new stacking order\n");
		free(s.wl); s.wl = NULL;
	}
	discardcrossing();
}

static int
segm_overlap(int min1, int max1, int min2, int max2)
{
	int tmp;

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
	if (min2 >= max1 || min1 >= max2) {
		// min1 ------------------------ max1 min2 ------------------------ max2
		// min2 ------------------------ max2 min1 ------------------------ max1
		return (0);
	}
	if (min1 <= min2) {
		if (max1 <= max2) {
			// min1 ------------------------ max1
			// min2 ------------------------ max2
			return (max1 - min2);
		} else {
			// min1 ------------------------------------------------ max1
			// min2 ------------------------ max2
			return (max2 - min2);
		}
	} else {
		if (max1 <= max2) {
			// min1 ------------------------ max1
			// min2 ------------------------------------------------ max2
			return (max1 - min1);
		} else {
			// min1 ------------------------ max1
			// min2 ------------------------ max2
			return (max2 - min1);
		}
	}
}

static Bool
wind_overlap(int min1, int max1, int min2, int max2)
{
	return segm_overlap(min1, max1, min2, max2) ? True : False;
}

static Bool
place_overlap(ClientGeometry *c, ClientGeometry *o)
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
			if (c->snext == o)
				/* already just above sibling */
				return;
			*cp = c->snext;
			*op = c;
			c->snext = o;
		} else {
			/* top of stack */
			if (cp == &scr->stack)
				/* already at top */
				return;
			*cp = c->snext;
			c->snext = scr->stack;
			scr->stack = c;
		}
		break;
	case Below:
		if (o) {
			/* just below sibling */
			if (o->snext == c)
				/* already just below sibling */
				return;
			*cp = c->snext;
			c->snext = o->snext;
			o->snext = c;
		} else {
			/* bottom of stack */
			if (lp == &c->snext)
				/* already at bottom */
				return;
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
		if (cp == &scr->stack)
			/* already at top */
			return;
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
		if (lp == &c->snext)
			/* already at bottom */
			return;
		*cp = c->snext;
		c->snext = *lp;
		*lp = c;
		break;
	case Opposite:
		if (o) {
			if (client_occludes(o, c)) {
				/* top of stack */
				if (cp == &scr->stack)
					/* already at top */
					return;
				*cp = c->snext;
				c->snext = scr->stack;
				scr->stack = c;
			} else if (client_occludes(c, o)) {
				/* bottom of stack */
				if (lp == &c->snext)
					/* already at bottom */
					return;
				*cp = c->snext;
				c->snext = *lp;
				*lp = c;
			} else
				return;
		} else {
			if (client_occluded_any(c)) {
				/* top of stack */
				if (cp == &scr->stack)
					/* already at top */
					return;
				*cp = c->snext;
				c->snext = scr->stack;
				scr->stack = c;
			} else if (client_occludes_any(c)) {
				/* bottom of stack */
				if (lp == &c->snext)
					/* already at bottom */
					return;
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
	if (!c || (!c->prog.above && c->is.managed))
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
	if (!c || (!c->prog.below && c->is.managed))
		return;
	c->is.below = !c->is.below;
	if (c->is.managed) {
		restack();
		ewmh_update_net_window_state(c);
	}
}

void
arrange(View *ov)
{
	Monitor *m;

	if (!ov) {
		for (m = scr->monitors; m; m = m->next) {
			assert(m->curview != NULL);
			arrange(m->curview);
		}
	} else if ((m = ov->curmon)) {
		Workarea wa;

		getworkarea(m, &wa);
		if (scr->options.useveil) {
			XMoveResizeWindow(dpy, m->veil, wa.x, wa.y, wa.w, wa.h);
			XMapRaised(dpy, m->veil);
		}
		arrangeview(ov);
		restack();
		if (scr->options.useveil) {
			XUnmapWindow(dpy, m->veil);
		}
		discardcrossing();
	}
}

void
needarrange(View *v)
{
	if (!v) {
		unsigned i;

		for (v = scr->views, i = 0; i < scr->ntags; i++, v++)
			needarrange(v);
	} else
		v->needarrange = True;
}

void
arrangeneeded(void)
{
	unsigned i;
	View *v;

	for (v = scr->views, i = 0; i < scr->ntags; i++, v++) {
		if (v->needarrange) {
			arrange(v);
			v->needarrange = False;
		}
	}
}

Arrangement arrangement_FLOAT = {
	.name = "float",
	.initlayout = initlayout_float,
	.arrange = arrange_float,
};

Arrangement arrangement_TILE = {
	.name = "tile",
	.initlayout = initlayout_tile,
	.arrange = arrange_tile,
};

Arrangement arrangement_GRID = {
	.name = "grid",
	.initlayout = initlayout_grid,
	.arrange = arrange_grid,
};

Arrangement arrangement_MONO = {
	.name = "monocle",
	.initlayout = initlayout_monocle,
	.arrange = arrange_monocle,
};

Layout layouts[] = {
	/* *INDENT-OFF* */
	/* arrangement		symbol	features				major		minor		placement		*/
	{  &arrangement_FLOAT,	'i',	OVERLAP,				0,		0,		ColSmartPlacement	},
	{  &arrangement_TILE,	't',	MWFACT | NMASTER | ZOOM | ROTL | MMOVE,	OrientLeft,	OrientBottom,	ColSmartPlacement	},
	{  &arrangement_TILE,	'b',	MWFACT | NMASTER | ZOOM | ROTL | MMOVE,	OrientBottom,	OrientLeft,	ColSmartPlacement	},
	{  &arrangement_TILE,	'u',	MWFACT | NMASTER | ZOOM | ROTL | MMOVE,	OrientTop,	OrientRight,	ColSmartPlacement	},
	{  &arrangement_TILE,	'l',	MWFACT | NMASTER | ZOOM | ROTL | MMOVE,	OrientRight,	OrientTop,	ColSmartPlacement	},
	{  &arrangement_MONO,	'm',	0,					0,		0,		ColSmartPlacement	},
	{  &arrangement_FLOAT,	'f',	OVERLAP,				0,		0,		ColSmartPlacement	},
	{  &arrangement_GRID,	'g',	NCOLUMNS | ROTL | MMOVE,		OrientLeft,	OrientTop,	ColSmartPlacement	},
	{  NULL,		'\0',	0,					0,		0,		0			}
	/* *INDENT-ON* */
};

void
setlayout(const char *arg)
{
	View *v;
	Layout *l;

	if (!(v = selview()) || !arg)
		return;
	for (l = layouts; l->symbol; l++)
		if (*arg == l->symbol)
			break;
	if (!l->symbol || v->layout == l)
		return;
	v->layout = l;
	v->major = l->major;
	v->minor = l->minor;
	v->placement = l->placement;
	if (l->arrange && l->arrange->initlayout)
		l->arrange->initlayout(v);
	arrange(v);
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
	Client *o;

	for (o = scr->stack; o; o = o->snext)
		if (isvisible(o, c->cview) &&
		    o->is.bastard == c->is.bastard && o->is.dockapp == c->is.dockapp)
			break;
	if (c == o)
		lowerclient(c);
	else
		raiseclient(c);
}

void
raisefloater(Client *c)
{
	if (c->is.floater || c->skip.arrange || c->is.full ||
		(c->cview && VFEATURES(c->cview, OVERLAP))) {
		CPRINTF(c, "raising floating client for focus\n");
		raiseclient(c);
	}
}

void
raisetiled(Client *c)
{
	if (c->is.bastard || c->is.dockapp || !isfloating(c, c->cview)) {
		CPRINTF(c, "raising non-floating client on focus\n");
		raiseclient(c);
	}
}

void
lowertiled(Client *c)
{
#if 0
	/* NEVER do this! */
	if (c->is.bastard || c->is.dockapp || !isfloating(c, c->cview)) {
		CPRINTF(c, "lowering non-floating client on loss of focus\n");
		lowerclient(c);
	}
#endif
}


void
setmwfact(View *v, double factor)
{
	if (factor < 0.1)
		factor = 0.1;
	if (factor > 0.9)
		factor = 0.9;
	if (v->mwfact != factor) {
		v->mwfact = factor;
		arrange(v);
	}
}

void
setnmaster(View *v, int n)
{
	int length;
	Monitor *m;

	if (!VFEATURES(v, NMASTER))
		return;
	if (!(m = v->curmon))
		return;
	if (n < 1)
		n = DEFNMASTER;
	switch (v->major) {
	case OrientTop:
	case OrientBottom:
		length = m->wa.h;
		switch (v->minor) {
		case OrientTop:
		case OrientBottom:
			length = (double) m->wa.h * v->mwfact;
			break;
		default:
			break;
		}
		if (length / n < 2 * (scr->style.titleheight + scr->style.border))
			return;
		break;
	case OrientLeft:
	case OrientRight:
		length = m->wa.w;
		switch (v->minor) {
		case OrientLeft:
		case OrientRight:
			length = (double) m->wa.w * v->mwfact;
			break;
		default:
			break;
		}
		if (length / n < 2 * (6 * scr->style.titleheight + scr->style.border))
			return;
		break;
	}
	if (v->nmaster != n) {
		v->nmaster = n;
		arrange(v);
	}
}

void
decnmaster(View *v, int n)
{
	if (!VFEATURES(v, NMASTER))
		return;
	setnmaster(v, v->nmaster - n);
}

void
incnmaster(View *v, int n)
{
	if (!VFEATURES(v, NMASTER))
		return;
	setnmaster(v, v->nmaster + n);
}

void
setncolumns(View *v, int n)
{
	int length;
	Monitor *m;

	if (!VFEATURES(v, NCOLUMNS))
		return;
	if (!(m = v->curmon))
		return;
	if (n < 1)
		n = DEFNCOLUMNS;
	switch (v->major) {
	case OrientTop:
	case OrientBottom:
		length = m->wa.h;
		if (length / n < 2 * (scr->style.titleheight + scr->style.border))
			return;
		break;
	case OrientLeft:
	case OrientRight:
		length = m->wa.w;
		if (length / n < 2 * (6 * scr->style.titleheight + scr->style.border))
			return;
		break;
	}
	if (v->ncolumns != n) {
		v->ncolumns = n;
		arrange(v);
	}
}

void
decncolumns(View *v, int n)
{
	if (!VFEATURES(v, NCOLUMNS))
		return;
	setncolumns(v, v->ncolumns - n);
}

void
incncolumns(View *v, int n)
{
	if (!VFEATURES(v, NCOLUMNS))
		return;
	setncolumns(v, v->ncolumns + n);
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

static void
reparentclient(Client *c, AScreen *new_scr, int x, int y)
{
	if (new_scr == scr)
		return;
	if (new_scr->managed) {
		View *v;

		/* screens must have the same default depth, visuals and colormaps */

		/* some of what unmanage() does */
		delclient(c);
		ewmh_del_client(c, CauseReparented);
		XDeleteContext(dpy, c->title, context[ScreenContext]);
		XDeleteContext(dpy, c->grips, context[ScreenContext]);
		XDeleteContext(dpy, c->frame, context[ScreenContext]);
		XDeleteContext(dpy, c->win, context[ScreenContext]);
		XUnmapWindow(dpy, c->frame);
		c->is.managed = False;
		scr = new_scr;
		/* some of what manage() does */
		XSaveContext(dpy, c->title, context[ScreenContext], (XPointer) scr);
		XSaveContext(dpy, c->grips, context[ScreenContext], (XPointer) scr);
		XSaveContext(dpy, c->frame, context[ScreenContext], (XPointer) scr);
		XSaveContext(dpy, c->win, context[ScreenContext], (XPointer) scr);
		if (!(v = getview(x, y)))
			v = nearview();
		c->tags = v->seltags;
		addclient(c, True, True);
		XReparentWindow(dpy, c->frame, scr->root, x, y);
		XMoveWindow(dpy, c->frame, x, y);
		XMapWindow(dpy, c->frame);
		c->is.managed = True;
		ewmh_update_net_window_desktop(c);
		if (c->with.struts) {
			ewmh_update_net_work_area();
			updategeom(NULL);
		}
		/* caller must reconfigure client */
	} else {
		Window win = c->win;

		unmanage(c, CauseReparented);
		XReparentWindow(dpy, win, new_scr->root, x, y);
		XMapWindow(dpy, win);
	}
}

static Client *
ontiled(Client *c, View *v, int cx, int cy)
{
	Client *s;

	for (s = nexttiled(scr->clients, v);
	     s && (s == c ||
		   s->c.x > cx || cx > s->c.x + s->c.w ||
		   s->c.y > cy || cy > s->c.y + s->c.h); s = nexttiled(s->next, v)) ;
	return s;
}

static Client *
ondockapp(Client *c, View *v, int cx, int cy)
{
	Client *s;

	for (s = nextdockapp(scr->clients, v);
	     s && (s == c ||
		   s->c.x > cx || cx > s->c.x + s->c.w ||
		   s->c.y > cy || cy > s->c.y + s->c.h); s = nextdockapp(s->next, v)) ;
	return s;
}

static Client *
onstacked(Client *c, View *v, int cx, int cy)
{
	return (c->is.dockapp ? ondockapp(c, v, cx, cy) : ontiled(c, v, cx, cy));
}

static int
findcorner_size(Client *c, int x_root, int y_root)
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
			if (!c->user.sizev)
				from = CursorLeft;
			else if (!c->user.sizeh)
				from = CursorTop;
			else {
				if (dx < dy * 0.4) {
					from = CursorTop;
				} else if (dy < dx * 0.4) {
					from = CursorLeft;
				} else {
					from = CursorTopLeft;
				}
			}
		} else {
			/* top-right */
			if (!c->user.sizev)
				from = CursorRight;
			else if (!c->user.sizeh)
				from = CursorTop;
			else {
				if (dx < dy * 0.4) {
					from = CursorTop;
				} else if (dy < dx * 0.4) {
					from = CursorRight;
				} else {
					from = CursorTopRight;
				}
			}
		}
	} else {
		/* bottom */
		if (x_root < cx) {
			/* bottom-left */
			if (!c->user.sizev)
				from = CursorLeft;
			else if (!c->user.sizeh)
				from = CursorBottom;
			else {
				if (dx < dy * 0.4) {
					from = CursorBottom;
				} else if (dy < dx * 0.4) {
					from = CursorLeft;
				} else {
					from = CursorBottomLeft;
				}
			}
		} else {
			/* bottom-right */
			if (!c->user.sizev)
				from = CursorRight;
			else if (!c->user.sizeh)
				from = CursorBottom;
			else {
				if (dx < dy * 0.4) {
					from = CursorBottom;
				} else if (dy < dx * 0.4) {
					from = CursorRight;
				} else {
					from = CursorBottomRight;
				}
			}
		}
	}
	return from;
}

/** @brief - find corner for moving window
  *
  * There are virtual grips over the window along the edges of the window that
  * change the behaviour of moving:
  * 
  * 1. Left edge grabbed will move horizontally only and only snap to the left
  *    edge of the window.  The left edge includes the left window border and
  *    extends 10% of the width of the window to a maximum of 20 pixels and a
  *    minimum of 5 pixels.
  *
  * 2. Right edge grabbed will move horizontally only and only snap to the right
  *    edge of the window.  The right edge includes the right window border and
  *    extends 10% of the width of the window to a maximum of 20 pixels and a
  *    minimum of 5 pixels.
  *
  * 3. Top edge grabbed will move vertically only and only snap to the top edge
  *    of the window.  The top edge includes the margin (when decorated) or top
  *    border (when not decorated) but never includes the title bar.  The top
  *    edge extends 10% of the height of the window to a maximum of 20 pixels
  *    and a minimum of 5 pixels.
  *
  * 4. Bottom edge grabbed will move vertically only and only snap to the bottom
  *    edge of the window.  The bottom edge includes the bottom border and
  *    extends 10% of the height of the window to a maximum of 20 pixels and a
  *    minimum of 5 pixels.
  *
  * 5. Otherwise, normal move is being performed.
  */
static int
findcorner_move(Client *c, int x_root, int y_root)
{
	int cx, cy, bx, by, bt, bg, from;
	float dx, dy;

	cx = c->c.x + c->c.w / 2;
	cy = c->c.y + c->c.h / 2;

	bt = c->c.y + c->c.t ? (c->c.b + c->c.t) : 0;
	bg = c->c.y + c->c.b + c->c.h - c->c.g;

	dx = (float) c->c.w * 0.10;
	dx = (dx < 5) ? 5 : ((dx > 20) ? 20 : dx);
	dy = (float) c->c.w * 0.10;
	dy = (dy < 5) ? 5 : ((dy > 20) ? 20 : dy);

	from = CursorEvery;

	/* handle really small windows as normal move */
	if ((abs(y_root - cy) > (int) dy) || (abs(x_root - cx) > (int) dx)) {
		if (y_root < cy) {
			/* somewhere toward the top */
			by = bt + (int) dx;
			if (x_root < cx) {
				/* somewhere toward the left */
				bx = c->c.x + c->c.b + (int) dy;
				if ((x_root < bx) && (y_root < by && y_root > bt))
					from = CursorTopLeft;
				else if (y_root < by && y_root > bt)
					from = CursorTop;
				else if (x_root < bx)
					from = CursorLeft;
			} else {
				/* somewhere toward the right */
				bx = c->c.x + c->c.b + c->c.w - (int) dy;
				if ((x_root > bx) && (y_root < by && y_root > bt))
					from = CursorTopRight;
				else if (y_root < by && y_root > bt)
					from = CursorTop;
				else if (x_root > bx)
					from = CursorRight;
			}
		} else {
			/* somewhere toward the bottom */
			by = bg - (int) dx;
			if (x_root < cx) {
				/* somewhere toward the left */
				bx = c->c.x + c->c.b + (int) dy;
				if ((x_root < bx) && (y_root > by && y_root < bg))
					from = CursorBottomLeft;
				else if (y_root > by && y_root < bg)
					from = CursorBottom;
				else if (x_root < bx)
					from = CursorLeft;
			} else {
				/* somewhere toward the right */
				bx = c->c.x + c->c.b + c->c.w - (int) dy;
				if ((x_root > bx) && (y_root > by && y_root < bg))
					from = CursorBottomRight;
				if (y_root > by && y_root < bg)
					from = CursorBottom;
				if (x_root > bx)
					from = CursorRight;
			}
		}
	}
	return (from);
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
move_begin(Client *c, View *v, Bool toggle, int from, IsUnion * was)
{
	Bool isfloater;

	if (!c->user.move)
		return False;

	/* regrab pointer with move cursor */
	XGrabPointer(dpy, scr->root, False, MOUSEMASK, GrabModeAsync,
		     GrabModeAsync, None, cursor[from], user_time);

	isfloater = isfloating(c, v) ? True : False;

	c->is.moveresize = True;
	was->is = 0;
	if (toggle || isfloater) {
		if ((was->full = c->is.full))
			c->is.full = False;
		if ((was->max = c->is.max))
			c->is.max = False;
		if ((was->maxv = c->is.maxv))
			c->is.maxv = False;
		if ((was->maxh = c->is.maxh))
			c->is.maxh = False;
		if ((was->fill = c->is.fill))
			c->is.fill = False;
		if ((was->lhalf = c->is.lhalf))
			c->is.lhalf = False;
		if ((was->rhalf = c->is.rhalf))
			c->is.rhalf = False;
		if ((was->shaded = c->is.shaded))
			c->is.shaded = False;
		if (!isfloater) {
			save(c);	/* tear out at current geometry */
			/* XXX: could this not just be raiseclient(c) ??? */
			detachstack(c);
			attachstack(c, True);
			togglefloating(c);
		} else if (was->is) {
			was->floater = isfloater;
			updatefloat(c, v);
			raiseclient(c);
		} else
			raiseclient(c);
	} else {
		/* can't move tiled in monocle mode */
		if (!c->is.dockapp && !VFEATURES(v, MMOVE)) {
			c->is.moveresize = False;
			return False;
		}
	}
	return True;
}

static Bool
move_cancel(Client *c, View *v, ClientGeometry *orig, IsUnion * was)
{
	Bool wasfloating;

	if (!c->is.moveresize)
		return False;	/* nothing to cancel */

	c->is.moveresize = False;
	wasfloating = was->floater;
	was->floater = False;

	if (isfloating(c, v)) {
		if (was->is) {
			c->is.full = was->full;
			c->is.max = was->max;
			c->is.maxv = was->maxv;
			c->is.maxh = was->maxh;
			c->is.fill = was->fill;
			c->is.lhalf = was->lhalf;
			c->is.rhalf = was->rhalf;
			c->is.shaded = was->shaded;
		}
		if (wasfloating) {
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, orig, False);
			save(c);
			updatefloat(c, v);
		} else
			togglefloating(c);
	} else
		arrange(v);
	return True;
}

static Bool
move_finish(Client *c, View *v, IsUnion * was)
{
	Bool wasfloating;

	if (!c->is.moveresize)
		return False;	/* didn't start or was cancelled */

	c->is.moveresize = False;
	wasfloating = was->floater;
	was->floater = False;

	if (isfloating(c, v)) {
		if (was->is) {
			c->is.full = was->full;
			if (was->maxv ^ was->maxh) {
				c->is.maxv = was->maxv;
				c->is.maxh = was->maxh;
			}
			c->is.fill = was->fill;
			c->is.shaded = was->shaded;
		}
		if (wasfloating) {
			updatefloat(c, v);
		} else
			arrange(v);
	} else
		arrange(v);
	return True;
}

/* TODO: handle movement across EWMH desktops */

Bool
mousemove_from(Client *c, int from, XEvent *e, Bool toggle)
{
	int dx, dy;
	int x_root, y_root;
	View *v, *nv;
	ClientGeometry n = { 0, }, o = { 0, };
	Bool moved = False, isfloater;
	IsUnion was = {.is = 0 };

	x_root = e->xbutton.x_root;
	y_root = e->xbutton.y_root;

	/* The monitor that we clicked in the window from and the monitor in which the
	   window was last laid out can be two different places for floating windows
	   (i.e. a window that overlaps the boundary between two monitors.  Because we
	   handle changing monitors and even changing screens here, we should use the
	   monitor to which it was last associated instead of where it was clicked.  This 
	   is actually important because the monitor it is on might be floating and the
	   monitor the edge was clicked on might be tiled. */

	if (!(v = getview(x_root, y_root)) || (c->cview && v != c->cview))
		if (!(v = c->cview)) {
			CPRINTF(c, "No monitor to move from!\n");
			XUngrabPointer(dpy, e->xbutton.time);
			return moved;
		}

	if (!c->user.floats || c->is.dockapp)
		toggle = False;

	isfloater = (toggle || isfloating(c, v)) ? True : False;

	if (c->is.dockapp || !isfloater)
		from = CursorEvery;

	if (XGrabPointer(dpy, scr->root, False, MOUSEMASK, GrabModeAsync,
			 GrabModeAsync, None, None, e->xbutton.time) != GrabSuccess) {
		CPRINTF(c, "Couldn't grab pointer!\n");
		XUngrabPointer(dpy, e->xbutton.time);
		return moved;
	}

	n = c->c;
	o = c->c;

	for (;;) {
		Bool sl, sr, st, sb;
		int snap;
		Client *s;
		XEvent ev;

		XIfEvent(dpy, &ev, &ismoveevent, (XPointer) c);
		if (ev.type == MotionNotify)
			while (XCheckMaskEvent(dpy, PointerMotionMask, &ev)) ;
		geteventscr(&ev);

		switch (ev.type) {
		case ButtonRelease:
			break;
		case ClientMessage:
			if (ev.xclient.message_type == _XA_NET_WM_MOVERESIZE) {
				if (ev.xclient.data.l[2] == 11) {
					/* _NET_WM_MOVERESIZE_CANCEL */
					CPRINTF(c, "Move cancelled!\n");
					moved = move_cancel(c, v, &o, &was);
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
				if (!(moved = move_begin(c, v, toggle, from, &was))) {
					CPRINTF(c, "Couldn't move client!\n");
					break;
				}
				n = c->c;
				o = c->c;
			}
			nv = getview(ev.xmotion.x_root, ev.xmotion.y_root);
			if (c->is.dockapp || !isfloater) {
				Client *s;

				/* cannot move off monitor when shuffling tiled */
				if (event_scr != scr || nv != v) {
					CPRINTF(c, "Cannot move off monitor!\n");
					continue;
				}
				if ((s = onstacked(c, v, ev.xmotion.x_root,
						   ev.xmotion.y_root))) {
					swapstacked(c, s);
					arrange(v);
				}
				/* move center to new position */
				n = c->c;
				n.x = ev.xmotion.x_root - n.w / 2;
				n.y = ev.xmotion.y_root - n.h / 2;
				DPRINTF("CALLING reconfigure()\n");
				reconfigure(c, &n, False);
				continue;
			}
			switch (from) {
			default:
			case CursorEvery:
				/* allowed to move vertical and horizontal */
				n.x = o.x + dx;
				n.y = o.y + dy;
				/* snap left, top, bottom, right */
				sl = True; st = True; sr = True; sb = True;
				break;
			case CursorTopLeft:
				/* allowed to move vertical and horizontal */
				n.x = o.x + dx;
				n.y = o.y + dy;
				/* snap top and left */
				sl = True; st = True; sr = False; sb = False;
				break;
			case CursorTopRight:
				/* allowed to move vertical and horizontal */
				n.x = o.x + dx;
				n.y = o.y + dy;
				/* snap top and right */
				sl = False; st = True; sr = True; sb = False;
				break;
			case CursorBottomLeft:
				/* allowed to move vertical and horizontal */
				n.x = o.x + dx;
				n.y = o.y + dy;
				/* snap bottom and left */
				sl = True; st = False; sr = False; sb = True;
				break;
			case CursorBottomRight:
				/* allowed to move vertical and horizontal */
				n.x = o.x + dx;
				n.y = o.y + dy;
				/* snap bottom and right */
				sl = False; st = False; sr = True; sb = True;
				break;
			case CursorTop:
				/* only allowed to move vertical */
				n.x = o.x;
				n.y = o.y + dy;
				/* snap top */
				sl = False; st = True; sr = False; sb = False;
				break;
			case CursorBottom:
				/* only allowed to move vertical */
				n.x = o.x;
				n.y = o.y + dy;
				/* snap bottom */
				sl = False; st = False; sr = False; sb = True;
				break;
			case CursorLeft:
				/* only allowed to move horizontal */
				n.x = o.x + dx;
				n.y = o.y;
				/* snap left */
				sl = True; st = False; sr = False; sb = False;
				break;
			case CursorRight:
				/* only allowed to move horizontal */
				n.x = o.x + dx;
				n.y = o.y;
				/* snap right */
				sl = False; st = False; sr = True; sb = False;
				break;
			}
			if (nv && isfloater) {
				Workarea wa, sc;
				
				getworkarea(nv->curmon, &wa);
				sc = nv->curmon->sc;

				if (!(ev.xmotion.state & ControlMask)) {
					if (ev.xmotion.y_root == sc.y && c->user.max) {
						if (!c->is.max || c->is.lhalf || c->is.rhalf) {
							c->is.max = True;
							c->is.lhalf = False;
							c->is.rhalf = False;
							ewmh_update_net_window_state(c);
							DPRINTF("CALLING reconfigure()\n");
							reconfigure(c, &o, False);
							save(c);
							updatefloat(c, v);
							restack();
						}
					} else if (ev.xmotion.x_root == sc.x && c->user.move && c->user.size) {
						if (c->is.max || !c->is.lhalf || c->is.rhalf) {
							c->is.max = False;
							c->is.lhalf = True;
							c->is.rhalf = False;
							ewmh_update_net_window_state(c);
							DPRINTF("CALLING reconfigure()\n");
							reconfigure(c, &o, False);
							save(c);
							updatefloat(c, v);
						}
					} else if (ev.xmotion.x_root == sc.x + sc.w - 1 && c->user.move && c->user.size) {
						if (c->is.max || c->is.lhalf || !c->is.rhalf) {
							c->is.max = False;
							c->is.lhalf = False;
							c->is.rhalf = True;
							ewmh_update_net_window_state(c);
							DPRINTF("CALLING reconfigure()\n");
							reconfigure(c, &o, False);
							save(c);
							updatefloat(c, v);
						}
					} else {
						if (c->is.max || c->is.lhalf || c->is.rhalf) {
							c->is.max = False;
							c->is.lhalf = False;
							c->is.rhalf = False;
							ewmh_update_net_window_state(c);
						}
						if ((snap = scr->options.snap)) {
							int nx2 = n.x + n.w + 2 * n.b;
							int ny2 = n.y + n.h + 2 * n.b;
							int wax2 = wa.x + wa.w;
							int way2 = wa.y + wa.h;
							int scx2 = sc.x + sc.w;
							int scy2 = sc.y + sc.h;
							int waxc = wa.x + wa.w / 2;
							int wayc = wa.y + wa.h / 2;

							if (sl && (abs(n.x - wa.x) < snap)) {
								DPRINTF("snapping left edge to workspace left edge\n");
								n.x += wa.x - n.x;
							} else if (sr && (abs(nx2 - wax2) < snap)) {
								DPRINTF("snapping right edge to workspace right edge\n");
								n.x += wax2 - nx2;
							} else if (sl && (abs(n.x - sc.x) < snap)) {
								DPRINTF("snapping left edge to screen left edge\n");
								n.x += sc.x - n.x;
							} else if (sr && (abs(nx2 - scx2) < snap)) {
								DPRINTF("snapping right edge to screen right edge\n");
								n.x += scx2 - nx2;
							} else if (sl && (abs(n.x - waxc) < snap)) {
								DPRINTF("snapping left edge to workspace center line\n");
								n.x += waxc - n.x;
							} else if (sr && (abs(nx2 - waxc) < snap)) {
								DPRINTF("snapping right edge to workspace center line\n");
								n.x += waxc - nx2;
							} else {
								Bool done = False;

								for (s = event_scr->stack; s && !done; s = s->snext) {
									int sx2 = s->c.x + s->c.w + 2 * s->c.b;
									int sy2 = s->c.y + s->c.h + 2 * s->c.b;

									if (s == c)
										continue;
									if (wind_overlap(n.y, ny2, s->c.y, sy2)) { 
										if (sl && (abs(n.x - sx2) < snap)) {
											CPRINTF(s, "snapping left edge to other window right edge");
											n.x = sx2;
											done = True;
										} else if (sr && (abs(nx2 - s->c.x) < snap)) {
											CPRINTF(s, "snapping right edge to other window left edge");
											n.x = s->c.x - (n.w + 2 * n.b);
											done = True;
										} else
											continue;
										break;
									}
								}
								for (s = event_scr->stack; s && !done; s = s->snext) {
									int sx2 = s->c.x + s->c.w + 2 * s->c.b;
									int sy2 = s->c.y + s->c.h + 2 * s->c.b;

									if (s == c)
										continue;
									if (wind_overlap(n.y, ny2, s->c.y, sy2)) { 
										if (sl && (abs(n.x - s->c.x) < snap)) {
											CPRINTF(s, "snapping left edge to other window left edge");
											n.x = s->c.x;
											done = True;
										} else if (sr && (abs(nx2 - sx2) < snap)) {
											CPRINTF(s, "snapping right edge to other window right edge");
											n.x = sx2 - (n.w + 2 * n.b);
											done = True;
										} else
											continue;
										break;
									}
								}
							}
							if (st && (abs(n.y - wa.y) < snap)) {
								DPRINTF("snapping top edge to workspace top edge\n");
								n.y += wa.y - n.y;
							} else if (sb && (abs(ny2 - way2) < snap)) {
								DPRINTF("snapping bottom edge to workspace bottom edge\n");
								n.y += way2 - ny2;
							} else if (st && (abs(n.y - sc.y) < snap)) {
								DPRINTF("snapping top edge to screen top edge\n");
								n.y += sc.y - n.y;
							} else if (sb && (abs(ny2 - scy2) < snap)) {
								DPRINTF("snapping bottom edge to screen bottom edge\n");
								n.y += scy2 - ny2;
							} else if (st && (abs(n.y - wayc) < snap)) {
								DPRINTF("snapping left edge to workspace center line\n");
								n.y += wayc - n.y;
							} else if (sb && (abs(ny2 - wayc) < snap)) {
								DPRINTF("snapping right edge to workspace center line\n");
								n.y += wayc - ny2;
							} else {
								Bool done;

								for (done = False, s = event_scr->stack; s && !done; s = s->snext) {
									int sx2 = s->c.x + s->c.w + 2 * s->c.b;
									int sy2 = s->c.y + s->c.h + 2 * s->c.b;

									if (s == c)
										continue;
									if (wind_overlap(n.x, nx2, s->c.x, sx2)) {
										if (st && (abs(n.y - sy2) < snap)) {
											CPRINTF(s, "snapping top edge to other window bottom edge");
											n.y = sy2;
											done = True;
										} else if (sb && (abs(ny2 - s->c.y) < snap)) {
											CPRINTF(s, "snapping bottom edge to other window top edge");
											n.y += s->c.y - ny2;
											done = True;
										} else
											continue;
										break;
									}
								}
								for (done = False, s = event_scr->stack; s && !done; s = s->snext) {
									int sx2 = s->c.x + s->c.w + 2 * s->c.b;
									int sy2 = s->c.y + s->c.h + 2 * s->c.b;

									if (s == c)
										continue;
									if (wind_overlap(n.x, nx2, s->c.x, sx2)) {
										if (st && (abs(n.y - s->c.y) < snap)) {
											CPRINTF(s, "snapping top edge to other window top edge");
											n.y = s->c.y;
											done = True;
										} else if (sb && (abs(ny2 - sy2) < snap)) {
											CPRINTF(s, "snapping bottom edge to other window bottom edge");
											n.y += sy2 - ny2;
											done = True;
										} else
											continue;
										break;
									}
								}
							}
						}
					}
				}
			}
			if (event_scr != scr)
				reparentclient(c, event_scr, n.x, n.y);
			if (nv && v != nv) {
				c->tags = nv->seltags;
				ewmh_update_net_window_desktop(c);
				drawclient(c);
				arrange(NULL);
				v = nv;
			}
			if (!isfloater || (!c->is.max && !c->is.lhalf && !c->is.rhalf)) {
				DPRINTF("CALLING reconfigure()\n");
				reconfigure(c, &n, False);
				save(c);
			}
			continue;
		}
		XUngrabPointer(dpy, ev.xmotion.time);
		break;
	}
	if (move_finish(c, v, &was))
		moved = True;
	discardcrossing();
	ewmh_update_net_window_state(c);
	return moved;
}

Bool
mousemove(Client *c, XEvent *e, Bool toggle)
{
	int from;

	if (!c->user.move) {
		XUngrabPointer(dpy, user_time);
		return False;
	}
	from = findcorner_move(c, e->xbutton.x_root, e->xbutton.y_root);
	return mousemove_from(c, from, e, toggle);
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
resize_begin(Client *c, View *v, Bool toggle, int from, IsUnion * was)
{
	Bool isfloater;

	if (!c->user.size)
		return False;

	switch (from) {
	case CursorTop:
	case CursorBottom:
		if (!c->user.sizev)
			return False;
		break;
	case CursorLeft:
	case CursorRight:
		if (!c->user.sizeh)
			return False;
		break;
	default:
		break;
	}

	/* regrab pointer with resize cursor */
	XGrabPointer(dpy, scr->root, False, MOUSEMASK, GrabModeAsync,
		     GrabModeAsync, None, cursor[from], user_time);

	isfloater = isfloating(c, v) ? True : False;

	c->is.moveresize = True;
	was->is = 0;
	if (toggle || isfloater) {
		if ((was->full = c->is.full))
			c->is.full = False;
		if ((was->max = c->is.max))
			c->is.max = False;
		if ((was->maxv = c->is.maxv))
			c->is.maxv = False;
		if ((was->maxh = c->is.maxh))
			c->is.maxh = False;
		if ((was->fill = c->is.fill))
			c->is.fill = False;
		if ((was->lhalf = c->is.lhalf))
			c->is.lhalf = False;
		if ((was->rhalf = c->is.rhalf))
			c->is.rhalf = False;
		if ((was->shaded = c->is.shaded))
			c->is.shaded = False;
		if (!isfloater) {
			save(c);	/* tear out at current geometry */
			/* XXX: could this not just be raiseclient(c) ??? */
			detachstack(c);
			attachstack(c, True);
			togglefloating(c);
		} else if (was->is) {
			was->floater = isfloater;
			updatefloat(c, v);
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
resize_cancel(Client *c, View *v, ClientGeometry *orig, IsUnion * was)
{
	Bool wasfloating;

	if (!c->is.moveresize)
		return False;	/* nothing to cancel */

	c->is.moveresize = False;
	wasfloating = was->floater;
	was->floater = False;

	if (isfloating(c, v)) {
		if (was->is) {
			c->is.full = was->full;
			c->is.max = was->max;
			c->is.maxv = was->maxv;
			c->is.maxh = was->maxh;
			c->is.fill = was->fill;
			c->is.lhalf = was->lhalf;
			c->is.rhalf = was->rhalf;
			c->is.shaded = was->shaded;
		}
		if (wasfloating) {
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, orig, False);
			save(c);
			updatefloat(c, v);
		} else
			togglefloating(c);
	} else
		arrange(v);
	return True;
}

static Bool
resize_finish(Client *c, View *v, IsUnion * was)
{
	Bool wasfloating;

	if (!c->is.moveresize)
		return False;	/* didn't start or was cancelled */

	c->is.moveresize = False;
	wasfloating = was->floater;
	was->floater = False;

	if (isfloating(c, v)) {
		if (was->is) {
			c->is.shaded = was->shaded;
		}
		if (wasfloating)
			updatefloat(c, v);
		else
			arrange(v);
	} else
		arrange(v);
	return True;
}

Bool
mouseresize_from(Client *c, int from, XEvent *e, Bool toggle)
{
	int dx, dy;
	int x_root, y_root;
	View *v, *nv;
	ClientGeometry n = { 0, }, o = { 0, };
	Bool resized = False;
	IsUnion was = {.is = 0 };

	x_root = e->xbutton.x_root;
	y_root = e->xbutton.y_root;

	if (!(v = getview(x_root, y_root)) || (c->cview && v != c->cview))
		if (!(v = c->cview)) {
			XUngrabPointer(dpy, e->xbutton.time);
			return resized;
		}

	if (!c->user.floats || c->is.dockapp)
		toggle = False;

	if (XGrabPointer(dpy, scr->root, False, MOUSEMASK, GrabModeAsync,
			 GrabModeAsync, None, None, e->xbutton.time) != GrabSuccess) {
		XUngrabPointer(dpy, e->xbutton.time);
		return resized;
	}

	o = c->c;
	n = c->c;

	for (;;) {
		Bool sl, st, sr, sb;
		int snap;
		Client *s;
		XEvent ev;

		XIfEvent(dpy, &ev, &isresizeevent, (XPointer) c);
		if (ev.type == MotionNotify)
			while (XCheckMaskEvent(dpy, PointerMotionMask, &ev)) ;
		geteventscr(&ev);

		switch (ev.type) {
		case ButtonRelease:
			break;
		case ClientMessage:
			if (ev.xclient.message_type == _XA_NET_WM_MOVERESIZE) {
				if (ev.xclient.data.l[2] == 11) {
					/* _NET_WM_MOVERESIZE_CANCEL */
					resized = resize_cancel(c, v, &o, &was);
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
			dx = (ev.xmotion.x_root - x_root);
			dy = (ev.xmotion.y_root - y_root);
			pushtime(ev.xmotion.time);
			if (!resized) {
				if (abs(dx) < scr->options.dragdist
				    && abs(dy) < scr->options.dragdist)
					continue;
				if (!(resized = resize_begin(c, v, toggle, from, &was)))
					break;
				o = c->c;
				n = c->c;
			}
			nv = getview(ev.xmotion.x_root, ev.xmotion.y_root);
			switch (from) {
			case CursorTopLeft:
				/* allowed to resize vertical and horizontal */
				n.w = o.w - dx; /* left */
				n.h = o.h - dy; /* top */
				/* edges that move: left, top */
				n.x = o.x + dx;
				n.y = o.y + dy;
				/* snap top and left */
				sl = True; st = True; sr = False; sb = False;
				break;
			case CursorTopRight:
				/* allowed to resize vertical and horizontal */
				n.w = o.w + dx; /* right */
				n.h = o.h - dy; /* top */
				/* edges that move: !left, top */
				n.x = o.x;
				n.y = o.y + dy;
				/* snap top and right */
				sl = False; st = True; sr = True; sb = False;
				break;
			case CursorBottomLeft:
				/* allowed to resize vertical and horizontal */
				n.w = o.w - dx; /* left */
				n.h = o.h + dy; /* bottom */
				/* edges that move: left, !top */
				n.x = o.x + dx;
				n.y = o.y;
				/* snap bottom and left */
				sl = True; st = False; sr = False; sb = True;
				break;
			default:
			case CursorBottomRight:
				/* allowed to resize vertical and horizontal */
				n.w = o.w + dx; /* right */
				n.h = o.h + dy; /* bottom */
				/* edges that move: !left, !top */
				n.x = o.x;
				n.y = o.y;
				/* snap bottom and right */
				sl = False; st = False; sr = True; sb = True;
				break;
			case CursorTop:
				/* only allowed to resize vertical */
				n.w = o.w;
				n.h = o.h - dy; /* top */
				/* edges that move: !left, top */
				n.x = o.x;
				n.y = o.y + dy;
				/* snap top */
				sl = False; st = True; sr = False; sb = False;
				break;
			case CursorBottom:
				/* only allowed to resize vertical */
				n.w = o.w;
				n.h = o.h + dy; /* bottom */
				/* edges that move: !left, !top */
				n.x = o.x;
				n.y = o.y;
				/* snap bottom */
				sl = False; st = False; sr = False; sb = True;
				break;
			case CursorLeft:
				/* only allowed to resize horizontal */
				n.w = o.w - dx; /* left */
				n.h = o.h;
				/* edges that move: left, !top */
				n.x = o.x + dx;
				n.y = o.y;
				/* snap left */
				sl = True; st = False; sr = False; sb = False;
				break;
			case CursorRight:
				/* only allowed to resize horizontal */
				n.w = o.w + dx; /* right */
				n.h = o.h;
				/* edges that move: !left, !top */
				n.x = o.x;
				n.y = o.y;
				/* snap right */
				sl = False; st = False; sr = True; sb = False;
				break;
			}
			if (nv) {
				Workarea wa, sc;

				getworkarea(nv->curmon, &wa);
				sc = nv->curmon->sc;

				if (!(ev.xmotion.state & ControlMask)) {
					if ((snap = scr->options.snap)) {
						int nx2 = n.x + n.w + 2 * c->c.b;
						int ny2 = n.y + n.h + 2 * c->c.b;
						int wax2 = wa.x + wa.w;
						int way2 = wa.y + wa.h;
						int scx2 = sc.x + sc.w;
						int scy2 = sc.y + sc.h;
						int waxc = wa.x + wa.w / 2;
						int wayc = wa.y + wa.h / 2;

						if (sl && (abs(n.x - wa.x) < snap)) {
							DPRINTF("snapping left edge to workspace left edge\n");
							n.w += wa.x - n.x;
						} else if (sr && (abs(nx2 - wax2) < snap)) {
							DPRINTF("snapping right edge to workspace right edge\n");
							n.w += wax2 - nx2;
						} else if (sl && (abs(n.x - sc.x) < snap)) {
							DPRINTF("snapping left edge to screen left edge\n");
							n.w += sc.x - n.x;
						} else if (sr && (abs(nx2 - scx2) < snap)) {
							DPRINTF("snapping right edge to screen right edge\n");
							n.w += scx2 - nx2;
						} else if (sl && (abs(n.x - waxc))) {
							DPRINTF("snapping left edge to workspace center line\n");
							n.w += waxc - n.x;
						} else if (sr && (abs(nx2 - waxc))) {
							DPRINTF("snapping right edge to workspace center line\n");
							n.w += waxc - nx2;
						} else {
							Bool done = False;

							for (s = event_scr->stack; s && !done; s = s->next) {
								int sx2 = s->c.x + s->c.w + 2 * s->c.b;
								int sy2 = s->c.y + s->c.h + 2 * s->c.b;

								if (s == c)
									continue;
								if (wind_overlap(n.y, ny2, s->c.y, sy2)) {
									if (sl && (abs(n.x - sx2) < snap)) {
										CPRINTF(s, "snapping left edge to other window right edge");
										n.w += sx2 - n.x;
										done = True;
									} else if (sr && (abs(nx2 - s->c.x) < snap)) {
										CPRINTF(s, "snapping right edge to other window left edge");
										n.w += s->c.x - nx2;
										done = True;
									} else
										continue;
									break;
								}
							}
							for (s = event_scr->stack; s && !done; s = s->next) {
								int sx2 = s->c.x + s->c.w + 2 * s->c.b;
								int sy2 = s->c.y + s->c.h + 2 * s->c.b;

								if (s == c)
									continue;
								if (wind_overlap(n.y, ny2, s->c.y, sy2)) {
									if (sl && (abs(n.x - s->c.x) < snap)) {
										CPRINTF(s, "snapping left edge to other window left edge");
										n.w += s->c.x - n.x;
										done = True;
									} else if (sr && (abs(nx2 - sx2) < snap)) {
										CPRINTF(s, "snapping right edge to other window right edge");
										n.w += sx2 - nx2;
										done = True;
									} else
										continue;
									break;
								}
							}
						}
						if (st && (abs(n.y - wa.y) < snap)) {
							DPRINTF("snapping top edge to workspace top edge\n");
							n.h += wa.y - n.y;
						} else if (sb && (abs(ny2 - way2) < snap)) {
							DPRINTF("snapping bottom edge to workspace bottom edge\n");
							n.h += way2 - ny2;
						} else if (st && (abs(n.y - sc.y) < snap)) {
							DPRINTF("snapping top edge to screen top edge\n");
							n.h += sc.y - n.y;
						} else if (sb && (abs(ny2 - scy2) < snap)) {
							DPRINTF("snapping bottom edge to screen bottom edge\n");
							n.h += scy2 - ny2;
						} else if (st && (abs(n.y - wayc))) {
							DPRINTF("snapping top edge to workspace center line\n");
							n.h += wayc - n.y;
						} else if (sb && (abs(ny2 - wayc))) {
							DPRINTF("snapping bottom edge to workspace center line\n");
							n.h += wayc - ny2;
						} else {
							Bool done;

							for (done = False, s = event_scr->stack; s && !done; s = s->snext) {
								int sx2 = s->c.x + s->c.w + 2 * s->c.b;
								int sy2 = s->c.y + s->c.h + 2 * s->c.b;

								if (s == c)
									continue;
								if (wind_overlap(n.x, nx2, s->c.x, sx2)) {
									if (st && (abs(n.y - sy2) < snap)) {
										CPRINTF(s, "snapping top edge to other window bottom edge");
										n.h += sy2 - n.y;
										done = True;
									} else if (sb && (abs(ny2 - s->c.y) < snap)) {
										CPRINTF(s, "snapping bottom edge to other window top edge");
										n.h += s->c.y - ny2;
										done = True;
									} else
										continue;
									break;
								}
							}
							for (done = False, s = event_scr->stack; s && !done; s = s->snext) {
								int sx2 = s->c.x + s->c.w + 2 * s->c.b;
								int sy2 = s->c.y + s->c.h + 2 * s->c.b;

								if (s == c)
									continue;
								if (wind_overlap(n.x, nx2, s->c.x, sx2)) {
									if (st && (abs(n.y - s->c.y) < snap)) {
										CPRINTF(s, "snapping top edge to other window top edge");
										n.h += s->c.y - n.y;
										done = True;
									} else if (sb && (abs(ny2 - sy2) < snap)) {
										CPRINTF(s, "snapping bottom edge to other window bottom edge");
										n.h += sy2 - ny2;
										done = True;
									} else
										continue;
									break;
								}
							}
						}
					}
				}
			}
			/* apply constraints */
			constrain(c, &n);
			switch (from) {
			case CursorTopLeft:
				/* edges that move: left, top */
				n.x = o.x + (o.w - n.w);
				n.y = o.y + (o.h - n.h);
				break;
			case CursorTopRight:
				/* edges that move: !left, top */
				n.x = o.x;
				n.y = o.y + (o.h - n.h);
				break;
			case CursorBottomLeft:
				/* edges that move: left, !top */
				n.x = o.x + (o.w - n.w);
				n.y = o.y;
				break;
			default:
			case CursorBottomRight:
				/* edges that move: !left, !top */
				n.x = o.x;
				n.y = o.y;
				break;
			case CursorTop:
				/* edges that move: !left, top */
				n.x = o.x;
				n.y = o.y + (o.h - n.h);
				break;
			case CursorBottom:
				/* edges that move: !left, !top */
				n.x = o.x;
				n.y = o.y;
				break;
			case CursorLeft:
				/* edges that move: left, !top */
				n.x = o.x + (o.w - n.w);
				n.y = o.y;
				break;
			case CursorRight:
				/* edges that move: !left, !top */
				n.x = o.x;
				n.y = o.y;
				break;
			}
			if (n.w < MINWIDTH)
				n.w = MINWIDTH;
			if (n.h < MINHEIGHT)
				n.h = MINHEIGHT;
			DPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &n, False);
			if (isfloating(c, v))
				save(c);
			continue;
		}
		XUngrabPointer(dpy, ev.xmotion.time);
		break;
	}
	if (resize_finish(c, v, &was))
		resized = True;
	discardcrossing();
	ewmh_update_net_window_state(c);
	return resized;
}

Bool
mouseresize(Client *c, XEvent *e, Bool toggle)
{
	int from;

	if (!c->user.size || (!c->user.sizeh && !c->user.sizev)) {
		XUngrabPointer(dpy, user_time);
		return False;
	}
	from = findcorner_size(c, e->xbutton.x_root, e->xbutton.y_root);
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
moveresizeclient(Client *c, int dx, int dy, int dw, int dh, int gravity)
{
	View *v;

	/* FIXME: this just resizes and moves the window, it does not handle changing
	   monitors */

	if (!(dx || dy || dw || dh))
		return;
	if (!(v = clientview(c)))
		return;
	if (!isfloating(c, v) && !c->is.bastard)
		return;
	if (dw || dh) {
		ClientGeometry g;
		int xr, yr;

		g = c->c;
		getreference(&xr, &yr, (Geometry *) &g, gravity);

		if (dw && c->incw && (abs(dw) < c->incw))
			dw = (dw / abs(dw)) * c->incw;
		if (dh && c->inch && (abs(dh) < c->inch))
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
			c->is.lhalf = False;
			c->is.rhalf = False;
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

void
moveresizekb(Client *c, int dx, int dy, int dw, int dh, int gravity)
{
	if (!c)
		return;
	if (!c->user.move) {
		dx = 0;
		dy = 0;
	}
	/* constraints should handle this */
	if (!c->user.sizeh)
		dw = 0;
	if (!c->user.sizev)
		dh = 0;
	moveresizeclient(c, dx, dy, dw, dh, gravity);
}

static void
getplace(Client *c, ClientGeometry *g)
{
	long *s = NULL;
	unsigned long n;

	if ((s = getcard(c->win, _XA_WIN_EXPANDED_SIZE, &n)) && n >= 4) {
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
	g->t = c->c.t;
	g->g = c->c.g;
	g->v = c->c.v;
	if (s)
		XFree(s);
}

static Client *
nextplaced(Client *x, Client *c, View *v)
{
	for (; c && (c == x || c->is.bastard || c->is.dockapp || !c->prog.floats
		     || !isvisible(c, v)); c = c->snext) ;
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

static ClientGeometry *
place_geom(Client *c)
{
	if (c->is.max || c->is.maxv || c->is.maxh || c->is.fill || c->is.full ||
			c->is.lhalf || c->is.rhalf)
		return &c->c;
	return &c->r;
}

static void
place_smart(Client *c, WindowPlacement p, ClientGeometry *g, View *v, Workarea *w)
{
	Client *s;
	ClientGeometry **stack = NULL, **unobs, *e, *o;
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
	for (num = 0, s = nextplaced(c, scr->stack, v); s;
	     num++, s = nextplaced(c, s->snext, v)) {
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
	for (i = 0; i < num && !place_overlap(g, stack[i]); i++) ;
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
			for (j = 0; j < num && !place_overlap(g, stack[j]);
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
			for (j = 0; j < num && !place_overlap(g, stack[j]);
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
			for (j = 0; j < num && !place_overlap(g, stack[j]);
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
			for (j = 0; j < num && !place_overlap(g, stack[j]);
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

static void
place_minoverlap(Client *c, WindowPlacement p, ClientGeometry *g, View *v, Workarea *w)
{
	/* TODO: write this */
}

static void
place_undermouse(Client *c, WindowPlacement p, ClientGeometry *g, View *v, Workarea *w)
{
	int mx, my;

	/* pick a different monitor than the default */
	getpointer(&g->x, &g->y);
	if (!(v = getview(g->x, g->y))) {
		v = closestview(g->x, g->y);
		g->x = v->curmon->mx;
		g->y = v->curmon->my;
	}
	getworkarea(v->curmon, w);

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
place_random(Client *c, WindowPlacement p, ClientGeometry *g, View *v, Workarea *w)
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
	ClientGeometry g = { 0, };
	Workarea w;
	View *v;
	Monitor *m;

	getplace(c, &g);

	/* FIXME: use initial location considering gravity */
	if (!(v = clientview(c)) || !v->curmon)
		v = nearview();
	m = v->curmon;
	g.x += m->sc.x;
	g.y += m->sc.y;

	getworkarea(m, &w);

	switch (p) {
	case ColSmartPlacement:
		place_smart(c, p, &g, v, &w);
		break;
	case RowSmartPlacement:
		place_smart(c, p, &g, v, &w);
		break;
	case MinOverlapPlacement:
		place_minoverlap(c, p, &g, v, &w);
		break;
	case UnderMousePlacement:
		place_undermouse(c, p, &g, v, &w);
		break;
	case CascadePlacement:
		place_smart(c, p, &g, v, &w);
		break;
	case RandomPlacement:
		place_random(c, p, &g, v, &w);
		break;
	}
	c->r.x = g.x;
	c->r.y = g.y;
}

static Bool
getclientstrings(Client *c, char **name, char **clas, char **cmd)
{
	char **argv = NULL;
	int argc = 0;
	char *str = NULL, *nam = NULL, *cls = NULL;
	int i;
	size_t tot = 0;
	XClassHint ch = { NULL, };

	if (getclasshint(c, &ch)) {
		if (ch.res_name) {
			nam = strdup(ch.res_name);
			XFree(ch.res_name);
		}
		if (ch.res_class) {
			cls = strdup(ch.res_class);
			XFree(ch.res_class);
		}
	}
	*name = nam;
	*clas = cls;
	if (getcommand(c, &argv, &argc)) {
		for (i = 0; i < argc; i++) {
			size_t len = strlen(argv[i]);
			const char *p = argv[i], *e = p + len;
			char *s, *q, *tmp;
			int count = 0;

			tmp = ecalloc(2 + 2 * len, sizeof(*str));
			s = tmp;
			q = tmp + 1;
			while (p < e) {
				if (*p == '\'') {
					*q++ = '\\';
					*q++ = *p++;
					count++;
				} else {
					*q++ = *p++;
				}
			}
			if (count) {
				*q++ = '\'';
				*s = '\'';
			} else {
				s++;
			}
			*q = '\0';
			len = strlen(s);
			str = erealloc(str, (tot + len + 2) * sizeof(*str));
			if (tot) {
				strcat(str, " ");
				strcat(str, s);
				tot += len + 1;
			} else {
				strcpy(str, s);
				tot += len;
			}
			free(tmp);
		}
		if (argv)
			XFreeStringList(argv);
	}
	*cmd = str;
	return ((nam || cls || str) ? True : False);
}

static Leaf *
findleaf(Container *n, char *name, char *clas, char *cmd)
{
	Leaf *l = NULL;
	Container *c;

	switch (n->type) {
	case TreeTypeNode:
	case TreeTypeTerm:
		for (c = n->node.children.head; c; c = c->next)
			if ((l = findleaf(c, name, clas, cmd)))
				break;
		break;
	case TreeTypeLeaf:
		if (n->leaf.client.client)
			break;
		if (n->leaf.client.name && name && strcmp(n->leaf.client.name, name))
			break;
		if (n->leaf.client.clas && clas && strcmp(n->leaf.client.clas, clas))
			break;
		if (n->leaf.client.command && cmd && strcmp(n->leaf.client.command, cmd))
			break;
		l = &n->leaf;
		break;
	}
	return l;
}

Container *
adddocknode(Container *t)
{
	Container *n;

	n = ecalloc(1, sizeof(*n));
	n->type = TreeTypeTerm;
	n->view = t->view;
	n->is.dockapp = True;
	switch (t->node.children.pos) {
	default:
	case PositionEast:
		n->term.children.pos = PositionCenter;
		n->term.children.ori = OrientTop;
		break;
	case PositionNorthEast:
		switch (t->node.children.ori) {
		case OrientTop:
		case OrientBottom:
			n->term.children.pos = PositionEast;
			n->term.children.ori = OrientLeft;
			break;
		case OrientLeft:
		case OrientRight:
			n->term.children.pos = PositionNorth;
			n->term.children.ori = OrientTop;
			break;
		}
		break;
	case PositionNorth:
		n->term.children.pos = PositionCenter;
		n->term.children.ori = OrientLeft;
		break;
	case PositionNorthWest:
		switch (t->node.children.ori) {
		case OrientTop:
		case OrientBottom:
			n->term.children.pos = PositionWest;
			n->term.children.ori = OrientLeft;
			break;
		case OrientLeft:
		case OrientRight:
			n->term.children.pos = PositionNorth;
			n->term.children.ori = OrientTop;
			break;
		}
		break;
	case PositionWest:
		n->term.children.pos = PositionCenter;
		n->term.children.ori = OrientTop;
		break;
	case PositionSouthWest:
		switch (t->node.children.ori) {
		case OrientTop:
		case OrientBottom:
			n->term.children.pos = PositionWest;
			n->term.children.ori = OrientLeft;
			break;
		case OrientLeft:
		case OrientRight:
			n->term.children.pos = PositionSouth;
			n->term.children.ori = OrientTop;
			break;
		}
		break;
	case PositionSouth:
		n->term.children.pos = PositionCenter;
		n->term.children.ori = OrientLeft;
		break;
	case PositionSouthEast:
		switch (t->node.children.ori) {
		case OrientTop:
		case OrientBottom:
			n->term.children.pos = PositionEast;
			n->term.children.ori = OrientLeft;
			break;
		case OrientLeft:
		case OrientRight:
			n->term.children.pos = PositionSouth;
			n->term.children.ori = OrientTop;
			break;
		}
		break;
	}
	appnode(t, n);
	return n;
}

static void
adddockapp(Client *c, Bool focusme, Bool raiseme)
{
	Container *t, *n;
	Leaf *l = NULL;
	char *name = NULL, *clas = NULL, *cmd = NULL;

	if (!(t = scr->dock.tree)) {
		DPRINTF("WARNING: no dock tree!\n");
		return;
	}

	if (!(n = (Container *) t->node.children.tail)) {
		DPRINTF("WARNING: no dock node!\n");
		n = adddocknode(t);
	}

	if (getclientstrings(c, &name, &clas, &cmd))
		l = findleaf((Container *) t, name, clas, cmd);
	if (l) {
		DPRINTF("found leaf '%s' '%s' '%s'\n", l->client.name ? : "",
			l->client.clas ? : "", l->client.command ? : "");
	} else {
		DPRINTF("creating leaf\n");
		l = ecalloc(1, sizeof(*l));
		l->type = TreeTypeLeaf;
		l->view = n->view;
		l->is.dockapp = True;
		appleaf(n, l, True);
		DPRINTF("dockapp tree now has %d active children\n", t->node.children.active);
	}
	l->client.client = c;
	l->client.next = c->leaves;
	c->leaves = l;
	if (name) {
		free(l->client.name);
		l->client.name = name;
	}
	if (clas) {
		free(l->client.clas);
		l->client.clas = clas;
	}
	if (cmd) {
		free(l->client.command);
		l->client.command = cmd;
	}
}

void
addclient(Client *c, Bool focusme, Bool raiseme)
{
	View *v;

	CPRINTF(c, "initial geometry c: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->c.w, c->c.h, c->c.x, c->c.y, c->c.b, c->c.t, c->c.g, c->c.v);
	CPRINTF(c, "initial geometry r: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->r.w, c->r.h, c->r.x, c->r.y, c->r.b, c->c.t, c->c.g, c->c.v);
	CPRINTF(c, "initial geometry s: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->s.w, c->s.h, c->s.x, c->s.y, c->s.b, c->c.t, c->c.g, c->c.v);

	if (!c->s.x && !c->s.y && c->prog.move && !c->is.dockapp) {
		/* put it on the monitor startup notification requested if not already
		   placed with its group */
		if (c->monitor && !clientview(c))
			c->tags = scr->monitors[c->monitor - 1].curview->seltags;
		place(c, ColSmartPlacement);
	}

	CPRINTF(c, "placed geometry c: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->c.w, c->c.h, c->c.x, c->c.y, c->c.b, c->c.t, c->c.g, c->c.h);
	CPRINTF(c, "placed geometry r: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->r.w, c->r.h, c->r.x, c->r.y, c->r.b, c->c.t, c->c.g, c->c.h);
	CPRINTF(c, "placed geometry s: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->s.w, c->s.h, c->s.x, c->s.y, c->s.b, c->c.t, c->c.g, c->c.h);

	if (!c->prog.move) {
		int mx, my;
		View *cv;

		/* wnck task bars figure window is on monitor containing center of window 
		 */
		mx = c->s.x + c->s.w / 2 + c->s.b;
		my = c->s.y + c->s.h / 2 + c->s.b;

		if (!(cv = getview(mx, my)))
			cv = closestview(mx, my);
		c->tags = cv->seltags;
	}

	attach(c, scr->options.attachaside);
	attachclist(c);
	attachflist(c, focusme);
	attachstack(c, raiseme);
	ewmh_update_net_client_lists();
	if (c->is.managed)
		ewmh_update_net_window_desktop(c);
	if (c->is.bastard)
		return;
	if (c->is.dockapp) {
		adddockapp(c, focusme, raiseme);
		return;
	}
	if (!(v = c->cview) && !(v = clientview(c)) && !(v = onview(c)))
		return;
	if (v->layout && v->layout->arrange && v->layout->arrange->addclient)
		v->layout->arrange->addclient(c, focusme, raiseme);
}

void
deldockapp(Client *c)
{
	Leaf *l, *ln;
	Container *cp;

	/* leaves leaf in place to accept restarted dock app */
	for (ln = c->leaves; (l = ln);) {
		ln = l->client.next;
		l->client.next = NULL;
		l->client.client = NULL;
		if (!l->is.hidden)
			for (cp = (Container *) l->parent; cp; cp = cp->parent)
				cp->node.children.active--;
	}
	c->leaves = NULL;
}

void
delclient(Client *c)
{
	View *v;

	detach(c);
	detachclist(c);
	detachflist(c);
	detachstack(c);
	if (c->is.dockapp) {
		deldockapp(c);
		return;
	}
	if (!(v = c->cview) && !(v = clientview(c)) && !(v = onview(c)))
		return;
	if (v->layout && v->layout->arrange && v->layout->arrange->delclient)
		v->layout->arrange->delclient(c);
}

void
moveto(Client *c, RelativeDirection position)
{
	Monitor *m;
	View *v;
	Workarea w;
	ClientGeometry g = { 0, };

	if (!c || !c->prog.move || !(v = c->cview))
		return;
	if (!(m = v->curmon))
		return;
	if (!isfloating(c, v))
		return;		/* for now */

	getworkarea(m, &w);
	g = c->c;

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
	reconfigure(c, &g, False);
	save(c);
	discardcrossing();
}

void
moveby(Client *c, RelativeDirection direction, int amount)
{
	Monitor *m;
	View *v;
	Workarea w;
	ClientGeometry g = { 0, };

	if (!c || !c->prog.move || !(v = c->cview))
		return;
	if (!(m = v->curmon))
		return;
	if (!isfloating(c, v))
		return;		/* for now */

	getworkarea(m, &w);
	g = c->c;
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
	reconfigure(c, &g, False);
	save(c);
	discardcrossing();
}

void
snapto(Client *c, RelativeDirection direction)
{
	Monitor *m;
	View *v;
	Workarea w;
	ClientGeometry g = { 0, };
	Client *s, *ox, *oy;
	int min1, max1, edge;

	if (!c || !c->prog.move || !(v = c->cview))
		return;
	if (!(m = v->curmon))
		return;
	if (!isfloating(c, v))
		return;		/* for now */

	getworkarea(m, &w);
	g = c->c;

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
			if (s->cview != v)
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
			if (s->cview != v)
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
			if (s->cview != v)
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
			if (s->cview != v)
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
			if (s->cview != v)
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
			if (s->cview != v)
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
			if (s->cview != v)
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
			if (s->cview != v)
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
			if (s->cview != v)
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
			if (s->cview != v)
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
			if (s->cview != v)
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
			if (s->cview != v)
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
	reconfigure(c, &g, False);
	save(c);
	discardcrossing();
}

void
edgeto(Client *c, int direction)
{
	Monitor *m;
	View *v;
	Workarea w;
	ClientGeometry g = { 0, };

	if (!c || !c->prog.move || !(v = c->cview))
		return;
	if (!(m = v->curmon))
		return;
	if (!isfloating(c, v))
		return;		/* for now */

	getworkarea(m, &w);
	g = c->c;

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
	reconfigure(c, &g, False);
	save(c);
	discardcrossing();
}

void
flipview(Client *c)
{
	View *v;

	if (!c || !(v = c->cview))
		return;
	if (!VFEATURES(v, ROTL))
		return;
	v->major = (v->major + 2) % 4;
	v->minor = (v->minor + 2) % 4;
	arrange(v);
}

void
rotateview(Client *c)
{
	View *v;

	if (!c || !(v = c->cview))
		return;
	if (!VFEATURES(v, ROTL))
		return;
	v->major = (v->major + 1) % 4;
	v->minor = (v->minor + 1) % 4;
	arrange(v);
}

void
unrotateview(Client *c)
{
	View *v;

	if (!c || !(v = c->cview))
		return;
	if (!VFEATURES(v, ROTL))
		return;
	v->major = (v->major + 4 - 1) % 4;
	v->minor = (v->minor + 4 - 1) % 4;
	arrange(v);
}

void
flipzone(Client *c)
{
	View *v;

	if (!c || !(v = c->cview))
		return;
	if (!VFEATURES(v, ROTL) || !VFEATURES(v, NMASTER))
		return;
	v->minor = (v->minor + 2) % 4;
	arrange(v);
}

void
rotatezone(Client *c)
{
	View *v;

	if (!c || !(v = c->cview))
		return;
	if (!VFEATURES(v, ROTL) || !VFEATURES(v, NMASTER))
		return;
	v->minor = (v->minor + 1) % 4;
	arrange(v);
}

void
unrotatezone(Client *c)
{
	View *v;

	if (!c || !(v = c->cview))
		return;
	if (!VFEATURES(v, ROTL) || !VFEATURES(v, NMASTER))
		return;
	v->minor = (v->minor + 4 - 1) % 4;
	arrange(v);
}

void
flipwins(Client *c)
{
	/* FIXME: make this do something */
}

void
rotatewins(Client *c)
{
	View *v;
	Client *s;

	if (!c || !(v = c->cview))
		return;
	if (!VFEATURES(v, ROTL))
		return;
	if ((s = nexttiled(scr->clients, v))) {
		reattach(s, True);
		arrange(v);
		focus(s);
	}
}

void
unrotatewins(Client *c)
{
	View *v;
	Client *last, *s;

	if (!c || !(v = c->cview))
		return;
	if (!VFEATURES(v, ROTL) || !VFEATURES(v, NMASTER))
		return;
	for (last = scr->clients; last && last->next; last = last->next) ;
	if ((s = prevtiled(last, v))) {
		reattach(s, False);
		arrange(v);
		focus(s);
	}
}

void
togglefloating(Client *c)
{
	View *v;

	if (!c || c->is.floater || !c->prog.floats || !(v = c->cview))
		return;
	c->skip.arrange = !c->skip.arrange;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, v);
		arrange(v);
	}
}

void
togglefill(Client *c)
{
	View *v;

	if (!c || (!c->prog.fill && c->is.managed) || !(v = c->cview))
		return;
	c->is.fill = !c->is.fill;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, v);
	}

}

void
togglefull(Client *c)
{
	View *v;

	if (!c || (!c->prog.full && c->is.managed) || !(v = c->cview))
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
		updatefloat(c, v);
		restack();
	}
}

void
togglemax(Client *c)
{
	View *v;

	if (!c || (!c->prog.max && c->is.managed) || !(v = c->cview))
		return;
	c->is.max = !c->is.max;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, v);
		restack();
	}
}

void
togglemaxv(Client *c)
{
	View *v;

	if (!c || (!c->prog.maxv && c->is.managed) || !(v = c->cview))
		return;
	c->is.maxv = !c->is.maxv;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, v);
	}
}

void
togglemaxh(Client *c)
{
	View *v;

	if (!c || (!c->prog.maxh && c->is.managed) || !(v = c->cview))
		return;
	c->is.maxh = !c->is.maxh;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, v);
	}
}

void
togglelhalf(Client *c)
{
	View *v;

	if (!c || ((!c->prog.size || !c->prog.move) && c->is.managed) || !(v = c->cview))
		return;
	if ((c->is.lhalf = !c->is.lhalf))
		c->is.rhalf = 0;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, v);
	}
}

void
togglerhalf(Client *c)
{
	View *v;

	if (!c || ((!c->prog.size || !c->prog.move) && c->is.managed) || !(v = c->cview))
		return;
	if ((c->is.rhalf = !c->is.rhalf))
		c->is.lhalf = 0;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, v);
	}
}

void
toggleshade(Client *c)
{
	View *v;

	if (!c || (!c->prog.shade && c->is.managed) || !(v = c->cview))
		return;
	c->is.shaded = !c->is.shaded;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, v);
	}
}

void
toggledectiled(View *v)
{
	v->dectiled = v->dectiled ? False : True;
	arrange(v);
}

void
zoom(Client *c)
{
	View *v;

	if (!c)
		return;
	if (!(v = c->cview))
		return;
	if (!VFEATURES(v, ZOOM) || c->skip.arrange)
		return;
	if (c == nexttiled(scr->clients, v))
		if (!(c = nexttiled(c->next, v)))
			return;
	reattach(c, False);
	arrange(v);
	focus(c);
}

void
zoomfloat(Client *c)
{
	View *v;

	if (!(v = c->cview))
		return;
	if (isfloating(c, v))
		togglefloating(c);
	else
		zoom(c);
}

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
