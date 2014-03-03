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
#include "draw.h"
#include "ewmh.h"
#include "layout.h"
#include "parse.h"
#include "tags.h"
#include "config.h"

#define EXTRANGE    16		/* all X11 extension event must fit in this range */

/* function declarations */
void arrange(Monitor *m);
Bool canfocus(Client *c);
void compileregs(void);
Group *getleader(Window leader, int group);
Monitor *findcurmonitor(Client *c);
Client *focusforw(Client *c);
Client *focusback(Client *c);
Client *focuslast(Client *c);
long getstate(Window w);
void incmodal(Client *c, Group *g);
void decmodal(Client *c, Group *g);
void freemonitors(void);
void updatemonitors(XEvent *e, int n, Bool size, Bool full);
void manage(Window w, XWindowAttributes *wa);
void m_shade(Client *c, XEvent *ev);
void m_zoom(Client *c, XEvent *ev);
void m_spawn(Client *c, XEvent *ev);
void m_prevtag(Client *c, XEvent *ev);
void m_nexttag(Client *c, XEvent *ev);
void reconfigure(Client *c, ClientGeometry * g);
void restack_belowif(Client *c, Client *sibling);
void run(void);
void scan(void);
void setfocus(Client *c);
void setup(char *);
void tag(Client *c, int index);
void unmanage(Client *c, WithdrawCause cause);
void updatestruts(void);
void updatesizehints(Client *c);
void updatetitle(Client *c);
void updategroup(Client *c, Window leader, int group, int *nonmodal);
Window *getgroup(Client *c, Window leader, int group, unsigned int *count);
void removegroup(Client *c, Window leader, int group);
void updateiconname(Client *c);
int xerror(Display *dpy, XErrorEvent *ee);
int xerrordummy(Display *dsply, XErrorEvent *ee);
int xerrorstart(Display *dsply, XErrorEvent *ee);
int (*xerrorxlib) (Display *, XErrorEvent *);

Bool isdockapp(Window win);
Bool issystray(Window win);
void delsystray(Window win);

/* variables */
volatile int signum = 0;
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
Client *gave;				/* gave focus last */
Client *took;				/* took focus last */
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

