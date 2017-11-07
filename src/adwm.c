/* See COPYING file for copyright and license details.  */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <sys/time.h>
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
#include <wordexp.h>
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
#include <X11/extensions/Xfixes.h>
#ifdef RENDER
#include <X11/extensions/Xrender.h>
#include <X11/extensions/render.h>
#endif
#ifdef XCOMPOSITE
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/composite.h>
#endif
#ifdef DAMAGE
#include <X11/extensions/Xdamage.h>
#endif
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
#ifdef SHAPE
#include <X11/extensions/shape.h>
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
#include "actions.h"
#include "config.h"
#include "image.h"

#define EXTRANGE    16		/* all X11 extension event must fit in this range */

/* function declarations */
void compileregs(void);
Group *getleader(Window leader, int group);
Client *focusforw(Client *c);
Client *focusback(Client *c);
long getstate(Window w);
void incmodal(Client *c, Group *g);
void decmodal(Client *c, Group *g);
void freemonitors(void);
void updatemonitors(XEvent *e, int n, Bool size, Bool full);
void manage(Window w, XWindowAttributes *wa);
void restack_belowif(Client *c, Client *sibling);
void run(void);
void scan(void);
void tag(Client *c, int index);
void updatestruts(void);
void updatehints(Client *c);
void updatesizehints(Client *c);
void updatetitle(Client *c);
void updatetransientfor(Client *c);
void updatesession(Client *c);
void updategroup(Client *c, Window leader, int group, int *nonmodal);
Window *getgroup(Window leader, int group, unsigned int *count);
void removegroup(Client *c, Window leader, int group);
void updateiconname(Client *c);
void updatecmapwins(Client *c);
int xerror(Display *dpy, XErrorEvent *ee);
int xerrordummy(Display *dsply, XErrorEvent *ee);
int xerrorstart(Display *dsply, XErrorEvent *ee);
int (*xerrorxlib) (Display *, XErrorEvent *);
int xioerror(Display *dpy);
int (*xioerrorxlib) (Display *);
unsigned long ignore_request = 0;

Bool issystray(Window win);
void manageoverride(Window win, XWindowAttributes *wa);
void delsystray(Window win);

/* variables */
volatile int signum = 0;
int cargc;
char **cargv;
Display *dpy;
AScreen *scr;
AScreen *event_scr;
AScreen *screens;
int nscr;
XdgDirs xdgdirs = { NULL, };

#ifdef STARTUP_NOTIFICATION
SnDisplay *sn_dpy;
#endif
XrmDatabase xrdb;
Bool otherwm;
Bool running = True;
Bool lockfocus = False;
Client *focuslock = NULL;
Client *sel;
Client *gave;				/* gave focus last */
Client *took;				/* took focus last */
Group window_stack = { NULL, 0, 0 };

XContext context[PartLast];
Cursor cursor[CursorLast];
Rule **rules;
int nrules;
unsigned int modkey;
unsigned int numlockmask;
unsigned int scrlockmask;
Time user_time;
Time give_time;
Time take_time;
Group systray = { NULL, 0, 0 };

ExtensionInfo einfo[BaseLast] = {
	/* *INDENT-OFF* */
#if 1
	[XkbBase]	 = { "XKEYBOARD", &XkbUseExtension,		},
#endif
#if 1
	[XfixesBase]	 = { "XFIXES",	  &XFixesQueryVersion,		},
#endif
#ifdef XRANDR
	[XrandrBase]	 = { "RANDR",	  &XRRQueryVersion,		},
#endif
#ifdef XINERAMA
	[XineramaBase]	 = { "XINERAMA",  &XineramaQueryVersion,	},
#endif
#ifdef SYNC
	[XsyncBase]	 = { "SYNC",	  &XSyncInitialize,		},
#endif
#ifdef RENDER
	[XrenderBase]	 = { "RENDER",	  &XRenderQueryVersion,		},
#endif
#ifdef XCOMPOSITE
	[XcompositeBase] = { "Composite", &XCompositeQueryVersion,	},
#endif
#ifdef DAMAGE
	[XdamageBase]	 = { "DAMAGE",	  &XDamageQueryVersion,		},
#endif
#ifdef SHAPE
	[XshapeBase]	 = { "SHAPE",	  &XShapeQueryVersion,		},
#endif
	/* *INDENT-ON* */
};

/* configuration, allows nested code to access above variables */

