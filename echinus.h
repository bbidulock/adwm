/* enums */

#ifdef STARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>
#endif

#ifdef IMLIB2
#include <Imlib2.h>
#endif

enum {
	Manager, Utf8String, WMProto, WMDelete, WMSaveYourself, WMState, WMChangeState,
	WMTakeFocus,
	ELayout, ESelTags, WMRestart, WMShutdown, DeskLayout,
	/* MWM/DTWM properties follow */
	WMDesktop, MWMBindings, MWMDefaultBindings, MWMMessages, MWMOffset, MWMHints, MWMMenu,
	MWMInfo, DTWorkspaceHints, DTWorkspacePresence, DTWorkspaceList, DTWorkspaceCurrent,
	DTWorkspaceInfo, DTWMHints, DTWMRequest, DTWMEmbedded, DTWMSaveHint,
	/* _WIN_PROTOCOLS following */
	WinAppState, WinAreaCount, WinArea, WinClientList, WinClientMoving,
	WinButtonProxy, WinExpandedSize, WinFocus, WinHints, WinIcons, WinLayer, WinMaxGeom,
	WinProtocols, WinResize, WinState, WinCheck, WinWorkarea, WinWorkCount, WinWorkNames,
	WinWorkspace, WinWorkspaces, SwmVroot,
	/* _NET_SUPPORTED following */
	ClientList, ActiveWindow, WindowDesk, WindowDeskMask, NumberOfDesk, DeskNames,
	CurDesk, WorkArea, DeskViewport, ShowingDesktop, DeskGeometry, DesksVisible,
	MonitorGeometry,
	DeskModes, DeskModeFloating, DeskModeTiled, DeskModeBottomTiled, DeskModeMonocle,
	DeskModeTopTiled, DeskModeLeftTiled,
	ClientListStacking, WindowOpacity, MoveResizeWindow, RestackWindow, WindowMoveResize,
	WindowExtents, HandledIcons, RequestFrameExt, VirtualRoots,
	WindowType, WindowTypeDesk, WindowTypeDock, WindowTypeToolbar, WindowTypeMenu,
	WindowTypeUtil, WindowTypeSplash, WindowTypeDialog, WindowTypeDrop,
	WindowTypePopup, WindowTypeTooltip, WindowTypeNotify, WindowTypeCombo,
	WindowTypeDnd, WindowTypeNormal,
	StrutPartial, Strut, WindowPid, WindowName, WindowNameVisible, WindowIconName,
	WindowIconNameVisible, WindowUserTime, UserTimeWindow, NetStartupId,
	StartupInfo, StartupInfoBegin,
	WindowSync, WindowCounter, WindowFsMonitors,
	WindowState, WindowStateModal, WindowStateSticky, WindowStateMaxV,
	WindowStateMaxH, WindowStateShaded, WindowStateNoTaskbar, WindowStateNoPager,
	WindowStateHidden, WindowStateFs, WindowStateAbove, WindowStateBelow,
	WindowStateAttn, WindowStateFocused, WindowStateFixed, WindowStateFloating,
	WindowStateFilled,
	WindowActions, WindowActionAbove, WindowActionBelow, WindowActionChangeDesk,
	WindowActionClose, WindowActionFs, WindowActionMaxH, WindowActionMaxV,
	WindowActionMin, WindowActionMove, WindowActionResize, WindowActionShade,
	WindowActionStick, WindowActionFloat, WindowActionFill,
	WMCheck, CloseWindow, WindowPing, Supported,
	SystemTrayWindows, WindowFrameStrut, WindowForSysTray, WindowTypeOverride,
	KdeSplashProgress, WindowChangeState,
	NATOMS
}; /* keep in sync with atomnames[] in ewmh.c */

#define WTFLAG(_type) (1<<((_type)-WindowTypeDesk))
#define WTTEST(_wintype, _type) (((_wintype) & WTFLAG(_type)) ? True : False)
#define WTCHECK(_client, _type) WTTEST((_client)->wintype, (_type))

