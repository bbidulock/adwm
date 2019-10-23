/* See COPYING file for copyright and license details.  */

#include "adwm.h"
#include "draw.h"
#include "ewmh.h"
#include "layout.h"
#include "parse.h"
#include "buttons.h"
#include "tags.h"
#include "actions.h"
#include "config.h"
#include "image.h"
#include "icons.h"

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
void updateclasshint(Client *c);
void updatehints(Client *c);
void updatesizehints(Client *c);
void updatetitle(Client *c);
void updatetransientfor(Client *c);
void updatesession(Client *c);
void updategroup(Client *c, Window leader, int group, int *nonmodal);
Window *getgroup(Window leader, int group, unsigned int *count);
void removegroup(Client *c, Window leader, int group);
#ifdef STARTUP_NOTIFICATION
Appl *getappl(Client *c);
void updateappl(Client *c);
void removeappl(Client *c);
#endif
Class *getclass(Client *c);
void updateclass(Client *c);
void removeclass(Client *c);
void updateiconname(Client *c);
void updatecmapwins(Client *c);
int xerror(Display *dpy, XErrorEvent *ee);
int xerrordummy(Display *dsply, XErrorEvent *ee);
int xerrorstart(Display *dsply, XErrorEvent *ee);
int (*xerrorxlib) (Display *, XErrorEvent *);
int xioerror(Display *dpy);
int (*xioerrorxlib) (Display *);

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
XErrorTrap *traps = NULL;
XrmDatabase xrdb = NULL;
XrmDatabase srdb = NULL;
Bool otherwm;
Bool running = True;
Bool lockfocus = False;
unsigned long focus_request = 0;
Appl *appls = NULL;
Class *classes = NULL;
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

char *clientId = NULL;

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
		[Button1-Button1] =	{ m_move,	    m_restack	    },
		[Button2-Button1] =	{ m_move,	    m_zoom	    },
		[Button3-Button1] =	{ m_resize,	    m_restack	    },
		[Button4-Button1] =	{ m_shade,	    NULL	    },
		[Button5-Button1] =	{ m_unshade,	    NULL	    },
	},
	[OnClientGrips]  = {
		[Button1-Button1] =	{ m_resize,	    m_restack	    },
		[Button2-Button1] =	{ NULL,		    NULL	    },
		[Button3-Button1] =	{ NULL,		    NULL	    },
		[Button4-Button1] =	{ NULL,		    NULL	    },
		[Button5-Button1] =	{ NULL,		    NULL	    },
	},
	[OnClientFrame]	 = {
		[Button1-Button1] =	{ m_resize,	    m_restack	    },
		[Button2-Button1] =	{ NULL,		    NULL	    },
		[Button3-Button1] =	{ NULL,		    NULL	    },
		[Button4-Button1] =	{ m_shade,	    NULL	    },
		[Button5-Button1] =	{ m_unshade,	    NULL	    },
	},
	[OnClientDock]   = {
		[Button1-Button1] =	{ m_move,	    m_restack	    },
		[Button2-Button1] =	{ NULL,		    NULL	    },
		[Button3-Button1] =	{ NULL,		    NULL	    },
		[Button4-Button1] =	{ NULL,		    NULL	    },
		[Button5-Button1] =	{ NULL,		    NULL	    },
	},
	[OnClientWindow] = {
		[Button1-Button1] =	{ m_move,	    m_restack	    },
		[Button2-Button1] =	{ m_move,	    m_zoom	    },
		[Button3-Button1] =	{ m_resize,	    m_restack	    },
		[Button4-Button1] =	{ m_shade,	    NULL	    },
		[Button5-Button1] =	{ m_unshade,	    NULL	    },
	},
	[OnClientIcon] = {
		[Button1-Button1] =	{ m_move,	    m_restack	    },
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
#ifdef DAMAGE
static Bool damagenotify(XEvent *e);
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
	[XFixesSelectionNotify + LASTEvent + EXTRANGE * XfixesBase] = IGNOREEVENT,
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
	[XDamageNotify + LASTEvent + EXTRANGE * XdamageBase] = damagenotify,
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
	View *cv = c->cview ? : selview();
	Monitor *cm = (cv && cv->curmon) ? cv->curmon : nearmonitor(); /* XXX: necessary? */

	/* rule matching */
	snprintf(buf, sizeof(buf), "%s:%s:%s",
		 c->ch.res_class ? c->ch.res_class : "", c->ch.res_name ? c->ch.res_name : "",
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
	if (!matched && cm)
		c->tags = (1ULL << cm->curview->index);
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
applywmhints(Client *c, XWMHints *wmh)
{
	if (!wmh || !c)
		return;
	c->wmh = *wmh;
	if (!(c->wmh.flags & InputHint))
		c->wmh.input = False;
	if (!(c->wmh.flags & StateHint))
		c->wmh.initial_state = NormalState;
	if (!(c->wmh.flags & IconPixmapHint))
		c->wmh.icon_pixmap = None;
	if (!(c->wmh.flags & IconWindowHint))
		c->wmh.icon_window = None;
	if (!(c->wmh.flags & IconPositionHint))
		c->wmh.icon_x = c->wmh.icon_y = 0;
	if (!(c->wmh.flags & IconMaskHint))
		c->wmh.icon_mask = None;
	if (!(c->wmh.flags & WindowGroupHint))
		c->wmh.window_group = None;
}

static void
applystate(Client *c)
{
	int state = NormalState;

	{
		XWMHints *wmh;

		/* ICCCM 2.0/4.1.9: Window managers will ignore any WM_HINTS properties
		   they find on icon windows. */
		if ((wmh = XGetWMHints(dpy, c->win))) {
			applywmhints(c, wmh);
			XFree(wmh);
		}
	}
	if (c->wmh.flags & StateHint)
		state = c->wmh.initial_state;

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
		c->icon = ((c->wmh.flags & IconWindowHint) && c->wmh.icon_window) ?  c->wmh.icon_window : c->win;
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
		setwmstate(c->win, state, c->is.dockapp ? c->frame : None);
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
		XEvent ev;

		c->is.banned = True;
		relfocus(c);
		XUnmapWindow(dpy, c->frame);
		XUnmapWindow(dpy, c->win);
		if (c->is.dockapp && c->icon && c->icon != c->win)
			XUnmapWindow(dpy, c->icon);
		XSync(dpy, False);
		XCheckIfEvent(dpy, &ev, &check_unmapnotify, (XPointer) c);
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
			} else if (WTCHECK(c, WindowTypeDock)) {
				if (!sel || sel != c)
					hide = True;
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
	Bool (*action) (Client *, XEvent *), result;
	int button, direct, state;

	if (Button1 > ev->button || ev->button > Button5)
		return False;
	button = ev->button - Button1;
	direct = (ev->type == ButtonPress) ? 0 : 1;
	state = ev->state &
	    ~(Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask);

	pushtime(ev->time);

	XPRINTF("BUTTON %d: window 0x%lx root 0x%lx subwindow 0x%lx time %ld x %d y %d x_root %d y_root %d state 0x%x\n",
	     ev->button, ev->window, ev->root, ev->subwindow, ev->time, ev->x, ev->y,
	     ev->x_root, ev->y_root, ev->state);

	if (ev->type == ButtonRelease && (button_proxy & (1 << button))) {
		/* if we proxied the press, we must proxy the release too */
		XPRINTF("BUTTON %d: passing to button proxy, state = 0x%08x\n",
			button + Button1, state);
		XSendEvent(dpy, scr->selwin, False, SubstructureNotifyMask, e);
		button_proxy &= ~(1 << button);
		return True;
	}

	if (ev->window == scr->root) {
		XPRINTF("SCREEN %d: 0x%lx button: %d\n", scr->screen, ev->window,
			ev->button);
		/* _WIN_DESKTOP_BUTTON_PROXY */
		/* modifiers or not interested in press or release */
		if (ev->type == ButtonPress) {
			if (state || (!actions[OnRoot][button][0] &&
				      !actions[OnRoot][button][1])) {
				XPRINTF("BUTTON %d: passing to button proxy, state = 0x%08x\n",
				     button + Button1, state);
				XUngrabPointer(dpy, ev->time);
				XSendEvent(dpy, scr->selwin, False,
					   SubstructureNotifyMask, e);
				button_proxy |= ~(1 << button);
				return True;
			}
		}
		if ((action = actions[OnRoot][button][direct])) {
			XPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
				OnRoot, button + Button1, direct);
			result = (*action) (NULL, (XEvent *) ev);
			/* perform release action if pressed action failed */
			if (!result && !direct && (action = actions[OnRoot][button][1])) {
				XPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
					OnRoot, button + Button1, 1);
				result = (*action) (NULL, (XEvent *) ev);
			}
		} else
			XPRINTF("No action for On=%d, button=%d, direct=%d\n", OnRoot,
				button + Button1, direct);
		XUngrabPointer(dpy, ev->time);
	} else if ((c = getmanaged(ev->window, ClientTitle))) {
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
			if (ev->x >= ec->eg.x && ev->x < ec->eg.x + ec->eg.w
			    && ev->y >= ec->eg.y && ev->y < ec->eg.y + ec->eg.h) {
				if (ev->type == ButtonPress) {
					XPRINTF("ELEMENT %d PRESSED\n", i);
					ec->pressed |= (1 << button);
					drawclient(c); /* just for button */
					/* resize needs to be on button press */
					if (action) {
						if (!(result = (*action) (c, (XEvent *) ev))) {
							XPRINTF("ELEMENT %d RELEASED\n", i);
							ec->pressed &= ~(1 << button);
							drawclient(c); /* just for button */
							/* perform release action if pressed action failed */
							if ((action = scr->element[i].action[button * 2 + 1]))
								(*action) (c, (XEvent *) ev);
						}
						drawclient(c); /* just for button */
					}
				} else if (ev->type == ButtonRelease) {
					/* only process release if processed press */
					if (ec->pressed & (1 << button)) {
						XPRINTF("ELEMENT %d RELEASED\n", i);
						ec->pressed &= ~(1 << button);
						drawclient(c); /* just for button */
						/* resize needs to be on button press */
						if (action) {
							(*action) (c, (XEvent *) ev);
							drawclient(c); /* just for button */
						}
					}
				}
				if (active)
					return True;
			} else {
				ec->pressed &= ~(1 << button);
				drawclient(c); /* just for button */
			}
		}
		if ((action = actions[OnClientTitle][button][direct])) {
			XPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
				OnClientTitle, button + Button1, direct);
			result = (*action) (c, (XEvent *) ev);
			/* perform release action if pressed action failed */
			if (!result && !direct && (action = actions[OnClientTitle][button][1])) {
				XPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
					OnClientTitle, button + Button1, 1);
				result = (*action) (NULL, (XEvent *) ev);
			}
		} else
			XPRINTF("No action for On=%d, button=%d, direct=%d\n",
				OnClientTitle, button + Button1, direct);
		XUngrabPointer(dpy, ev->time);
		drawclient(c); /* just for button */
	} else if ((c = getmanaged(ev->window, ClientGrips))) {
		if ((action = actions[OnClientGrips][button][direct])) {
			XPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
				ClientGrips, button + Button1, direct);
			result = (*action) (c, (XEvent *) ev);
			/* perform release action if pressed action failed */
			if (!result && !direct && (action = actions[OnClientGrips][button][1])) {
				XPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
					OnClientGrips, button + Button1, 1);
				result = (*action) (NULL, (XEvent *) ev);
			}
		} else
			XPRINTF("No action for On=%d, button=%d, direct=%d\n",
				ClientGrips, button + Button1, direct);
		XUngrabPointer(dpy, ev->time);
		drawclient(c); /* just for button */
	} else if ((c = getmanaged(ev->window, ClientIcon))) {
		XPRINTF("ICON %s: 0x%lx button: %d\n", c->name, ev->window, ev->button);
		if ((CLEANMASK(ev->state) & modkey) != modkey) {
			XAllowEvents(dpy, ReplayPointer, ev->time);
			return True;
		}
		if ((action = actions[OnClientIcon][button][direct])) {
			XPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
				ClientIcon, button + Button1, direct);
			result = (*action) (c, (XEvent *) ev);
			/* perform release action if pressed action failed */
			if (!result && !direct && (action = actions[OnClientIcon][button][1])) {
				XPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
					OnClientIcon, button + Button1, 1);
				result = (*action) (NULL, (XEvent *) ev);
			}
		} else
			XPRINTF("No action for On=%d, button=%d, direct=%d\n", ClientIcon,
				button + Button1, direct);
		XUngrabPointer(dpy, ev->time);
		drawclient(c); /* just for button */
	} else if ((c = getmanaged(ev->window, ClientWindow))) {
		XPRINTF("WINDOW %s: 0x%lx button: %d\n", c->name, ev->window, ev->button);
		if ((CLEANMASK(ev->state) & modkey) != modkey) {
			XAllowEvents(dpy, ReplayPointer, ev->time);
			return True;
		}
		if ((action = actions[OnClientWindow][button][direct])) {
			XPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
				OnClientWindow, button + Button1, direct);
			result = (*action) (c, (XEvent *) ev);
			/* perform release action if pressed action failed */
			if (!result && !direct && (action = actions[OnClientWindow][button][1])) {
				XPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
					OnClientWindow, button + Button1, 1);
				result = (*action) (NULL, (XEvent *) ev);
			}
		} else
			XPRINTF("No action for On=%d, button=%d, direct=%d\n",
				OnClientWindow, button + Button1, direct);
		XUngrabPointer(dpy, ev->time);
		drawclient(c); /* just for button */
	} else if ((c = getmanaged(ev->window, ClientFrame))) {
		XPRINTF("FRAME %s: 0x%lx button: %d\n", c->name, ev->window, ev->button);
		if (c->is.dockapp) {
			if ((action = actions[OnClientDock][button][direct])) {
				XPRINTF("Action %p for On=%d, button=%d, direct=%d\n",
					action, OnClientDock, button + Button1, direct);
				result = (*action) (c, (XEvent *) ev);
				/* perform release action if pressed action failed */
				if (!result && !direct && (action = actions[OnClientDock][button][1])) {
					XPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
						OnClientDock, button + Button1, 1);
					result = (*action) (NULL, (XEvent *) ev);
				}
			} else
				XPRINTF("No action for On=%d, button=%d, direct=%d\n",
					OnClientDock, button + Button1, direct);
		} else {
			if ((action = actions[OnClientFrame][button][direct])) {
				XPRINTF("Action %p for On=%d, button=%d, direct=%d\n",
					action, OnClientFrame, button + Button1, direct);
				result = (*action) (c, (XEvent *) ev);
				/* perform release action if pressed action failed */
				if (!result && !direct && (action = actions[OnClientFrame][button][1])) {
					XPRINTF("Action %p for On=%d, button=%d, direct=%d\n", action,
						OnClientFrame, button + Button1, 1);
					result = (*action) (NULL, (XEvent *) ev);
				}
			} else
				XPRINTF("No action for On=%d, button=%d, direct=%d\n",
					OnClientFrame, button + Button1, direct);
		}
		XUngrabPointer(dpy, ev->time);
		drawclient(c); /* just for button */
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
			XPRINTF("unmanage cleanup\n");
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
getclientgeometry(Client *c, Geometry *g, Geometry *x)
{
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

	if ((c = getmanaged(ev->window, ClientWindow))) {
		XPRINTF("calling configureclient for ConfigureReqeust event\n");
		configureclient(c, e, c->sh.win_gravity);
	} else
	if ((c = getmanaged(ev->window, ClientAny)))
		EPRINTF(__CFMTS(c) "trying to change config of non-client window 0x%lx\n", __CARGS(c), ev->window);
	if (!c) {
		XWindowChanges wc = { 0, };

		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		xtrap_push(1,_WCFMTS(wc, ev->value_mask) "configuring unmanaged window 0x%lx\n", _WCARGS(wc, ev->value_mask), ev->window);
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
		xtrap_pop();
		XSync(dpy, False);
	}
	return True;
}