Bool (*actions[LastOn][Button5-Button1+1][2]) (Client *, XEvent *) = {
	/* *INDENT-OFF* */
	/* OnWhere */
	[OnClientTitle]	 = {
					/* ButtonPress	    ButtonRelease */
		[Button1-Button1] =	{ m_move,	    NULL	    },
		[Button2-Button1] =	{ m_zoom,	    NULL	    },
		[Button3-Button1] =	{ m_resize,	    NULL	    },
		[Button4-Button1] =	{ m_shade,	    NULL	    },
		[Button5-Button1] =	{ m_shade,	    NULL	    },
	},
	[OnClientGrips]  = {
		[Button1-Button1] =	{ m_resize,	    NULL	    },
		[Button2-Button1] =	{ NULL,		    NULL	    },
		[Button3-Button1] =	{ NULL,		    NULL	    },
		[Button4-Button1] =	{ NULL,		    NULL	    },
		[Button5-Button1] =	{ NULL,		    NULL	    },
	},
	[OnClientFrame]	 = {
		[Button1-Button1] =	{ m_resize,	    NULL	    },
		[Button2-Button1] =	{ NULL,		    NULL	    },
		[Button3-Button1] =	{ NULL,		    NULL	    },
		[Button4-Button1] =	{ m_shade,	    NULL	    },
		[Button5-Button1] =	{ m_shade,	    NULL	    },
	},
	[OnClientDock]   = {
		[Button1-Button1] =	{ m_move,	    NULL	    },
		[Button2-Button1] =	{ NULL,		    NULL	    },
		[Button3-Button1] =	{ NULL,		    NULL	    },
		[Button4-Button1] =	{ NULL,		    NULL	    },
		[Button5-Button1] =	{ NULL,		    NULL	    },
	},
	[OnClientWindow] = {
		[Button1-Button1] =	{ m_move,	    NULL	    },
		[Button2-Button1] =	{ m_zoom,	    NULL	    },
		[Button3-Button1] =	{ m_resize,	    NULL	    },
		[Button4-Button1] =	{ m_shade,	    NULL	    },
		[Button5-Button1] =	{ m_shade,	    NULL	    },
	},
	[OnClientIcon] = {
		[Button1-Button1] =	{ m_move,	    NULL	    },
		[Button2-Button1] =	{ NULL,		    NULL	    },
		[Button3-Button1] =	{ NULL,		    NULL	    },
		[Button4-Button1] =	{ NULL,		    NULL	    },
		[Button5-Button1] =	{ NULL,		    NULL	    },
	},
	[OnRoot]	 = {
		[Button1-Button1] =	{ m_spawn3,	    NULL	    },
		[Button2-Button1] =	{ m_spawn2,	    NULL	    },
		[Button3-Button1] =	{ m_spawn,	    NULL	    },
		[Button4-Button1] =	{ m_nexttag,	    NULL	    },
		[Button5-Button1] =	{ m_prevtag,	    NULL	    },
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
static Bool motionnotify(XEvent *e);
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
static Bool colormapnotify(XEvent *e);
Bool clientmessage(XEvent *e);
static Bool mappingnotify(XEvent *e);
static Bool initmonitors(XEvent *e);
#ifdef SYNC
static Bool alarmnotify(XEvent *e);
#endif
#ifdef SHAPE
static Bool shapenotify(XEvent *e);
#endif

Bool (*handler[LASTEvent + (EXTRANGE * BaseLast)]) (XEvent *) = {
	[KeyPress] = keypress,
	[KeyRelease] = keyrelease,
	[ButtonPress] = buttonpress,
	[ButtonRelease] = buttonpress,
	[MotionNotify] = motionnotify,
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
	[ColormapNotify] = colormapnotify,
	[ClientMessage] = clientmessage,
	[MappingNotify] = mappingnotify,
	[GenericEvent] = IGNOREEVENT,
#if 1
	[XFixesDisplayCursorNotify + LASTEvent + EXTRANGE * XfixesBase] = IGNOREEVENT,
	[XFixesCursorNotify + LASTEvent + EXTRANGE * XfixesBase] = IGNOREEVENT,
#endif
#ifdef XRANDR
	[RRScreenChangeNotify + LASTEvent + EXTRANGE * XrandrBase] = initmonitors,
	[RRNotify + LASTEvent + EXTRANGE * XrandrBase] = IGNOREEVENT,
#endif
#ifdef SYNC
	[XSyncAlarmNotify + LASTEvent + EXTRANGE * XsyncBase] = alarmnotify,
#endif
#ifdef DAMAGE
	[XDamageNotify + LASTEvent + EXTRANGE * XdamageBase] = IGNOREEVENT,
#endif
#ifdef SHAPE
	[ShapeNotify + LASTEvent + EXTRANGE * XshapeBase] = shapenotify,
#endif
};

/* function implementations */
static void
applyatoms(Client *c)
{
	long *t;
	unsigned long n;

	/* restore tag number from atom */
	if ((t = getcard(c->win, _XA_NET_WM_DESKTOP, &n))) {
		unsigned long long oldtags = c->tags;
		unsigned long i;
		unsigned tag;

		c->tags = 0;
		for (i = 0; i < n; i++) {
			tag = t[i];
			if (tag == 0xffffffff) {
				c->tags = ((1ULL << scr->ntags) - 1);
				break;
			}
			if (tag >= MAXTAGS)
				continue;
			c->tags |= (1ULL << tag);
		}
		XFree(t);
		if (!c->tags && !c->is.sticky)
			c->tags = oldtags;
	}
}

static Monitor *nearmonitor(void);

static void
applyrules(Client *c)
{
	static char buf[512];
	unsigned int i, j;
	regmatch_t tmp;
	Bool matched = False;
	XClassHint ch = { 0, };
	View *cv = c->cview ? : selview();
	Monitor *cm = (cv && cv->curmon) ? cv->curmon : nearmonitor(); /* XXX: necessary? */

	/* rule matching */
	getclasshint(c, &ch);
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
applystate(Client *c)
{
	XWMHints *wmh;
	int state = NormalState;

	if ((wmh = XGetWMHints(dpy, c->win)) && (wmh->flags & StateHint))
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
		c->is.above = False;
		c->is.below = True;
		c->skip.skip = -1U;
		c->skip.arrange = False;
		c->skip.focus = False;
		c->skip.sloppy = False;
		c->can.can = 0;
		c->can.move = True;
		c->can.select = True;
		c->has.has = 0;
		c->needs.has = 0;
		c->is.floater = True;
		c->is.sticky = True;
		c->tags = ((1ULL << scr->ntags) - 1);
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
	if (wmh)
		XFree(wmh);
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
		setwmstate(c->win, state, c->is.dockapp ? (c->icon ? : c->win) : None);
		c->winstate = state;
	}
}

static Bool
relfocus(Client *c)
{
	Bool refocus = False;

	if (took == c) {
		took = NULL;
		refocus = True;
	}
	if (gave == c) {
		gave = NULL;
		refocus = True;
	}
	if (sel == c) {
		sel = NULL;
		refocus = True;
	}
	return refocus;
}

static Bool
check_unmapnotify(Display *dpy, XEvent *ev, XPointer arg)
{
	Client *c = (typeof(c)) arg;

	return (ev->type == UnmapNotify && !ev->xunmap.send_event
		&& ev->xunmap.window == c->win && ev->xunmap.event == c->frame);
}

void
ban(Client *c)
{
	c->cview = NULL;

	setclientstate(c, c->is.icon ? IconicState : NormalState);
	if (!c->is.banned) {
#if 1
		XEvent ev;
#endif
		c->is.banned = True;
		relfocus(c);
		XUnmapWindow(dpy, c->frame);
#if 0
		XSelectInput(dpy, c->frame, FRAMEMASK & ~SubstructureNotifyMask);
		XSync(dpy, False);
		XUnmapWindow(dpy, c->win);
		XSync(dpy, False);
		XSelectInput(dpy, c->frame, FRAMEMASK);
		XSync(dpy, False);
#else
		if (c->is.dockapp)
			XUnmapWindow(dpy, c->icon ? : c->win);
		else
			XUnmapWindow(dpy, c->win);
		XSync(dpy, False);
		XCheckIfEvent(dpy, &ev, &check_unmapnotify, (XPointer) c);
#endif
	}
}

void
unban(Client *c, View *v)
{
	Bool hide = False;

	if (c->is.shaded)
		hide = True;
	if (v) {
		if (v->barpos == StrutsOff) {
			if (c->is.dockapp) {
				if (!sel || !sel->is.dockapp)
					hide = True;
#if 1
			} else if (WTCHECK(c, WindowTypeDock)) {
				if (!sel || sel != c)
					hide = True;
#endif
			}
		}
		c->cview = v;
	}
	if (hide) {
		if (c->is.dockapp)
			XUnmapWindow(dpy, c->icon ? : c->win);
		else
			XUnmapWindow(dpy, c->win);
	} else {
		if (c->is.dockapp)
			XMapWindow(dpy, c->icon ? : c->win);
		else
			XMapWindow(dpy, c->win);
	}
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
	static int button_proxy = 0;
	Bool (*action) (Client *, XEvent *);
	int button, direct, state;

	if (Button1 > ev->button || ev->button > Button5)
		return False;
	button = ev->button - Button1;
	direct = (ev->type == ButtonPress) ? 0 : 1;
	state = ev->state &
	    ~(Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask);

	pushtime(ev->time);

	DPRINTF
	    ("BUTTON %d: window 0x%lx root 0x%lx subwindow 0x%lx time %ld x %d y %d x_root %d y_root %d state 0x%x\n",
	     ev->button, ev->window, ev->root, ev->subwindow, ev->time, ev->x, ev->y,
	     ev->x_root, ev->y_root, ev->state);

	if (ev->type == ButtonRelease && (button_proxy & (1 << button))) {
		/* if we proxied the press, we must proxy the release too */
		DPRINTF("BUTTON %d: passing to button proxy, state = 0x%08x\n",
			button + Button1, state);
		XSendEvent(dpy, scr->selwin, False, SubstructureNotifyMask, e);
		button_proxy &= ~(1 << button);
		return True;
	}

	if (ev->window == scr->root) {
		DPRINTF("SCREEN %d: 0x%lx button: %d\n", scr->screen, ev->window,
			ev->button);
		/* _WIN_DESKTOP_BUTTON_PROXY */
		/* modifiers or not interested in press or release */
		if (ev->type == ButtonPress) {
			if (state || (!actions[OnRoot][button][0] &&
				      !actions[OnRoot][button][1])) {
				DPRINTF
				    ("BUTTON %d: passing to button proxy, state = 0x%08x\n",
				     button + Button1, state);
				XUngrabPointer(dpy, ev->time);
				XSendEvent(dpy, scr->selwin, False,
					   SubstructureNotifyMask, e);
				button_proxy |= ~(1 << button);
				return True;
			}
		}
		if ((action = actions[OnRoot][button][direct])) {
			DPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
				OnRoot, button + Button1, direct);
			(*action) (NULL, (XEvent *) ev);
		} else
			DPRINTF("No action for On=%d, button=%d, direct=%d\n", OnRoot,
				button + Button1, direct);
		XUngrabPointer(dpy, ev->time);
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
						ec->pressed &= ~(1 << button);
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
				ec->pressed &= ~(1 << button);
				drawclient(c);
			}
		}
		if ((action = actions[OnClientTitle][button][direct])) {
			DPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
				OnClientTitle, button + Button1, direct);
			(*action) (c, (XEvent *) ev);
		} else
			DPRINTF("No action for On=%d, button=%d, direct=%d\n",
				OnClientTitle, button + Button1, direct);
		XUngrabPointer(dpy, ev->time);
		drawclient(c);
	} else if ((c = getclient(ev->window, ClientGrips)) && ev->window == c->grips) {
		if ((action = actions[OnClientGrips][button][direct])) {
			DPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
				ClientGrips, button + Button1, direct);
			(*action) (c, (XEvent *) ev);
		} else
			DPRINTF("No action for On=%d, button=%d, direct=%d\n",
				ClientGrips, button + Button1, direct);
		XUngrabPointer(dpy, ev->time);
		drawclient(c);
	} else if ((c = getclient(ev->window, ClientIcon)) && ev->window == c->icon) {
		DPRINTF("ICON %s: 0x%lx button: %d\n", c->name, ev->window, ev->button);
		if ((CLEANMASK(ev->state) & modkey) != modkey) {
			XAllowEvents(dpy, ReplayPointer, ev->time);
			return True;
		}
		if ((action = actions[OnClientIcon][button][direct])) {
			DPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
				ClientIcon, button + Button1, direct);
			(*action) (c, (XEvent *) ev);
		} else
			DPRINTF("No action for On=%d, button=%d, direct=%d\n", ClientIcon,
				button + Button1, direct);
		XUngrabPointer(dpy, ev->time);
		drawclient(c);
	} else if ((c = getclient(ev->window, ClientWindow)) && ev->window == c->win) {
		DPRINTF("WINDOW %s: 0x%lx button: %d\n", c->name, ev->window, ev->button);
		if ((CLEANMASK(ev->state) & modkey) != modkey) {
			XAllowEvents(dpy, ReplayPointer, ev->time);
			return True;
		}
		if ((action = actions[OnClientWindow][button][direct])) {
			DPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
				OnClientWindow, button + Button1, direct);
			(*action) (c, (XEvent *) ev);
		} else
			DPRINTF("No action for On=%d, button=%d, direct=%d\n",
				OnClientWindow, button + Button1, direct);
		XUngrabPointer(dpy, ev->time);
		drawclient(c);
	} else if ((c = getclient(ev->window, ClientFrame)) && ev->window == c->frame) {
		DPRINTF("FRAME %s: 0x%lx button: %d\n", c->name, ev->window, ev->button);
		if (c->is.dockapp) {
			if ((action = actions[OnClientDock][button][direct])) {
				DPRINTF("Action %p for On=%d, button=%d, direct=%d\n",
					action, OnClientDock, button + Button1, direct);
				(*action) (c, (XEvent *) ev);
			} else
				DPRINTF("No action for On=%d, button=%d, direct=%d\n",
					OnClientDock, button + Button1, direct);
		} else {
			if ((action = actions[OnClientFrame][button][direct])) {
				DPRINTF("Action %p for On=%d, button=%d, direct=%d\n",
					action, OnClientFrame, button + Button1, direct);
				(*action) (c, (XEvent *) ev);
			} else
				DPRINTF("No action for On=%d, button=%d, direct=%d\n",
					OnClientFrame, button + Button1, direct);
		}
		XUngrabPointer(dpy, ev->time);
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

AdwmOperations *baseops;

static void
checkotherwm(void)
{
	Atom wm_sn, wm_protocols, manager;
	Window wm_sn_owner;
	XTextProperty hname = { NULL, };
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

	class_hint.res_name = baseops->name;
	class_hint.res_class = baseops->clas;

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
	xioerrorxlib = XSetIOErrorHandler(xioerror);
	XSync(dpy, False);
}

static void
cleanup(WithdrawCause cause)
{

	for (scr = screens; scr < screens + nscr; scr++) {
		while (scr->stack) {
			unban(scr->stack, NULL);
			DPRINTF("unmanage cleanup\n");
			unmanage(scr->stack, cause);
		}
	}

	for (scr = screens; scr < screens + nscr; scr++) {
		freekeys();
		freemonitors();
	}

	/* free resource database */
	XrmDestroyDatabase(xrdb);

	for (scr = screens; scr < screens + nscr; scr++) {
		freestyle();
		XUngrabKey(dpy, AnyKey, AnyModifier, scr->root);
	}

	XFreeCursor(dpy, cursor[CursorTopLeft]);
	XFreeCursor(dpy, cursor[CursorTop]);
	XFreeCursor(dpy, cursor[CursorTopRight]);
	XFreeCursor(dpy, cursor[CursorRight]);
	XFreeCursor(dpy, cursor[CursorBottomRight]);
	XFreeCursor(dpy, cursor[CursorBottom]);
	XFreeCursor(dpy, cursor[CursorBottomLeft]);
	XFreeCursor(dpy, cursor[CursorLeft]);
	XFreeCursor(dpy, cursor[CursorEvery]);
	XFreeCursor(dpy, cursor[CursorNormal]);

	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XSync(dpy, False);
}

void
send_configurenotify(Client *c, Window above)
{
	XConfigureEvent ce = { 0, };

	/* This is not quite correct.  We need to report the position of the client
	   window in root coordinates, however, we should report it using the specified
	   gravity and without consideration for dercorative borders.  Width and height
	   are correct here but not the position.  Also, the border must be c->s.b. */

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->c.x;
	ce.y = c->c.y + c->c.t;
	if (c->sync.waiting) {
		ce.width = c->sync.w;
		ce.height = c->sync.h;
	} else {
		ce.width = c->c.w - 2 * c->c.v;
		ce.height = c->c.h - c->c.t - c->c.g - c->c.v;
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
		_CPRINTF(c, "unmanage destroyed window\n");
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
motionnotify(XEvent *e)
{
	Client *c;
	XMotionEvent *ev = &e->xmotion;

	pushtime(ev->time);

	if ((c = getclient(ev->window, ClientTitle)) && ev->window == c->title) {
		Bool needdraw = False;
		int i;

		for (i = 0; i < LastElement; i++) {
			ElementClient *ec = &c->element[i];

			if (!ec->present)
				continue;
			if (ec->pressed) {
				if (ec->hovered) {
					ec->hovered = False;
					needdraw = True;
				}
				continue;
			}
			if (ev->x >= ec->g.x && ev->x < ec->g.x + ec->g.w &&
			    ev->y >= ec->g.y && ev->y < ec->g.y + ec->g.h) {
				if (!ec->hovered) {
					ec->hovered = True;
					needdraw = True;
				}
			} else {
				if (ec->hovered) {
					ec->hovered = False;
					needdraw = True;
				}
			}
		}
		if (needdraw)
			drawclient(c);
		return True;
	}
	return False;
}

Colormap *
listcolormaps(AScreen *s, int *nump)
{
	Colormap *list;
	int num = 0;

	if ((list = XListInstalledColormaps(dpy, s->root, &num)))
		if (nump)
			*nump = num;
	return (list);
}

void
installcolormap(AScreen *s, Window w)
{
	XWindowAttributes wa;
	Colormap *list;
	int i, num = 0;

	if (s->colormapnotified)
		return;
	if (!XGetWindowAttributes(dpy, w, &wa))
		return;
	if (!wa.colormap || wa.colormap == DefaultColormap(dpy, s->screen) || wa.colormap == s->colormap)
		return;
	if ((list = listcolormaps(s, &num))) {
		for (i = 0; i < num && list[i] != wa.colormap; i++) ;
		if (i == num)
			XInstallColormap(dpy, wa.colormap);
		XFree(list);
	}
}

void
installcolormaps(AScreen *s, Client *c, Window *w)
{
	if (!w || !*w)
		return;
	installcolormaps(s, c, w + 1);
	installcolormap(s, *w);
}

static Bool
need_discard(Display *dpy, XEvent *ev, XPointer arg)
{
	Client *s = (Client *) arg;
	Client *c;

	geteventscr(ev);
	if (ev->type != EnterNotify || ev->xcrossing.mode != NotifyNormal)
		return False;
	if ((c = findclient(ev->xcrossing.window))) {
		if (ev->xcrossing.detail == NotifyInferior)
			return False;
		if (c == s)
			return False;
		return True;
	}
	if (ev->xcrossing.window == event_scr->root
	    && ev->xcrossing.detail == NotifyInferior)
		if (sel && (WTCHECK(sel, WindowTypeDock) || sel->is.dockapp
			    || sel->is.bastard))
			return True;
	return False;
}

static void
discarding(Client *c)
{
	XEvent ev;

	XSync(dpy, False);
	while (XCheckIfEvent(dpy, &ev, &need_discard, (XPointer) c)) ;
}

void
discardcrossing(Client *c)
{
	lockfocus = True;
	focuslock = c ? : sel;
}

static Bool
enternotify(XEvent *e)
{
	XCrossingEvent *ev = &e->xcrossing;
	Client *c;

	if (e->type != EnterNotify || ev->mode != NotifyNormal)
		return False;

	if ((c = findclient(ev->window))) {
		if (ev->detail == NotifyInferior)
			return False;
		CPRINTF(c, "EnterNotify received\n");
		enterclient(e, c);
		if (c != sel)
			installcolormaps(event_scr, c, c->cmapwins);
		return True;
	} else if ((c = getclient(ev->window, ClientColormap))) {
		installcolormap(event_scr, ev->window);
		return True;
	} else if (ev->window == event_scr->root && ev->detail != NotifyInferior) {
		DPRINTF("Not focusing root\n");
		if (sel && (WTCHECK(sel, WindowTypeDock) || sel->is.dockapp || sel->is.bastard)) {
			focus(NULL);
			return True;
		}
		return False;
	} else {
		DPRINTF("Unknown entered window 0x%08lx\n", ev->window);
		return False;
	}
}

static Bool
colormapnotify(XEvent *e)
{
	XColormapEvent *ev = &e->xcolormap;
	Client *c;

	if ((c = getclient(ev->window, ClientColormap))) {
		if (ev->new) {
			_CPRINTF(c, "colormap for window 0x%lx changed to 0x%lx\n", ev->window, ev->colormap);
			if (c == sel)
				installcolormaps(event_scr, c, c->cmapwins);
			return True;
		} else {
			switch (ev->state) {
			case ColormapInstalled:
				_CPRINTF(c, "colormap 0x%lx for window 0x%lx installed\n", ev->colormap, ev->window);
				return True;
			case ColormapUninstalled:
				_CPRINTF(c, "colormap 0x%lx for window 0x%lx uninstalled\n", ev->colormap, ev->window);
				return True;
			default:
				break;
			}
		}
	}
	return False;
}

const char *
_timestamp(void)
{
	static struct timeval tv = { 0, 0 };
	static char buf[BUFSIZ];
	double stamp;

	gettimeofday(&tv, NULL);
	stamp = (double)tv.tv_sec + (double)((double)tv.tv_usec/1000000.0);
	snprintf(buf, BUFSIZ-1, "%f", stamp);
	return buf;
}

void
dumpstack(const char *file, const int line, const char *func)
{
	void *buffer[32];
	int nptr;
	char **strings;
	int i;

	if ((nptr = backtrace(buffer, 32)) && (strings = backtrace_symbols(buffer, nptr)))
		for (i = 0; i < nptr; i++)
			fprintf(stderr, NAME ": E: %12s +%4d : %s() : \t%s\n", file, line, func, strings[i]);
}

void
eprint(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);

	dumpstack(__FILE__, __LINE__, __func__);
	abort();
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

#ifdef SHAPE
static Bool
shapenotify(XEvent *e)
{
	XShapeEvent *ev = (typeof(ev)) e;
	Client *c;

	if ((c = getclient(ev->window, ClientAny)) && ev->window == c->win) {
		CPRINTF(c, "Got shape notify, redrawing shapes \n");
		if (!c->is.dockapp)
			return configureshapes(c);
	}
	return False;
}
#endif

static Bool
canfocus(Client *c)
{
	if (c && c->can.focus && !c->nonmodal && !c->is.banned)
		return True;
	return False;
}

Bool
canselect(Client *c)
{
	if (c && c->can.select && !c->is.banned)
		return True;
	return False;
}

static Bool
isviewable(Client *c)
{
	XWindowAttributes wa = { 0, };
	Window win;

	if (!c)
		return False;

	win = c->icon ? : c->win;

	if (!XGetWindowAttributes(dpy, win, &wa))
		return False;
	if (wa.map_state != IsViewable)
		return False;
	return True;
}

Bool
selectok(Client *c)
{
	if (!c)
		return False;
	if (!canselect(c))
		return False;
	if ((!c->is.dockapp && c->is.icon) || c->is.hidden)
		return False;
	if (!clientview(c) && !c->can.select) {
		_CPRINTF(c, "attempt to select an unselectable client\n");
		return False;
	}
	if (!isvisible(c, NULL)) {
		_CPRINTF(c, "attempt to select an invisible client\n");
		return False;
	}
	return True;
}

Bool
focusok(Client *c)
{
	if (!c)
		return False;
	if (!canfocus(c))
		return False;
	if (!isviewable(c)) {
		_CPRINTF(c, "attempt to focus unviewable client\n");
		return False;
	}
	return True;
}

static void
setfocus(Client *c)
{
	if (focusok(c)) {
		CPRINTF(c, "setting focus\n");
		if (c->can.focus & GIVE_FOCUS)
			XSetInputFocus(dpy, c->icon ? : c->win, RevertToPointerRoot, user_time);
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
		}
		gave = c;
	} else if (!c) {
		Window win = None;
		int revert = RevertToPointerRoot;

		XGetInputFocus(dpy, &win, &revert);
		if (!win)
			XSetInputFocus(dpy, PointerRoot, revert, user_time);
	} else {
		/* this happens often now */
		CPRINTF(c, "cannot set focus\n");
	}
}

static Bool
shouldsel(Client *c)
{
	if (!selectok(c))
		return False;
	if (c->is.bastard)
		return False;
	if (c->is.dockapp)
		return False;
	return True;
}

static Bool
shouldfocus(Client *c)
{
	if (!focusok(c))
		return False;
	if (c->is.bastard)
		return False;
	if (c->is.dockapp)
		return False;
	return True;
}

Client *
findselect(Client *not)
{
	Client *c;

	for (c = scr->alist; c && (c == not || !shouldsel(c)); c = c->anext) ;
	return (c);
}

Client *
findfocus(Client *not)
{
	Client *c;

	for (c = scr->flist; c && (c == not || !shouldfocus(c)); c = c->fnext) ;
	return (c);
}

static Bool
focuschange(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;
	Client *c = NULL;

	/* Different approach: don't force focus, just track it.  When it goes to
	   PointerRoot or None, set it to something reasonable. */

	DPRINTF("FOCUS: XFocusChangeEvent type %s window 0x%lx mode %d detail %d\n",
		 e->type == FocusIn ? "FocusIn" : "FocusOut", ev->window,
		 ev->mode, ev->detail);

	if (ev->mode != NotifyNormal) {
		DPRINTF("FOCUS: mode = %d != %d\n", e->xfocus.mode, NotifyNormal);
		return True;
	}
	switch (ev->window) {
	case None:
	      none:
		setfocus(NULL);
		return True;
	case PointerRoot:
	      pointerroot:
		/* if nothing else particular is focussed, focus back on the client that
		   last took focus, or focus on something, if possible (when took ==
		   NULL). */
		focus(NULL);
		return True;
	default:
		if (ev->window == event_scr->root) {
			if (ev->detail == NotifyDetailNone)
				goto none;
			if (ev->detail == NotifyPointerRoot)
				goto pointerroot;
			return True;
		}
		break;
	}
	if ((c = findclient(ev->window))) {
		if (c == took && e->type == FocusOut) {
			tookfocus(NULL);
			return True;
		}
		if (c != took && e->type == FocusIn) {
			tookfocus(c);
			return True;
		}
		return True;
	}
	if (e->type != FocusIn)
		return True;
	tookfocus(NULL);
	return True;
}

void
focus(Client *c)
{
	Client *o;

	o = sel;
	/* note that the client does not necessarily have to be focusable to be selected. */
	if ((!c && scr->managed) || (c && !selectok(c)))
		c = findfocus(NULL);

	sel = c;


	if (!scr->managed)
		return;
	ewmh_update_net_active_window();
	if (c && c != o) {
		CPRINTF(c, "selecting\n");
		setselected(c);
		/* clear urgent */
		if (c->is.attn) {
			c->is.attn = False;
			ewmh_update_net_window_state(c);
		}
		/* FIXME: why would it be otherwise if it is focusable? Also, a client
		   could take the focus because its child window is an icon (e.g.
		   dockapp). */
		setclientstate(c, NormalState); /* probably does nothing */
		drawclient(c);
		raisetiled(c); /* this will restack, probably when not necessary */
		XSetWindowBorder(dpy, c->frame, scr->style.color.sele[ColBorder]);
		if ((c->is.shaded && scr->options.autoroll))
			arrange(c->cview);
		else if (o && ((WTCHECK(c, WindowTypeDock)||c->is.dockapp) != (WTCHECK(o, WindowTypeDock)||o->is.dockapp)))
			arrange(c->cview);
	}
	if (o && o != c) {
		CPRINTF(o, "deselecting\n");
		drawclient(o);
		lowertiled(o); /* does nothing at the moment */
		XSetWindowBorder(dpy, o->frame, scr->style.color.norm[ColBorder]);
		if ((o->is.shaded && scr->options.autoroll))
			arrange(o->cview);
		else if (c && ((WTCHECK(o, WindowTypeDock)||o->is.dockapp) != (WTCHECK(c, WindowTypeDock)||c->is.dockapp)) && o->cview != c->cview)
			arrange(o->cview);
	}
	/* note that this does not necessarily change the focus */
	setfocus(sel);
	XSync(dpy, False);
}

void
focusicon()
{
	Client *c;
	View *v;

	if (!(v = selview()))
		return;
	for (c = scr->clients;
	     c && (!canfocus(c) || c->skip.focus || c->is.dockapp || !c->is.icon
		   || !c->can.min || !isvisible(c, v)); c = c->next) ;
	if (!c)
		return;
	if (!c->is.dockapp && c->is.icon) {
		c->is.icon = False;
		ewmh_update_net_window_state(c);
	}
	focus(c);
	if (c->is.managed)
		arrange(v);
}

Client *
focusforw(Client *c)
{
	View *v;

	if (!c)
		return (c);
	if (!(v = c->cview))
		return NULL;
	for (c = c->next;
	     c && (!canfocus(c) || c->skip.focus || (c->is.icon || c->is.hidden)
		   || !isvisible(c, v)); c = c->next) ;
	if (!c)
		for (c = scr->clients;
		     c && (!canfocus(c) || c->skip.focus || (c->is.icon || c->is.hidden)
			   || !isvisible(c, v)); c = c->next) ;
	if (c)
		focus(c);
	return (c);
}

Client *
focusback(Client *c)
{
	View *v;

	if (!c)
		return (c);
	if (!(v = c->cview))
		return NULL;
	for (c = c->prev;
	     c && (!canfocus(c) || c->skip.focus || (c->is.icon || c->is.hidden)
		   || !isvisible(c, v)); c = c->prev) ;
	if (!c) {
		for (c = scr->clients; c && c->next; c = c->next) ;
		for (; c && (!canfocus(c) || c->skip.focus || (c->is.icon || c->is.hidden)
			     || !isvisible(c, v)); c = c->prev) ;
	}
	if (c)
		focus(c);
	return (c);
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
focusmain(Client *c)
{
	View *v;

	if (!c)
		return;
	if (!(v = c->cview))
		return;
	if (!(c = nexttiled(scr->clients, v)))
		return;
	focus(c);
}

void
focusurgent(Client *c)
{
	Client *s;
	View *v;

	if (!c)
		return;
	if (!(v = c->cview))
		return;
	for (s = scr->flist; s && (s == c || !shouldsel(s) || !s->is.attn);
	     s = s->fnext) ;
	if (s)
		focus(s);
}

Bool
with_transients(Client *c, Bool (*each) (Client *, int), int data)
{
	Client *t;
	Window *m;
	unsigned int i, g, n = 0;
	int ctx[2] = { ClientTransFor, ClientTransForGroup };
	Bool any = False;

	if (!c)
		return (any);
	for (g = 0; g < 2; g++)
		if ((m = getgroup(c->win, ctx[g], &n)))
			for (i = 0; i < n; i++)
				if ((t = getclient(m[i], ClientWindow)) && t != c)
					any |= each(t, data);
	any |= each(c, data);
	return (any);
}

static Bool
_iconify(Client *c, int dummy)
{
	if (c->is.icon)
		return False;
	c->is.icon = True;
	needarrange(clientview(c));
	ewmh_update_net_window_state(c);
	return True;
}

void
iconify(Client *c)
{
	if (!c || (!c->can.min && c->is.managed))
		return;
	if (with_transients(c, &_iconify, 0)) {
		arrangeneeded();
		focus(sel);
	}
}

void
iconifyall(View *v)
{
	Client *c;
	Bool any = False;

	for (c = scr->clients; c; c = c->next) {
		if (!isvisible(c, v))
			continue;
		if (c->is.bastard || c->is.dockapp)
			continue;
		if (!c->can.min && c->is.managed)
			continue;
		any |= with_transients(c, &_iconify, 0);
	}
	if (any) {
		arrangeneeded();
		focus(sel);
	}
}

static Bool
_deiconify(Client *c, int dummy)
{
	if (!c->is.icon)
		return False;
	c->is.icon = False;
	needarrange(clientview(c));
	ewmh_update_net_window_state(c);
	return True;
}

void
deiconify(Client *c)
{
	if (!c || (!c->can.min && c->is.managed))
		return;
	if (with_transients(c, &_deiconify, 0)) {
		arrangeneeded();
		focus(sel);
	}
}

void
deiconifyall(View *v)
{
	Client *c;
	Bool any = False;

	for (c = scr->clients; c; c = c->next) {
		if (!isvisible(c, v))
			continue;
		if (c->is.bastard || c->is.dockapp)
			continue;
		if (!c->can.min && c->is.managed)
			continue;
		any |= with_transients(c, &_deiconify, 0);
	}
	if (any) {
		arrangeneeded();
		focus(sel);
	}
}

static Bool
_hide(Client *c, int dummy)
{
	if (c->is.hidden)
		return False;
	c->is.hidden = True;
	needarrange(clientview(c));
	ewmh_update_net_window_state(c);
	return True;
}

void
hide(Client *c)
{
	if (!c || (!c->can.hide && c->is.managed))
		return;
	if (with_transients(c, &_hide, 0)) {
		arrangeneeded();
		focus(sel);
	}
}

void
hideall(View *v)
{
	Client *c;
	Bool any = False;

	for (c = scr->clients; c; c = c->next) {
		if (!isvisible(c, v))
			continue;
		if (c->is.bastard || c->is.dockapp)
			continue;
		if (!c->can.hide && c->is.managed)
			continue;
		any |= with_transients(c, &_hide, 0);
	}
	if (any) {
		arrangeneeded();
		focus(sel);
	}
}

static Bool
_show(Client *c, int dummy)
{
	if (!c->is.hidden)
		return False;
	c->is.hidden = False;
	needarrange(clientview(c));
	ewmh_update_net_window_state(c);
	return True;
}

void
show(Client *c)
{
	if (!c || (!c->can.hide && c->is.managed))
		return;
	if (with_transients(c, &_show, 0)) {
		arrangeneeded();
		focus(sel);
	}
}

void
showall(View *v)
{
	Client *c;
	Bool any = False;

	for (c = scr->clients; c; c = c->next) {
		if (!isvisible(c, v))
			continue;
		if (c->is.bastard || c->is.dockapp)
			continue;
		if (!c->can.hide && c->is.managed)
			continue;
		any |= with_transients(c, &_show, 0);
	}
	if (any) {
		arrangeneeded();
		focus(sel);
	}
}

void
togglehidden(Client *c)
{
	if (!c)
		return;
	if (c->is.hidden)
		show(c);
	else
		hide(c);
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

Client *
findclient(Window fwind)
{
	Client *c = NULL;
	Window froot = None, fparent = None, *children = NULL;
	unsigned int nchild = 0;

	do {
		if (children) {
			XFree(children);
			children = NULL;
		}
		XFindContext(dpy, froot, context[ScreenContext], (XPointer *) &scr);
		XFindContext(dpy, fwind, context[ClientAny], (XPointer *) &c);
	}
	while (!c && fwind != froot && (!froot || fparent != froot) &&
	       XQueryTree(dpy, fwind, &froot, &fparent, &children, &nchild));

	return (c);
}

long
getstate(Window w)
{
	int format, status;
	long *ret = NULL, val = -1;
	unsigned long extra, nitems = 0;
	Atom real;

	/* note this deletes property on read */
	status = XGetWindowProperty(dpy, w, _XA_WM_STATE, 0L, 1L, True, _XA_WM_STATE,
				    &real, &format, &nitems, &extra,
				    (unsigned char **) &ret);
	if (status == Success && nitems > 0)
		val = ret[0];
	if (ret)
		XFree(ret);
	return (val);
}

Window *
getcmapwins(Window w)
{
	Window *result, *cmapwins;
	unsigned long count, i;

	if ((result = getwind(w, _XA_WM_COLORMAP_WINDOWS, &count))) {
		for (i = 0; i < count && result[i] != w; i++) ;
		if (i < count) {
			if ((cmapwins = calloc(count + 2, sizeof(*cmapwins)))) {
				cmapwins[0] = w;
				for (i = 0; i < count; i++)
					cmapwins[i + 1] = result[i];
			}
		} else {
			if ((cmapwins = calloc(count + 1, sizeof(*cmapwins)))) {
				for (i = 0; i < count; i++)
					cmapwins[i] = result[i];
			}
		}
		XFree(result);
	} else {
		if ((cmapwins = calloc(2, sizeof(*cmapwins))))
			cmapwins[0] = w;
	}
	return (cmapwins);
}

const char *
getresource(const char *resource, const char *defval)
{
	static char name[256], clas[256], *type = NULL;
	XrmValue value = { 0, NULL };

	snprintf(name, sizeof(name), "%s.%s", RESNAME, resource);
	snprintf(clas, sizeof(clas), "%s.%s", RESCLASS, resource);
	if (XrmGetResource(xrdb, name, clas, &type, &value) && value.addr)
		return value.addr;
	return defval;
}

const char *
getscreenres(const char *resource, const char *defval)
{
	static char name[256], clas[256], *type = NULL;
	XrmValue value = { 0, NULL };

	snprintf(name, sizeof(name), "%s.screen%d.%s", RESNAME, scr->screen, resource);
	snprintf(clas, sizeof(clas), "%s.Screen%d.%s", RESCLASS, scr->screen, resource);
	if (XrmGetResource(xrdb, name, clas, &type, &value) && value.addr)
		return value.addr;
	return defval;
}

const char *
getsessionres(const char *resource, const char *defval)
{
	static char name[256], clas[256], *type = NULL;
	XrmValue value = { 0, NULL };

	snprintf(name, sizeof(name), "%s.session.%s", RESNAME, resource);
	snprintf(clas, sizeof(clas), "%s.Session.%s", RESCLASS, resource);
	if (XrmGetResource(xrdb, name, clas, &type, &value) && value.addr)
		return value.addr;
	return defval;
}

Bool
gettextprop(Window w, Atom atom, char **text)
{
	char **list = NULL, *str;
	int n = 0;
	XTextProperty name = { NULL, };

	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return False;
	if (name.encoding == XA_STRING) {
		if ((str = strndup((char *) name.value, name.nitems))) {
			free(*text);
			*text = str;
		}
	} else {
		if (Xutf8TextPropertyToTextList(dpy, &name, &list, &n) >= Success
		    && n > 0 && list && *list) {
			if ((str = strndup(*list, name.nitems))) {
				free(*text);
				*text = str;
			}
			if (list)
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
isvisible(Client *c, View *v)
{
	if (!c)
		return False;
	if (c->is.sticky)
		return True;
	if (!v) {
		unsigned i;

		for (v = scr->views, i = 0; i < scr->ntags; i++, v++)
			if (c->tags & v->seltags)
				return True;
	} else {
		if (c->tags & v->seltags)
			return True;
	}
	return False;
}

void
grabkeys(void)
{
	unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask | LockMask };
	unsigned int j;
	KeyCode code;
	Key *k;

	XUngrabKey(dpy, AnyKey, AnyModifier, scr->root);
	for (k = scr->keylist; k; k = k->cnext) {
		if ((code = XKeysymToKeycode(dpy, k->keysym))) {
			for (j = 0; j < LENGTH(modifiers); j++)
				XGrabKey(dpy, code, k->mod | modifiers[j],
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
			if (!k) {
				for (k = scr->keylist; k; k = k->cnext)
					if (k->keysym == keysym && k->mod == mod)
						break;
			}
			if (k) {
				DPRINTF("KeyPress: activating action for chain: %s\n", showchain(k));
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
	XUngrabKeyboard(dpy, user_time);
	return handled;
}

void
killxclient(Client *c)
{
	XKillClient(dpy, c->win);
}

void
killproc(Client *c)
{
	pid_t pid = 0;
	char *machine = NULL;

	/* NOTE: Before killing the client we should attempt to kill the process using
	   the _NET_WM_PID and WM_CLIENT_MACHINE because XKillClient might still leave
	   the process hanging. Try using SIGTERM first, following up with SIGKILL */

	if ((pid = getnetpid(c)) && (machine = getclientmachine(c))) {
		char hostname[65] = { 0, };

		gethostname(hostname, 64);
		if (!strcmp(hostname, machine)) {
			kill(pid, c->is.killing ? SIGKILL : SIGTERM);
			c->is.killing = 1;
			free(machine);
			return;
		}
		free(machine);
	}
	killxclient(c);
}

void
killclient(Client *c)
{
	if (!c)
		return;
	if (!c->is.killing) {
		Window win;

		if (((win = c->icon) && checkatom(win, _XA_WM_PROTOCOLS, _XA_NET_WM_PING)) ||
	            ((win = c->win)  && checkatom(win, _XA_WM_PROTOCOLS, _XA_NET_WM_PING))) {
			if (!c->is.pinging) {
				XEvent ev;

				/* Give me a ping: one ping only.... Red October */
				ev.type = ClientMessage;
				ev.xclient.display = dpy;
				ev.xclient.window = win;
				ev.xclient.message_type = _XA_WM_PROTOCOLS;
				ev.xclient.format = 32;
				ev.xclient.data.l[0] = _XA_NET_WM_PING;
				ev.xclient.data.l[1] = user_time;
				ev.xclient.data.l[2] = c->win;
				ev.xclient.data.l[3] = 0;
				ev.xclient.data.l[4] = 0;
				XSendEvent(dpy, c->win, False, NoEventMask, &ev);
				c->is.pinging = 1;
			}
		}
		if (((win = c->icon) && checkatom(win, _XA_WM_PROTOCOLS, _XA_WM_DELETE_WINDOW)) ||
		    ((win = c->win)  && checkatom(win, _XA_WM_PROTOCOLS, _XA_WM_DELETE_WINDOW))) {
			if (!c->is.closing) {
				XEvent ev;

				ev.type = ClientMessage;
				ev.xclient.window = win;
				ev.xclient.message_type = _XA_WM_PROTOCOLS;
				ev.xclient.format = 32;
				ev.xclient.data.l[0] = _XA_WM_DELETE_WINDOW;
				ev.xclient.data.l[1] = user_time;
				ev.xclient.data.l[2] = 0;
				ev.xclient.data.l[3] = 0;
				ev.xclient.data.l[4] = 0;
				XSendEvent(dpy, c->win, False, NoEventMask, &ev);
				c->is.closing = 1;
				return;
			}
		}
	}
	killproc(c);
}

static Bool
leavenotify(XEvent *e)
{
	XCrossingEvent *ev = &e->xcrossing;
	Client *c;

	if (e->type != LeaveNotify || ev->mode != NotifyNormal || ev->detail == NotifyInferior)
		return False;

	if (!ev->same_screen) {
		XFindContext(dpy, ev->window, context[ScreenContext], (XPointer *) &scr);
		if (!scr->managed)
			focus(NULL); /* XXX */
	}
	if ((c = getclient(ev->window, ClientTitle)) && ev->window == c->title) {
		Bool needdraw = False;
		int i;

		for (i = 0; i < LastElement; i++) {
			ElementClient *ec = &c->element[i];

			if (!ec->present)
				continue;
			if (ec->hovered) {
				ec->hovered = False;
				needdraw = True;
			}
		}
		if (needdraw)
			drawclient(c);
	}
	return True;
}

void
show_client_state(Client *c)
{
	CPRINTF(c, "%-20s: 0x%08x\n", "wintype", c->wintype);
#if 1
	CPRINTF(c, "%-20s: 0x%08x\n", "skip.skip", c->skip.skip);
#else
	CPRINTF(c, "%-20s: %s\n", "skip.taskbar", c->skip.taskbar ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "skip.pager", c->skip.pager ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "skip.winlist", c->skip.winlist ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "skip.cycle", c->skip.cycle ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "skip.focus", c->skip.focus ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "skip.arrange", c->skip.arrange ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "skip.sloppy", c->skip.sloppy ? "true" : "false");
#endif
#if 1
	CPRINTF(c, "%-20s: 0x%08x\n", "is.is", c->is.is);
#else
	CPRINTF(c, "%-20s: %s\n", "is.transient", c->is.transient ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.grptrans", c->is.grptrans ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.banned", c->is.banned ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.floater", c->is.floater ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.max", c->is.max ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.maxv", c->is.maxv ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.maxh", c->is.maxh ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.lhalf", c->is.lhalf ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.rhalf", c->is.rhalf ? "true" : "false");
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
	CPRINTF(c, "%-20s: %s\n", "is.selected", c->is.selected ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.dockapp", c->is.dockapp ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "is.managed", c->is.managed ? "true" : "false");
#endif
#if 1
	CPRINTF(c, "%-20s: 0x%08x\n", "has.has", c->has.has);
#else
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
	CPRINTF(c, "%-20s: %s\n", "has.but.half", c->has.but.half ? "true" : "false");
#endif
#if 1
	CPRINTF(c, "%-20s: 0x%08x\n", "needs.has", c->needs.has);
#else
	CPRINTF(c, "%-20s: %s\n", "needs.border", c->needs.border ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "needs.grips", c->needs.grips ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "needs.title", c->needs.title ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "needs.but.menu", c->needs.but.menu ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "needs.but.min", c->needs.but.min ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "needs.but.max", c->needs.but.max ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "needs.but.close", c->needs.but.close ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "needs.but.size", c->needs.but.size ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "needs.but.shade", c->needs.but.shade ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "needs.but.fill", c->needs.but.fill ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "needs.but.floats", c->needs.but.floats ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "needs.but.half", c->needs.but.half ? "true" : "false");
#endif
#if 0
	CPRINTF(c, "%-20s: %s\n", "with.struts", c->with.struts ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "with.time", c->with.time ? "true" : "false");
#endif
#if 1
	CPRINTF(c, "%-20s: 0x%08x\n", "can.can", c->can.can);
#else
	CPRINTF(c, "%-20s: %s\n", "can.move", c->can.move ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.size", c->can.size ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.sizev", c->can.sizev ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.sizeh", c->can.sizeh ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.min", c->can.min ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.max", c->can.max ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.maxv", c->can.maxv ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.maxh", c->can.maxh ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.lhalf", c->can.lhalf ? "true" : "false");
	CPRINTF(c, "%-20s: %s\n", "can.rhalf", c->can.rhalf ? "true" : "false");
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
#endif
}

static void
checkshaped(Client *c)
{
#if SHAPE
	XWindowAttributes wa = { 0, };
	Bool boundary = False;
	Bool clipping = False;
	Extent be = { 0, };
	Extent ce = { 0, };

	if (c->is.dockapp)
		return;
	if (einfo[XshapeBase].have) {
		XShapeSelectInput(dpy, c->win, ShapeNotifyMask);
		XGetWindowAttributes(dpy, c->win, &wa);
		XShapeQueryExtents(dpy, c->win,
				   &boundary, &be.x, &be.y, &be.w, &be.h,
				   &clipping, &ce.x, &ce.y, &ce.w, &ce.h);

		c->with.boundary = boundary;
		c->with.clipping = clipping;

		if ((boundary) &&
		    ((be.x > -wa.border_width) || (be.y > -wa.border_width) ||
		     (be.x + be.w < wa.width + wa.border_width) ||
		     (be.y + be.h < wa.height + wa.border_width))) {
			/* boundary extents are partially or fully contained within
			   window */
			c->with.wshape = True;
		}
		if ((clipping) &&
		    ((ce.x > be.x) && (ce.y > be.y) &&
		     (ce.x + ce.w < be.x + be.w) && (ce.y + ce.h < be.y + be.h))) {
			/* clipping extents are fully contained within boundary */
			c->with.bshape = True;
		}
	}
#endif
}

static void
updateshape(Client *c)
{
	if (c->is.dockapp)
		return;
	checkshaped(c);
	if (c->with.wshape || c->with.bshape) {
		/* When we have a fully shaped window or fully shaped border, it is
		 * counterproductive to have a frame.  But rather than not adding decorations, just
		 * set the default of the window to be undecorated by default. */
		c->has.title = False;
		c->has.grips = False;
		c->has.border = False;
		c->is.undec = True;
		c->skip.skip = -1U;
	}
}

Bool latertime(Time time);

/*
 * Conventions for icon windows: (from ICCCM 2.0)
 *
 *  1. The icon window should be an InputOutput child of the root.
 *  2. The icon window should be one of the sizes specified in the
 *     WM_ICON_SIZE property on the root.
 *  3. The icon window should use the root visual and default colormap
 *     for the screen in question.
 *  4. Clients should not map their icon windows.
 *  5. Clients should not unmap their icon windows.
 *  6. Clients should not configure their icon windows.
 *  7. Clients should not set override-redirect on their icon windows
 *     or select for ResizeRedirect events on them.
 *  8. Clients must not depend on being able to receive input events by
 *     means of their icon windows.
 *  9. Clients must not manipulate the borders of their icon windows.
 * 10. Clients must select for Exposure events on their icon window and
 *     repaint it when requested.
 */

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc = { 0, };
	XSetWindowAttributes twa = { 0, };
	unsigned long mask = 0;
	Bool focusnew = True;
	int depth;
	Visual *visual;

	if ((c = getclient(w, ClientAny))) {
		_CPRINTF(c, "client already managed!\n");
		return;
	}
	DPRINTF("managing window 0x%lx\n", w);
	c = emallocz(sizeof(Client));
	c->win = w;
	c->name = ecalloc(1, 1);
	c->icon_name = ecalloc(1, 1);
	XSaveContext(dpy, c->win, context[ClientWindow], (XPointer) c);
	XSaveContext(dpy, c->win, context[ClientAny], (XPointer) c);
	XSaveContext(dpy, c->win, context[ScreenContext], (XPointer) scr);
	c->has.has = -1U;
	c->needs.has = -1U;
	c->can.can = -1U;
	applystate(c);
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
			XSaveContext(dpy, c->icon, context[ScreenContext], (XPointer) scr);
		}
	}
	// c->tags = 0;
	c->r.b = c->c.b = c->is.bastard ? 0 : scr->style.border;
	/* XXX: had.border? */

	c->u.x = wa->x;
	c->u.y = wa->y;
	c->u.w = wa->width;
	c->u.h = wa->height;
	c->u.b = wa->border_width;

	updatehints(c);
	updatesizehints(c);
	updatetitle(c);
	updateiconname(c);
	applyrules(c);
	applyatoms(c);

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

				if ((group = getgroup(c->leader, ClientGroup, &m))) {
					for (i = 0; i < m; i++) {
						trans = group[i];
						if ((t = getclient(trans, ClientWindow)))
							break;
					}
				}
			} else if (t) {
				/* Transients are not allowed to perform some actions when window
				   for which they are are transient for is also managed.  ICCCM 2.0
				   says such applications should act on the non-transient managed
				   client and let the window manager decide what to do about the
				   transients. */
				c->can.min  = False;	/* can't (de)iconify */
				c->can.hide = False;	/* can't hide or show */
				c->can.tag  = False;	/* can't change desktops */
			}
			if (t)
				c->tags = t->tags;
		}
		c->is.transient = True;
		c->is.floater = True;
	} else
		c->is.transient = False;

	updatesession(c);

	ewmh_process_net_window_user_time_window(c);
	ewmh_process_net_startup_id(c);

	if ((c->with.time) && latertime(c->user_time))
		focusnew = False;

	if (c->is.attn)
		focusnew = True;
	if (!canfocus(c))
		focusnew = False;

	if (!c->is.floater)
		c->is.floater = (!c->can.sizeh || !c->can.sizev);

	if (c->has.title)
		c->c.t = c->r.t = scr->style.titleheight;
	else {
		c->can.shade = False;
		c->has.grips = False;
	}
	if (c->has.grips) {
		if (!(c->c.g = c->r.g = scr->style.gripsheight))
			c->has.grips = False;
		else if (scr->style.fullgrips)
			c->c.v = c->r.v = c->c.g;
	}
	c->c.x = c->r.x = c->u.x;
	c->c.y = c->r.y = c->u.y;
	c->c.w = c->r.w = c->u.w + 2 * c->c.v;;
	c->c.h = c->r.h = c->u.h + c->c.t + c->c.g + c->c.v;

	c->s.x = wa->x;
	c->s.y = wa->y;
	c->s.w = wa->width;
	c->s.h = wa->height;
	c->s.b = wa->border_width;

	if (c->icon) {
		c->r.x = c->c.x = 0;
		c->r.y = c->c.y = 0;
		if (c->icon != c->win && XGetWindowAttributes(dpy, c->icon, wa)) {
			c->c.x = c->r.x = wa->x;
			c->c.y = c->r.y = wa->y;
			c->c.w = c->r.w = wa->width;
			c->c.h = c->r.h = wa->height;
			c->r.b = wa->border_width;
		}
	}

	c->with.struts = getstruts(c);

	/* try not grabbing Button4 and Button5 */
	XGrabButton(dpy, Button1, AnyModifier, c->win, True,
		    ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
	XGrabButton(dpy, Button2, AnyModifier, c->win, True,
		    ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
	XGrabButton(dpy, Button3, AnyModifier, c->win, True,
		    ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
	if (c->icon && c->icon != c->win) {
		XGrabButton(dpy, Button1, AnyModifier, c->icon, True,
			    ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
		XGrabButton(dpy, Button2, AnyModifier, c->icon, True,
			    ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
		XGrabButton(dpy, Button3, AnyModifier, c->icon, True,
			    ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
	}
	twa.override_redirect = True;
	mask |= CWOverrideRedirect;
	twa.event_mask = FRAMEMASK;
	mask |= CWEventMask;
	if (c->is.dockapp) {
		/* dockapp frames (tiles) must be created with default visual */
		/* some set ParentRelative and icon windows must be default visual */
		twa.event_mask |= ExposureMask | MOUSEMASK;
		depth = DefaultDepth(dpy, scr->screen);
		visual = DefaultVisual(dpy, scr->screen);
		twa.colormap = DefaultColormap(dpy, scr->screen);
		mask |= CWColormap;
		twa.background_pixel = scr->style.color.norm[ColBG];
		mask |= CWBackPixel;
	} else {
		/* all other frames, titles, grips can be created with 32 ARGB visual */
		depth = scr->depth;
		visual = scr->visual;
		twa.colormap = scr->colormap;
		mask |= CWColormap;
		twa.border_pixel = BlackPixel(dpy, scr->screen);
		mask |= CWBorderPixel;
		twa.background_pixmap = None;
		mask |= CWBackPixmap;
	}
	updatecmapwins(c);
	c->frame = XCreateWindow(dpy, scr->root, c->c.x, c->c.y, c->c.w, c->c.h,
				 c->c.b, depth, InputOutput, visual, mask, &twa);
	XSaveContext(dpy, c->frame, context[ClientFrame], (XPointer) c);
	XSaveContext(dpy, c->frame, context[ClientAny], (XPointer) c);
	XSaveContext(dpy, c->frame, context[ScreenContext], (XPointer) scr);

	wc.border_width = c->c.b;
	XConfigureWindow(dpy, c->frame, CWBorderWidth, &wc);
	send_configurenotify(c, None);
	XSetWindowBorder(dpy, c->frame, scr->style.color.norm[ColBorder]);

	twa.event_mask = ExposureMask | MOUSEMASK | WINDOWMASK;
	/* we create title as root's child as a workaround for 32bit visuals */
	if (c->needs.title) {
		c->element = ecalloc(LastElement, sizeof(*c->element));
		c->title = XCreateWindow(dpy, scr->root, 0, 0, c->c.w, scr->style.titleheight,
					 0, depth, CopyFromParent, visual, mask, &twa);
		XSaveContext(dpy, c->title, context[ClientTitle], (XPointer) c);
		XSaveContext(dpy, c->title, context[ClientAny], (XPointer) c);
		XSaveContext(dpy, c->title, context[ScreenContext], (XPointer) scr);
	}
	if (c->needs.grips) {
		c->grips = XCreateWindow(dpy, scr->root, 0, 0, c->c.w, scr->style.gripsheight,
					 0, depth, CopyFromParent, visual, mask, &twa);
		XSaveContext(dpy, c->grips, context[ClientGrips], (XPointer) c);
		XSaveContext(dpy, c->grips, context[ClientAny], (XPointer) c);
		XSaveContext(dpy, c->grips, context[ScreenContext], (XPointer) scr);

	}

	addclient(c, False, False, True);

	wc.border_width = 0;
	mask = 0;
	twa.event_mask = CLIENTMASK;
	mask |= CWEventMask;
	twa.do_not_propagate_mask = CLIENTNOPROPAGATEMASK;
	mask |= CWDontPropagate;
	/* in case of stupid client */
	twa.win_gravity = NorthWestGravity;
	mask |= CWWinGravity;
	// twa.save_under = False;
	// mask |= CWSaveUnder;

	if (c->icon) {
		if (einfo[XfixesBase].have)
			XFixesChangeSaveSet(dpy, c->icon, SetModeInsert, SaveSetNearest, SaveSetUnmap);
		else
			XChangeSaveSet(dpy, c->icon, SetModeInsert);
		twa.backing_store = Always;
		mask |= CWBackingStore;
		XChangeWindowAttributes(dpy, c->icon, mask, &twa);
		XSelectInput(dpy, c->icon, CLIENTMASK);
		updateshape(c); /* do not shape frames for dock apps */
		XReparentWindow(dpy, c->icon, c->frame, c->r.x, c->r.y);
		XConfigureWindow(dpy, c->icon, CWBorderWidth, &wc);
		XMapWindow(dpy, c->icon);
#if 0
		/* not necessary and doesn't help */
		if (c->win && c->win != c->icon) {
			XWindowChanges cwc;

			/* map primary window offscreen */
			cwc.x = DisplayWidth(dpy, scr->screen) + 10;
			cwc.y = DisplayHeight(dpy, scr->screen) + 10;
			XConfigureWindow(dpy, c->win, CWX | CWY, &cwc);
			XMapWindow(dpy, c->win);
		}
#endif
	} else {
		if (einfo[XfixesBase].have)
			XFixesChangeSaveSet(dpy, c->win, SetModeInsert, SaveSetNearest, SaveSetMap);
		else
			XChangeSaveSet(dpy, c->win, SetModeInsert);
		// twa.backing_store = NotUseful;
		// mask |= CWBackingStore;
		XChangeWindowAttributes(dpy, c->win, mask, &twa);
		XSelectInput(dpy, c->win, CLIENTMASK);
		updateshape(c);
		XReparentWindow(dpy, c->win, c->frame, 0, c->c.t);
		if (c->grips)
			XReparentWindow(dpy, c->grips, c->frame, 0, c->c.h - c->c.g);
		if (c->title)
			XReparentWindow(dpy, c->title, c->frame, 0, 0);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
		XMapWindow(dpy, c->win);
	}

	ban(c);

	ewmh_process_net_window_desktop(c);
	ewmh_process_net_window_desktop_mask(c);
	ewmh_process_net_window_sync_request_counter(c);
	ewmh_process_net_window_state(c);
	ewmh_process_net_window_opacity(c);
	c->is.managed = True;
	setwmstate(c->win, c->winstate, c->is.dockapp ? (c->icon ? : c->win) : None);
	ewmh_update_net_window_state(c);
	ewmh_update_net_window_desktop(c);
	ewmh_update_ob_app_props(c);

	if (c->grips && c->c.g) {
		XRectangle r = { 0, c->c.h - c->c.g, c->c.w, c->c.g };

		XMoveResizeWindow(dpy, c->grips, r.x, r.y, r.width, r.height);
		XMapWindow(dpy, c->grips);
	}
	if (c->title && c->c.t) {
		XRectangle r = { 0, 0, c->c.w, c->c.t };

		XMoveResizeWindow(dpy, c->title, r.x, r.y, r.width, r.height);
		XMapWindow(dpy, c->title);
	}
	if (!c->is.dockapp)
		configureshapes(c);
	if ((c->grips && c->c.g) || (c->title && c->c.t))
		drawclient(c);

	if (c->with.struts) {
		ewmh_update_net_work_area();
		updategeom(NULL);
	}
	XSync(dpy, False);
	show_client_state(c);
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
	XWindowAttributes wa = { 0, };
	Client *c;
	XMapRequestEvent *ev = &e->xmaprequest;

	if (!XGetWindowAttributes(dpy, ev->window, &wa))
		return True;
	if (wa.override_redirect) {
		manageoverride(ev->window, &wa);
		return True;
	}
	if (issystray(ev->window))
		return True;
	if (!(c = getclient(ev->window, ClientWindow))) {
		manage(ev->window, &wa);
		return True;
	}
	return False;
}

void
getpointer(int *x, int *y)
{
	int di1, di2;
	unsigned int dui;
	Window dummy1, dummy2;

	XQueryPointer(dpy, scr->root, &dummy1, &dummy2, x, y, &di1, &di2, &dui);
}

static Monitor *
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

View *getview(int x, int y)
{
	Monitor *m = getmonitor(x, y);

	return m ? m->curview : NULL;
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

static Monitor *
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

static Monitor *
findmonitor(Client *c)
{
	int xmin, xmax, ymin, ymax;

	xmin = c->r.x;
	xmax = c->r.x + c->r.w + 2 * c->r.b;
	ymin = c->r.y;
	ymax = c->r.y + c->r.h + 2 * c->r.b;

	return bestmonitor(xmin, ymin, xmax, ymax);
}

View *
clientview(Client *c)
{
	Monitor *m;

	for (m = scr->monitors; m; m = m->next)
		if (isvisible(c, m->curview))
			return m->curview;
	return NULL;
}

View *
onview(Client *c)
{
	View *v;
	unsigned i;

	for (i = 0, v = scr->views; i < scr->ntags; i++, v++)
		if (isvisible(c, v))
			return v;
	return NULL;
}

static Monitor *
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

View *
closestview(int x, int y)
{
	Monitor *m = closestmonitor(x, y);

	return m ? m->curview : NULL;
}

static Monitor *
nearmonitor()
{
	Monitor *m;
	int x, y;

	getpointer(&x, &y);
	if (!(m = getmonitor(x, y)))
		m = closestmonitor(x, y);
	return m;
}

View *
nearview()
{
	return nearmonitor()->curview;
}

View *
selview()
{
	if (sel && sel->cview)
		return sel->cview;
	return nearview();
}

#ifdef SYNC

void
sync_request(Client *c, Time time)
{
	int overflow = 0;
	XEvent ce;
	XSyncValue inc;
	XSyncAlarmAttributes aa = { { 0, }, };

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
		     c->c.w - 2 * c->c.v, c->c.h - c->c.t - c->c.g - c->c.v, w, h, c->frame, c->win, c->name);
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
	XWindowChanges wc = { 0, };
	unsigned mask = 0;

	if (!(c = getclient(ae->alarm, ClientAny))) {
		XPRINTF("Recevied alarm notify for unknown alarm 0x%08lx\n", ae->alarm);
		return False;
	}
	CPRINTF(c, "alarm notify on 0x%08lx\n", ae->alarm);
	if (!c->sync.waiting) {
		DPRINTF("%s", "Alarm was cancelled!\n");
		return True;
	}
	c->sync.waiting = False;

	if ((wc.width = c->c.w - 2 * c->c.v) != c->sync.w) {
		XPRINTF("Width changed from %d to %u since last request\n", c->sync.w,
			wc.width);
		mask |= CWWidth;
	}
	if ((wc.height = c->c.h - c->c.t - c->c.g - c->c.v) != c->sync.h) {
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

static Bool
reparentnotify(XEvent *e)
{
	Client *c;
	XReparentEvent *ev = &e->xreparent;

	if ((c = getclient(ev->window, ClientWindow))) {
		if (ev->parent != c->frame) {
			_CPRINTF(c, "unmanage reparented window\n");
			unmanage(c, CauseReparented);
		}
		return True;
	}
	return False;
}

/*
 * Client leader properties
 */
static Bool
updatesessionprop(Client *c, Atom prop, int state)
{
	char *name = NULL;

	if (prop <= XA_LAST_PREDEFINED) {
		switch (prop) {
		case XA_WM_NAME:
			/* Client leaders are never managed and should not have this
			   property. */
			goto bad;
		case XA_WM_ICON_NAME:
			/* Client leaders are never managed and should not have this
			   property. */
			goto bad;
		case XA_WM_NORMAL_HINTS:
			/* Client leaders are never managed and should not have this
			   property. */
			goto bad;
		case XA_WM_HINTS:
			/* Client leaders are never managed and should not have this
			   property. */
			goto bad;
		case XA_WM_CLASS:
			/* Client leaders are never managed and should not have this
			   property.  We get this property on demand anyhoo. */
			ewmh_process_net_startup_id(c);
			goto bad;
		case XA_WM_TRANSIENT_FOR:
			/* Client leaders cannot be transient. */
			goto bad;
		case XA_WM_COMMAND:
			/* TODO: Can be set on a client leader window and changed in
			   response to a WM_SAVE_YOURSELF client message. */
			ewmh_process_net_startup_id(c);
			goto bad;
		case XA_WM_CLIENT_MACHINE:
			/* Typically set on client leader window. We use it in
			   conjunction with _NET_WM_PID for startup sequence
			   identification. As with all client leader properties, it
			   should not be changed while a client is managed; however, some
			   clients or launchers might change this too late, so we will
			   still update what relies on it. */
			ewmh_process_net_startup_id(c);
			return True;
		case XA_WM_ICON_SIZE:
			/* Placed on the root only and by the window manager. */
			goto bad;
		case XA_WM_SIZE_HINTS:
			/* Obsolete property. */
			goto bad;
		case XA_WM_ZOOM_HINTS:
			/* Obsolete property. */
			goto bad;
		default:
			return False;
		}
	} else {
		if (0) {
		} else if (prop == _XA_SM_CLIENT_ID) {
			/* Note: the SM_CLIENT_ID property cannot be updated when clients
			   in the session are managed: so we do not need to update this.
			   It should never be updated on a managed client anyhoo. */
			goto bad;
		} else if (prop == _XA_WM_CLIENT_LEADER) {
			/* Note: the WM_CLIENT_LEADER property cannot be updated when
			   clients in the session are managed: so we do not need to
			   update this.  It should never be updated on a managed client
			   anyhoo. */
			goto bad;
		} else if (prop == _XA_WM_WINDOW_ROLE) {
			/* Because client leaders are never managed, they cannot have a
			   window role property, and the window role property must be
			   unique per window anyhoo. */
			goto bad;
		} else if (prop == _XA_WM_PROTOCOLS) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_WM_STATE) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_WM_COLORMAP_WINDOWS) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_WIN_LAYER) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_WIN_STATE) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_WIN_HINTS) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_WIN_APP_STATE) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_WIN_EXPANDED_SIZE) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_WIN_ICONS) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_WIN_WORKSPACE) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_NAME) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_VISIBLE_NAME) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_ICON_NAME) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_VISIBLE_ICON_NAME) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_DESKTOP) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_WINDOW_TYPE) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_STATE) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_ALLOWED_ACTIONS) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_STRUT) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_STRUT_PARTIAL) {
			/* Client leaders are never managed. */
			goto bad;
#if 0
		} else if (prop == _XA_NET_WM_ICON_GEOMETRY) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_ICON) {
			/* Client leaders are never managed. */
			goto bad;
#endif
		} else if (prop == _XA_NET_WM_PID) {
			/* Could be set on a client leader window (so that it will apply
			   to all members of the session).  This property should not
			   change after a client in the session is managed; however, some
			   launchers might set it too late. We use it with
			   WM_CLIENT_MACHINE to identify startup notification sequences
			   and must reinvoke the check here when that happens. */
			ewmh_process_net_startup_id(c);
		} else if (prop == _XA_NET_WM_HANDLED_ICONS) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_USER_TIME) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_USER_TIME_WINDOW) {
			/* Client leaders are never managed. */
			goto bad;
#if 0
		} else if (prop == _XA_NET_WM_OPAQUE_REGION) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_BYPASS_COMPOSITOR) {
			/* Client leaders are never managed. */
			goto bad;
#endif
		} else if (prop == _XA_NET_WM_WINDOW_OPACITY) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_WM_SYNC_REQUEST_COUNTER) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_NET_STARTUP_ID) {
			/* Typically set on client leader window.  As with all client
			   leader properties, this should not be changed while a session
			   member is managed; however, some clients (e.g. roxterm,
			   launchers) are setting this property too late, soe we must
			   recheck startup notification when that happens. */
			ewmh_process_net_startup_id(c);
		} else
			return False;
	}
	return True;
      bad:
	_CPRINTF(c, "bad attempt to change client leader %s\n", (name = XGetAtomName(dpy, prop)));
	if (name)
		XFree(name);
	return False;
}