#define _XA_MANAGER				atom[Manager]
#define _XA_UTF8_STRING				atom[Utf8String]
#define _XA_WM_PROTOCOLS			atom[WMProto]
#define _XA_WM_DELETE_WINDOW			atom[WMDelete]
#define _XA_WM_SAVE_YOURSELF			atom[WMSaveYourself]
#define _XA_WM_STATE				atom[WMState]
#define _XA_WM_CHANGE_STATE			atom[WMChangeState]
#define _XA_WM_TAKE_FOCUS			atom[WMTakeFocus]
#define _XA_ECHINUS_LAYOUT			atom[ELayout]
#define _XA_ECHINUS_SELTAGS			atom[ESelTags]
#define _XA_NET_RESTART				atom[WMRestart]
#define _XA_NET_SHUTDOWN			atom[WMShutdown]
#define _XA_NET_DESKTOP_LAYOUT			atom[DeskLayout]
/* MWM/DTWM properties follow */
#define _XA_WM_DESKTOP				atom[WMDesktop]
#define _XA_MOTIF_BINDINGS			atom[MWMBindings]
#define _XA_MOTIF_DEFAULT_BINDINGS		atom[MWMDefaultBindings]
#define _XA_MOTIF_WM_MESSAGES			atom[MWMMessages]
#define _XA_MOTIF_WM_OFFSET			atom[MWMOffset]
#define _XA_MOTIF_WM_HINTS			atom[MWMHints]
#define _XA_MOTIF_WM_MENU			atom[MWMMenu]
#define _XA_MOTIF_WM_INFO			atom[MWMInfo]
#define _XA_DT_WORKSPACE_HINTS			atom[DTWorkspaceHints]
#define _XA_DT_WORKSPACE_PRESENCE		atom[DTWorkspacePresence]
#define _XA_DT_WORKSPACE_LIST			atom[DTWorkspaceList]
#define _XA_DT_WORKSPACE_CURRENT		atom[DTWorkspaceCurrent]
#define _XA_DT_WORKSPACE_INFO			atom[DTWorkspaceInfo]
#define _XA_DT_WM_HINTS				atom[DTWMHints]
#define _XA_DT_WM_REQUEST			atom[DTWMRequest]
#define _XA_DT_WORKSPACE_EMBEDDED_CLIENTS	atom[DTWMEmbedded]
#define _XA_DT_WMSAVE_HINT			atom[DTWMSaveHint]
/* _WIN_PROTOCOLS following */
#define _XA_WIN_APP_STATE			atom[WinAppState]
#define _XA_WIN_AREA_COUNT			atom[WinAreaCount]
#define _XA_WIN_AREA				atom[WinArea]
#define _XA_WIN_CLIENT_LIST			atom[WinClientList]
#define _XA_WIN_CLIENT_MOVING			atom[WinClientMoving]
#define _XA_WIN_DESKTOP_BUTTON_PROXY		atom[WinButtonProxy]
#define _XA_WIN_EXPANDED_SIZE			atom[WinExpandedSize]
#define _XA_WIN_FOCUS				atom[WinFocus]
#define _XA_WIN_HINTS				atom[WinHints]
#define _XA_WIN_ICONS				atom[WinIcons]
#define _XA_WIN_LAYER				atom[WinLayer]
#define _XA_WIN_MAXIMIZED_GEOMETRY		atom[WinMaxGeom]
#define _XA_WIN_PROTOCOLS			atom[WinProtocols]
#define _XA_WIN_RESIZE				atom[WinResize]
#define _XA_WIN_STATE				atom[WinState]
#define _XA_WIN_SUPPORTING_WM_CHECK		atom[WinCheck]
#define _XA_WIN_WORKAREA			atom[WinWorkarea]
#define _XA_WIN_WORKSPACE_COUNT			atom[WinWorkCount]
#define _XA_WIN_WORKSPACE_NAMES			atom[WinWorkNames]
#define _XA_WIN_WORKSPACE			atom[WinWorkspace]
#define _XA_WIN_WORKSPACES			atom[WinWorkspaces]
#define _XA_SWM_VROOT				atom[SwmVroot]
/* _NET_SUPPORTED following */
#define _XA_NET_CLIENT_LIST			atom[ClientList]
#define _XA_NET_ACTIVE_WINDOW			atom[ActiveWindow]
#define _XA_NET_WM_DESKTOP			atom[WindowDesk]
#define _XA_NET_WM_DESKTOP_MASK			atom[WindowDeskMask]
#define _XA_NET_NUMBER_OF_DESKTOPS		atom[NumberOfDesk]
#define _XA_NET_DESKTOP_NAMES			atom[DeskNames]
#define _XA_NET_CURRENT_DESKTOP			atom[CurDesk]
#define _XA_NET_WORKAREA			atom[WorkArea]
#define _XA_NET_DESKTOP_VIEWPORT		atom[DeskViewport]
#define _XA_NET_SHOWING_DESKTOP			atom[ShowingDesktop]
#define _XA_NET_DESKTOP_GEOMETRY		atom[DeskGeometry]
#define _XA_NET_VISIBLE_DESKTOPS		atom[DesksVisible]
#define _XA_NET_MONITOR_GEOMETRY		atom[MonitorGeometry]

