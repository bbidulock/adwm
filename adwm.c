/* See COPYING file for copyright and license details.
 */
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

/* macros */
#define WINDOWMASK		(EnterWindowMask | LeaveWindowMask)
#define BUTTONMASK		(ButtonPressMask | ButtonReleaseMask)
#define CLEANMASK(mask)		(mask & ~(numlockmask | LockMask))
#define MOUSEMASK		(BUTTONMASK | PointerMotionMask)
#define CLIENTMASK	        (PropertyChangeMask | StructureNotifyMask)
#define CLIENTNOPROPAGATEMASK 	(BUTTONMASK | ButtonMotionMask)
#define FRAMEMASK               (MOUSEMASK | WINDOWMASK | SubstructureRedirectMask | SubstructureNotifyMask | FocusChangeMask)
#define MAPPINGMASK		(StructureNotifyMask | SubstructureRedirectMask | SubstructureNotifyMask | EnterWindowMask)
#define ROOTMASK		(BUTTONMASK | WINDOWMASK | MAPPINGMASK | FocusChangeMask)

#define EXTRANGE    16		/* all X11 extension event must fit in this range */

/* enums */
enum { CurResizeTopLeft, CurResizeTop, CurResizeTopRight, CurResizeRight,
       CurResizeBottomRight, CurResizeBottom, CurResizeBottomLeft, CurResizeLeft,
       CurMove, CurNormal, CurLast };	    /* cursor */
enum { Clk2Focus, SloppyFloat, AllSloppy, SloppyRaise };    /* focus model */

/* function declarations */
Bool alarmnotify(XEvent *e);
void applyatoms(Client *c);
void applyrules(Client *c);
void arrange(Monitor *m);
void attach(Client *c, Bool attachaside);
void attachstack(Client *c);
void ban(Client *c);
Bool buttonpress(XEvent *e);
Bool canfocus(Client *c);
void checkotherwm(void);
void cleanup(WithdrawCause cause);
Monitor *closestmonitor(int x, int y);
void compileregs(void);
void send_configurenotify(Client *c, Window above);
Bool configurenotify(XEvent *e);
Bool configurerequest(XEvent *e);
Bool destroynotify(XEvent *e);
void detach(Client *c);
void detachstack(Client *c);
void discardenter(void);
void *ecalloc(size_t nmemb, size_t size);
void *emallocz(size_t size);
void *erealloc(void *ptr, size_t size);
Bool enternotify(XEvent *e);
void eprint(const char *errstr, ...);
Bool expose(XEvent *e);
Group *getleader(Window leader, int group);
Bool handle_event(XEvent *ev);
void iconify(Client *c);
void incnmaster(const char *arg);
Monitor *findcurmonitor(Client *c);
void focus(Client *c);
Client *focusforw(Client *c);
Client *focusback(Client *c);
Client *focuslast(Client *c);
void focusnext(Client *c);
void focusprev(Client *c);
Client *getclient(Window w, int part);
const char *getresource(const char *resource, const char *defval);
long getstate(Window w);
Bool gettextprop(Window w, Atom atom, char **text);
void getpointer(int *x, int *y);
Monitor *getmonitor(int x, int y);
Monitor *curmonitor();
Monitor *selmonitor();
Monitor *clientmonitor(Client *c);
Bool isvisible(Client *c, Monitor *m);
void incmodal(Client *c, Group *g);
void decmodal(Client *c, Group *g);
Bool initmonitors(XEvent *e);
void freemonitors(void);
void updatemonitors(XEvent *e, int n, Bool size, Bool full);
Bool keypress(XEvent *e);
Bool keyrelease(XEvent *e);
void killclient(Client *c);
Bool leavenotify(XEvent *e);
Bool focusin(XEvent *e);
Bool focusout(XEvent *e);
AScreen *geteventscr(XEvent *ev);
void gavefocus(Client *c);
void grid(Monitor *m);
void manage(Window w, XWindowAttributes *wa);
Bool mappingnotify(XEvent *e);
void monocle(Monitor *m);
Bool maprequest(XEvent *e);
void mousemove(Client *c, XEvent *ev);
void mouseresize_from(Client *c, int from, XEvent *ev);
void mouseresize(Client *c, XEvent *ev);
void m_move(Client *c, XEvent *ev);
void m_shade(Client *c, XEvent *ev);
void m_zoom(Client *c, XEvent *ev);
void m_resize(Client *c, XEvent *ev);
void m_spawn(Client *c, XEvent *ev);
void m_prevtag(Client *c, XEvent *ev);
void m_nexttag(Client *c, XEvent *ev);
void moveresizekb(Client *c, int dx, int dy, int dw, int dh);
Monitor *nearmonitor(void);
Client *nexttiled(Client *c, Monitor *m);
Client *prevtiled(Client *c, Monitor *m);
void place(Client *c, WindowPlacement p);
Bool propertynotify(XEvent *e);
Bool reparentnotify(XEvent *e);
void quit(const char *arg);
void raiseclient(Client *c);
void restart(const char *arg);
Bool constrain(Client *c, ClientGeometry *g);
void resize(Client *c, ClientGeometry *g);
void restack(void);
void restack_belowif(Client *c, Client *sibling);
void restack_client(Client *c, int stack_mode, Client *sibling);
void run(void);
void save(Client *c);
void restore(Client *c);
void restore_float(Client *c);
void scan(void);
void setclientstate(Client *c, long state);
void setfocus(Client *c);
void setlayout(const char *arg);
void setmwfact(const char *arg);
void setup(char *);
void spawn(const char *arg);
void tag(Client *c, int index);
void tile(Monitor *m);
void takefocus(Client *c);
void togglestruts(void);
void togglefloating(Client *c);
void togglemax(Client *c);
void togglemaxv(Client *c);
void togglemaxh(Client *c);
void togglefill(Client *c);
void toggleshade(Client *c);
void toggletag(Client *c, int index);
void toggleview(int index);
void togglemonitor(void);
void toggleshowing(void);
void togglehidden(Client *c);
void focusview(int index);
void unban(Client *c, Monitor *m);
void unmanage(Client *c, WithdrawCause cause);
void updategeom(Monitor *m);
void updatestruts(void);
Bool unmapnotify(XEvent *e);
void updatesizehints(Client *c);
void updatetitle(Client *c);
void updategroup(Client *c, Window leader, int group, int *nonmodal);
Window *getgroup(Client *c, Window leader, int group, unsigned int *count);
void removegroup(Client *c, Window leader, int group);
void updateiconname(Client *c);
void updatefloat(Client *c, Monitor *m);
void view(int index);
void viewprevtag(void);			/* views previous selected tags */
void viewlefttag(void);
void viewrighttag(void);
int xerror(Display *dpy, XErrorEvent *ee);
int xerrordummy(Display *dsply, XErrorEvent *ee);
int xerrorstart(Display *dsply, XErrorEvent *ee);
int (*xerrorxlib) (Display *, XErrorEvent *);
void zoom(Client *c);

void addtag(void);
void appendtag(void);
void deltag(void);
void rmlasttag(void);
void settags(unsigned int numtags);

Bool isdockapp(Window win);
Bool issystray(Window win);
void delsystray(Window win);

/* variables */
int cargc;
char **cargv;
Display *dpy;
AScreen *scr;
AScreen *event_scr;
AScreen *screens;
unsigned int nscr;
#ifdef STARTUP_NOTIFICATION
SnDisplay *sn_dpy;
SnMonitorContext *sn_ctx;
Notify *notifies;
#endif
XrmDatabase xrdb;
Bool otherwm;
Bool running = True;
Client *sel;
Client *give;	/* gave focus last */
Client *take;	/* take focus last */
Group window_stack = { NULL, 0, 0 };
XContext context[PartLast];
Cursor cursor[CurLast];
int ebase[BaseLast];
Bool haveext[BaseLast];
Rule **rules;
unsigned int nrules;
unsigned int modkey;
unsigned int numlockmask;
Time user_time;
Time give_time;
Time take_time;
Group systray = { NULL, 0, 0 };
/* configuration, allows nested code to access above variables */
#include "config.h"

struct {
	Bool attachaside;
	Bool dectiled;
	Bool hidebastards;
	Bool autoroll;
	int focus;
	int snap;
	char command[255];
} options;

void (*actions[LastOn][5][2])(Client *, XEvent *) = {
	/* *INDENT-OFF* */
	/* OnWhere */
	[OnClientTitle]	 = {
				/* ButtonPress	    ButtonRelease */
		[Button1-1] =	{ mousemove,	    NULL	    },
		[Button2-1] =	{ NULL,		    m_zoom	    },
		[Button3-1] =	{ mouseresize,	    NULL	    },
		[Button4-1] =	{ NULL,		    m_shade	    },
		[Button5-1] =	{ NULL,		    m_shade	    },
	},
	[OnClientGrips]  = {
		[Button1-1] =	{ mouseresize,	    NULL	    },
		[Button2-1] =	{ NULL,		    NULL	    },
		[Button3-1] =	{ NULL,		    NULL	    },
		[Button4-1] =	{ NULL,		    NULL	    },
		[Button5-1] =	{ NULL,		    NULL	    },
	},
	[OnClientFrame]	 = {
		[Button1-1] =	{ mouseresize,	    NULL	    },
		[Button2-1] =	{ NULL,		    NULL	    },
		[Button3-1] =	{ NULL,		    NULL	    },
		[Button4-1] =	{ NULL,		    m_shade	    },
		[Button5-1] =	{ NULL,		    m_shade	    },
	},
	[OnClientWindow] = {
		[Button1-1] =	{ m_move,	    NULL	    },
		[Button2-1] =	{ m_zoom,	    NULL	    },
		[Button3-1] =	{ m_resize,	    NULL	    },
		[Button4-1] =	{ m_shade,	    NULL	    },
		[Button5-1] =	{ m_shade,	    NULL	    },
	},
	[OnRoot]	 = {
		[Button1-1] =	{ NULL,		    NULL	    },
		[Button2-1] =	{ NULL,		    NULL	    },
		[Button3-1] =	{ NULL,		    m_spawn	    },
		[Button4-1] =	{ NULL,		    m_prevtag	    },
		[Button5-1] =	{ NULL,		    m_nexttag	    },
	},
	/* *INDENT-ON* */
};

Layout layouts[] = {
	/* *INDENT-OFF* */
	/* function	symbol	features			major		minor		placement		*/
	{  NULL,	'i',	OVERLAP,			0,		0,		ColSmartPlacement	},
	{  tile,	't',	MWFACT | NMASTER | ZOOM | ROTL,	OrientLeft,	OrientBottom,	ColSmartPlacement	},
	{  tile,	'b',	MWFACT | NMASTER | ZOOM | ROTL,	OrientBottom,	OrientLeft,	ColSmartPlacement	},
	{  tile,	'u',	MWFACT | NMASTER | ZOOM | ROTL,	OrientTop,	OrientRight,	ColSmartPlacement	},
	{  tile,	'l',	MWFACT | NMASTER | ZOOM | ROTL,	OrientRight,	OrientTop,	ColSmartPlacement	},
	{  monocle,	'm',	0,				0,		0,		ColSmartPlacement	},
	{  NULL,	'f',	OVERLAP,			0,		0,		ColSmartPlacement	},
	{  grid,	'g',	NCOLUMNS | ROTL,		OrientLeft,	OrientTop,	ColSmartPlacement	},
	{  NULL,	'\0',	0,				0,		0,		0			}
	/* *INDENT-ON* */
};

Bool
IGNOREEVENT(XEvent *e)
{
	XPRINTF("Got ignored event %d\n", e->type);
	return False;
}

Bool (*handler[LASTEvent+(EXTRANGE*BaseLast)]) (XEvent *) = {
	[KeyPress] = keypress,
	[KeyRelease] = keyrelease,
	[ButtonPress] = buttonpress,
	[ButtonRelease] = buttonpress,
	[MotionNotify] = IGNOREEVENT,
	[EnterNotify] = enternotify,
	[LeaveNotify] = leavenotify,
	[FocusIn] = focusin,
	[FocusOut] = focusout,
	[KeymapNotify] = IGNOREEVENT,
	[Expose] = expose,
	[GraphicsExpose] = IGNOREEVENT,
	[NoExpose] = IGNOREEVENT,
	[VisibilityNotify] = IGNOREEVENT,
	[CreateNotify] = IGNOREEVENT,
	[DestroyNotify] = destroynotify,
	[UnmapNotify] = unmapnotify,
	[MapNotify] = IGNOREEVENT,
	[MapRequest] = maprequest,
	[ReparentNotify] = reparentnotify,
	[ConfigureNotify] = configurenotify,
	[ConfigureRequest] = configurerequest,
	[GravityNotify] = IGNOREEVENT,
	[ResizeRequest] = IGNOREEVENT,
	[CirculateNotify] = IGNOREEVENT,
	[CirculateRequest] = IGNOREEVENT,
	[PropertyNotify] = propertynotify,
	[SelectionClear] = selectionclear,
	[SelectionRequest] = IGNOREEVENT,
	[SelectionNotify] = IGNOREEVENT,
	[ColormapNotify] = IGNOREEVENT,
	[ClientMessage] = clientmessage,
	[MappingNotify] = mappingnotify,
	[GenericEvent] = IGNOREEVENT,
#ifdef XRANDR
	[RRScreenChangeNotify + LASTEvent + EXTRANGE*XrandrBase] = initmonitors,
#endif
#ifdef SYNC
	[XSyncAlarmNotify + LASTEvent + EXTRANGE*XsyncBase] = alarmnotify,
#endif
};

/* function implementations */
void
applyatoms(Client * c) {
	long *t;
	unsigned long n;
	unsigned int i, tag;

	/* restore tag number from atom */
	t = getcard(c->win, _XA_NET_WM_DESKTOP, &n);
	if (n > 0) {
		tag = *t;
		XFree(t);
		if (tag >= scr->ntags)
			return;
		for (i = 0; i < scr->ntags; i++)
			c->tags[i] = (i == tag) ? 1 : 0;
	}
}

void
applyrules(Client * c) {
	static char buf[512];
	unsigned int i, j;
	regmatch_t tmp;
	Bool matched = False;
	XClassHint ch = { 0 };
	Monitor *cm = c->curmon ?: selmonitor();

	/* rule matching */
	XGetClassHint(dpy, c->win, &ch);
	snprintf(buf, sizeof(buf), "%s:%s:%s",
	    ch.res_class ? ch.res_class : "", ch.res_name ? ch.res_name : "", c->name);
	buf[LENGTH(buf)-1] = 0;
	for (i = 0; i < nrules; i++)
		if (rules[i]->propregex && !regexec(rules[i]->propregex, buf, 1, &tmp, 0)) {
			c->skip.arrange = rules[i]->isfloating;
			if (!(c->has.title = rules[i]->hastitle))
				c->has.grips = False;
			for (j = 0; rules[i]->tagregex && j < scr->ntags; j++) {
				if (!regexec(rules[i]->tagregex, scr->tags[j], 1, &tmp, 0)) {
					matched = True;
					c->tags[j] = True;
				}
			}
		}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	if (!matched && cm)
		memcpy(c->tags, cm->seltags, scr->ntags * sizeof(cm->seltags[0]));
}

void
arrangefloats(Monitor * m) {
	Client *c;

	for (c = scr->stack; c; c = c->snext)
		if (isvisible(c, m) && !c->is.bastard) /* XXX: can.move? can.tag? */
			updatefloat(c, m);
}

static void
arrangemon(Monitor *m)
{
	Client *c;

	if (scr->views[m->curtag].layout->arrange)
		scr->views[m->curtag].layout->arrange(m);
	arrangefloats(m);
	for (c = scr->stack; c; c = c->snext) {
		if ((clientmonitor(c) == m)
		    && ((!c->is.bastard && !(c->is.icon || c->is.hidden))
			|| (c->is.bastard && scr->views[m->curtag].barpos == StrutsOn))) {
			unban(c, m);
		}
	}

	for (c = scr->stack; c; c = c->snext) {
		if ((clientmonitor(c) == NULL)
		    || (!c->is.bastard && (c->is.icon || c->is.hidden))
		    || (c->is.bastard && scr->views[m->curtag].barpos == StrutsHide)) {
			ban(c);
		}
	}
	for (c = scr->stack; c; c = c->snext)
		ewmh_update_net_window_state(c);
}

void
discardenter()
{
	XEvent ev;

	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev)) ;
}