/*
 * Group leader properties are properties that apply to the group as a whole.
 * This includes just about every property that can be set on a specific client.
 */
static Bool
updateleaderprop(Client *c, Atom prop, int state)
{
	char *name = NULL;

	if (prop <= XA_LAST_PREDEFINED) {
		switch (prop) {
		case XA_WM_NAME:
			updatetitle(c);
			drawclient(c);
			break;
		case XA_WM_ICON_NAME:
			updateiconname(c);
			break;
		case XA_WM_NORMAL_HINTS:
			/* size hints cannot apply to a group */
			goto bad;
		case XA_WM_HINTS:
			/* hints cannot apply to a group (the group itself is in the
			   hint) */
			goto bad;
		case XA_WM_CLASS:
			/* Class should not be on a group basis (unless WM_WINDOW_ROLE is
			   provided). We check this property on demand, so we can ignore
			   updates. */
			ewmh_process_net_startup_id(c);
			break;
		case XA_WM_TRANSIENT_FOR:
			/* Note that transient windows need not have a WM_CLIENT_LEADER
			   property when the window they are transient for has one.  This
			   property should not be changed after a window is managed.  */
			updatetransientfor(c);
			goto bad;
		case XA_WM_COMMAND:
			/* TODO: Typically not set on group window (but on client leader
			   window). we don't do anything with it yet. */
			ewmh_process_net_startup_id(c);
			break;
		case XA_WM_CLIENT_MACHINE:
			/* Typically not set on group window (but on client leader
			   window).  We use it in conjunction with _NET_WM_PID for
			   startup sequence identification.  Nevertheless it should not
			   be updated on a client leader once a window in the session is
			   managed.  Some clients might change is too late, so we will
			   still update what relies on it (TODO). */
			ewmh_process_net_startup_id(c);
			break;
		case XA_WM_ICON_SIZE:
			/* Placed on the root only and by the window manager. */
			goto bad;
		case XA_WM_SIZE_HINTS:
			/* Obsolete property. */
			goto bad;
		case XA_WM_ZOOM_HINTS:
			/* Obsolete property. */
			goto bad;
		default:
			return False;
		}
	} else {
		if (0) {
		} else if (prop == _XA_SM_CLIENT_ID) {
			/* Note: the SM_CLIENT_ID property cannot be updated when clients
			   in the session are managed: so we do not need to update this.
			   It should never be updated on a managed client anyhoo. */
			goto bad;
		} else if (prop == _XA_WM_CLIENT_LEADER) {
			/* Note: the WM_CLIENT_LEADER property cannot be updated when
			   clients in the session are managed: so we do not need to
			   update this.  It should never be updated on a managed client
			   anyhoo. */
			goto bad;
		} else if (prop == _XA_WM_WINDOW_ROLE) {
			/* Note: if there is no WM_WINDOW_ROLE property, try to use
			   WM_CLASS and WM_NAME to uniquely identify the window.  This
			   should only be set on manageable client windows and is meant
			   for the window manager to identify the window in conjunction
			   with the SM_CLIENT_ID. */
			goto bad;
		} else if (prop == _XA_WM_PROTOCOLS) {
			/* We tend to test these on a demand basis. */
		} else if (prop == _XA_WM_STATE) {
			/* Should only be set by the window manager after the window is
			   managed.  We could potentially set it back... */
			goto bad;
		} else if (prop == _XA_WM_COLORMAP_WINDOWS) {
			goto bad;
		} else if (prop == _XA_WIN_LAYER) {
			/* Should only be set by the window manager after the window is
			   managed.  Clients should use client messages instead.  Also,
			   if we should change it for a group window, we might change it
			   for the members of the group too (TODO). */
			goto bad;
		} else if (prop == _XA_WIN_STATE) {
			/* Should only be set by the window manager after the window is
			   managed.  Clients should use client messages instead.  Does
			   not apply on a group basis. */
			goto bad;
		} else if (prop == _XA_WIN_HINTS) {
			/* Should not be set on a group basis after a window in the group
			   is managed. */
			goto bad;
		} else if (prop == _XA_WIN_APP_STATE) {
			/* XXX: unfortunately, wm-comp.txt does not explain this
			   property. */
			goto bad;
		} else if (prop == _XA_WIN_EXPANDED_SIZE) {
			/* XXX: unfortunately, wm-comp.txt does not explain this
			   property. */
			goto bad;
		} else if (prop == _XA_WIN_ICONS) {
			/* XXX: unfortunately, wm-comp.txt does not explain this
			   property. */
			goto bad;
		} else if (prop == _XA_WIN_WORKSPACE) {
			/* Should only be set by the window manager after the windows is
			   managed.  Clients should use client messages instead.  Also,
			   if we should change it for a group window, we might change it
			   for the members of the group too (TODO). */
			goto bad;
		} else if (prop == _XA_NET_WM_NAME) {
			updatetitle(c);
			drawclient(c);
		} else if (prop == _XA_NET_WM_VISIBLE_NAME) {
			/* Should only ever be set by the window manager. */
			goto bad;
		} else if (prop == _XA_NET_WM_ICON_NAME) {
			updateiconname(c);
		} else if (prop == _XA_NET_WM_VISIBLE_ICON_NAME) {
			/* Should only ever be set by the window manager. */
			goto bad;
		} else if (prop == _XA_NET_WM_DESKTOP) {
			/* Should only be set by the window manager after the window is
			   managed.  Clients should use client messages instead. */
			goto bad;
		} else if (prop == _XA_NET_WM_WINDOW_TYPE) {
			/* Should only be set by the window manager (but not necessary)
			   after the window is managed.  Clients cannot change it. */
			goto bad;
		} else if (prop == _XA_NET_WM_STATE) {
			/* Should only be set by the window manager after the window is
			   managed.  Clients should use client messages instead.  Does
			   not apply to groups. */
			goto bad;
		} else if (prop == _XA_NET_WM_ALLOWED_ACTIONS) {
			/* Should only be set by the window manager.  Clients can request
			   changes using client messages.  Does not apply to groups. */
			goto bad;
		} else if (prop == _XA_NET_WM_STRUT) {
			/* Can be set by the client, but does not apply to groups. */
			goto bad;
		} else if (prop == _XA_NET_WM_STRUT_PARTIAL) {
			/* Can be set by the client, but does not apply to groups. */
			goto bad;
#if 0
		} else if (prop == _XA_NET_WM_ICON_GEOMETRY) {
			/* Normally set by pagers at any time.  Used by window managers
			   to animate window iconification (in this case of a group).  We
			   don't do this so we can ignore it. */
			goto bad;
		} else if (prop == _XA_NET_WM_ICON) {
			/* Set of icons to display.  We don't do this so we can ignore
			   it.  At some point we might display iconified windows in a
			   windowmaker-style clip. */
			goto bad;
#endif
		} else if (prop == _XA_NET_WM_PID) {
			/* Normally set on individual (managed) windows.  This property
			   should not change after the window is managed, however, some
			   launchers might set it too late.  We use it with
			   WM_CLIENT_MACHINE to identify startup notification sequences
			   and must reinvoke the check here when that happens. */
			ewmh_process_net_startup_id(c);
		} else if (prop == _XA_NET_WM_HANDLED_ICONS) {
			/* Indicates that this client handles iconified windows.  We
			   don't provide icons for iconified windows, so, for now, it can
			   be ignored. */
		} else if (prop == _XA_NET_WM_USER_TIME) {
			ewmh_process_net_window_user_time(c);
		} else if (prop == _XA_NET_WM_USER_TIME_WINDOW) {
			ewmh_process_net_window_user_time_window(c);
		} else if (prop == _XA_NET_FRAME_EXTENTS) {
			/* Only set by the window manager on managed windows.  Does not
			   apply to groups. */
			goto bad;
#if 0
		} else if (prop == _XA_NET_WM_OPAQUE_REGION) {
			/* Set by client but only used by compositing manager.  We are
			   not a compositing manager (yet), so we can ignore it.  Also,
			   does not apply to groups. */
			goto bad;
		} else if (prop == _XA_NET_WM_BYPASS_COMPOSITOR) {
			/* Set by client but only used by compositing manager.  We are
			   not a compositing manager (yet), so we can ignore it.  Could
			   apply to a group, though. */
			goto bad;
#endif
		} else if (prop == _XA_NET_WM_WINDOW_OPACITY) {
			goto bad;
		} else if (prop == _XA_NET_WM_SYNC_REQUEST_COUNTER) {
			/* Set by client as part of sync request protocol.  We use the
			   XSync extension to determine when updates have completed, so
			   we can ignore this property change.  Also does not apply to
			   groups. */
			goto bad;
		} else if (prop == _XA_NET_STARTUP_ID) {
			/* Typically not set on group window (but on client leader
			   window). As with most client leader properties, this should
			   not be changed while a session member is managed; however,
			   some clients (e.g. roxterm, launchers) are setting this
			   property too late, so we must recheck startup notification
			   when that happens. */
			ewmh_process_net_startup_id(c);
		} else
			return False;
	}
	return True;
      bad:
	_CPRINTF(c, "bad attempt to change group leader %s\n", (name = XGetAtomName(dpy, prop)));
	if (name)
		XFree(name);
	return False;
}

