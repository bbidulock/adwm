/* See COPYING file for copyright and license details. */

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
		XPRINTF(c, "setting as focused\n");
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
		XPRINTF(c, "setting as selected\n");
		if (c->cview)
			c->cview->lastsel = c;
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
		EPRINTF("ERROR: no leaf\n");
		assert(l != NULL);
	}
	if (l->type != TreeTypeLeaf) {
		EPRINTF("ERROR: not a leaf\n");
		assert(l->type == TreeTypeLeaf);
	}
	if (!(t = l->parent)) {
		XPRINTF("WARNING: attempting to delete detached leaf\n");
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
		EPRINTF("ERROR: l is not a leaf\n");
		assert(l->type == TreeTypeLeaf);
	}
	if (l->parent) {
		XPRINTF("WARNING: attempting to append attached leaf\n");
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
		EPRINTF("ERROR: no parent node\n");
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
		EPRINTF("ERROR: l is not a leaf\n");
		assert(l->type == TreeTypeLeaf);
	}
	if (l->parent) {
		XPRINTF("WARNING: attempting to insert attached leaf\n");
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
		EPRINTF("ERROR: no parent node\n");
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
		EPRINTF("ERROR: attempting to delete non-node\n");
		assert(cc->type == TreeTypeNode);
	}
	if (!(cp = cc->parent)) {
		XPRINTF("WARNING: attempting to delete detached node\n");
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
		XPRINTF("WARNING: attempting to append attached node\n");
		delnode(cc);
	}
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
		XPRINTF("WARNING: attempting to append attached node\n");
		delnode(cc);
	}
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
	if (front || !took || took == c) {
		c->fnext = scr->flist;
		scr->flist = c;
	} else {
		for (s = scr->flist; s && s != took; s = s->fnext) ;
		assert(s == took);
		c->fnext = s->fnext;
		s->fnext = c;
	}
}