#define _XA_NET_DESKTOP_MODES			atom[DeskModes]
#define _XA_NET_DESKTOP_MODE_FLOATING		atom[DeskModeFloating]
#define _XA_NET_DESKTOP_MODE_TILED		atom[DeskModeTiled]
#define _XA_NET_DESKTOP_MODE_BOTTOM_TILED	atom[DeskModeBottomTiled]
#define _XA_NET_DESKTOP_MODE_MONOCLE		atom[DeskModeMonocle]
#define _XA_NET_DESKTOP_MODE_TOP_TILED		atom[DeskModeTopTiled]
#define _XA_NET_DESKTOP_MODE_LEFT_TILED		atom[DeskModeLeftTiled]

#define _XA_NET_CLIENT_LIST_STACKING		atom[ClientListStacking]
#define _XA_NET_WM_WINDOW_OPACITY		atom[WindowOpacity]
#define _XA_NET_MOVERESIZE_WINDOW		atom[MoveResizeWindow]
#define _XA_NET_RESTACK_WINDOW			atom[RestackWindow]
#define _XA_NET_WM_MOVERESIZE			atom[WindowMoveResize]
#define _XA_NET_FRAME_EXTENTS			atom[WindowExtents]
#define _XA_NET_WM_HANDLED_ICONS		atom[HandledIcons]
#define _XA_NET_REQUEST_FRAME_EXTENTS		atom[RequestFrameExt]
#define _XA_NET_VIRTUAL_ROOTS			atom[VirtualRoots]

#define _XA_NET_WM_WINDOW_TYPE			atom[WindowType]
#define _XA_NET_WM_WINDOW_TYPE_DESKTOP		atom[WindowTypeDesk]
#define _XA_NET_WM_WINDOW_TYPE_DOCK		atom[WindowTypeDock]
#define _XA_NET_WM_WINDOW_TYPE_TOOLBAR		atom[WindowTypeToolbar]
#define _XA_NET_WM_WINDOW_TYPE_MENU		atom[WindowTypeMenu]
#define _XA_NET_WM_WINDOW_TYPE_UTILITY		atom[WindowTypeUtil]
#define _XA_NET_WM_WINDOW_TYPE_SPLASH		atom[WindowTypeSplash]
#define _XA_NET_WM_WINDOW_TYPE_DIALOG		atom[WindowTypeDialog]
#define _XA_NET_WM_WINDOW_TYPE_DROPDOWN_MENU	atom[WindowTypeDrop]
#define _XA_NET_WM_WINDOW_TYPE_POPUP_MENU	atom[WindowTypePopup]
#define _XA_NET_WM_WINDOW_TYPE_TOOLTIP		atom[WindowTypeTooltip]
#define _XA_NET_WM_WINDOW_TYPE_NOTIFICATION	atom[WindowTypeNotify]
#define _XA_NET_WM_WINDOW_TYPE_COMBO		atom[WindowTypeCombo]
#define _XA_NET_WM_WINDOW_TYPE_DND		atom[WindowTypeDnd]
#define _XA_NET_WM_WINDOW_TYPE_NORMAL		atom[WindowTypeNormal]