static Bool
updateclientprop(Client *c, Atom prop, int state)
{
	char *name = NULL;

	if (prop <= XA_LAST_PREDEFINED) {
		switch (prop) {
		case XA_WM_NAME:
			updatetitle(c);
			drawclient(c);
			break;
		case XA_WM_ICON_NAME:
			updateiconname(c);
			break;
		case XA_WM_NORMAL_HINTS:
			updatesizehints(c);
			break;
		case XA_WM_HINTS:
			updatehints(c);
			break;
		case XA_WM_CLASS:
			/* We check this property on demand, so we can ignore updates. */
			ewmh_process_net_startup_id(c);
			break;
		case XA_WM_TRANSIENT_FOR:
			/* This property should not be changed after a window is already
			   managed. */
			updatetransientfor(c);
			goto bad;
		case XA_WM_COMMAND:
			/* TODO: after sending a WM_SAVE_YOURSELF we expect an update to
			   WM_COMMAND by the client as a part of session management. */
			ewmh_process_net_startup_id(c);
			break;
		case XA_WM_CLIENT_MACHINE:
			/* Typically not set on an individual window (but on client
			   leader window).  We use it in conjunction with _NET_WM_PID for
			   startup sequence identification.  Nevertheless, it should not
			   be updated on a window once it is managed.  Some clients might
			   change too late, so we will still update what relies on it. */
			ewmh_process_net_startup_id(c);
			goto bad;
		case XA_WM_ICON_SIZE:
			/* Placed on the root only and by the window manager. */
		case XA_WM_SIZE_HINTS:
			/* Obsolete property. */
		case XA_WM_ZOOM_HINTS:
			/* Obsolete property. */
		default:
			return False;
		}
	} else {
		if (0) {
		} else if (prop == _XA_SM_CLIENT_ID) {
			/* Note: the SM_CLIENT_ID property cannot be updated when clients
			   in the session are managed: so we do not need to update this.
			   It should never be updated on a managed client anyhoo. */
			goto bad;
		} else if (prop == _XA_WM_CLIENT_LEADER) {
			/* Note: the WM_CLIENT_LEADER property cannot be updated when
			   clients in the session are managed: so we do not need to
			   update this.  It should never be updated on a managed client
			   anyhoo. */
			goto bad;
		} else if (prop == _XA_WM_WINDOW_ROLE) {
			/* Note: if there is no WM_WINDOW_ROLE property, try to use
			   WM_CLASS and WM_NAME to uniquely identify the window.  This
			   should only be set on manageable client windows and is meant
			   for the window manager to identify the window in conjunction
			   with the SM_CLIENT_ID. */
			goto bad;
		} else if (prop == _XA_WM_PROTOCOLS) {
			/* We tend to test these on a demand basis. */
		} else if (prop == _XA_WM_STATE) {
			/* Should only be set by the window manager after the window is
			   managed.  We could potentially set it back... */
			goto bad;
		} else if (prop == _XA_WM_COLORMAP_WINDOWS) {
			updatecmapwins(c);
			drawclient(c);
		} else if (prop == _XA_WIN_LAYER) {
			/* Should only be set by the window manager after the window is
			   managed.  Clients should use client messages instead. */
			goto bad;
		} else if (prop == _XA_WIN_STATE) {
			/* Should only be set by the window manager after the window is
			   managed.  Clients should use client messages instead. */
			goto bad;
		} else if (prop == _XA_WIN_HINTS) {
			/* TODO: when this property is changed by the client, the window
			   manager must honor its changes. */
		} else if (prop == _XA_WIN_APP_STATE) {
			/* XXX: unfortunately, wm-comp.txt does not explain this
			   property. */
			goto bad;
		} else if (prop == _XA_WIN_EXPANDED_SIZE) {
			/* XXX: unfortunately, wm-comp.txt does not explain this
			   property. */
			goto bad;
		} else if (prop == _XA_WIN_ICONS) {
			/* XXX: unfortunately, wm-comp.txt does not explain this
			   property. */
			goto bad;
		} else if (prop == _XA_WIN_WORKSPACE) {
			/* Should only be set by the window manager after the windows is
			   managed.  Clients should use client messages instead. */
			goto bad;
		} else if (prop == _XA_NET_WM_NAME) {
			updatetitle(c);
			drawclient(c);
		} else if (prop == _XA_NET_WM_VISIBLE_NAME) {
			/* Should only ever be set by the window manager. */
			goto bad;
		} else if (prop == _XA_NET_WM_ICON_NAME) {
			updateiconname(c);
		} else if (prop == _XA_NET_WM_VISIBLE_ICON_NAME) {
			/* Should only ever be set by the window manager. */
			goto bad;
		} else if (prop == _XA_NET_WM_DESKTOP) {
			/* Should only be set by the window manager after the window is
			   managed.  Clients should use client messages instead. */
			goto bad;
		} else if (prop == _XA_NET_WM_WINDOW_TYPE) {
			/* Should only be set by the window manager (but not necessary)
			   after the window is managed.  Clients cannot change it. */
			goto bad;
		} else if (prop == _XA_NET_WM_STATE) {
			/* Should only be set by the window manager after the window is
			   managed.  Clients should use client messages instead. */
			goto bad;
		} else if (prop == _XA_NET_WM_ALLOWED_ACTIONS) {
			/* Should only be set by the window manager.  Clients can request
			   changes using client messages. */
			goto bad;
		} else if (prop == _XA_NET_WM_STRUT) {
			/* The client MAY change this property at any time, therefore the
			   window manager MUST watch for property notify events if the
			   window manager uses this property to assign special semantics
			   to the window. */
			c->with.struts = getstruts(c);
			updatestruts();
		} else if (prop == _XA_NET_WM_STRUT_PARTIAL) {
			/* The client MAY change this property at any time, therefore the
			   window manager MUST watch for property notify events if the
			   window manager uses this property to assign special semantics
			   to the window.  If both this property and the _NET_WM_STRUT
			   property values are set, the window manager MUST ignore the
			   _NET_WM_STRUPT property and use this one instead. */
			c->with.struts = getstruts(c);
			updatestruts();
#if 0
		} else if (prop == _XA_NET_WM_ICON_GEOMETRY) {
			/* Normally set by pagers at any time.  Used by window managers
			   to animate window iconification.  We don't do this so we can
			   ignore it. */
			return False;
		} else if (prop == _XA_NET_WM_ICON) {
			/* Set of icons to display.  We don't do this so we can ignore
			   it.  At some point we might display iconified windows in a
			   windowmaker-style clip. */
			return False;
#endif
		} else if (prop == _XA_NET_WM_PID) {
			/* This property should not change after the window is managed,
			   however, some startup notification assistance programs might
			   set it too late.  We use it with WM_CLIENT_MACHINE to identify
			   startup notification sequences and must reinvoke the check
			   here when that happens. */
			ewmh_process_net_startup_id(c);
		} else if (prop == _XA_NET_WM_HANDLED_ICONS) {
			/* Indicates that this client handles iconified windows.  We
			   don't provide icons for iconified windows, so, for now, it can
			   be ignored. */
			return False;
		} else if (prop == _XA_NET_WM_USER_TIME) {
			ewmh_process_net_window_user_time(c);
		} else if (prop == _XA_NET_WM_USER_TIME_WINDOW) {
			ewmh_process_net_window_user_time_window(c);
		} else if (prop == _XA_NET_FRAME_EXTENTS) {
			/* Only set by the window manager on managed windows. */
			goto bad;
#if 0
		} else if (prop == _XA_NET_WM_OPAQUE_REGION) {
			/* Set by client but only used by compositing manager.  We are
			   not a compositing manager (yet), so we can ignore it. */
			return False;
		} else if (prop == _XA_NET_WM_BYPASS_COMPOSITOR) {
			/* Set by client but only used by compositing manager.  We are
			   not a compositing manager (yet), so we can ignore it. */
			return False;
#endif
		} else if (prop == _XA_NET_WM_WINDOW_OPACITY) {
			ewmh_process_net_window_opacity(c);
		} else if (prop == _XA_NET_WM_SYNC_REQUEST_COUNTER) {
			/* Set by client as part of sync request protocol.  We use the
			   XSync extension to determine when updates have completed, so
			   we can ignore this property change. */
			return False;
		} else if (prop == _XA_NET_STARTUP_ID) {
			/* Typcially not set on client window (but on client leader
			   window).  As with most client leader properties, this should
			   not be changed while a session member is managed; however,
			   some clients (e.g. roxterm) are setting this property too
			   late, so we must recheck startup notification when that
			   happens. */
			ewmh_process_net_startup_id(c);
		} else
			return False;
	}
	return True;
      bad:
	CPRINTF(c, "bad attempt to change client %s\n", (name = XGetAtomName(dpy, prop)));
	if (name)
		XFree(name);
	return False;
}