void
arrange(Monitor *om) {
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

void
attach(Client * c, Bool attachaside) {
	if (attachaside) {
		if (scr->clients) {
			Client * lastClient = scr->clients;
			while (lastClient->next)
				lastClient = lastClient->next;
			c->prev = lastClient;
			lastClient->next = c;
		}
		else
			scr->clients = c;
	}
	else {
		if (scr->clients)
			scr->clients->prev = c;
		c->next = scr->clients;
		scr->clients = c;
	}
}

void
attachclist(Client *c)
{
	Client **cp;

	assert(c->cnext == NULL);
	for (cp = &scr->clist; *cp; cp = &(*cp)->cnext) ;
	*cp = c;
	c->cnext = NULL;
}

void
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

void
attachstack(Client *c) {
	c->snext = scr->stack;
	scr->stack = c;
}

void
getgeometry(Client *c, Geometry *g, ClientGeometry *gc)
{
	*(Geometry *)gc = *g;
	gc->t = c->th;
	gc->g = c->gh;
}

void
setwmstate(Window win, long state, Window icon_win)
{
	long data[] = { state, icon_win };
	XEvent ev;

	ev.xclient.display = dpy;
	ev.xclient.type = ClientMessage;
	ev.xclient.window = win;
	ev.xclient.message_type = _XA_KDE_WM_CHANGE_STATE;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = state;
	ev.xclient.data.l[1] = ev.xclient.data.l[2] = 0;
	ev.xclient.data.l[3] = ev.xclient.data.l[4] = 0;

	XSendEvent(dpy, scr->root, False,
		   SubstructureRedirectMask | SubstructureNotifyMask, &ev);

	XChangeProperty(dpy, win, _XA_WM_STATE, _XA_WM_STATE, 32,
			PropModeReplace, (unsigned char *) data, 2);
}

void
setclientstate(Client *c, long state)
{
	setwmstate(c->win, state, None);

	if (state == NormalState && (c->is.icon || c->is.hidden)) {
		c->is.icon = False;
		c->is.hidden = False;
		ewmh_update_net_window_state(c);
	}
}

void
ban(Client *c)
{
	c->curmon = NULL;

	if (c->is.icon)
		setclientstate(c, IconicState);
	else
		setwmstate(c->win, NormalState, None);

	if (!c->is.banned) {
		c->is.banned = True;
		XUnmapWindow(dpy, c->frame);
	}
}

void
unban(Client *c, Monitor *m)
{
	c->curmon = m;
	if (c->is.banned) {
		XMapWindow(dpy, c->frame);
		c->is.banned = False;
		setclientstate(c, NormalState);
	}
}

Bool
isfloating(Client *c, Monitor *m)
{
	if (c->is.floater || c->skip.arrange)
		return True;
	if (m && MFEATURES(m, OVERLAP))
		return True;
	return False;
}

Bool
buttonpress(XEvent *e)
{
	Client *c;
	int i;
	XButtonPressedEvent *ev = &e->xbutton;
	static int button_states[Button5] = { 0, };
	static int button_mask = 0;
	void (*action) (Client *, XEvent *);
	int button, direct;

	if (Button1 > ev->button || ev->button > Button5)
		return False;
	button = ev->button - Button1;
	direct = (ev->type == ButtonPress) ? 0 : 1;

	if (user_time == CurrentTime || (int) ev->time - (int) user_time > 0)
		user_time = ev->time;

	XPRINTF("BUTTON %d: window 0x%lx root 0x%lx subwindow 0x%lx time %ld x %d y %d x_root %d y_root %d state 0x%x\n",
			ev->button, ev->window, ev->root, ev->subwindow,
			ev->time, ev->x, ev->y, ev->x_root, ev->y_root,
			ev->state);

	if (ev->window != scr->root)
		button_mask &= ~(1 << button);

	if (ev->window == scr->root) {
		XPRINTF("SCREEN %d: 0x%lx button: %d\n", scr->screen, ev->window, ev->button);
		/* _WIN_DESKTOP_BUTTON_PROXY */
		/* modifiers or not interested in press */
		if (ev->type == ButtonPress) {
			if (ev->state || ev->button < Button3 || ev->button > Button5) {
				button_states[button] = ev->state;
				XUngrabPointer(dpy, CurrentTime);	// ev->time ??
				XSendEvent(dpy, scr->selwin, False,
					   SubstructureNotifyMask, e);
				return True;
			}
			/* only process button presses once per button */
			if ((button_mask & (1 << button)))
				return True;
			button_mask |= (1 << button);
		} else if (ev->type == ButtonRelease) {
			if (button_states[ev->button] || ev->button < Button3
			    || ev->button > Button5) {
				XSendEvent(dpy, scr->selwin, False,
					   SubstructureNotifyMask, e);
				return True;
			}
			/* do not process button releases with no corresponding press */
			if (!(button_mask & (1 << button)))
				return True;
			button_mask &= ~(1 << button);
		}
		if ((action = actions[OnRoot][button][direct]))
			(*action) (NULL, (XEvent *) ev);
		XUngrabPointer(dpy, CurrentTime);
	} else if ((c = getclient(ev->window, ClientTitle)) && ev->window == c->title) {
		XPRINTF("TITLE %s: 0x%lx button: %d\n", c->name, ev->window, ev->button);
		for (i = 0; i < LastElement; i++) {
			ElementClient *ec = &c->element[i];
			Bool active;

			active = (scr->element[i].action
				  && (scr->element[i].action[button * 2 + 0]
				      || scr->element[i].action[button * 2 + 1]))
			    ? True : False;
			action = (scr->element[i].action
				  && (scr->element[i].action[button * 2 + 0]
				      || scr->element[i].action[button * 2 + 1]))
			    ? scr->element[i].action[button * 2 + direct] : NULL;

			if (!ec->present)
				continue;
			if (ev->x >= ec->g.x && ev->x < ec->g.x + ec->g.w
			    && ev->y >= ec->g.y && ev->y < ec->g.y + ec->g.h) {
				if (ev->type == ButtonPress) {
					XPRINTF("ELEMENT %d PRESSED\n", i);
					ec->pressed |= (1 << button);
					drawclient(c);
					/* resize needs to be on button press */
					if (action) {
						(*action) (c, (XEvent *) ev);
						drawclient(c);
					}
				} else if (ev->type == ButtonRelease) {
					/* only process release if processed press */
					if (ec->pressed & (1 << button)) {
						XPRINTF("ELEMENT %d RELEASED\n", i);
						ec->pressed &= !(1 << button);
						drawclient(c);
						/* resize needs to be on button press */
						if (action) {
							(*action) (c, (XEvent *) ev);
							drawclient(c);
						}
					}
				}
				if (active)
					return True;
			} else {
				ec->pressed &= !(1 << button);
				drawclient(c);
			}
		}
		if ((action = actions[OnClientTitle][button][direct]))
			(*action) (c, (XEvent *) ev);
		XUngrabPointer(dpy, CurrentTime);
		drawclient(c);
	} else if ((c = getclient(ev->window, ClientGrips)) && ev->window == c->grips) {
		if ((action = actions[OnClientGrips][button][direct]))
			(*action) (c, (XEvent *) ev);
		XUngrabPointer(dpy, CurrentTime);
		drawclient(c);
	} else if ((c = getclient(ev->window, ClientWindow)) && ev->window == c->win) {
		XPRINTF("WINDOW %s: 0x%lx button: %d\n", c->name, ev->window, ev->button);
		if (CLEANMASK(ev->state) != modkey) {
			XAllowEvents(dpy, ReplayPointer, CurrentTime);
			return True;
		}
		if ((action = actions[OnClientWindow][button][direct]))
			(*action) (c, (XEvent *) ev);
		XUngrabPointer(dpy, CurrentTime);	// ev->time ??
		drawclient(c);
	} else if ((c = getclient(ev->window, ClientFrame)) && ev->window == c->frame) {
		XPRINTF("FRAME %s: 0x%lx button: %d\n", c->name, ev->window, ev->button);
		if ((action = actions[OnClientFrame][button][direct]))
			(*action) (c, (XEvent *) ev);
		XUngrabPointer(dpy, CurrentTime);	// ev->time ??
		drawclient(c);
	} else
		return False;
	return True;
}

static Bool selectionreleased(Display *display, XEvent *event, XPointer arg)
{
	if (event->type == DestroyNotify) {
		if (event->xdestroywindow.window == (Window)arg) {
			return True;
		}
	}
	return False;
}

Bool
selectionclear(XEvent * e)
{
	char *name = XGetAtomName(dpy, e->xselectionclear.selection);
	Bool ret = False;

	if (name != NULL && strncmp(name, "WM_S", 4) == 0) {
		/* lost WM selection - must exit */
		ret = True;
		quit(NULL);
	}
	if (name)
		XFree(name);
	return ret;
}

void
prepare_display(void)
{
}

void
checkotherwm(void) {
	Atom wm_sn, wm_protocols, manager;
	Window wm_sn_owner;
	XTextProperty hname;
	XClassHint class_hint;
	XClientMessageEvent manager_event;
	char name[32], hostname[64] = { 0, };
	char *names[25] = {
		"WM_COMMAND",
		"WM_HINTS",
		"WM_CLIENT_MACHINE",
		"WM_ICON_NAME",
		"WM_NAME",
		"WM_NORMAL_HINTS",
		"WM_SIZE_HINTS",
		"WM_ZOOM_HINTS",
		"WM_CLASS",
		"WM_TRANSIENT_FOR",
		"WM_CLIENT_LEADER",
		"WM_DELETE_WINDOW",
		"WM_LOCALE_NAME",
		"WM_PROTOCOLS",
		"WM_TAKE_FOCUS",
		"WM_WINDOW_ROLE",
		"WM_STATE",
		"WM_CHANGE_STATE",
		"WM_SAVE_YOURSELF",
		"SM_CLIENT_ID",
		"_MOTIF_WM_HINTS",
		"_MOTIF_WM_MESSAGES",
		"_MOTIF_WM_OFFSET",
		"_MOTIF_WM_INFO",
		"__SWM_ROOT"
	};
	Atom atoms[25] = { None, };

	snprintf(name, 32, "WM_S%d", scr->screen);
	wm_sn = XInternAtom(dpy, name, False);
	wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
	manager = XInternAtom(dpy, "MANAGER", False);
	scr->selwin = XCreateSimpleWindow(dpy, scr->root, DisplayWidth(dpy, scr->screen),
			DisplayHeight(dpy, scr->screen), 1, 1, 0, 0L, 0L);
	XGrabServer(dpy);
	wm_sn_owner = XGetSelectionOwner(dpy, wm_sn);
	if (wm_sn_owner != None) {
		XSelectInput(dpy, wm_sn_owner, StructureNotifyMask);
		XSync(dpy, False);
	}
	XUngrabServer(dpy);

	XSetSelectionOwner(dpy, wm_sn, scr->selwin, CurrentTime);

	if (wm_sn_owner != None) {
		XEvent event_return;
		XIfEvent(dpy, &event_return, &selectionreleased,
				(XPointer) wm_sn_owner);
		wm_sn_owner = None;
	}
	gethostname(hostname, 64);
	hname.value = (unsigned char *) hostname;
	hname.encoding = XA_STRING;
	hname.format = 8;
	hname.nitems = strnlen(hostname, 64);

	class_hint.res_name = NULL;
	class_hint.res_class = "adwm";

	Xutf8SetWMProperties(dpy, scr->selwin, "Adwm version: " VERSION,
			"adwm " VERSION, cargv, cargc, NULL, NULL,
			&class_hint);
	XSetWMClientMachine(dpy, scr->selwin, &hname);
	XInternAtoms(dpy, names, 25, False, atoms);
	XChangeProperty(dpy, scr->selwin, wm_protocols, XA_ATOM, 32,
			PropModeReplace, (unsigned char *)atoms,
			sizeof(atoms)/sizeof(atoms[0]));

	manager_event.display = dpy;
	manager_event.type = ClientMessage;
	manager_event.window = scr->root;
	manager_event.message_type = manager;
	manager_event.format = 32;
	manager_event.data.l[0] = CurrentTime; /* FIXME: timestamp */
	manager_event.data.l[1] = wm_sn;
	manager_event.data.l[2] = scr->selwin;
	manager_event.data.l[3] = 2;
	manager_event.data.l[4] = 0;
	XSendEvent(dpy, scr->root, False, StructureNotifyMask, (XEvent *)&manager_event);
	XSync(dpy, False);

	otherwm = False;
	XSetErrorHandler(xerrorstart);

	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, scr->root, SubstructureRedirectMask);
	XSync(dpy, False);
	if (otherwm) {
		XDestroyWindow(dpy, scr->selwin);
		scr->selwin = None;
		scr->managed = False;
	}
	XSync(dpy, False);
	XSetErrorHandler(NULL);
	xerrorxlib = XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(WithdrawCause cause) {

	for (scr = screens; scr < screens + nscr; scr++) {
		while (scr->stack) {
			unban(scr->stack, NULL);
			unmanage(scr->stack, cause);
		}
	}

	for (scr = screens; scr < screens + nscr; scr++) {
		free(scr->tags);
		scr->tags = NULL;
		free(scr->keys);
		scr->keys = NULL;
		freemonitors();
	}

	/* free resource database */
	XrmDestroyDatabase(xrdb);

	for (scr = screens; scr < screens + nscr; scr++) {
		deinitstyle();
		XUngrabKey(dpy, AnyKey, AnyModifier, scr->root);
	}

	XFreeCursor(dpy, cursor[CurResizeTopLeft]);
	XFreeCursor(dpy, cursor[CurResizeTop]);
	XFreeCursor(dpy, cursor[CurResizeTopRight]);
	XFreeCursor(dpy, cursor[CurResizeRight]);
	XFreeCursor(dpy, cursor[CurResizeBottomRight]);
	XFreeCursor(dpy, cursor[CurResizeBottom]);
	XFreeCursor(dpy, cursor[CurResizeBottomLeft]);
	XFreeCursor(dpy, cursor[CurResizeLeft]);
	XFreeCursor(dpy, cursor[CurMove]);
	XFreeCursor(dpy, cursor[CurNormal]);

	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XSync(dpy, False);
}

void
send_configurenotify(Client * c, Window above) {
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->c.x;
	ce.y = c->c.y + c->th;
	if (c->sync.waiting) {
		ce.width = c->sync.w;
		ce.height = c->sync.h;
	} else {
		ce.width = c->c.w;
		ce.height = c->c.h - c->th - c->gh;
	}
	ce.border_width = c->c.b; /* ICCCM 2.0 4.1.5 */
	ce.above = above;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *) & ce);
}

Bool
configurenotify(XEvent * e)
{
	XConfigureEvent *ev = &e->xconfigure;

	if (ev->window == scr->root) {
		initmonitors(e);
		return True;
	}
	return False;
}

void
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
void
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
applygravity(Client *c, ClientGeometry *g, int gravity)
{
	int xr, yr;

	if (gravity == StaticGravity)
		*(Geometry *)g = c->s;
	getreference(&xr, &yr, (Geometry *)g, gravity);
	g->h += g->t + g->g;
	if (gravity != StaticGravity) {
		DPRINTF("CALLING: constrain()\n");
		constrain(c, g);
	}
	putreference(xr, yr, (Geometry *)g, gravity);
}

Bool
configurerequest(XEvent *e)
{
	Client *c;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = getclient(ev->window, ClientWindow))) {
		Monitor *cm;

		/* XXX: if c->curmon is set, we are displayed on a monitor, but
		   c->curview should also be set.  We only need the monitor for the
		   layout.  When there is no curmon, use one of the views to which the
		   client is tagged? How meaningful is it moving a window not in the
		   current view? Perhaps we should treat it a just moving the saved
		   floating state.  This is the only function that calls
		   findcurmonitor(). */

		if (!(cm = c->curmon ?: selmonitor()))
			return False;
		if (!c->is.max && isfloating(c, cm)) {
			ClientGeometry g;

			getgeometry(c, &c->c, &g);

			g.x = ((ev->value_mask & CWX) && c->can.move) ? ev->x : g.x;
			g.y = ((ev->value_mask & CWY) && c->can.move) ? ev->y : g.y;
			g.w = ((ev->value_mask & CWWidth)
				 && c->can.sizeh) ? ev->width : g.w;
			g.h = ((ev->value_mask & CWHeight)
				 && c->can.sizev) ? ev->height : g.h - g.t - g.g;
			g.b = (ev->value_mask & CWBorderWidth) ? ev->border_width : g.b;

			applygravity(c, &g, c->gravity);
			DPRINTF("CALLING resize()\n");
			resize(c, &g);
			if (ev->value_mask &
			    (CWX | CWY | CWWidth | CWHeight | CWBorderWidth))
				save(c);
			/* TODO: check _XA_WIN_CLIENT_MOVING and handle moves between
			   monitors */
		} else {
			ClientGeometry g;

			getgeometry(c, &c->c, &g);

			g.b = (ev->value_mask & CWBorderWidth) ? ev->border_width : g.b;

			DPRINTF("CALLING resize()\n");
			resize(c, &g);
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
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
		XSync(dpy, False);
	}
	return True;
}

Bool
destroynotify(XEvent * e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = getclient(ev->window, ClientWindow))) {
		XPRINTF("unmanage destroyed window (%s)\n", c->name);
		unmanage(c, CauseDestroyed);
		return True;
	}
	if (XFindContext(dpy, ev->window, context[SysTrayWindows],
				(XPointer *)&c) == Success) {
		XDeleteContext(dpy, ev->window, context[SysTrayWindows]);
		delsystray(ev->window);
		return True;
	}
	XDeleteContext(dpy, ev->window, context[ScreenContext]);
	return True;
}

void
detach(Client * c) {
	if (c->prev)
		c->prev->next = c->next;
	if (c->next)
		c->next->prev = c->prev;
	if (c == scr->clients)
		scr->clients = c->next;
	c->next = c->prev = NULL;
}

void
detachclist(Client *c)
{
	Client **cp;

	for (cp = &scr->clist; *cp && *cp != c; cp = &(*cp)->cnext) ;
	assert(*cp == c);
	*cp = c->cnext;
	c->cnext = NULL;
}

void
detachflist(Client *c)
{
	Client **cp;

	for (cp = &scr->flist; *cp && *cp != c; cp = &(*cp)->fnext) ;
	assert(*cp == c);
	*cp = c->fnext;
	c->fnext = NULL;
}

void
detachstack(Client *c) {
	Client **cp;

	for (cp = &scr->stack; *cp && *cp != c; cp = &(*cp)->snext);
	assert(*cp == c);
	*cp = c->snext;
	c->snext = NULL;
}

void *
ecalloc(size_t nmemb, size_t size) {
	void *res = calloc(nmemb, size);

	if (!res)
		eprint("fatal: could not calloc() %z x %z bytes\n", nmemb, size);
	return res;
}

void *
emallocz(size_t size) {
	return ecalloc(1, size);
}

void *
erealloc(void *ptr, size_t size) {
	void *res = realloc(ptr, size);

	if (!res)
		eprint("fatal: could not realloc() %z bytes\n", size);
	return res;
}