#define _XA_NET_WM_STRUT_PARTIAL		atom[StrutPartial]
#define _XA_NET_WM_STRUT			atom[Strut]
#define _XA_NET_WM_PID				atom[WindowPid]
#define _XA_NET_WM_NAME				atom[WindowName]
#define _XA_NET_WM_VISIBLE_NAME			atom[WindowNameVisible]
#define _XA_NET_WM_ICON_NAME			atom[WindowIconName]
#define _XA_NET_WM_VISIBLE_ICON_NAME		atom[WindowIconNameVisible]
#define _XA_NET_WM_USER_TIME			atom[WindowUserTime]
#define _XA_NET_WM_USER_TIME_WINDOW		atom[UserTimeWindow]
#define _XA_NET_STARTUP_ID			atom[NetStartupId]
#define _XA_NET_STARTUP_INFO			atom[StartupInfo]
#define _XA_NET_STARTUP_INFO_BEGIN		atom[StartupInfoBegin]
#define _XA_NET_WM_SYNC_REQUEST			atom[WindowSync]
#define _XA_NET_WM_SYNC_REQUEST_COUNTER		atom[WindowCounter]
#define _XA_NET_WM_FULLSCREEN_MONITORS		atom[WindowFsMonitors]

#define _XA_NET_WM_STATE			atom[WindowState]
#define _XA_NET_WM_STATE_MODAL			atom[WindowStateModal]
#define _XA_NET_WM_STATE_STICKY			atom[WindowStateSticky]
#define _XA_NET_WM_STATE_MAXIMIZED_VERT		atom[WindowStateMaxV]
#define _XA_NET_WM_STATE_MAXIMIZED_HORZ		atom[WindowStateMaxH]
#define _XA_NET_WM_STATE_SHADED			atom[WindowStateShaded]
#define _XA_NET_WM_STATE_SKIP_TASKBAR		atom[WindowStateNoTaskbar]
#define _XA_NET_WM_STATE_SKIP_PAGER		atom[WindowStateNoPager]
#define _XA_NET_WM_STATE_HIDDEN			atom[WindowStateHidden]
#define _XA_NET_WM_STATE_FULLSCREEN		atom[WindowStateFs]
#define _XA_NET_WM_STATE_ABOVE			atom[WindowStateAbove]
#define _XA_NET_WM_STATE_BELOW			atom[WindowStateBelow]
#define _XA_NET_WM_STATE_DEMANDS_ATTENTION	atom[WindowStateAttn]
#define _XA_NET_WM_STATE_FOCUSED		atom[WindowStateFocused]
#define _XA_NET_WM_STATE_FIXED			atom[WindowStateFixed]
#define _XA_NET_WM_STATE_FLOATING		atom[WindowStateFloating]
#define _XA_NET_WM_STATE_FILLED			atom[WindowStateFilled]

#define _XA_NET_WM_ALLOWED_ACTIONS		atom[WindowActions]
#define _XA_NET_WM_ACTION_ABOVE			atom[WindowActionAbove]
#define _XA_NET_WM_ACTION_BELOW			atom[WindowActionBelow]
#define _XA_NET_WM_ACTION_CHANGE_DESKTOP	atom[WindowActionChangeDesk]
#define _XA_NET_WM_ACTION_CLOSE			atom[WindowActionClose]
#define _XA_NET_WM_ACTION_FULLSCREEN		atom[WindowActionFs]
#define _XA_NET_WM_ACTION_MAXIMIZE_HORZ		atom[WindowActionMaxH]
#define _XA_NET_WM_ACTION_MAXIMIZE_VERT		atom[WindowActionMaxV]
#define _XA_NET_WM_ACTION_MINIMIZE		atom[WindowActionMin]
#define _XA_NET_WM_ACTION_MOVE			atom[WindowActionMove]
#define _XA_NET_WM_ACTION_RESIZE		atom[WindowActionResize]
#define _XA_NET_WM_ACTION_SHADE			atom[WindowActionShade]
#define _XA_NET_WM_ACTION_STICK			atom[WindowActionStick]
#define _XA_NET_WM_ACTION_FLOAT			atom[WindowActionFloat]
#define _XA_NET_WM_ACTION_FILL			atom[WindowActionFill]

#define _XA_NET_SUPPORTING_WM_CHECK		atom[WMCheck]
#define _XA_NET_CLOSE_WINDOW			atom[CloseWindow]
#define _XA_NET_WM_PING				atom[WindowPing]
#define _XA_NET_SUPPORTED			atom[Supported]