static Bool
updateclienttime(Client *c, Atom prop, int state)
{
	if (prop <= XA_LAST_PREDEFINED) {
		return False;
	} else {
		if (0) {
		} else if (prop == _XA_NET_WM_USER_TIME) {
			ewmh_process_net_window_user_time(c);
		} else if (prop == _XA_NET_WM_USER_TIME_WINDOW) {
			ewmh_process_net_window_user_time_window(c);
		} else
			return False;
	}
	return True;
}

static Bool
updaterootprop(Window root, Atom prop, int state)
{
	if (prop <= XA_LAST_PREDEFINED) {
		switch (prop) {
		case XA_WM_NAME:
		case XA_WM_ICON_NAME:
		case XA_WM_NORMAL_HINTS:
		case XA_WM_HINTS:
		case XA_WM_CLASS:
		case XA_WM_TRANSIENT_FOR:
		case XA_WM_COMMAND:
			return False;
		case XA_WM_ICON_SIZE:
			/* Only set on the root window by the window manager, and then
			   only if it wishes to constrain the icon pixmap or icon window.
			   Note that this applies to dock apps too, but few (if any) dock
			   apps observe it. */
			return True;
		default:
			return False;
		}
	} else {
		char *name;

		if (0) {
		} else if (prop == _XA_WIN_PROTOCOLS) {
		} else if (prop == _XA_WIN_ICONS) {
		} else if (prop == _XA_WIN_WORKSPACE) {
		} else if (prop == _XA_WIN_WORKSPACE_COUNT) {
		} else if (prop == _XA_WIN_WORKSPACE_NAMES) {
		} else if (prop == _XA_WIN_AREA) {
		} else if (prop == _XA_WIN_AREA_COUNT) {
		} else if (prop == _XA_WIN_CLIENT_LIST) {
		} else if (prop == _XA_NET_SUPPORTED) {
		} else if (prop == _XA_NET_CLIENT_LIST) {
		} else if (prop == _XA_NET_NUMBER_OF_DESKTOPS) {
		} else if (prop == _XA_NET_DESKTOP_GEOMETRY) {
		} else if (prop == _XA_NET_DESKTOP_VIEWPORT) {
		} else if (prop == _XA_NET_CURRENT_DESKTOP) {
		} else if (prop == _XA_NET_DESKTOP_NAMES) {
			ewmh_process_net_desktop_names();
			return True;
		} else if (prop == _XA_NET_ACTIVE_WINDOW) {
		} else if (prop == _XA_NET_WORKAREA) {
		} else if (prop == _XA_NET_SUPPORTING_WM_CHECK) {
		} else if (prop == _XA_NET_VIRTUAL_ROOTS) {
		} else if (prop == _XA_NET_DESKTOP_LAYOUT) {
			ewmh_process_net_desktop_layout();
			return True;
		} else if (prop == _XA_NET_SHOWING_DESKTOP) {
		} else
			return False;

		name = XGetAtomName(dpy, prop);
		EPRINTF("%s WM property %s\n",
			 state == PropertyDelete ? "deletion of" : "change to",
			 (name = XGetAtomName(dpy, prop)));
		if (name)
			XFree(name);
		return False;
	}
}

static Bool
propertynotify(XEvent *e)
{
	Client *c;
	Window *m;
	XPropertyEvent *ev = &e->xproperty;
	unsigned i, n = 0;
	Bool result = False;

	if ((m = getgroup(ev->window, ClientSession, &n))) {
		for (i = 0; i < n; i++)
			if ((c = getclient(m[i], ClientWindow)))
				result |= updatesessionprop(c, ev->atom, ev->state);
		/* client leader windows must not be managed */
		return result;
	} else if ((m = getgroup(ev->window, ClientGroup, &n))) {
		for (i = 0; i < n; i++)
			if ((c = getclient(m[i], ClientWindow)))
				result |= updateleaderprop(c, ev->atom, ev->state);
		/* group leader window may also be managed */
		if ((c = getclient(ev->window, ClientWindow)))
			result |= updateclientprop(c, ev->atom, ev->state);
		return result;
	} else if ((c = getclient(ev->window, ClientWindow))) {
		return updateclientprop(c, ev->atom, ev->state);
	} else if ((c = getclient(ev->window, ClientTimeWindow))) {
		return updateclienttime(c, ev->atom, ev->state);
	} else if (ev->window == scr->root) {
		return updaterootprop(scr->root, ev->atom, ev->state);
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
		DPRINTF("cleanup switching\n");
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
		DPRINTF("cleanup switching\n");
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

		DPRINTF("cleanup restarting\n");
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
			if (!einfo[i].have)
				continue;
			if (ev->type >= einfo[i].event && ev->type < einfo[i].event + EXTRANGE) {
				int slot = ev->type - einfo[i].event + LASTEvent + EXTRANGE * i;

				if (handler[slot])
					return (handler[slot]) (ev);
			}
		}
	DPRINTF("WARNING: No handler for event type %d\n", ev->type);
	return False;
}