void (*actions[LastOn][5][2]) (Client *, XEvent *) = {
	/* *INDENT-OFF* */
	/* OnWhere */
	[OnClientTitle]	 = {
				/* ButtonPress	    ButtonRelease */
		[Button1-1] =	{ m_move,	    NULL	    },
		[Button2-1] =	{ NULL,		    m_zoom	    },
		[Button3-1] =	{ m_resize,	    NULL	    },
		[Button4-1] =	{ NULL,		    m_shade	    },
		[Button5-1] =	{ NULL,		    m_shade	    },
	},
	[OnClientGrips]  = {
		[Button1-1] =	{ m_resize,	    NULL	    },
		[Button2-1] =	{ NULL,		    NULL	    },
		[Button3-1] =	{ NULL,		    NULL	    },
		[Button4-1] =	{ NULL,		    NULL	    },
		[Button5-1] =	{ NULL,		    NULL	    },
	},
	[OnClientFrame]	 = {
		[Button1-1] =	{ m_resize,	    NULL	    },
		[Button2-1] =	{ NULL,		    NULL	    },
		[Button3-1] =	{ NULL,		    NULL	    },
		[Button4-1] =	{ NULL,		    m_shade	    },
		[Button5-1] =	{ NULL,		    m_shade	    },
	},
	[OnClientDock]   = {
		[Button1-1] =	{ m_move,	    NULL	    },
		[Button2-1] =	{ NULL,		    NULL	    },
		[Button3-1] =	{ NULL,		    NULL	    },
		[Button4-1] =	{ NULL,		    NULL	    },
		[Button5-1] =	{ NULL,		    NULL	    },
	},
	[OnClientWindow] = {
		[Button1-1] =	{ m_move,	    NULL	    },
		[Button2-1] =	{ m_zoom,	    NULL	    },
		[Button3-1] =	{ m_resize,	    NULL	    },
		[Button4-1] =	{ m_shade,	    NULL	    },
		[Button5-1] =	{ m_shade,	    NULL	    },
	},
	[OnClientIcon] = {
		[Button1-1] =	{ m_move,	    NULL	    },
		[Button2-1] =	{ NULL,		    NULL	    },
		[Button3-1] =	{ NULL,		    NULL	    },
		[Button4-1] =	{ NULL,		    NULL	    },
		[Button5-1] =	{ NULL,		    NULL	    },
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

static Bool
IGNOREEVENT(XEvent *e)
{
	XPRINTF("Got ignored event %d\n", e->type);
	return False;
}

static Bool keypress(XEvent *e);
static Bool keyrelease(XEvent *e);
static Bool buttonpress(XEvent *e);
static Bool enternotify(XEvent *e);
static Bool leavenotify(XEvent *e);
static Bool focuschange(XEvent *e);
static Bool expose(XEvent *e);
static Bool destroynotify(XEvent *e);
static Bool unmapnotify(XEvent *e);
static Bool maprequest(XEvent *e);
static Bool reparentnotify(XEvent *e);
static Bool configurenotify(XEvent *e);
Bool configurerequest(XEvent *e);
static Bool propertynotify(XEvent *e);
Bool selectionclear(XEvent *e);
Bool clientmessage(XEvent *e);
static Bool mappingnotify(XEvent *e);
static Bool initmonitors(XEvent *e);
static Bool alarmnotify(XEvent *e);

Bool (*handler[LASTEvent + (EXTRANGE * BaseLast)]) (XEvent *) = {
	[KeyPress] = keypress,
	    [KeyRelease] = keyrelease,
	    [ButtonPress] = buttonpress,
	    [ButtonRelease] = buttonpress,
	    [MotionNotify] = IGNOREEVENT,
	    [EnterNotify] = enternotify,
	    [LeaveNotify] = leavenotify,
	    [FocusIn] = focuschange,
	    [FocusOut] = focuschange,
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
	    [MappingNotify] = mappingnotify,[GenericEvent] = IGNOREEVENT,
#ifdef XRANDR
	    [RRScreenChangeNotify + LASTEvent + EXTRANGE * XrandrBase] = initmonitors,
#endif
#ifdef SYNC
	    [XSyncAlarmNotify + LASTEvent + EXTRANGE * XsyncBase] = alarmnotify,
#endif
};

/* function implementations */
static void
applyatoms(Client *c)
{
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
			if (i == tag)
				c->tags |= (1ULL << i);
			else
				c->tags &= ~(1ULL << i);
	}
}

static void
applyrules(Client *c)
{
	static char buf[512];
	unsigned int i, j;
	regmatch_t tmp;
	Bool matched = False;
	XClassHint ch = { 0 };
	Monitor *cm = c->cmon ? : selmonitor();  /* XXX: is cmon ever not NULL? */

	/* rule matching */
	XGetClassHint(dpy, c->win, &ch);
	snprintf(buf, sizeof(buf), "%s:%s:%s",
		 ch.res_class ? ch.res_class : "", ch.res_name ? ch.res_name : "",
		 c->name);
	buf[LENGTH(buf) - 1] = 0;
	for (i = 0; i < nrules; i++)
		if (rules[i]->propregex && !regexec(rules[i]->propregex, buf, 1, &tmp, 0)) {
			c->skip.arrange = rules[i]->isfloating;
			if (!(c->has.title = rules[i]->hastitle))
				c->has.grips = False;
			for (j = 0; rules[i]->tagregex && j < scr->ntags; j++) {
				if (!regexec
				    (rules[i]->tagregex, scr->tags[j].name, 1, &tmp, 0)) {
					matched = True;
					c->tags |= (1ULL << j);
				}
			}
		}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	if (!matched && cm)
		c->tags = cm->curview->seltags;
}

static void
setwmstate(Window win, long state, Window icon)
{
	if (state == WithdrawnState)
		XDeleteProperty(dpy, win, _XA_WM_STATE);
	else {
		long data[] = { state, icon };
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
}

static void
applystate(Client *c, XWMHints * wmh)
{
	int state = NormalState;

	if (wmh && (wmh->flags & StateHint))
		state = wmh->initial_state;

	switch (state) {
#if 0
	case DontCareState:	/* obsolete */
		/* don't know or don't care */
		break;
#endif
	case ZoomState:	/* obsolete */
		/* application wants to start zoomed */
		c->is.full = True;
		/* fall through */
	case NormalState:
	default:
		/* most applications want to start this way */
		c->is.icon = False;
		c->is.hidden = False;
		state = NormalState;
		break;
	case WithdrawnState:
		/* for windows that are not mapped */
		c->is.dockapp = True;
		c->is.above = True;	/* for now */
		c->skip.skip = -1U;
		c->skip.arrange = False;
		c->skip.sloppy = False;
		c->can.can = 0;	/* maybe move later */
		c->can.move = True;
		c->has.has = 0;
		c->is.floater = True;
		c->icon = ((wmh->flags & IconWindowHint) && wmh->icon_window) ?
		    wmh->icon_window : c->win;
		/* fall through */
	case IconicState:
		/* application wants to start as icon */
		c->is.icon = True;
		c->is.hidden = False;
		state = IconicState;
		break;
	case InactiveState:	/* obsolete */
		/* application believes it is seldom used: some wm's may put it on
		   inactive menu. */
		c->is.icon = False;
		c->is.hidden = True;
		state = NormalState;
		break;
	}
	c->winstate = state;
}

static void
setclientstate(Client *c, long state)
{
	if (c->is.dockapp && state == NormalState)
		state = IconicState;

	switch (state) {
	case WithdrawnState:
		break;
	case NormalState:
	default:
		if (c->is.icon) {
			c->is.icon = False;
			ewmh_update_net_window_state(c);
		}
		break;
	case IconicState:
		if (!c->is.icon) {
			c->is.icon = True;
			ewmh_update_net_window_state(c);
		}
		break;
	}
	if (c->winstate != state) {
		setwmstate(c->win, state, c->icon ? c->frame : None);
		c->winstate = state;
	}
}

void
ban(Client *c)
{
	c->cmon = NULL;

	setclientstate(c, c->is.icon ? IconicState : NormalState);
	if (!c->is.banned) {
		c->is.banned = True;
		XUnmapWindow(dpy, c->frame);
	}
}

void
unban(Client *c, Monitor *m)
{
	c->cmon = m;
	c->cview = m->curview;
	if (c->is.banned) {
		XMapWindow(dpy, c->frame);
		c->is.banned = False;
		setclientstate(c, NormalState);
	}
}

static Bool
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

	pushtime(ev->time);

	DPRINTF
	    ("BUTTON %d: window 0x%lx root 0x%lx subwindow 0x%lx time %ld x %d y %d x_root %d y_root %d state 0x%x\n",
	     ev->button, ev->window, ev->root, ev->subwindow, ev->time, ev->x, ev->y,
	     ev->x_root, ev->y_root, ev->state);

	if (ev->window != scr->root)
		button_mask &= ~(1 << button);

	if (ev->window == scr->root) {
		DPRINTF("SCREEN %d: 0x%lx button: %d\n", scr->screen, ev->window,
			ev->button);
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
		DPRINTF("TITLE %s: 0x%lx button: %d\n", c->name, ev->window, ev->button);
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
					DPRINTF("ELEMENT %d PRESSED\n", i);
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
						DPRINTF("ELEMENT %d RELEASED\n", i);
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
	} else if ((c = getclient(ev->window, ClientIcon)) && ev->window == c->icon) {
		DPRINTF("ICON %s: 0x%lx button: %d\n", c->name, ev->window, ev->button);
		if ((CLEANMASK(ev->state) & modkey) != modkey) {
			XAllowEvents(dpy, ReplayPointer, CurrentTime);
			return True;
		}
		if ((action = actions[OnClientIcon][button][direct]))
			(*action) (c, (XEvent *) ev);
		XUngrabPointer(dpy, CurrentTime);	// ev->time ??
		drawclient(c);
	} else if ((c = getclient(ev->window, ClientWindow)) && ev->window == c->win) {
		DPRINTF("WINDOW %s: 0x%lx button: %d\n", c->name, ev->window, ev->button);
		if ((CLEANMASK(ev->state) & modkey) != modkey) {
			XAllowEvents(dpy, ReplayPointer, CurrentTime);
			return True;
		}
		if ((action = actions[OnClientWindow][button][direct]))
			(*action) (c, (XEvent *) ev);
		XUngrabPointer(dpy, CurrentTime);	// ev->time ??
		drawclient(c);
	} else if ((c = getclient(ev->window, ClientFrame)) && ev->window == c->frame) {
		DPRINTF("FRAME %s: 0x%lx button: %d\n", c->name, ev->window, ev->button);
		if (c->is.dockapp) {
			if ((action = actions[OnClientDock][button][direct]))
				(*action) (c, (XEvent *) ev);
		} else {
			if ((action = actions[OnClientFrame][button][direct]))
				(*action) (c, (XEvent *) ev);
		}
		XUngrabPointer(dpy, CurrentTime);	// ev->time ??
		drawclient(c);
	} else
		return False;
	return True;
}

static Bool
selectionreleased(Display *display, XEvent *event, XPointer arg)
{
	if (event->type == DestroyNotify) {
		if (event->xdestroywindow.window == (Window) arg) {
			return True;
		}
	}
	return False;
}

Bool
selectionclear(XEvent *e)
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

static void
checkotherwm(void)
{
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
					  DisplayHeight(dpy, scr->screen), 1, 1, 0, 0L,
					  0L);
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

		XIfEvent(dpy, &event_return, &selectionreleased, (XPointer) wm_sn_owner);
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
			     "adwm " VERSION, cargv, cargc, NULL, NULL, &class_hint);
	XSetWMClientMachine(dpy, scr->selwin, &hname);
	XInternAtoms(dpy, names, 25, False, atoms);
	XChangeProperty(dpy, scr->selwin, wm_protocols, XA_ATOM, 32,
			PropModeReplace, (unsigned char *) atoms,
			sizeof(atoms) / sizeof(atoms[0]));

	manager_event.display = dpy;
	manager_event.type = ClientMessage;
	manager_event.window = scr->root;
	manager_event.message_type = manager;
	manager_event.format = 32;
	manager_event.data.l[0] = CurrentTime;	/* FIXME: timestamp */
	manager_event.data.l[1] = wm_sn;
	manager_event.data.l[2] = scr->selwin;
	manager_event.data.l[3] = 2;
	manager_event.data.l[4] = 0;
	XSendEvent(dpy, scr->root, False, StructureNotifyMask, (XEvent *) &manager_event);
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

static void
cleanup(WithdrawCause cause)
{

	for (scr = screens; scr < screens + nscr; scr++) {
		while (scr->stack) {
			unban(scr->stack, NULL);
			unmanage(scr->stack, cause);
		}
	}

	for (scr = screens; scr < screens + nscr; scr++) {
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
send_configurenotify(Client *c, Window above)
{
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
	ce.border_width = c->c.b;	/* ICCCM 2.0 4.1.5 */
	ce.above = above;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *) &ce);
}

static Bool
configurenotify(XEvent *e)
{
	XConfigureEvent *ev = &e->xconfigure;

	if (ev->window == scr->root) {
		initmonitors(e);
		return True;
	}
	return False;
}