#define _XA_KDE_NET_SYSTEM_TRAY_WINDOWS		atom[SystemTrayWindows]
#define _XA_KDE_NET_WM_FRAME_STRUT		atom[WindowFrameStrut]
#define _XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR	atom[WindowForSysTray]
#define _XA_KDE_NET_WM_WINDOW_TYPE_OVERRIDE	atom[WindowTypeOverride]
#define _XA_KDE_SPLASH_PROGRESS			atom[KdeSplashProgress]
#define _XA_KDE_WM_CHANGE_STATE			atom[WindowChangeState]

enum { LeftStrut, RightStrut, TopStrut, BotStrut, LastStrut }; /* ewmh struts */
enum { ColFG, ColBG, ColBorder, ColButton, ColLast };	/* colors */
enum { ClientWindow, ClientTitle, ClientFrame, ClientTimeWindow, ClientGroup,
       ClientTransFor, ClientTransForGroup, ClientLeader, ClientAny, SysTrayWindows,
       ClientPing, ClientDead, ScreenContext, PartLast };	/* client parts */
enum { Iconify, Maximize, Close, LastBtn }; /* window buttons */
typedef enum { CauseDestroyed, CauseUnmapped, CauseReparented,
	       CauseQuitting, CauseSwitching, CauseRestarting } WithdrawCause;
typedef enum { ColSmartPlacement, RowSmartPlacement, MinOverlapPlacement,
	       UnderMousePlacement, CascadePlacement, RandomPlacement } WindowPlacement;
typedef enum { ModalModeless, ModalPrimary, ModalSystem, ModalGroup } Modality;
typedef enum { NoInputModel, PassiveInputModel, GloballyActiveModel, LocallyActiveModel } InputModel;
typedef enum { OrientLeft, OrientTop, OrientRight, OrientBottom, OrientLast } LayoutOrientation;
typedef enum { StrutsOn, StrutsOff, StrutsHide } StrutsPosition;

#define GIVE_FOCUS (1<<0)
#define TAKE_FOCUS (1<<1)

/* typedefs */
typedef struct {
	int x, y, w, h, b, n;
} LayoutArgs;

typedef struct {
	int x, y, w, h, b;
} Geometry;

typedef struct {
	int x, y, w, h;
} Workarea;

typedef struct Monitor Monitor;
struct Monitor {
	Workarea sc, wa;
	unsigned long struts[LastStrut];
	Bool *seltags;
	Bool *prevtags;
	Monitor *next;
	int mx, my;
	unsigned int curtag;
	unsigned int num, index;
	Window veil;
};

typedef struct {
	void (*arrange) (Monitor *m);
	char symbol;
#define MWFACT		(1<<0)	/* adjust master factor */
#define NMASTER		(1<<1)	/* adjust number of masters */
#define	ZOOM		(1<<2)	/* adjust zoom */
#define	OVERLAP		(1<<3)	/* floating layout */
#define NCOLUMNS	(1<<4)	/* adjust number of columns */
#define ROTL		(1<<5)	/* adjust rotation */
	int features;
	LayoutOrientation major;	/* overall orientation */
	LayoutOrientation minor;	/* master area orientation */
	WindowPlacement placement;	/* float placement policy */
} Layout;

#define FEATURES(_layout, _which) (!(!((_layout)->features & (_which))))
#define M2LT(_mon) (scr->views[(_mon)->curtag].layout)
#define MFEATURES(_monitor, _which) ((_monitor) && FEATURES(M2LT(_monitor), (_which)))