AScreen *
getscreen(Window win, Bool query)
{
	Window *wins = NULL, wroot, parent;
	unsigned int num;
	AScreen *s = NULL;

	if (!win)
		return (s);
	if (!XFindContext(dpy, win, context[ScreenContext], (XPointer *) &s))
		return (s);
	if (!query)
		return (s);
	ignorenext();
	if (XQueryTree(dpy, win, &wroot, &parent, &wins, &num))
		XFindContext(dpy, wroot, context[ScreenContext], (XPointer *) &s);
	else
		EPRINTF("XQueryTree(0x%lx) failed!\n", win);
	if (wins)
		XFree(wins);
	return (s);
}

AScreen *
geteventscr(XEvent *ev)
{
	Window win = ev->xany.window;
	Bool query = False;

	if (ev->type == DestroyNotify) {
		if (ev->xdestroywindow.event == ev->xdestroywindow.window)
			query = False;
		else
			win = ev->xdestroywindow.event;
	} else if (ev->type > PropertyNotify || ev->type == NoExpose || ev->type == GraphicsExpose)
		query = False;
	if (!(event_scr = getscreen(win, query))) {
		if (query)
			EPRINTF("Could not find event screen for event %d with window 0x%lx\n", ev->type, win);
		event_scr = scr;
	}
	return (event_scr);
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
		int sig;

		if ((sig = signum)) {
			signum = 0;

			switch (sig) {
			case SIGHUP:
				reload();
				break;
			case SIGTERM:
			case SIGINT:
			case SIGQUIT:
				quit(NULL);
				break;
			case SIGCHLD:
				wait(&sig);
				break;
			default:
				break;
			}
		}

		if (poll(&pfd, 1, -1) == -1) {
			if (errno == EAGAIN || errno == EINTR || errno == ERESTART) {
				errno = 0;
				continue;
			}
			eprint("%s", "poll failed: %s\n", strerror(errno));
		} else {
			if (pfd.revents & (POLLNVAL | POLLHUP | POLLERR)) {
				if (pfd.revents & POLLNVAL)
					EPRINTF("POLLNVAL bit set!\n");
				if (pfd.revents & POLLHUP)
					EPRINTF("POLLHUP bit set!\n");
				if (pfd.revents & POLLERR)
					EPRINTF("POLLERR bit set!\n");
				eprint("%s", "poll error\n");
			}
			if (pfd.revents & POLLIN) {
				while (XPending(dpy) && running) {
					XNextEvent(dpy, &ev);
					scr = geteventscr(&ev);
					if (handle_event(&ev))
						XFlush(dpy);
					else
						XPRINTF("WARNING: Event %d not handled\n",
							ev.type);
					if (lockfocus) {
						discarding(focuslock);
						lockfocus = False;
						focuslock = NULL;
					}
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
manageoverride(Window win, XWindowAttributes *wa)
{
	/* for when we want to manage opacity on override windows */
}

void
scan(void)
{
	unsigned int i, num;
	Window *wins, d1, d2;

	wins = NULL;
	if (XQueryTree(dpy, scr->root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			XWindowAttributes wa = { 0, };
			XWMHints *wmh = NULL;
			long state = -1;
			Client *c;

			if (!wins[i])
				continue;

			DPRINTF("scan checking window 0x%08lx\n", wins[i]);

			if ((c = getclient(wins[i], ClientAny))) {
				DPRINTF("-> deleting 0x%08lx (already managed by %s)\n", wins[i], c->name);
				wins[i] = None;
				continue;
			}
			if (!XGetWindowAttributes(dpy, wins[i], &wa)) {
				DPRINTF("-> deleting 0x%08lx (no window attributes)\n", wins[i]);
				wins[i] = None;
				continue;
			}
			if (wa.override_redirect) {
				DPRINTF("-> deleting 0x%08lx (override redirect set)\n", wins[i]);
				wins[i] = None;
				continue;
			}
			if (issystray(wins[i])) {
				DPRINTF("-> deleting 0x%08lx (is a system tray icon)\n", wins[i]);
				wins[i] = None;
				continue;
			}
			if ((wa.map_state != IsViewable) && ((state = getstate(wins[i])) != IconicState) && (state != NormalState)) {
				DPRINTF("-> deleting 0x%08lx (not viewable and state = %ld)\n", wins[i], state);
				wins[i] = None;
				continue;
			}
			if (XGetTransientForHint(dpy, wins[i], &d1)) {
				DPRINTF("-> skipping 0x%08lx (transient-for property set)\n", wins[i]);
				continue;
			}
			if (!(wmh = XGetWMHints(dpy, wins[i])) ||
					((wmh->flags & WindowGroupHint) && (wmh->window_group != wins[i])) ||
					!(wmh->flags & IconWindowHint)) {
				DPRINTF("-> skipping 0x%08lx (not group leader)\n", wins[i]);
				if (wmh)
					XFree(wmh);
				continue;
			}
			DPRINTF("-> managing 0x%08lx\n", wins[i]);
			manage(wins[i], &wa);
			wins[i] = None;
			if (wmh)
				XFree(wmh);
		}
		for (i = 0; i < num; i++) {
			XWindowAttributes wa = { 0, };

			if (!wins[i])
				continue;
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			DPRINTF("-> managing 0x%08lx\n", wins[i]);
			manage(wins[i], &wa);
			wins[i] = None;
		}
	} else
		EPRINTF("XQueryTree(0x%lx) failed\n", scr->root);
	if (wins)
		XFree(wins);
	DPRINTF("done scanning screen %d\n", scr->screen);
	focus(sel);
	ewmh_update_kde_splash_progress();
}

static Bool
isomni(Client *c)
{
	if (!c->is.sticky) {
		unsigned long long alltags = (1ULL << scr->ntags) - 1;

		if ((c->tags & alltags) != alltags)
			return False;
	}
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

static Monitor *
findmonitornear(int row, int col)
{
	int w = scr->sw / scr->m.cols;
	int h = scr->sh / scr->m.rows;
	float dist = hypotf((scr->m.rows + 1) * h, (scr->m.cols + 1) * w);
	Monitor *m, *best = NULL;

	DPRINTF("Finding monitor nearest (%d,%d), max dist = %f\n", col, row, dist);
	for (m = scr->monitors; m; m = m->next) {
		float test;

		test = hypotf((m->row - row) * h, (m->col - col) * w);
		DPRINTF("Testing monitor %d (%d,%d), dist = %f\n", m->num, m->col, m->row, test);
		if (test <= dist) {
			DPRINTF("Monitor %d is best!\n", m->num);
			dist = test;
			best = m;
		}
		if (dist == 0)
			break;
	}
	return best;
}

static void
updatedock(void)
{
	Monitor *m;
	int i, dockmon;

	DPRINTF("Number of dock monitors is %d\n", scr->nmons);
	for (m = scr->monitors, i = 0; i < scr->nmons; i++, m++) {
		m->dock.position = DockNone;
		m->dock.wa = m->wa;
	}
	scr->dock.monitor = NULL;
	DPRINTF("Dock monitor option is %d\n", scr->options.dockmon);
	dockmon = (int) scr->options.dockmon - 1;
	DPRINTF("Dock monitor chosen is %d\n", dockmon);
	if (dockmon > scr->nmons - 1)
		dockmon = scr->nmons - 1;
	DPRINTF("Dock monitor adjusted is %d\n", dockmon);
	/* find the monitor if dock position is screen relative */
	if (dockmon < 0) {
		DockPosition pos = scr->options.dockpos;
		int row, col;
		Monitor *dm;

		switch (pos) {
		default:
			pos = DockNone;
		case DockNone:
			row = 0;
			col = 0;
			break;
		case DockEast:
			row = (scr->m.rows + 1) / 2 - 1;	/* Center */
			col = scr->m.cols - 1;	/* East */
			break;
		case DockNorthEast:
			row = 0;	/* North */
			col = scr->m.cols - 1;	/* East */
			break;
		case DockNorth:
			row = 0;	/* North */
			col = (scr->m.cols + 1) / 2 - 1;	/* Center */
			break;
		case DockNorthWest:
			row = 0;	/* North */
			col = 0;	/* West */
			break;
		case DockWest:
			row = (scr->m.rows + 1) / 2 - 1;	/* Center */
			col = 0;	/* West */
			break;
		case DockSouthWest:
			row = scr->m.rows - 1;	/* South */
			col = 0;	/* West */
			break;
		case DockSouth:
			row = scr->m.rows - 1;	/* South */
			col = (scr->m.cols + 1) / 2 - 1;	/* Center */
			break;
		case DockSouthEast:
			row = scr->m.rows - 1;	/* South */
			col = scr->m.cols - 1;	/* East */
			break;
		}
		if (pos != DockNone) {
			if ((dm = findmonitornear(row, col))) {
				dockmon = dm->num;
				DPRINTF("Found monitor %d near (%d,%d)\n", dockmon, col, row);
			} else {
				DPRINTF("Cannot find monitor near (%d,%d)\n", col, row);
			}
		}
	}
	if (dockmon >= 0) {
		DPRINTF("Looking for monitor %d assigned to dock...\n", dockmon);
		for (m = scr->monitors; m; m = m->next) {
			if (m->num == dockmon) {
				m->dock.position = scr->options.dockpos;
				m->dock.orient = scr->options.dockori;
				scr->dock.monitor = m;
				DPRINTF("Found monitor %d assigned to dock.\n", dockmon);
				break;
			}
		}
	}
	if (!scr->dock.monitor) {
		DPRINTF("Did not find monitor assigned dock %d\n", dockmon);
		if ((m = scr->monitors)) {
			DPRINTF("Falling back to default monitor for dock.\n");
			m->dock.position = scr->options.dockpos;
			m->dock.orient = scr->options.dockori;
			scr->dock.monitor = m;
		}
	}
}

void
updatebarriers(void)
{
	Monitor *m;
	int i;

	if (!einfo[XfixesBase].have)
		return;
	for (m = scr->monitors; m; m = m->next)
		for (i = 0; i < 8; i++)
			if (m->bars[i])
				XFixesDestroyPointerBarrier(dpy, m->bars[i]);
	for (m = scr->monitors; m; m = m->next) {
		int w, h;

		w = m->sc.w / 20;
		h = m->sc.h / 20;

		m->bars[0] =
		    XFixesCreatePointerBarrier(dpy, scr->root,
					       m->sc.x, m->sc.y,
					       m->sc.x, m->sc.y + h,
					       BarrierPositiveX, 0, NULL);
		m->bars[1] =
		    XFixesCreatePointerBarrier(dpy, scr->root,
					       m->sc.x, m->sc.y,
					       m->sc.x + w, m->sc.y,
					       BarrierPositiveY, 0, NULL);
		m->bars[2] =
		    XFixesCreatePointerBarrier(dpy, scr->root,
					       m->sc.x + m->sc.w - w,
					       m->sc.y, m->sc.x + m->sc.w, m->sc.y,
					       BarrierPositiveY, 0, NULL);
		m->bars[3] =
		    XFixesCreatePointerBarrier(dpy, scr->root,
					       m->sc.x + m->sc.w, m->sc.y,
					       m->sc.x + m->sc.w, m->sc.y + h,
					       BarrierNegativeX, 0, NULL);
		m->bars[4] =
		    XFixesCreatePointerBarrier(dpy, scr->root,
					       m->sc.x + m->sc.w, m->sc.y + m->sc.h - h,
					       m->sc.x + m->sc.w, m->sc.y + m->sc.h,
					       BarrierNegativeX, 0, NULL);
		m->bars[5] =
		    XFixesCreatePointerBarrier(dpy, scr->root,
					       m->sc.x + m->sc.w - w, m->sc.y + m->sc.h,
					       m->sc.x + m->sc.w, m->sc.y + m->sc.h,
					       BarrierNegativeY, 0, NULL);
		m->bars[6] =
		    XFixesCreatePointerBarrier(dpy, scr->root,
					       m->sc.x, m->sc.y + m->sc.h,
					       m->sc.x + w, m->sc.y + m->sc.h,
					       BarrierNegativeY, 0, NULL);
		m->bars[7] =
		    XFixesCreatePointerBarrier(dpy, scr->root,
					       m->sc.x, m->sc.y + m->sc.h - h,
					       m->sc.x, m->sc.y + m->sc.h,
					       BarrierPositiveX, 0, NULL);
	}
}

void
updatemonitors(XEvent *e, int n, Bool size_update, Bool full_update)
{
	int i, j;
	Client *c;
	Monitor *m;
	int w, h;
	Bool changed;
	View *v;

	DPRINTF("There are now %d monitors (was %d)\n", n, scr->nmons);
	for (i = 0; i < n; i++)
		scr->monitors[i].next = &scr->monitors[i + 1];
	scr->monitors[n - 1].next = NULL;
	if (scr->nmons != n)
		full_update = True;
	if (full_update)
		size_update = True;
	scr->nmons = n;
	DPRINTF("Performing %s/%s updates\n",
			full_update ? "full" : "partial",
			size_update ? "resize" : "no-resize");
	if (e) {
		DPRINTF("Responding to an event\n");
		if (full_update) {
			for (c = scr->clients; c; c = c->next) {
				if (isomni(c))
					continue;
				if (!(m = findmonitor(c)))
					m = nearmonitor();
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
	updatebarriers();
	/* find largest monitor */
	DPRINTF("Finding largest monitor\n");
	for (w = 0, h = 0, scr->sw = 0, scr->sh = 0, i = 0; i < n; i++) {
		m = scr->monitors + i;
		DPRINTF("Processing monitor %d\n", m->index);
		w = max(w, m->sc.w);
		h = max(h, m->sc.h);
		DPRINTF("Maximum monitor dimensions %dx%d\n", w, h);
		scr->sw = max(scr->sw, m->sc.x + m->sc.w);
		scr->sh = max(scr->sh, m->sc.y + m->sc.h);
		DPRINTF("Maximum screen  dimensions %dx%d\n", w, h);
	}
	DPRINTF("Maximum screen  size is now %dx%d\n", scr->sw, scr->sh);
	DPRINTF("Maximum monitor size is now %dx%d\n", w, h);
	scr->m.rows = (scr->sh + h - 1) / h;
	scr->m.cols = (scr->sw + w - 1) / w;
	DPRINTF("Monitor array is %dx%d\n", scr->m.cols, scr->m.rows);
	h = scr->sh / scr->m.rows;
	w = scr->sw / scr->m.cols;
	DPRINTF("Each monitor cell is %dx%d\n", w, h);
	for (i = 0; i < n; i++) {
		m = scr->monitors + i;
		m->row = m->my / h;
		m->col = m->mx / w;
		DPRINTF("Monitor %d at (%d,%d)\n", i+1, m->col, m->row);
	}
	/* handle insane geometries, push overlaps to the right */
	DPRINTF("Handling insane geometries...\n");
	do {
		changed = False;
		for (i = 0; i < n && !changed; i++) {
			m = scr->monitors + i;
			for (j = i + 1; j < n && !changed; j++) {
				Monitor *o = scr->monitors + j;

				if (m->row != o->row || m->col != o->col)
					continue;
				DPRINTF("Monitors %d and %d conflict at (%d,%d)\n",
						m->index, o->index, m->col, m->row);
				if (m->mx < o->mx) {
					o->col++;
					DPRINTF("Moving monitor %d to the right (%d,%d)\n",
							o->index, o->col, o->row);
					scr->m.cols = max(scr->m.cols, o->col + 1);
					DPRINTF("Monitor array is now %dx%d\n",
							scr->m.cols, scr->m.rows);
				} else {
					m->col++;
					DPRINTF("Moving monitor %d to the right (%d,%d)\n",
							m->index, m->col, m->row);
					scr->m.cols = max(scr->m.cols, m->col + 1);
					DPRINTF("Monitor array is now %dx%d\n",
							scr->m.cols, scr->m.rows);
				}
				changed = True;
			}
		}
	} while (changed);
	/* update all view pointers */
	DPRINTF("There are %d tags\n", scr->ntags);
	for (v = scr->views, i = 0; i < scr->ntags; i++, v++)
		v->curmon = NULL;
	for (m = scr->monitors, i = 0; i < n; i++, m++) {
		DPRINTF("Setting view %d to monitor %d\n", m->curview->index, m->index);
		m->curview->curmon = m;
	}
	DPRINTF("Updating dock...\n");
	updatedock();
	updatestruts();
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
	if (einfo[XineramaBase].have) {
		int i;
		XineramaScreenInfo *si;

		DPRINTF("XINERAMA extension supported\n");
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
		DPRINTF("XINERAMA defines %d monitors for screen %d\n", n, scr->screen);
		if (n < 2) {
			XFree(si);
			goto no_xinerama;
		}
		for (i = 0; i < n; i++) {
			if (i < scr->nmons) {
				m = &scr->monitors[i];
				DPRINTF("Checking existing monitor %d\n", m->index);
				if (m->sc.x != si[i].x_org) {
					DPRINTF("Monitor %d x position changed from %d to %d\n",
							m->index, m->sc.x, si[i].x_org);
					m->sc.x = m->wa.x = m->dock.wa.x = si[i].x_org;
					m->mx = m->sc.x + m->sc.w / 2;
					full_update = True;
				}
				if (m->sc.y != si[i].y_org) {
					DPRINTF("Monitor %d y position changed from %d to %d\n",
							m->index, m->sc.y, si[i].y_org);
					m->sc.y = m->wa.y = m->dock.wa.y = si[i].y_org;
					m->my = m->sc.y + m->sc.h / 2;
					full_update = True;
				}
				if (m->sc.w != si[i].width) {
					DPRINTF("Monitor %d width changed from %d to %d\n",
							m->index, m->sc.w, si[i].width);
					m->sc.w = m->wa.w = m->dock.wa.w = si[i].width;
					m->mx = m->sc.x + m->sc.w / 2;
					size_update = True;
				}
				if (m->sc.h != si[i].height) {
					DPRINTF("Monitor %d height changed from %d to %d\n",
							m->index, m->sc.h, si[i].height);
					m->sc.h = m->wa.h = m->dock.wa.h = si[i].height;
					m->my = m->sc.y + m->sc.h / 2;
					size_update = True;
				}
				if (m->num != si[i].screen_number) {
					DPRINTF("Monitor %d screen number changed from %d to %d\n",
							m->index, m->num, si[i].screen_number);
					m->num = si[i].screen_number;
					full_update = True;
				}
			} else {
				XSetWindowAttributes wa;
				int mask = 0;
				int j;

				DPRINTF("Adding new monitor %d\n", i);
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
				// m->dock.position = scr->options.dockpos;
				// m->dock.orient = scr->options.dockori;
				m->mx = m->sc.x + m->sc.w / 2;
				m->my = m->sc.y + m->sc.h / 2;
				m->num = si[i].screen_number;
				m->curview = scr->views + (m->num % scr->ntags);
				m->veil =
				    XCreateSimpleWindow(dpy, scr->root, m->sc.x, m->sc.y,
							m->sc.w, m->sc.h, 0, 0, 0);
				wa.background_pixmap = None;
				mask |= CWBackPixmap;
				wa.override_redirect = True;
				mask |= CWOverrideRedirect;
				wa.border_pixmap = CopyFromParent;
				mask |= CWBorderPixmap;
				wa.backing_store = NotUseful;
				mask |= CWBackingStore;
				wa.save_under = True;
				mask |= CWSaveUnder;
				XChangeWindowAttributes(dpy, m->veil, mask, &wa);
				for (j = 0; j < 8; j++)
					m->bars[j] = None;
			}
			DPRINTF("Monitor %d:\n", m->index);
			DPRINTF("\tindex           = %d\n", m->index);
			DPRINTF("\tscreen          = %d\n", m->num);
			DPRINTF("\tscreen.x        = %d\n", m->sc.x);
			DPRINTF("\tscreen.y        = %d\n", m->sc.y);
			DPRINTF("\tscreen.w        = %d\n", m->sc.w);
			DPRINTF("\tscreen.h        = %d\n", m->sc.h);
			DPRINTF("\tworkarea.x      = %d\n", m->wa.x);
			DPRINTF("\tworkarea.y      = %d\n", m->wa.y);
			DPRINTF("\tworkarea.w      = %d\n", m->wa.w);
			DPRINTF("\tworkarea.h      = %d\n", m->wa.h);
			DPRINTF("\tdock.workarea.x = %d\n", m->dock.wa.x);
			DPRINTF("\tdock.workarea.y = %d\n", m->dock.wa.y);
			DPRINTF("\tdock.workarea.w = %d\n", m->dock.wa.w);
			DPRINTF("\tdock.workarea.h = %d\n", m->dock.wa.h);
			DPRINTF("\tmiddle.x        = %d\n", m->mx);
			DPRINTF("\tmiddle.y        = %d\n", m->my);
			DPRINTF("\tmiddle.y        = %d\n", m->my);
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
	if (einfo[XrandrBase].have) {
		XRRScreenResources *sr;
		int i, j;

		DPRINTF("RANDR extension supported\n");
		if (!(sr = XRRGetScreenResources(dpy, scr->root))) {
			DPRINTF("RANDR not active for display\n");
			goto no_xrandr;
		}
		DPRINTF("RANDR defines %d ctrc for display\n", sr->ncrtc);
		if (sr->ncrtc < 2) {
			XRRFreeScreenResources(sr);
			goto no_xrandr;
		}
		for (i = 0, n = 0; i < sr->ncrtc; i++) {
			XRRCrtcInfo *ci;

			DPRINTF("Checking CRTC %d\n", i);
			if (!(ci = XRRGetCrtcInfo(dpy, sr, sr->crtcs[i]))) {
				DPRINTF("CRTC %d not defined\n", i);
				continue;
			}
			if (!ci->width || !ci->height) {
				DPRINTF("CRTC %d is %dx%d\n", i, ci->width, ci->height);
				XRRFreeCrtcInfo(ci);
				continue;
			}
			/* skip mirrors */
			for (j = 0; j < n; j++)
				if (scr->monitors[j].sc.x == scr->monitors[n].sc.x &&
				    scr->monitors[j].sc.y == scr->monitors[n].sc.y)
					break;
			if (j < n) {
				DPRINTF("Monitor %d is a mirror of %d\n", n, j);
				XRRFreeCrtcInfo(ci);
				continue;
			}

			if (n < scr->nmons) {
				m = &scr->monitors[n];
				DPRINTF("Checking existing monitor %d\n", m->index);
				if (m->sc.x != ci->x) {
					DPRINTF("Monitor %d x position changed from %d to %d\n",
							m->index, m->sc.x, ci->x);
					m->sc.x = m->wa.x = m->dock.wa.x = ci->x;
					m->mx = m->sc.x + m->sc.w / 2;
					full_update = True;
				}
				if (m->sc.y != ci->y) {
					DPRINTF("Monitor %d y position changed from %d to %d\n",
							m->index, m->sc.y, ci->y);
					m->sc.y = m->wa.y = m->dock.wa.y = ci->y;
					m->my = m->sc.y + m->sc.h / 2;
					full_update = True;
				}
				if (m->sc.w != ci->width) {
					DPRINTF("Monitor %d width changed from %d to %d\n",
							m->index, m->sc.w, ci->width);
					m->sc.w = m->wa.w = m->dock.wa.w = ci->width;
					m->mx = m->sc.x + m->sc.w / 2;
					size_update = True;
				}
				if (m->sc.h != ci->height) {
					DPRINTF("Monitor %d height changed from %d to %d\n",
							m->index, m->sc.h, ci->height);
					m->sc.h = m->wa.h = m->dock.wa.h = ci->height;
					m->my = m->sc.y + m->sc.h / 2;
					size_update = True;
				}
				if (m->num != i) {
					DPRINTF("Monitor %d screen number changed from %d to %d\n",
							m->index, m->num, i);
					m->num = i;
					full_update = True;
				}
			} else {
				XSetWindowAttributes wa;
				int mask = 0;
				int j;

				DPRINTF("Adding new monitor %d\n", n);
				scr->monitors =
				    erealloc(scr->monitors,
					     (n + 1) * sizeof(*scr->monitors));
				m = &scr->monitors[n];
				full_update = True;
				m->index = n;
				// m->dock.position = scr->options.dockpos;
				// m->dock.orient = scr->options.dockori;
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
				mask |= CWBackPixmap;
				wa.override_redirect = True;
				mask |= CWOverrideRedirect;
				wa.border_pixmap = CopyFromParent;
				mask |= CWBorderPixmap;
				wa.backing_store = NotUseful;
				mask |= CWBackingStore;
				wa.save_under = True;
				mask |= CWSaveUnder;
				XChangeWindowAttributes(dpy, m->veil, mask, &wa);
				for (j = 0; j < 8; j++)
					m->bars[j] = None;
			}
			DPRINTF("Monitor %d:\n", m->index);
			DPRINTF("\tindex           = %d\n", m->index);
			DPRINTF("\tscreen          = %d\n", m->num);
			DPRINTF("\tscreen.x        = %d\n", m->sc.x);
			DPRINTF("\tscreen.y        = %d\n", m->sc.y);
			DPRINTF("\tscreen.w        = %d\n", m->sc.w);
			DPRINTF("\tscreen.h        = %d\n", m->sc.h);
			DPRINTF("\tworkarea.x      = %d\n", m->wa.x);
			DPRINTF("\tworkarea.y      = %d\n", m->wa.y);
			DPRINTF("\tworkarea.w      = %d\n", m->wa.w);
			DPRINTF("\tworkarea.h      = %d\n", m->wa.h);
			DPRINTF("\tdock.workarea.x = %d\n", m->dock.wa.x);
			DPRINTF("\tdock.workarea.y = %d\n", m->dock.wa.y);
			DPRINTF("\tdock.workarea.w = %d\n", m->dock.wa.w);
			DPRINTF("\tdock.workarea.h = %d\n", m->dock.wa.h);
			DPRINTF("\tmiddle.x        = %d\n", m->mx);
			DPRINTF("\tmiddle.y        = %d\n", m->my);
			DPRINTF("\tmiddle.y        = %d\n", m->my);
			n++;
			XRRFreeCrtcInfo(ci);
		}
		XRRFreeScreenResources(sr);
		if (n < 1)
			goto no_xrandr;
		updatemonitors(e, n, size_update, full_update);
		return True;

	}
      no_xrandr:
#endif
	n = 1;
	if (n <= scr->nmons) {
		m = &scr->monitors[0];
		DPRINTF("Checking existing monitor %d\n", m->index);
		if (m->sc.x != 0) {
			DPRINTF("Monitor %d x position changed from %d to %d\n",
					m->index, m->sc.x, 0);
			m->sc.x = m->wa.x = m->dock.wa.x = 0;
			m->mx = m->sc.x + m->sc.w / 2;
			full_update = True;
		}
		if (m->sc.y != 0) {
			DPRINTF("Monitor %d y position changed from %d to %d\n",
					m->index, m->sc.y, 0);
			m->sc.y = m->wa.y = m->dock.wa.y = 0;
			m->my = m->sc.y + m->sc.h / 2;
			full_update = True;
		}
		if (m->sc.w != DisplayWidth(dpy, scr->screen)) {
			DPRINTF("Monitor %d width changed from %d to %d\n",
					m->index, m->sc.w, (int) DisplayWidth(dpy, scr->screen));
			m->sc.w = m->wa.w = m->dock.wa.w = DisplayWidth(dpy, scr->screen);
			m->mx = m->sc.x + m->sc.w / 2;
			size_update = True;
		}
		if (m->sc.h != DisplayHeight(dpy, scr->screen)) {
			DPRINTF("Monitor %d height changed from %d to %d\n",
					m->index, m->sc.h, (int) DisplayHeight(dpy, scr->screen));
			m->sc.h = m->wa.h = m->dock.wa.h =
			    DisplayHeight(dpy, scr->screen);
			m->my = m->sc.y + m->sc.h / 2;
			size_update = True;
		}
		if (m->num != 0) {
			DPRINTF("Monitor %d screen number changed from %d to %d\n",
					m->index, m->num, 0);
			m->num = 0;
			full_update = True;
		}
	} else {
		XSetWindowAttributes wa;
		int mask = 0;
		int j;

		DPRINTF("Adding new monitor %d\n", 0);
		scr->monitors = erealloc(scr->monitors, n * sizeof(*scr->monitors));
		m = &scr->monitors[0];
		full_update = True;
		m->index = 0;
		// m->dock.position = scr->options.dockpos;
		// m->dock.orient = scr->options.dockori;
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
		mask |= CWBackPixmap;
		wa.override_redirect = True;
		mask |= CWOverrideRedirect;
		wa.border_pixmap = CopyFromParent;
		mask |= CWBorderPixmap;
		wa.backing_store = NotUseful;
		mask |= CWBackingStore;
		wa.save_under = True;
		mask |= CWSaveUnder;
		XChangeWindowAttributes(dpy, m->veil, mask, &wa);
		for (j = 0; j < 8; j++)
			m->bars[j] = None;
	}
	DPRINTF("Monitor %d:\n", m->index);
	DPRINTF("\tindex           = %d\n", m->index);
	DPRINTF("\tscreen          = %d\n", m->num);
	DPRINTF("\tscreen.x        = %d\n", m->sc.x);
	DPRINTF("\tscreen.y        = %d\n", m->sc.y);
	DPRINTF("\tscreen.w        = %d\n", m->sc.w);
	DPRINTF("\tscreen.h        = %d\n", m->sc.h);
	DPRINTF("\tworkarea.x      = %d\n", m->wa.x);
	DPRINTF("\tworkarea.y      = %d\n", m->wa.y);
	DPRINTF("\tworkarea.w      = %d\n", m->wa.w);
	DPRINTF("\tworkarea.h      = %d\n", m->wa.h);
	DPRINTF("\tdock.workarea.x = %d\n", m->dock.wa.x);
	DPRINTF("\tdock.workarea.y = %d\n", m->dock.wa.y);
	DPRINTF("\tdock.workarea.w = %d\n", m->dock.wa.w);
	DPRINTF("\tdock.workarea.h = %d\n", m->dock.wa.h);
	DPRINTF("\tmiddle.x        = %d\n", m->mx);
	DPRINTF("\tmiddle.y        = %d\n", m->my);
	DPRINTF("\tmiddle.y        = %d\n", m->my);
	updatemonitors(e, n, size_update, full_update);
	return True;
}

void
sighandler(int sig)
{
	if (sig)
		signum = sig;
	if (signum == SIGCHLD) {
		int status;

		wait(&status);
		signum = 0;
	} else if (signum == SIGSEGV || signum == SIGBUS)
		eprint("fatal: caught signal %d\n", signum);
}

void
initcursors(Bool reload)
{
	/* init cursors */
	/* *INDENT-OFF* */
	if (!cursor[CursorTopLeft])	cursor[CursorTopLeft]	  = XCreateFontCursor(dpy, XC_top_left_corner);
	if (!cursor[CursorTop])		cursor[CursorTop]	  = XCreateFontCursor(dpy, XC_top_side);
	if (!cursor[CursorTopRight])	cursor[CursorTopRight]	  = XCreateFontCursor(dpy, XC_top_right_corner);
	if (!cursor[CursorRight])	cursor[CursorRight]	  = XCreateFontCursor(dpy, XC_right_side);
	if (!cursor[CursorBottomRight])	cursor[CursorBottomRight] = XCreateFontCursor(dpy, XC_bottom_right_corner);
	if (!cursor[CursorBottom])	cursor[CursorBottom]	  = XCreateFontCursor(dpy, XC_bottom_side);
	if (!cursor[CursorBottomLeft])	cursor[CursorBottomLeft]  = XCreateFontCursor(dpy, XC_bottom_left_corner);
	if (!cursor[CursorLeft])	cursor[CursorLeft]	  = XCreateFontCursor(dpy, XC_left_side);
	if (!cursor[CursorEvery])	cursor[CursorEvery]	  = XCreateFontCursor(dpy, XC_fleur);
	if (!cursor[CursorNormal])	cursor[CursorNormal]	  = XCreateFontCursor(dpy, XC_left_ptr);
	/* *INDENT-ON* */

	/* set the normal cursor for the root window of each managed screen */
	if (cursor[CursorNormal]) {
		XSetWindowAttributes wa;

		wa.cursor = cursor[CursorNormal];
		for (scr = screens; scr < screens + nscr; scr++)
			if (scr->managed)
				XChangeWindowAttributes(dpy, scr->root, CWCursor, &wa);
	}
}

void
initmodmap(Bool reload)
{
	const KeyCode numcode = XKeysymToKeycode(dpy, XK_Num_Lock);
	const KeyCode scrcode = XKeysymToKeycode(dpy, XK_Scroll_Lock);
	XModifierKeymap *modmap;
	int i, j;

	/* init modifier map */
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++) {
		for (j = 0; j < modmap->max_keypermod; j++) {
			KeyCode code;

			if ((code = modmap->modifiermap[i * modmap->max_keypermod + j])) {
				if (code == numcode)
					numlockmask = (1 << i);
				if (code == scrcode)
					scrlockmask = (1 << i);
			}
		}
	}
	XFreeModifiermap(modmap);
}

void
initselect(Bool reload)
{
	unsigned long event_mask;

	/* select for events */
	event_mask = ROOTMASK;
	for (scr = screens; scr < screens + nscr; scr++)
		if (scr->managed)
			XSelectInput(dpy, scr->root, event_mask);
}

void
initstartup(Bool reload)
{
#ifdef STARTUP_NOTIFICATION
	if (!sn_dpy)
		sn_dpy = sn_display_new(dpy, NULL, NULL);
#endif
}

void
initstruts(Bool reload)
{
	Monitor *m;

	for (m = scr->monitors; m; m = m->next) {
		m->struts[RightStrut] = m->struts[LeftStrut] =
		    m->struts[TopStrut] = m->struts[BotStrut] = 0;
		updategeom(m);
	}
	ewmh_update_net_work_area();
	ewmh_process_net_showing_desktop();
	ewmh_update_net_showing_desktop();

	arrange(NULL);
}

static void
findscreen(Bool reload)
{
	int d = 0;
	unsigned int mask = 0;
	Window w = None, proot = None;

	/* multihead support */
	XQueryPointer(dpy, screens->root, &proot, &w, &d, &d, &d, &d, &mask);
	XFindContext(dpy, proot, context[ScreenContext], (XPointer *) &scr);
}

static void
initdirs(Bool reload)
{
	const char *env;
	int len;

	free(xdgdirs.home);
	if ((env = getenv("HOME"))) {
		xdgdirs.home = strdup(env);
	} else {
		xdgdirs.home = strdup(".");
	}
	free(xdgdirs.runt);
	if ((env = getenv("XDG_RUNTIME_DIR"))) {
		xdgdirs.runt = strdup(env);
	} else {
		uid_t uid = getuid();
		static char buf[12] = { 0, };

		snprintf(buf, sizeof(buf), "%d", (int)uid);
		len = strlen("/run/user/") + strlen(buf);
		xdgdirs.runt = calloc(len + 1, sizeof(*xdgdirs.runt));
		strncpy(xdgdirs.runt, "/run/user/", len);
		strncat(xdgdirs.runt, buf, len);
	}
	free(xdgdirs.cach);
	if ((env = getenv("XDG_CACHE_HOME"))) {
		xdgdirs.cach = strdup(env);
	} else {
		len = strlen(xdgdirs.home) + strlen("/.cache");
		xdgdirs.cach = calloc(len + 1, sizeof(*xdgdirs.cach));
		strncpy(xdgdirs.cach, xdgdirs.home, len);
		strncat(xdgdirs.cach, "/.cache", len);
	}
	free(xdgdirs.conf.home);
	if ((env = getenv("XDG_CONFIG_HOME"))) {
		xdgdirs.conf.home = strdup(env);
	} else {
		len = strlen(xdgdirs.home) + strlen("/.config");
		xdgdirs.conf.home = calloc(len + 1, sizeof(*xdgdirs.conf.home));
		strncpy(xdgdirs.conf.home, xdgdirs.home, len);
		strncat(xdgdirs.conf.home, "/.config", len);
	}
	free(xdgdirs.conf.dirs);
	if ((env = getenv("XDG_CONFIG_DIRS"))) {
		xdgdirs.conf.dirs = strdup(env);
	} else {
		xdgdirs.conf.dirs = strdup("/etc/xdg");
	}
	free(xdgdirs.data.home);
	if ((env = getenv("XDG_DATA_HOME"))) {
		xdgdirs.data.home = strdup(env);
	} else {
		len = strlen(xdgdirs.home) + strlen("/.local/share");
		xdgdirs.data.home = calloc(len + 1, sizeof(*xdgdirs.data.home));
		strncpy(xdgdirs.data.home, xdgdirs.home, len);
		strncat(xdgdirs.data.home, "/.local/share", len);
	}
	free(xdgdirs.data.dirs);
	if ((env = getenv("XDG_DATA_DIRS"))) {
		xdgdirs.data.dirs = strdup(env);
	} else {
		xdgdirs.data.dirs = strdup("/usr/local/share:/usr/share");
	}
}

void
initialize(const char *conf, AdwmOperations * ops, Bool reload)
{
	char *owd;

	/* save original working directory to restore after processing rc files */
	owd = ecalloc(PATH_MAX, sizeof(*owd));
	if (!getcwd(owd, PATH_MAX))
		strcpy(owd, "/");

	initcursors(reload);	/* init cursors */
	initmodmap(reload);	/* init modifier map */
	initselect(reload);	/* select for events */
	initstartup(reload);	/* init startup notification */

	initdirs(reload);	/* init HOME and XDG directories */
	initrcfile(conf, reload);	/* find the configuration file */
	initrules(reload);	/* initialize window class.name rules */
	initconfig(reload);	/* initialize configuration */

	for (scr = screens; scr < screens + nscr; scr++) {
		if (!scr->managed)
			continue;

		initscreen(reload);	/* init per-screen configuration */
		initewmh(ops->name);	/* init EWMH atoms */
		inittags(reload);	/* init tags */

		if (!reload) {
			initmonitors(NULL);	/* init geometry */
			/* I think that we can do this all the time. */
		}

		initkeys(reload);	/* init key bindings */
		initdock(reload);	/* initialize dock */
		initlayouts(reload);	/* initialize layouts */

		grabkeys();

		initstyle(reload);	/* init appearance */
		initstruts(reload);	/* initialize struts and workareas */
	}

	findscreen(reload);	/* find current screen (with pointer) */

	if (owd) {
		if (chdir(owd))
			DPRINTF("Could not change directory to %s: %s\n", owd,
				strerror(errno));
		free(owd);
	}
}

void
reload(void)
{
	return initialize(NULL, baseops, True);
}

void
setup(char *conf, AdwmOperations *ops)
{
	return initialize(conf, ops, False);
}

void
spawn(const char *arg)
{
	wordexp_t we = { 0, };
	int status;

	if (!arg)
		return;
	if ((status = wordexp(arg, &we, 0)) != 0 || we.we_wordc < 1) {
		switch(status) {
		case WRDE_BADCHAR:
			EPRINTF("bad character in command string: %s\n", arg);
			break;
		case WRDE_BADVAL:
			EPRINTF("undefined variable substitution in command string: %s\n", arg);
			break;
		case WRDE_CMDSUB:
			EPRINTF("command substitution in command string: %s\n", arg);
			break;
		case WRDE_NOSPACE:
			EPRINTF("out of memory processing command string: %s\n", arg);
			break;
		case WRDE_SYNTAX:
			EPRINTF("syntax error in command string: %s\n", arg);
			break;
		default:
			EPRINTF("unknown error processing command string: %s\n", arg);
			break;
		}
		wordfree(&we); /* necessary ??? */
		return;
	}
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
		execvp(we.we_wordv[0], we.we_wordv);
		EPRINTF("execvp %s (%s) failed: %s\n", we.we_wordv[0], arg, strerror(errno));
		abort();
	} else {
		wordfree(&we);
	}
}

void
togglestruts(View *v)
{
	if (v->barpos != StrutsOn)
		v->barpos = StrutsOn;
	else {
		if (scr->options.hidebastards == 2) {
			v->barpos = StrutsDown;
			DPRINTF("Setting struts to StrutsDown\n");
		} else if (scr->options.hidebastards) {
			v->barpos = StrutsHide;
			DPRINTF("Setting struts to StrutsHide\n");
		} else {
			v->barpos = StrutsOff;
			DPRINTF("Setting struts to StrutsOff\n");
		}
	}
	updategeom(v->curmon);
	arrange(v);
}

void
togglemin(Client *c)
{
	if (!c || (!c->can.min && c->is.managed))
		return;
	if (c->is.icon) {
		deiconify(c);
		c->is.icon = False;
		if (c->is.managed && !c->is.hidden) {
			focus(c);
			arrange(clientview(c));
		}
	} else
		iconify(c);
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
	/* FIXME: this should actually be the last selected client in the view on the monitor to which
	 * we are switching. */
	focus(NULL);
}

void
unmanage(Client *c, WithdrawCause cause)
{
	XWindowChanges wc = { 0, };
	Bool doarrange, dostruts;
	Window *w, trans = None;

	doarrange = !(c->skip.arrange || c->is.floater ||
		      (cause != CauseDestroyed &&
		       XGetTransientForHint(dpy, c->win, &trans))) ||
	    c->is.bastard || c->is.dockapp;
	dostruts = c->with.struts;
	/* The server grab construct avoids race conditions. */
	XGrabServer(dpy);
	XSelectInput(dpy, c->frame, NoEventMask);
	XUnmapWindow(dpy, c->frame);
	XSetErrorHandler(xerrordummy);
	c->can.focus = 0;
	if (relfocus(c))
		focus(sel);
	c->is.managed = False;
	if (c->is.modal)
		togglemodal(c);
#ifdef SYNC
	if (c->sync.alarm) {
		c->sync.waiting = False;
		XSyncDestroyAlarm(dpy, c->sync.alarm);
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
	if ((w = c->cmapwins)) {
		while (*w)
			XDeleteContext(dpy, *w++, context[ClientColormap]);
		free(c->cmapwins);
		c->cmapwins = NULL;
	}
	if (c->cmap) {
		XFreeColormap(dpy, c->cmap);
		c->cmap = None;
	}
	if (c->cview && c->cview->lastsel == c)
		c->cview->lastsel = NULL;
	if (cause != CauseDestroyed) {
		if (einfo[XfixesBase].have)
			XFixesChangeSaveSet(dpy, c->icon ? c->icon : c->win, SetModeDelete, SaveSetNearest, SaveSetMap);
		else
			XChangeSaveSet(dpy, c->icon ? c->icon : c->win, SetModeDelete);
		XSelectInput(dpy, c->win, CLIENTMASK & ~MAPPINGMASK);
		XUngrabButton(dpy, Button1, AnyModifier, c->win);
		XUngrabButton(dpy, Button2, AnyModifier, c->win);
		XUngrabButton(dpy, Button3, AnyModifier, c->win);
		if (c->icon && c->icon != c->win) {
			XSelectInput(dpy, c->icon, CLIENTMASK & ~MAPPINGMASK);
			XUngrabButton(dpy, Button1, AnyModifier, c->icon);
			XUngrabButton(dpy, Button2, AnyModifier, c->icon);
			XUngrabButton(dpy, Button3, AnyModifier, c->icon);
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
				XConfigureWindow(dpy, c->icon,
						 (CWX | CWY | CWWidth | CWHeight |
						  CWBorderWidth), &wc);
			} else {
				XReparentWindow(dpy, c->win, scr->root, wc.x, wc.y);
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
	removegroup(c, c->session, ClientSession);
	if (c->is.grptrans)
		removegroup(c, c->transfor, ClientTransForGroup);
	else if (c->is.transient)
		removegroup(c, c->transfor, ClientTransFor);
#ifdef STARTUP_NOTIFICATION
	if (c->seq) {
		sn_startup_sequence_unref(c->seq);
		c->seq = NULL;
	}
#endif
	free(c->name);
	free(c->icon_name);
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
	int t = 0, l = 0, b = 0, r = 0;

	if (m->curview->barpos != StrutsOff) {
		l = m->struts[LeftStrut];
		t = m->struts[TopStrut];
		r = m->struts[RightStrut];
		b = m->struts[BotStrut];
	}

	m->wa = m->sc;

	if (m->curview->barpos == StrutsHide) {
		switch (m->dock.position) {
		case DockNone:
			break;
		case DockEast:
			r = max(1, r);
			break;
		case DockWest:
			l = max(1, l);
			break;
		case DockNorth:
			t = max(1, t);
			break;
		case DockSouth:
			b = max(1, b);
			break;
		case DockNorthEast:
			if (m->dock.orient == DockHorz)
				t = max(1, t);
			else
				r = max(1, r);
			break;
		case DockNorthWest:
			if (m->dock.orient == DockHorz)
				t = max(1, t);
			else
				l = max(1, l);
			break;
		case DockSouthWest:
			if (m->dock.orient == DockHorz)
				b = max(1, b);
			else
				l = max(1, l);
			break;
		case DockSouthEast:
			if (m->dock.orient == DockHorz)
				b = max(1, b);
			else
				r = max(1, r);
			break;
		}
		l = min(1, l);
		t = min(1, t);
		r = min(1, r);
		b = min(1, b);
	}

	m->wa.x += l;
	m->wa.y += t;
	m->wa.w -= l + r;
	m->wa.h -= t + b;

	XMoveResizeWindow(dpy, m->veil,
			m->wa.x + scr->style.border,
			m->wa.y + scr->style.border,
			m->wa.w - 2 * scr->style.border,
			m->wa.h - 2 * scr->style.border);
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

	if ((c = getclient(ev->window, ClientWindow))) {
		CPRINTF(c, "self-unmapped window\n");
		if (ev->send_event) {
			/* synthetic */
			if (ev->event == event_scr->root) {
				CPRINTF(c, "unmanage self-unmapped window (synthetic)\n");
				unmanage(c, CauseUnmapped);
				return True;
			}
		} else {
			/* real event */
			if (ev->event == c->frame && c->is.managed) {
				CPRINTF(c, "unmanage self-unmapped window (real event)\n");
				unmanage(c, CauseUnmapped);
				return True;
			}
		}
	}
	return False;
}

void
updatehints(Client *c)
{
	XWMHints *wmh;
	Window leader;
	int take_focus, give_focus;

	take_focus =
	    ((c->icon && checkatom(c->icon, _XA_WM_PROTOCOLS, _XA_WM_TAKE_FOCUS)) ||
	                 checkatom(c->win, _XA_WM_PROTOCOLS, _XA_WM_TAKE_FOCUS))
	    ?  TAKE_FOCUS : 0;
	give_focus = c->is.dockapp ? 0 : GIVE_FOCUS;
	c->can.focus = take_focus | give_focus;

	if ((wmh = XGetWMHints(dpy, c->win))) {

		if (wmh->flags & XUrgencyHint && !c->is.attn) {
			c->is.attn = True;
			if (c->is.managed)
				ewmh_update_net_window_state(c);
		}
		if (wmh->flags & InputHint)
			c->can.focus = take_focus | (wmh->input ? GIVE_FOCUS : 0);
		if (wmh->flags & WindowGroupHint) {
			leader = wmh->window_group;
			if (c->leader != leader) {
				removegroup(c, c->leader, ClientGroup);
				c->leader = leader;
				updategroup(c, c->leader, ClientGroup, &c->nonmodal);
			}
		}
		XFree(wmh);
	}
}

void
updatesizehints(Client *c)
{
	long *sh, msize = 0;
	XSizeHints size = { 0, };
	unsigned long n = 0;

#if 0
	if (!XGetWMNormalHints(dpy, c->win, &size, &msize) || !size.flags)
		size.flags = PSize;
#else
	if (!c->icon || !(sh = gethints(c->icon, XA_WM_NORMAL_HINTS, &n)))
		sh = gethints(c->win, XA_WM_NORMAL_HINTS, &n);
	if (sh) {
		//  1 flags
		size.flags = sh[0];
		CPRINTF(c, "got %lu words of size hints 0x%lx\n", n, size.flags);
		//  2 x
		//  3 y
		if (n >= 3) {
			msize |= (USPosition|PPosition);
			if (size.flags & (USPosition|PPosition)) {
				size.x = sh[1];
				size.y = sh[2];
			}
		}
		//  4 width
		//  5 height
		if (n >= 5) {
			msize |= (USSize|PSize);
			if (size.flags & (USSize|PSize)) {
				size.width = sh[3];
				size.height = sh[4];
			}
		}
		//  6 min_width
		//  7 min_height
		if (n >= 7) {
			msize |= (PMinSize);
			if (size.flags & (PMinSize)) {
				size.min_width = sh[5];
				size.min_height = sh[6];
			}
		}
		//  8 max_width
		//  9 max_height
		if (n >= 9) {
			msize |= (PMaxSize);
			if (size.flags & (PMaxSize)) {
				size.max_width = sh[7];
				size.max_height = sh[8];
			}
		}
		//  10 width_inc
		//  11 height_inc
		if (n >= 11) {
			msize |= (PResizeInc);
			if (size.flags & (PResizeInc)) {
				size.width_inc = sh[9];
				size.height_inc = sh[10];
			}
		}
		// 12 min_aspect.x
		// 13 min_aspect.y
		// 14 max_aspect.x
		// 15 max_aspect.y
		if (n >= 15) {
			msize |= (PAspect);
			if (size.flags & (PAspect)) {
				size.min_aspect.x = sh[11];
				size.min_aspect.y = sh[12];
				size.max_aspect.x = sh[13];
				size.max_aspect.y = sh[14];
			}
		}
		// 16 base_width
		// 17 base_height
		if (n >= 17) {
			msize |= (PBaseSize);
			if (size.flags & (PBaseSize)) {
				size.base_width = sh[15];
				size.base_height = sh[16];
			}
		}
		// 18 win_gravity
		if (n >= 18) {
			msize |= (PWinGravity);
			if (size.flags & (PWinGravity)) {
				size.win_gravity = sh[17];
			}
		}
		size.flags &= msize;
	}
#endif
	c->flags = size.flags;

	if (c->flags & (USPosition|PPosition)) {
		if (size.x || size.y) {
			c->u.x = size.x;
			c->u.y = size.y;
		}
	}
	if (c->flags & (USSize|PSize)) {
		if (size.width && size.height) {
			c->u.w = size.width;
			c->u.h = size.height;
		}
	}
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
	if ((!c->icon || !gettextprop(c->icon, _XA_NET_WM_NAME, &c->name) || !c->name) &&
	    (!c->icon || !gettextprop(c->icon, XA_WM_NAME, &c->name) || !c->name) &&
	    (!gettextprop(c->win, _XA_NET_WM_NAME, &c->name) || !c->name) &&
	    (!gettextprop(c->win, XA_WM_NAME, &c->name)))
		return;
	ewmh_update_net_window_visible_name(c);
}

void
updateiconname(Client *c)
{
	if ((!c->icon || !gettextprop(c->icon, _XA_NET_WM_ICON_NAME, &c->icon_name) || !c->icon_name) &&
	    (!c->icon || !gettextprop(c->icon, XA_WM_ICON_NAME, &c->icon_name) || !c->icon_name) &&
	    (!gettextprop(c->win, _XA_NET_WM_ICON_NAME, &c->icon_name) || !c->icon_name) &&
	    (!gettextprop(c->win, XA_WM_ICON_NAME, &c->icon_name)))
		return;
	ewmh_update_net_window_visible_icon_name(c);
}

void
updatetransientfor(Client *c)
{
	Window trans;

	if (XGetTransientForHint(dpy, c->win, &trans) && trans == None)
		trans = scr->root;
	if (!c->is.floater && (c->is.floater = (trans != None))) {
		arrange(NULL);
		ewmh_update_net_window_state(c);
	}
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
	if (nonmodal)
		*nonmodal += g->modal_transients;
}

Window *
getgroup(Window leader, int group, unsigned int *count)
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
	unsigned int i;

	g->modal_transients++;
	for (i = 0; i < g->count; i++)
		if ((t = getclient(g->members[i], ClientWindow)) && t != c)
			t->nonmodal++;
}

void
decmodal(Client *c, Group *g)
{
	Client *t;
	unsigned int i;

	--g->modal_transients;
	assert(g->modal_transients >= 0);
	for (i = 0; i < g->count; i++)
		if ((t = getclient(g->members[i], ClientWindow)) && t != c) {
			t->nonmodal--;
			assert(t->nonmodal >= 0);
		}
}

void
updatesession(Client *c)
{
	Window win;

	win = getrecwin(c, _XA_WM_CLIENT_LEADER);
	if (win == c->session)
		return;
	removegroup(c, c->session, ClientSession);
	if (win == None)
		return;

	c->session = win;
	updategroup(c, c->session, ClientSession, NULL);
	if (win != c->win) {
		XSelectInput(dpy, win, PropertyChangeMask);
	}
}

void
updatecmapwins(Client *c)
{
	Window *p;

	if (c->cmapwins) {
		for (p = c->cmapwins; p && *p; p++) {
			XDeleteContext(dpy, *p, context[ClientColormap]);
		}
		free(c->cmapwins);
		c->cmapwins = NULL;
	}
	if ((c->cmapwins = getcmapwins(c->win))) {
		for (p = c->cmapwins; p && *p; p++) {
			XSaveContext(dpy, *p, context[ClientColormap], (XPointer) c);
		}
	}
}

void
ignorenext(void)
{
	ignore_request = NextRequest(dpy);
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (ebastardly on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.	*/
int
xerror(Display *dsply, XErrorEvent *ee)
{
	Bool dead = True;
	char msg[81] = { 0, }, req[81] = { 0, }, num[81] = { 0, };

	if (ignore_request && ee->serial == ignore_request) {
		ignore_request = 0;
		return (0);
	}
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
		dead = False;
	snprintf(num, 80, "%d", ee->request_code);
	XGetErrorDatabaseText(dsply, "XRequest", num, "", req, 80);
	if (!req[0])
		snprintf(req, 80, "[request_code=%d]", ee->request_code);
	if (XGetErrorText(dsply, ee->error_code, msg, 80) != Success)
		msg[0] = '\0';
	if (!dead) {
		EPRINTF("X error %s(0x%lx): %s\n", req, ee->resourceid, msg);
		return 0;
	}
	EPRINTF("Fatal X error %s(0x%lx): %s\n", req, ee->resourceid, msg);
	dumpstack(__FILE__, __LINE__, __func__);
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

int
xioerror(Display *dsply)
{
	EPRINTF("error is %s\n", strerror(errno));
	dumpstack(__FILE__, __LINE__, __func__);
	return xioerrorxlib(dsply);
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

int
main(int argc, char *argv[])
{
	char conf[256] = "", *p;
	int i;

	if (argc == 3 && !strcmp("-f", argv[1]))
		snprintf(conf, sizeof(conf), "%s", argv[2]);
	else if (argc == 2 && !strcmp("-v", argv[1])) {
		fprintf(stdout, "adwm-" VERSION " (c) 2016 Brian Bidulock\n");
		exit(0);
	} else if (argc != 1)
		eprint("%s", "usage: adwm [-v] [-f conf]\n");

	setlocale(LC_CTYPE, "");
	if (!(dpy = XOpenDisplay(0)))
		eprint("%s", "adwm: cannot open display\n");
	signal(SIGHUP, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGCHLD, sighandler);
	signal(SIGSEGV, sighandler);
	signal(SIGBUS, sighandler);
	cargc = argc;
	cargv = argv;

	if (!(baseops = get_adwm_ops("adwm")))
		eprint("%s", "could not load base operations\n");

	/* create contexts */
	for (i = 0; i < PartLast; i++)
		context[i] = XUniqueContext();
	/* query extensions */
	for (i = 0; i < BaseLast; i++) {
		einfo[i].have = XQueryExtension(dpy, einfo[i].name, &einfo[i].opcode, &einfo[i].event, &einfo[i].error);
		if (einfo[i].have) {
			DPRINTF("have %s extension (%d,%d,%d)\n", einfo[i].name, einfo[i].opcode, einfo[i].event, einfo[i].error);
			if (einfo[i].version) {
				einfo[i].version(dpy, &einfo[i].major, &einfo[i].minor);
				OPRINTF("have %-10s extension version %d.%d\n", einfo[i].name, einfo[i].major, einfo[i].minor);
			}
		} else
			DPRINTF("%s", "%s extension is not supported\n", einfo[i].name);
	}
	nscr = ScreenCount(dpy);
	DPRINTF("there are %u screens\n", nscr);
	screens = calloc(nscr, sizeof(*screens));
	for (i = 0, scr = screens; i < nscr; i++, scr++) {
		scr->screen = i;
		scr->root = RootWindow(dpy, i);
		XSaveContext(dpy, scr->root, context[ScreenContext], (XPointer) scr);
		DPRINTF("screen %d has root 0x%lx\n", scr->screen, scr->root);
		initimage();
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
		eprint("%s", "adwm: another window manager is already running on each screen\n");
	setup(conf, baseops);
	for (scr = screens; scr < screens + nscr; scr++)
		if (scr->managed) {
			DPRINTF("scanning screen %d\n", scr->screen);
			scan();
		}
	DPRINTF("%s", "entering main event loop\n");
	run();
	DPRINTF("cleanup quitting\n");
	cleanup(CauseQuitting);

	XCloseDisplay(dpy);
	return 0;
}