Bool
configurerequest(XEvent *e)
{
	Client *c;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;

	if ((c = getclient(ev->window, ClientWindow))) {
		configureclient(e, c, c->gravity);
	} else {
		XWindowChanges wc;

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

static Bool
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = getclient(ev->window, ClientWindow))) {
		XPRINTF("unmanage destroyed window (%s)\n", c->name);
		unmanage(c, CauseDestroyed);
		return True;
	}
	if (XFindContext(dpy, ev->window, context[SysTrayWindows],
			 (XPointer *) &c) == Success) {
		XDeleteContext(dpy, ev->window, context[SysTrayWindows]);
		delsystray(ev->window);
		return True;
	}
	XDeleteContext(dpy, ev->window, context[ScreenContext]);
	return True;
}

void *
ecalloc(size_t nmemb, size_t size)
{
	void *res = calloc(nmemb, size);

	if (!res)
		eprint("fatal: could not calloc() %z x %z bytes\n", nmemb, size);
	return res;
}

void *
emallocz(size_t size)
{
	return ecalloc(1, size);
}

void *
erealloc(void *ptr, size_t size)
{
	void *res;

	if (!(res = realloc(ptr, size)))
		eprint("fatal: could not realloc() %z bytes\n", size);
	return res;
}

static Bool
enternotify(XEvent *e)
{
	XCrossingEvent *ev = &e->xcrossing;
	Window win = ev->window, froot = scr->root, fparent = None, *children = NULL;
	unsigned nchild = 0;
	Client *c;

	if (ev->mode != NotifyNormal || ev->detail == NotifyInferior)
		return True;

	while (XQueryTree(dpy, win, &froot, &fparent, &children, &nchild)) {
		if (children)
			XFree(children);
		if (win == froot || fparent == froot)
			break;
		win = fparent;
	}
	XFindContext(dpy, froot, context[ScreenContext], (XPointer *) &scr);

	if ((c = getclient(ev->window, ClientAny))) {
		CPRINTF(c, "EnterNotify received\n");
		enterclient(e, c);
	} else if (ev->window == scr->root) {
		DPRINTF("Not focusing root\n");
		if (took)
			focus(took);
	} else {
		DPRINTF("Unknown entered window 0x%08lx\n", ev->window);
		return False;
	}
	return True;
}

void
dumpstack()
{
	void *buffer[32];
	int nptr;
	char **strings;
	int i;

	if ((nptr = backtrace(buffer, 32)) && (strings = backtrace_symbols(buffer, nptr)))
		for (i = 0; i < nptr; i++)
			fprintf(stderr, "%s\n", strings[i]);
}

void
eprint(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);

	dumpstack();
	exit(EXIT_FAILURE);
}

static Bool
focuschange(XEvent *e)
{
	XEvent ev;
	Window win, froot = scr->root, fparent = None, *children = NULL;
	int revert;
	unsigned nchild = 0;
	Client *c;

	/* Different approach: don't force focus, just track it.  When it goes to
	   PointerRoot or None, set it to something reasonable. */

	XSync(dpy, False);
	/* discard all subsequent focus change events */
	while (XCheckMaskEvent(dpy, FocusChangeMask, &ev)) ;

	XGetInputFocus(dpy, &win, &revert);
	while (XQueryTree(dpy, win, &froot, &fparent, &children, &nchild)) {
		if (children)
			XFree(children);
		if (win == froot || fparent == froot)
			break;
		win = fparent;
	}
	XFindContext(dpy, froot, context[ScreenContext], (XPointer *) &scr);

	switch (win) {
	case None:
	case PointerRoot:
		focus(gave);
		break;
	default:
		if ((c = getclient(win, ClientAny))) {
			if (gave && c != gave) {
				if (took != gave && !(gave->can.focus & TAKE_FOCUS)) {
					CPRINTF(c, "stole focus\n");
					CPRINTF(gave, "giving back focus\n");
					focus(gave);
					return True;
				}
			}
		}
		tookfocus(c);
		break;
	}
	return True;
}

static Bool
expose(XEvent *e)
{
	XExposeEvent *ev = &e->xexpose;
	Client *c;

	if ((c = getclient(ev->window, ClientAny))) {
		XEvent tmp;

		XSync(dpy, False);
		/* discard all exposures for the same window */
		while (XCheckWindowEvent(dpy, ev->window, ExposureMask, &tmp)) ;
		drawclient(c);
		return True;
	}
	return False;
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
	/* more simple */
	if (c && canfocus(c)) {
		if (c->can.focus & TAKE_FOCUS) {
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
		} else if (c->can.focus & GIVE_FOCUS)
			XSetInputFocus(dpy, c->win, RevertToPointerRoot, user_time);
		gave = c;
	} else
		XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
}