typedef struct Client Client;
struct Client {
	char *name;
	char *icon_name;
	char *startup_id;
	int monitor;			/* initial monitor */
	Geometry c, r, s;		/* current, restore, static */
	int th, tw;			/* title height and width */
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int minax, maxax, minay, maxay, gravity;
	int ignoreunmap;
	long flags;
	int wintype;
	Bool wasfloating;
	Bool *tags;
	int nonmodal;
	union {
		struct {
			unsigned int taskbar:1;
			unsigned int pager:1;
			unsigned int winlist:1;
			unsigned int cycle:1;
			unsigned int focus:1;
			unsigned int arrange:1;
			unsigned int sloppy:1;
		};
		unsigned int skip;
	} skip;
	union {
		struct {
			unsigned int transient:1;
			unsigned int grptrans:1;
			unsigned int banned:1;
			unsigned int max:1;
			unsigned int floater:1;
			unsigned int maxv:1;
			unsigned int maxh:1;
			unsigned int shaded:1;
			unsigned int icon:1;
			unsigned int fill:1;
			unsigned int modal:2;
			unsigned int above:1;
			unsigned int below:1;
			unsigned int attn:1;
			unsigned int sticky:1;
			unsigned int hidden:1;
			unsigned int bastard:1;
			unsigned int fs:1;
			unsigned int managed:1;
		};
		unsigned int is;
	} is, was;
	union {
		struct {
			unsigned int border:1;
			unsigned int handles:1;
			unsigned int title:1;
			struct {
				unsigned int menu:1;
				unsigned int min:1;
				unsigned int max:1;
				unsigned int close:1;
				unsigned int size:1;
				unsigned int shade:1;
				unsigned int stick:1;
				unsigned int fill:1;
				unsigned int floats:1;
			} but __attribute__ ((packed));
		};
		unsigned int has;
	} has;
	union {
		struct {
			unsigned int struts:1;
			unsigned int time:1;
		};
		unsigned int with;
	} with;
	union {
		struct {
			unsigned int move:1;
			unsigned int size:1;
			unsigned int sizev:1;
			unsigned int sizeh:1;
			unsigned int min:1;
			unsigned int max:1;
			unsigned int maxv:1;
			unsigned int maxh:1;
			unsigned int close:1;
			unsigned int shade:1;
			unsigned int stick:1;
			unsigned int fs:1;
			unsigned int above:1;
			unsigned int below:1;
			unsigned int fill:1;
			unsigned int fillh:1;
			unsigned int fillv:1;
			unsigned int floats:1;
			unsigned int hide:1;
			unsigned int tag:1;
			unsigned int arrange:1;
			unsigned int focus:2;
		};
		unsigned int can;
	} can;
	Client *next;
	Client *prev;
	Client *snext;
	Client *cnext;
	Window win;
	Window title;
	Window frame;
	Window time_window;
	Window leader;
	Window transfor;
	Time user_time;
	Pixmap drawable;
	XftDraw *xftdraw;
	XID sync;
#ifdef STARTUP_NOTIFICATION
	SnStartupSequence *seq;
#endif
};

typedef struct Group Group;
struct Group {
	Window *members;
	unsigned int count;
	int modal_transients;
};

typedef struct View {
	StrutsPosition barpos;		/* show struts? */
	Bool dectiled;			/* decorate tiled? */
	int nmaster;
	int ncolumns;
	double mwfact;			/* master width factor */
	double mhfact;			/* master height factor */
	LayoutOrientation major;	/* overall orientation */
	LayoutOrientation minor;	/* master area orientation */
	WindowPlacement placement;	/* float placement policy */
	Layout *layout;
} View; /* per-tag settings */

typedef struct {
	unsigned int x, y, w, h;
	struct {
		XGlyphInfo *extents;
		int ascent;
		int descent;
		int height;
		int width;
	} font;
	GC gc;
} DC;				/* draw context */

typedef struct {
#if defined IMLIB2 || defined XPM
	Imlib_Image image;
	Pixmap pixmap, mask;
#endif
	Pixmap bitmap;
	int px, py;
	unsigned int pw, ph, po;
	int x;
	int pressed;
	void (*action) (const char *arg);
} Button; /* window buttons */

typedef struct {
	unsigned int border;
	unsigned int margin;
	unsigned int outline;
	unsigned int titleheight;
	unsigned int opacity;
	char titlelayout[32];
	XftFont *font;
	struct {
		unsigned long norm[ColLast];
		unsigned long sel[ColLast];
		XftColor *font[2];
	} color;
} Style;

typedef struct {
	unsigned long mod;
	KeySym keysym;
	void (*func) (const char *arg);
	const char *arg;
} Key; /* keyboard shortcuts */

typedef struct EScreen EScreen;
struct EScreen {
	Bool managed;
	Window root;
	Window selwin;
	Client *clients;
	Monitor *monitors;
	unsigned int nmons;
	Client *stack;
	Client *clist;
	Bool showing_desktop;
	int screen;
	char **tags;
	unsigned int ntags;
	Atom *dt_tags;
	View *views;
	Key **keys;
	unsigned int nkeys;
	DC dc;
	Button button[LastBtn];
	Style style;
#ifdef IMLIB2
	Imlib_Context context;
#endif
};