static Bool
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = getmanaged(ev->window, ClientWindow))) {
		XPRINTF(c, "unmanage destroyed window\n");
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

	if ((c = getmanaged(ev->window, ClientTitle))) {
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
			if (ev->x >= ec->eg.x && ev->x < ec->eg.x + ec->eg.w &&
			    ev->y >= ec->eg.y && ev->y < ec->eg.y + ec->eg.h) {
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
			drawclient(c); /* just for button */
	}
	if (c || (c = findmanaged(ev->window))) {
		if (c->cview && c->cview->strut_time)
			motionclient(e, c);
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
		if (i == num) {
			xtrap_push(1, "Installing colormaps");
			XInstallColormap(dpy, wa.colormap);
			xtrap_pop();
		}
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

void
focuslockclient(Client *c)
{
	lockfocus = True;
	focus_request = NextRequest(dpy);
	XSync(dpy, False);
	focuslock = c ? : gave ? : took ? : sel;
}

Bool
checkfocuslock(Client *c, unsigned long serial)
{
	if (lockfocus) {
		if (serial > focus_request) {
			lockfocus = False;
			focus_request = 0;
		} else if (c && c == focuslock)
			return (False);
	}
	return (lockfocus);
}

static Bool
enternotify(XEvent *e)
{
	XCrossingEvent *ev = &e->xcrossing;
	Client *c;

	if (e->type != EnterNotify || ev->mode != NotifyNormal)
		return False;

	if ((c = findmanaged(ev->window))) {
		if (ev->detail == NotifyInferior)
			return False;
		XPRINTF(c, "EnterNotify received\n");
		enterclient(e, c);
		if (c != sel)
			installcolormaps(event_scr, c, c->cmapwins);
		return True;
	} else if ((c = getmanaged(ev->window, ClientColormap))) {
		installcolormap(event_scr, ev->window);
		return True;
	} else if (ev->window == event_scr->root && ev->detail != NotifyInferior) {
		XPRINTF("Not focusing root\n");
		if (sel && (WTCHECK(sel, WindowTypeDock) || sel->is.dockapp || sel->is.bastard)) {
			focus(NULL);
			return True;
		}
		return False;
	} else {
		XPRINTF("Unknown entered window 0x%08lx\n", ev->window);
		return False;
	}
}

static Bool
colormapnotify(XEvent *e)
{
	XColormapEvent *ev = &e->xcolormap;
	Client *c;

	if ((c = getmanaged(ev->window, ClientColormap))) {
		if (ev->new) {
			if (c == sel)
				installcolormaps(event_scr, c, c->cmapwins);
			return True;
		} else {
			switch (ev->state) {
			case ColormapInstalled:
				return True;
			case ColormapUninstalled:
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

	if ((c = getmanaged(ev->window, ClientAny))) {
		XEvent tmp;

		XSync(dpy, False);
		/* discard all exposures for the same window */
		while (XCheckWindowEvent(dpy, ev->window, ExposureMask, &tmp)) ;
		/* might not be necessary any more... */
		/* maybe we should just watch for damage on the backing pixmap and render 
		   only the damage region. */
		drawclient(c);	/* just for exposure */
		return True;
	}
	return False;
}

#ifdef DAMAGE
static Bool
damagenotify(XEvent *e)
{
	XDamageNotifyEvent *ev = (typeof(ev)) e;
	Client *c;

	if ((c = getmanaged(ev->drawable, ClientWindow))) {
		XPRINTF(c, "Got damage notify, redrawing damage \n");
		return drawdamage(c, ev);
	}
	return False;
}
#endif

#ifdef SHAPE
static Bool
shapenotify(XEvent *e)
{
	XShapeEvent *ev = (typeof(ev)) e;
	Client *c;

	if ((c = getmanaged(ev->window, ClientWindow))) {
		XPRINTF(c, "Got shape notify, redrawing shapes \n");
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
		EPRINTF(__CFMTS(c) "attempt to select an unselectable client\n", __CARGS(c));
		return False;
	}
	if (!isvisible(c, NULL)) {
		EPRINTF(__CFMTS(c) "attempt to select an invisible client\n", __CARGS(c));
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
		EPRINTF(__CFMTS(c) "attempt to focus unviewable client\n", __CARGS(c));
		return False;
	}
	return True;
}

static void
setfocus(Client *c)
{
	if (focusok(c)) {
		XPRINTF(c, "setting focus\n");
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
		gavefocus(c);
	} else if (!c) {
		Window win = None;
		int revert = RevertToPointerRoot;

		XGetInputFocus(dpy, &win, &revert);
		if (!win)
			XSetInputFocus(dpy, PointerRoot, revert, user_time);
	} else {
		/* this happens often now */
		XPRINTF(c, "cannot set focus\n");
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

	XPRINTF("FOCUS: XFocusChangeEvent type %s window 0x%lx mode %d detail %d\n",
		 e->type == FocusIn ? "FocusIn" : "FocusOut", ev->window,
		 ev->mode, ev->detail);

	if (ev->mode != NotifyNormal) {
		XPRINTF("FOCUS: mode = %d != %d\n", e->xfocus.mode, NotifyNormal);
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
	if ((c = findmanaged(ev->window))) {
		if (c == took && e->type == FocusOut) {
			if (!checkfocuslock(NULL, e->xany.serial))
				tookfocus(NULL);
			return True;
		}
		if (c != took && e->type == FocusIn) {
			if (!checkfocuslock(c, e->xany.serial))
				tookfocus(c);
			return True;
		}
		return True;
	}
	if (e->type != FocusIn)
		return True;
	if (!checkfocuslock(NULL, e->xany.serial))
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
		XPRINTF(c, "selecting\n");
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
		drawclient(c); /* just for focus change */
		raisetiled(c); /* this will restack, probably when not necessary */
		XSetWindowBorder(dpy, c->frame, scr->style.color.sele[ColBorder].pixel);
		if ((c->is.shaded && scr->options.autoroll))
			arrange(c->cview);
		else if (o && ((WTCHECK(c, WindowTypeDock)||c->is.dockapp) != (WTCHECK(o, WindowTypeDock)||o->is.dockapp)))
			arrange(c->cview);
	}
	if (o && o != c) {
		XPRINTF(o, "deselecting\n");
		drawclient(o); /* just for focus change */
		lowertiled(o); /* does nothing at the moment */
		XSetWindowBorder(dpy, o->frame, scr->style.color.norm[ColBorder].pixel);
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
	for (s = scr->flist; s && (s == c || !shouldfocus(s) || !s->is.attn); s = s->fnext) ;
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
	if ((scr->options.showdesk = scr->options.showdesk ? False : True))
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
getmanaged(Window w, int part)
{
	Client *c;

	if ((c = getclient(w, part)) && c->is.managed)
		return (c);
	return (NULL);
}

Client *
findclient(Window fwind)
{
	Client *c = NULL;
	Window froot = None, fparent = None, *children = NULL;
	unsigned int nchild = 0;

	xtrap_push(0,NULL);
	XGrabServer(dpy);

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

	XUngrabServer(dpy);
	xtrap_pop();

	return (c);
}

Client *
findmanaged(Window fwind)
{
	Client *c;

	if ((c = findclient(fwind)) && c->is.managed)
		return (c);
	return (NULL);
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

void
putresource(const char *resource, const char *value)
{
	static char clas[256] = { 0, };

	snprintf(clas, sizeof(clas), "%s*%s:\t\t%s\n", RESCLASS, resource, value);
	XrmPutStringResource(&srdb, clas, value);
}

void
putscreenres(const char *resource, const char *value)
{
	static char clas[256] = { 0, };

	snprintf(clas, sizeof(clas), "%s*screen%d.%s", RESCLASS, scr->screen, resource);
	XrmPutStringResource(&srdb, clas, value);
}

void
putintscreenres(const char *resource, int value)
{
	static char clas[256] = { 0, };

	snprintf(clas, sizeof(clas), "%d", value);
	putscreenres(resource, clas);
}

void
putcharscreenres(const char *resource, char value)
{
	static char clas[256] = { 0, };

	snprintf(clas, sizeof(clas), "%c", value);
	putscreenres(resource, clas);
}

void
putfloatscreenres(const char *resource, double value)
{
	static char clas[256] = { 0, };

	snprintf(clas, sizeof(clas), "%f", value);
	putscreenres(resource, clas);
}

void
putviewres(unsigned v, const char *resource, const char *value)
{
	static char clas[256] = { 0, };

	snprintf(clas, sizeof(clas), "view%u.%s", v, resource);
	putscreenres(clas, value);
}

void
putintviewres(unsigned v, const char *resource, int value)
{
	static char clas[256] = { 0, };

	snprintf(clas, sizeof(clas), "%d", value);
	putviewres(v, resource, clas);
}

void
putcharviewres(unsigned v, const char *resource, char value)
{
	static char clas[256] = { 0, };

	snprintf(clas, sizeof(clas), "%c", value);
	putviewres(v, resource, clas);
}

void
putfloatviewres(unsigned v, const char *resource, double value)
{
	static char clas[256] = { 0, };

	snprintf(clas, sizeof(clas), "%f", value);
	putviewres(v, resource, clas);
}

void
puttagsres(unsigned t, const char *resource, const char *value)
{
	static char clas[256] = { 0, };
	snprintf(clas, sizeof(clas), "tags.%s%u", resource, t);
	putresource(clas, value);
}

void
putsessionres(const char *resource, const char *value)
{
	static char clas[256] = { 0, };

	snprintf(clas, sizeof(clas), "%s.session.%s", RESCLASS, resource);
	XrmPutStringResource(&srdb, clas, value);
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
			XPRINTF("KeyRelease: 0x%02lx %s\n", mod, XKeysymToString(keysym));
			/* a key release other than the active key is a release of a
			   modifier indicating a stop */
			if (k && k->keysym != keysym) {
				XPRINTF("KeyRelease: stopping sequence\n");
				if (k->stop)
					k->stop(&ev, k);
				k = NULL;
			}
			break;
		case KeyPress:
			XPRINTF("KeyPress: 0x%02lx %s\n", mod, XKeysymToString(keysym));
			/* a press of a different key, even a modifier, or a press of the
			   same key with a different modifier mask indicates a stop of
			   the current sequence and the potential start of a new one */
			if (k && (k->keysym != keysym || k->mod != mod)) {
				XPRINTF("KeyPress: stopping sequence\n");
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
				XPRINTF("KeyPress: activating action for chain: %s\n", showchain(k));
				handled = True;
				if (k->func)
					k->func(&ev, k);
				if (k->chain || !k->stop)
					k = NULL;
			}
			break;
		}
	} while (k);
	XPRINTF("Done handling keypress function\n");
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
		/* ICCCM 2.0/4.1.9 Window managers will ignore any WM_PROTOCOLS
		   properties they may find on icon windows. */
		if (!c->is.pinging
		    && checkatom(c->win, _XA_WM_PROTOCOLS, _XA_NET_WM_PING)) {
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
			c->is.pinging = 1;
		}
		/* ICCCM 2.0/4.1.9 Window managers will ignore any WM_PROTOCOLS
		   properties they may find on icon windows. */
		if (!c->is.closing
		    && checkatom(c->win, _XA_WM_PROTOCOLS, _XA_WM_DELETE_WINDOW)) {
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
			c->is.closing = 1;
			return;
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
	if ((c = getmanaged(ev->window, ClientTitle))) {
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
			drawclient(c); /* just for button */
	}
	return True;
}

void
show_client_state(Client *c)
{
	XPRINTF(c, "%-20s: 0x%08x\n", "wintype", c->wintype);
	XPRINTF(c, "%-20s: 0x%08x\n", "skip.skip", c->skip.skip);
	XPRINTF(c, "%-20s: 0x%08x\n", "is.is", c->is.is);
	XPRINTF(c, "%-20s: 0x%08x\n", "has.has", c->has.has);
	XPRINTF(c, "%-20s: 0x%08x\n", "needs.has", c->needs.has);
	XPRINTF(c, "%-20s: 0x%08x\n", "can.can", c->can.can);
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
		EPRINTF(__CFMTS(c) "client already managed!\n", __CARGS(c));
		return;
	}
	XPRINTF("managing window 0x%lx\n", w);
	xtrap_push(0,NULL);
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

	updateclasshint(c);
	updatehints(c);
	updatesizehints(c);
	updatetitle(c);
	updateiconname(c);
	applyrules(c);
	applyatoms(c);

	/* Somewhere here we want to call the restore function
	   for the client and decide whether it is to be
	   restored from session management or not. */

	/* If the client has been restored at this point, we want
	   to mark it so that it is not changed by some of the later
	   EWMH and WMH functions. */

	/* do this before transients */
	wmh_process_win_window_hints(c);

	/* ICCCM 2.0/4.1.9: Window managers will ignore any WM_TRANSIENT_FOR properties
	   they find on icon windows. */
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

	if ((c->with.time) && latertime(c->user_time))
		focusnew = False;

	if (c->is.attn)
		focusnew = True;
	if (!canfocus(c))
		focusnew = False;

	if (!c->is.floater)
		c->is.floater = (!c->can.sizeh || !c->can.sizev);

	if (c->has.title) {
		c->c.t = c->r.t = scr->style.titleheight;
	} else {
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

	c->with.struts = getstruts(c) ? True : False;
	if (c->with.struts) {
		c->r.b = c->c.b = 0;
	}

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
		twa.background_pixel = scr->style.color.norm[ColBG].pixel;
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
	XSetWindowBorder(dpy, c->frame, scr->style.color.norm[ColBorder].pixel);

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
#if 0
		c->tgrip = XCreateWindow(dpy, scr->root, 0, 0, c->c.w, scr->style.gripsheight,
					 0, depth, CopyFromParent, visual, mask, &twa);
		XSaveContext(dpy, c->tgrip, context[ClientGrips], (XPointer) c);
		XSaveContext(dpy, c->tgrip, context[ClientAny], (XPointer) c);
		XSaveContext(dpy, c->tgrip, context[ScreenContext], (XPointer) scr);
		c->lgrip = XCreateWindow(dpy, scr->root, 0, 0, scr->style.gripsheight, c->c.h,
					 0, depth, CopyFromParent, visual, mask, &twa);
		XSaveContext(dpy, c->lgrip, context[ClientGrips], (XPointer) c);
		XSaveContext(dpy, c->lgrip, context[ClientAny], (XPointer) c);
		XSaveContext(dpy, c->lgrip, context[ScreenContext], (XPointer) scr);
		c->rgrip = XCreateWindow(dpy, scr->root, 0, 0, scr->style.gripsheight, c->c.h,
					 0, depth, CopyFromParent, visual, mask, &twa);
		XSaveContext(dpy, c->rgrip, context[ClientGrips], (XPointer) c);
		XSaveContext(dpy, c->rgrip, context[ClientAny], (XPointer) c);
		XSaveContext(dpy, c->rgrip, context[ScreenContext], (XPointer) scr);
#endif
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
		if (c->title)
			XReparentWindow(dpy, c->title, c->frame, 0, 0);
		if (c->tgrip)
			XReparentWindow(dpy, c->tgrip, c->frame, 0, 0);
		if (c->grips)
			XReparentWindow(dpy, c->grips, c->frame, 0, c->c.h - c->c.g);
		if (c->lgrip)
			XReparentWindow(dpy, c->lgrip, c->frame, 0, 0);
		if (c->rgrip)
			XReparentWindow(dpy, c->rgrip, c->frame, 0, c->c.w - c->c.v);
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
	ewmh_process_net_startup_id(c);
	ewmh_update_net_window_desktop(c);
	ewmh_update_net_window_extents(c);
	XPRINTF(c, "updating icon due initial manage\n");
	ewmh_process_net_window_icon(c);
	ewmh_update_ob_app_props(c);

	if (c->title && c->c.t) {
		XRectangle r = { 0, 0, c->c.w, c->c.t };

		XMoveResizeWindow(dpy, c->title, r.x, r.y, r.width, r.height);
		XMapWindow(dpy, c->title);
	}
	if (c->grips && c->c.g) {
		XRectangle r = { 0, c->c.h - c->c.g, c->c.w, c->c.g };

		XMoveResizeWindow(dpy, c->grips, r.x, r.y, r.width, r.height);
		XMapWindow(dpy, c->grips);
	}
	if (c->tgrip && c->c.v) {
		XRectangle tr = { 0, 0, c->c.w, c->c.v };
		XRectangle lr = { 0, 0, c->c.v, c->c.h };
		XRectangle rr = { c->c.w - c->c.v, 0, c->c.v, c->c.h };

		XMoveResizeWindow(dpy, c->tgrip, tr.x, tr.y, tr.width, tr.height);
		XMoveResizeWindow(dpy, c->lgrip, lr.x, lr.y, lr.width, lr.height);
		XMoveResizeWindow(dpy, c->rgrip, rr.x, rr.y, rr.width, rr.height);
		XMapWindow(dpy, c->tgrip);
		XMapWindow(dpy, c->lgrip);
		XMapWindow(dpy, c->rgrip);
	}
	if (!c->is.dockapp)
		configureshapes(c);
	if ((c->title && c->c.t) || (c->grips && c->c.g) || (c->tgrip && c->c.v))
		drawclient(c);

	if (c->with.struts) {
		ewmh_update_net_work_area();
		updategeom(NULL);
	}
	XSync(dpy, False);
	show_client_state(c);
	if (!c->is.bastard && (focusnew || (canfocus(c) && !canfocus(sel)))) {
		XPRINTF("Focusing newly managed %sclient: frame 0x%08lx win 0x%08lx name %s\n",
		     c->is.bastard ? "bastard " : "", c->frame, c->win, c->name);
		arrange(NULL);
		focus(c);
	} else {
		XPRINTF("Lowering newly managed %sclient: frame 0x%08lx win 0x%08lx name %s\n",
		     c->is.bastard ? "bastard " : "", c->frame, c->win, c->name);
		restack_belowif(c, sel);
		arrange(NULL);
		focus(sel);
	}
	xtrap_pop();
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

	XPRINTF(c, "Arming alarm 0x%08lx\n", c->sync.alarm);
	aa.trigger.counter = c->sync.counter;
	aa.trigger.wait_value = c->sync.val;
	aa.trigger.value_type = XSyncAbsolute;
	aa.trigger.test_type = XSyncPositiveComparison;
	aa.events = True;

	XSyncChangeAlarm(dpy, c->sync.alarm,
			 XSyncCACounter | XSyncCAValueType | XSyncCAValue |
			 XSyncCATestType | XSyncCAEvents, &aa);

	XPRINTF(c, "%s", "Sending client meessage\n");
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
		XPRINTF("Deferring size request from %dx%d to %dx%d for 0x%08lx 0x%08lx %s\n",
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

	if (!(c = getmanaged(ae->alarm, ClientAny))) {
		XPRINTF("Recevied alarm notify for unknown alarm 0x%08lx\n", ae->alarm);
		return False;
	}
	XPRINTF(c, "alarm notify on 0x%08lx\n", ae->alarm);
	if (!c->sync.waiting) {
		XPRINTF("%s", "Alarm was cancelled!\n");
		return True;
	}
	c->sync.waiting = False;

	if ((wc.width = c->c.w - 2 * c->c.v) != c->sync.w) {
		XPRINTF("Width changed from %d to %u since last request\n", c->sync.w, wc.width);
		mask |= CWWidth;
	}
	if ((wc.height = c->c.h - c->c.t - c->c.g - c->c.v) != c->sync.h) {
		XPRINTF("Height changed from %d to %u since last request\n", c->sync.h, wc.height);
		mask |= CWHeight;
	}
	if (mask && newsize(c, wc.width, wc.height, ae->time)) {
		XPRINTF("Configuring window %ux%u\n", wc.width, wc.height);
		xtrap_push(1,_WCFMTS(wc, mask), _WCARGS(wc, mask));
		XConfigureWindow(dpy, c->win, mask, &wc);
		xtrap_pop();
		/* don't have to send synthetic here cause changing w or h */
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

	if ((c = getmanaged(ev->window, ClientWindow))) {
		if (ev->parent != c->frame) {
			XPRINTF(c, "unmanage reparented window\n");
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
			   property. */
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
#endif
		} else if (prop == _XA_NET_WM_ICON) {
			/* Client leaders are never managed. */
			goto bad;
		} else if (prop == _XA_KWM_WIN_ICON) {
			goto bad;
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
	XPRINTF(c, "bad attempt to change client leader %s\n", (name = XGetAtomName(dpy, prop)));
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
			drawclient(c); /* just for title */
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
			   provided). */
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
			drawclient(c); /* just for title */
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
#endif
		} else if (prop == _XA_NET_WM_ICON) {
			/* Set of icons to display.  We don't do this so we can ignore
			   it.  At some point we might display iconified windows in a
			   windowmaker-style clip. */
			XPRINTF(c, "updating icon due to _NET_WM_ICON update\n");
			ewmh_process_net_window_icon(c);
		} else if (prop == _XA_KWM_WIN_ICON) {
			XPRINTF(c, "updating icon due to KWM_WIN_ICON update\n");
			ewmh_process_net_window_icon(c);
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
	XPRINTF(c, "bad attempt to change group leader %s\n", (name = XGetAtomName(dpy, prop)));
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
			drawclient(c); /* just for title */
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
			updateclasshint(c);
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
			drawclient(c); /* just for colormap */
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
			drawclient(c); /* just for title */
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
			c->with.struts = getstruts(c) ? True : False;
			if (c->with.struts)
				c->r.b = c->c.b = 0;
			updatestruts();
		} else if (prop == _XA_NET_WM_STRUT_PARTIAL) {
			/* The client MAY change this property at any time, therefore the
			   window manager MUST watch for property notify events if the
			   window manager uses this property to assign special semantics
			   to the window.  If both this property and the _NET_WM_STRUT
			   property values are set, the window manager MUST ignore the
			   _NET_WM_STRUPT property and use this one instead. */
			c->with.struts = getstruts(c) ? True : False;
			if (c->with.struts)
				c->r.b = c->c.b = 0;
			updatestruts();
#if 0
		} else if (prop == _XA_NET_WM_ICON_GEOMETRY) {
			/* Normally set by pagers at any time.  Used by window managers
			   to animate window iconification.  We don't do this so we can
			   ignore it. */
			return False;
#endif
		} else if (prop == _XA_NET_WM_ICON) {
			/* Set of icons to display.  We don't do this so we can ignore
			   it.  At some point we might display iconified windows in a
			   windowmaker-style clip. */
			XPRINTF(c, "updating icon due to _NET_WM_ICON update\n");
			ewmh_process_net_window_icon(c);
		} else if (prop == _XA_KWM_WIN_ICON) {
			XPRINTF(c, "updating icon due to KWM_WIN_ICON update\n");
			ewmh_process_net_window_icon(c);
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
	XPRINTF(c, "bad attempt to change client %s\n", (name = XGetAtomName(dpy, prop)));
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
			if ((c = getmanaged(m[i], ClientWindow)))
				result |= updatesessionprop(c, ev->atom, ev->state);
		/* client leader windows must not be managed */
		return result;
	} else if ((m = getgroup(ev->window, ClientGroup, &n))) {
		for (i = 0; i < n; i++)
			if ((c = getmanaged(m[i], ClientWindow)))
				result |= updateleaderprop(c, ev->atom, ev->state);
		/* group leader window may also be managed */
		if ((c = getmanaged(ev->window, ClientWindow)))
			result |= updateclientprop(c, ev->atom, ev->state);
		return result;
	} else if ((c = getmanaged(ev->window, ClientWindow))) {
		return updateclientprop(c, ev->atom, ev->state);
	} else if ((c = getmanaged(ev->window, ClientTimeWindow))) {
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
		XPRINTF("cleanup switching\n");
		cleanup(CauseSwitching);
		XCloseDisplay(dpy);
		execlp("sh", "sh", "-c", arg, NULL);
		eprint("Can't exec sh -c \"%s\": %s\n", arg, strerror(errno));
	}
}

void
restart(const char *arg)
{
	running = False;
	if (arg) {
		XPRINTF("cleanup switching\n");
		cleanup(CauseSwitching);
		XCloseDisplay(dpy);
		execlp("sh", "sh", "-c", arg, NULL);
		eprint("Can't exec sh -c \"%s\": %s\n", arg, strerror(errno));
	} else {
		char **argv;
		int i;

		/* argv must be NULL terminated and writable */
		argv = calloc(cargc + 1, sizeof(*argv));
		for (i = 0; i < cargc; i++)
			argv[i] = strdup(cargv[i]);

		XPRINTF("cleanup restarting\n");
		cleanup(CauseRestarting);
		XCloseDisplay(dpy);
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
	XPRINTF("WARNING: No handler for event type %d\n", ev->type);
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
	xtrap_push(1,NULL);
	if (XQueryTree(dpy, win, &wroot, &parent, &wins, &num))
		XFindContext(dpy, wroot, context[ScreenContext], (XPointer *) &s);
	else
		EPRINTF("XQueryTree(0x%lx) failed!\n", win);
	xtrap_pop();
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
				/* in case we missed a SIGCHLD */
				while (waitpid(-1, &sig, WNOHANG) > 0) ;
				reload();
				break;
			case SIGTERM:
			case SIGINT:
			case SIGQUIT:
				quit(NULL);
				break;
			case SIGCHLD:
				while (waitpid(-1, &sig, WNOHANG) > 0) ;
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
				while (running && XPending(dpy)) {
					XNextEvent(dpy, &ev);
					scr = geteventscr(&ev);
					DPRINTF("Got an event!");
					if (!handle_event(&ev))
						DPRINTF("WARNING: Event %d not handled\n", ev.type);
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
	xtrap_push(0,NULL);
	XGrabServer(dpy);
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
			/* ICCCM 2.0/4.1.9: Window managers will ignore any WM_TRANSIENT_FOR properties
			   they find on icon windows. */
			if (XGetTransientForHint(dpy, wins[i], &d1)) {
				DPRINTF("-> skipping 0x%08lx (transient-for property set)\n", wins[i]);
				continue;
			}
			/* ICCCM 2.0/4.1.9: Window managers will ignore any WM_HINTS properties
			   they find on icon windows. */
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
		DPRINTF("XQueryTree(0x%lx) failed\n", scr->root);
	XUngrabServer(dpy);
	xtrap_pop();
	if (wins)
		XFree(wins);
	DPRINTF("done scanning screen %d\n", scr->screen);
	focus(sel);
	ewmh_update_kde_splash_progress();
	ewmh_update_startup_notification();
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

	XPRINTF("Finding monitor nearest (%d,%d), max dist = %f\n", col, row, dist);
	for (m = scr->monitors; m; m = m->next) {
		float test;

		test = hypotf((m->row - row) * h, (m->col - col) * w);
		XPRINTF("Testing monitor %d (%d,%d), dist = %f\n", m->num, m->col, m->row, test);
		if (test <= dist) {
			XPRINTF("Monitor %d is best!\n", m->num);
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

	XPRINTF("Number of dock monitors is %d\n", scr->nmons);
	for (m = scr->monitors, i = 0; i < scr->nmons; i++, m++) {
		m->dock.position = DockNone;
		m->dock.wa = m->wa;
	}
	scr->dock.monitor = NULL;
	XPRINTF("Dock monitor option is %d\n", scr->options.dockmon);
	dockmon = (int) scr->options.dockmon - 1;
	XPRINTF("Dock monitor chosen is %d\n", dockmon);
	if (dockmon > scr->nmons - 1)
		dockmon = scr->nmons - 1;
	XPRINTF("Dock monitor adjusted is %d\n", dockmon);
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
				XPRINTF("Found monitor %d near (%d,%d)\n", dockmon, col, row);
			} else {
				XPRINTF("Cannot find monitor near (%d,%d)\n", col, row);
			}
		}
	}
	if (dockmon >= 0) {
		XPRINTF("Looking for monitor %d assigned to dock...\n", dockmon);
		for (m = scr->monitors; m; m = m->next) {
			if (m->num == dockmon) {
				m->dock.position = scr->options.dockpos;
				m->dock.orient = scr->options.dockori;
				scr->dock.monitor = m;
				XPRINTF("Found monitor %d assigned to dock.\n", dockmon);
				break;
			}
		}
	}
	if (!scr->dock.monitor) {
		XPRINTF("Did not find monitor assigned dock %d\n", dockmon);
		if ((m = scr->monitors)) {
			XPRINTF("Falling back to default monitor for dock.\n");
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

	XPRINTF("There are now %d monitors (was %d)\n", n, scr->nmons);
	for (i = 0; i < n; i++)
		scr->monitors[i].next = &scr->monitors[i + 1];
	scr->monitors[n - 1].next = NULL;
	if (scr->nmons != n)
		full_update = True;
	if (full_update)
		size_update = True;
	scr->nmons = n;
	XPRINTF("Performing %s/%s updates\n",
			full_update ? "full" : "partial",
			size_update ? "resize" : "no-resize");
	if (e) {
		XPRINTF("Responding to an event\n");
		if (full_update) {
			for (c = scr->clients; c; c = c->next) {
				if (isomni(c))
					continue;
				if (!(m = findmonitor(c)))
					m = nearmonitor();
				c->tags = (1ULL << m->curview->index);
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
	XPRINTF("Finding largest monitor\n");
	for (w = 0, h = 0, scr->sw = 0, scr->sh = 0, i = 0; i < n; i++) {
		m = scr->monitors + i;
		XPRINTF("Processing monitor %d\n", m->index);
		w = max(w, m->sc.w);
		h = max(h, m->sc.h);
		XPRINTF("Maximum monitor dimensions %dx%d\n", w, h);
		scr->sw = max(scr->sw, m->sc.x + m->sc.w);
		scr->sh = max(scr->sh, m->sc.y + m->sc.h);
		XPRINTF("Maximum screen  dimensions %dx%d\n", w, h);
	}
	XPRINTF("Maximum screen  size is now %dx%d\n", scr->sw, scr->sh);
	XPRINTF("Maximum monitor size is now %dx%d\n", w, h);
	scr->m.rows = (scr->sh + h - 1) / h;
	scr->m.cols = (scr->sw + w - 1) / w;
	XPRINTF("Monitor array is %dx%d\n", scr->m.cols, scr->m.rows);
	h = scr->sh / scr->m.rows;
	w = scr->sw / scr->m.cols;
	XPRINTF("Each monitor cell is %dx%d\n", w, h);
	for (i = 0; i < n; i++) {
		m = scr->monitors + i;
		m->row = m->my / h;
		m->col = m->mx / w;
		XPRINTF("Monitor %d at (%d,%d)\n", i+1, m->col, m->row);
	}
	/* handle insane geometries, push overlaps to the right */
	XPRINTF("Handling insane geometries...\n");
	do {
		changed = False;
		for (i = 0; i < n && !changed; i++) {
			m = scr->monitors + i;
			for (j = i + 1; j < n && !changed; j++) {
				Monitor *o = scr->monitors + j;

				if (m->row != o->row || m->col != o->col)
					continue;
				XPRINTF("Monitors %d and %d conflict at (%d,%d)\n",
						m->index, o->index, m->col, m->row);
				if (m->mx < o->mx) {
					o->col++;
					XPRINTF("Moving monitor %d to the right (%d,%d)\n",
							o->index, o->col, o->row);
					scr->m.cols = max(scr->m.cols, o->col + 1);
					XPRINTF("Monitor array is now %dx%d\n",
							scr->m.cols, scr->m.rows);
				} else {
					m->col++;
					XPRINTF("Moving monitor %d to the right (%d,%d)\n",
							m->index, m->col, m->row);
					scr->m.cols = max(scr->m.cols, m->col + 1);
					XPRINTF("Monitor array is now %dx%d\n",
							scr->m.cols, scr->m.rows);
				}
				changed = True;
			}
		}
	} while (changed);
	/* update all view pointers */
	XPRINTF("There are %d tags\n", scr->ntags);
	for (v = scr->views, i = 0; i < scr->ntags; i++, v++)
		v->curmon = NULL;
	for (m = scr->monitors, i = 0; i < n; i++, m++) {
		XPRINTF("Setting view %d to monitor %d\n", m->curview->index, m->index);
		m->curview->curmon = m;
	}
	XPRINTF("Updating dock...\n");
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
	XPRINTF("%s", "compiled without RANDR support\n");
#endif

#ifdef XINERAMA
	if (einfo[XineramaBase].have) {
		int i;
		XineramaScreenInfo *si;

		XPRINTF("XINERAMA extension supported\n");
		if (!XineramaIsActive(dpy)) {
			XPRINTF("XINERAMA is not active for screen %d\n", scr->screen);
			goto no_xinerama;
		}
		XPRINTF("XINERAMA is active for screen %d\n", scr->screen);
		si = XineramaQueryScreens(dpy, &n);
		if (!si) {
			XPRINTF("XINERAMA defines no monitors for screen %d\n",
				scr->screen);
			goto no_xinerama;
		}
		XPRINTF("XINERAMA defines %d monitors for screen %d\n", n, scr->screen);
		if (n < 2) {
			XFree(si);
			goto no_xinerama;
		}
		for (i = 0; i < n; i++) {
			if (i < scr->nmons) {
				m = &scr->monitors[i];
				XPRINTF("Checking existing monitor %d\n", m->index);
				if (m->sc.x != si[i].x_org) {
					XPRINTF("Monitor %d x position changed from %d to %d\n",
							m->index, m->sc.x, si[i].x_org);
					m->sc.x = m->wa.x = m->dock.wa.x = si[i].x_org;
					m->mx = m->sc.x + m->sc.w / 2;
					full_update = True;
				}
				if (m->sc.y != si[i].y_org) {
					XPRINTF("Monitor %d y position changed from %d to %d\n",
							m->index, m->sc.y, si[i].y_org);
					m->sc.y = m->wa.y = m->dock.wa.y = si[i].y_org;
					m->my = m->sc.y + m->sc.h / 2;
					full_update = True;
				}
				if (m->sc.w != si[i].width) {
					XPRINTF("Monitor %d width changed from %d to %d\n",
							m->index, m->sc.w, si[i].width);
					m->sc.w = m->wa.w = m->dock.wa.w = si[i].width;
					m->mx = m->sc.x + m->sc.w / 2;
					size_update = True;
				}
				if (m->sc.h != si[i].height) {
					XPRINTF("Monitor %d height changed from %d to %d\n",
							m->index, m->sc.h, si[i].height);
					m->sc.h = m->wa.h = m->dock.wa.h = si[i].height;
					m->my = m->sc.y + m->sc.h / 2;
					size_update = True;
				}
				if (m->num != si[i].screen_number) {
					XPRINTF("Monitor %d screen number changed from %d to %d\n",
							m->index, m->num, si[i].screen_number);
					m->num = si[i].screen_number;
					full_update = True;
				}
			} else {
				XSetWindowAttributes wa;
				int mask = 0;
				int j;

				XPRINTF("Adding new monitor %d\n", i);
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
			XPRINTF("Monitor %d:\n", m->index);
			XPRINTF("\tindex           = %d\n", m->index);
			XPRINTF("\tscreen          = %d\n", m->num);
			XPRINTF("\tscreen.x        = %d\n", m->sc.x);
			XPRINTF("\tscreen.y        = %d\n", m->sc.y);
			XPRINTF("\tscreen.w        = %d\n", m->sc.w);
			XPRINTF("\tscreen.h        = %d\n", m->sc.h);
			XPRINTF("\tworkarea.x      = %d\n", m->wa.x);
			XPRINTF("\tworkarea.y      = %d\n", m->wa.y);
			XPRINTF("\tworkarea.w      = %d\n", m->wa.w);
			XPRINTF("\tworkarea.h      = %d\n", m->wa.h);
			XPRINTF("\tdock.workarea.x = %d\n", m->dock.wa.x);
			XPRINTF("\tdock.workarea.y = %d\n", m->dock.wa.y);
			XPRINTF("\tdock.workarea.w = %d\n", m->dock.wa.w);
			XPRINTF("\tdock.workarea.h = %d\n", m->dock.wa.h);
			XPRINTF("\tmiddle.x        = %d\n", m->mx);
			XPRINTF("\tmiddle.y        = %d\n", m->my);
			XPRINTF("\tmiddle.y        = %d\n", m->my);
		}
		XFree(si);
		updatemonitors(e, n, size_update, full_update);
		return True;

	} else
		XPRINTF("no XINERAMA extension for screen %d\n", scr->screen);
      no_xinerama:
#else
	XPRINTF("%s", "compiled without XINERAMA support\n");
#endif
#ifdef XRANDR
	if (einfo[XrandrBase].have) {
		XRRScreenResources *sr;
		int i, j;

		XPRINTF("RANDR extension supported\n");
		if (!(sr = XRRGetScreenResources(dpy, scr->root))) {
			XPRINTF("RANDR not active for display\n");
			goto no_xrandr;
		}
		XPRINTF("RANDR defines %d ctrc for display\n", sr->ncrtc);
		if (sr->ncrtc < 2) {
			XRRFreeScreenResources(sr);
			goto no_xrandr;
		}
		for (i = 0, n = 0; i < sr->ncrtc; i++) {
			XRRCrtcInfo *ci;

			XPRINTF("Checking CRTC %d\n", i);
			if (!(ci = XRRGetCrtcInfo(dpy, sr, sr->crtcs[i]))) {
				XPRINTF("CRTC %d not defined\n", i);
				continue;
			}
			if (!ci->width || !ci->height) {
				XPRINTF("CRTC %d is %dx%d\n", i, ci->width, ci->height);
				XRRFreeCrtcInfo(ci);
				continue;
			}
			/* skip mirrors */
			for (j = 0; j < n; j++)
				if (scr->monitors[j].sc.x == scr->monitors[n].sc.x &&
				    scr->monitors[j].sc.y == scr->monitors[n].sc.y)
					break;
			if (j < n) {
				XPRINTF("Monitor %d is a mirror of %d\n", n, j);
				XRRFreeCrtcInfo(ci);
				continue;
			}

			if (n < scr->nmons) {
				m = &scr->monitors[n];
				XPRINTF("Checking existing monitor %d\n", m->index);
				if (m->sc.x != ci->x) {
					XPRINTF("Monitor %d x position changed from %d to %d\n",
							m->index, m->sc.x, ci->x);
					m->sc.x = m->wa.x = m->dock.wa.x = ci->x;
					m->mx = m->sc.x + m->sc.w / 2;
					full_update = True;
				}
				if (m->sc.y != ci->y) {
					XPRINTF("Monitor %d y position changed from %d to %d\n",
							m->index, m->sc.y, ci->y);
					m->sc.y = m->wa.y = m->dock.wa.y = ci->y;
					m->my = m->sc.y + m->sc.h / 2;
					full_update = True;
				}
				if (m->sc.w != ci->width) {
					XPRINTF("Monitor %d width changed from %d to %d\n",
							m->index, m->sc.w, ci->width);
					m->sc.w = m->wa.w = m->dock.wa.w = ci->width;
					m->mx = m->sc.x + m->sc.w / 2;
					size_update = True;
				}
				if (m->sc.h != ci->height) {
					XPRINTF("Monitor %d height changed from %d to %d\n",
							m->index, m->sc.h, ci->height);
					m->sc.h = m->wa.h = m->dock.wa.h = ci->height;
					m->my = m->sc.y + m->sc.h / 2;
					size_update = True;
				}
				if (m->num != i) {
					XPRINTF("Monitor %d screen number changed from %d to %d\n",
							m->index, m->num, i);
					m->num = i;
					full_update = True;
				}
			} else {
				XSetWindowAttributes wa;
				int mask = 0;
				int j;

				XPRINTF("Adding new monitor %d\n", n);
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
			XPRINTF("Monitor %d:\n", m->index);
			XPRINTF("\tindex           = %d\n", m->index);
			XPRINTF("\tscreen          = %d\n", m->num);
			XPRINTF("\tscreen.x        = %d\n", m->sc.x);
			XPRINTF("\tscreen.y        = %d\n", m->sc.y);
			XPRINTF("\tscreen.w        = %d\n", m->sc.w);
			XPRINTF("\tscreen.h        = %d\n", m->sc.h);
			XPRINTF("\tworkarea.x      = %d\n", m->wa.x);
			XPRINTF("\tworkarea.y      = %d\n", m->wa.y);
			XPRINTF("\tworkarea.w      = %d\n", m->wa.w);
			XPRINTF("\tworkarea.h      = %d\n", m->wa.h);
			XPRINTF("\tdock.workarea.x = %d\n", m->dock.wa.x);
			XPRINTF("\tdock.workarea.y = %d\n", m->dock.wa.y);
			XPRINTF("\tdock.workarea.w = %d\n", m->dock.wa.w);
			XPRINTF("\tdock.workarea.h = %d\n", m->dock.wa.h);
			XPRINTF("\tmiddle.x        = %d\n", m->mx);
			XPRINTF("\tmiddle.y        = %d\n", m->my);
			XPRINTF("\tmiddle.y        = %d\n", m->my);
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
		XPRINTF("Checking existing monitor %d\n", m->index);
		if (m->sc.x != 0) {
			XPRINTF("Monitor %d x position changed from %d to %d\n",
					m->index, m->sc.x, 0);
			m->sc.x = m->wa.x = m->dock.wa.x = 0;
			m->mx = m->sc.x + m->sc.w / 2;
			full_update = True;
		}
		if (m->sc.y != 0) {
			XPRINTF("Monitor %d y position changed from %d to %d\n",
					m->index, m->sc.y, 0);
			m->sc.y = m->wa.y = m->dock.wa.y = 0;
			m->my = m->sc.y + m->sc.h / 2;
			full_update = True;
		}
		if (m->sc.w != DisplayWidth(dpy, scr->screen)) {
			XPRINTF("Monitor %d width changed from %d to %d\n",
					m->index, m->sc.w, (int) DisplayWidth(dpy, scr->screen));
			m->sc.w = m->wa.w = m->dock.wa.w = DisplayWidth(dpy, scr->screen);
			m->mx = m->sc.x + m->sc.w / 2;
			size_update = True;
		}
		if (m->sc.h != DisplayHeight(dpy, scr->screen)) {
			XPRINTF("Monitor %d height changed from %d to %d\n",
					m->index, m->sc.h, (int) DisplayHeight(dpy, scr->screen));
			m->sc.h = m->wa.h = m->dock.wa.h =
			    DisplayHeight(dpy, scr->screen);
			m->my = m->sc.y + m->sc.h / 2;
			size_update = True;
		}
		if (m->num != 0) {
			XPRINTF("Monitor %d screen number changed from %d to %d\n",
					m->index, m->num, 0);
			m->num = 0;
			full_update = True;
		}
	} else {
		XSetWindowAttributes wa;
		int mask = 0;
		int j;

		XPRINTF("Adding new monitor %d\n", 0);
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
	XPRINTF("Monitor %d:\n", m->index);
	XPRINTF("\tindex           = %d\n", m->index);
	XPRINTF("\tscreen          = %d\n", m->num);
	XPRINTF("\tscreen.x        = %d\n", m->sc.x);
	XPRINTF("\tscreen.y        = %d\n", m->sc.y);
	XPRINTF("\tscreen.w        = %d\n", m->sc.w);
	XPRINTF("\tscreen.h        = %d\n", m->sc.h);
	XPRINTF("\tworkarea.x      = %d\n", m->wa.x);
	XPRINTF("\tworkarea.y      = %d\n", m->wa.y);
	XPRINTF("\tworkarea.w      = %d\n", m->wa.w);
	XPRINTF("\tworkarea.h      = %d\n", m->wa.h);
	XPRINTF("\tdock.workarea.x = %d\n", m->dock.wa.x);
	XPRINTF("\tdock.workarea.y = %d\n", m->dock.wa.y);
	XPRINTF("\tdock.workarea.w = %d\n", m->dock.wa.w);
	XPRINTF("\tdock.workarea.h = %d\n", m->dock.wa.h);
	XPRINTF("\tmiddle.x        = %d\n", m->mx);
	XPRINTF("\tmiddle.y        = %d\n", m->my);
	XPRINTF("\tmiddle.y        = %d\n", m->my);
	updatemonitors(e, n, size_update, full_update);
	return True;
}

void
sighandler(int sig)
{
	signum = sig;
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

/* This must be done after the style has loaded because we want to set one of the sizes
 * to the size of icons desired for the titlebar.  Not sure whether toolkits will detect a change in
 * this property and supply better sizes in _NET_WM_ICON or not.
 *
 * We want 56x56 so that we can put the icon in the clip, we want 32x32 for and feedback
 * displays that we add in the future.  We also want hxh where h = scr->style.titleheight
 * so that we do not have to scale icons when placing them in the titlebar.  The first one
 * should be the most desired one for now, which is the titlebar icon.
 */
static void
initsizes(Bool reload)
{
	int h = scr->style.titleheight;

	if (h < 12) {
		EPRINTF("Icon size is too small: %d\n", h);
		h = 12;
	}
	if (h > 64) {
		EPRINTF("Icon size is too large: %d\n", h);
		h = 64;
	}
	long data[18] = { h, h, h, h, 1, 1, 32, 32, 64, 64, 8, 8, 12, 12, 24, 24, 4, 4 };
	/* maybe it is letting size increment be zero that is causing problems */
	XChangeProperty(dpy, scr->root, XA_WM_ICON_SIZE, XA_WM_ICON_SIZE, 32,
			PropModeReplace, (unsigned char *) data, 18);
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
		strcpy(xdgdirs.runt, "/run/user/");
		strcat(xdgdirs.runt, buf);
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

	OPRINTF("initializing cursors\n");
	initcursors(reload);	/* init cursors */
	OPRINTF("initializing modifier map\n");
	initmodmap(reload);	/* init modifier map */
	OPRINTF("initializing event selection\n");
	initselect(reload);	/* select for events */
	OPRINTF("initializing startup notification\n");
	initstartup(reload);	/* init startup notification */

	OPRINTF("initializing home and XDG directories\n");
	initdirs(reload);	/* init HOME and XDG directories */
	OPRINTF("initializing location of config file\n");
	initrcfile(conf, reload);	/* find the configuration file */
	OPRINTF("initializing client rules\n");
	initrules(reload);	/* initialize window class.name rules */
	OPRINTF("initializing configuration\n");
	initconfig(reload);	/* initialize configuration */
	OPRINTF("initializing icon theme\n");
	initicons(reload);	/* initialize icon theme */

	for (scr = screens; scr < screens + nscr; scr++) {
		if (!scr->managed)
			continue;

		OPRINTF("initializing screen\n");
		initscreen(reload);	/* init per-screen configuration */
		OPRINTF("initializing EWMH atoms and root properties\n");
		initewmh(ops->name);	/* init EWMH atoms */
		OPRINTF("initializing tags (desktops)\n");
		inittags(reload);	/* init tags */

		if (!reload) {
			OPRINTF("initializing monitor geometry\n");
			initmonitors(NULL);	/* init geometry */
			/* I think that we can do this all the time. */
		}

		OPRINTF("initializing key bindings\n");
		initkeys(reload);	/* init key bindings */
		OPRINTF("initializing button bindings\n");
		initbuttons(reload);	/* init button bindings */
		OPRINTF("initializing dock\n");
		initdock(reload);	/* initialize dock */
		OPRINTF("initializing layouts and views\n");
		initviews(reload);	/* initialize layouts */

		OPRINTF("initializing key grabs\n");
		grabkeys();

		OPRINTF("initializing style\n");
		initstyle(reload);	/* init appearance */
		OPRINTF("initializing struts and workareas\n");
		initstruts(reload);	/* initialize struts and workareas */
		OPRINTF("initializing icon sizes\n");
		initsizes(reload);	/* init icon sizes */
	}

	findscreen(reload);	/* find current screen (with pointer) */

	if (owd) {
		if (chdir(owd))
			XPRINTF("Could not change directory to %s: %s\n", owd,
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
	if (!arg)
		return;

	if (fork() == 0) {
		close(ConnectionNumber(dpy));
		execlp("sh", "sh", "-c", arg, NULL);
		eprint("Can't exec sh -c \"%s\": %s\n", arg, strerror(errno));
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
			XPRINTF("Setting struts to StrutsDown\n");
		} else if (scr->options.hidebastards) {
			v->barpos = StrutsHide;
			XPRINTF("Setting struts to StrutsHide\n");
		} else {
			v->barpos = StrutsOff;
			XPRINTF("Setting struts to StrutsOff\n");
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

	/* ICCCM 2.0/4.1.9: Window managers will ignore any WM_TRANSIENT_FOR properties
	   they find on icon windows. */
	doarrange = !(c->skip.arrange || c->is.floater || (cause != CauseDestroyed &&
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
#ifdef RENDER
	if (c->pict.win) {
		XRenderFreePicture(dpy, c->pict.win);
		if (c->pict.icon) {
			if (c->pict.icon != c->pict.win)
				XRenderFreePicture(dpy, c->pict.icon);
			c->pict.icon = None;
		}
		c->pict.win = None;
	}
	if (c->pict.icon) {
		XRenderFreePicture(dpy, c->pict.icon);
		c->pict.icon = None;
	}
#endif
	if (c->title) {
#ifdef RENDER
		if (c->pict.title) {
			XRenderFreePicture(dpy, c->pict.title);
			c->pict.title = None;
		}
#endif
		XDestroyWindow(dpy, c->title);
		XDeleteContext(dpy, c->title, context[ClientTitle]);
		XDeleteContext(dpy, c->title, context[ClientAny]);
		XDeleteContext(dpy, c->title, context[ScreenContext]);
		c->title = None;
		free(c->element);
	}
	if (c->grips) {
#ifdef RENDER
		if (c->pict.grips) {
			XRenderFreePicture(dpy, c->pict.grips);
			c->pict.grips = None;
		}
#endif
		XDestroyWindow(dpy, c->grips);
		XDeleteContext(dpy, c->grips, context[ClientGrips]);
		XDeleteContext(dpy, c->grips, context[ClientAny]);
		XDeleteContext(dpy, c->grips, context[ScreenContext]);
		c->grips = None;
	}
	if (c->tgrip) {
#ifdef RENDER
		if (c->pict.tgrip) {
			XRenderFreePicture(dpy, c->pict.tgrip);
			c->pict.tgrip = None;
		}
#endif
		XDestroyWindow(dpy, c->tgrip);
		XDeleteContext(dpy, c->tgrip, context[ClientGrips]);
		XDeleteContext(dpy, c->tgrip, context[ClientAny]);
		XDeleteContext(dpy, c->tgrip, context[ScreenContext]);
		c->tgrip = None;
	}
	if (c->lgrip) {
#ifdef RENDER
		if (c->pict.lgrip) {
			XRenderFreePicture(dpy, c->pict.lgrip);
			c->pict.lgrip = None;
		}
#endif
		XDestroyWindow(dpy, c->lgrip);
		XDeleteContext(dpy, c->lgrip, context[ClientGrips]);
		XDeleteContext(dpy, c->lgrip, context[ClientAny]);
		XDeleteContext(dpy, c->lgrip, context[ScreenContext]);
		c->lgrip = None;
	}
	if (c->rgrip) {
#ifdef RENDER
		if (c->pict.rgrip) {
			XRenderFreePicture(dpy, c->pict.rgrip);
			c->pict.rgrip = None;
		}
#endif
		XDestroyWindow(dpy, c->rgrip);
		XDeleteContext(dpy, c->rgrip, context[ClientGrips]);
		XDeleteContext(dpy, c->rgrip, context[ClientAny]);
		XDeleteContext(dpy, c->rgrip, context[ScreenContext]);
		c->rgrip = None;
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
			unsigned mask = 0;

			if (c->sh.win_gravity == StaticGravity || c->is.dockapp) {
				/* restore static geometry */
				wc.x = c->s.x; mask |= CWX;
				wc.y = c->s.y; mask |= CWY;
				wc.width = c->s.w; mask |= CWWidth;
				wc.height = c->s.h; mask |= CWHeight;
			} else {
				/* restore geometry */
				wc.x = c->r.x; mask |= CWX;
				wc.y = c->r.y; mask |= CWY;
				wc.width = c->r.w; mask |= CWWidth;
				wc.height = c->r.h; mask |= CWHeight;
			}
			wc.border_width = c->s.b; mask |= CWBorderWidth;
			xtrap_push(1,_WCFMTS(wc, mask), _WCARGS(wc, mask));
			if (c->icon) {
				XReparentWindow(dpy, c->icon, scr->root, wc.x, wc.y);
				XConfigureWindow(dpy, c->icon, mask, &wc);
			} else {
				XReparentWindow(dpy, c->win, scr->root, wc.x, wc.y);
				XConfigureWindow(dpy, c->win, mask, &wc);
			}
			xtrap_pop();
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
#ifdef STARTUP_NOTIFICATION
	removeappl(c);
#endif
	removeclass(c);
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
	if (c->ch.res_name) {
		XFree(c->ch.res_name);
		c->ch.res_name = NULL;
	}
	if (c->ch.res_class) {
		XFree(c->ch.res_class);
		c->ch.res_class = NULL;
	}
	removebutton(&c->button);
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

	if ((c = getmanaged(ev->window, ClientWindow))) {
		XPRINTF(c, "self-unmapped window\n");
		if (ev->send_event) {
			/* synthetic */
			if (ev->event == event_scr->root) {
				XPRINTF(c, "unmanage self-unmapped window (synthetic)\n");
				unmanage(c, CauseUnmapped);
				return True;
			}
		} else {
			/* real event */
			if (ev->event == c->frame && c->is.managed) {
				XPRINTF(c, "unmanage self-unmapped window (real event)\n");
				unmanage(c, CauseUnmapped);
				return True;
			}
		}
	}
	return False;
}

void
updateclasshint(Client *c)
{
	if (c->ch.res_class) {
		XFree(c->ch.res_class);
		c->ch.res_class = NULL;
	}
	if (c->ch.res_name) {
		XFree(c->ch.res_name);
		c->ch.res_name = NULL;
	}
	getclasshint(c, &c->ch);
	updateclass(c);
}

void
updatehints(Client *c)
{
	Window leader;
	int take_focus, give_focus;

	/* ICCCM 2.0/4.1.9: Window managers will ignore any WM_PROTOCOLS properties they
	   find on icon windows. ... Clients must not depend on being able to receive
	   input events by means of their icon windows. */
	take_focus =
	    checkatom(c->win, _XA_WM_PROTOCOLS, _XA_WM_TAKE_FOCUS) ? TAKE_FOCUS : 0;
	give_focus = c->is.dockapp ? 0 : GIVE_FOCUS;
	c->can.focus = take_focus | give_focus;

	{
		XWMHints *wmh;

		/* UXTerm updates WM_HINTS all the time without changing it */
		/* ICCCM 2.0/4.1.9: Window managers will ignore any WM_HINTS properties
		   they find on icon windows. */
		if ((wmh = XGetWMHints(dpy, c->win))) {
			if (c->is.managed) {
				if (((c->wmh.flags ^ wmh->flags) & (IconPixmapHint | IconMaskHint))
				    || wmh->icon_window != c->wmh.icon_window
				    || wmh->icon_mask != c->wmh.icon_mask) {
					applywmhints(c, wmh);
					XPRINTF(c, "updating icon due to WM_HINTS update\n");
					ewmh_process_net_window_icon(c);
				} else
					applywmhints(c, wmh);
			} else
				applywmhints(c, wmh);
			XFree(wmh);
		}
	}
	if (c->wmh.flags & XUrgencyHint && !c->is.attn) {
		c->is.attn = True;
		if (c->is.managed)
			ewmh_update_net_window_state(c);
	}
	if (c->wmh.flags & InputHint)
		c->can.focus = take_focus | (c->wmh.input ? GIVE_FOCUS : 0);
	if (c->wmh.flags & WindowGroupHint) {
		leader = c->wmh.window_group;
		if (c->leader != leader) {
			removegroup(c, c->leader, ClientGroup);
			c->leader = leader;
			updategroup(c, c->leader, ClientGroup, &c->nonmodal);
		}
	}
}

void
updatesizehints(Client *c)
{
	long supplied = 0;

	/* ICCCM 2.0/4.1.9: Window managers will ignore any WM_NORMAL_HINTS properties
	   they fined on icon windows. */
	if (!XGetWMNormalHints(dpy, c->win, &c->sh, &supplied))
		return;

	if (c->sh.flags & (USPosition | PPosition)) {
		/* can't trust these values, overwrite with those from
		   XGetWindowAttributes() */
		c->sh.x = c->u.x;
		c->sh.y = c->u.y;
	} else {
		c->sh.x = 0;
		c->sh.y = 0;
	}
	if (c->sh.flags & (USSize | PSize)) {
		/* can't trust these values, overwrite with those from
		   XGetWindowAttributes() */
		/* some clients (namely ROXterm) are putting pure garbage in these
		   fields. */
		c->sh.width = c->u.w;
		c->sh.height = c->u.h;
	} else {
		c->sh.width = 0;
		c->sh.height = 0;
	}
	if (c->sh.flags & PBaseSize) {
		c->sh.base_width = c->sh.base_width;
		c->sh.base_height = c->sh.base_height;
	} else if (c->sh.flags & PMinSize) {
		c->sh.base_width = c->sh.min_width;
		c->sh.base_height = c->sh.min_height;
	} else {
		c->sh.base_width = 0;
		c->sh.base_height = 0;
	}
	if (c->sh.flags & PResizeInc) {
		c->sh.width_inc = c->sh.width_inc;
		c->sh.height_inc = c->sh.height_inc;
	} else {
		c->sh.width_inc = 0;
		c->sh.height_inc = 0;
	}
	if (c->sh.flags & PMaxSize) {
		c->sh.max_width = c->sh.max_width;
		c->sh.max_height = c->sh.max_height;
	} else {
		c->sh.max_width = 0;
		c->sh.max_height = 0;
	}
	if (c->sh.flags & PMinSize) {
		c->sh.min_width = c->sh.min_width;
		c->sh.min_height = c->sh.min_height;
	} else if (c->sh.flags & PBaseSize) {
		c->sh.min_width = c->sh.base_width;
		c->sh.min_height = c->sh.base_height;
	} else {
		c->sh.min_width = 0;
		c->sh.min_height = 0;
	}
	if (c->sh.flags & PAspect) {
		c->sh.min_aspect.x = c->sh.min_aspect.x;
		c->sh.min_aspect.y = c->sh.min_aspect.y;
		c->sh.max_aspect.x = c->sh.max_aspect.x;
		c->sh.max_aspect.y = c->sh.max_aspect.y;
	} else {
		c->sh.min_aspect.x = 0;
		c->sh.min_aspect.y = 0;
		c->sh.max_aspect.x = 0;
		c->sh.max_aspect.y = 0;
	}
	if (c->sh.flags & PWinGravity)
		c->sh.win_gravity = c->sh.win_gravity;
	else
		c->sh.win_gravity = NorthWestGravity;
	if (c->sh.max_width && c->sh.min_width && c->sh.max_width == c->sh.min_width) {
		c->can.sizeh = False;
		c->can.maxh = False;
		c->can.fillh = False;
	}
	if (c->sh.max_height && c->sh.min_height && c->sh.max_height == c->sh.min_height) {
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

static void
addprefix(Client *c, char **name)
{
	char *vname = NULL;
	Class *r;

	if ((r = getclass(c)) && r->count > 1) {
		char buf[16] = { 0, };
		int i, len;

		for (i = 0; i < r->count; i++)
			if (r->members[i] == c->win)
				break;
		/* TODO: add this format string to style */
		snprintf(buf, sizeof(buf), "[%d] ", i + 1);
		len = strlen(buf) + strlen(*name);
		if ((vname = calloc(len + 1, sizeof(*vname)))) {
			strncpy(vname, buf, len);
			strncat(vname, *name, len);
		}
	}
	if (vname) {
		free(*name);
		*name = vname;
	}
}

void
updatetitle(Client *c)
{
	/* Window managers will ignore any WM_NAME properties they find on icon windows. */
	if ((!gettextprop(c->win, _XA_NET_WM_NAME, &c->name) || !c->name) &&
	    (!gettextprop(c->win, XA_WM_NAME, &c->name)))
		return;
	addprefix(c, &c->name);
	ewmh_update_net_window_visible_name(c);
}

void
updateiconname(Client *c)
{
	/* Window managers will ignore any WM_ICON_NAME properties they find on icon windows. */
	if ((!gettextprop(c->win, _XA_NET_WM_ICON_NAME, &c->icon_name) || !c->icon_name) &&
	    (!gettextprop(c->win, XA_WM_ICON_NAME, &c->icon_name)))
		return;
	addprefix(c, &c->icon_name);
	ewmh_update_net_window_visible_icon_name(c);
}

void
updatetransientfor(Client *c)
{
	Window trans;

	/* ICCCM 2.0/4.1.9: Window managers will ignore any WM_TRANSIENT_FOR properties
	   they find on icon windows. */
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

#ifdef STARTUP_NOTIFICATION
Appl *
getappl(Client *c)
{
	Appl *a = NULL;

	if (c->win)
		XFindContext(dpy, c->win, context[ClientAppl], (XPointer *) &a);
	return (a);
}

Appl *
findappl(SnStartupSequence *seq)
{
	Appl *a;
	const char *appid, *str;

	if (!seq)
		return NULL;
	if (!(appid = sn_startup_sequence_get_application_id(seq)))
		return NULL;
	for (a = appls; a; a = a->next)
		if (!strcmp(a->appid, appid))
			break;
	if (!a) {
		a = ecalloc(1, sizeof(*a));
		a->next = appls;
		appls = a;
		a->appid = strdup(appid);
		if ((str = sn_startup_sequence_get_name(seq)))
			a->name = strdup(str);
		if ((str = sn_startup_sequence_get_description(seq)))
			a->description = strdup(str);
		if ((str = sn_startup_sequence_get_wmclass(seq)))
			a->wmclass = strdup(str);
		if ((str = sn_startup_sequence_get_binary_name(seq)))
			a->binary = strdup(str);
		if ((str = sn_startup_sequence_get_icon_name(seq)))
			a->icon = strdup(str);
		a->members = NULL;
		a->count = 0;
	}
	return (a);
}

void
updateappl(Client *c)
{
	Appl *a;
	const char *appid;

	if (!c->seq || !(appid = sn_startup_sequence_get_application_id(c->seq))) {
		removeappl(c);
		return;
	}
	if ((a = getappl(c))) {
		if (!strcmp(a->appid, appid))
			return;
		removeappl(c);
		a = NULL;
	}
	if (!a) {
		a = findappl(c->seq);
		a->members = reallocarray(a->members, a->count + 1, sizeof(*a->members));
		a->members[a->count] = c->win;
		a->count++;
		XSaveContext(dpy, c->win, context[ClientAppl], (XPointer) a);
	}
}

void
removeappl(Client *c)
{
	Appl *a;

	if ((a = getappl(c))) {
		Window *list;
		unsigned i, j;

		list = ecalloc(a->count, sizeof(*list));
		for (i = 0, j = 0; i < a->count; i++)
			if (a->members[i] != c->win)
				list[j++] = a->members[i];
		if (j == 0) {
			free(list);
			free(a->members);
			a->members = NULL;
			a->count = 0;
			/* we never actually delete applications, just their members */
		} else {
			free(a->members);
			a->members = list;
			a->count = j;
		}
		XDeleteContext(dpy, c->win, context[ClientAppl]);
	}
}

#endif

Class *
getclass(Client *c)
{
	Class *r = NULL;

	if (c->win)
		XFindContext(dpy, c->win, context[ClientClass], (XPointer *) &r);
	return r;
}

Class *
findclass(const XClassHint *ch)
{
	Class *r;

	if (!ch->res_class || !ch->res_name)
		return NULL;
	for (r = classes; r; r = r->next)
		if (!strcmp(r->ch.res_class, ch->res_class) &&
		    !strcmp(r->ch.res_name, ch->res_name))
			break;
	if (!r) {
		r = ecalloc(1, sizeof(*r));
		r->next = classes;
		classes = r;
		r->ch.res_class = strdup(ch->res_class);
		r->ch.res_name = strdup(ch->res_name);
		r->members = NULL;
		r->count = 0;
	}
	return (r);
}

void
updateclass(Client *c)
{
	Class *r;
	
	if (!c->ch.res_class || !c->ch.res_name) {
		removeclass(c);
		return;
	}
	if ((r = getclass(c))) {
		if (!strcmp(r->ch.res_class, c->ch.res_class) &&
				!strcmp(r->ch.res_name, c->ch.res_name))
			return;
		removeclass(c);
		r = NULL;
	}
	if (!r) {
		r = findclass(&c->ch);
		r->members = reallocarray(r->members, r->count + 1, sizeof(*r->members));
		r->members[r->count] = c->win;
		r->count++;
		if (r->count > 1) {
			Client *s;

			if ((s = getmanaged(r->members[0], ClientWindow))) {
				updatetitle(s);
				updateiconname(s);
				drawclient(s); /* just for title */
			}
		}
		if (c->is.managed) {
			updatetitle(c);
			updateiconname(c);
			drawclient(c); /* just for title */
		}
		XSaveContext(dpy, c->win, context[ClientClass], (XPointer) r);
	}
}

void
removeclass(Client *c)
{
	Class *r;

	if ((r = getclass(c))) {
		Window *list;
		unsigned int i, j;

		list = ecalloc(r->count, sizeof(*list));
		for (i = 0, j = 0; i < r->count; i++)
			if (r->members[i] != c->win)
				list[j++] = r->members[i];
		if (j == 0) {
			free(list);
			free(r->members);
			r->members = NULL;
			r->count = 0;
			/* we never actually delete classes, just their members */
		} else {
			free(r->members);
			r->members = list;
			r->count = j;
			for (i = 0; i < r->count; i++) {
				Client *s;

				if ((s = getmanaged(r->members[i], ClientWindow))) {
					updatetitle(s);
					updateiconname(s);
					drawclient(s); /* just for title */
				}
			}
		}
		XDeleteContext(dpy, c->win, context[ClientClass]);
	}
}

#ifdef STARTUP_NOTIFICATION
ButtonImage *
getappbutton(Client *c)
{
	Appl *a = getappl(c);

	if (a)
		return &a->button;
	return (NULL);
}
#endif

ButtonImage *
getresbutton(Client *c)
{
	Class *r = getclass(c);

	if (r)
		return &r->button;
	return (NULL);
}

ButtonImage *
getwinbutton(Client *c)
{
	if (c)
		return &c->button;
	return (NULL);
}

ButtonImage **
getbuttons(Client *c)
{
	static ButtonImage *buttons[4] = { NULL, };
	int i = 0;

#ifdef STARTUP_NOTIFICATION
	if ((buttons[i] = getappbutton(c)))
		i++;
#endif
	if ((buttons[i] = getresbutton(c)))
		i++;
	if ((buttons[i] = getwinbutton(c)))
		i++;
	buttons[i] = NULL;
	return (buttons);
}

ButtonImage *
getbutton(Client *c)
{
	ButtonImage **bis;

	for (bis = getbuttons(c); bis && *bis; bis++)
		if ((*bis)->present)
			break;
	return (*bis);
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
	/* ICCCM 2.0/4.1.9: Window managers will ignore any WM_COLORMAP_WINDOWS
	   properties they find on icon windows. */
	if ((c->cmapwins = getcmapwins(c->win))) {
		for (p = c->cmapwins; p && *p; p++) {
			XSaveContext(dpy, *p, context[ClientColormap], (XPointer) c);
		}
	}
}

void
_xtrap_push(Bool ignore, const char *time, const char *file, int line, const char *func, const char *fmt, ...)
{
	XErrorTrap *trap;
	va_list args;
	char *msg;
	int len;

	msg = calloc(BUFSIZ + 1, sizeof(*msg));
	len = snprintf(msg, BUFSIZ, NAME ": X: [%s] %12s: +%4d : %s() : ", time, file, line, func);

	if (fmt && *fmt) {
		va_start(args, fmt);
		len += vsnprintf(msg + len, BUFSIZ - len, fmt, args);
		va_end(args);
	} else {
		len += snprintf(msg + len, BUFSIZ - len, "xerror occured during trap\n");
	}
	if ((trap = calloc(1, sizeof(*trap)))) {
		trap->next = traps;
		traps = trap;
		trap->trap_string = strdup(msg);
		XSync(dpy, False);
		trap->trap_next = NextRequest(dpy);
		trap->trap_last = LastKnownRequestProcessed(dpy);
		trap->trap_qlen = QLength(dpy);
		trap->trap_ignore = ignore;
	}
	free(msg);
}

void
_xtrap_pop(int canary)
{
	XErrorTrap *trap;

	XSync(dpy, False);
	if ((trap = traps)) {
		traps = trap->next;
		trap->next = NULL;
		free(trap->trap_string);
		trap->trap_string = NULL;
		free(trap);
	} else
		EPRINTF("_xtrap_pop() when no trap was pushed!\n");
}

Bool
xerror_critical(Display *dsply, XErrorEvent *ee, XErrorTrap *trap)
{
	Bool critical = True;

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
		critical = False;
	return (critical);
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (ebastardly on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.	*/
int
xerror(Display *dsply, XErrorEvent *ee)
{
	char msg[81] = { 0, }, req[81] = { 0,}, num[81] = { 0,};
	XErrorTrap *trap;
	Bool ignore = False, critical;

	for (trap = traps; trap; trap = trap->next) {
		if (ee->serial >= trap->trap_next) {
			if (trap->trap_string) {
				fputs(trap->trap_string, stderr);
				if (trap->trap_string[strlen(trap->trap_string)-1] != '\n')
					fputc('\n', stderr);
				fflush(stderr);
			}
			ignore = trap->trap_ignore;
			break;
		}
	}
	if (!trap)
		trap = traps;
	snprintf(num, 80, "%d", ee->request_code);
	XGetErrorDatabaseText(dsply, "XRequest", num, "", req, 80);
	if (!req[0])
		snprintf(req, 80, "[request_code=%d]", ee->request_code);
	if (XGetErrorText(dsply, ee->error_code, msg, 80) != Success)
		msg[0] = '\0';
	critical = xerror_critical(dsply, ee, trap);
	EPRINTF("X error %s(0x%lx): %s\n", req, ee->resourceid, msg);
	EPRINTF("\tResource id 0x%lx\n", ee->resourceid);
	EPRINTF("\tFailed request %lu\n", ee->serial);
	if (trap)
		EPRINTF("\tNext request trap %lu\n", trap->trap_next);
		EPRINTF("\tNext request now  %lu\n", NextRequest(dsply));
	if (trap)
		EPRINTF("\tLast known processed request trap %lu\n", trap->trap_last);
		EPRINTF("\tLast known processed request now  %lu\n", LastKnownRequestProcessed(dsply));
	if (critical || ignore)
		dumpstack(__FILE__, __LINE__, __func__);
	if (!critical || ignore) {
		return 0;
	}
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
	XPRINTF("attempting to dlopen %s\n", dlfile);
	if ((handle = dlopen(dlfile, RTLD_NOW | RTLD_LOCAL))) {
		XPRINTF("dlopen of %s succeeded\n", dlfile);
		if ((ops = dlsym(handle, "adwm_ops")))
			ops->handle = handle;
		else
			XPRINTF("could not find symbol adwm_ops");
	} else
		XPRINTF("dlopen of %s failed: %s\n", dlfile, dlerror());
	return ops;
}

static void
copying(int argc, char *argv[])
{
	if (!options.output && !options.debug)
		return;
	(void) fprintf(stdout, "\
--------------------------------------------------------------------------------\n\
%1$s\n\
--------------------------------------------------------------------------------\n\
Copyright (c) 2010-2018  Monavacon Limited <http://www.monavacon.com/>\n\
Copyright (c) 2002-2009  OpenSS7 Corporation <http://www.openss7.com/>\n\
Copyright (c) 1997-2001  Brian F. G. Bidulock <bidulock@openss7.org>\n\
\n\
All Rights Reserved.\n\
--------------------------------------------------------------------------------\n\
This program is free software: you can  redistribute it  and/or modify  it under\n\
the terms of the  GNU  General Public License  as published by the Free Software\n\
Foundation, version 3 of the license.\n\
\n\
This program is distributed in the hope that it will  be useful, but WITHOUT ANY\n\
WARRANTY; without even  the implied warranty of MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.  See the GNU General Public License for more details.\n\
\n\
You should have received  a copy of the  GNU  General Public License  along with\n\
this program.   If not, see <http://www.gnu.org/licenses/>, or write to the\n\
Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\
--------------------------------------------------------------------------------\n\
U.S. GOVERNMENT RESTRICTED RIGHTS.  If you are licensing this Software on behalf\n\
of the U.S. Government (\"Government\"), the following provisions apply to you. If\n\
the Software is supplied by the Department of Defense (\"DoD\"), it is classified\n\
as \"Commercial  Computer  Software\"  under  paragraph  252.227-7014  of the  DoD\n\
Supplement  to the  Federal Acquisition Regulations  (\"DFARS\") (or any successor\n\
regulations) and the  Government  is acquiring  only the  license rights granted\n\
herein (the license rights customarily provided to non-Government users). If the\n\
Software is supplied to any unit or agency of the Government  other than DoD, it\n\
is  classified as  \"Restricted Computer Software\" and the Government's rights in\n\
the Software  are defined  in  paragraph 52.227-19  of the  Federal  Acquisition\n\
Regulations (\"FAR\")  (or any successor regulations) or, in the cases of NASA, in\n\
paragraph  18.52.227-86 of  the  NASA  Supplement  to the FAR (or any  successor\n\
regulations).\n\
--------------------------------------------------------------------------------\n\
", NAME " " VERSION);
}

static void
version(int argc, char *argv[])
{
	if (!options.output && !options.debug)
		return;
	(void) fprintf(stdout, "\
%1$s-%3$s (OpenSS7 %2$s)\n\
Written by Brian Bidulock.\n\
\n\
Copyright (c) 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018  Monavacon Limited.\n\
Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009  OpenSS7 Corporation.\n\
Copyright (c) 1997, 1998, 1999, 2000, 2001  Brian F. G. Bidulock.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
\n\
Distributed by OpenSS7 under GNU General Public License Version 3,\n\
with conditions, incorporated herein by reference.\n\
\n\
See `%1$s --copying' for copying permissions.\n\
", NAME, PACKAGE, VERSION);
}

void
usage(int argc, char *argv[])
{
	if (!options.output && !options.debug)
		return;
	(void) fprintf(stderr, "\
Usage:\n\
    %1$s [{-f|--file} {PATH/}RCFILE]\n\
    %1$s {-h|--help}\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
", argv[0]);
}

void
help(int argc, char *argv[])
{
	if (!options.output && !options.debug)
		return;
	(void) fprintf(stdout, "\
Usage:\n\
    %1$s [{-f|--file} {PATH/}RCFILE]\n\
    %1$s {-h|--help}\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
Options:\n\
    -f, --file {PATH/}RCFILE [default: 'adwmrc']\n\
        config file name, RCFILE, or abs path and name, PATH/RCFILE\n\
    -h, --help, -?, --?\n\
        print this usage information and exit\n\
    -v, --version\n\
        print version and exit\n\
    -C, --copying\n\
        print copying permission and exit\n\
    -D, --debug [LEVEL]\n\
        increment or set debug LEVEL [default: '%2$d']\n\
    -V, --verbose [LEVEL]\n\
        increment or set output verbosity LEVEL [default: '%3$d']\n\
        this option may be repeated.\n\
", argv[0], options.debug, options.output);
}

int
main(int argc, char *argv[])
{
	char conf[PATH_MAX + 1] = { 0, }, *p;
	int i;

	/* don't care about job control */
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

	signal(SIGHUP,  sighandler);
	signal(SIGINT,  sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGCHLD, sighandler);

	setlocale(LC_CTYPE, "");

	options.debug = 0;
	options.output = 1;

	while (1) {
		int c, val;
		char *endptr = NULL;
#ifdef _GNU_SOURCE
		int option_index = 0;
		/* *INDENT-OFF* */
		static struct option long_options[] = {
			{"file",	required_argument,	NULL, 'f'},

			{"debug",	optional_argument,	NULL, 'D'},
			{"verbose",	optional_argument,	NULL, 'V'},
			{"help",	no_argument,		NULL, 'h'},
			{"version",	no_argument,		NULL, 'v'},
			{"copying",	no_argument,		NULL, 'C'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "D::V::hvC", long_options, &option_index);
#else				/* defined _GNU_SOURCE */
		c = getopt(argc, argv, "DVhvC");
#endif				/* defined _GNU_SOURCE */
		if (c == -1) {
			if (options.debug)
				fprintf(stderr, "%s: done options processing\n", argv[0]);
			break;
		}
		switch (c) {
		case 0:
			goto bad_usage;
		case 'f':	/* -f, --file {PATH/}FILE */
			snprintf(conf, sizeof(conf) - 1, optarg);
			break;
		case 'D':	/* -D, --debug [level] */
			if (options.debug)
				fprintf(stderr, "%s: increasing debug verbosity\n", argv[0]);
			if (optarg == NULL) {
				options.debug++;
				break;
			}
			val = strtoul(optarg, &endptr, 0);
			if (*endptr)
				goto bad_option;
			options.debug = val;
			break;
		case 'V':	/* -V, --verbose [level] */
			if (options.debug)
				fprintf(stderr, "%s: increasing output verbosity\n", argv[0]);
			if (optarg == NULL) {
				options.output++;
				break;
			}
			val = strtoul(optarg, &endptr, 0);
			if (*endptr)
				goto bad_option;
			options.output = val;
			break;
		case 'h':	/* -h, --help */
			help(argc, argv);
			exit(EXIT_SUCCESS);
		case 'v':	/* -v, --version */
			version(argc, argv);
			exit(EXIT_SUCCESS);
		case 'C':	/* -C, --copying */
			copying(argc, argv);
			exit(EXIT_SUCCESS);
		default:
		      bad_option:
			optind--;
		      bad_nonopt:
			if (options.output || options.debug) {
				if (optind < argc) {
					fprintf(stderr, "%s: syntax error near '", argv[0]);
					while (optind < argc)
						fprintf(stderr, "%s ", argv[optind++]);
					fprintf(stderr, "'\n");
				} else {
					fprintf(stderr, "%s: missing option or argument", argv[0]);
					fprintf(stderr, "\n");
				}
				fflush(stderr);
			      bad_usage:
				usage(argc, argv);
			}
			exit(2);
		}
	}
	if (optind > argc)
		goto bad_nonopt;
#if 0
	if (argc == 3 && !strcmp("-f", argv[1]))
		snprintf(conf, sizeof(conf), "%s", argv[2]);
	else if (argc == 2 && !strcmp("-v", argv[1])) {
		fprintf(stdout, "adwm-" VERSION " (c) 2018 Brian Bidulock\n");
		exit(0);
	} else if (argc != 1)
		eprint("%s", "usage: adwm [-v] [-f conf]\n");
#endif
	cargc = argc;
	cargv = argv;

	if (!(dpy = XOpenDisplay(0)))
		eprint("%s", "adwm: cannot open display\n");

	setsid();
#ifdef __linux__
#ifdef PR_SET_CHILD_SUBREAPER
	prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0);
#else
#warn "Not using prctl!"
#endif
#else
#warn "Not using prctl!"
#endif
	if (!(baseops = get_adwm_ops("adwm")))
		eprint("%s", "could not load base operations\n");

	/* create contexts */
	for (i = 0; i < PartLast; i++)
		context[i] = XUniqueContext();
	/* query extensions */
	for (i = 0; i < BaseLast; i++) {
		einfo[i].have = XQueryExtension(dpy, einfo[i].name, &einfo[i].opcode, &einfo[i].event, &einfo[i].error);
		if (einfo[i].have) {
			OPRINTF("have %s extension (%d,%d,%d)\n", einfo[i].name, einfo[i].opcode, einfo[i].event, einfo[i].error);
			if (einfo[i].version) {
				einfo[i].version(dpy, &einfo[i].major, &einfo[i].minor);
				OPRINTF("have %-10s extension version %d.%d\n", einfo[i].name, einfo[i].major, einfo[i].minor);
			}
		} else
			OPRINTF("%s extension is not supported\n", einfo[i].name);
	}
	nscr = ScreenCount(dpy);
	OPRINTF("there are %u screens\n", nscr);
	screens = calloc(nscr, sizeof(*screens));
	for (i = 0, scr = screens; i < nscr; i++, scr++) {
		scr->screen = i;
		scr->root = RootWindow(dpy, i);
		XSaveContext(dpy, scr->root, context[ScreenContext], (XPointer) scr);
		OPRINTF("screen %d has root 0x%lx\n", scr->screen, scr->root);
		initimage();
	}
	if ((p = getenv("DISPLAY")) && (p = strrchr(p, '.')) && strlen(p + 1)
	    && strspn(p + 1, "0123456789") == strlen(p + 1) && (i = atoi(p + 1)) < nscr) {
		OPRINTF("managing one screen: %d\n", i);
		screens[i].managed = True;
	} else
		for (scr = screens; scr < screens + nscr; scr++)
			scr->managed = True;
	for (scr = screens; scr < screens + nscr; scr++)
		if (scr->managed) {
			OPRINTF("checking screen %d\n", scr->screen);
			checkotherwm();
		}
	for (scr = screens; scr < screens + nscr && !scr->managed; scr++) ;
	if (scr == screens + nscr)
		eprint("%s", "adwm: another window manager is already running on each screen\n");
	setup(conf, baseops);
	for (scr = screens; scr < screens + nscr; scr++)
		if (scr->managed) {
			OPRINTF("scanning screen %d\n", scr->screen);
			scan();
		}
	OPRINTF("%s", "entering main event loop\n");
	run();
	OPRINTF("cleanup quitting\n");
	cleanup(CauseQuitting);

	OPRINTF("closing display\n");
	XCloseDisplay(dpy);
	return 0;
}