void
focus(Client *c)
{
	Client *o;
	Monitor *cm;

	cm = c ? c->cmon : selmonitor();

	o = sel;
	if ((!c && scr->managed)
	    || (c && (c->is.bastard || !canfocus(c) || !isvisible(c, cm))))
		for (c = scr->flist;
		     c && (c->is.bastard || !canfocus(c)
			   || (!c->is.dockapp && (c->is.icon || c->is.hidden))
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
	if (c && c != o) {
		if (c->is.attn)
			c->is.attn = False;
		/* FIXME: why would it be otherwise if it is focusable? Also, a client
		   could take the focus because its child window is an icon (e.g.
		   dockapp). */
		setclientstate(c, NormalState);
		/* drawclient does this does it not? */
		XSetWindowBorder(dpy, sel->frame, scr->style.color.sel[ColBorder]);
		drawclient(c);
		if (c->is.shaded && scr->options.autoroll)
			arrange(cm);
		raisetiled(c);
		ewmh_update_net_window_state(c);
	}
	if (o && o != sel) {
		drawclient(o);
		if (o->is.shaded && scr->options.autoroll)
			arrange(cm);
		lowertiled(o);
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
	     c && (!canfocus(c) || c->skip.focus || c->is.dockapp || !c->is.icon
		   || !c->can.min || !isvisible(c, cm)); c = c->next) ;
	if (!c)
		return;
	if (!c->is.dockapp && c->is.icon) {
		c->is.icon = False;
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
	if (!(m = c->cmon))
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
focusback(Client *c)
{
	Monitor *m;

	if (!c)
		return (c);
	if (!(m = c->cmon))
		return NULL;
	for (c = c->prev;
	     c && (!canfocus(c) || c->skip.focus || (c->is.icon || c->is.hidden)
		   || !isvisible(c, m)); c = c->prev) ;
	if (!c) {
		for (c = scr->clients; c && c->next; c = c->next) ;
		for (; c && (!canfocus(c) || c->skip.focus || (c->is.icon || c->is.hidden)
			     || !isvisible(c, m)); c = c->prev) ;
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
	if (!(m = c->cmon))
		return (NULL);
	for (s = scr->flist; s &&
	     (s == c
	      || (s->is.bastard || !canfocus(s)
		  || (!c->is.dockapp && (s->is.icon || s->is.hidden))
		  || !isvisible(s, m))); s = s->fnext) ;
	focus(s);
	return (s);
}

void
focusnext(Client *c)
{
	if ((c = focusforw(c)))
		raiseclient(c);
}

void
focusprev(Client *c)
{
	if ((c = focusback(c)))
		raiseclient(c);
}

void
with_transients(Client *c, void (*each) (Client *, int), int data)
{
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
_iconify(Client *c, int dummy)
{
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
iconify(Client *c)
{
	if (!c || (!c->can.min && c->is.managed))
		return;
	return with_transients(c, &_iconify, 0);
}

void
iconifyall(Monitor *m)
{
	Client *c;

	for (c = scr->clients; c; c = c->next) {
		if (!isvisible(c, m))
			continue;
		if (c->is.bastard || c->is.dockapp)
			continue;
		iconify(c);
	}
}

static void
_deiconify(Client *c, int dummy)
{
	if (!c->is.icon)
		return;
	c->is.icon = False;
	if (c->is.managed)
		arrange(NULL);
}

void
deiconify(Client *c)
{
	if (!c || (!c->can.min && c->is.managed))
		return;
	return with_transients(c, &_deiconify, 0);
}

static void
_hide(Client *c, int dummy)
{
	if (c->is.hidden || !c->can.hide || WTCHECK(c, WindowTypeDock)
	    || WTCHECK(c, WindowTypeDesk))
		return;
	if (c == sel)
		focuslast(c);
	c->is.hidden = True;
	ewmh_update_net_window_state(c);
}

void
hide(Client *c)
{
	if (!c || (!c->can.hide && c->is.managed))
		return;
	return with_transients(c, &_hide, 0);
}

void
hideall(Monitor *m)
{
	Client *c;

	for (c = scr->clients; c; c = c->next) {
		if (!isvisible(c, m))
			continue;
		if (c->is.bastard || c->is.dockapp)
			continue;
		hide(c);
	}
}

static void
_show(Client *c, int dummy)
{
	if (!c->is.hidden)
		return;
	c->is.hidden = False;
	ewmh_update_net_window_state(c);
}

void
show(Client *c)
{
	if (!c || (!c->can.hide && c->is.managed))
		return;
	return with_transients(c, &_show, 0);
}

void
showall(Monitor *m)
{
	Client *c;

	for (c = scr->clients; c; c = c->next) {
		if (!isvisible(c, m))
			continue;
		if (c->is.bastard || c->is.dockapp)
			continue;
		show(c);
	}
}

void
togglehidden(Client *c)
{
	if (!c || !c->can.hide)
		return;
	if (c->is.hidden)
		show(c);
	else
		hide(c);
	arrange(clientmonitor(c));
}

void
toggleshowing()
{
	if ((scr->showing_desktop = scr->showing_desktop ? False : True))
		hideall(NULL);
	else
		showall(NULL);
	ewmh_update_net_showing_desktop();
}

void
setborder(int px)
{
	int border = scr->style.border;

	border = px;
	if (border < 0)
		border = 0;
	if (border > scr->style.titleheight / 2)
		border = scr->style.titleheight / 2;
	if (scr->style.border != border) {
		scr->style.border = border;
		arrange(NULL);
	}
}

void
incborder(int px)
{
	setborder(scr->style.border + px);
}

void
decborder(int px)
{
	setborder(scr->style.border - px);
}

void
setmargin(int px)
{
	int margin = scr->style.margin;

	margin = px;
	if (margin < 0)
		margin = 0;
	if (margin > scr->style.titleheight)
		margin = scr->style.titleheight;
	if (scr->style.margin != margin) {
		scr->style.margin = margin;
		arrange(NULL);
	}
}

void
incmargin(int px)
{
	setmargin(scr->style.margin + px);
}

void
decmargin(int px)
{
	setmargin(scr->style.margin - px);
}

Client *
getclient(Window w, int part)
{
	Client *c = NULL;

	XFindContext(dpy, w, context[part], (XPointer *) &c);
	return c;
}

long
getstate(Window w)
{
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
getresource(const char *resource, const char *defval)
{
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
gettextprop(Window w, Atom atom, char **text)
{
	char **list = NULL, *str;
	int n;
	XTextProperty name;

	XGetTextProperty(dpy, w, &name, atom);
	if (!name.nitems)
		return False;
	if (name.encoding == XA_STRING) {
		if ((str = strdup((char *) name.value))) {
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

	return (m->sc.x <= mx && mx < m->sc.x + m->sc.w && m->sc.y <= my
		&& my < m->sc.y + m->sc.h) ? True : False;
}

Bool
isvisible(Client *c, Monitor *m)
{
	if (!c)
		return False;
	if (!m) {
		for (m = scr->monitors; m; m = m->next)
			if (c->tags & m->curview->seltags)
				return True;
	} else {
		if (c->tags & m->curview->seltags)
			return True;
	}
	return False;
}

void
grabkeys(void)
{
	unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask | LockMask };
	unsigned int i, j;
	KeyCode code;

	XUngrabKey(dpy, AnyKey, AnyModifier, scr->root);
	for (i = 0; i < scr->nkeys; i++) {
		if ((code = XKeysymToKeycode(dpy, scr->keys[i]->keysym))) {
			for (j = 0; j < LENGTH(modifiers); j++)
				XGrabKey(dpy, code, scr->keys[i]->mod | modifiers[j],
					 scr->root, True, GrabModeAsync, GrabModeAsync);
		}
	}
}

static Bool
keyrelease(XEvent *e)
{
	pushtime(e->xkey.time);
	return True;
}

static Bool
keypress(XEvent *e)
{
	Key *k = NULL;
	Bool handled = False;

	XPutBackEvent(dpy, e);

	do {
		XEvent ev;
		KeySym keysym;
		unsigned long mod;

		XMaskEvent(dpy, KeyPressMask | KeyReleaseMask, &ev);
		pushtime(ev.xkey.time);
		keysym = XkbKeycodeToKeysym(dpy, (KeyCode) ev.xkey.keycode, 0, 0);
		mod = CLEANMASK(ev.xkey.state);

		switch (ev.type) {
			Key **kp;
			int i;

		case KeyRelease:
			DPRINTF("KeyRelease: 0x%02lx %s\n", mod, XKeysymToString(keysym));
			/* a key release other than the active key is a release of a
			   modifier indicating a stop */
			if (k && k->keysym != keysym) {
				DPRINTF("KeyRelease: stopping sequence\n");
				if (k->stop)
					k->stop(&ev, k);
				k = NULL;
			}
			break;
		case KeyPress:
			DPRINTF("KeyPress: 0x%02lx %s\n", mod, XKeysymToString(keysym));
			/* a press of a different key, even a modifier, or a press of the 
			   same key with a different modifier mask indicates a stop of
			   the current sequence and the potential start of a new one */
			if (k && (k->keysym != keysym || k->mod != mod)) {
				DPRINTF("KeyPress: stopping sequence\n");
				if (k->stop)
					k->stop(&ev, k);
				k = NULL;
			}
			for (i = 0, kp = scr->keys; !k && i < scr->nkeys; i++, kp++)
				if ((*kp)->keysym == keysym && (*kp)->mod == mod)
					k = *kp;
			if (k) {
				DPRINTF("KeyPress: activating action\n");
				handled = True;
				if (k->func)
					k->func(&ev, k);
				if (k->chain || !k->stop)
					k = NULL;
			}
			break;
		}
	} while (k);
	DPRINTF("Done handling keypress function\n");
	XUngrabKeyboard(dpy, CurrentTime);
	return handled;
}

void
killclient(Client *c)
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

				XSaveContext(dpy, c->win, context[ClientPing],
					     (XPointer) c);
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
	/* NOTE: Before killing the client we should attempt to kill the process using
	   the _NET_WM_PID and WM_CLIENT_MACHINE because XKillClient might still leave
	   the process hanging. Try using SIGTERM first, following up with SIGKILL */
	{
		long *pids;
		unsigned long n = 0;

		pids = getcard(c->win, _XA_NET_WM_PID, &n);
		if (n == 0 && c->leader)
			pids = getcard(c->win, _XA_NET_WM_PID, &n);
		if (n > 0) {
			char hostname[64], *machine;
			pid_t pid = pids[0];

			if (gettextprop(c->win, XA_WM_CLIENT_MACHINE, &machine)
			    || (c->leader
				&& gettextprop(c->leader, XA_WM_CLIENT_MACHINE,
					       &machine))) {
				if (!strncmp(hostname, machine, 64)) {
					XSaveContext(dpy, c->win, context[ClientDead],
						     (XPointer) c);
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

static Bool
leavenotify(XEvent *e)
{
	XCrossingEvent *ev = &e->xcrossing;

	if (!ev->same_screen) {
		XFindContext(dpy, ev->window, context[ScreenContext], (XPointer *) &scr);
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
		if (!(m = getmonitor(x, y)))
			if (!(m = curmonitor()))
				m = nearmonitor();
		c->tags = m->curview->seltags;
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

Bool latertime(Time time);

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
	// c->is.icon = False;
	// c->is.hidden = False;
	wmh = XGetWMHints(dpy, c->win);
	applystate(c, wmh);
	if (c->is.dockapp)
		c->wintype = WTFLAG(WindowTypeDock);
	else {
		ewmh_process_net_window_type(c);
		ewmh_process_kde_net_window_type_override(c);
	}
	if (c->icon) {
		XSaveContext(dpy, c->icon, context[ClientIcon], (XPointer) c);
		if (c->icon != c->win) {
			XSaveContext(dpy, c->icon, context[ClientAny], (XPointer) c);
			XSaveContext(dpy, c->icon, context[ScreenContext],
				     (XPointer) scr);
		}
	}
	// c->tags = 0;
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

	if ((c->with.time) && latertime(c->user_time))
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

	if (wmh) {
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
				c->tags = t->tags;
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

	if (c->icon) {
		c->r.x = c->c.x = 0;
		c->r.y = c->c.y = 0;
		if (c->icon != c->win && XGetWindowAttributes(dpy, c->icon, wa)) {
			c->r.x = wa->x;
			c->r.y = wa->y;
			c->r.w = wa->width;
			c->r.h = wa->height;
			c->r.b = wa->border_width;
			c->c.w = c->r.w + 2 * c->r.x;
			c->c.h = c->r.h + 2 * c->r.y;
		}
	}

	c->with.struts = getstruts(c);

	XGrabButton(dpy, AnyButton, AnyModifier, c->win, True,
		    ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
	if (c->icon && c->icon != c->win)
		XGrabButton(dpy, AnyButton, AnyModifier, c->icon, True,
			    ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
	twa.override_redirect = True;
	twa.event_mask = FRAMEMASK;
	if (c->is.dockapp)
		twa.event_mask |= ExposureMask | MOUSEMASK;
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

	addclient(c, False, True);

	wc.border_width = 0;
	twa.event_mask = CLIENTMASK;
	twa.do_not_propagate_mask = CLIENTNOPROPAGATEMASK;

	if (c->icon) {
		XChangeWindowAttributes(dpy, c->icon, CWEventMask | CWDontPropagate,
					&twa);
		XSelectInput(dpy, c->icon, CLIENTMASK);

		XReparentWindow(dpy, c->icon, c->frame, c->r.x, c->r.y);
		XAddToSaveSet(dpy, c->icon);
		XConfigureWindow(dpy, c->icon, CWBorderWidth, &wc);
		XMapWindow(dpy, c->icon);
	} else {
		XChangeWindowAttributes(dpy, c->win, CWEventMask | CWDontPropagate, &twa);
		XSelectInput(dpy, c->win, CLIENTMASK);

		XReparentWindow(dpy, c->win, c->frame, 0, c->th);
		if (c->grips)
			XReparentWindow(dpy, c->grips, c->frame, 0, c->c.h - c->gh);
		if (c->title)
			XReparentWindow(dpy, c->title, c->frame, 0, 0);
		XAddToSaveSet(dpy, c->win);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
		XMapWindow(dpy, c->win);
	}

	ban(c);

	ewmh_process_net_window_desktop(c);
	ewmh_process_net_window_desktop_mask(c);
	ewmh_process_net_window_sync_request_counter(c);
	ewmh_process_net_window_state(c);
	c->is.managed = True;
	setwmstate(c->win, c->winstate, c->icon ? c->frame : None);
	ewmh_update_net_window_state(c);
	ewmh_update_net_window_desktop(c);

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
	CPRINTF(c, "%-20s: %s\n", "skip.taskbar", c->skip.taskbar ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "skip.pager", c->skip.pager ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "skip.winlist", c->skip.winlist ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "skip.cycle", c->skip.cycle ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "skip.focus", c->skip.focus ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "skip.arrange", c->skip.arrange ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "skip.sloppy", c->skip.sloppy ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.transient", c->is.transient ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.grptrans", c->is.grptrans ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.banned", c->is.banned ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.floater", c->is.floater ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.max", c->is.max ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.maxv", c->is.maxv ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.maxh", c->is.maxh ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.shaded", c->is.shaded ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.icon", c->is.icon ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.fill", c->is.fill ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.modal", c->is.modal ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.above", c->is.above ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.below", c->is.below ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.attn", c->is.attn ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.sticky", c->is.sticky ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.hidden", c->is.hidden ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.bastard", c->is.bastard ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.full", c->is.full ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.focused", c->is.focused ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.dockapp", c->is.dockapp ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.managed", c->is.managed ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "has.border", c->has.border ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "has.grips", c->has.grips ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "has.title", c->has.title ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "has.but.menu", c->has.but.menu ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "has.but.min", c->has.but.min ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "has.but.max", c->has.but.max ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "has.but.close", c->has.but.close ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "has.but.size", c->has.but.size ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "has.but.shade", c->has.but.shade ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "has.but.fill", c->has.but.fill ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "has.but.floats", c->has.but.floats ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "with.struts", c->with.struts ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "with.time", c->with.time ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.move", c->can.move ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.size", c->can.size ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.sizev", c->can.sizev ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.sizeh", c->can.sizeh ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.min", c->can.min ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.max", c->can.max ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.maxv", c->can.maxv ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.maxh", c->can.maxh ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.close", c->can.close ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.shade", c->can.shade ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.stick", c->can.stick ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.full", c->can.full ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.above", c->can.above ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.below", c->can.below ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.fill", c->can.fill ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.fillh", c->can.fillh ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.fillv", c->can.fillv ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.floats", c->can.floats ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.hide", c->can.hide ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.tag", c->can.tag ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.arrange", c->can.arrange ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.focus", c->can.focus ? "true" : "false");
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

static Bool
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
	return True;
}

static Bool
maprequest(XEvent *e)
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
getworkarea(Monitor *m, Workarea *w)
{
	Workarea *wa;

	switch (m->curview->barpos) {
	case StrutsOn:
	default:
		if (m->dock.position)
			wa = &m->dock.wa;
		else
			wa = &m->wa;
		break;
	case StrutsHide:
	case StrutsOff:
		wa = &m->sc;
		break;
	}
	w->x = max(wa->x, 1);
	w->y = max(wa->y, 1);
	w->w = min(wa->x + wa->w, DisplayWidth(dpy, scr->screen) - 1) - w->x;
	w->h = min(wa->y + wa->h, DisplayHeight(dpy, scr->screen) - 1) - w->y;
}

void
getpointer(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	XQueryPointer(dpy, scr->root, &dummy, &dummy, x, y, &di, &di, &dui);
}

Monitor *
getmonitor(int x, int y)
{
	Monitor *m;

	for (m = scr->monitors; m; m = m->next) {
		if ((x >= m->sc.x && x <= m->sc.x + m->sc.w) &&
		    (y >= m->sc.y && y <= m->sc.y + m->sc.h))
			return m;
	}
	return NULL;
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

static int
area_overlap(int xmin1, int ymin1, int xmax1, int ymax1,
	     int xmin2, int ymin2, int xmax2, int ymax2)
{
	int w = 0, h = 0;

	w = segm_overlap(xmin1, xmax1, xmin2, xmax2);
	h = segm_overlap(ymin1, ymax1, ymin2, ymax2);

	return (w && h) ? (w * h) : 0;
}

Monitor *
bestmonitor(int xmin, int ymin, int xmax, int ymax)
{
	int a, area = 0;
	Monitor *m, *best = NULL;

	for (m = scr->monitors; m; m = m->next) {
		if ((a = area_overlap(xmin, ymin, xmax, ymax,
				      m->sc.x, m->sc.y, m->sc.x + m->sc.w,
				      m->sc.y + m->sc.h)) > area) {
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
clientmonitor(Client *c)
{
	Monitor *m;

	assert(c != NULL);
	for (m = scr->monitors; m; m = m->next)
		if (isvisible(c, m))
			return m;
	return NULL;
}

Monitor *
curmonitor()
{
	int x, y;

	getpointer(&x, &y);
	return getmonitor(x, y);
}

Monitor *
selmonitor()
{
	return (sel ? sel->cmon : curmonitor());
}

Monitor *
closestmonitor(int x, int y)
{
	Monitor *m, *near = scr->monitors;
	float mind =
	    hypotf(DisplayHeight(dpy, scr->screen), DisplayWidth(dpy, scr->screen));

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
nearmonitor()
{
	Monitor *m;
	int x, y;

	getpointer(&x, &y);
	if (!(m = getmonitor(x, y)))
		m = closestmonitor(x, y);
	return m;
}

#ifdef SYNC

void
sync_request(Client *c, Time time)
{
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
			 XSyncCACounter | XSyncCAValueType | XSyncCAValue |
			 XSyncCATestType | XSyncCAEvents, &aa);

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
		DPRINTF
		    ("Deferring size request from %dx%d to %dx%d for 0x%08lx 0x%08lx %s\n",
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

static Bool
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
		XPRINTF("Width changed from %d to %u since last request\n", c->sync.w,
			wc.width);
		mask |= CWWidth;
	}
	if ((wc.height = c->c.h - c->th - c->gh) != c->sync.h) {
		XPRINTF("Height changed from %d to %u since last request\n", c->sync.h,
			wc.height);
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

void
m_move(Client *c, XEvent *e)
{
	DPRINT;
	focus(c);
	if (!mousemove(c, e, (e->xbutton.state & ControlMask) ? False : True))
		raiselower(c);
}

void
m_resize(Client *c, XEvent *e)
{
	focus(c);
	if (!mouseresize(c, e, (e->xbutton.state & ControlMask) ? False : True))
		raiselower(c);
}

static Bool
reparentnotify(XEvent *e)
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

static Bool
propertynotify(XEvent *e)
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
				if (!c->is.floater && (c->is.floater = (trans != None))) {
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
				ewmh_process_net_desktop_layout();
			} else
				return False;
		} else
			return False;
	} else
		return False;
	return True;
}

Bool
latertime(Time time)
{
	if (time == CurrentTime)
		return False;
	if (user_time == CurrentTime || (int) time - (int) user_time > 0)
		return True;
	return False;
}

void
pushtime(Time time)
{
	if (latertime(time))
		user_time = time;
}

void
quit(const char *arg)
{
	running = False;
	if (arg) {
		cleanup(CauseSwitching);
		execlp("sh", "sh", "-c", arg, NULL);
		eprint("Can't exec '%s': %s\n", arg, strerror(errno));
	}
}

void
restart(const char *arg)
{
	running = False;
	if (arg) {
		cleanup(CauseSwitching);
		execlp("sh", "sh", "-c", arg, NULL);
		eprint("Can't exec '%s': %s\n", arg, strerror(errno));
	} else {
		char **argv;
		int i;

		/* argv must be NULL terminated and writable */
		argv = calloc(cargc + 1, sizeof(*argv));
		for (i = 0; i < cargc; i++)
			argv[i] = strdup(cargv[i]);

		cleanup(CauseRestarting);
		execvp(argv[0], argv);
		eprint("Can't restart: %s\n", strerror(errno));
	}
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
sn_handler(SnMonitorEvent * event, void *dummy)
{
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

		if (signum) {
			if (signum == SIGHUP)
				quit("HUP!");
			else
				quit(NULL);
			break;
		}

		if (poll(&pfd, 1, -1) == -1) {
			if (errno == EAGAIN || errno == EINTR || errno == ERESTART)
				continue;
			cleanup(CauseRestarting);
			eprint("poll failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		} else {
			if (pfd.revents & (POLLNVAL | POLLHUP | POLLERR)) {
				cleanup(CauseRestarting);
				eprint("poll error\n");
				exit(EXIT_FAILURE);
			}
			if (pfd.revents & POLLIN) {
				while (XPending(dpy) && running) {
					XNextEvent(dpy, &ev);
					scr = geteventscr(&ev);
					if (!handle_event(&ev))
						XPRINTF("WARNING: Event %d not handled\n",
							ev.type);
				}
			}
		}
	}
}

void
delsystray(Window win)
{
	unsigned int i, j;

	for (i = 0, j = 0; i < systray.count; i++) {
		if (systray.members[i] == win)
			continue;
		systray.members[i] = systray.members[j++];
	}
	if (j < systray.count) {
		systray.count = j;
		XChangeProperty(dpy, scr->root, _XA_KDE_NET_SYSTEM_TRAY_WINDOWS,
				XA_WINDOW, 32, PropModeReplace,
				(unsigned char *) systray.members, systray.count);
	}
}

Bool
isdockapp(Window win)
{
#if 0
	XWMHints *wmh;
	Bool ret;

	if ((ret = ((wmh = XGetWMHints(dpy, win)) &&
		    (wmh->flags & StateHint) && (wmh->initial_state == WithdrawnState))))
		setwmstate(win, WithdrawnState, None);
	return ret;
#else
	return False;
#endif
}

Bool
issystray(Window win)
{
	int format, status;
	long *data = NULL;
	unsigned long extra, nitems = 0;
	Atom real;
	Bool ret;
	unsigned int i;

	status =
	    XGetWindowProperty(dpy, win, _XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR, 0L, 1L,
			       False, AnyPropertyType, &real, &format, &nitems, &extra,
			       (unsigned char **) &data);
	if ((ret = (status == Success && real != None))) {
		for (i = 0; i < systray.count && systray.members[i] != win; i++) ;
		if (i == systray.count) {
			XSelectInput(dpy, win, StructureNotifyMask);
			XSaveContext(dpy, win, context[SysTrayWindows],
				     (XPointer) &systray);
			XSaveContext(dpy, win, context[ScreenContext], (XPointer) scr);

			systray.members =
			    erealloc(systray.members, (i + 1) * sizeof(Window));
			systray.members[i] = win;
			systray.count++;
			XChangeProperty(dpy, scr->root, _XA_KDE_NET_SYSTEM_TRAY_WINDOWS,
					XA_WINDOW, 32, PropModeReplace,
					(unsigned char *) systray.members, systray.count);
		}
	} else
		delsystray(win);
	if (data)
		XFree(data);
	return ret;
}

void
scan(void)
{
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

static Bool
isomni(Client *c)
{
	if (!c->is.sticky)
		if ((c->tags & ((1ULL << scr->ntags) - 1)) != ((1ULL << scr->ntags) - 1))
			return False;
	return True;
}

void
freemonitors()
{
	int i;

	for (i = 0; i < scr->nmons; i++)
		XDestroyWindow(dpy, scr->monitors[i].veil);
	free(scr->monitors);
}

void
updatemonitors(XEvent *e, int n, Bool size_update, Bool full_update)
{
	int i, j;
	Client *c;
	Monitor *m;
	int w, h;
	Bool changed;

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
				c->tags = m->curview->seltags;
				if (c->is.managed)
					ewmh_update_net_window_desktop(c);
			}
			for (i = 0; i < scr->nmons; i++) {
				m = scr->monitors + i;
				m->curview = scr->views + (i % scr->ntags);
			}
		}
		if (size_update) {
			updatestruts();
		}
	}
	/* find largest monitor */
	for (w = 0, h = 0, scr->sw = 0, scr->sh = 0, i = 0; i < n; i++) {
		m = scr->monitors + i;
		w = max(w, m->sc.w);
		h = max(h, m->sc.h);
		scr->sw = max(scr->sw, m->sc.x + m->sc.w);
		scr->sh = max(scr->sh, m->sc.y + m->sc.h);
	}
	scr->m.rows = (scr->sh + h - 1) / h;
	scr->m.cols = (scr->sw + w - 1) / w;
	h = scr->sh / scr->m.rows;
	w = scr->sw / scr->m.cols;
	for (i = 0; i < n; i++) {
		m = scr->monitors + i;
		m->row = m->my / h;
		m->col = m->mx / w;
	}
	/* handle insane geometries, push overlaps to the right */
	do {
		changed = False;
		for (i = 0; i < n && !changed; i++) {
			m = scr->monitors + i;
			for (j = i + 1; j < n && !changed; j++) {
				Monitor *o = scr->monitors + j;

				if (m->row != o->row || m->col != o->col)
					continue;
				if (m->mx < o->mx) {
					o->col++;
					scr->m.cols = max(scr->m.cols, o->col + 1);
				} else {
					m->col++;
					scr->m.cols = max(scr->m.cols, m->col + 1);
				}
				changed = True;
			}
		}
	} while (changed);
	ewmh_update_net_desktop_geometry();
}

static Bool
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
			DPRINTF("XINERAMA defines no monitors for screen %d\n",
				scr->screen);
			goto no_xinerama;
		}
		DPRINTF("XINERAMA defineds %d monitors for screen %d\n", n, scr->screen);
		if (n < 2)
			goto no_xinerama;
		for (i = 0; i < n; i++) {
			if (i < scr->nmons) {
				m = &scr->monitors[i];
				if (m->sc.x != si[i].x_org) {
					m->sc.x = m->wa.x = m->dock.wa.x = si[i].x_org;
					m->mx = m->sc.x + m->sc.w / 2;
					full_update = True;
				}
				if (m->sc.y != si[i].y_org) {
					m->sc.y = m->wa.y = m->dock.wa.y = si[i].y_org;
					m->my = m->sc.y + m->sc.h / 2;
					full_update = True;
				}
				if (m->sc.w != si[i].width) {
					m->sc.w = m->wa.w = m->dock.wa.w = si[i].width;
					m->mx = m->sc.x + m->sc.w / 2;
					size_update = True;
				}
				if (m->sc.h != si[i].height) {
					m->sc.h = m->wa.h = m->dock.wa.h = si[i].height;
					m->my = m->sc.y + m->sc.h / 2;
					size_update = True;
				}
				if (m->num != si[i].screen_number) {
					m->num = si[i].screen_number;
					full_update = True;
				}
			} else {
				XSetWindowAttributes wa;

				scr->monitors =
				    erealloc(scr->monitors,
					     (i + 1) * sizeof(*scr->monitors));
				m = &scr->monitors[i];
				full_update = True;
				m->index = i;
				m->sc.x = m->wa.x = m->dock.wa.x = si[i].x_org;
				m->sc.y = m->wa.y = m->dock.wa.y = si[i].y_org;
				m->sc.w = m->wa.w = m->dock.wa.w = si[i].width;
				m->sc.h = m->wa.h = m->dock.wa.h = si[i].height;
				m->dock.position = scr->options.dockpos;
				m->dock.orient = scr->options.dockori;
				m->mx = m->sc.x + m->sc.w / 2;
				m->my = m->sc.y + m->sc.h / 2;
				m->num = si[i].screen_number;
				m->curview = scr->views + (m->num % scr->ntags);
				m->veil =
				    XCreateSimpleWindow(dpy, scr->root, m->sc.x, m->sc.y,
							m->sc.w, m->sc.h, 0, 0, 0);
				wa.background_pixmap = None;
				wa.override_redirect = True;
				XChangeWindowAttributes(dpy, m->veil,
							CWBackPixmap | CWOverrideRedirect,
							&wa);
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
					m->sc.x = m->wa.x = m->dock.wa.x = ci->x;
					m->mx = m->sc.x + m->sc.w / 2;
					full_update = True;
				}
				if (m->sc.y != ci->y) {
					m->sc.y = m->wa.y = m->dock.wa.y = ci->y;
					m->my = m->sc.y + m->sc.h / 2;
					full_update = True;
				}
				if (m->sc.w != ci->width) {
					m->sc.w = m->wa.w = m->dock.wa.w = ci->width;
					m->mx = m->sc.x + m->sc.w / 2;
					size_update = True;
				}
				if (m->sc.h != ci->height) {
					m->sc.h = m->wa.h = m->dock.wa.h = ci->height;
					m->my = m->sc.y + m->sc.h / 2;
					size_update = True;
				}
				if (m->num != i) {
					m->num = i;
					full_update = True;
				}
			} else {
				XSetWindowAttributes wa;

				scr->monitors =
				    erealloc(scr->monitors,
					     (n + 1) * sizeof(*scr->monitors));
				m = &scr->monitors[n];
				full_update = True;
				m->index = n;
				m->dock.position = scr->options.dockpos;
				m->dock.orient = scr->options.dockori;
				m->sc.x = m->wa.x = m->dock.wa.x = ci->x;
				m->sc.y = m->wa.y = m->dock.wa.y = ci->y;
				m->sc.w = m->wa.w = m->dock.wa.w = ci->width;
				m->sc.h = m->wa.h = m->dock.wa.h = ci->height;
				m->mx = m->sc.x + m->sc.w / 2;
				m->my = m->sc.y + m->sc.h / 2;
				m->num = i;
				m->curview = scr->views + (m->num % scr->ntags);
				m->veil =
				    XCreateSimpleWindow(dpy, scr->root, m->sc.x, m->sc.y,
							m->sc.w, m->sc.h, 0, 0, 0);
				wa.background_pixmap = None;
				wa.override_redirect = True;
				XChangeWindowAttributes(dpy, m->veil,
							CWBackPixmap | CWOverrideRedirect,
							&wa);
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
			m->sc.x = m->wa.x = m->dock.wa.x = 0;
			m->mx = m->sc.x + m->sc.w / 2;
			full_update = True;
		}
		if (m->sc.y != 0) {
			m->sc.y = m->wa.y = m->dock.wa.y = 0;
			m->my = m->sc.y + m->sc.h / 2;
			full_update = True;
		}
		if (m->sc.w != DisplayWidth(dpy, scr->screen)) {
			m->sc.w = m->wa.w = m->dock.wa.w = DisplayWidth(dpy, scr->screen);
			m->mx = m->sc.x + m->sc.w / 2;
			size_update = True;
		}
		if (m->sc.h != DisplayHeight(dpy, scr->screen)) {
			m->sc.h = m->wa.h = m->dock.wa.h =
			    DisplayHeight(dpy, scr->screen);
			m->my = m->sc.y + m->sc.h / 2;
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
		m->dock.position = scr->options.dockpos;
		m->dock.orient = scr->options.dockori;
		m->sc.x = m->wa.x = m->dock.wa.x = 0;
		m->sc.y = m->wa.y = m->dock.wa.y = 0;
		m->sc.w = m->wa.w = m->dock.wa.w = DisplayWidth(dpy, scr->screen);
		m->sc.h = m->wa.h = m->dock.wa.h = DisplayHeight(dpy, scr->screen);
		m->mx = m->sc.x + m->sc.w / 2;
		m->my = m->sc.y + m->sc.h / 2;
		m->num = 0;
		m->curview = scr->views + (m->num % scr->ntags);
		m->veil = XCreateSimpleWindow(dpy, scr->root, m->sc.x, m->sc.y,
					      m->sc.w, m->sc.h, 0, 0, 0);
		wa.background_pixmap = None;
		wa.override_redirect = True;
		XChangeWindowAttributes(dpy, m->veil, CWBackPixmap | CWOverrideRedirect,
					&wa);
	}
	updatemonitors(e, n, size_update, full_update);
	return True;
}

void
sighandler(int sig)
{
	if (sig)
		signum = sig;
}

void
setup(char *conf)
{
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
	cursor[CurResizeTopLeft] = XCreateFontCursor(dpy, XC_top_left_corner);
	cursor[CurResizeTop] = XCreateFontCursor(dpy, XC_top_side);
	cursor[CurResizeTopRight] = XCreateFontCursor(dpy, XC_top_right_corner);
	cursor[CurResizeRight] = XCreateFontCursor(dpy, XC_right_side);
	cursor[CurResizeBottomRight] = XCreateFontCursor(dpy, XC_bottom_right_corner);
	cursor[CurResizeBottom] = XCreateFontCursor(dpy, XC_bottom_side);
	cursor[CurResizeBottomLeft] = XCreateFontCursor(dpy, XC_bottom_left_corner);
	cursor[CurResizeLeft] = XCreateFontCursor(dpy, XC_left_side);
	cursor[CurMove] = XCreateFontCursor(dpy, XC_fleur);
	cursor[CurNormal] = XCreateFontCursor(dpy, XC_left_ptr);

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

	initconfig();

	for (scr = screens; scr < screens + nscr; scr++) {
		if (!scr->managed)
			continue;

#ifdef STARTUP_NOTIFICATION
		sn_dpy = sn_display_new(dpy, NULL, NULL);
		sn_ctx =
		    sn_monitor_context_new(sn_dpy, scr->screen, &sn_handler, NULL, NULL);
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

		ewmh_process_net_desktop_layout();
		ewmh_update_net_desktop_layout();
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
m_spawn(Client *c, XEvent *e)
{
	spawn(scr->options.command);
}

void
togglestruts(Monitor *m, View *v)
{
	v->barpos = (v->barpos == StrutsOn)
	    ? (scr->options.hidebastards ? StrutsHide : StrutsOff) : StrutsOn;
	if (m) {
		updategeom(m);
		arrange(m);
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
togglemin(Client *c)
{
	if (!c || (!c->can.min && c->is.managed))
		return;
	if (c->is.icon) {
		c->is.icon = False;
		if (c->is.managed && !c->is.hidden) {
			focus(c);
			arrange(clientmonitor(c));
		}
	} else {
		c->is.icon = True;
		if (c->is.managed)
			iconify(c);
	}
}

void
togglepager(Client *c)
{
	if (!c || c->is.bastard || c->is.dockapp)
		return;
	c->skip.pager = !c->skip.pager;
	ewmh_update_net_window_state(c);
}

void
toggletaskbar(Client *c)
{
	if (!c || c->is.bastard || c->is.dockapp)
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
unmanage(Client *c, WithdrawCause cause)
{
	XWindowChanges wc;
	Bool doarrange, dostruts;
	Window trans = None;

	doarrange = !(c->skip.arrange || c->is.floater ||
		      (cause != CauseDestroyed &&
		       XGetTransientForHint(dpy, c->win, &trans))) ||
	    c->is.bastard || c->is.dockapp;
	dostruts = c->with.struts;
	c->can.focus = 0;
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
		c->grips = None;
	}
	if (cause != CauseDestroyed) {
		XSelectInput(dpy, c->win, CLIENTMASK & ~MAPPINGMASK);
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (c->icon && c->icon != c->win) {
			XSelectInput(dpy, c->icon, CLIENTMASK & ~MAPPINGMASK);
			XUngrabButton(dpy, AnyButton, AnyModifier, c->icon);
		}
		if (cause != CauseReparented) {
			if (c->gravity == StaticGravity || c->is.dockapp) {
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
			if (c->icon) {
				XReparentWindow(dpy, c->icon, scr->root, wc.x, wc.y);
				XMoveWindow(dpy, c->icon, wc.x, wc.y);
				XConfigureWindow(dpy, c->icon,
						 (CWX | CWY | CWWidth | CWHeight |
						  CWBorderWidth), &wc);
			} else {
				XReparentWindow(dpy, c->win, scr->root, wc.x, wc.y);
				XMoveWindow(dpy, c->win, wc.x, wc.y);
				XConfigureWindow(dpy, c->win,
						 (CWX | CWY | CWWidth | CWHeight |
						  CWBorderWidth), &wc);
			}
			if (!running)
				XMapWindow(dpy, c->win);
		}
	}
	delclient(c);

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
	if (c->icon) {
		XDeleteContext(dpy, c->icon, context[ClientIcon]);
		if (c->icon != c->win) {
			XDeleteContext(dpy, c->icon, context[ClientAny]);
			XDeleteContext(dpy, c->icon, context[ScreenContext]);
		}
	}
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
updategeommon(Monitor *m)
{
	m->wa = m->sc;
	switch (m->curview->barpos) {
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
updategeom(Monitor *m)
{
	if (!m)
		for (m = scr->monitors; m; m = m->next)
			updategeommon(m);
	else
		updategeommon(m);
}

void
updatestruts()
{
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

static Bool
unmapnotify(XEvent *e)
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
updatesizehints(Client *c)
{
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
updatetitle(Client *c)
{
	if (!gettextprop(c->win, _XA_NET_WM_NAME, &c->name))
		gettextprop(c->win, XA_WM_NAME, &c->name);
	ewmh_update_net_window_visible_name(c);
}

void
updateiconname(Client *c)
{
	if (!gettextprop(c->win, _XA_NET_WM_ICON_NAME, &c->icon_name))
		gettextprop(c->win, XA_WM_ICON_NAME, &c->icon_name);
	ewmh_update_net_window_visible_icon_name(c);
}

Group *
getleader(Window leader, int group)
{
	Group *g = NULL;

	if (leader)
		XFindContext(dpy, leader, context[group], (XPointer *) &g);
	return g;
}

void
updategroup(Client *c, Window leader, int group, int *nonmodal)
{
	Group *g;

	if (leader == None || leader == c->win)
		return;
	if (!(g = getleader(leader, group))) {
		g = emallocz(sizeof(*g));
		g->members = ecalloc(8, sizeof(g->members[0]));
		g->count = 0;
		XSaveContext(dpy, leader, context[group], (XPointer) g);
	} else
		g->members = erealloc(g->members, (g->count + 1) * sizeof(g->members[0]));
	g->members[g->count] = c->win;
	g->count++;
	*nonmodal += g->modal_transients;
}

Window *
getgroup(Client *c, Window leader, int group, unsigned int *count)
{
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
removegroup(Client *c, Window leader, int group)
{
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
xerror(Display *dsply, XErrorEvent *ee)
{
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
xerrordummy(Display *dsply, XErrorEvent *ee)
{
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dsply, XErrorEvent *ee)
{
	otherwm = True;
	return -1;
}

void
m_prevtag(Client *c, XEvent *e)
{
	viewlefttag();
}

void
m_nexttag(Client *c, XEvent *e)
{
	viewrighttag();
}

void
m_zoom(Client *c, XEvent *e)
{
	zoomfloat(c);
}

AdwmOperations *
get_adwm_ops(const char *name)
{
	char dlfile[128];
	void *handle;
	AdwmOperations *ops = NULL;

	snprintf(dlfile, sizeof(dlfile), "adwm-%s.so", name);
	DPRINTF("attempting to dlopen %s\n", dlfile);
	if ((handle = dlopen(dlfile, RTLD_NOW | RTLD_LOCAL))) {
		DPRINTF("dlopen of %s succeeded\n", dlfile);
		if ((ops = dlsym(handle, "adwm_ops")))
			ops->handle = handle;
		else
			DPRINTF("could not find symbol adwm_ops");
	} else
		DPRINTF("dlopen of %s failed: %s\n", dlfile, dlerror());
	return ops;
}

AdwmOperations *baseops;

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
	signal(SIGTERM, sighandler);
	signal(SIGQUIT, sighandler);
	cargc = argc;
	cargv = argv;

	if (!(baseops = get_adwm_ops("adwm")))
		eprint("could not load base operations\n");

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
		imlib_context_set_dither(1);
		imlib_context_set_blend(1);
		imlib_context_set_mask(None);
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
		eprint
		    ("adwm: another window manager is already running on each screen\n");
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