typedef struct {
	char *prop;
	char *tags;
	Bool isfloating;
	Bool hastitle;
	regex_t *propregex;
	regex_t *tagregex;
} Rule; /* window matching rules */

#ifdef STARTUP_NOTIFICATION
typedef struct Notify Notify;
struct Notify {
	Notify *next;
	SnStartupSequence *seq;
	Bool assigned;
};
#endif

/* ewmh.c */
Bool checkatom(Window win, Atom bigatom, Atom smallatom);
unsigned int getwintype(Window win);
Bool checkwintype(Window win, int wintype);
void clientmessage(XEvent * e);
void ewmh_release_user_time_window(Client *c);
Atom *getatom(Window win, Atom atom, unsigned long *nitems);
long *getcard(Window win, Atom atom, unsigned long *nitems);
void initewmh(Window w);
void exitewmh(WithdrawCause cause);
void ewmh_add_client(Client *c);
void ewmh_del_client(Client *c, WithdrawCause cause);
void setopacity(Client * c, unsigned int opacity);
int getstruts(Client * c);

void ewmh_process_net_desktop_names(void);
void ewmh_process_net_number_of_desktops(void);
void ewmh_process_net_showing_desktop(void);
void ewmh_update_kde_splash_progress(void);
void ewmh_update_net_active_window(void);
void ewmh_update_net_client_list(void);
void ewmh_update_net_current_desktop(void);
void ewmh_update_net_desktop_geometry(void);
void ewmh_update_net_desktop_modes(void);
void ewmh_update_net_number_of_desktops(void);
void ewmh_update_net_showing_desktop(void);
void ewmh_update_net_virtual_roots(void);
void ewmh_update_net_work_area(void);

void mwmh_process_motif_wm_hints(Client *);

Bool ewmh_process_net_window_desktop(Client *);
Bool ewmh_process_net_window_desktop_mask(Client *);

void ewmh_process_net_window_type(Client *);
void ewmh_process_kde_net_window_type_override(Client *);
void ewmh_process_net_startup_id(Client *);
void ewmh_process_net_window_state(Client *c);
void ewmh_process_net_window_sync_request_counter(Client *);
void ewmh_process_net_window_user_time(Client *);
void ewmh_process_net_window_user_time_window(Client *);
void ewmh_update_net_window_desktop(Client *);
void ewmh_update_net_window_extents(Client *);
void ewmh_update_net_window_fs_monitors(Client *);
void ewmh_update_net_window_state(Client *);
void ewmh_update_net_window_visible_icon_name(Client *);
void ewmh_update_net_window_visible_name(Client *);
void wmh_process_win_window_hints(Client *);
void wmh_process_win_layer(Client *);

/* main */
void inittag(unsigned int i);
void arrange(Monitor * m);
Monitor *clientmonitor(Client * c);
Monitor *curmonitor();
void *ecalloc(size_t nmemb, size_t size);
void *emallocz(size_t size);
void *erealloc(void *ptr, size_t size);
void eprint(const char *errstr, ...);
const char *getresource(const char *resource, const char *defval);
Client *getclient(Window w, int part);
Monitor *getmonitor(int x, int y);
Bool gettextprop(Window w, Atom atom, char **text);
void iconify(Client *c);
void incnmaster(const char *arg);
Bool isfloating(Client *c, Monitor *m);
Bool isvisible(Client *c, Monitor *m);
Monitor *findmonbynum(int num);
void focus(Client * c);
void focusicon(const char *arg);
void focusnext(Client *c);
void focusprev(Client *c);
void focusview(int index);
EScreen *getscreen(Window win);
void killclient(Client *c);
void applygravity(Client *c, int *xp, int *yp, int *wp, int *hp, int bw, int gravity);
void resize(Client * c, int x, int y, int w, int h, int b);
void restack(void);
void restack_client(Client *c, int stack_mode, Client *sibling);
void configurerequest(XEvent * e);
void moveresizekb(Client *c, int dx, int dy, int dw, int dh);
void mousemove(Client *c, unsigned int button, int x_root, int y_root);
void mouseresize_from(Client *c, int from, unsigned int button, int x_root, int y_root);
void quit(const char *arg);
void restart(const char *arg);
void rotateview(Client *c);
void unrotateview(Client *c);
void rotatezone(Client *c);
void unrotatezone(Client *c);
void rotatewins(Client *c);
void unrotatewins(Client *c);
void setmwfact(const char *arg);
void setlayout(const char *arg);
void spawn(const char *arg);
void tag(Client *c, int index);
void togglestruts(const char *arg);
void toggledectiled(const char *arg);
void togglefloating(Client *c);
void togglefill(Client *c);
void togglemax(Client *c);
void togglemaxv(Client *c);
void togglemaxh(Client *c);
void toggleshade(Client *c);
void togglesticky(Client *c);
void togglemin(Client *c);
void toggleabove(Client *c);
void togglebelow(Client *c);
void togglepager(Client *c);
void toggletaskbar(Client *c);
void togglemodal(Client *c);
void togglemonitor(const char *arg);
void toggletag(Client *c, int index);
void toggleview(int index);
void toggleshowing(void);
void togglehidden(Client *c);
void updateframe(Client *c);
void view(int index);
void viewlefttag(const char *arg);
void viewprevtag(const char *arg);
void viewrighttag(const char *arg);
void zoom(Client *c);
void selectionclear(XEvent *e);
void appendtag(const char *arg);
void rmlasttag(const char *arg);
void settags(unsigned int numtags);