static void
attachalist(Client *c, Bool front)
{
	Client *s;

	assert(c && c->anext == NULL);
	if (front || !sel || sel == c) {
		c->anext = scr->alist;
		scr->alist = c;
	} else {
		for (s = scr->alist; s && s != sel; s = s->anext) ;
		assert(s == sel);
		c->anext = s->anext;
		s->anext = c;
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
detachalist(Client *c)
{
	Client **cp;

	for (cp = &scr->alist; *cp && *cp != c; cp = &(*cp)->anext) ;
	assert(*cp == c);
	*cp = c->anext;
	c->anext = NULL;
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
reattachalist(Client *c, Bool front)
{
	detachalist(c);
	attachalist(c, front);
}

void
gavefocus(Client *next)
{
	Client *last = gave;

	gave = next;
	if (next && next != last) {
		reattachalist(next, True);
		reattachflist(next, True);
	}
	if (gave == took)
		gave = NULL;
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
			drawclient(last); /* just for focus */
			if (last != sel)
				XSetWindowBorder(dpy, last->frame, scr->style.color.norm[ColBorder].pixel);
		}
	}
	if (next && next != last) {
		if (!next->is.focused) {
			next->is.focused = True;
			ewmh_update_net_window_state(next);
			drawclient(next); /* just for focus */
			if (next != sel)
				XSetWindowBorder(dpy, next->frame, scr->style.color.focu[ColBorder].pixel);
		}
		reattachflist(next, True);
	}
	if (gave == took)
		gave = NULL;
	setfocused(took);
}

void
tookselect(Client *next)
{
	Client *last = sel;

	sel = next;
	if (last && last != next) {
		if (last->is.selected) {
			last->is.selected = False;
			ewmh_update_net_window_state(last);
		}
	}
	if (next && next != last) {
		if (!next->is.selected) {
			next->is.selected = True;
			ewmh_update_net_window_state(next);
		}
		reattachalist(next, True);
	}
	setselected(sel);
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

static Bool
isdock(Client *c)
{
	return (c && (c->with.struts || c->is.dockapp)) ? True : False;
}

static Bool
isbar(Client *c)
{
	return (c && (isdock(c) || WTCHECK(c, WindowTypeDock))) ? True : False;
}


static void
sloppyfocus(Client *c)
{
	switch (scr->options.focus) {
	case Clk2Focus:
		break;
	case SloppyFloat:
		XPRINTF(c, "FOCUS: sloppy focus\n");
		focus(c);
		break;
	case AllSloppy:
		XPRINTF(c, "FOCUS: sloppy focus\n");
		focus(c);
		break;
	case SloppyRaise:
		XPRINTF(c, "FOCUS: sloppy focus\n");
		focus(c);
		raiseclient(c);	/* probably should not do this here */
		break;
	}
}

/* NOTE: this is where we can delay entering docks when they are autoraised.  We
 * can use motion later to set the focus: need the timestamp from the Event,
 * however.  Or we can select them here, but let the autoraise function delay
 * until some time after the selected/focus time.  */

static Bool
delayedfocus(Client *c, Time t)
{
	View *v;

	if (!(v = c->cview))
		return False;
	if (!isdock(c))
		goto cancel_delay;
	if (isdock(sel))
		goto cancel_delay;
	switch (v->barpos) {
	case StrutsDown:
		if (VFEATURES(v, OVERLAP))
			break;
	case StrutsHide:
		if (!v->strut_time)
			v->strut_time = t;
		/* FIXME: config setting for strut delay */
		if (t >= v->strut_time + scr->options.strutsdelay) {
			v->strut_time = None;
			sloppyfocus(c);
		}
		return True;
	case StrutsOff:
	case StrutsOn:
	default:
		break;
	}
      cancel_delay:
	v->strut_time = None;
	return False;
}

static Bool
wouldfocus(XEvent *e, Client *c)
{
	if (!c || !canselect(c)) {
		XPRINTF(c, "FOCUS: cannot select client.\n");
		return False;
	}
	if (c->skip.focus) {
		XPRINTF(c, "FOCUS: should not focus (nor casually select) client.\n");
		return False;
	}
	if (c->skip.sloppy || scr->options.focus == Clk2Focus) {
		XPRINTF(c, "FOCUS: will not sloppy-focus client.\n");
		return False;
	}
	if (scr->options.focus == SloppyFloat && !isfloating(c, c->cview)) {
		XPRINTF(c, "FOCUS: will not sloppy-focus tiled client.\n");
		return False;
	}
	if (checkfocuslock(c, e->xany.serial)) {
		XPRINTF(c, "FOCUS: cannot focus while focus locked.\n");
		return False;
	}
	return True;
}

Bool
enterclient(XEvent *e, Client *c)
{
	XCrossingEvent *ev = &e->xcrossing;

	if (!wouldfocus(e, c)) {
		XPRINTF(c, "FOCUS: cannot autofocus client.\n");
		return True;
	}
	if (delayedfocus(c, ev->time)) {
		XPRINTF(c, "FOCUS: delaying focus that would raise struts.\n");
		return True;
	}
	sloppyfocus(c);
	return True;
}

Bool
motionclient(XEvent *e, Client *c)
{
	XMotionEvent *ev = &e->xmotion;

	if (wouldfocus(e, c))
		delayedfocus(c, ev->time);
	return True;
}

Bool
configureshapes(Client *c)
{
	XRectangle wind = { -c->c.b, -c->c.b, c->c.w + 2 * c->c.b, c->c.h + 2 * c->c.b };
	XRectangle clip = { 0, 0, c->c.w, c->c.h };
	XRectangle clnt = { 0, c->c.t, c->c.w, c->c.h - (c->c.t + c->c.g) };
	XRectangle titl = { 0, 0, c->c.w, c->c.t };
	XRectangle grip = { 0, c->c.h - c->c.g, c->c.w, c->c.g };
	XRectangle btit = { -c->c.b, -c->c.b, c->c.w + 2 * c->c.b, c->c.t + 2 * c->c.b };
	XRectangle brip = { -c->c.b, c->c.h - c->c.g - c->c.b, c->c.w + 2 * c->c.b, c->c.g + 2 * c->c.b };
	XRectangle deco[2] = { {0,}, };
	unsigned dec = 0;

	if (!einfo[XshapeBase].have)
		return False;

	if (c->with.wshape || c->with.bshape) {
		/* draw titlebar and grips separate */
		XShapeCombineShape(dpy, c->frame, ShapeClip,
				   0, c->c.t, c->win, ShapeBounding, ShapeSet);
		if (c->with.clipping)
			XShapeCombineShape(dpy, c->frame, ShapeClip,
					   0, c->c.t, c->win, ShapeClip, ShapeIntersect);
		deco[0] = titl;
		deco[1] = grip;
		XShapeCombineRectangles(dpy, c->frame, ShapeClip,
					0, 0, deco, 2, ShapeUnion, Unsorted);

		XRectangle *rect;
		int i, count = 0, ordering = Unsorted;

		if ((rect = XShapeGetRectangles(dpy, c->frame, ShapeClip,
						&count, &ordering))) {
			for (i = 0; i < count; i++) {
				rect[i].x -= c->c.b;
				rect[i].y -= c->c.b;
				rect[i].width += 2 * c->c.b;
				rect[i].height += 2 * c->c.b;
			}
			XShapeCombineRectangles(dpy, c->frame, ShapeBounding,
						0, 0, rect, count, ShapeSet, Unsorted);
		} else {
			deco[0] = btit;
			deco[1] = brip;
			XShapeCombineRectangles(dpy, c->frame, ShapeBounding,
						0, 0, deco, 2, ShapeSet, Unsorted);
		}
	} else
	if (c->with.boundary || c->with.clipping) {
		XShapeCombineRectangles(dpy, c->frame, ShapeBounding,
					0, 0, &wind, 1, ShapeSet, Unsorted);
		XShapeCombineRectangles(dpy, c->frame, ShapeBounding,
					0, 0, &clnt, 1, ShapeSubtract, Unsorted);
		XShapeCombineRectangles(dpy, c->frame, ShapeClip,
					0, 0, &clip, 1, ShapeSet, Unsorted);
		if (c->c.t)
			deco[dec++] = titl;
		if (c->c.g)
			deco[dec++] = grip;

		if (dec)
			XShapeCombineRectangles(dpy, c->frame, ShapeClip,
						0, 0, deco, dec, ShapeIntersect,
						Unsorted);
		if (c->with.clipping) {
			XShapeCombineShape(dpy, c->frame, ShapeBounding,
					   0, c->c.t, c->win, ShapeClip, ShapeUnion);
			XShapeCombineShape(dpy, c->frame, ShapeClip,
					   0, c->c.t, c->win, ShapeClip, ShapeUnion);
		} else if (c->with.boundary) {
			XShapeCombineShape(dpy, c->frame, ShapeBounding,
					   0, c->c.t, c->win, ShapeBounding, ShapeUnion);
			XShapeCombineShape(dpy, c->frame, ShapeClip,
					   0, c->c.t, c->win, ShapeBounding, ShapeUnion);
		}
	} else {
		XShapeCombineRectangles(dpy, c->frame, ShapeBounding,
					0, 0, &wind, 1, ShapeSet, Unsorted);
		XShapeCombineRectangles(dpy, c->frame, ShapeClip,
					0, 0, &clip, 1, ShapeSet, Unsorted);
	}
	return True;
}

static void
getdockappgeometry(Client *c, ClientGeometry *n)
{
	c->s.x = n->x + n->b + c->c.x - c->s.b;
	c->s.y = n->y + n->b + c->c.y - c->s.b;
	c->s.w = n->w;
	c->s.h = n->h;
}

#ifdef USE_XCAIRO
static void
reconfigure_cairo(cairo_t *cr, XWindowChanges *wc, unsigned mask)
{
	if (!cr)
		return;
	if ((mask & CWWidth) && (mask & CWHeight))
		cairo_xlib_surface_set_size(cairo_get_target(cr), wc->width, wc->height);
	else if (mask & CWWidth)
		cairo_xlib_surface_set_width(cairo_get_target(cr), wc->width);
	else if (mask & CWHeight)
		cairo_xlib_surface_set_height(cairo_get_target(cr), wc->height);
}

static void
resize_cairo(cairo_t *cr, int w, int h)
{
	if (!cr)
		return;
	cairo_xlib_surface_set_size(cairo_get_target(cr), w, h);
}
#endif				/* USE_XCAIRO */

static void
reconfigure_dockapp(Client *c, const ClientGeometry *n, Bool force)
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
	getdockappgeometry(c, &c->r);
	XMapWindow(dpy, c->frame);	/* not mapped for some reason... */
	wwc.width = c->r.w;
	wwc.height = c->r.h;
	wwc.border_width = c->r.b;
	if (fmask) {
		xtrap_push(1,_WCFMTS(fwc, fmask), _WCARGS(fwc, fmask));
		XConfigureWindow(dpy, c->frame, fmask, &fwc);
		xtrap_pop();
#ifdef USE_XCAIRO
		reconfigure_cairo(c->cctx.frame, &fwc, fmask);
#endif
	}
	if (wmask) {
		xtrap_push(1,_WCFMTS(wwc, wmask), _WCARGS(wwc, wmask));
		XConfigureWindow(dpy, c->icon, wmask, &wwc);
		xtrap_pop();
#ifdef USE_XCAIRO
		reconfigure_cairo(c->cctx.icon, &wwc, wmask);
#endif
	}
	if (force || ((fmask | wmask) && !(wmask & (CWWidth | CWHeight))))
		send_configurenotify(c);
	XSync(dpy, False);
	drawclient(c);
	ewmh_update_net_window_extents(c);
	XSync(dpy, False);
}

static Bool
check_unmapnotify(Display *dpy, XEvent *ev, XPointer arg)
{
	Client *c = (typeof(c)) arg;

	return (ev->type == UnmapNotify && !ev->xunmap.send_event
		&& ev->xunmap.window == c->win && ev->xunmap.event == c->frame);
}

static void
getclientgeometry(Client *c, ClientGeometry *n)
{
	c->s.x = n->x + n->b + n->v - c->s.b;
	c->s.y = n->y + n->b + n->v + n->t - c->s.b;
	c->s.w = n->w - n->v - n->v;
	c->s.h = n->h - n->v - n->t - n->g;
}

/* FIXME: this does not handle moving the window across monitor
 * or desktop boundaries. */

static void
reconfigure(Client *c, const ClientGeometry *n, Bool force)
{
	XWindowChanges wwc = { 0, }, fwc = {
	0,};
	unsigned wmask = 0, fmask = 0;
	Bool tchange = False;		/* titlebar height or presence changed */
	Bool gchange = False;		/* grip height or presense changed */
	Bool vchange = False;		/* all-around grip width or presence changed */
	Bool shaded = False;		/* the window will be shaded */
	int t = c->title ? n->t : 0;
	int g = c->grips ? n->g : 0;
	int v = c->grips ? n->v : 0;

	if (n->w <= 0 || n->h <= 0) {
		EPRINTF(__CFMTS(c) "zero width %d or height %d\n", __CARGS(c), n->w, n->h);
		return;
	}
#if 0
	/* offscreen appearance fixes */
	if (n->x > DisplayWidth(dpy, scr->screen))
		n->x = DisplayWidth(dpy, scr->screen) - n->w - 2 * n->b;
	if (n->y > DisplayHeight(dpy, scr->screen))
		n->y = DisplayHeight(dpy, scr->screen) - n->h - 2 * n->b;
#endif
	XPRINTF("x = %d y = %d w = %d h = %d b = %d t = %d g = %d v = %d\n", n->x, n->y,
		n->w, n->h, n->b, n->t, n->g, n->v);

	if (c->is.dockapp)
		return reconfigure_dockapp(c, n, force);

	if (c->c.x != (fwc.x = n->x)) {
		c->c.x = n->x;
		XPRINTF("frame wc.x = %d\n", fwc.x);
		fmask |= CWX;
	}
	if (c->c.y != (fwc.y = n->y)) {
		c->c.y = n->y;
		XPRINTF("frame wc.y = %d\n", fwc.y);
		fmask |= CWY;
	}
	if (c->s.w != (wwc.width = n->w - 2 * v)) {
		c->s.w = wwc.width;
		XPRINTF("wind  wc.w = %u\n", wwc.width);
		wmask |= CWWidth;
	}
	if (c->c.w != (fwc.width = n->w)) {
		c->c.w = n->w;
		XPRINTF("frame wc.w = %u\n", fwc.width);
		fmask |= CWWidth;
	}
	if (c->s.h != (wwc.height = n->h - v - t - g)) {
		c->s.h = wwc.height;
		XPRINTF("wind  wc.h = %u\n", wwc.height);
		wmask |= CWHeight;
	}
	if (c->c.h != (fwc.height = n->h)) {
		c->c.h = n->h;
		XPRINTF("frame wc.h = %u\n", fwc.height);
		fmask |= CWHeight;
	}
	if (c->c.v != (wwc.x = v)) {
		XPRINTF("wind  wc.x = %d\n", wwc.x);
		wmask |= CWX;
	}
	if (c->c.v + c->c.t != (wwc.y = v + t)) {
		XPRINTF("wind  wc.y = %d\n", wwc.y);
		wmask |= CWY;
	}
	if (c->c.t != t) {
		c->c.t = t;
		tchange = True;
	}
	if (c->c.g != g) {
		c->c.g = g;
		gchange = True;
	}
	if (c->c.v != v) {
		c->c.v = v;
		vchange = True;
	}
	if ((t || v) && (c->is.shaded && (c != sel || !scr->options.autoroll))) {
		fwc.height = t + 2 * v;
		XPRINTF("frame wc.h = %u\n", fwc.height);
		fmask |= CWHeight;
		shaded = True;
	} else {
		fwc.height = n->h;
		XPRINTF("frame wc.h = %u\n", fwc.height);
		fmask |= CWHeight;
		shaded = False;
	}
	if (c->c.b != (fwc.border_width = n->b)) {
		c->c.b = n->b;
		XPRINTF("frame wc.b = %u\n", fwc.border_width);
		fmask |= CWBorderWidth;
	}
	if (fmask) {
		configureshapes(c);
		xtrap_push(1, _WCFMTS(fwc, fmask), _WCARGS(fwc, fmask));
		XConfigureWindow(dpy, c->frame, fmask, &fwc);
		xtrap_pop();
#ifdef USE_XCAIRO
		reconfigure_cairo(c->cctx.frame, &fwc, fmask);
#endif
	}
	getclientgeometry(c, &c->c);
	/* do we send sync request _before_ configure? */
	if ((wmask & (CWWidth | CWHeight)) && !newsize(c, wwc.width, wwc.height, CurrentTime))
		wmask &= ~(CWWidth | CWHeight);
	if (shaded) {
		XEvent ev;

		XUnmapWindow(dpy, c->win);
		XSync(dpy, False);
		XCheckIfEvent(dpy, &ev, &check_unmapnotify, (XPointer) c);
	} else
		XMapWindow(dpy, c->win);
	if (wmask) {
		/* if we are configuring otherwise, also set border to zero */
		xtrap_push(1, _WCFMTS(wwc, wmask | CWBorderWidth), _WCARGS(wwc, wmask | CWBorderWidth));
		XConfigureWindow(dpy, c->win, wmask | CWBorderWidth, &wwc);
		xtrap_pop();
#ifdef USE_XCAIRO
		reconfigure_cairo(c->cctx.win, &wwc, wmask);
#endif
	}
	/* ICCCM Version 2.0, §4.1.5 */
	if (force || ((fmask | wmask) && !(wmask & (CWWidth | CWHeight))))
		send_configurenotify(c);
	XSync(dpy, False);
	if (c->title && (tchange || vchange || ((wmask | fmask) & CWWidth))) {
		/* this is ok for all-around grips */
		if (tchange) {
			if (t)
				XMapWindow(dpy, c->title);
			else
				XUnmapWindow(dpy, c->title);
		}
		if (t && (tchange || vchange || ((wmask | fmask) & CWWidth))) {
			XRectangle r = { v, v, wwc.width, t };

			XMoveResizeWindow(dpy, c->title, r.x, r.y, r.width, r.height);
#ifdef USE_XCAIRO
			resize_cairo(c->cctx.title, r.width, r.height);
#endif
		}
	}
	if (c->tgrip && (vchange || ((wmask | fmask) & (CWWidth|CWHeight)))) {
		if (vchange) {
			if (v) {
				XMapWindow(dpy, c->tgrip);
				XMapWindow(dpy, c->lgrip);
				XMapWindow(dpy, c->rgrip);
			} else {
				XUnmapWindow(dpy, c->tgrip);
				XUnmapWindow(dpy, c->lgrip);
				XUnmapWindow(dpy, c->rgrip);
			}
		}
		if (v && (vchange || ((wmask | fmask) & (CWWidth|CWHeight)))) {
			XRectangle tr = { 0, 0, fwc.width, v };
			XRectangle lr = { 0, 0, v, fwc.height };
			XRectangle rr = { fwc.width - v, 0, v, fwc.height };

			XMoveResizeWindow(dpy, c->tgrip, tr.x, tr.y, tr.width, tr.height);
			XMoveResizeWindow(dpy, c->lgrip, lr.x, lr.y, lr.width, lr.height);
			XMoveResizeWindow(dpy, c->rgrip, rr.x, rr.y, rr.width, rr.height);
#ifdef USE_XCAIRO
			resize_cairo(c->cctx.tgrip, tr.width, tr.height);
			resize_cairo(c->cctx.lgrip, lr.width, lr.height);
			resize_cairo(c->cctx.rgrip, rr.width, rr.height);
#endif
		}

	}
	if (c->grips) {
		if (g && (!shaded || v))
			XMapWindow(dpy, c->grips);
		else
			XUnmapWindow(dpy, c->grips);
		if (g && (gchange || vchange || ((wmask | fmask) & (CWWidth | CWHeight | CWY)))) {
			XRectangle r = { 0, n->h - g, wwc.width, g };

			XMoveResizeWindow(dpy, c->grips, r.x, r.y, r.width, r.height);
#ifdef USE_XCAIRO
			resize_cairo(c->cctx.grips, r.width, r.height);
#endif
		}
	}
	if (((c->title && t) || (c->grips && g) || (c->grips && v)) &&
	    ((tchange && t) || (gchange && g) || (vchange && v)
	     || (wmask & CWWidth)))
		drawclient(c);
	ewmh_update_net_window_extents(c);
	XSync(dpy, False);
}

static Bool
constrain(Client *c, ClientGeometry *g)
{
	int w = g->w, h = g->h;
	Bool ret = False;

	XPRINTF(c, "geometry before constraint: %dx%d+%d+%d:%d[%d,%d]\n",
		g->w, g->h, g->x, g->y, g->b, g->t, g->g);

	/* remove decoration */
	h -= g->t + g->g;

	/* set minimum possible */
	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;

	/* temporarily remove base dimensions */
	w -= c->sh.base_width;
	h -= c->sh.base_height;

	/* adjust for aspect limits */
	if (c->sh.min_aspect.y > 0 && c->sh.max_aspect.y > 0 && c->sh.min_aspect.x > 0 && c->sh.max_aspect.x > 0) {
		if (w * c->sh.max_aspect.y > h * c->sh.max_aspect.x)
			w = h * c->sh.max_aspect.x / c->sh.max_aspect.y;
		else if (w * c->sh.min_aspect.y < h * c->sh.min_aspect.x)
			h = w * c->sh.min_aspect.y / c->sh.min_aspect.x;
	}

	/* adjust for increment value */
	if (c->sh.width_inc)
		w -= w % c->sh.width_inc;
	if (c->sh.height_inc)
		h -= h % c->sh.height_inc;

	/* restore base dimensions */
	w += c->sh.base_width;
	h += c->sh.base_height;

	if (c->sh.min_width > 0 && w < c->sh.min_width)
		w = c->sh.min_width;
	if (c->sh.min_height > 0 && h < c->sh.min_height)
		h = c->sh.min_height;
	if (c->sh.max_width > 0 && w > c->sh.max_width)
		w = c->sh.max_width;
	if (c->sh.max_height > 0 && h > c->sh.max_height)
		h = c->sh.max_height;

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
	XPRINTF(c, "geometry after constraints: %dx%d+%d+%d:%d[%d,%d]\n",
		g->w, g->h, g->x, g->y, g->b, g->t, g->g);
	return ret;
}

static void
save(Client *c)
{
	XPRINTF(c, "%dx%d+%d+%d:%d <= %dx%d+%d+%d:%d\n",
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
	XPRINTF("CALLING: constrain()\n");
	constrain(c, &g);
	XPRINTF("CALLING reconfigure()\n");
	reconfigure(c, &g, False);
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
		XPRINTF("CALLING reconfigure()\n");
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
	g->v = decorate ? ((c->grips && c->has.grips && scr->style.fullgrips) ?  g->g : 0) : 0;
}

static void
getworkarea(Monitor *m, Workarea *w)
{
	Workarea *wa;
	View *v = m->curview;

	switch (v->barpos) {
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
	case StrutsDown:
		if (!VFEATURES(v, OVERLAP)) {
			if (isdock(sel)) {
				if (m->dock.position != DockNone)
					wa = &m->dock.wa;
				else
					wa = &m->wa;
			} else
				wa = &m->sc;
		} else
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
	g = c->r;
	g.b = scr->style.border;
	get_decor(c, v, &g);
	if (c->is.full) {
		calc_full(c, v, &g);
	} else {
		if (c->is.max) {
			calc_max(c, &wa, &g);
		} else if (c->is.lhalf) {
			calc_lhalf(c, &wa, &g);
		} else if (c->is.rhalf) {
			calc_rhalf(c, &wa, &g);
		} else if (c->is.fill) {
			calc_fill(c, v, &wa, &g);
		} else {
			if (c->is.maxv) {
				calc_maxv(c, &wa, &g);
			}
			if (c->is.maxh) {
				calc_maxh(c, &wa, &g);
			}
		}
	}
	if (!c->is.max && !c->is.full) {
		/* TODO: more than just northwest gravity */
		XPRINTF("CALLING: constrain()\n");
		constrain(c, &g);
	}
	reconfigure(c, &g, False);
	if (c->is.max)
		ewmh_update_net_window_fs_monitors(c);
	focuslockclient(NULL);
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
		EPRINTF("ERROR: no node!\n");
		assert(nnew != NULL);
	}
	if (!(nold = nnew->next)) {
		EPRINTF("ERROR: no next node!\n");
		assert(nold != NULL);
	}
	if (!(l = nold->term.children.head)) {
		EPRINTF("ERROR: no leaf node!\n");
		assert(l != NULL);
	}
	if (l->type != TreeTypeLeaf) {
		EPRINTF("ERROR: not a leaf node!\n");
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
		EPRINTF("ERROR: no node!\n");
		assert(nold != NULL);
	}
	if (!(nnew = nold->next)) {
		EPRINTF("ERROR: no next node!\n");
		assert(nnew != NULL);
	}
	if (!(l = nold->term.children.tail)) {
		EPRINTF("ERROR: no leaf node!\n");
		assert(l != NULL);
	}
	if (l->type != TreeTypeLeaf) {
		EPRINTF("ERROR: not a leaf node!\n");
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
	XPRINTF("there are %d dock apps\n", num);
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
		EPRINTF("ERROR: invalid child orientation\n");
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
			EPRINTF("ERROR: invalid childe orientation\n");
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
				XPRINTF("CALLING reconfigure()\n");
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
	XPRINTF("there are %d masters\n", ma.n);
	XPRINTF("there are %d slaves\n", sa.n);

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
	XPRINTF("overlap is %d\n", overlap);

	/* master and slave work area position */
	switch (v->major) {
	case OrientBottom:
		XPRINTF("major orientation is masters bottom(%d)\n", v->major);
		ma.x = wa.x;
		ma.y = wa.y + sa.h;
		sa.x = wa.x;
		sa.y = wa.y;
		ma.y -= overlap;
		ma.h += overlap;
		break;
	case OrientRight:
	default:
		XPRINTF("major orientation is masters right(%d)\n", v->major);
		ma.x = wa.x + sa.w;
		ma.y = wa.y;
		sa.x = wa.x;
		sa.y = wa.y;
		ma.x -= overlap;
		ma.w += overlap;
		break;
	case OrientLeft:
		XPRINTF("major orientation is masters left(%d)\n", v->major);
		ma.x = wa.x;
		ma.y = wa.y;
		sa.x = wa.x + ma.w;
		sa.y = wa.y;
		ma.w += overlap;
		break;
	case OrientTop:
		XPRINTF("major orientation is masters top(%d)\n", v->major);
		ma.x = wa.x;
		ma.y = wa.y;
		sa.x = wa.x;
		sa.y = wa.y + ma.h;
		ma.h += overlap;
		break;
	}
	XPRINTF("master work area %dx%d+%d+%d:%d\n", ma.w, ma.h, ma.x, ma.y, ma.b);
	XPRINTF("slave  work area %dx%d+%d+%d:%d\n", sa.w, sa.h, sa.x, sa.y, sa.b);

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
		XPRINTF("minor orientation is top to bottom(%d)\n", v->minor);
		m.x = ma.x;
		m.y = ma.y;
		break;
	case OrientBottom:
		XPRINTF("minor orientation is bottom to top(%d)\n", v->minor);
		m.x = ma.x;
		m.y = ma.y + ma.h - m.h;
		break;
	case OrientLeft:
		XPRINTF("minor orientation is left to right(%d)\n", v->minor);
		m.x = ma.x;
		m.y = ma.y;
		break;
	case OrientRight:
	default:
		XPRINTF("minor orientation is right to left(%d)\n", v->minor);
		m.x = ma.x + ma.w - m.w;
		m.y = ma.y;
		break;
	}
	XPRINTF("initial master %dx%d+%d+%d:%d\n", m.w, m.h, m.x, m.y, m.b);

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
			XPRINTF(&g, "CALLING reconfigure()\n");
			reconfigure(c, &g, False);
		} else {
			ClientGeometry C = g;

			/* center it where it was before */
			C.x = (c->c.x + c->c.w / 2) - C.w / 2;
			C.y = (c->c.y + c->c.h / 2) - C.h / 2;
			XPRINTF(&C, "CALLING reconfigure()\n");
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
		XPRINTF("slave orientation is top to bottom(%d)\n", v->major);
		s.x = sa.x;
		s.y = sa.y;
		break;
	case OrientLeft:
		XPRINTF("slave orientation is bottom to top(%d)\n", v->major);
		s.x = sa.x;
		s.y = sa.y + sa.h - s.h;
		break;
	case OrientTop:
		XPRINTF("slave orientation is left to right(%d)\n", v->major);
		s.x = sa.x;
		s.y = sa.y;
		break;
	case OrientBottom:
	default:
		XPRINTF("slave orientation is right to left(%d)\n", v->major);
		s.x = sa.x + sa.w - s.w;
		s.y = sa.y;
		break;
	}
	XPRINTF("initial slave  %dx%d+%d+%d:%d\n", s.w, s.h, s.x, s.y, s.b);

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
			XPRINTF(&g, "CALLING reconfigure()\n");
			reconfigure(c, &g, False);
		} else {
			ClientGeometry C = g;

			/* center it where it was before */
			C.x = (c->c.x + c->c.w / 2) - C.w / 2;
			C.y = (c->c.y + c->c.h / 2) - C.h / 2;
			XPRINTF(&C, "CALLING reconfigure()\n");
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
			XPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &n, False);
		} else {
			ClientGeometry C = n;

			/* center it where it was before */
			C.x = (c->c.x + c->c.w / 2) - C.w / 2;
			C.y = (c->c.y + c->c.h / 2) - C.h / 2;
			XPRINTF("CALLING reconfigure()\n");
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
	XPRINTF("work area is %dx%d+%d+%d\n", w.w, w.h, w.x, w.y);
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

		XPRINTF("CALLING reconfigure()\n");
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

	if (v->layout && v->layout->arrange && v->layout->arrange->arrange)
		v->layout->arrange->arrange(v);
	for (c = scr->stack; c; c = c->snext) {
		if ((clientview(c) == v) &&
		    ((!c->is.bastard && !c->is.dockapp && !(c->is.icon || c->is.hidden))
		     || ((c->is.bastard || c->is.dockapp) && v->barpos != StrutsHide))) {
			unban(c, v);
		}
	}

	for (c = scr->stack; c; c = c->snext) {
		if ((clientview(c) == NULL)
		    || (!c->is.bastard && !c->is.dockapp && (c->is.icon || c->is.hidden))
		    || ((c->is.bastard || c->is.dockapp) && v->barpos == StrutsHide)) {
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
static Bool
restack()
{
	StackContext s = { 0, };
	Client *c;

	XPRINTF("%s\n", "RESTACKING: -------------------------------------");
	for (s.n = 0, c = scr->stack; c; c = c->snext, s.n++)
		c->breadcrumb = 0;
	if (!s.n) {
		ewmh_update_net_client_list_stacking();
		return False;
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
	/* 2. Focused windows with state _NET_WM_STATE_FULLSCREEN (but not type Desk) */
	for (s.i = 0; s.i < s.n; s.i++) {
		if (!(c = s.cl[s.i]))
			continue;
		if (WTCHECK(c, WindowTypeDesk))
			continue;
		if (took == c && c->is.full)
			stack_clients(&s, c);
	}
	/* 3. Dockapps when a dockapp is selected and docks when selected. */
	if (isdock(sel)) {
		for (s.i = 0; s.i < s.n; s.i++) {
			if (!(c = s.cl[s.i]))
				continue;
			if (isdock(c))
				stack_clients(&s, c);
		}
	}
	/* 4. Window with type Dock and not state Below and windows with state Above. */
	for (s.i = 0; s.i < s.n; s.i++) {
		if (!(c = s.cl[s.i]))
			continue;
		if (WTCHECK(c, WindowTypeDesk))
			continue;
		if (isdock(c) || c->is.below)
			continue;
		if (WTCHECK(c, WindowTypeDock) || WTCHECK(c, WindowTypeSplash)
		    || c->is.above)
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
		if (isbar(c) || c->is.below)
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
	free(s.cl);
	s.cl = NULL;

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
	free(s.ol);
	s.ol = NULL;
	free(s.sl);
	s.sl = NULL;

	if (!window_stack.members || (window_stack.count != s.n) ||
	    bcmp(window_stack.members, s.wl, s.n * sizeof(*s.wl))) {
		free(window_stack.members);
		window_stack.members = s.wl;
		window_stack.count = s.n;

		XRestackWindows(dpy, s.wl, s.n);

		ewmh_update_net_client_list_stacking();
		return True;
	} else {
		XPRINTF("%s", "No new stacking order\n");
		free(s.wl);
		s.wl = NULL;
		return False;
	}
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
	XPRINTF("%dx%d+%d+%d and %dx%d+%d+%d %s\n", c->w, c->h, c->x, c->y,
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
	if (restack())
		focuslockclient(NULL);
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
		if (restack())
			focuslockclient(NULL);
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
		if (restack())
			focuslockclient(NULL);
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
		focuslockclient(NULL);
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
	/* arrangement		symbol	features				major		minor		*/
	{  &arrangement_FLOAT,	'i',	OVERLAP,				0,		0,		},
	{  &arrangement_TILE,	't',	MWFACT | NMASTER | ZOOM | ROTL | MMOVE,	OrientLeft,	OrientBottom,	},
	{  &arrangement_TILE,	'b',	MWFACT | NMASTER | ZOOM | ROTL | MMOVE,	OrientBottom,	OrientLeft,	},
	{  &arrangement_TILE,	'u',	MWFACT | NMASTER | ZOOM | ROTL | MMOVE,	OrientTop,	OrientRight,	},
	{  &arrangement_TILE,	'l',	MWFACT | NMASTER | ZOOM | ROTL | MMOVE,	OrientRight,	OrientTop,	},
	{  &arrangement_MONO,	'm',	0,					0,		0,		},
	{  &arrangement_FLOAT,	'f',	OVERLAP,				0,		0,		},
	{  &arrangement_GRID,	'g',	NCOLUMNS | ROTL | MMOVE,		OrientLeft,	OrientTop,	},
	{  NULL,		'\0',	0,					0,		0,		}
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
	if (restack())
		focuslockclient(NULL);
}

void
lowerclient(Client *c)
{
	detachstack(c);
	attachstack(c, False);
	if (restack())
		focuslockclient(NULL);
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
		XPRINTF(c, "raising floating client for focus\n");
		raiseclient(c);
	}
}

void
raisetiled(Client *c)
{
	if (!c->is.dockapp && (c->is.bastard || !isfloating(c, c->cview))) {
		XPRINTF(c, "raising non-floating client on focus\n");
		raiseclient(c);
	}
}

void
lowertiled(Client *c)
{
#if 0
	/* NEVER do this! */
	if (!c->is.dockapp && (c->is.bastard || !isfloating(c, c->cview))) {
		XPRINTF(c, "lowering non-floating client on loss of focus\n");
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
tearout(Client *c, int x_root, int y_root)
{
	/* rather than tearing out at the current tiled or maximized/maximus state, tear
	   out at the restore (previously saved) state, but move the restore position so
	   that the pointer will stil be in the same place, proportionally, in the
	   restored window as it was in the current geometry, without moving the pointer. 
	 */
	float pdown = (float) (y_root - c->c.y + c->c.b) / (float) c->c.h;
	float pleft = (float) (x_root - c->c.x + c->c.b) / (float) c->c.w;

	c->r.y = y_root - (int) roundf(pdown * (float) c->r.h);
	c->r.x = x_root - (int) roundf(pleft * (float) c->r.w);
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
		if (c->title)
			XDeleteContext(dpy, c->title, context[ScreenContext]);
		if (c->grips)
			XDeleteContext(dpy, c->grips, context[ScreenContext]);
		if (c->tgrip) {
			XDeleteContext(dpy, c->tgrip, context[ScreenContext]);
			XDeleteContext(dpy, c->lgrip, context[ScreenContext]);
			XDeleteContext(dpy, c->rgrip, context[ScreenContext]);
		}
		XDeleteContext(dpy, c->frame, context[ScreenContext]);
		XDeleteContext(dpy, c->win, context[ScreenContext]);
		XUnmapWindow(dpy, c->frame);
		c->is.managed = False;
		scr = new_scr;
		/* some of what manage() does */
		if (c->title)
			XSaveContext(dpy, c->title, context[ScreenContext], (XPointer) scr);
		if (c->grips)
			XSaveContext(dpy, c->grips, context[ScreenContext], (XPointer) scr);
		if (c->tgrip) {
			XSaveContext(dpy, c->tgrip, context[ScreenContext], (XPointer) scr);
			XSaveContext(dpy, c->lgrip, context[ScreenContext], (XPointer) scr);
			XSaveContext(dpy, c->rgrip, context[ScreenContext], (XPointer) scr);
		}
		XSaveContext(dpy, c->frame, context[ScreenContext], (XPointer) scr);
		XSaveContext(dpy, c->win, context[ScreenContext], (XPointer) scr);
		if (!(v = getview(x, y)))
			v = nearview();
		c->tags = v->seltags;
		addclient(c, True, True, True);
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
			if (!c->can.sizev)
				from = CursorLeft;
			else if (!c->can.sizeh)
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
			if (!c->can.sizev)
				from = CursorRight;
			else if (!c->can.sizeh)
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
			if (!c->can.sizev)
				from = CursorLeft;
			else if (!c->can.sizeh)
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
			if (!c->can.sizev)
				from = CursorRight;
			else if (!c->can.sizeh)
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
  *    extends 10% of the width of the window to a maximum of 40 pixels and a
  *    minimum of 20 pixels.
  *
  * 2. Right edge grabbed will move horizontally only and only snap to the right
  *    edge of the window.  The right edge includes the right window border and
  *    extends 10% of the width of the window to a maximum of 40 pixels and a
  *    minimum of 20 pixels.
  *
  * 3. Top edge grabbed will move vertically only and only snap to the top edge
  *    of the window.  The top edge includes the margin (when decorated) or top
  *    border (when not decorated) but never includes the title bar.  The top
  *    edge extends 10% of the height of the window to a maximum of 40 pixels
  *    and a minimum of 20 pixels.
  *
  * 4. Bottom edge grabbed will move vertically only and only snap to the bottom
  *    edge of the window.  The bottom edge includes the bottom border and
  *    extends 10% of the height of the window to a maximum of 40 pixels and a
  *    minimum of 20 pixels.
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
	dx = (dx < VGRIPMIN) ? VGRIPMIN : ((dx > VGRIPMAX) ? VGRIPMAX : dx);
	dy = (float) c->c.w * 0.10;
	dy = (dy < VGRIPMIN) ? VGRIPMIN : ((dy > VGRIPMAX) ? VGRIPMAX : dy);

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
	case MotionNotify:
	case KeymapNotify:
	case Expose:
	case GraphicsExpose:
	case NoExpose:
	case ClientMessage:
	case ConfigureRequest:
	case MapRequest:
		return True;
	default:
#if 1
		if (einfo[XfixesBase].have && event->type == XFixesDisplayCursorNotify + einfo[XfixesBase].event)
			return True;
#endif
#ifdef SYNC
		if (einfo[XsyncBase].have && event->type == XSyncAlarmNotify + einfo[XsyncBase].event)
			return True;
#endif
#ifdef DAMAGE
		if (einfo[XdamageBase].have && event->type == XDamageNotify + einfo[XdamageBase].event)
			return True;
#endif
#ifdef SHAPE
		if (einfo[XshapeBase].have && event->type == ShapeNotify + einfo[XshapeBase].event)
			return True;
#endif
		return False;
	}
}

static Bool
move_begin(Client *c, View *v, Bool toggle, int from, IsUnion * was, int x_root, int y_root)
{
	Bool isfloater;

	if (!c->can.move)
		return False;

	/* regrab pointer with move cursor */
	XChangeActivePointerGrab(dpy, MOUSEMASK, cursor[from], user_time);

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
			tearout(c, x_root, y_root);
			/* XXX: could this not just be raiseclient(c) ??? */
			detachstack(c);
			attachstack(c, True);
			togglefloating(c);
		} else if (was->is) {
			tearout(c, x_root, y_root);
			was->floater = isfloater;
			updatefloat(c, v);
			raiseclient(c);
		} else {
			raiseclient(c);
		}
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
			XPRINTF("CALLING reconfigure()\n");
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
			EPRINTF(__CFMTS(c) "No monitor to move from!\n", __CARGS(c));
			XUngrabPointer(dpy, e->xbutton.time);
			return moved;
		}

	if (!c->can.floats || c->is.dockapp)
		toggle = False;

	isfloater = (toggle || isfloating(c, v)) ? True : False;

	if (c->is.dockapp || !isfloater)
		from = CursorEvery;

	if (XGrabPointer(dpy, c->frame, False, MOUSEMASK, GrabModeAsync,
			 GrabModeAsync, None, None, e->xbutton.time) != GrabSuccess) {
		EPRINTF(__CFMTS(c) "Couldn't grab pointer!\n", __CARGS(c));
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
		if (ev.type == MotionNotify) {
			XSync(dpy, False);
			while (XCheckMaskEvent(dpy, PointerMotionMask, &ev)) ;
		}
		geteventscr(&ev);

		switch (ev.type) {
		case ButtonRelease:
			break;
		case ClientMessage:
			if (ev.xclient.message_type == _XA_NET_WM_MOVERESIZE) {
				if (ev.xclient.data.l[2] == 11) {
					/* _NET_WM_MOVERESIZE_CANCEL */
					XPRINTF(c, "Move cancelled!\n");
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
				if (!(moved = move_begin(c, v, toggle, from, &was, x_root, y_root))) {
					EPRINTF(__CFMTS(c) "Couldn't move client!\n", __CARGS(c));
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
					EPRINTF(__CFMTS(c) "Cannot move off monitor!\n", __CARGS(c));
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
				XPRINTF("CALLING reconfigure()\n");
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
					if (ev.xmotion.y_root == sc.y && c->can.max) {
						if (!c->is.max || c->is.lhalf || c->is.rhalf) {
							c->is.max = True;
							c->is.lhalf = False;
							c->is.rhalf = False;
							ewmh_update_net_window_state(c);
							XPRINTF("CALLING reconfigure()\n");
							reconfigure(c, &o, False);
							save(c);
							updatefloat(c, v);
							restack();
						}
					} else if (ev.xmotion.x_root == sc.x && c->can.move && c->can.size) {
						if (c->is.max || !c->is.lhalf || c->is.rhalf) {
							c->is.max = False;
							c->is.lhalf = True;
							c->is.rhalf = False;
							ewmh_update_net_window_state(c);
							XPRINTF("CALLING reconfigure()\n");
							reconfigure(c, &o, False);
							save(c);
							updatefloat(c, v);
						}
					} else if (ev.xmotion.x_root == sc.x + sc.w - 1 && c->can.move && c->can.size) {
						if (c->is.max || c->is.lhalf || !c->is.rhalf) {
							c->is.max = False;
							c->is.lhalf = False;
							c->is.rhalf = True;
							ewmh_update_net_window_state(c);
							XPRINTF("CALLING reconfigure()\n");
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
								XPRINTF("snapping left edge to workspace left edge\n");
								n.x += wa.x - n.x;
							} else if (sr && (abs(nx2 - wax2) < snap)) {
								XPRINTF("snapping right edge to workspace right edge\n");
								n.x += wax2 - nx2;
							} else if (sl && (abs(n.x - sc.x) < snap)) {
								XPRINTF("snapping left edge to screen left edge\n");
								n.x += sc.x - n.x;
							} else if (sr && (abs(nx2 - scx2) < snap)) {
								XPRINTF("snapping right edge to screen right edge\n");
								n.x += scx2 - nx2;
							} else if (sl && (abs(n.x - waxc) < snap)) {
								XPRINTF("snapping left edge to workspace center line\n");
								n.x += waxc - n.x;
							} else if (sr && (abs(nx2 - waxc) < snap)) {
								XPRINTF("snapping right edge to workspace center line\n");
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
											XPRINTF(s, "snapping left edge to other window right edge");
											n.x = sx2;
											done = True;
										} else if (sr && (abs(nx2 - s->c.x) < snap)) {
											XPRINTF(s, "snapping right edge to other window left edge");
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
											XPRINTF(s, "snapping left edge to other window left edge");
											n.x = s->c.x;
											done = True;
										} else if (sr && (abs(nx2 - sx2) < snap)) {
											XPRINTF(s, "snapping right edge to other window right edge");
											n.x = sx2 - (n.w + 2 * n.b);
											done = True;
										} else
											continue;
										break;
									}
								}
							}
							if (st && (abs(n.y - wa.y) < snap)) {
								XPRINTF("snapping top edge to workspace top edge\n");
								n.y += wa.y - n.y;
							} else if (sb && (abs(ny2 - way2) < snap)) {
								XPRINTF("snapping bottom edge to workspace bottom edge\n");
								n.y += way2 - ny2;
							} else if (st && (abs(n.y - sc.y) < snap)) {
								XPRINTF("snapping top edge to screen top edge\n");
								n.y += sc.y - n.y;
							} else if (sb && (abs(ny2 - scy2) < snap)) {
								XPRINTF("snapping bottom edge to screen bottom edge\n");
								n.y += scy2 - ny2;
							} else if (st && (abs(n.y - wayc) < snap)) {
								XPRINTF("snapping left edge to workspace center line\n");
								n.y += wayc - n.y;
							} else if (sb && (abs(ny2 - wayc) < snap)) {
								XPRINTF("snapping right edge to workspace center line\n");
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
											XPRINTF(s, "snapping top edge to other window bottom edge");
											n.y = sy2;
											done = True;
										} else if (sb && (abs(ny2 - s->c.y) < snap)) {
											XPRINTF(s, "snapping bottom edge to other window top edge");
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
											XPRINTF(s, "snapping top edge to other window top edge");
											n.y = s->c.y;
											done = True;
										} else if (sb && (abs(ny2 - sy2) < snap)) {
											XPRINTF(s, "snapping bottom edge to other window bottom edge");
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
				ewmh_update_net_window_extents(c);
				arrange(NULL);
				v = nv;
			}
			if (!isfloater || (!c->is.max && !c->is.lhalf && !c->is.rhalf)) {
				XPRINTF("CALLING reconfigure()\n");
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
	focuslockclient(c);
	ewmh_update_net_window_state(c);
	return moved;
}

Bool
mousemove(Client *c, XEvent *e, Bool toggle)
{
	int from;

	if (!c->can.move) {
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
	case MotionNotify:
	case KeymapNotify:
	case Expose:
	case GraphicsExpose:
	case NoExpose:
	case ClientMessage:
	case ConfigureRequest:
	case MapRequest:
		return True;
	default:
#if 1
		if (einfo[XfixesBase].have && event->type == XFixesDisplayCursorNotify + einfo[XfixesBase].event)
			return True;
#endif
#ifdef SYNC
		if (einfo[XsyncBase].have && event->type == XSyncAlarmNotify + einfo[XsyncBase].event)
			return True;
#endif
#ifdef DAMAGE
		if (einfo[XdamageBase].have && event->type == XDamageNotify + einfo[XdamageBase].event)
			return True;
#endif
#ifdef SHAPE
		if (einfo[XshapeBase].have && event->type == ShapeNotify + einfo[XshapeBase].event)
			return True;
#endif
		return False;
	}
}

static Bool
resize_begin(Client *c, View *v, Bool toggle, int from, IsUnion * was, int x_root, int y_root)
{
	Bool isfloater;

	if (!c->can.size)
		return False;

	switch (from) {
	case CursorTop:
	case CursorBottom:
		if (!c->can.sizev)
			return False;
		break;
	case CursorLeft:
	case CursorRight:
		if (!c->can.sizeh)
			return False;
		break;
	default:
		break;
	}

	/* regrab pointer with resize cursor */
	XChangeActivePointerGrab(dpy, MOUSEMASK, cursor[from], user_time);

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
			save(c); /* resize from current geometry */
			/* XXX: could this not just be raiseclient(c) ??? */
			detachstack(c);
			attachstack(c, True);
			togglefloating(c);
		} else if (was->is) {
			save(c); /* resize from current geometry */
			was->floater = isfloater;
			updatefloat(c, v);
			raiseclient(c);
		} else {
			save(c); /* resize from current geometry */
			raiseclient(c);
		}
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
			XPRINTF("CALLING reconfigure()\n");
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

	if (!c->can.floats || c->is.dockapp)
		toggle = False;

	if (XGrabPointer(dpy, c->frame, False, MOUSEMASK, GrabModeAsync,
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
		if (ev.type == MotionNotify) {
			XSync(dpy, False);
			while (XCheckMaskEvent(dpy, PointerMotionMask, &ev)) ;
		}
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
				if (!(resized = resize_begin(c, v, toggle, from, &was, x_root, y_root)))
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
							XPRINTF("snapping left edge to workspace left edge\n");
							n.w += n.x - wa.x;
						} else if (sr && (abs(nx2 - wax2) < snap)) {
							XPRINTF("snapping right edge to workspace right edge\n");
							n.w += wax2 - nx2;
						} else if (sl && (abs(n.x - sc.x) < snap)) {
							XPRINTF("snapping left edge to screen left edge\n");
							n.w += n.x - sc.x;
						} else if (sr && (abs(nx2 - scx2) < snap)) {
							XPRINTF("snapping right edge to screen right edge\n");
							n.w += scx2 - nx2;
						} else if (sl && (abs(n.x - waxc) < snap)) {
							XPRINTF("snapping left edge to workspace center line\n");
							n.w += n.x - waxc;
						} else if (sr && (abs(nx2 - waxc) < snap)) {
							XPRINTF("snapping right edge to workspace center line\n");
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
										XPRINTF(s, "snapping left edge to other window right edge");
										n.w += n.x - sx2;
										done = True;
									} else if (sr && (abs(nx2 - s->c.x) < snap)) {
										XPRINTF(s, "snapping right edge to other window left edge");
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
										XPRINTF(s, "snapping left edge to other window left edge");
										n.w += n.x - s->c.x;
										done = True;
									} else if (sr && (abs(nx2 - sx2) < snap)) {
										XPRINTF(s, "snapping right edge to other window right edge");
										n.w += sx2 - nx2;
										done = True;
									} else
										continue;
									break;
								}
							}
						}
						if (st && (abs(n.y - wa.y) < snap)) {
							XPRINTF("snapping top edge to workspace top edge\n");
							n.h += n.y - wa.y;
						} else if (sb && (abs(ny2 - way2) < snap)) {
							XPRINTF("snapping bottom edge to workspace bottom edge\n");
							n.h += way2 - ny2;
						} else if (st && (abs(n.y - sc.y) < snap)) {
							XPRINTF("snapping top edge to screen top edge\n");
							n.h += n.y - sc.y;
						} else if (sb && (abs(ny2 - scy2) < snap)) {
							XPRINTF("snapping bottom edge to screen bottom edge\n");
							n.h += scy2 - ny2;
						} else if (st && (abs(n.y - wayc) < snap)) {
							XPRINTF("snapping top edge to workspace center line\n");
							n.h += n.y - wayc;
						} else if (sb && (abs(ny2 - wayc) < snap)) {
							XPRINTF("snapping bottom edge to workspace center line\n");
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
										XPRINTF(s, "snapping top edge to other window bottom edge");
										n.h += n.y - sy2;
										done = True;
									} else if (sb && (abs(ny2 - s->c.y) < snap)) {
										XPRINTF(s, "snapping bottom edge to other window top edge");
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
										XPRINTF(s, "snapping top edge to other window top edge");
										n.h += n.y - s->c.y;
										done = True;
									} else if (sb && (abs(ny2 - sy2) < snap)) {
										XPRINTF(s, "snapping bottom edge to other window bottom edge");
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
			XPRINTF("CALLING reconfigure()\n");
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
	focuslockclient(c);
	ewmh_update_net_window_state(c);
	return resized;
}

Bool
mouseresize(Client *c, XEvent *e, Bool toggle)
{
	int from;

	if (!c->can.size || (!c->can.sizeh && !c->can.sizev)) {
		XUngrabPointer(dpy, user_time);
		return False;
	}
	from = findcorner_size(c, e->xbutton.x_root, e->xbutton.y_root);
	return mouseresize_from(c, from, e, toggle);
}

/* Notes from wm-spec-1.5:  If the Application requests a new position (x, y)
 * (and possibly also a new size), the Window Manager calculates a new reference
 * point (ref_x, ref_y), based on the client window's (possibly new) size
 * (width, height), border width (bw) and win_gravity as explained in the table
 * below.  The Window Manager will use the new reference point until the next
 * request for a new position.
 */
static void
getreference(int *xr, int *yr, Geometry *g, ClientGeometry *n, int gravity)
{
	switch (gravity) {
	case UnmapGravity:
	case NorthWestGravity:
	default:
		*xr = n->x;
		*yr = n->y;
		g->x = *xr;
		g->y = *yr;
		break;
	case NorthGravity:
		*xr = n->x + n->b + n->w / 2;
		*yr = n->y;
		g->x = *xr - g->b + g->w / 2;
		g->y = *yr;
		break;
	case NorthEastGravity:
		*xr = n->x + n->b + n->w + n->b;
		*yr = n->y;
		g->x = *xr - g->b - g->w - g->b;
		g->y = *yr;
		break;
	case WestGravity:
		*xr = n->x;
		*yr = n->y + n->b + n->h / 2;
		g->x = *xr;
		g->y = *yr - g->b - g->h / 2;
		break;
	case CenterGravity:
		*xr = n->x + n->b + n->w / 2;
		*yr = n->y + n->b + n->h / 2;
		g->x = *xr - g->b - g->w / 2;
		g->y = *yr - g->b - g->h / 2;
		break;
	case EastGravity:
		*xr = n->x + n->b + n->w + n->b;
		*yr = n->y + n->b + n->h / 2;
		g->x = *xr - g->b - g->w - g->b;
		g->y = *yr - g->b - g->h / 2;
		break;
	case SouthWestGravity:
		*xr = n->x;
		*yr = n->y + n->b + n->h + n->b;
		g->x = *xr;
		g->y = *yr - g->b - g->h - g->b;
		break;
	case SouthGravity:
		*xr = n->x + n->b + n->w / 2;
		*yr = n->y + n->b + n->h + n->b;
		g->x = *xr - g->b - g->w / 2;
		g->y = *yr - g->b - g->h - g->b;
		break;
	case SouthEastGravity:
		*xr = n->x + n->b + n->w + n->b;
		*yr = n->y + n->b + n->h + n->b;
		g->x = *xr - g->b - g->w - g->b;
		g->y = *yr - g->b - g->h - g->b;
		break;
	case StaticGravity:
		*xr = n->x + n->v + n->b;
		*yr = n->y + n->v + n->t + n->b;
		g->x = *xr - g->b;
		g->y = *yr - g->b;
		break;
	}
}

static void
getframereference(Client *c, int *xr, int *yr, Geometry *g, int gravity)
{
	if (gravity == 0)
		gravity = c->sh.win_gravity;

	/* border that client expects */
	g->b = c->s.b;
#if 0
	/* top left outer corner of client */
	g->x = c->c.x + c->c.b + c->c.v - g->b;
	g->y = c->c.y + c->c.b + c->c.v + c->c.t - g->b;
#endif
	/* client inner width and height from frame */
	g->w = c->c.w - c->c.v - c->c.v;
	g->h = c->c.h - c->c.v - c->c.t - c->c.g;

	getreference(xr, yr, g, &c->c, gravity);
}

static void
putreference(int xr, int yr, ClientGeometry *g, int gravity)
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
		g->x = xr - g->b - g->w - g->b;
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
		g->x = xr - g->b - g->w - g->b;
		g->y = yr - g->b - g->h / 2;
		break;
	case SouthWestGravity:
		g->x = xr;
		g->y = yr - g->b - g->h - g->b;
		break;
	case SouthGravity:
		g->x = xr - g->b - g->w / 2;
		g->y = yr - g->b - g->h - g->b;
		break;
	case SouthEastGravity:
		g->x = xr - g->b - g->w - g->b;
		g->y = yr - g->b - g->h - g->b;
		break;
	case StaticGravity:
		g->x = xr - g->v - g->b;
		g->y = yr - g->v - g->t - g->b;
		break;
	}
}

static void
putframereference(Client *c, int xr, int yr, Geometry *g, ClientGeometry *n, int gravity)
{
	if (gravity == 0)
		gravity = c->sh.win_gravity;

	/* border that client expects */
	c->s.b = g->b;

	*n = c->c;
	/* frame inner width and height from client */
	n->w = n->v + g->w + n->v;
	n->h = n->v + n->t + g->h + n->g;

	/* no move, size change only, reference point stays the same */
	putreference(xr, yr, n, gravity);
}

static void
setreference(int xr, int yr, Geometry *g, ClientGeometry *n, int gravity)
{
	switch (gravity) {
	case UnmapGravity:
	case NorthWestGravity:
	default:
		xr = g->x;
		yr = g->y;
		n->x = xr;
		n->y = yr;
		break;
	case NorthGravity:
		xr = g->x + g->b + g->w / 2;
		yr = g->y;
		n->x = xr - n->b + n->w / 2;
		n->y = yr;
		break;
	case NorthEastGravity:
		xr = g->x + g->b + g->w + g->b;
		yr = g->y;
		n->x = xr - n->b - n->w - n->b;
		n->y = yr;
		break;
	case WestGravity:
		xr = g->x;
		yr = g->y + g->b + g->h / 2;
		n->x = xr;
		n->y = yr - n->b - n->h / 2;
		break;
	case CenterGravity:
		xr = g->x + g->b + g->w / 2;
		yr = g->y + g->b + g->h / 2;
		n->x = xr - n->b - n->w / 2;
		n->y = yr - n->b - n->h / 2;
		break;
	case EastGravity:
		xr = g->x + g->b + g->w + g->b;
		yr = g->y + g->b + g->h / 2;
		n->x = xr - n->b - n->w - n->b;
		n->y = yr - n->b - n->h / 2;
		break;
	case SouthWestGravity:
		xr = g->x;
		yr = g->y + g->b + g->h + g->b;
		n->x = xr;
		n->y = yr - n->b - n->h - n->b;
		break;
	case SouthGravity:
		xr = g->x + g->b + g->w / 2;
		yr = g->y + g->b + g->h + g->b;
		n->x = xr - n->b - n->w / 2;
		n->y = yr - n->b - n->h - n->b;
		break;
	case SouthEastGravity:
		xr = g->x + g->b + g->w + g->b;
		yr = g->y + g->b + g->h + g->b;
		n->x = xr - n->b - n->w - n->b;
		n->y = yr - n->b - n->h - n->b;
		break;
	case StaticGravity:
		xr = g->x + g->b;
		yr = g->y + g->b;
		n->x = xr - n->v - n->b;
		n->y = yr - n->v - n->t - n->b;
		break;
	}
}

static void
setframereference(Client *c, int xr, int yr, Geometry *g, ClientGeometry *n, int gravity)
{
	if (gravity == 0)
		gravity = c->sh.win_gravity;

	/* border that client expects */
	c->s.b = g->b;

	*n = c->c;
	/* frame inner width and height from client */
	n->w = n->v + g->w + n->v;
	n->h = n->v + n->t + g->h + n->g;

	/* move and possibly size change, reference point moves */
	setreference(xr, yr, g, n, gravity);
}

static void
getnotifygeometry(Client *c, Geometry *g)
{
	if (c->is.dockapp) {
		/* dock apps should not be configuring their icon windows, unless they
		   are also their top-level window */
		g->x = c->r.x + c->c.x - c->s.b;
		g->y = c->r.y + c->c.y - c->s.b;
		g->w = c->r.w;
		g->h = c->r.h;
		g->b = c->r.b;
	} else {
		/* Notes from wm-spec-1.5: When generating synthetic ConfigureNotify
		   events, the position given MUST be the [outer?] top-left corner of the 
		   client window in relation to the origin of the root window (i.e.,
		   ignoring win_gravity) (ICCCM Version 2.0 §4.2.3) */
		/* x position of frame plus frame border plus frame grip minus client
		   border (saved static border) */
		g->x = c->c.x + c->c.b + c->c.v - c->s.b;
		/* y position of frame plus frame border plus frame grip plus frame
		   titlebar minus client border (saved static border). */
		g->y = c->c.y + c->c.b + c->c.v + c->c.t - c->s.b;
		/* inner width of frame minus two grip widths */
		g->w = c->c.w - c->c.v - c->c.v;
		/* inner height of frame minus two grips minus titlebar height */
		g->h = c->c.h - c->c.v - c->c.t - c->c.g;
		/* saved static border */
		g->b = c->s.b;	/* client expected border */
	}
}

void
send_configurenotify(Client *c)
{
	XConfigureEvent ev = { 0, };
	Geometry g = { 0, };
	Window above = c->snext ? c->snext->win : None;

	getnotifygeometry(c, &g);
	ev.type = ConfigureNotify;
	ev.display = dpy;
	ev.event = c->win;
	ev.window = c->win;
	ev.x = g.x;
	ev.y = g.y;
	if (c->sync.waiting) {
		ev.width = c->sync.w;
		ev.height = c->sync.h;
	} else {
		ev.width = g.w;
		ev.height = g.h;
	}
	ev.border_width = g.b;
	ev.above = above;
	ev.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *) &ev);
}

Bool
configureclient(Client *c, XEvent *e, int gravity)
{
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	Bool notify = False, move = False, resize = False, border = False,
	    restack = False;
	View *v;

	XPRINTF(c, "request to configure client\n");
	if (ev->value_mask & CWX)
		XPRINTF("x = %d\n", ev->x);
	if (ev->value_mask & CWY)
		XPRINTF("y = %d\n", ev->y);
	if (ev->value_mask & CWWidth)
		XPRINTF("w = %d\n", ev->width);
	if (ev->value_mask & CWHeight)
		XPRINTF("h = %d\n", ev->height);
	if (ev->value_mask & CWBorderWidth)
		XPRINTF("b = %d\n", ev->border_width);
	if (ev->value_mask & CWStackMode)
		XPRINTF("above = 0x%lx\n", (ev->value_mask & CWSibling) ? ev->above : 0UL);
	if (!(v = c->cview ? : selview())) {
		EPRINTF(__CFMTS(c) "refusing to reconfigure client\n", __CARGS(c));
		notify = True;
	} else if (c->is.dockapp || (!c->is.bastard && !isfloating(c, v))) {
		EPRINTF(__CFMTS(c) "refusing to reconfigure client\n", __CARGS(c));
		notify = True;
	} else {
		Client *o = NULL;
		Geometry g = { 0, };
		ClientGeometry n = { 0, };
		unsigned may, will;
		int stack_mode = Above;
		int xr, yr;

		may = CWX | CWY | CWWidth | CWHeight | CWBorderWidth | CWSibling |
		    CWStackMode;

		/* another approach instead of refusing under these conditions is to
		   cancel the condition when we would have refused them */
		if (c->is.max || c->is.lhalf || c->is.rhalf || c->is.fill || c->is.full
		    || c->is.moveresize)
			may &= ~(CWX | CWY | CWWidth | CWHeight);
		if (c->is.maxv)
			may &= ~(CWY | CWWidth);
		if (c->is.maxh)
			may &= ~(CWX | CWWidth);

		will = ev->value_mask & may;

		/* get current client geometry as seen by client */
		getframereference(c, &xr, &yr, &g, gravity);

		if (will & CWX) {
			g.x = ev->x;
			move = True;
		}
		if (will & CWY) {
			g.y = ev->y;
			move = True;
		}
		if (will & CWWidth) {
			g.w = ev->width;
			resize = True;
		}
		if (will & CWHeight) {
			g.h = ev->height;
			resize = True;
		}
		if (will & CWBorderWidth) {
			g.b = ev->border_width;
			border = True;
		}

		if (will & CWSibling) {
			if (ev->above && !(o = getclient(ev->above, ClientAny)))
				will &= ~(CWSibling | CWStackMode);
			else
				restack = True;
		}
		if (will & CWStackMode) {
			stack_mode = ev->detail;
			restack = True;
		}
		if ((move || restack) && !(resize || border))
			notify = True;
		if (!move && !restack && !resize && !border)
			notify = True;
		if (resize || border)
			notify = False;

		if (restack)
			restack_client(c, stack_mode, o);
		if (move) {
			XPRINTF(&c->c, "before setframereference\n");
			setframereference(c, xr, yr, &g, &n, gravity);
			XPRINTF(&n, "after setframereference\n");
			reconfigure(c, &n, notify);
			return True;
		} else {
			XPRINTF(&c->c, "before putframereference\n");
			putframereference(c, xr, yr, &g, &n, gravity);
			XPRINTF(&n, "after putframereference\n");
			reconfigure(c, &n, notify);
			return True;
		}
	}
	if (notify) {
		if (ev->value_mask & CWBorderWidth)
			c->s.b = ev->border_width;
		send_configurenotify(c);
	}
	return True;
}

void
moveresizeclient(Client *c, int dx, int dy, int dw, int dh, int gravity)
{
	XConfigureRequestEvent cev = { 0, };

	/* FIXME: this just resizes and moves the window, it does not handle changing
	   monitors */
	if (!(dx || dy || dw || dh))
		return;
	cev.value_mask = 0;
	if (dw) {
		if (dw && c->sh.width_inc && (abs(dw) < c->sh.width_inc))
			dw = (dw / abs(dw)) * c->sh.width_inc;
		if (dw) {
			cev.value_mask |= CWWidth;
			cev.width = c->s.w + dw;
		}
	}
	if (dh) {
		if (dh && c->sh.height_inc && (abs(dh) < c->sh.height_inc))
			dh = (dh / abs(dh)) * c->sh.height_inc;
		if (dh) {
			cev.value_mask |= CWHeight;
			cev.width = c->s.h + dh;
		}
	}
	if (dx) {
		cev.value_mask |= CWX;
		cev.x = c->s.x + dx;
	}
	if (dy) {
		cev.value_mask |= CWY;
		cev.y = c->s.y + dy;
	}
	XPRINTF("calling configureclient for moveresizeclient action\n");
	configureclient(c, (XEvent *) &cev, gravity);
}

void
moveresizekb(Client *c, int dx, int dy, int dw, int dh, int gravity)
{
	if (!c)
		return;
	/* moveresizeclient should handle c->can checks */
	if (!c->can.move) {
		dx = 0;
		dy = 0;
	}
	/* constraints should handle this */
	if (!c->can.size) {
		dw = 0;
		dh = 0;
	}
	if (!c->can.sizeh)
		dw = 0;
	if (!c->can.sizev)
		dh = 0;
	if (!gravity)
		gravity = c->sh.win_gravity;
	if (dx || dy || dw || dh)
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
		/* original supplied or static geometry */
		g->x = c->u.x;
		g->y = c->u.y;
		g->w = c->u.w;
		g->h = c->u.h;
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
	for (; c && (c == x || c->is.bastard || c->is.dockapp || !c->can.floats
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
	XPRINTF("There are %d stacked windows\n", num);

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

	XPRINTF("There are %d unoccluded windows\n", j);

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
		XPRINTF("sorting top to bottom, left to right\n");
		qsort(stack, num, sizeof(*stack), &qsort_t2b_l2r);
		break;
	case ColSmartPlacement:
		/* sort left to right, top to bottom */
		XPRINTF("sorting left to right, top to bottom\n");
		qsort(stack, num, sizeof(*stack), &qsort_l2r_t2b);
		break;
	case CascadePlacement:
	default:
		XPRINTF("sorting top left to bottom right\n");
		qsort(stack, num, sizeof(*stack), &qsort_cascade);
		break;
	}
	XPRINTF("Sort result:\n");
	for (i = 0; i < num; i++) {
		e = stack[i];
		XPRINTF("%d: %dx%d+%d+%d\n", i, e->w, e->h, e->x, e->y);
	}

	switch (p) {
	case RowSmartPlacement:
		XPRINTF("trying RowSmartPlacement\n");
		/* try below, right each window */
		/* below */
		for (i = 0; i < num; i++) {
			e = stack[i];
			XPRINTF("below: %dx%d+%d+%d this\n", e->w, e->h, e->x, e->y);
			g->x = e->x;
			g->y = e->y + e->h;
			if (g->x < w->x || g->y < w->y || g->x + g->w > w->x + w->w
			    || g->y + g->h > w->y + w->h) {
				XPRINTF("below: %dx%d+%d+%d outside\n", g->w, g->h, g->x,
					g->y);
				continue;
			}
			for (j = 0; j < num && !place_overlap(g, stack[j]);
			     j++) ;
			if (j == num) {
				XPRINTF("below: %dx%d+%d+%d good\n", g->w, g->h, g->x,
					g->y);
				free(stack);
				return;
			}
			XPRINTF("below: %dx%d+%d+%d no good\n", g->w, g->h, g->x, g->y);
		}
		/* right */
		for (i = 0; i < num; i++) {
			e = stack[i];
			XPRINTF("right: %dx%d+%d+%d this\n", e->w, e->h, e->x, e->y);
			g->x = e->x + e->w;
			g->y = e->y;
			if (g->x < w->x || g->y < w->y || g->x + g->w > w->x + w->w
			    || g->y + g->h > w->y + w->h) {
				XPRINTF("right: %dx%d+%d+%d outside\n", g->w, g->h, g->x,
					g->y);
				continue;
			}
			for (j = 0; j < num && !place_overlap(g, stack[j]);
			     j++) ;
			if (j == num) {
				XPRINTF("right: %dx%d+%d+%d good\n", g->w, g->h, g->x,
					g->y);
				free(stack);
				return;
			}
			XPRINTF("right: %dx%d+%d+%d no good\n", g->w, g->h, g->x, g->y);
		}
		XPRINTF("defaulting to CascadePlacement\n");
		break;
	case ColSmartPlacement:
		XPRINTF("trying ColSmartPlacement\n");
		/* try right, below each window */
		/* right */
		for (i = 0; i < num; i++) {
			e = stack[i];
			XPRINTF("right: %dx%d+%d+%d this\n", e->w, e->h, e->x, e->y);
			g->x = e->x + e->w;
			g->y = e->y;
			if (g->x < w->x || g->y < w->y || g->x + g->w > w->x + w->w
			    || g->y + g->h > w->y + w->h) {
				XPRINTF("right: %dx%d+%d+%d outside\n", g->w, g->h, g->x,
					g->y);
				continue;
			}
			for (j = 0; j < num && !place_overlap(g, stack[j]);
			     j++) ;
			if (j == num) {
				XPRINTF("right: %dx%d+%d+%d good\n", g->w, g->h, g->x,
					g->y);
				free(stack);
				return;
			}
			XPRINTF("right: %dx%d+%d+%d no good\n", g->w, g->h, g->x, g->y);
		}
		/* below */
		for (i = 0; i < num; i++) {
			e = stack[i];
			XPRINTF("below: %dx%d+%d+%d this\n", e->w, e->h, e->x, e->y);
			g->x = e->x;
			g->y = e->y + e->h;
			if (g->x < w->x || g->y < w->y || g->x + g->w > w->x + w->w
			    || g->y + g->h > w->y + w->h) {
				XPRINTF("below: %dx%d+%d+%d outside\n", g->w, g->h, g->x,
					g->y);
				continue;
			}
			for (j = 0; j < num && !place_overlap(g, stack[j]);
			     j++) ;
			if (j == num) {
				XPRINTF("below: %dx%d+%d+%d good\n", g->w, g->h, g->x,
					g->y);
				free(stack);
				return;
			}
			XPRINTF("below: %dx%d+%d+%d no good\n", g->w, g->h, g->x, g->y);
		}
		XPRINTF("defaulting to CascadePlacement\n");
		break;
	default:
		XPRINTF("trying CascadePlacement\n");
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

	putreference(g->x, g->y, g, c->sh.win_gravity);

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

	if (c->ch.res_name || c->ch.res_class) {
		if (c->ch.res_name)
			nam = strdup(c->ch.res_name);
		if (c->ch.res_class)
			cls = strdup(c->ch.res_class);
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
adddockapp(Client *c, Bool choseme, Bool focusme, Bool raiseme)
{
	Container *t, *n;
	Leaf *l = NULL;
	char *name = NULL, *clas = NULL, *cmd = NULL;

	if (!(t = scr->dock.tree)) {
		XPRINTF("WARNING: no dock tree!\n");
		return;
	}

	if (!(n = (Container *) t->node.children.tail)) {
		XPRINTF("WARNING: no dock node!\n");
		n = adddocknode(t);
	}

	if (getclientstrings(c, &name, &clas, &cmd))
		l = findleaf((Container *) t, name, clas, cmd);
	if (l) {
		XPRINTF("found leaf '%s' '%s' '%s'\n", l->client.name ? : "",
			l->client.clas ? : "", l->client.command ? : "");
	} else {
		XPRINTF("creating leaf\n");
		l = ecalloc(1, sizeof(*l));
		l->type = TreeTypeLeaf;
		l->view = n->view;
		l->is.dockapp = True;
		appleaf(n, l, True);
		XPRINTF("dockapp tree now has %d active children\n", t->node.children.active);
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
addclient(Client *c, Bool choseme, Bool focusme, Bool raiseme)
{
	View *v;

	XPRINTF(c, "initial geometry c: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->c.w, c->c.h, c->c.x, c->c.y, c->c.b, c->c.t, c->c.g, c->c.v);
	XPRINTF(c, "initial geometry r: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->r.w, c->r.h, c->r.x, c->r.y, c->r.b, c->c.t, c->c.g, c->c.v);
	XPRINTF(c, "initial geometry s: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->s.w, c->s.h, c->s.x, c->s.y, c->s.b, c->c.t, c->c.g, c->c.v);
	XPRINTF(c, "initial geometry u: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->u.w, c->u.h, c->u.x, c->u.y, c->u.b, c->c.t, c->c.g, c->c.v);

	if (!c->u.x && !c->u.y && c->can.move && !c->is.dockapp) {
		/* put it on the monitor startup notification requested if not already
		   placed with its group */
		if (c->monitor && !clientview(c))
			c->tags = scr->monitors[c->monitor - 1].curview->seltags;
		place(c, ColSmartPlacement);
	}

	XPRINTF(c, "placed geometry c: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->c.w, c->c.h, c->c.x, c->c.y, c->c.b, c->c.t, c->c.g, c->c.h);
	XPRINTF(c, "placed geometry r: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->r.w, c->r.h, c->r.x, c->r.y, c->r.b, c->c.t, c->c.g, c->c.h);
	XPRINTF(c, "placed geometry s: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->s.w, c->s.h, c->s.x, c->s.y, c->s.b, c->c.t, c->c.g, c->c.h);
	XPRINTF(c, "initial geometry u: %dx%d+%d+%d:%d t %d g %d v %d\n",
		c->u.w, c->u.h, c->u.x, c->u.y, c->u.b, c->c.t, c->c.g, c->c.v);

	if (!c->can.move) {
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
	attachalist(c, choseme);
	attachflist(c, focusme);
	attachstack(c, raiseme);
	ewmh_update_net_client_lists();
	if (c->is.managed)
		ewmh_update_net_window_desktop(c);
	if (c->is.bastard)
		return;
	if (c->is.dockapp) {
		adddockapp(c, choseme, focusme, raiseme);
		return;
	}
	if (!(v = c->cview) && !(v = clientview(c)) && !(v = onview(c)))
		return;
	if (v->layout && v->layout->arrange && v->layout->arrange->addclient)
		v->layout->arrange->addclient(c, choseme, focusme, raiseme);
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
	detachalist(c);
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

	if (!c || !c->can.move || !(v = c->cview))
		return;
	if (!(m = v->curmon))
		return;
	if (!isfloating(c, v))
		return;		/* for now */

	getworkarea(m, &w);
	g = c->c;

	switch (position) {
	case RelativeNone:
		if (c->sh.win_gravity == ForgetGravity)
			return;
		return moveto(c, c->sh.win_gravity);
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
	XPRINTF("CALLING reconfigure()\n");
	reconfigure(c, &g, False);
	save(c);
	focuslockclient(c);
}

void
moveby(Client *c, RelativeDirection direction, int amount)
{
	Monitor *m;
	View *v;
	Workarea w;
	ClientGeometry g = { 0, };

	if (!c || !c->can.move || !(v = c->cview))
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
		if (c->sh.win_gravity == ForgetGravity)
			return;
		return moveby(c, c->sh.win_gravity, amount);
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
	XPRINTF("CALLING reconfigure()\n");
	reconfigure(c, &g, False);
	save(c);
	focuslockclient(c);
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

	if (!c || !c->can.move || !(v = c->cview))
		return;
	if (!(m = v->curmon))
		return;
	if (!isfloating(c, v))
		return;		/* for now */

	getworkarea(m, &w);
	g = c->c;

	switch (direction) {
	case RelativeNone:
		if (c->sh.win_gravity == ForgetGravity)
			return;
		return snapto(c, c->sh.win_gravity);
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
	XPRINTF("CALLING reconfigure()\n");
	reconfigure(c, &g, False);
	save(c);
	focuslockclient(c);
}

void
edgeto(Client *c, int direction)
{
	Monitor *m;
	View *v;
	Workarea w;
	ClientGeometry g = { 0, };

	if (!c || !c->can.move || !(v = c->cview))
		return;
	if (!(m = v->curmon))
		return;
	if (!isfloating(c, v))
		return;		/* for now */

	getworkarea(m, &w);
	g = c->c;

	switch (direction) {
	case RelativeNone:
		if (c->sh.win_gravity == ForgetGravity)
			return;
		return edgeto(c, c->sh.win_gravity);
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
	XPRINTF("CALLING reconfigure()\n");
	reconfigure(c, &g, False);
	save(c);
	focuslockclient(c);
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
		focus(s); /* XXX: focus before arrange */
		arrange(v);
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
		focus(s); /* XXX: focus before arrange */
		arrange(v);
	}
}

void
togglefloating(Client *c)
{
	View *v;

	if (!c || c->is.floater || !c->can.floats || !(v = c->cview))
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

	if (!c || (!c->can.fill && c->is.managed) || !(v = c->cview))
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

	if (!c || (!c->can.full && c->is.managed) || !(v = c->cview))
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
		if (restack())
			focuslockclient(NULL);
	}
}

void
togglemax(Client *c)
{
	View *v;

	if (!c || (!c->can.max && c->is.managed) || !(v = c->cview))
		return;
	c->is.max = !c->is.max;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, v);
		if (restack())
			focuslockclient(NULL);
	}
}

void
togglemaxv(Client *c)
{
	View *v;

	if (!c || (!c->can.maxv && c->is.managed) || !(v = c->cview))
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

	if (!c || (!c->can.maxh && c->is.managed) || !(v = c->cview))
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

	if (!c || ((!c->can.size || !c->can.move) && c->is.managed) || !(v = c->cview))
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

	if (!c || ((!c->can.size || !c->can.move) && c->is.managed) || !(v = c->cview))
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

	if (!c || (!c->can.shade && c->is.managed) || !(v = c->cview))
		return;
	c->is.shaded = !c->is.shaded;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, v);
	}
}

void
toggleundec(Client *c)
{
	View *v;

	if (!c || (!c->can.undec && c->is.managed) || !(v = c->cview))
		return;
	if ((c->is.undec = !c->is.undec)) {
		c->has.grips = False;
		c->has.title = False;
		c->has.border = False;
	} else {
		c->has.title = c->needs.title;
		c->has.grips = c->needs.grips;
		c->has.border = c->needs.border;
	}
	if (c->is.managed) {
		XPRINTF("CALLING reconfigure()\n");
		reconfigure(c, &c->c, False);
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
	focus(c); /* XXX: focus before arrange */
	arrange(v);
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