Bool
enternotify(XEvent * e) {
	XCrossingEvent *ev = &e->xcrossing;
	Client *c;

	if (ev->mode != NotifyNormal || ev->detail == NotifyInferior)
		return True;
	if ((c = getclient(ev->window, ClientFrame))) {
		if (c->is.bastard)
			return True;
		/* focus when switching monitors */
		if (!isvisible(sel, c->curmon)) {
			XPRINTF("FOCUS: monitor switching focus to 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
			focus(c);
		}
		switch (options.focus) {
		case Clk2Focus:
			break;
		case SloppyFloat:
			if (!c->skip.sloppy && isfloating(c, c->curmon)) {
				XPRINTF("FOCUS: sloppy focus to 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
				focus(c);
			}
			break;
		case AllSloppy:
			if (!c->skip.sloppy) {
				XPRINTF("FOCUS: sloppy focus to 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
				focus(c);
			}
			break;
		case SloppyRaise:
			if (!c->skip.sloppy) {
				XPRINTF("FOCUS: sloppy focus to 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
				focus(c);
				raiseclient(c);
			}
			break;
		}
	} else if (ev->window == scr->root) {
#if 0
		/* no no no, stay with previously focused client */
		if (scr->managed)
			focus(NULL);
#endif
	} else
		return False;
	return True;
}

void
eprint(const char *errstr, ...) {
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

Bool
focusin(XEvent *e)
{
	XFocusInEvent *ev = &e->xfocus;
	Client *c;

	/* FIXME: this is not quite correct: under some focus models we should allow one
	   window in group to move keyboard focus to another of its windows. */

	/*
	 * No Input
	 *	    - The client never expects keyboard input.  An example would be xload or
	 *	      another output-only client.
	 *
	 * Passive Input
	 *	    - The client expects keyboard input but never explicitly sets the input
	 *	      focus.  An example would be a simply client with no subwindows, which
	 *	      will accept input in PointerRoot mode or when the window manager sets
	 *	      the input focus to is top-level window (in click-to-type mode).
	 *
	 * Locally Active
	 *	    - The client expects keyboard input and explicitly sets the input focus,
	 *	      but it only does so when one of its windows alreay has the focus.  An
	 *	      example would be a client with subwindows defining various data entry
	 *	      fields that uses Next and Prev keys to move the input focus between
	 *	      the fields.  It does so when its top-level window has acquired the
	 *	      focus in PoitnerRoot mode or when the window manager sets the input
	 *	      focus to its top-level window (in click-to-type mode).
	 *
	 * Globally Active Input
	 *	    - The client expects keyboard input and explicitly sets the input focus,
	 *	      even when it is in windows the client does not own.  An example would
	 *	      be a client with a scroll bar that wants to allow users to scroll the
	 *	      window without disturbing the input focus even if it is in some other
	 *	      window.  It wants to acquire the input focus when the user clicks in
	 *	      the scrolled region but not when the user clicks in the scroll bar
	 *	      itself.  Thus, it wants to prevent the window manager from setting the
	 *	      input focus to any of its windows.
	 */

	c = getclient(ev->window, ClientFrame);
	switch (ev->mode) {
	case NotifyWhileGrabbed:
	case NotifyGrab:
		return False;
	case NotifyNormal:
		/* Events generated by SetInputFocus when the keyboard is not grabbed have mode
		 * NotifyNormal */
		break;
	case NotifyUngrab:
		/* Event generated when a keyboard grab deactivates have mode NotifyUngrab. */
		/* When a keyboard grab deactivates (but after generating any actual KeyRelease
		 * event that deactivates the grab), G is the grab-window for the grab, and F is
		 * the current focus, FocusIn and FocusOut events with mode Ungrab are generated (as
		 * for Normal below) as if the focus were to change from G to F. */
		/* Expect a FocusIn NonlinearVirtual for F.  If F is not the frame of the selected
		 * window, set the focus to the selected window. */
		break;
	default:
		XPRINTF("Unwanted FocusIn %d from 0x%lx\n", ev->mode, ev->window);
		return False;
	}
	switch (ev->detail) {
	case NotifyAncestor: /* not between top-levels */
	case NotifyVirtual: /* not between top-levels */
	case NotifyInferior: /* not between top-levels */
	case NotifyNonlinear: /* not between frames */
		return False;
	case NotifyNonlinearVirtual: /* can happen between toplevels (on client frame) */
		if (c && ev->window == c->frame) {
			/* When the focus moves from window A to window B, window C is their
			 * least common ancestor, and the pointer is in window P: FocusIn
			 * with detail NonlinearVirtual is generated on each window between C
			 * and B exclusive (in order). */
		} else if ((c && ev->window == c->frame) || ev->window == scr->root) {
			/* When the focus moves from window A to window B on different
			 * screens and the pointer is in window P: if A is not a root window,
			 * FocusIn with detail NonlinearVirtual is generated on each window
			 * from B's root down to but not including B (in order). */
			/* When the focus moves from PointerRoot (or None) to window A and
			 * the pointer is in window P: if A is not a root window, FocusIn
			 * with detail NonlinearVirtual is generated on each window from A's
			 * root down to but not including A (in order). */
		} else {
			XPRINTF("Unwanted FocusIn NotifyNonlinearVirtual from 0x%lx\n", ev->window);
			return False;
		}
		break;
	case NotifyPointer:
		if ((c && ev->window == c->frame) || ev->window == scr->root) {
			/* When the focus moves from PointerRoot (or None) to window A and
			 * the pointer is in window P: if the new focus is in PointerRoot,
			 * FocusIn with detail Pointer is generated on each window from P's
			 * root down to and including P (in order). */
			/* When the focus moves from window A to PointerRoot (or None) and
			 * the pointer is in window P: if the new focus is PointerRoot,
			 * FocusIn with detail Pointer is generated on each window from P's
			 * root down to and including P (in order). */
		} else {
			XPRINTF("Unwanted FocusIn NotifyPointer from 0x%lx\n", ev->window);
			return False;
		}
		break;
	case NotifyPointerRoot:
	case NotifyDetailNone:
		if (ev->window == scr->root) {
			/* When the focus moves from PointerRoot (or None) to window A and
			 * the pointer is in window P: FocusIn with detail None (or
			 * PointerRoot) is generated on all root windows. */
		} else {
			XPRINTF("Unwanted FocusIn NotifyPointer(Root|None) from 0x%lx\n", ev->window);
			return False;
		}
		break;
	default:
		XPRINTF("Unwanted FocusIn %d from 0x%lx\n", ev->detail, ev->window);
		return False;
	}
	if (c && ev->window == c->frame) {
		if (c == give) {
			DPRINTF("FocusIn: gave focus to 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
			gavefocus(c);
			return True; /* yes, we gave you the focus */
		}
		if (c == take) {
			DPRINTF("FocusIn: gave focus to 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
			gavefocus(c);
			takefocus(NULL);
			return True; /* yes, we asked you to take the focus */
		}
		if (take) {
			if (take->can.focus & GIVE_FOCUS) {
				DPRINTF("Client frame 0x%08lx win 0x%08lx name %s stole focus\n",
						c->frame, c->win, c->name);
				DPRINTF("Giving back to frame 0x%08lx win 0x%08lx name %s\n",
						take->frame, take->win, take->name);
				setfocus(take); /* you weren't to give it away */
			} else {
				DPRINTF("FocusIn: gave focus to 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
				gavefocus(c); /* yes, we asked you to assign focus */
				takefocus(NULL);
			}
		} else if (give) {
			DPRINTF("Client frame 0x%08lx win 0x%08lx name %s stole focus\n",
					c->frame, c->win, c->name);
			DPRINTF("Giving back to frame 0x%08lx win 0x%08lx name %s\n",
					give->frame, give->win, give->name);
			setfocus(give); /* you stole the focus */
		} else {
			if (canfocus(c)) {
				DPRINTF("FocusIn: gave focus to 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
				gavefocus(c); /* you can have the focus */
			} else {
				DPRINTF("Client frame 0x%08lx win 0x%08lx name %s stole focus\n",
						c->frame, c->win, c->name);
				DPRINTF("Giving back to 0x%08lx %s\n", scr->root, "root");
				setfocus(NULL); /* you can't have the focus */
			}
		}
	} else if (ev->window == scr->root) {
		/* nothing to do here really */
		if (give)
			DPRINTF("FocusIn: took focus from 0x%08lx 0x%08lx %s\n", give->frame, give->win, give->name);
		gavefocus(NULL);
		takefocus(NULL);
	} else {
		fprintf(stderr, "Unwanted FocusIn for unknown window 0x%lx\n", ev->window);
		return False;
	}
	return True;
}

Bool
focusout(XEvent *e)
{
	XFocusOutEvent *ev = &e->xfocus;
	Client *c;

	c = getclient(ev->window, ClientFrame);
	switch (ev->mode) {
	case NotifyWhileGrabbed:
	case NotifyGrab:
		return False;
	case NotifyNormal:
		/* Events generated by SetInputFocus when the keyboard is not grabbed have mode
		 * NotifyNormal */
		break;
	case NotifyUngrab:
		/* Event generated when a keyboard grab deactivates have mode NotifyUngrab. */
		/* When a keyboard grab deactivates (but after generating any actual KeyRelease
		 * event that deactivates the grab), G is the grab-window for the grab, and F is
		 * the current focus, FocusIn and FocusOut events with mode Ungrab are generated (as
		 * for Normal below) as if the focus were to change from G to F. */
		break;
	default:
		XPRINTF("Unwanted FocusOut %d from 0x%lx\n", ev->mode, ev->window);
		return False;
	}
	switch (ev->detail) {
	case NotifyAncestor: /* not between top-levels */
	case NotifyVirtual: /* not between top-levels */
	case NotifyInferior: /* not between top-levels */
	case NotifyNonlinear: /* not between frames */
		return False;
	case NotifyNonlinearVirtual: /* can happen between toplevels (on client frame) */
		if (c && ev->window == c->frame) {
			/* When the focus moves from window A to window B, window C is their
			 * least common ancestor, and the pointer is in window P: FocusOut
			 * with detail NonlinearVirtual is generated on each window between
			 * A and C exclusive (in order). */
		} else if ((c && ev->window == c->frame) || ev->window == scr->root) {
			/* When the focus moves from window A to window B on different
			 * screens and the pointer is in window P: if A is not a root
			 * window, FocusOut with detail NonlinearVirtual is generated on
			 * each window above A up to and including its root (in order). */
			/* When the focus moves from window A to PointerRoot (or None) and
			 * the pointer is in window P: if A is not a root window, FocusOut
			 * with detail NonlinearVirtual is generated on each window above A
			 * up to and including its root (in order). */
		} else {
			XPRINTF("Unwanted FocusOut NotifyNonlinearVirtual from 0x%lx\n", ev->window);
			return False;
		}
		break;
	case NotifyPointer:
		if ((c && ev->window == c->frame) || ev->window == scr->root) {
			/* When the focus moves from PointerRoot (or None) to window A and
			 * the pointer is in window P: if the old focus is PointerRoot,
			 * FocusOut with detail Pointer is generated on each window from P
			 * up to and including P's root (in order). */
		} else {
			XPRINTF("Unwanted FocusOut NotifyPointer from 0x%lx\n", ev->window);
			return False;
		}
		break;
	case NotifyPointerRoot:
	case NotifyDetailNone:
		if (ev->window == scr->root) {
			/* When the focus moves from window A to PointerRoot (or None) and
			 * the pointer is in window P: FocusOut with detail PointerRoot (or
			 * None) is generated on all root windows. */
			/* When the focus moves from PointerRoot (or None) to window A and
			 * the pointer is in window P: FocusOut with detail PointerRoot (or
			 * None) is generated on all root windows. */
		} else {
			XPRINTF("Unwanted FocusIn NotifyPointer(Root|None) from 0x%lx\n", ev->window);
			return False;
		}
		break;
	default:
		XPRINTF("Unwanted FocusOut %d from 0x%lx\n", ev->detail, ev->window);
		return False;
	}
	if (c && ev->window == c->frame) {
		if (c == give) {
			DPRINTF("FocusOut: lost focus from 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
			gavefocus(NULL);
		}
		else if (c->is.focused) {
			DPRINTF("FocusOut: lost focus from 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
			c->is.focused = False;
			ewmh_update_net_window_state(c);
		}
	} else if (ev->window == scr->root) {
		if (give) {
			DPRINTF("FocusOut: lost focus from 0x%08lx 0x%08lx %s\n", give->frame, give->win, give->name);
			gavefocus(NULL);
		}
	} else {
		fprintf(stderr, "Unwanted FocusOut for unknown window 0x%lx\n", ev->window);
		return False;
	}
	return True;
}

Bool
expose(XEvent * e)
{
	XExposeEvent *ev = &e->xexpose;
	XEvent tmp;
	Client *c;

	while (XCheckWindowEvent(dpy, ev->window, ExposureMask, &tmp));
	if ((c = getclient(ev->window, ClientTitle)))
		drawclient(c);
	if ((c = getclient(ev->window, ClientGrips)))
		drawclient(c);
	return True;
}

void
gavefocus(Client *c)
{
	if (give != c && give && give->is.focused) {
		give->is.focused = False;
		ewmh_update_net_window_state(give);
	}
	if ((give = c) && give->is.focused) {
		give->is.focused = True;
		ewmh_update_net_window_state(give);
	}
}

void
givefocus(Client *c)
{
	if (give != c && give && give->is.focused) {
		give->is.focused = False;
		ewmh_update_net_window_state(give);
	}
	if ((give = (c && (c->can.focus & GIVE_FOCUS)) ? c : NULL)) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, user_time);
		give_time = user_time;
		if (!c->is.focused) {
			c->is.focused = True;
			ewmh_update_net_window_state(c);
		}
	}
}

void
takefocus(Client *c) {
	if ((take = (c && (c->can.focus & TAKE_FOCUS)) ? c : NULL)) {
		XEvent ce;

		ce.xclient.type = ClientMessage;
		ce.xclient.message_type = _XA_WM_PROTOCOLS;
		ce.xclient.display = dpy;
		ce.xclient.window = c->win;
		ce.xclient.format = 32;
		ce.xclient.data.l[0] = _XA_WM_TAKE_FOCUS;
		ce.xclient.data.l[1] = user_time;
		ce.xclient.data.l[2] = 0l;
		ce.xclient.data.l[3] = 0l;
		ce.xclient.data.l[4] = 0l;
		XSendEvent(dpy, c->win, False, NoEventMask, &ce);
		take_time = user_time;
	}
}

void
raiseclient(Client *c) {
	detachstack(c);
	attachstack(c);
	restack();
}

void
lowerclient(Client *c) {
	Client **cp;

	for (cp = &scr->stack; *cp; cp = &(*cp)->snext) ;
	detachstack(c);
	*cp = c;
	c->snext = NULL;
	restack();
}

Bool
canfocus(Client *c)
{
	if (c && c->can.focus && !c->nonmodal)
		return True;
	return False;
}

void
setfocus(Client *c)
{
	/* focus() function is more like activate... */
	if (c && canfocus(c)) {
		DPRINTF("Setting focus to 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		/* top of focus stack */
		detachflist(c);
		attachflist(c, True);
		givefocus(c);
		takefocus(c);
		return;
	}
	DPRINTF("Setting focus to 0x%08lx %s\n", scr->root, "root");
	givefocus(NULL);
	takefocus(NULL);
	XSetInputFocus(dpy, scr->root, RevertToPointerRoot, CurrentTime);
}

void
focus(Client *c)
{
	Client *o;
	Monitor *cm;
	
	cm = c ? c->curmon : selmonitor();

	o = sel;
	if ((!c && scr->managed)
	    || (c && (c->is.bastard || !canfocus(c) || !isvisible(c, cm))))
		for (c = scr->flist;
		     c && (c->is.bastard || !canfocus(c) || (c->is.icon || c->is.hidden)
			   || !isvisible(c, cm)); c = c->fnext) ;
	if (sel && sel != c) {
		XSetWindowBorder(dpy, sel->frame, scr->style.color.norm[ColBorder]);
	}
	if (sel)
		XPRINTF("Deselecting %sclient frame 0x%08lx win 0x%08lx named %s\n",
			sel->is.bastard ? "bastard " : "",
			sel->frame, sel->win, sel->name);
	sel = c;
	if (c)
		XPRINTF("Selecting   %sclient frame 0x%08lx win 0x%08lx named %s\n",
			c->is.bastard ? "bastard " : "", c->frame, c->win, c->name);
	if (!scr->managed)
		return;
	ewmh_update_net_active_window();
	setfocus(sel);
	if (c) {
		if (c->is.attn)
			c->is.attn = False;
		/* FIXME: why would it be otherwise if it is focusable? Also, a client
		   could take the focus because its child window is an icon (e.g.
		   dockapp). */
		setclientstate(c, NormalState);
		XSetWindowBorder(dpy, sel->frame, scr->style.color.sel[ColBorder]);
		drawclient(c);
		if (c->is.shaded && options.autoroll)
			arrange(cm);
		ewmh_update_net_window_state(c);
		if (!isfloating(c, cm))
			raiseclient(c);
	}
	if (o && o != sel) {
		drawclient(o);
		if (o->is.shaded && options.autoroll)
			arrange(cm);
		ewmh_update_net_window_state(o);
	}
	XSync(dpy, False);
}

void
focusicon()
{
	Client *c;
	Monitor *cm;
	
	if (!(cm = selmonitor()))
		return;
	for (c = scr->clients;
	     c && (!canfocus(c) || c->skip.focus || !c->is.icon || !c->can.min
		   || !isvisible(c, cm)); c = c->next) ;
	if (!c)
		return;
	if (c->is.icon) {
		c->is.icon = False;
		c->is.hidden = False;
		ewmh_update_net_window_state(c);
	}
	focus(c);
	if (c->is.managed)
		arrange(cm);
}

Client *
focusforw(Client *c)
{
	Monitor *m;

	if (!c)
		return (c);
	if (!(m = c->curmon))
		return NULL;
	for (c = c->next;
	     c && (!canfocus(c) || c->skip.focus || (c->is.icon || c->is.hidden)
		   || !isvisible(c, m)); c = c->next) ;
	if (!c)
		for (c = scr->clients;
		     c && (!canfocus(c) || c->skip.focus || (c->is.icon || c->is.hidden)
			   || !isvisible(c, m)); c = c->next) ;
	if (c)
		focus(c);
	return (c);
}

Client *
focusback(Client *c) {
	Monitor *m;

	if (!c)
		return (c);
	if (!(m = c->curmon))
		return NULL;
	for (c = c->prev;
	    c && (!canfocus(c) || c->skip.focus || (c->is.icon || c->is.hidden)
		    || !isvisible(c, m)); c = c->prev);
	if (!c) {
		for (c = scr->clients; c && c->next; c = c->next);
		for (;
		    c && (!canfocus(c) || c->skip.focus || (c->is.icon || c->is.hidden)
			|| !isvisible(c, m)); c = c->prev);
	}
	if (c)
		focus(c);
	return (c);
}

Client *
focuslast(Client *c)
{
	Client *s;
	Monitor *m;

	if (!c || c != sel)
		return (sel);
	if (!(m = c->curmon))
		return (NULL);
	for (s = scr->flist; s &&
	     (s == c || (s->is.bastard || !canfocus(s) || (s->is.icon || s->is.hidden)
			 || !isvisible(s, m))); s = s->fnext) ;
	focus(s);
	return (s);
}

void
focusnext(Client *c) {
	if ((c = focusforw(c)))
		raiseclient(c);
}

void
focusprev(Client *c) {
	if ((c = focusback(c)))
		raiseclient(c);
}

static void
with_transients(Client *c, void (*each)(Client *, int), int data) {
	Client *t;
	Window *m;
	unsigned int i, g, n = 0;
	int ctx[2] = { ClientTransFor, ClientTransForGroup };

	if (!c)
		return;
	for (g = 0; g < 2; g++)
		if ((m = getgroup(c, c->win, ctx[g], &n)))
			for (i = 0; i < n; i++)
				if ((t = getclient(m[i], ClientWindow)) && t != c)
					each(t, data);
	each(c, data);
}

static void
_iconify(Client *c, int dummy) {
	if (!c->is.icon) {
		c->is.icon = True;
		ewmh_update_net_window_state(c);
	}
	if (c == sel)
		focuslast(c);
	ban(c);
	arrange(clientmonitor(c));
}

void
iconify(Client *c) {
	if (!c || (!c->can.min && c->is.managed))
		return;
	return with_transients(c, &_iconify, 0);
}

static void
_deiconify(Client *c, int dummy) {
	if (!c->is.icon)
		return;
	c->is.icon = False;
	c->is.hidden = False;
	if (c->is.managed)
		arrange(NULL);
}

void
deiconify(Client *c) {
	if (!c || (!c->can.min && c->is.managed))
		return;
	return with_transients(c, &_deiconify, 0);
}


static void
_hide(Client *c, int dummy) {
	if (c->is.hidden || !c->can.hide || WTCHECK(c, WindowTypeDock)
			|| WTCHECK(c, WindowTypeDesk))
		return;
	if (c == sel)
		focuslast(c);
	c->is.hidden = True;
	ewmh_update_net_window_state(c);
}

void
hide(Client *c) {
	if (!c || (!c->can.hide && c->is.managed))
		return;
	return with_transients(c, &_hide, 0);
}


static void
_show(Client *c, int dummy) {
	if (!c->is.hidden)
		return;
	c->is.hidden = False;
	ewmh_update_net_window_state(c);
}

void
show(Client *c) {
	if (!c || (!c->can.hide && c->is.managed))
		return;
	return with_transients(c, &_show, 0);
}

void
togglehidden(Client *c) {
	if (!c || !c->can.hide)
		return;
	if (c->is.hidden)
		show(c);
	else
		hide(c);
	arrange(clientmonitor(c));
}

void
hideall() {
	Client *c;

	for (c = scr->clients; c; c = c->next)
		_hide(c, 0);
	arrange(NULL);
}

void
showall() {
	Client *c;

	for (c = scr->clients; c; c = c->next)
		_show(c, 0);
	arrange(NULL);
}

void
toggleshowing() {
	if ((scr->showing_desktop = scr->showing_desktop ? False : True))
		hideall();
	else
		showall();
	ewmh_update_net_showing_desktop();
}

void
incnmaster(const char *arg)
{
	unsigned int i;
	Monitor *cm;
	View *v;

	if (!(cm = selmonitor()))
		return;
	v = scr->views + cm->curtag;

	if (!FEATURES(v->layout, NMASTER) && !FEATURES(v->layout, NCOLUMNS))
		return;
	if (FEATURES(v->layout, NMASTER)) {
		if (!arg)
			v->nmaster = DEFNMASTER;
		else {
			i = atoi(arg);
			if ((v->nmaster + i) < 1 ||
			    cm->wa.h / (v->nmaster + i) <= 2 * scr->style.border)
				return;
			v->nmaster += i;
		}
	} else if (FEATURES(v->layout, NCOLUMNS)) {
		if (!arg)
			v->ncolumns = DEFNCOLUMNS;
		else {
			i = atoi(arg);
			if ((v->ncolumns + i) < 1 || cm->wa.w / (v->ncolumns + i) < 100)
				return;
			v->ncolumns += i;
		}
	}
	if (sel)
		arrange(cm);
}

Client *
getclient(Window w, int part) {
	Client *c = NULL;

	XFindContext(dpy, w, context[part], (XPointer *) & c);
	return c;
}

long
getstate(Window w) {
	long ret = -1;
	long *p = NULL;
	unsigned long n;

	p = getcard(w, _XA_WM_STATE, &n);
	if (n != 0)
		ret = *p;
	if (p)
		XFree(p);
	return ret;
}

const char *
getresource(const char *resource, const char *defval) {
	static char name[256], class[256], *type;
	XrmValue value;

	snprintf(name, sizeof(name), "%s.%s", RESNAME, resource);
	snprintf(class, sizeof(class), "%s.%s", RESCLASS, resource);
	XrmGetResource(xrdb, name, class, &type, &value);
	if (value.addr)
		return value.addr;
	return defval;
}

Bool
gettextprop(Window w, Atom atom, char **text) {
	char **list = NULL, *str;
	int n;
	XTextProperty name;

	XGetTextProperty(dpy, w, &name, atom);
	if (!name.nitems)
		return False;
	if (name.encoding == XA_STRING) {
		if ((str = strdup((char *)name.value))) {
			free(*text);
			*text = str;
		}
	} else {
		if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success
		    && n > 0 && *list) {
			if ((str = strdup(*list))) {
				free(*text);
				*text = str;
			}
			XFreeStringList(list);
		}
	}
	if (name.value)
		XFree(name.value);
	return True;
}

Bool
isonmonitor(Client *c, Monitor *m)
{
	int mx, my;

	mx = c->c.x + c->c.w / 2 + c->c.b;
	my = c->c.y + c->c.h / 2 + c->c.b;

	return (m->sc.x <= mx && mx < m->sc.x + m->sc.w && m->sc.y <= my && my < m->sc.y + m->sc.h) ?
	    True : False;
}

Bool
isvisible(Client *c, Monitor *m)
{
	unsigned int i;

	if (!c)
		return False;
	if (!m) {
		for (m = scr->monitors; m; m = m->next)
			if (isvisible(c, m))
				return True;
	} else {
		for (i = 0; i < scr->ntags; i++)
			if (c->tags[i] && m->seltags[i])
				return True;
	}
	return False;
}

void
grabkeys(void) {
	unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
	unsigned int i, j;
	KeyCode code;
	XUngrabKey(dpy, AnyKey, AnyModifier, scr->root);
	for (i = 0; i < scr->nkeys; i++) {
		if ((code = XKeysymToKeycode(dpy, scr->keys[i]->keysym))) {
			for (j = 0; j < LENGTH(modifiers); j++)
				XGrabKey(dpy, code, scr->keys[i]->mod | modifiers[j], scr->root,
					 True, GrabModeAsync, GrabModeAsync);
		}
    }
}

Bool
keyrelease(XEvent *e) {
	XKeyEvent *ev;

	ev = &e->xkey;
	if (user_time == CurrentTime || (int)ev->time - (int)user_time > 0)
		user_time = ev->time;
	return True;
}

Bool
keypress(XEvent *e)
{
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XkbKeycodeToKeysym(dpy, (KeyCode) ev->keycode, 0, 0);
	for (i = 0; i < scr->nkeys; i++)
		if (keysym == scr->keys[i]->keysym
		    && CLEANMASK(scr->keys[i]->mod) == CLEANMASK(ev->state)) {
			if ((int) ev->time - (int) user_time > 0)
				user_time = ev->time;
			if (scr->keys[i]->func)
				scr->keys[i]->func(scr->keys[i]->arg);
			XUngrabKeyboard(dpy, CurrentTime);
			return True;
		}
	return False;
}

void
killclient(Client * c)
{
	int signal = SIGTERM;

	if (!c)
		return;
	if (!getclient(c->win, ClientDead)) {
		if (!getclient(c->win, ClientPing)) {
			if (checkatom(c->win, _XA_WM_PROTOCOLS, _XA_NET_WM_PING)) {
				XEvent ev;

				/* Give me a ping: one ping only.... Red October */
				ev.type = ClientMessage;
				ev.xclient.display = dpy;
				ev.xclient.window = c->win;
				ev.xclient.message_type = _XA_WM_PROTOCOLS;
				ev.xclient.format = 32;
				ev.xclient.data.l[0] = _XA_NET_WM_PING;
				ev.xclient.data.l[1] = user_time;
				ev.xclient.data.l[2] = c->win;
				ev.xclient.data.l[3] = 0;
				ev.xclient.data.l[4] = 0;
				XSendEvent(dpy, c->win, False, NoEventMask, &ev);

				XSaveContext(dpy, c->win, context[ClientPing], (XPointer) c);
			}

			if (checkatom(c->win, _XA_WM_PROTOCOLS, _XA_WM_DELETE_WINDOW)) {
				XEvent ev;

				ev.type = ClientMessage;
				ev.xclient.window = c->win;
				ev.xclient.message_type = _XA_WM_PROTOCOLS;
				ev.xclient.format = 32;
				ev.xclient.data.l[0] = _XA_WM_DELETE_WINDOW;
				ev.xclient.data.l[1] = user_time;
				ev.xclient.data.l[2] = 0;
				ev.xclient.data.l[3] = 0;
				ev.xclient.data.l[4] = 0;
				XSendEvent(dpy, c->win, False, NoEventMask, &ev);
				return;
			}
		}
	} else
		signal = SIGKILL;
	/* NOTE: Before killing the client we should attempt to kill the process
	   using the _NET_WM_PID and WM_CLIENT_MACHINE because XKillClient might
	   still leave the process hanging. Try using SIGTERM first, following
	   up with SIGKILL */
	{
		long *pids;
		unsigned long n = 0;

		pids = getcard(c->win, _XA_NET_WM_PID, &n);
		if (n == 0 && c->leader)
			pids = getcard(c->win, _XA_NET_WM_PID, &n);
		if (n > 0) {
			char hostname[64], *machine;
			pid_t pid = pids[0];

			if (gettextprop(c->win, XA_WM_CLIENT_MACHINE, &machine) || (c->leader &&
			    gettextprop(c->leader, XA_WM_CLIENT_MACHINE, &machine))) {
				if (!strncmp(hostname, machine, 64)) {
					XSaveContext(dpy, c->win, context[ClientDead], (XPointer)c);
					kill(pid, signal);
					free(machine);
					return;
				}
				free(machine);
			}
		}
	}
	XKillClient(dpy, c->win);
}

Bool
leavenotify(XEvent * e) {
	XCrossingEvent *ev = &e->xcrossing;

	if (!ev->same_screen) {
		XFindContext(dpy, ev->window, context[ScreenContext], (XPointer *)&scr);
		if (!scr->managed)
			focus(NULL);
	}
	return True;
}

void
reparentclient(Client *c, AScreen *new_scr, int x, int y)
{
	if (new_scr == scr)
		return;
	if (new_scr->managed) {
		Monitor *m;

		/* screens must have the same default depth, visuals and colormaps */

		/* some of what unmanage() does */
		detach(c);
		detachclist(c);
		detachflist(c);
		detachstack(c);
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
		if (!(m = getmonitor(x, y)))
			if (!(m = curmonitor()))
				m = nearmonitor();
		c->tags = erealloc(c->tags, scr->ntags * sizeof(*c->tags));
		memcpy(c->tags, m->seltags, scr->ntags * sizeof(*c->tags));
		attach(c, options.attachaside);
		attachclist(c);
		attachflist(c, True);
		attachstack(c);
		ewmh_update_net_client_list();
		XReparentWindow(dpy, c->frame, scr->root, x, y);
		XMoveWindow(dpy, c->frame, x, y);
		XMapWindow(dpy, c->frame);
		c->is.managed = True;
		ewmh_update_net_window_desktop(c);
		if (c->with.struts) {
			ewmh_update_net_work_area();
			updategeom(NULL);
		}
		/* caller must resize client */
	} else {
		Window win = c->win;

		unmanage(c, CauseReparented);
		XReparentWindow(dpy, win, new_scr->root, x, y);
		XMapWindow(dpy, win);
	}
}

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;
	XSetWindowAttributes twa;
	XWMHints *wmh;
	unsigned long mask = 0;
	Bool focusnew = True;
	Monitor *cm;
	int take_focus;

	DPRINTF("managing window 0x%lx\n", w);
	c = emallocz(sizeof(Client));
	c->win = w;
	c->name = ecalloc(1, 1);
	c->icon_name = ecalloc(1, 1);
	XSaveContext(dpy, c->win, context[ClientWindow], (XPointer) c);
	XSaveContext(dpy, c->win, context[ClientAny], (XPointer) c);
	XSaveContext(dpy, c->win, context[ScreenContext], (XPointer) scr);
	// c->skip.skip = 0;
	// c->is.is = 0;
	// c->with.with = 0;
	c->has.has = -1U;
	c->can.can = -1U;
	ewmh_process_net_window_type(c);
	ewmh_process_kde_net_window_type_override(c);

	c->is.icon = False;
	c->is.hidden = False;
	c->tags = ecalloc(scr->ntags, sizeof(*c->tags));
	c->r.b = c->c.b = c->is.bastard ? 0 : scr->style.border;
	/* XXX: had.border? */
	mwmh_process_motif_wm_hints(c);
	updatesizehints(c);

	updatetitle(c);
	updateiconname(c);
	applyrules(c);
	applyatoms(c);

	ewmh_process_net_window_user_time_window(c);
	ewmh_process_net_startup_id(c);

	if ((c->with.time) &&
	    (c->user_time == CurrentTime ||
	     (int) ((int) c->user_time - (int) user_time) < 0))
		focusnew = False;

	take_focus =
	    checkatom(c->win, _XA_WM_PROTOCOLS, _XA_WM_TAKE_FOCUS) ? TAKE_FOCUS : 0;
	c->can.focus = take_focus | GIVE_FOCUS;

	/* FIXME: we aren't check whether the client requests to be mapped in the
	   IconicState or NormalState here, and we should.  It appears that previous code 
	   base was simply mapping all windows in the NormalState.  This explains some
	   ugliness.  We already filtered out WithdrawnState (the very old DontCareState) 
	   during the scan.  We should check StateHint flag and set is.icon and
	   setwmstate() at the end of this sequence (clients use appearance of WM_STATE
	   to indicate mapping). */

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (wmh->flags & InputHint)
			c->can.focus = take_focus | (wmh->input ? GIVE_FOCUS : 0);
		c->is.attn = (wmh->flags & XUrgencyHint) ? True : False;
		if ((wmh->flags & WindowGroupHint) &&
		    (c->leader = wmh->window_group) != None) {
			updategroup(c, c->leader, ClientGroup, &c->nonmodal);
		}
		XFree(wmh);
	}

	/* do this before transients */
	wmh_process_win_window_hints(c);

	if (XGetTransientForHint(dpy, w, &trans) || c->is.grptrans) {
		if (trans == None || trans == scr->root) {
			trans = c->leader;
			c->is.grptrans = True;
			if (!trans)
				trans = scr->root;
		}
		if ((c->transfor = trans) != None) {
			if (c->is.grptrans)
				updategroup(c, trans, ClientTransForGroup, &c->nonmodal);
			else
				updategroup(c, trans, ClientTransFor, &c->nonmodal);
			if (!(t = getclient(trans, ClientWindow)) && trans == c->leader) {
				Window *group;
				unsigned int i, m = 0;

				if ((group = getgroup(c, c->leader, ClientGroup, &m))) {
					for (i = 0; i < m; i++) {
						trans = group[i];
						if ((t = getclient(trans, ClientWindow)))
							break;
					}
				}
			} else if (t) {
				/* Transients are not allowed to perform some actions
				   when window for which they are are transient for is
				   also managed.  ICCCM 2.0 says such applications should 
				   act on the non-transient managed client and let the
				   window manager decide what to do about the transients. 
				 */
				c->can.min = False;	/* can't (de)iconify */
				c->can.hide = False;	/* can't hide or show */
				c->can.tag = False;	/* can't change desktops */
			}
			if (t)
				memcpy(c->tags, t->tags, scr->ntags * sizeof(*c->tags));
		}
		c->is.transient = True;
		c->is.floater = True;
	} else
		c->is.transient = False;

	if (c->is.attn)
		focusnew = True;
	if (!canfocus(c))
		focusnew = False;

	if (!c->is.floater)
		c->is.floater = (!c->can.sizeh || !c->can.sizev);

	if (c->has.title)
		c->th = scr->style.titleheight;
	else {
		c->can.shade = False;
		c->has.grips = False;
	}
	if (c->has.grips)
		if (!(c->gh = scr->style.gripsheight))
			c->has.grips = False;

	c->c.x = c->r.x = wa->x;
	c->c.y = c->r.y = wa->y;
	c->c.w = c->r.w = wa->width;
	c->c.h = c->r.h = wa->height + c->th + c->gh;

	c->s.x = wa->x;
	c->s.y = wa->y;
	c->s.w = wa->width;
	c->s.h = wa->height;
	c->s.b = wa->border_width;

	CPRINTF(c, "initial geometry c: %dx%d+%d+%d:%d t %d g %d\n",
			c->c.w, c->c.h, c->c.x, c->c.y, c->c.b, c->th, c->gh);
	CPRINTF(c, "initial geometry r: %dx%d+%d+%d:%d t %d g %d\n",
			c->r.w, c->r.h, c->r.x, c->r.y, c->r.b, c->th, c->gh);
	CPRINTF(c, "initial geometry s: %dx%d+%d+%d:%d t %d g %d\n",
			c->s.w, c->s.h, c->s.x, c->s.y, c->s.b, c->th, c->gh);

	if (!wa->x && !wa->y && c->can.move) {
		/* put it on the monitor startup notification requested if not already
		   placed with its group */
		if (c->monitor && !clientmonitor(c))
			memcpy(c->tags, scr->monitors[c->monitor - 1].seltags,
			       scr->ntags * sizeof(*c->tags));
		place(c, CascadePlacement);
	}

	CPRINTF(c, "placed geometry c: %dx%d+%d+%d:%d t %d g %d\n",
			c->c.w, c->c.h, c->c.x, c->c.y, c->c.b, c->th, c->gh);
	CPRINTF(c, "placed geometry r: %dx%d+%d+%d:%d t %d g %d\n",
			c->r.w, c->r.h, c->r.x, c->r.y, c->r.b, c->th, c->gh);
	CPRINTF(c, "placed geometry s: %dx%d+%d+%d:%d t %d g %d\n",
			c->s.w, c->s.h, c->s.x, c->s.y, c->s.b, c->th, c->gh);

	c->with.struts = getstruts(c);

	if (!c->can.move) {
		int rx, ry;

		getreference(&rx, &ry, (Geometry *)wa, wa->win_gravity);
		if (!(cm = getmonitor(rx, ry)))
			cm = closestmonitor(rx, ry);
		memcpy(c->tags, cm->seltags, scr->ntags * sizeof(*c->tags));
	}

	XGrabButton(dpy, AnyButton, AnyModifier, c->win, True,
		    ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
	twa.override_redirect = True;
	twa.event_mask = FRAMEMASK;
	mask = CWOverrideRedirect | CWEventMask;
	if (wa->depth == 32) {
		mask |= CWColormap | CWBorderPixel | CWBackPixel;
		twa.colormap = XCreateColormap(dpy, scr->root, wa->visual, AllocNone);
		twa.background_pixel = BlackPixel(dpy, scr->screen);
		twa.border_pixel = BlackPixel(dpy, scr->screen);
	}
	c->frame =
	    XCreateWindow(dpy, scr->root, c->c.x, c->c.y, c->c.w,
			  c->c.h, c->c.b, wa->depth == 32 ? 32 :
			  DefaultDepth(dpy, scr->screen),
			  InputOutput, wa->depth == 32 ? wa->visual :
			  DefaultVisual(dpy, scr->screen), mask, &twa);
	XSaveContext(dpy, c->frame, context[ClientFrame], (XPointer) c);
	XSaveContext(dpy, c->frame, context[ClientAny], (XPointer) c);
	XSaveContext(dpy, c->frame, context[ScreenContext], (XPointer) scr);

	wc.border_width = c->c.b;
	XConfigureWindow(dpy, c->frame, CWBorderWidth, &wc);
	send_configurenotify(c, None);
	XSetWindowBorder(dpy, c->frame, scr->style.color.norm[ColBorder]);

	twa.event_mask = ExposureMask | MOUSEMASK;
	/* we create title as root's child as a workaround for 32bit visuals */
	if (c->has.title) {
		c->element = ecalloc(LastElement, sizeof(*c->element));
		c->title = XCreateWindow(dpy, scr->root, 0, 0, c->c.w, c->th,
					 0, DefaultDepth(dpy, scr->screen),
					 CopyFromParent, DefaultVisual(dpy, scr->screen),
					 CWEventMask, &twa);
		XSaveContext(dpy, c->title, context[ClientTitle], (XPointer) c);
		XSaveContext(dpy, c->title, context[ClientAny], (XPointer) c);
		XSaveContext(dpy, c->title, context[ScreenContext], (XPointer) scr);
	}
	if (c->has.grips) {
		c->grips = XCreateWindow(dpy, scr->root, 0, 0, c->c.w, c->gh,
					 0, DefaultDepth(dpy, scr->screen),
					 CopyFromParent, DefaultVisual(dpy, scr->screen),
					 CWEventMask, &twa);
		XSaveContext(dpy, c->grips, context[ClientGrips], (XPointer) c);
		XSaveContext(dpy, c->grips, context[ClientAny], (XPointer) c);
		XSaveContext(dpy, c->grips, context[ScreenContext], (XPointer) scr);

	}

	attach(c, options.attachaside);
	attachclist(c);
	attachflist(c, False);
	attachstack(c);
	ewmh_update_net_client_list();

	twa.event_mask = CLIENTMASK;
	twa.do_not_propagate_mask = CLIENTNOPROPAGATEMASK;
	XChangeWindowAttributes(dpy, c->win, CWEventMask | CWDontPropagate, &twa);
	XSelectInput(dpy, c->win, CLIENTMASK);

	XReparentWindow(dpy, c->win, c->frame, 0, c->th);
	if (c->grips)
		XReparentWindow(dpy, c->grips, c->frame, 0, c->c.h - c->gh);
	if (c->title)
		XReparentWindow(dpy, c->title, c->frame, 0, 0);
	XAddToSaveSet(dpy, c->win);
	XMapWindow(dpy, c->win);
	wc.border_width = 0;
	XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);

	/* FIXME: wrong in so may ways.  ban() unmaps the c->win we just mapped after
	   reparenting. It also unmaps an already unmapped c->frame.  What's worse is it
	   sets the WM_STATE to the IconicState and can confuse the heck out of clients
	   that requested being mapped in the NormalState. */

	ban(c);

	ewmh_process_net_window_desktop(c);
	ewmh_process_net_window_desktop_mask(c);
	ewmh_process_net_window_sync_request_counter(c);
	ewmh_process_net_window_state(c);
	c->is.managed = True;
	ewmh_update_net_window_desktop(c);

	XMoveResizeWindow(dpy, c->win, 0, c->th, c->c.w, c->c.h - c->th - c->gh);
	if (c->grips && c->gh) {
		XMoveResizeWindow(dpy, c->grips, 0, c->c.h - c->gh, c->c.w, c->gh);
		XMapWindow(dpy, c->grips);
	}
	if (c->title && c->th) {
		XMoveResizeWindow(dpy, c->title, 0, 0, c->c.w, c->th);
		XMapWindow(dpy, c->title);
	}
	if ((c->grips && c->gh) || (c->title && c->th))
		drawclient(c);

	if (c->with.struts) {
		ewmh_update_net_work_area();
		updategeom(NULL);
	}
	XSync(dpy, False);
	if (!c->is.bastard && (focusnew || (canfocus(c) && !canfocus(sel)))) {
		DPRINTF
		    ("Focusing newly managed %sclient: frame 0x%08lx win 0x%08lx name %s\n",
		     c->is.bastard ? "bastard " : "", c->frame, c->win, c->name);
		arrange(NULL);
		focus(c);
	} else {
		DPRINTF
		    ("Lowering newly managed %sclient: frame 0x%08lx win 0x%08lx name %s\n",
		     c->is.bastard ? "bastard " : "", c->frame, c->win, c->name);
		restack_belowif(c, sel);
		arrange(NULL);
		focus(sel);
	}
}

Bool
mappingnotify(XEvent * e) {
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
	return True;
}

Bool
maprequest(XEvent * e)
{
	static XWindowAttributes wa;
	Client *c;
	XMapRequestEvent *ev = &e->xmaprequest;

	if (!XGetWindowAttributes(dpy, ev->window, &wa))
		return True;
	if (wa.override_redirect)
		return True;
	if (issystray(ev->window))
		return True;
	if (isdockapp(ev->window))
		return True;
	if (!(c = getclient(ev->window, ClientWindow))) {
		manage(ev->window, &wa);
		return True;
	}
	return False;
}

void
getworkarea(Monitor * m, Workarea *w) {
	w->x = max(m->wa.x, 1);
	w->y = max(m->wa.y, 1);
	w->w = min(m->wa.x + m->wa.w, DisplayWidth(dpy, scr->screen) - 1) - w->x;
	w->h = min(m->wa.y + m->wa.h, DisplayHeight(dpy, scr->screen) - 1) - w->y;
}

void
monocle(Monitor * m) {
	Client *c;
	Workarea w;
	View *v;

	getworkarea(m, &w);
	v = &scr->views[m->curtag];
	for (c = nexttiled(scr->clients, m); c; c = nexttiled(c->next, m)) {
		ClientGeometry g;

		memcpy(&g, &w, sizeof(w));
		g.b = c->c.b;
		g.w -= 2 * g.b;
		g.h -= 2 * g.b;
		g.t = (v->dectiled && c->has.title) ? scr->style.titleheight : 0;
		g.g = (v->dectiled && c->has.grips) ? scr->style.gripsheight : 0;

		DPRINTF("CALLING resize()\n");
		resize(c, &g);
	}
}

void
grid(Monitor *m) {
	Client *c;
	Workarea wa;
	View *v;
	int rows, cols, n, i, *rc, *rh, col, row;
	int cw, cl, *rl;
	int gap;

	if (!(c = nexttiled(scr->clients, m)))
		return;

	v = &scr->views[m->curtag];

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

	gap = scr->style.margin > scr->style.border ? scr->style.margin - scr->style.border : 0;

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
		DPRINTF("CALLING resize()\n");
		resize(c, &n);
	}
	free(rc);
	free(rh);
	free(rl);
}

void
moveresizekb(Client * c, int dx, int dy, int dw, int dh) {
	Monitor *m;

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

		getgeometry(c, &c->c, &g);

		if (dw && (dw < c->incw))
			dw = (dw / abs(dw)) * c->incw;
		if (dh && (dh < c->inch))
			dh = (dh / abs(dh)) * c->inch;
		g.w += dw;
		g.h += dh;
		DPRINTF("CALLING: constrain()\n");
		constrain(c, &g);
		if (g.w != c->c.w || g.h != c->c.h) {
			c->is.max = False;
			c->is.maxv = False;
			c->is.maxh = False;
			c->is.fill = False;
			ewmh_update_net_window_state(c);
		}
		c->r.x = g.x + dx;
		c->r.y = g.y + dy;
		c->r.w = g.w;
		c->r.h = g.h;
	} if (dx || dy) {
		c->r.x += dx;
		c->r.y += dy;
	}
	updatefloat(c, NULL);
}

void
getpointer(int *x, int *y) {
	int di;
	unsigned int dui;
	Window dummy;

	XQueryPointer(dpy, scr->root, &dummy, &dummy, x, y, &di, &di, &dui);
}

Monitor *
getmonitor(int x, int y) {
	Monitor *m;

	for (m = scr->monitors; m; m = m->next) {
		if ((x >= m->sc.x && x <= m->sc.x + m->sc.w) &&
		    (y >= m->sc.y && y <= m->sc.y + m->sc.h))
			return m;
	}
	return NULL;
}

static int
segm_overlap(int min1, int max1, int min2, int max2) {
	int tmp, res = 0;

	if (min1 > max1) { tmp = min1; min1 = max1; max1 = tmp; }
	if (min2 > max2) { tmp = min2; min2 = max2; max2 = tmp; }
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
wind_overlap(int min1, int max1, int min2, int max2) {
	return segm_overlap(min1, max1, min2, max2) ? True : False;
}

static int
area_overlap(int xmin1, int ymin1, int xmax1, int ymax1,
	     int xmin2, int ymin2, int xmax2, int ymax2)
{
	int w = 0, h = 0;

	w = segm_overlap(xmin1, xmax1, xmin2, xmax2);
	h = segm_overlap(ymin1, ymax1, ymin2, ymax2);

	return (w && h) ? (w*h) : 0;
}

Monitor *
bestmonitor(int xmin, int ymin, int xmax, int ymax) {
	int a, area = 0;
	Monitor *m, *best = NULL;

	for (m = scr->monitors; m; m = m->next) {
		if ((a = area_overlap(xmin, ymin, xmax, ymax,
			m->sc.x, m->sc.y, m->sc.x + m->sc.w, m->sc.y + m->sc.h)) > area) {
			area = a;
			best = m;
		}
	}
	return best;
}

Monitor *
findmonitor(Client *c)
{
	int xmin, xmax, ymin, ymax;

	xmin = c->r.x;
	xmax = c->r.x + c->r.w + 2 * c->r.b;
	ymin = c->r.y;
	ymax = c->r.y + c->r.h + 2 * c->r.b;

	return bestmonitor(xmin, ymin, xmax, ymax);
}

Monitor *
findcurmonitor(Client *c)
{
	int xmin, xmax, ymin, ymax;

	xmin = c->c.x;
	xmax = c->c.x + c->c.w + 2 * c->c.b;
	ymin = c->c.y;
	ymax = c->c.y + c->c.h + 2 * c->c.b;

	return bestmonitor(xmin, ymin, xmax, ymax);
}

Monitor *
clientmonitor(Client * c) {
	Monitor *m;

	assert(c != NULL);
	for (m = scr->monitors; m; m = m->next)
		if (isvisible(c, m))
			return m;
	return NULL;
}

Layout *
clientlayout(Client *c) {
	Monitor *m;
	int i;

	assert(c != NULL);
	for (m = scr->monitors; m; m = m->next)
		if (isvisible(c, m))
			return M2LT(m);
	for (i = 0; i < scr->ntags; i++)
		if (c->tags[i])
			return scr->views[i].layout;
	return NULL;
}

Monitor *
curmonitor() {
	int x, y;
	getpointer(&x, &y);
	return getmonitor(x, y);
}

Monitor *
selmonitor()
{
	return (sel ? sel->curmon : curmonitor());
}


Monitor *
closestmonitor(int x, int y)
{
	Monitor *m, *near = scr->monitors;
	float mind = hypotf(DisplayHeight(dpy, scr->screen), DisplayWidth(dpy, scr->screen));

	for (m = scr->monitors; m; m = m->next) {
		float fd = hypotf(m->mx - x, m->my - y);

		if (fd < mind) {
			mind = fd;
			near = m;
		}
	}
	return near;
}

Monitor *
nearmonitor() {
	Monitor *m;
	int x, y;

	getpointer(&x, &y);
	if (!(m = getmonitor(x, y)))
		m = closestmonitor(x, y);
	return m;
}

#ifdef SYNC
void
sync_request(Client *c, Time time) {
	int overflow = 0;
	XEvent ce;
	XSyncValue inc;
	XSyncAlarmAttributes aa;

	XSyncIntToValue(&inc, 1);
	XSyncValueAdd(&c->sync.val, c->sync.val, inc, &overflow);
	if (overflow)
		XSyncMinValue(&c->sync.val);

	CPRINTF(c, "Arming alarm 0x%08lx\n", c->sync.alarm);
	aa.trigger.counter = c->sync.counter;
	aa.trigger.wait_value = c->sync.val;
	aa.trigger.value_type = XSyncAbsolute;
	aa.trigger.test_type = XSyncPositiveComparison;
	aa.events = True;

	XSyncChangeAlarm(dpy, c->sync.alarm,
		XSyncCACounter|XSyncCAValueType|XSyncCAValue|XSyncCATestType|
		XSyncCAEvents, &aa);

	CPRINTF(c, "%s", "Sending client meessage\n");
	ce.xclient.type = ClientMessage;
	ce.xclient.message_type = _XA_WM_PROTOCOLS;
	ce.xclient.display = dpy;
	ce.xclient.window = c->win;
	ce.xclient.format = 32;
	ce.xclient.data.l[0] = _XA_NET_WM_SYNC_REQUEST;
	ce.xclient.data.l[1] = time;
	ce.xclient.data.l[2] = XSyncValueLow32(c->sync.val);
	ce.xclient.data.l[3] = XSyncValueHigh32(c->sync.val);
	ce.xclient.data.l[4] = 0;
	XSendEvent(dpy, c->win, False, NoEventMask, &ce);

	c->sync.waiting = True;
}

Bool
newsize(Client *c, int w, int h, Time time)
{
	if (!c->sync.alarm)
		return True;
	if (c->sync.waiting) {
		DPRINTF("Deferring size request from %dx%d to %dx%d for 0x%08lx 0x%08lx %s\n",
				c->c.w, c->c.h - c->th - c->gh, w, h, c->frame, c->win, c->name);
		return False;
	}
	c->sync.w = w;
	c->sync.h = h;
	if (time == CurrentTime)
		time = user_time;
	sync_request(c, time);
	return True;
}

Bool
alarmnotify(XEvent *e)
{
	XSyncAlarmNotifyEvent *ae = (typeof(ae)) e;
	Client *c;
	XWindowChanges wc;
	unsigned mask = 0;

	if (!(c = getclient(ae->alarm, ClientSync))) {
		XPRINTF("Recevied alarm notify for unknown alarm 0x%08lx\n", ae->alarm);
		return False;
	}
	CPRINTF(c, "alarm notify on 0x%08lx\n", ae->alarm);
	if (!c->sync.waiting) {
		DPRINTF("%s", "Alarm was cancelled!\n");
		return True;
	}
	c->sync.waiting = False;

	if ((wc.width = c->c.w) != c->sync.w) {
		XPRINTF("Width changed from %d to %u since last request\n", c->sync.w, wc.width);
		mask |= CWWidth;
	}
	if ((wc.height = c->c.h - c->th - c->gh) != c->sync.h) {
		XPRINTF("Height changed from %d to %u since last request\n", c->sync.h, wc.height);
		mask |= CWHeight;
	}
	if (mask && newsize(c, wc.width, wc.height, ae->time)) {
		DPRINTF("Configuring window %ux%u\n", wc.width, wc.height);
		XConfigureWindow(dpy, c->win, mask, &wc);
	}
	return True;
}
#else

Bool
newsize(Client *c, int w, int h, Time time)
{
	return True;
}

#endif

/* FIXME: this does not handle moving the window across monitor
 * or desktop boundaries. */

void
resize(Client *c, ClientGeometry * n)
{
	XWindowChanges wwc, fwc;
	unsigned wmask, fmask;
	Bool tchange = False, gchange = False;

	if (n->w <= 0 || n->h <= 0)
		return;
	/* offscreen appearance fixes */
	if (n->x > DisplayWidth(dpy, scr->screen))
		n->x = DisplayWidth(dpy, scr->screen) - n->w - 2 * n->b;
	if (n->y > DisplayHeight(dpy, scr->screen))
		n->y = DisplayHeight(dpy, scr->screen) - n->h - 2 * n->b;
	DPRINTF("x = %d y = %d w = %d h = %d b = %d t = %d g = %d\n", n->x, n->y, n->w,
		n->h, n->b, n->t, n->g);
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
	if (n->t && (c->is.shaded && (c != sel || !options.autoroll))) {
		fwc.height = n->t;
		DPRINTF("frame wc.h = %u\n", fwc.height);
		fmask |= CWHeight;
	} else {
		fwc.height = n->h;
		DPRINTF("frame wc.h = %u\n", fwc.height);
		fmask |= CWHeight;
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
	if (c->grips && (gchange || ((wmask | fmask) & (CWWidth | CWHeight | CWY)))) {
		if (gchange) {
			if (n->g)
				XMapWindow(dpy, c->grips);
			else
				XUnmapWindow(dpy, c->grips);
		}
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

/* TODO: handle movement across EWMH desktops */

static Bool wind_overlap(int min1, int max1, int min2, int max2);

void
mousemove(Client *c, XEvent *e)
{
	int nx2, ny2;
	int x_root, y_root;
	int moved = 0;
	unsigned int i;
	Monitor *m, *nm;
	ClientGeometry g;
	Geometry n, o;

	if (!c->can.move) {
		XUngrabPointer(dpy, CurrentTime);
		return;
	}
	x_root = e->xbutton.x_root;
	y_root = e->xbutton.y_root;

	/* The monitor that we clicked in the window from and the monitor in which the
	   window was last laid out can be two different places for floating windows
	   (i.e. a window that overlaps the boundary between two monitors.  Because we
	   handle changing monitors and even changing screens here, we should use the
	   monitor to which it was last associated instead of where it was clicked.  This 
	   is actually important because the monitor it is on might be floating and the
	   monitor the edge was clicked on might be tiled. */

	if (!(m = getmonitor(x_root, y_root)) || (c->curmon && m != c->curmon))
		if (!(m = c->curmon)) {
			XUngrabPointer(dpy, CurrentTime);
			return;
		}
	if (XGrabPointer(dpy, scr->root, False, MOUSEMASK, GrabModeAsync,
			 GrabModeAsync, None, cursor[CurMove],
			 CurrentTime) != GrabSuccess)
		return;
	c->was.is = 0;
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
	/* If the cursor is not over the window move the window under the curor instead
	   of warping the pointer. */
	if (x_root < c->r.x || c->r.x + c->r.w < x_root) {
		c->r.x = x_root - c->r.w / 2 - c->r.b;
		moved = 1;
	}
	if (y_root < c->r.y || c->r.y + c->r.h < y_root) {
		c->r.y = y_root - c->r.h / 2 - c->r.b;
		moved = 1;
	}
	if (c->was.is || moved)
		updatefloat(c, m);
	getgeometry(c, &c->c, &g);
	o = c->c;
	n = c->c;
	for (;;) {
		Workarea w;
		unsigned int snap;
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
					g.x = o.x;
					g.y = o.y;
					DPRINTF("CALLING resize()\n");
					resize(c, &g);
					save(c);
					break;
				}
				continue;
			}
			scr = event_scr;
			handle_event(&ev);
			continue;
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			scr = event_scr;
			handle_event(&ev);
			continue;
		case MotionNotify:
			XSync(dpy, False);
			/* we are probably moving to a different monitor */
			if (!(nm = getmonitor(ev.xmotion.x_root, ev.xmotion.y_root)))
				continue;
			getworkarea(nm, &w);
			n.x = o.x + (ev.xmotion.x_root - x_root);
			n.y = o.y + (ev.xmotion.y_root - y_root);
			nx2 = n.x + c->c.w + 2 * c->c.b;
			ny2 = n.y + c->c.h + 2 * c->c.b;
			if ((snap = (ev.xmotion.state & ControlMask) ? 0 : options.snap)) {
				if (abs(n.x - w.x) < snap)
					n.x += w.x - n.x;
				else if (abs(nx2 - (w.x + w.w)) < snap)
					n.x += (w.x + w.w) - nx2;
				else
					for (s = event_scr->stack; s; s = s->snext) {
						int sx = s->c.x;
						int sy = s->c.y;
						int sx2 = s->c.x + s->c.w + 2 * s->c.b;
						int sy2 = s->c.y + s->c.h + 2 * s->c.b;

						if (wind_overlap(n.y, ny2, sy, sy2)) {
							if (abs(n.x - sx) < snap)
								n.x += sx - n.x;
							else if (abs(nx2 - sx2) < snap)
								n.x += sx2 - nx2;
							else
								continue;
							break;
						}
					}
				if (abs(n.y - w.y) < snap)
					n.y += w.y - n.y;
				else if (abs(ny2 - (w.y + w.h)) < snap)
					n.y += (w.y + w.h) - ny2;
				else
					for (s = event_scr->stack; s; s = s->snext) {
						int sx = s->c.x;
						int sy = s->c.y;
						int sx2 = s->c.x + s->c.w + 2 * s->c.b;
						int sy2 = s->c.y + s->c.h + 2 * s->c.b;

						if (wind_overlap(n.x, nx2, sx, sx2)) {
							if (abs(n.y - sy) < snap)
								n.y += sy - n.y;
							else if (abs(ny2 - sy2) < snap)
								n.y += sy2 - ny2;
							else
								continue;
							break;
						}
					}
			}
			if (event_scr != scr)
				reparentclient(c, event_scr, n.x, n.y);
			g.x = n.x;
			g.y = n.y;
			DPRINTF("CALLING resize()\n");
			resize(c, &g);
			save(c);
			if (m != nm) {
				for (i = 0; i < scr->ntags; i++)
					c->tags[i] = nm->seltags[i];
				ewmh_update_net_window_desktop(c);
				drawclient(c);
				arrange(NULL);
				m = nm;
			}
			continue;
		default:
			scr = event_scr;
			handle_event(&ev);
			continue;
		}
		XUngrabPointer(dpy, CurrentTime);
		break;
	}
	if (c->was.is) {
		if (c->was.max)
			c->is.max = True;
		if (c->was.maxv)
			c->is.maxv = True;
		if (c->was.maxh)
			c->is.maxh = True;
		if (c->was.fill)
			c->is.fill = True;
		if (c->was.shaded)
			c->is.shaded = True;
		updatefloat(c, m);
	}
	discardenter();
	ewmh_update_net_window_state(c);
}
void
m_move(Client *c, XEvent *e)
{
	if (!isfloating(c, c->curmon))
		togglefloating(c);
	mousemove(c, e);
}

int
findcorner(Client *c, int x_root, int y_root)
{
	int cx, cy, from;
	float dx, dy;

	cx = c->c.x + c->c.w / 2;
	cy = c->c.y + c->c.h / 2;
	dx = (float) abs(cx - x_root)/ (float) c->c.w;
	dy = (float) abs(cy - y_root)/ (float) c->c.h;

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

void
mouseresize_from(Client *c, int from, XEvent *e)
{
	int dx, dy;
	int x_root, y_root;
	Monitor *m, *nm;
	ClientGeometry n, o;

	if (!c->can.size ||
	    ((from == CurResizeTop || from == CurResizeBottom) && !c->can.sizev) ||
	    ((from == CurResizeLeft || from == CurResizeRight) && !c->can.sizeh)) {
		XUngrabPointer(dpy, CurrentTime);
		return;
	}

	x_root = e->xbutton.x_root;
	y_root = e->xbutton.y_root;

	if (!(m = getmonitor(x_root, y_root)) || (c->curmon && m != c->curmon))
		if (!(m = c->curmon)) {
			XUngrabPointer(dpy, CurrentTime);
			return;
		}

	getgeometry(c, &c->c, &o);
	getgeometry(c, &c->c, &n);

	if (XGrabPointer(dpy, scr->root, False, MOUSEMASK, GrabModeAsync,
			 GrabModeAsync, None, cursor[from], CurrentTime) != GrabSuccess)
		return;
	c->was.is = 0;
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
	if (c->was.is)
		updatefloat(c, m);
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
					DPRINTF("CALLING resize()\n");
					resize(c, &o);
					save(c);
					break;
				}
				continue;
			}
			scr = event_scr;
			handle_event(&ev);
			continue;
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			scr = event_scr;
			handle_event(&ev);
			continue;
		case MotionNotify:
			if (event_scr != scr)
				continue;
			XSync(dpy, False);
			dx = (x_root - ev.xmotion.x_root);
			dy = (y_root - ev.xmotion.y_root);
			user_time = ev.xmotion.time;
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
			if ((snap = (ev.xmotion.state & ControlMask) ? 0 : options.snap)) {
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
			DPRINTF("CALLING resize()\n");
			resize(c, &n);
			save(c);
			continue;
		default:
			scr = event_scr;
			handle_event(&ev);
			continue;
		}
		XUngrabPointer(dpy, CurrentTime);
		break;
	}
	if (c->was.shaded) {
		c->is.shaded = True;
		updatefloat(c, m);
	}
	discardenter();
	ewmh_update_net_window_state(c);
}

void
mouseresize(Client *c, XEvent *e)
{
	int from;

	if (!c->can.size || (!c->can.sizeh && !c->can.sizev)) {
		XUngrabPointer(dpy, CurrentTime);
		return;
	}
	from = findcorner(c, e->xbutton.x_root, e->xbutton.y_root);
	mouseresize_from(c, from, e);
}

void
m_resize(Client *c, XEvent *e)
{
	if (!isfloating(c, c->curmon))
		togglefloating(c);
	mouseresize(c, e);
}

Client *
nexttiled(Client *c, Monitor *m) {
	for (; c && (c->is.floater || c->skip.arrange || !isvisible(c, m) || c->is.bastard
		     || (c->is.icon || c->is.hidden)); c = c->next) ;
	return c;
}

Client *
prevtiled(Client *c, Monitor *m) {
	for (; c && (c->is.floater || c->skip.arrange || !isvisible(c, m) || c->is.bastard
		     || (c->is.icon || c->is.hidden)); c = c->prev) ;
	return c;
}

Bool
reparentnotify(XEvent * e)
{
	Client *c;
	XReparentEvent *ev = &e->xreparent;

	if ((c = getclient(ev->window, ClientWindow))) {
		if (ev->parent != c->frame) {
			DPRINTF("unmanage reparented window (%s)\n", c->name);
			unmanage(c, CauseReparented);
		}
		return True;
	}
	return False;
}

void
getplace(Client *c, ClientGeometry *g)
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

int
total_overlap(Client *c, Monitor *m, Geometry *g) {
	Client *o;
	int x1, x2, y1, y2, a;

	x1 = g->x;
	x2 = g->x + g->w + 2 * g->b;
	y1 = g->y;
	y2 = g->y + g->h + 2 * g->b + c->th + c->gh;
	a = 0;

	for (o = scr->clients; o; o = o->next)
		if (o != c && !o->is.bastard && isfloating(o, m) && isvisible(o, m))
			a += area_overlap(x1, y1, x2, y2,
					  o->c.x, o->c.y,
					  o->c.x + o->c.w + 2 * o->c.b,
					  o->c.y + o->c.h + 2 * o->c.b);
	return a;
}

void
place_smart(Client *c, WindowPlacement p, ClientGeometry *g, Monitor *m, Workarea *w)
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

				if ((a = total_overlap(c, m, (Geometry *)g)) == 0)
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

				if ((a = total_overlap(c, m, (Geometry *)g)) == 0)
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

void
place_minoverlap(Client *c, WindowPlacement p, ClientGeometry *g, Monitor *m, Workarea *w)
{
}

void
place_undermouse(Client *c, WindowPlacement p, ClientGeometry *g, Monitor *m, Workarea *w)
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

	putreference(g->x, g->y, (Geometry *)g, c->gravity);

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

void
place_random(Client *c, WindowPlacement p, ClientGeometry *g, Monitor *m, Workarea *w)
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

void
place(Client * c, WindowPlacement p)
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

Bool
propertynotify(XEvent * e)
{
	Client *c;
	Window trans = None;
	XPropertyEvent *ev = &e->xproperty;

	if ((c = getclient(ev->window, ClientWindow))) {
		if (ev->atom == _XA_NET_WM_STRUT_PARTIAL || ev->atom == _XA_NET_WM_STRUT) {
			c->with.struts = getstruts(c);
			updatestruts();
		}
		if (ev->state == PropertyDelete) 
			return True;
		if (ev->atom <= XA_LAST_PREDEFINED) {
			switch (ev->atom) {
			case XA_WM_TRANSIENT_FOR:
				if (XGetTransientForHint(dpy, c->win, &trans) &&
				    trans == None)
					trans = scr->root;
				if (!c->is.floater &&
				    (c->is.floater = (trans != None))) {
					arrange(NULL);
					ewmh_update_net_window_state(c);
				}
				break;
			case XA_WM_NORMAL_HINTS:
				updatesizehints(c);
				break;
			case XA_WM_NAME:
				updatetitle(c);
				drawclient(c);
				break;
			case XA_WM_ICON_NAME:
				updateiconname(c);
				break;
			default:
				return False;
			}
		} else {
			if (0) {
			} else if (ev->atom == _XA_NET_WM_NAME) {
				updatetitle(c);
				drawclient(c);
			} else if (ev->atom == _XA_NET_WM_ICON_NAME) {
				updateiconname(c);
			} else if (ev->atom == _XA_NET_WM_WINDOW_TYPE) {
				/* TODO */
			} else if (ev->atom == _XA_NET_WM_USER_TIME) {
				ewmh_process_net_window_user_time(c);
			} else if (ev->atom == _XA_NET_WM_USER_TIME_WINDOW) {
				ewmh_process_net_window_user_time_window(c);
			} else if (ev->atom == _XA_NET_WM_SYNC_REQUEST_COUNTER) {
				/* TODO */
			} else if (ev->atom == _XA_WIN_HINTS) {
				wmh_process_win_window_hints(c);
			} else
				return False;
		}
	} else if ((c = getclient(ev->window, ClientTimeWindow))) {
		if (ev->atom > XA_LAST_PREDEFINED) {
			if (0) {
			} else if (ev->atom == _XA_NET_WM_USER_TIME) {
				ewmh_process_net_window_user_time(c);
			} else if (ev->atom == _XA_NET_WM_USER_TIME_WINDOW) {
				ewmh_process_net_window_user_time_window(c);
			} else
				return False;
		} else
			return False;
	} else if (ev->window == scr->root) {
		if (ev->atom > XA_LAST_PREDEFINED) {
			if (0) {
			} else if (ev->atom == _XA_NET_DESKTOP_NAMES) {
				ewmh_process_net_desktop_names();
			} else if (ev->atom == _XA_NET_DESKTOP_LAYOUT) {
				/* TODO */
			} else
				return False;
		} else
			return False;
	} else
		return False;
	return True;
}

void
quit(const char *arg) {
	running = False;
	if (arg) {
		cleanup(CauseSwitching);
		execlp("sh", "sh", "-c", arg, NULL);
		eprint("Can't exec '%s': %s\n", arg, strerror(errno));
	}
}

void
restart(const char *arg) {
	running = False;
	if (arg) {
		cleanup(CauseSwitching);
		execlp("sh", "sh", "-c", arg, NULL);
		eprint("Can't exec '%s': %s\n", arg, strerror(errno));
	} else {
		char **argv;
		int i;

		/* argv must be NULL terminated and writable */
		argv = calloc(cargc+1, sizeof(*argv));
		for (i = 0; i < cargc; i++)
			argv[i] = strdup(cargv[i]);

		cleanup(CauseRestarting);
		execvp(argv[0], argv);
		eprint("Can't restart: %s\n", strerror(errno));
	}
}

Bool
constrain(Client *c, ClientGeometry *g)
{
	int w = g->w, h = g->h;
	Bool ret = False;

	CPRINTF(c, "geometry before constraint: %dx%d+%d+%d:%d\n",
			g->w, g->h, g->x, g->y, g->b);

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
	CPRINTF(c, "geometry after constraints: %dx%d+%d+%d:%d\n",
			g->w, g->h, g->x, g->y, g->b);
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
			cl[i] = NULL; wl[j] = c->frame; sl[j] = c; j++;
			XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		}
	}
	/* windows of type _NET_WM_TYPE_DOCK (unless they have state _NET_WM_STATE_BELOW) and
	   windows having state _NET_WM_STATE_ABOVE. */
	XPRINTF("%s", "LAYER: _NET_WINDOW_TYPE_DOCK not _NET_WM_STATE_BELOW and _NET_WM_STATE_ABOVE:\n");
	for (i = 0; i < n; i++) {
		if (!(c = cl[i]))
			continue;
		if ((WTCHECK(c, WindowTypeDock) && !c->is.below) || c->is.above) {
			cl[i] = NULL; wl[j] = c->frame; sl[j] = c; j++;
			XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		}
	}
	/* windows not belonging in any other layer (but we put floating above special above tiled) 
	 */
	XPRINTF("%s", "LAYER: floaters:\n");
	for (i = 0; i < n; i++) {
		if (!(c = cl[i]))
			continue;
		if (!c->is.bastard && (c->is.floater || c->skip.arrange) && !c->is.below && !WTCHECK(c, WindowTypeDesk)) {
			cl[i] = NULL; wl[j] = c->frame; sl[j] = c; j++;
			XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		}
	}
	XPRINTF("%s", "LAYER: bastards:\n");
	for (i = 0; i < n; i++) {
		if (!(c = cl[i]))
			continue;
		if (c->is.bastard && !c->is.below && !WTCHECK(c, WindowTypeDesk)) {
			cl[i] = NULL; wl[j] = c->frame; sl[j] = c; j++;
			XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		}
	}
	XPRINTF("%s", "LAYER: tiled:\n");
	for (i = 0; i < n; i++) {
		if (!(c = cl[i]))
			continue;
		if (!c->is.bastard && !(c->is.floater || c->skip.arrange) && !c->is.below && !WTCHECK(c, WindowTypeDesk)) {
			cl[i] = NULL; wl[j] = c->frame; sl[j] = c; j++;
			XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		}
	}
	/* windows having state _NET_WM_STATE_BELOW */
	XPRINTF("%s", "LAYER: windows having _NET_WM_STATE_BELOW:\n");
	for (i = 0; i < n; i++) {
		if (!(c = cl[i]))
			continue;
		if (c->is.below && !WTCHECK(c, WindowTypeDesk)) {
			cl[i] = NULL; wl[j] = c->frame; sl[j] = c; j++;
			XPRINTF("client 0x%08lx 0x%08lx %s\n", c->frame, c->win, c->name);
		}
	}
	/* windows of type _NET_WM_TYPE_DESKTOP */
	XPRINTF("%s", "LAYER: windows having _NET_WINDOW_TYPE_DESKTOP:\n");
	for (i = 0; i < n; i++) {
		if (!(c = cl[i]))
			continue;
		if (WTCHECK(c, WindowTypeDesk)) {
			cl[i] = NULL; wl[j] = c->frame; sl[j] = c; j++;
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

AScreen *
getscreen(Window win)
{
	Window *wins, wroot, parent;
	unsigned int num;
	AScreen *s = NULL;

	if (!win)
		return (s);
	if (!XFindContext(dpy, win, context[ScreenContext], (XPointer *) &s))
		return (s);
	if (!XQueryTree(dpy, win, &wroot, &parent, &wins, &num))
		return (s);
	if (!XFindContext(dpy, wroot, context[ScreenContext], (XPointer *) &s))
		return (s);
	if (wins)
		XFree(wins);
	return (s);
}

AScreen *
geteventscr(XEvent *ev)
{
	return (event_scr = getscreen(ev->xany.window) ? : scr);
}

Bool
handle_event(XEvent *ev)
{
	int i;

	if (ev->type <= LASTEvent) {
		if (handler[ev->type])
			return (handler[ev->type]) (ev);
	} else
		for (i = BaseLast - 1; i >= 0; i--) {
			if (!haveext[i])
				continue;
			if (ev->type >= ebase[i] && ev->type < ebase[i] + EXTRANGE) {
				int slot = ev->type - ebase[i] + LASTEvent + EXTRANGE * i;

				if (handler[slot])
					return (handler[slot]) (ev);
			}
		}
	DPRINTF("WARNING: No handler for event type %d\n", ev->type);
	return False;
}

#ifdef STARTUP_NOTIFICATION
void
sn_handler(SnMonitorEvent *event, void *dummy) {
	Notify *n, **np;
	SnStartupSequence *seq = NULL;

	seq = sn_monitor_event_get_startup_sequence(event);

	switch (sn_monitor_event_get_type(event)) {
	case SN_MONITOR_EVENT_INITIATED:
		n = emallocz(sizeof(*n));
		n->seq = sn_monitor_event_get_startup_sequence(event);
		n->assigned = False;
		n->next = notifies;
		notifies = n;
		break;
	case SN_MONITOR_EVENT_CHANGED:
		break;
	case SN_MONITOR_EVENT_COMPLETED:
	case SN_MONITOR_EVENT_CANCELED:
		seq = sn_monitor_event_get_startup_sequence(event);
		for (n = notifies, np = &notifies; n; np = &n->next, n = *np) {
			if (n->seq == seq) {
				sn_startup_sequence_unref(n->seq);
				*np = n->next;
				free(n);
			}
		}
		break;
	}
	if (seq)
		sn_startup_sequence_unref(seq);
	sn_monitor_event_unref(event);
}
#endif

void
run(void)
{
	int xfd;
	XEvent ev;

	/* main event loop */
	XSync(dpy, False);
	xfd = ConnectionNumber(dpy);
	while (running) {
		struct pollfd pfd = { xfd, POLLIN | POLLERR | POLLHUP, 0 };

		switch (poll(&pfd, 1, -1)) {
		case -1:
			if (errno == EAGAIN || errno == EINTR || errno == ERESTART)
				continue;
			cleanup(CauseRestarting);
			eprint("poll failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		case 1:
			if (pfd.revents & (POLLNVAL | POLLHUP | POLLERR)) {
				cleanup(CauseRestarting);
				eprint("poll error\n");
				exit(EXIT_FAILURE);
			}
			if (pfd.revents & POLLIN) {
				while (XPending(dpy)) {
					XNextEvent(dpy, &ev);
					scr = geteventscr(&ev);
					if (!handle_event(&ev))
						XPRINTF("WARNING: Event %d not handled\n", ev.type);
				}
			}
		}
	}
}

void
save(Client *c) {
	CPRINTF(c, "%dx%d+%d+%d:%d <= %dx%d+%d+%d:%d\n",
			c->r.w, c->r.h, c->r.x, c->r.y, c->r.b,
			c->c.w, c->c.h, c->c.x, c->c.y, c->c.b);
	c->r = c->c;
}

void
restore(Client *c)
{
	ClientGeometry g;

	getgeometry(c, &c->r, &g);
	DPRINTF("CALLING: constrain()\n");
	constrain(c, &g);
	DPRINTF("CALLING resize()\n");
	resize(c, &g);
}

Monitor *
findmonbynum(int num) {
	Monitor *m;

	for (m = scr->monitors; m && m->num != num; m = m->next) ;
	return m;
}

void
calc_max(Client *c, Monitor *m, ClientGeometry *g) {
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

void
calc_fill(Client *c, Monitor *m, Workarea *wa, ClientGeometry *g) {
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
		if (o->is.bastard)
			continue;
		if (!isfloating(o, m))
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

void
calc_maxv(Client *c, Workarea *wa, ClientGeometry *g) {
	g->y = wa->y;
	g->h = wa->h;
	g->b = scr->style.border;
}

void
calc_maxh(Client *c, Workarea *wa, ClientGeometry *g) {
	g->x = wa->x;
	g->w = wa->w;
	g->b = scr->style.border;
}

void
get_decor(Client *c, Monitor *m, ClientGeometry *g)
{
	int i;
	Bool decorate;

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
	else if (c->is.shaded && (c != sel || !options.autoroll))
		decorate = True;
	else if (!m && !(m = clientmonitor(c))) {
		decorate = False;
		for (i = 0; i < scr->ntags; i++) {
			if (c->tags[i] && (scr->views[i].dectiled ||
						FEATURES(scr->views[i].layout, OVERLAP))) {
				decorate = True;
				break;
			}
		}
	} else {
		decorate = (scr->views[m->curtag].dectiled || MFEATURES(m, OVERLAP)) ?
			True : False;
	}
	g->t = decorate ? scr->style.titleheight : 0;
	g->g = decorate ? scr->style.gripsheight : 0;
}

void
updatefloat(Client *c, Monitor *m) {
	ClientGeometry g;
	Workarea wa;

	if (!m && !(m = c->curmon))
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
	if (c->is.max)
		calc_max(c, m, &g);
	else if (c->is.fill)
		calc_fill(c, m, &wa, &g);
	CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
	if (c->is.maxv)
		calc_maxv(c, &wa, &g);
	CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
	if (c->is.maxh)
		calc_maxh(c, &wa, &g);
	CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
	if (!c->is.max) {
		/* TODO: more than just northwest gravity */
		DPRINTF("CALLING: constrain()\n");
		constrain(c, &g);
	}
	CPRINTF(c, "g: %dx%d+%d+%d:%d\n", g.w, g.h, g.x, g.y, g.b);
	DPRINTF("CALLING resize()\n");
	resize(c, &g);
	if (c->is.max)
		ewmh_update_net_window_fs_monitors(c);
	discardenter();
}

void
delsystray(Window win) {
	unsigned int i, j;

	for (i = 0, j = 0; i < systray.count; i++) {
		if (systray.members[i] == win)
			continue;
		systray.members[i] = systray.members[j++];
	}
	if (j < systray.count) {
		systray.count = j;
		XChangeProperty(dpy, scr->root, _XA_KDE_NET_SYSTEM_TRAY_WINDOWS, XA_WINDOW, 32,
				PropModeReplace, (unsigned char *) systray.members,
				systray.count);
	}
}

Bool
isdockapp(Window win) {
	XWMHints *wmh;
	Bool ret;

	if ((ret = ((wmh = XGetWMHints(dpy, win)) &&
		(wmh->flags & StateHint) && (wmh->initial_state == WithdrawnState))))
		setwmstate(win, WithdrawnState, None);
	return ret;
}

Bool
issystray(Window win) {
	int format, status;
	long *data = NULL;
	unsigned long extra, nitems = 0;
	Atom real;
	Bool ret;
	unsigned int i;

	status =
	    XGetWindowProperty(dpy, win, _XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR, 0L, 1L, False,
			       AnyPropertyType, &real, &format, &nitems, &extra,
			       (unsigned char **) &data);
	if ((ret = (status == Success && real != None))) {
		for (i = 0; i < systray.count && systray.members[i] != win; i++) ;
		if (i == systray.count) {
			XSelectInput(dpy, win, StructureNotifyMask);
			XSaveContext(dpy, win, context[SysTrayWindows], (XPointer)&systray);
			XSaveContext(dpy, win, context[ScreenContext], (XPointer) scr);

			systray.members =
			    erealloc(systray.members, (i + 1) * sizeof(Window));
			systray.members[i] = win;
			systray.count++;
			XChangeProperty(dpy, scr->root, _XA_KDE_NET_SYSTEM_TRAY_WINDOWS, XA_WINDOW,
					32, PropModeReplace,
					(unsigned char *) systray.members,
					systray.count);
		}
	} else
		delsystray(win);
	if (data)
		XFree(data);
	return ret;
}

void
scan(void) {
	unsigned int i, num;
	Window *wins, d1, d2;
	XWindowAttributes wa;
	XWMHints *wmh;

	wins = NULL;
	if (XQueryTree(dpy, scr->root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			DPRINTF("scan checking window 0x%lx\n", wins[i]);
			if (!XGetWindowAttributes(dpy, wins[i], &wa) ||
			    wa.override_redirect || issystray(wins[i] ||
			    isdockapp(wins[i]))
			    || XGetTransientForHint(dpy, wins[i], &d1) ||
			    ((wmh = XGetWMHints(dpy, wins[i])) &&
			     (wmh->flags & WindowGroupHint) &&
			     (wmh->window_group != wins[i])))
				continue;
			DPRINTF("scan checking non-transient window 0x%lx\n", wins[i]);
			if (wa.map_state == IsViewable
			    || getstate(wins[i]) == IconicState
			    || getstate(wins[i]) == NormalState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) {
			DPRINTF("scan checking window 0x%lx\n", wins[i]);
			/* now the transients and group members */
			if (!XGetWindowAttributes(dpy, wins[i], &wa) ||
			    wa.override_redirect || issystray(wins[i]) ||
			    isdockapp(wins[i]))
				continue;
			DPRINTF("scan checking transient window 0x%lx\n", wins[i]);
			if ((XGetTransientForHint(dpy, wins[i], &d1) ||
			     ((wmh = XGetWMHints(dpy, wins[i])) &&
			      (wmh->flags & WindowGroupHint) &&
			      (wmh->window_group != wins[i])))
			    && (wa.map_state == IsViewable
				|| getstate(wins[i]) == IconicState
				|| getstate(wins[i]) == NormalState))
				manage(wins[i], &wa);
		}
	}
	if (wins)
		XFree(wins);
	DPRINTF("done scanning screen %d\n", scr->screen);
	ewmh_update_kde_splash_progress();
}

void
setlayout(const char *arg)
{
	unsigned int i;
	Monitor *cm;

	if (!(cm = selmonitor()))
		return;
	if (arg) {
		View *v;

		for (i = 0; i < LENGTH(layouts); i++)
			if (*arg == layouts[i].symbol)
				break;
		if (i == LENGTH(layouts))
			return;
		v = scr->views + cm->curtag;
		v->layout = &layouts[i];
		v->major = layouts[i].major;
		v->minor = layouts[i].minor;
		v->placement = layouts[i].placement;
	}
	arrange(cm);
	ewmh_update_net_desktop_modes();
}

void
setmwfact(const char *arg)
{
	double delta;
	Monitor *cm;
	View *v;

	if (!(cm = selmonitor()))
		return;

	v = scr->views + cm->curtag;
	if (!FEATURES(v->layout, MWFACT))
		return;
	/* arg handling, manipulate mwfact */
	if (arg == NULL)
		v->mwfact = DEFMWFACT;
	else if (sscanf(arg, "%lf", &delta) == 1) {
		if (arg[0] == '+' || arg[0] == '-') {
			switch (v->major) {
			case OrientBottom:
			case OrientLeft:
				v->mwfact += delta;
				break;
			case OrientTop:
			case OrientRight:
			case OrientLast:
				v->mwfact -= delta;
				break;
			}
		} else
			v->mwfact = delta;
		if (v->mwfact < 0.1)
			v->mwfact = 0.1;
		else if (v->mwfact > 0.9)
			v->mwfact = 0.9;
	}
	arrange(cm);
}

void
initview(unsigned int i, double mwfact, double mhfact, int nmaster, int ncolumns, const char *deflayout) {
	unsigned int j;
	char conf[32], ltname;

	scr->views[i].layout = &layouts[0];
	snprintf(conf, sizeof(conf), "tags.layout%d", i);
	strncpy(&ltname, getresource(conf, deflayout), 1);
	for (j = 0; j < LENGTH(layouts); j++) {
		if (layouts[j].symbol == ltname) {
			scr->views[i].layout = &layouts[j];
			break;
		}
	}
	scr->views[i].barpos = StrutsOn;
	scr->views[i].dectiled = options.dectiled;
	scr->views[i].nmaster = nmaster;
	scr->views[i].ncolumns = ncolumns;
	scr->views[i].mwfact = mwfact;
	scr->views[i].mhfact = mhfact;
	scr->views[i].major = scr->views[i].layout->major;
	scr->views[i].minor = scr->views[i].layout->minor;
	scr->views[i].placement = scr->views[i].layout->placement;
}

void
newview(unsigned int i) {
	double mwfact, mhfact;
	int nmaster, ncolumns;
	const char *deflayout;

	mwfact = atof(getresource("mwfact", STR(DEFMWFACT)));
	mhfact = atof(getresource("mhfact", STR(DEFMHFACT)));
	nmaster = atoi(getresource("nmaster", STR(DEFNMASTER)));
	ncolumns = atoi(getresource("ncolumns", STR(DEFNCOLUMNS)));
	deflayout = getresource("deflayout", "i");
	if (nmaster < 1)
		nmaster = 1;
	if (ncolumns < 1)
		ncolumns = 1;
	initview(i, mwfact, mhfact, nmaster, ncolumns, deflayout);
}

void
initlayouts() {
	unsigned int i;
	double mwfact, mhfact;
	int nmaster, ncolumns;
	const char *deflayout;

	/* init layouts */
	mwfact = atof(getresource("mwfact", STR(DEFMWFACT)));
	mhfact = atof(getresource("mhfact", STR(DEFMHFACT)));
	nmaster = atoi(getresource("nmaster", STR(DEFNMASTER)));
	ncolumns = atoi(getresource("ncolumns", STR(DEFNCOLUMNS)));
	deflayout = getresource("deflayout", "i");
	if (nmaster < 1)
		nmaster = 1;
	if (ncolumns < 1)
		ncolumns = 1;
	for (i = 0; i < scr->ntags; i++)
		initview(i, mwfact, mhfact, nmaster, ncolumns, deflayout);
	ewmh_update_net_desktop_modes();
}

static Bool
isomni(Client *c) {
	unsigned int i;

	if (!c->is.sticky)
		for (i = 0; i < scr->ntags; i++)
			if (!c->tags[i])
				return False;
	return True;
}

void
freemonitors() {
	int i;

	for (i = 0; i < scr->nmons; i++) {
		free(scr->monitors[i].seltags);
		free(scr->monitors[i].prevtags);
		XDestroyWindow(dpy, scr->monitors[i].veil);
	}
	free(scr->monitors);
}

void
updatemonitors(XEvent *e, int n, Bool size_update, Bool full_update)
{
	int i, j;
	Client *c;
	Monitor *m;

	for (i = 0; i < n; i++)
		scr->monitors[i].next = &scr->monitors[i + 1];
	scr->monitors[n - 1].next = NULL;
	if (scr->nmons != n)
		full_update = True;
	if (full_update)
		size_update = True;
	scr->nmons = n;
	if (e) {
		if (full_update) {
			for (c = scr->clients; c; c = c->next) {
				if (isomni(c))
					continue;
				if (!(m = findmonitor(c)))
					if (!(m = curmonitor()))
						m = scr->monitors;
				memcpy(c->tags, m->seltags, scr->ntags * sizeof(*c->tags));
			}
			for (i = 0; i < scr->nmons; i++) {
				m = &scr->monitors[i];
				m->curtag = i % scr->ntags;
				for (j = 0; j < scr->ntags; j++)
					m->seltags[j] = False;
				m->seltags[i % scr->ntags] = True;
			}
		}
		if (size_update) {
			updatestruts();
		}
	}
	ewmh_update_net_desktop_geometry();
}

Bool
initmonitors(XEvent *e)
{
	int n;
	Monitor *m;
	Bool size_update = False, full_update = False;

#ifdef XRANDR
	if (e)
		XRRUpdateConfiguration(e);
#else
	DPRINTF("%s", "compiled without RANDR support\n");
#endif

#ifdef XINERAMA
	if (haveext[XineramaBase]) {
		int i;
		XineramaScreenInfo *si;

		if (!XineramaIsActive(dpy)) {
			DPRINTF("XINERAMA is not active for screen %d\n", scr->screen);
			goto no_xinerama;
		}
		DPRINTF("XINERAMA is active for screen %d\n", scr->screen);
		si = XineramaQueryScreens(dpy, &n);
		if (!si) {
			DPRINTF("XINERAMA defines no monitors for screen %d\n", scr->screen);
			goto no_xinerama;
		}
		DPRINTF("XINERAMA defineds %d monitors for screen %d\n", n, scr->screen);
		if (n < 2)
			goto no_xinerama;
		for (i = 0; i < n; i++) {
			if (i < scr->nmons) {
				m = &scr->monitors[i];
				if (m->sc.x != si[i].x_org) {
					m->sc.x = m->wa.x = si[i].x_org;
					m->mx = m->sc.x + m->sc.w/2;
					full_update = True;
				}
				if (m->sc.y != si[i].y_org) {
					m->sc.y = m->wa.y = si[i].y_org;
					m->my = m->sc.y + m->sc.h/2;
					full_update = True;
				}
				if (m->sc.w != si[i].width) {
					m->sc.w = m->wa.w = si[i].width;
					m->mx = m->sc.x + m->sc.w/2;
					size_update = True;
				}
				if (m->sc.h != si[i].height) {
					m->sc.h = m->wa.h = si[i].height;
					m->my = m->sc.y + m->sc.h/2;
					size_update = True;
				}
				if (m->num != si[i].screen_number) {
					m->num = si[i].screen_number;
					full_update = True;
				}
			} else {
				XSetWindowAttributes wa;

				scr->monitors = erealloc(scr->monitors, (i+1) * sizeof(*scr->monitors));
				m = &scr->monitors[i];
				full_update = True;
				m->index = i;
				m->sc.x = m->wa.x = si[i].x_org;
				m->sc.y = m->wa.y = si[i].y_org;
				m->sc.w = m->wa.w = si[i].width;
				m->sc.h = m->wa.h = si[i].height;
				m->mx = m->sc.x + m->sc.w/2;
				m->my = m->sc.y + m->sc.h/2;
				m->num = si[i].screen_number;
				m->prevtags = ecalloc(scr->ntags, sizeof(*m->prevtags));
				m->seltags = ecalloc(scr->ntags, sizeof(*m->seltags));
				m->prevtags[m->num % scr->ntags] = True;
				m->seltags[m->num % scr->ntags] = True;
				m->curtag = m->num % scr->ntags;
				m->veil = XCreateSimpleWindow(dpy, scr->root, m->wa.x, m->wa.y,
						m->wa.w, m->wa.h, 0, 0, 0);
				wa.background_pixmap = None;
				wa.override_redirect = True;
				XChangeWindowAttributes(dpy, m->veil, CWBackPixmap|CWOverrideRedirect, &wa);
			}
		}
		XFree(si);
		updatemonitors(e, n, size_update, full_update);
		return True;

	} else
		DPRINTF("no XINERAMA extension for screen %d\n", scr->screen);
      no_xinerama:
#else
	DPRINTF("%s", "compiled without XINERAMA support\n");
#endif
#ifdef XRANDR
	if (haveext[XrandrBase]) {
		XRRScreenResources *sr;
		int i, j;

		if (!(sr = XRRGetScreenResources(dpy, scr->root)) || sr->ncrtc < 2)
			goto no_xrandr;
		for (i = 0, n = 0; i < sr->ncrtc; i++) {
			XRRCrtcInfo *ci;

			if (!(ci = XRRGetCrtcInfo(dpy, sr, sr->crtcs[i])))
				continue;
			if (!ci->width || !ci->height)
				continue;
			/* skip mirrors */
			for (j = 0; j < n; j++)
				if (scr->monitors[j].sc.x == scr->monitors[n].sc.x &&
				    scr->monitors[j].sc.y == scr->monitors[n].sc.y)
					break;
			if (j < n)
				continue;

			if (n < scr->nmons) {
				m = &scr->monitors[n];
				if (m->sc.x != ci->x) {
					m->sc.x = m->wa.x = ci->x;
					m->mx = m->sc.x + m->sc.w/2;
					full_update = True;
				}
				if (m->sc.y != ci->y) {
					m->sc.y = m->wa.y = ci->y;
					m->my = m->sc.y + m->sc.h/2;
					full_update = True;
				}
				if (m->sc.w != ci->width) {
					m->sc.w = m->wa.w = ci->width;
					m->mx = m->sc.x + m->sc.w/2;
					size_update = True;
				}
				if (m->sc.h != ci->height) {
					m->sc.h = m->wa.h = ci->height;
					m->my = m->sc.y + m->sc.h/2;
					size_update = True;
				}
				if (m->num != i) {
					m->num = i;
					full_update = True;
				}
			} else {
				XSetWindowAttributes wa;

				scr->monitors = erealloc(scr->monitors, (n+1) * sizeof(*scr->monitors));
				m = &scr->monitors[n];
				full_update = True;
				m->index = n;
				m->sc.x = m->wa.x = ci->x;
				m->sc.y = m->wa.y = ci->y;
				m->sc.w = m->wa.w = ci->width;
				m->sc.h = m->wa.h = ci->height;
				m->mx = m->sc.x + m->sc.w/2;
				m->my = m->sc.y + m->sc.h/2;
				m->num = i;
				m->prevtags = ecalloc(scr->ntags, sizeof(*m->prevtags));
				m->seltags = ecalloc(scr->ntags, sizeof(*m->seltags));
				m->prevtags[m->num % scr->ntags] = True;
				m->seltags[m->num % scr->ntags] = True;
				m->curtag = m->num % scr->ntags;
				m->veil = XCreateSimpleWindow(dpy, scr->root, m->wa.x, m->wa.y,
						m->wa.w, m->wa.h, 0, 0, 0);
				wa.background_pixmap = None;
				wa.override_redirect = True;
				XChangeWindowAttributes(dpy, m->veil, CWBackPixmap|CWOverrideRedirect, &wa);
			}
			n++;
		}
		updatemonitors(e, n, size_update, full_update);
		return True;

	}
      no_xrandr:
#endif
	n = 1;
	if (n <= scr->nmons) {
		m = &scr->monitors[0];
		if (m->sc.x != 0) {
			m->sc.x = m->wa.x = 0;
			m->mx = m->sc.x + m->sc.w/2;
			full_update = True;
		}
		if (m->sc.y != 0) {
			m->sc.y = m->wa.y = 0;
			m->my = m->sc.y + m->sc.h/2;
			full_update = True;
		}
		if (m->sc.w != DisplayWidth(dpy, scr->screen)) {
			m->sc.w = m->wa.w = DisplayWidth(dpy, scr->screen);
			m->mx = m->sc.x + m->sc.w/2;
			size_update = True;
		}
		if (m->sc.h != DisplayHeight(dpy, scr->screen)) {
			m->sc.h = m->wa.h = DisplayHeight(dpy, scr->screen);
			m->my = m->sc.y + m->sc.h/2;
			size_update = True;
		}
		if (m->num != 0) {
			m->num = 0;
			full_update = True;
		}
	} else {
		XSetWindowAttributes wa;

		scr->monitors = erealloc(scr->monitors, n * sizeof(*scr->monitors));
		m = &scr->monitors[0];
		full_update = True;
		m->index = 0;
		m->sc.x = m->wa.x = 0;
		m->sc.y = m->wa.y = 0;
		m->sc.w = m->wa.w = DisplayWidth(dpy, scr->screen);
		m->sc.h = m->wa.h = DisplayHeight(dpy, scr->screen);
		m->mx = m->sc.x + m->sc.w/2;
		m->my = m->sc.y + m->sc.h/2;
		m->num = 0;
		m->prevtags = ecalloc(scr->ntags, sizeof(*m->prevtags));
		m->seltags = ecalloc(scr->ntags, sizeof(*m->seltags));
		m->prevtags[m->num % scr->ntags] = True;
		m->seltags[m->num % scr->ntags] = True;
		m->curtag = m->num % scr->ntags;
		m->veil = XCreateSimpleWindow(dpy, scr->root, m->wa.x, m->wa.y,
				m->wa.w, m->wa.h, 0, 0, 0);
		wa.background_pixmap = None;
		wa.override_redirect = True;
		XChangeWindowAttributes(dpy, m->veil, CWBackPixmap|CWOverrideRedirect, &wa);
	}
	updatemonitors(e, n, size_update, full_update);
	return True;
}

void
inittag(unsigned int i) {
	char tmp[25] = "", def[8] = "";

	scr->tags[i] = emallocz(sizeof(tmp));
	snprintf(tmp, sizeof(tmp), "tags.name%d", i);
	snprintf(def, sizeof(def), "%u", i);
	snprintf(scr->tags[i], sizeof(tmp), "%s", getresource(tmp, def));
	scr->dt_tags[i] = XInternAtom(dpy, scr->tags[i], False);
	DPRINTF("Assigned name '%s' to tag %u\n", scr->tags[i], i);
}

void
newtag(unsigned int i) {
	inittag(i);
}

void
inittags() {
	ewmh_process_net_number_of_desktops();
	scr->views = ecalloc(scr->ntags, sizeof(*scr->views));
	scr->tags = ecalloc(scr->ntags, sizeof(*scr->tags));
	scr->dt_tags = ecalloc(scr->ntags, sizeof(*scr->dt_tags));
	ewmh_process_net_desktop_names();
}

void
deltag() {
	Client *c;
	unsigned int i, last;
	Monitor *cm;

	if (!(cm = selmonitor()))
		return;
	if (scr->ntags < 2)
		return;
	last = scr->ntags - 1;

	/* move off the desktop being deleted */
	if (cm->curtag == last)
		view(last - 1);

	/* move windows off the desktop being deleted */
	for (c = scr->clients; c; c = c->next) {
		if (isomni(c)) {
			c->tags[last] = False;
			continue;
		}
		for (i = 0; i < last; i++)
			if (c->tags[i])
				break;
		if (i == last)
			tag(c, last - 1);
		else if (c->tags[last])
			c->tags[last] = False;
	}

	free(scr->tags[last]);
	scr->tags[last] = NULL;

	--scr->ntags;
#if 0
	/* caller's responsibility */
	ewmh_update_net_number_of_desktops();
#endif

}

void
rmlasttag() {
	deltag();
	ewmh_process_net_desktop_names();
	ewmh_update_net_number_of_desktops();
}

void
addtag() {
	Client *c;
	Monitor *m;
	unsigned int i, n;

	if (scr->ntags >= 64)
		return; /* stop the insanity, go organic */

	n = scr->ntags + 1;
	scr->views = erealloc(scr->views, n * sizeof(scr->views[scr->ntags]));
	newview(scr->ntags);
	scr->tags = erealloc(scr->tags, n * sizeof(scr->tags[scr->ntags]));
	newtag(scr->ntags);

	for (c = scr->clients; c; c = c->next) {
		c->tags = erealloc(c->tags, n * sizeof(c->tags[scr->ntags]));
		for (i = 0; i < scr->ntags && c->tags[i]; i++);
		c->tags[scr->ntags] = (i == scr->ntags || c->is.sticky) ? True : False;
	}

	for (m = scr->monitors; m; m = m->next) {
		m->prevtags = erealloc(m->prevtags, n * sizeof(m->prevtags[0]));
		m->prevtags[scr->ntags] = False;
		m->seltags  = erealloc(m->seltags,  n * sizeof(m->seltags [0]));
		m->seltags [scr->ntags] = False;
	}

	scr->ntags++;
#if 0
	/* caller's responsibility */
	ewmh_update_net_number_of_desktops();
#endif
}

void
appendtag() {
	addtag();
	ewmh_process_net_desktop_names();
	ewmh_update_net_number_of_desktops();
}

void
settags(unsigned int numtags) {
	if (1 > numtags || numtags > 64)
		return;
	while (scr->ntags < numtags) { addtag(); }
	while (scr->ntags > numtags) { deltag(); }
	ewmh_process_net_desktop_names();
	ewmh_update_net_number_of_desktops();
}

void
sighandler(int signum) {
	if (signum == SIGHUP)
		quit("HUP!");
	else
		quit(NULL);
}

void
setup(char *conf) {
	int d;
	int i, j;
	unsigned int mask;
	Window w, proot;
	Monitor *m;
	XModifierKeymap *modmap;
	XSetWindowAttributes wa;
	char oldcwd[256], path[256] = "/";
	char *home, *slash;
	/* configuration files to open (%s gets converted to $HOME) */
	const char *confs[] = {
		conf,
		"%s/.adwm/adwmrc",
		SYSCONFPATH "/adwmrc",
		NULL
	};

	/* init cursors */
	cursor[CurResizeTopLeft]	= XCreateFontCursor(dpy, XC_top_left_corner);
	cursor[CurResizeTop]		= XCreateFontCursor(dpy, XC_top_side);
	cursor[CurResizeTopRight]	= XCreateFontCursor(dpy, XC_top_right_corner);
	cursor[CurResizeRight]		= XCreateFontCursor(dpy, XC_right_side);
	cursor[CurResizeBottomRight]	= XCreateFontCursor(dpy, XC_bottom_right_corner);
	cursor[CurResizeBottom]		= XCreateFontCursor(dpy, XC_bottom_side);
	cursor[CurResizeBottomLeft]	= XCreateFontCursor(dpy, XC_bottom_left_corner);
	cursor[CurResizeLeft]		= XCreateFontCursor(dpy, XC_left_side);
	cursor[CurMove]			= XCreateFontCursor(dpy, XC_fleur);
	cursor[CurNormal]		= XCreateFontCursor(dpy, XC_left_ptr);

	/* init modifier map */
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++) {
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
			    == XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
		}
	XFreeModifiermap(modmap);

	/* select for events */
	wa.event_mask = ROOTMASK;
	wa.cursor = cursor[CurNormal];
	for (scr = screens; scr < screens + nscr; scr++) {
		XChangeWindowAttributes(dpy, scr->root, CWEventMask | CWCursor, &wa);
		XSelectInput(dpy, scr->root, wa.event_mask);
	}

	/* init resource database */
	XrmInitialize();

	home = getenv("HOME");
	if (!home)
		*home = '/';
	if (!getcwd(oldcwd, sizeof(oldcwd)))
		eprint("adwm: getcwd error: %s\n", strerror(errno));

	for (i = 0; confs[i] != NULL; i++) {
		if (*confs[i] == '\0')
			continue;
		snprintf(conf, 255, confs[i], home);
		/* retrieve path to chdir(2) to it */
		slash = strrchr(conf, '/');
		if (slash)
			snprintf(path, slash - conf + 1, "%s", conf);
		if (chdir(path) != 0)
			fprintf(stderr, "adwm: cannot change directory\n");
		xrdb = XrmGetFileDatabase(conf);
		/* configuration file loaded successfully; break out */
		if (xrdb)
			break;
	}
	if (!xrdb)
		fprintf(stderr, "adwm: no configuration file found, using defaults\n");

	initrules();

	/* init appearance */
	options.attachaside = atoi(getresource("attachaside", "1")) ? True : False;
	strncpy(options.command, getresource("command", COMMAND), LENGTH(options.command));
	options.command[LENGTH(options.command) - 1] = '\0';
	options.dectiled = atoi(getresource("decoratetiled", STR(DECORATETILED)));
	options.hidebastards = atoi(getresource("hidebastards", "0")) ? True : False;
	options.autoroll = atoi(getresource("autoroll", "0")) ? True : False;
	options.focus = atoi(getresource("sloppy", "0"));
	options.snap = atoi(getresource("snap", STR(SNAP)));

	for (scr = screens; scr < screens + nscr; scr++) {
		if (!scr->managed)
			continue;

#ifdef STARTUP_NOTIFICATION
		sn_dpy = sn_display_new(dpy, NULL, NULL);
		sn_ctx = sn_monitor_context_new(sn_dpy, scr->screen, &sn_handler, NULL, NULL);
		DPRINTF("startup notification on screen %d\n", scr->screen);
#else
		DPRINTF("startup notification not supported screen %d\n", scr->screen);
#endif
		/* init EWMH atom */
		initewmh(scr->selwin);

		/* init tags */
		inittags();
		/* init geometry */
		initmonitors(NULL);

		/* init modkey */
		initkeys();
		initlayouts();

		ewmh_update_net_number_of_desktops();
		ewmh_update_net_current_desktop();
		ewmh_update_net_virtual_roots();

		grabkeys();

		/* init appearance */
		initstyle();

		for (m = scr->monitors; m; m = m->next) {
			m->struts[RightStrut] = m->struts[LeftStrut] =
			    m->struts[TopStrut] = m->struts[BotStrut] = 0;
			updategeom(m);
		}
		ewmh_update_net_work_area();
		ewmh_process_net_showing_desktop();
		ewmh_update_net_showing_desktop();
	}

	if (chdir(oldcwd) != 0)
		fprintf(stderr, "adwm: cannot change directory\n");

	/* multihead support */
	XQueryPointer(dpy, scr->root, &proot, &w, &d, &d, &d, &d, &mask);
	XFindContext(dpy, proot, context[ScreenContext], (XPointer *) &scr);
}

void
spawn(const char *arg)
{
	static char shell[] = "/bin/sh";

	if (!arg)
		return;
	/* The double-fork construct avoids zombie processes and keeps the code clean
	   from stupid signal handlers. */
	if (fork() == 0) {
		if (fork() == 0) {
			char *d, *p;

			if (dpy)
				close(ConnectionNumber(dpy));
			setsid();
			d = strdup(getenv("DISPLAY"));
			if (!(p = strrchr(d, '.')) || !strlen(p + 1)
			    || strspn(p + 1, "0123456789") != strlen(p + 1)) {
				size_t len = strlen(d) + 12;
				char *s = calloc(len, sizeof(*s));

				snprintf(s, len, "%s.%d", d, scr->screen);
				setenv("DISPLAY", s, 1);
			}
			execl(shell, shell, "-c", arg, (char *) NULL);
			fprintf(stderr, "adwm: execl '%s -c %s'", shell, arg);
			perror(" failed");
		}
		exit(0);
	}
	wait(0);
}

void
m_spawn(Client *c, XEvent *e) {
	spawn(options.command);
}

static void
_tag(Client *c, int index) {
	unsigned int i;

	for (i = 0; i < scr->ntags; i++)
		c->tags[i] = (index == -1);
	i = (index == -1) ? 0 : index;
	c->tags[i] = True;
	ewmh_update_net_window_desktop(c);
	arrange(NULL);
	focus(NULL);
}

void
tag(Client *c, int index) {
	if (!c || (!c->can.tag && c->is.managed))
		return;
	return with_transients(c, &_tag, index);
}

void
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
	v = &scr->views[cm->curtag];
	for (wa.n = wa.s = ma.s = sa.s = 0, c = nexttiled(scr->clients, cm);
	     c; c = nexttiled(c->next, cm), wa.n++) {
		if (c->is.shaded && (c != sel || !options.autoroll)) {
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
#if 0
	if (!v->dectiled && (mdec || sdec))
		v->dectiled = True;
#endif

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
		if ((c->was.shaded = c->is.shaded) && (c != sel || !options.autoroll))
			if (!ma.s)
				c->is.shaded = False;
		g.x += ma.g;
		g.y += ma.g;
		g.w -= 2 * (ma.g + g.b);
		g.h -= 2 * (ma.g + g.b);
		DPRINTF("CALLING resize()\n");
		resize(c, &g);
		if (c->is.shaded && (c != sel || !options.autoroll))
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
		if (ma.s && c->is.shaded && (c != sel || !options.autoroll)) {
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
		if ((c->was.shaded = c->is.shaded) && (c != sel || !options.autoroll))
			if (!sa.s)
				c->is.shaded = False;
		g.x += sa.g;
		g.y += sa.g;
		g.w -= 2 * (sa.g + g.b);
		g.h -= 2 * (sa.g + g.b);
		DPRINTF("CALLING resize()\n");
		resize(c, &g);
		if (c->is.shaded && (c != sel || !options.autoroll))
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
		if (sa.s && c->is.shaded && (c != sel || !options.autoroll)) {
			n.h = s.h;
			sa.s--;
		}
		c->is.shaded = c->was.shaded;
	}
}

void
togglestruts() {
	Monitor *cm;
	View *v;
	
	if (!(cm = selmonitor()))
		return;
	v = scr->views + cm->curtag;
	v->barpos = (v->barpos == StrutsOn)
	    ? (options.hidebastards ? StrutsHide : StrutsOff) : StrutsOn;
	updategeom(cm);
	arrange(cm);
}

void
toggledectiled() {
	Monitor *cm;
	View *v;
	
	if (!(cm = selmonitor()))
		return;
	v = scr->views + cm->curtag;
	v->dectiled = v->dectiled ? False : True;
	arrange(cm);
}

void
togglefloating(Client *c) {
	Monitor *m;

	if (!c || c->is.floater || !(m = c->curmon))
		return;
	if (MFEATURES(m, OVERLAP)) /* XXX: why? */
		return;

	c->skip.arrange = !c->skip.arrange;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, m);
		arrange(m);
	}
}

void
togglefill(Client * c) {
	Monitor *m;

	if (!c || (!c->can.fill && c->is.managed) || !(m = c->curmon))
		return;
	c->is.fill = !c->is.fill;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, m);
	}

}

void
togglemax(Client * c) {
	Monitor *m;

	if (!c || (!c->can.max && c->is.managed) || !(m = c->curmon))
		return;
	c->is.max = !c->is.max;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, m);
		restack();
	}
}

void
togglemaxv(Client * c) {
	Monitor *m;

	if (!c || (!c->can.maxv && c->is.managed) || !(m = c->curmon))
		return;
	c->is.maxv = !c->is.maxv;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, m);
	}
}

void
toggleshade(Client * c) {
	Monitor *m;

	if (!c || (!c->can.shade && c->is.managed) || !(m = c->curmon))
		return;
	c->was.shaded = c->is.shaded;
	c->is.shaded = !c->is.shaded;
	if (c->is.managed) {
		ewmh_update_net_window_state(c);
		updatefloat(c, m);
	}
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
togglesticky(Client *c)
{
	Monitor *m;

	if (!c || (!c->can.stick && c->is.managed) || !(m = c->curmon))
		return;

	c->is.sticky = !c->is.sticky;
	if (c->is.managed) {
		if (c->is.sticky)
			tag(c, -1);
		else
			tag(c, m->curtag);
		ewmh_update_net_window_state(c);
	}
}

void
togglemin(Client *c) {
	if (!c || (!c->can.min && c->is.managed))
		return;
	if (c->is.icon) {
		c->is.icon = False;
		c->is.hidden = False;
		if (c->is.managed) {
			focus(c);
			arrange(clientmonitor(c));
		}
	} else {
		if (c->is.managed)
			iconify(c);
	}
}

void
toggleabove(Client *c) {
	if (!c || (!c->can.above && c->is.managed))
		return;
	c->is.above = !c->is.above;
	if (c->is.managed) {
		restack();
		ewmh_update_net_window_state(c);
	}
}

void
togglebelow(Client *c) {
	if (!c || (!c->can.below && c->is.managed))
		return;
	c->is.below = !c->is.below;
	if (c->is.managed) {
		restack();
		ewmh_update_net_window_state(c);
	}
}

void
togglepager(Client *c) {
	if (!c || c->is.bastard)
		return;
	c->skip.pager = !c->skip.pager;
	ewmh_update_net_window_state(c);
}

void
toggletaskbar(Client *c) {
	if (!c || c->is.bastard)
		return;
	c->skip.taskbar = !c->skip.taskbar;
	ewmh_update_net_window_state(c);
}

void
togglemodal(Client *c)
{
	Group *g;

	if (!c)
		return;
	switch ((Modality) c->is.modal) {
	case ModalModeless:
		if (c->is.grptrans) {
			if (c->leader) {
				c->is.modal = ModalGroup;
				if ((g = getleader(c->leader, ClientLeader)))
					incmodal(c, g);
			} else {
				c->is.modal = ModalSystem;
				incmodal(c, &window_stack);
			}
		} else if (c->is.transient) {
			c->is.modal = ModalPrimary;
			if ((g = getleader(c->transfor, ClientTransFor)))
				incmodal(c, g);
		}
		if (c->is.managed)
			focus(sel);
		break;
	case ModalGroup:
		if ((g = getleader(c->leader, ClientLeader)))
			decmodal(c, g);
		c->is.modal = ModalModeless;
		break;
	case ModalSystem:
		decmodal(c, &window_stack);
		c->is.modal = ModalModeless;
		break;
	case ModalPrimary:
		if ((g = getleader(c->transfor, ClientTransFor)))
			decmodal(c, g);
		c->is.modal = ModalModeless;
		break;
	}
	if (c->is.managed)
		ewmh_update_net_window_state(c);
}

void
togglemaxh(Client *c) {
	Monitor *m;

	if (!c || (!c->can.maxh && c->is.managed) || !(m = c->curmon))
		return;
	c->is.maxh = !c->is.maxh;
	ewmh_update_net_window_state(c);
	updatefloat(c, m);
}

void
rotateview(Client *c) {
	Monitor *m;
	View *v;

	if (!c || !(m = c->curmon))
		return;
	v = scr->views + m->curtag;
	if (!FEATURES(v->layout, ROTL))
		return;
	v->major = (v->major + 1) % OrientLast;
	v->minor = (v->minor + 1) % OrientLast;
	arrange(m);
}

void
unrotateview(Client *c) {
	Monitor *m;
	View *v;

	if (!c || !(m = c->curmon))
		return;
	v = scr->views + m->curtag;
	if (!FEATURES(v->layout, ROTL))
		return;
	v->major = (v->major + OrientLast - 1) % OrientLast;
	v->minor = (v->minor + OrientLast - 1) % OrientLast;
	arrange(m);
}

void
rotatezone(Client *c) {
	Monitor *m;
	View *v;

	if (!c || !(m = c->curmon))
		return;
	v = scr->views + m->curtag;
	if (!FEATURES(v->layout, ROTL) || !FEATURES(v->layout, NMASTER))
		return;
	v->minor = (v->minor + 1) % OrientLast;
	arrange(m);
}

void
unrotatezone(Client *c) {
	Monitor *m;
	View *v;

	if (!c || !(m = c->curmon))
		return;
	v = scr->views + m->curtag;
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

	if (!c || !(m = c->curmon))
		return;
	v = scr->views + m->curtag;
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

	if (!c || !(m = c->curmon))
		return;
	v = scr->views + m->curtag;
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
toggletag(Client *c, int index)
{
	unsigned int i, j;

	if (!c || (!c->can.tag && c->is.managed))
		return;
	i = (index == -1) ? 0 : index;
	c->tags[i] = !c->tags[i];
	for (j = 0; j < scr->ntags && !c->tags[j]; j++) ;
	if (j == scr->ntags)
		c->tags[i] = True;	/* at least one tag must be enabled */
	ewmh_update_net_window_desktop(c);
	drawclient(c);
	arrange(NULL);
}

void
togglemonitor()
{
	Monitor *m, *cm;
	int x, y;

	/* Should we use the pointer first? Perhaps we should use the monitor with the
	   keyboard focus first instead and only then use the monitor with the pointer in 
	   it or nearest it.  We are going to do a focus(NULL) on the new monitor anyway. 
	 */
	getpointer(&x, &y);
	if (!(cm = getmonitor(x, y)))
		return;
	cm->mx = x;
	cm->my = y;
	for (m = scr->monitors; m == cm && m && m->next; m = m->next) ;
	if (!m)
		return;
	XWarpPointer(dpy, None, scr->root, 0, 0, 0, 0, m->mx, m->my);
	focus(NULL);
}

void
toggleview(int index)
{
	unsigned int i, j;
	Monitor *m, *cm;

	/* Typically from a keyboard command.  Again, we should use the monitor with the
	   keyboard focus before the monitor with the pointer in it. */

	i = (index == -1) ? 0 : index;
	if (!(cm = selmonitor()))
		return;

	memcpy(cm->prevtags, cm->seltags, scr->ntags * sizeof(cm->seltags[0]));
	cm->seltags[i] = !cm->seltags[i];
	for (m = scr->monitors; m; m = m->next) {
		if (m->seltags[i] && m != cm) {
			memcpy(m->prevtags, m->seltags,
			       scr->ntags * sizeof(m->seltags[0]));
			m->seltags[i] = False;
			for (j = 0; j < scr->ntags && !m->seltags[j]; j++) ;
			if (j == scr->ntags) {
				m->seltags[i] = True;	/* at least one tag must be
							   viewed */
				cm->seltags[i] = False;	/* can't toggle */
				j = i;
			}
			if (m->curtag == i)
				m->curtag = j;
			arrange(m);
		}
	}
	arrange(cm);
	focus(NULL);
	ewmh_update_net_current_desktop();
}

void
focusview(int index)
{
	unsigned int i;
	Client *c;
	Monitor *cm;

	if (!(cm = selmonitor()))
		return;
	i = (index == -1) ? 0 : index;
	toggleview(i);
	if (!cm->seltags[i])
		return;
	for (c = scr->stack; c; c = c->snext) {
		if (c->tags[i] && !c->is.bastard) {	/* XXX: c->can.focus? */
			focus(c);
			break;
		}
	}
	restack();
}

void
unmanage(Client * c, WithdrawCause cause) {
	XWindowChanges wc;
	Bool doarrange, dostruts;
	Window trans = None;

	doarrange = !(c->skip.arrange || c->is.floater ||
		(cause != CauseDestroyed &&
		 XGetTransientForHint(dpy, c->win, &trans))) ||
		c->is.bastard;
	dostruts = c->with.struts;
	c->can.focus = 0;
	if (c == give)
		givefocus(NULL);
	if (c == take)
		takefocus(NULL);
	if (sel == c)
		focuslast(c);
	/* The server grab construct avoids race conditions. */
	XGrabServer(dpy);
	XSelectInput(dpy, c->frame, NoEventMask);
	XUnmapWindow(dpy, c->frame);
	XSetErrorHandler(xerrordummy);
	c->is.managed = False;
	if (c->is.modal)
		togglemodal(c);
#ifdef SYNC
	if (c->sync.alarm) {
		c->sync.waiting = False;
		XSyncDestroyAlarm(dpy, c->sync.alarm);
		XDeleteContext(dpy, c->sync.alarm, context[ClientSync]);
		XDeleteContext(dpy, c->sync.alarm, context[ClientAny]);
		XDeleteContext(dpy, c->sync.alarm, context[ScreenContext]);
		c->sync.alarm = None;
	}
#endif
	if (c->title) {
		XDestroyWindow(dpy, c->title);
		XDeleteContext(dpy, c->title, context[ClientTitle]);
		XDeleteContext(dpy, c->title, context[ClientAny]);
		XDeleteContext(dpy, c->title, context[ScreenContext]);
		c->title = None;
		free(c->element);
	}
	if (c->grips) {
		XDestroyWindow(dpy, c->grips);
		XDeleteContext(dpy, c->grips, context[ClientGrips]);
		XDeleteContext(dpy, c->grips, context[ClientAny]);
		XDeleteContext(dpy, c->grips, context[ScreenContext]);
		c->title = None;
	}
	if (cause != CauseDestroyed) {
		XSelectInput(dpy, c->win, CLIENTMASK & ~MAPPINGMASK);
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (cause != CauseReparented) {
			if (c->gravity == StaticGravity) {
				/* restore static geometry */
				wc.x = c->s.x;
				wc.y = c->s.y;
				wc.width = c->s.w;
				wc.height = c->s.h;
			} else {
				/* restore geometry */
				wc.x = c->r.x;
				wc.y = c->r.y;
				wc.width = c->r.w;
				wc.height = c->r.h;
			}
			wc.border_width = c->s.b;
			XReparentWindow(dpy, c->win, scr->root, wc.x, wc.y);
			XMoveWindow(dpy, c->win, wc.x, wc.y);
			XConfigureWindow(dpy, c->win,
				(CWX|CWY|CWWidth|CWHeight|CWBorderWidth), &wc);
			if (!running)
				XMapWindow(dpy, c->win);
		}
	}
	detach(c);
	detachclist(c);
	detachflist(c);
	detachstack(c);

	ewmh_del_client(c, cause);

	XDestroyWindow(dpy, c->frame);
	XDeleteContext(dpy, c->frame, context[ClientFrame]);
	XDeleteContext(dpy, c->frame, context[ClientAny]);
	XDeleteContext(dpy, c->frame, context[ScreenContext]);
	XDeleteContext(dpy, c->win, context[ClientWindow]);
	XDeleteContext(dpy, c->win, context[ClientAny]);
	XDeleteContext(dpy, c->win, context[ClientPing]);
	XDeleteContext(dpy, c->win, context[ClientDead]);
	XDeleteContext(dpy, c->win, context[ScreenContext]);
	ewmh_release_user_time_window(c);
	removegroup(c, c->leader, ClientGroup);
	if (c->is.grptrans)
		removegroup(c, c->transfor, ClientTransForGroup);
	else if (c->is.transient)
		removegroup(c, c->transfor, ClientTransFor);
#ifdef STARTUP_NOTIFICATION
	if (c->seq)
		sn_startup_sequence_unref(c->seq);
#endif
	free(c->name);
	free(c->icon_name);
	free(c->startup_id);
	free(c);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XUngrabServer(dpy);
	if (dostruts)
		updatestruts();
	else if (doarrange) 
		arrange(NULL);
}

static void
updategeommon(Monitor * m) {
	m->wa = m->sc;
	switch (scr->views[m->curtag].barpos) {
	default:
		m->wa.x += m->struts[LeftStrut];
		m->wa.y += m->struts[TopStrut];
		m->wa.w -= m->struts[LeftStrut] + m->struts[RightStrut];
		m->wa.h -= m->struts[TopStrut] + m->struts[BotStrut];
		break;
	case StrutsHide:
	case StrutsOff:
		break;
	}
	XMoveWindow(dpy, m->veil, m->wa.x, m->wa.y);
	XResizeWindow(dpy, m->veil, m->wa.w, m->wa.h);
}

void
updategeom(Monitor *m) {
	if (!m)
		for (m = scr->monitors; m; m = m->next)
			updategeommon(m);
	else
		updategeommon(m);
}

void
updatestruts() {
	Client *c;
	Monitor *m;

	for (m = scr->monitors; m; m = m->next)
		m->struts[RightStrut]
			= m->struts[LeftStrut]
			= m->struts[TopStrut]
			= m->struts[BotStrut] = 0;
	for (c = scr->clients; c; c = c->next)
		if (c->with.struts)
			getstruts(c);
	ewmh_update_net_work_area();
	updategeom(NULL);
	arrange(NULL);
}

Bool
unmapnotify(XEvent * e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = getclient(ev->window, ClientWindow)) && ev->send_event) {
		CPRINTF(c, "unmanage self-unmapped window\n");
		unmanage(c, CauseUnmapped);
		return True;
	}
	return False;
}

void
updatesizehints(Client * c) {
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize) || !size.flags)
		size.flags = PSize;
	c->flags = size.flags;
	if (c->flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (c->flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (c->flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (c->flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (c->flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (c->flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (c->flags & PAspect) {
		c->minax = size.min_aspect.x;
		c->maxax = size.max_aspect.x;
		c->minay = size.min_aspect.y;
		c->maxay = size.max_aspect.y;
	} else
		c->minax = c->maxax = c->minay = c->maxay = 0;
	if (c->flags & PWinGravity)
		c->gravity = size.win_gravity;
	else
		c->gravity = NorthWestGravity;
	if (c->maxw && c->minw && c->maxw == c->minw) {
		c->can.sizeh = False;
		c->can.maxh = False;
		c->can.fillh = False;
	}
	if (c->maxh && c->minh && c->maxh == c->minh) {
		c->can.sizev = False;
		c->can.maxv = False;
		c->can.fillv = False;
	}
	if (!c->can.sizeh && !c->can.sizev) {
		c->can.size = False;
		c->has.grips = False;
	}
	if (!c->can.maxh && !c->can.maxv)
		c->can.max = False;
	if (!c->can.fillh && !c->can.fillv)
		c->can.fill = False;
}

void
updatetitle(Client * c) {
	if (!gettextprop(c->win, _XA_NET_WM_NAME, &c->name))
		gettextprop(c->win, XA_WM_NAME, &c->name);
	ewmh_update_net_window_visible_name(c);
}

void
updateiconname(Client *c) {
	if (!gettextprop(c->win, _XA_NET_WM_ICON_NAME, &c->icon_name))
		gettextprop(c->win, XA_WM_ICON_NAME, &c->icon_name);
	ewmh_update_net_window_visible_icon_name(c);
}

Group *
getleader(Window leader, int group) {
	Group *g = NULL;

	if (leader)
		XFindContext(dpy, leader, context[group], (XPointer *) &g);
	return g;
}

void
updategroup(Client *c, Window leader, int group, int *nonmodal) {
	Group *g;

	if (leader == None || leader == c->win)
		return;
	if (!(g = getleader(leader, group))) {
		g = emallocz(sizeof(*g));
		g->members = ecalloc(8, sizeof(g->members[0]));
		g->count = 0;
		XSaveContext(dpy, leader, context[group], (XPointer)g);
	} else
		g->members = erealloc(g->members, (g->count + 1) * sizeof(g->members[0]));
	g->members[g->count] = c->win;
	g->count++;
	*nonmodal += g->modal_transients;
}

Window *
getgroup(Client *c, Window leader, int group, unsigned int *count) {
	Group *g;

	if (leader == None) {
		*count = 0;
		return NULL;
	}
	if (!(g = getleader(leader, group))) {
		*count = 0;
		return NULL;
	}
	*count = g->count;
	return g->members;
}

void
removegroup(Client *c, Window leader, int group) {
	Group *g;

	if (leader == None || leader == c->win)
		return;
	if ((g = getleader(leader, group))) {
		Window *list;
		unsigned int i, j;

		list = ecalloc(g->count, sizeof(*list));
		for (i = 0, j = 0; i < g->count; i++)
			if (g->members[i] != c->win)
				list[j++] = g->members[i];
		if (j == 0) {
			free(list);
			free(g->members);
			free(g);
			XDeleteContext(dpy, leader, context[group]);
		} else {
			free(g->members);
			g->members = list;
			g->count = j;
		}
	}
}

void
incmodal(Client *c, Group *g)
{
	Client *t;
	int i;

	g->modal_transients++;
	for (i = 0; i < g->count; i++)
		if ((t = getclient(g->members[i], ClientWindow)) && t != c)
			t->nonmodal++;
}

void
decmodal(Client *c, Group *g)
{
	Client *t;
	int i;

	--g->modal_transients;
	assert(g->modal_transients >= 0);
	for (i = 0; i < g->count; i++)
		if ((t = getclient(g->members[i], ClientWindow)) && t != c) {
			t->nonmodal--;
			assert(t->nonmodal >= 0);
		}
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (ebastardly on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.	*/
int
xerror(Display * dsply, XErrorEvent * ee) {
	if (ee->error_code == BadWindow
	    || (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	    || (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	    || (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	    || (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	    || (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	    || (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	    || (ee->request_code == X_CopyArea && ee->error_code == BadDrawable)
	    || (ee->request_code == 134 && ee->error_code == 134)
	    )
		return 0;
	fprintf(stderr,
	    "adwm: fatal error: request code=%d, error code=%d\n",
	    ee->request_code, ee->error_code);
	return xerrorxlib(dsply, ee);	/* may call exit */
}

int
xerrordummy(Display * dsply, XErrorEvent * ee) {
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display * dsply, XErrorEvent * ee) {
	otherwm = True;
	return -1;
}

void
view(int index) {
	int i, j;
	Monitor *m, *cm;
	int prevtag;

	i = (index == -1) ? 0 : index;
	if (!(cm = selmonitor()))
		return;

	if (cm->seltags[i])
		return;

	memcpy(cm->prevtags, cm->seltags, scr->ntags * sizeof(cm->seltags[0]));

	for (j = 0; j < scr->ntags; j++)
		cm->seltags[j] = 0;
	cm->seltags[i] = True;
	prevtag = cm->curtag;
	cm->curtag = i;
	for (m = scr->monitors; m; m = m->next) {
		if (m->seltags[i] && m != cm) {
			m->curtag = prevtag;
			memcpy(m->prevtags, m->seltags, scr->ntags * sizeof(m->seltags[0]));
			memcpy(m->seltags, cm->prevtags, scr->ntags * sizeof(cm->seltags[0]));
			updategeom(m);
			arrange(m);
		}
	}
	updategeom(cm);
	arrange(cm);
	focus(NULL);
	ewmh_update_net_current_desktop();
}

void
viewprevtag() {
	Bool tmptags[scr->ntags];
	unsigned int i = 0;
	int prevcurtag;
	Monitor *cm;

	if (!(cm = selmonitor()))
		return;

	while (i < scr->ntags - 1 && !cm->prevtags[i])
		i++;
	prevcurtag = cm->curtag;
	cm->curtag = i;

	memcpy(tmptags, cm->seltags, scr->ntags * sizeof(cm->seltags[0]));
	memcpy(cm->seltags, cm->prevtags, scr->ntags * sizeof(cm->seltags[0]));
	memcpy(cm->prevtags, tmptags, scr->ntags * sizeof(cm->seltags[0]));
	if (scr->views[prevcurtag].barpos != scr->views[cm->curtag].barpos)
		updategeom(cm);
	arrange(NULL);
	focus(NULL);
	ewmh_update_net_current_desktop();
}

void
viewlefttag() {
	unsigned int i;
	Monitor *cm;

	if (!(cm = selmonitor()))
		return;

	/* wrap around: TODO: do full _NET_DESKTOP_LAYOUT */
	if (cm->seltags[0]) {
		view(scr->ntags - 1);
		return;
	}
	for (i = 1; i < scr->ntags; i++) {
		if (cm->seltags[i]) {
			view(i - 1);
			return;
		}
	}
}

void
viewrighttag() {
	unsigned int i;
	Monitor *cm;

	if (!(cm = selmonitor()))
		return;

	for (i = 0; i < scr->ntags - 1; i++) {
		if (cm->seltags[i]) {
			view(i + 1);
			return;
		}
	}
	/* wrap around: TODO: do full _NET_DESKTOP_LAYOUT */
	if (i == scr->ntags - 1) {
		view(0);
		return;
	}
}

void
m_prevtag(Client *c, XEvent *e) {
	viewlefttag();
}

void
m_nexttag(Client *c, XEvent *e) {
	viewrighttag();
}

void
zoom(Client *c)
{
	Monitor *m;

	if (!c)
		return;
	if (!(m = c->curmon))
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
m_zoom(Client *c, XEvent *e)
{
	Monitor *m;

	if (!(m = c->curmon))
		return;
	if (isfloating(c, m))
		togglefloating(c);
	else
		zoom(c);
}

int
main(int argc, char *argv[])
{
	char conf[256] = "", *p;
	int i, dummy;

	if (argc == 3 && !strcmp("-f", argv[1]))
		snprintf(conf, sizeof(conf), "%s", argv[2]);
	else if (argc == 2 && !strcmp("-v", argv[1]))
		eprint("adwm-" VERSION " (c) 2011 Alexander Polakov\n");
	else if (argc != 1)
		eprint("usage: adwm [-v] [-f conf]\n");

	setlocale(LC_CTYPE, "");
	if (!(dpy = XOpenDisplay(0)))
		eprint("adwm: cannot open display\n");
	signal(SIGHUP, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGQUIT, sighandler);
	cargc = argc;
	cargv = argv;
	for (i = 0; i < PartLast; i++)
		context[i] = XUniqueContext();
#ifdef XRANDR
	haveext[XrandrBase]
	    = XRRQueryExtension(dpy, &ebase[XrandrBase], &dummy);
	if (haveext[XrandrBase])
		DPRINTF("have RANDR extension with base %d\n", ebase[XrandrBase]);
	else
		DPRINTF("%s", "RANDR extension is not supported\n");
#endif
#ifdef XINERAMA
	haveext[XineramaBase]
	    = XineramaQueryExtension(dpy, &ebase[XineramaBase], &dummy);
	if (haveext[XineramaBase])
		DPRINTF("have XINERAMA extension with base %d\n", ebase[XineramaBase]);
	else
		DPRINTF("%s", "XINERAMA extension is not supported\n");
#endif
#ifdef SYNC
	haveext[XsyncBase]
	    = XSyncQueryExtension(dpy, &ebase[XsyncBase], &dummy);
	if (haveext[XsyncBase])
		DPRINTF("have SYNC extension with base %d\n", ebase[XsyncBase]);
	else
		DPRINTF("%s", "SYNC extension is not supported\n");
#endif
	nscr = ScreenCount(dpy);
	DPRINTF("there are %u screens\n", nscr);
	screens = calloc(nscr, sizeof(*screens));
	for (i = 0, scr = screens; i < nscr; i++, scr++) {
		scr->screen = i;
		scr->root = RootWindow(dpy, i);
		XSaveContext(dpy, scr->root, context[ScreenContext], (XPointer) scr);
		DPRINTF("screen %d has root 0x%lx\n", scr->screen, scr->root);
#ifdef IMLIB2
		scr->context = imlib_context_new();
		imlib_context_push(scr->context);
		imlib_context_set_display(dpy);
		imlib_context_set_drawable(RootWindow(dpy, scr->screen));
		imlib_context_set_colormap(DefaultColormap(dpy, scr->screen));
		imlib_context_set_visual(DefaultVisual(dpy, scr->screen));
		imlib_context_set_anti_alias(1);
		imlib_context_pop();
#endif
	}
	if ((p = getenv("DISPLAY")) && (p = strrchr(p, '.')) && strlen(p + 1)
	    && strspn(p + 1, "0123456789") == strlen(p + 1) && (i = atoi(p + 1)) < nscr) {
		DPRINTF("managing one screen: %d\n", i);
		screens[i].managed = True;
	} else
		for (scr = screens; scr < screens + nscr; scr++)
			scr->managed = True;
	for (scr = screens; scr < screens + nscr; scr++)
		if (scr->managed) {
			DPRINTF("checking screen %d\n", scr->screen);
			checkotherwm();
		}
	for (scr = screens; scr < screens + nscr && !scr->managed; scr++) ;
	if (scr == screens + nscr)
		eprint("adwm: another window manager is already running on each screen\n");
	setup(conf);
	for (scr = screens; scr < screens + nscr; scr++)
		if (scr->managed) {
			DPRINTF("scanning screen %d\n", scr->screen);
			scan();
		}
	DPRINTF("%s", "entering main event loop\n");
	run();
	cleanup(CauseQuitting);

	XCloseDisplay(dpy);
	return 0;
}