/* parse.c */
void initrules();
int initkeys();

/* draw.c */
void drawclient(Client * c);
void deinitstyle();
void initstyle();

/* XXX: this block of defines must die */
#define curseltags curmonitor()->seltags
#define curprevtags curmonitor()->prevtags
#define cursx curmonitor()->sc.x
#define cursy curmonitor()->sc.y
#define cursh curmonitor()->sc.h
#define cursw curmonitor()->sc.w
#define curwax curmonitor()->wa.x
#define curway curmonitor()->wa.y
#define curwaw curmonitor()->wa.w
#define curwah curmonitor()->wa.h
#define curmontag curmonitor()->curtag
#define curstruts curmonitor()->struts
#define curlayout scr->views[curmontag].layout

#define LENGTH(x)		(sizeof(x) / sizeof x[0])
#ifdef DEBUG
#define DPRINT			fprintf(stderr, "%s: %s() %d\n",__FILE__,__func__, __LINE__);
#define DPRINTF(format, ...)	fprintf(stderr, "%s %s():%d " format, __FILE__, __func__, __LINE__, __VA_ARGS__)
#else
#define DPRINT			;
#define DPRINTF(format, ...)
#endif
#define DPRINTCLIENT(c) DPRINTF("%s: x: %d y: %d w: %d h: %d th: %d f: %d b: %d m: %d\n", \
				    c->name, c->x, c->y, c->w, c->h, c->th, c->skip.arrange, c->is.bastard, c->is.max)

#define OPAQUE			0xffffffff
#define RESNAME		       "echinus"
#define RESCLASS	       "Echinus"
#define STR(_s)			TOSTR(_s)
#define TOSTR(_s)		#_s
#define min(_a, _b)		((_a) < (_b) ? (_a) : (_b))
#define max(_a, _b)		((_a) > (_b) ? (_a) : (_b))

/* globals */
extern Atom atom[NATOMS];
extern Display *dpy;
extern EScreen *scr;
extern EScreen *screens;
extern EScreen *event_scr;
// extern Window root;
// extern Window selwin;
// extern Client *clients;
// extern Monitor *monitors;
extern Client *sel;
extern Client *give;	/* gave focus last */
extern Client *take;	/* take focus last */
// extern Client *stack;
// extern Client *clist;
// extern unsigned int nmons;
// extern unsigned int ntags;
extern unsigned int nscr;
// extern unsigned int nkeys;
extern unsigned int nrules;
// extern Bool showing_desktop;
// extern int screen;
// extern Style style;
// extern Button button[LastBtn];
// extern char **tags;
// extern Atom *dt_tags;
// extern Key **keys;
extern Rule **rules;
extern Layout layouts[];
extern unsigned int modkey;
// extern View *views;
extern XContext context[];
extern Time user_time;
#ifdef STARTUP_NOTIFICATION
extern SnDisplay *sn_dpy;
extern SnMonitorContext *sn_ctx;
extern Notify *notifies;
#endif
