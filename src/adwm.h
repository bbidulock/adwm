#ifndef __LOCAL_ADWM_H__
#define __LOCAL_ADWM_H__

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
#include <dirent.h>
#include <fcntl.h>
#ifdef _GNU_SOURCE
#include <getopt.h>
#endif
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
#ifdef __linux__
#include <sys/prctl.h>
#else
#warn "Not using prctl!"
#endif

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/XF86keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
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
#ifdef SMLIB
#include <X11/ICE/ICEutil.h>
#include <X11/SM/SMlib.h>
#endif
#ifdef IMLIB2
#include <Imlib2.h>
#endif
#ifdef PIXBUF
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
#endif
#ifdef XCAIRO
#include <cairo-xlib.h>
#include <cairo-xlib-xrender.h>
#endif
#ifdef XPM
#include <X11/xpm.h>
#endif
#ifdef LIBPNG
#include <png.h>
#endif
#ifdef LIBJPEG
#include <jpeglib.h>
#endif
#ifdef LIBSVG
#include <svg-cairo.h>
#endif
#ifdef XFT
#include <X11/Xft/Xft.h>
#endif
#ifdef PANGOXFT
#include <pango/pangoxft.h>
#endif
#ifdef PANGOCAIRO
#include <pango/pangocairo.h>
#endif
#ifdef STARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>
#endif

/* enums */
enum {
	Manager, Utf8String, SMClientId, WMProto, WMDelete, WMSaveYourself, WMState,
	WMChangeState, WMTakeFocus, WMClientLeader, WMWindowRole, WMColormapWindows,
	WMColormapNotify,
	ELayout, ESelTags,
	ADwmRcFile, ADwmPrvDir, ADwmRunDir, ADwmUsrDir, ADwmXdgDir, ADwmSysDir,
	ADwmStyleName, ADwmThemeName, ADwmIconThemeName, ADwmCursorThemeName,
	ADwmStyle, ADwmTheme, ADwmIconTheme, ADwmCursorTheme, ADwmCheck,
	WMReload, WMRestart, WMShutdown, DeskLayout,
	/* MWM/DTWM properties follow */
	WMDesktop, MWMBindings, MWMDefaultBindings, MWMMessages, MWMOffset, MWMHints,
	MWMMenu, MWMInfo, DTWorkspaceHints, DTWorkspacePresence, DTWorkspaceList,
	DTWorkspaceCurrent, DTWorkspaceInfo, DTWMHints, DTWMRequest, DTWMEmbedded,
	DTWMSaveHint,
	/* _WIN_PROTOCOLS following */
	WinAppState, WinAreaCount, WinArea, WinClientList, WinClientMoving,
	WinButtonProxy, WinExpandedSize, WinFocus, WinHints, WinIcons, WinLayer,
	WinMaxGeom, WinProtocols, WinResize, WinState, WinCheck, WinWorkarea,
	WinWorkCount, WinWorkNames, WinWorkspace, WinWorkspaces, SwmVroot,
	/* _NET_SUPPORTED following */
	ClientList, ActiveWindow, WindowDesk, WindowDeskMask, NumberOfDesk, DeskNames,
	CurDesk, WorkArea, DeskViewport, ShowingDesktop, DeskGeometry, DesksVisible,
	MonitorGeometry, DeskModes, DeskModeFloating, DeskModeTiled,
	DeskModeBottomTiled, DeskModeMonocle, DeskModeTopTiled, DeskModeLeftTiled,
	DeskModeGrid,
	ClientListStacking, WindowOpacity, MoveResizeWindow, RestackWindow,
	WindowMoveResize, WindowExtents, HandledIcons, RequestFrameExt, VirtualRoots,
	WindowType, WindowTypeDesk, WindowTypeDock, WindowTypeToolbar, WindowTypeMenu,
	WindowTypeUtil, WindowTypeSplash, WindowTypeDialog, WindowTypeDrop,
	WindowTypePopup, WindowTypeTooltip, WindowTypeNotify, WindowTypeCombo,
	WindowTypeDnd, WindowTypeNormal,
	StrutPartial, Strut, WindowPid, WindowIcon, WindowName, WindowNameVisible,
	WindowIconName, WindowIconNameVisible, WindowUserTime, UserTimeWindow,
	NetStartupId, StartupInfo, StartupInfoBegin,
	WindowSync, WindowCounter, WindowFsMonitors,
	WindowState, WindowStateModal, WindowStateSticky, WindowStateMaxV,
	WindowStateMaxH, WindowStateShaded, WindowStateNoTaskbar, WindowStateNoPager,
	WindowStateHidden, WindowStateFs, WindowStateAbove, WindowStateBelow,
	WindowStateAttn, WindowStateFocused, WindowStateFixed, WindowStateFloating,
	WindowStateFilled, WindowStateMaxL, WindowStateMaxR,
	WindowActions, WindowActionMove, WindowActionResize, WindowActionMin,
	WindowActionShade, WindowActionStick, WindowActionMaxH, WindowActionMaxV,
	WindowActionFs, WindowActionChangeDesk, WindowActionClose,
	WindowActionAbove, WindowActionBelow, WindowActionFloat, WindowActionFill,
	WindowActionMaxL, WindowActionMaxR,
	WMCheck, CloseWindow, WindowPing, Supported,
	KwmWindowIcon,
	SystemTrayWindows, WindowFrameStrut, WindowForSysTray, WindowTypeOverride,
	KdeSplashProgress, WindowChangeState,
	ObPid, ObControl, ObConfigFile, ObTheme, ObVersion,
	ObAppGrpClass, ObAppGrpName, ObAppClass, ObAppName, ObAppRole, ObAppTitle, ObAppType,
	WindowStateUndec, WindowActionUndec,
	NetSnApplicationId, NetSnLauncher, NetSnLaunchee, NetSnHostname, NetSnPid,
	NetSnSequence, NetSnTimestamp, NetSnName, NetSnDescription, NetSnIconName,
	NetSnBinaryName, NetSnWmClass, NetSnScreen, NetSnWorkspace,
	NATOMS
};					/* keep in sync with atomnames[] in ewmh.c */

#define WTINDEX(_type) ((_type)-WindowTypeDesk)
#define WTFLAG(_type) (1<<WTINDEX(_type))
#define WTTEST(_wintype, _type) (((_wintype) & WTFLAG(_type)) ? True : False)
#define WTCHECK(_client, _type) WTTEST((_client)->wintype, (_type))

#define _XA_MANAGER				atom[Manager]
#define _XA_UTF8_STRING				atom[Utf8String]
#define _XA_SM_CLIENT_ID			atom[SMClientId]
#define _XA_WM_PROTOCOLS			atom[WMProto]
#define _XA_WM_DELETE_WINDOW			atom[WMDelete]
#define _XA_WM_SAVE_YOURSELF			atom[WMSaveYourself]
#define _XA_WM_STATE				atom[WMState]
#define _XA_WM_CHANGE_STATE			atom[WMChangeState]
#define _XA_WM_TAKE_FOCUS			atom[WMTakeFocus]
#define _XA_WM_CLIENT_LEADER			atom[WMClientLeader]
#define _XA_WM_WINDOW_ROLE			atom[WMWindowRole]
#define _XA_WM_COLORMAP_WINDOWS			atom[WMColormapWindows]
#define _XA_WM_COLORMAP_NOTIFY			atom[WMColormapNotify]
#define _XA_ECHINUS_LAYOUT			atom[ELayout]
#define _XA_ECHINUS_SELTAGS			atom[ESelTags]
#define _XA_ADWM_RCFILE				atom[ADwmRcFile]
#define _XA_ADWM_PRVDIR				atom[ADwmPrvDir]
#define _XA_ADWM_RUNDIR				atom[ADwmRunDir]
#define _XA_ADWM_USRDIR				atom[ADwmUsrDir]
#define _XA_ADWM_XDGDIR				atom[ADwmXdgDir]
#define _XA_ADWM_SYSDIR				atom[ADwmSysDir]
#define _XA_ADWM_STYLE_NAME			atom[ADwmStyleName]
#define _XA_ADWM_THEME_NAME			atom[ADwmThemeName]
#define _XA_ADWM_ICON_THEME_NAME		atom[ADwmIconThemeName]
#define _XA_ADWM_CURSOR_THEME_NAME		atom[ADwmCursorThemeName]
#define _XA_ADWM_STYLE				atom[ADwmStyle]
#define _XA_ADWM_THEME				atom[ADwmTheme]
#define _XA_ADWM_ICON_THEME			atom[ADwmIconTheme]
#define _XA_ADWM_CURSOR_THEME			atom[ADwmCursorTheme]
#define _XA_ADWM_CHECK				atom[ADwmCheck]
#define _XA_NET_RELOAD				atom[WMReload]
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
#define _XA_NET_DESKTOP_MODE_GRID		atom[DeskModeGrid]

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
#define _XA_NET_WM_ICON				atom[WindowIcon]
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
#define _XA_NET_WM_STATE_MAXIMUS_LEFT		atom[WindowStateMaxL]
#define _XA_NET_WM_STATE_MAXIMUS_RIGHT		atom[WindowStateMaxR]

#define _XA_NET_WM_ALLOWED_ACTIONS		atom[WindowActions]
#define _XA_NET_WM_ACTION_MOVE			atom[WindowActionMove]
#define _XA_NET_WM_ACTION_RESIZE		atom[WindowActionResize]
#define _XA_NET_WM_ACTION_MINIMIZE		atom[WindowActionMin]
#define _XA_NET_WM_ACTION_SHADE			atom[WindowActionShade]
#define _XA_NET_WM_ACTION_STICK			atom[WindowActionStick]
#define _XA_NET_WM_ACTION_MAXIMIZE_HORZ		atom[WindowActionMaxH]
#define _XA_NET_WM_ACTION_MAXIMIZE_VERT		atom[WindowActionMaxV]
#define _XA_NET_WM_ACTION_FULLSCREEN		atom[WindowActionFs]
#define _XA_NET_WM_ACTION_CHANGE_DESKTOP	atom[WindowActionChangeDesk]
#define _XA_NET_WM_ACTION_CLOSE			atom[WindowActionClose]
#define _XA_NET_WM_ACTION_ABOVE			atom[WindowActionAbove]
#define _XA_NET_WM_ACTION_BELOW			atom[WindowActionBelow]
#define _XA_NET_WM_ACTION_FLOAT			atom[WindowActionFloat]
#define _XA_NET_WM_ACTION_FILL			atom[WindowActionFill]
#define _XA_NET_WM_ACTION_MAXIMUS_LEFT		atom[WindowActionMaxL]
#define _XA_NET_WM_ACTION_MAXIMUS_RIGHT		atom[WindowActionMaxR]

#define _XA_NET_SUPPORTING_WM_CHECK		atom[WMCheck]
#define _XA_NET_CLOSE_WINDOW			atom[CloseWindow]
#define _XA_NET_WM_PING				atom[WindowPing]
#define _XA_NET_SUPPORTED			atom[Supported]

#define _XA_KWM_WIN_ICON			atom[KwmWindowIcon]
#define _XA_KDE_NET_SYSTEM_TRAY_WINDOWS		atom[SystemTrayWindows]
#define _XA_KDE_NET_WM_FRAME_STRUT		atom[WindowFrameStrut]
#define _XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR	atom[WindowForSysTray]
#define _XA_KDE_NET_WM_WINDOW_TYPE_OVERRIDE	atom[WindowTypeOverride]
#define _XA_KDE_SPLASH_PROGRESS			atom[KdeSplashProgress]
#define _XA_KDE_WM_CHANGE_STATE			atom[WindowChangeState]

#define _XA_OPENBOX_PID				atom[ObPid]
#define _XA_OB_CONTROL				atom[ObControl]

#define _XA_OB_CONFIG_FILE			atom[ObConfigFile]
#define _XA_OB_THEME				atom[ObTheme]
#define _XA_OB_VERSION				atom[ObVersion]

#define _XA_OB_APP_GROUP_CLASS			atom[ObAppGrpClass]
#define _XA_OB_APP_GROUP_NAME			atom[ObAppGrpName]
#define _XA_OB_APP_CLASS			atom[ObAppClass]
#define _XA_OB_APP_NAME				atom[ObAppName]
#define _XA_OB_APP_ROLE				atom[ObAppRole]
#define _XA_OB_APP_TITLE			atom[ObAppTitle]
#define _XA_OB_APP_TYPE				atom[ObAppType]

#define _XA_OB_WM_STATE_UNDECORATED		atom[WindowStateUndec]
#define _XA_OB_WM_ACTION_UNDECORATE		atom[WindowActionUndec]

#define _XA_NET_APP_APPLICATION_ID		atom[NetSnApplicationId]
#define _XA_NET_APP_LAUNCHER			atom[NetSnLauncher]
#define _XA_NET_APP_LAUNCHEE			atom[NetSnLaunchee]
#define _XA_NET_APP_HOSTNAME			atom[NetSnHostname]
#define _XA_NET_APP_PID				atom[NetSnPid]
#define _XA_NET_APP_SEQUENCE			atom[NetSnSequence]
#define _XA_NET_APP_TIMESTAMP			atom[NetSnTimestamp]
#define _XA_NET_APP_NAME			atom[NetSnName]
#define _XA_NET_APP_DESCRIPTION			atom[NetSnDescription]
#define _XA_NET_APP_ICON_NAME			atom[NetSnIconName]
#define _XA_NET_APP_BINARY_NAME			atom[NetSnBinaryName]
#define _XA_NET_APP_WMCLASS			atom[NetSnWmClass]
#define _XA_NET_APP_SCREEN			atom[NetSnScreen]
#define _XA_NET_APP_WORKSPACE			atom[NetSnWorkspace]

typedef struct XErrorTrap XErrorTrap;

struct XErrorTrap {
	XErrorTrap *next;
	char *trap_string;
	unsigned long trap_next;
	unsigned long trap_last;
	int trap_qlen;
	Bool trap_ignore;
};

typedef struct {
	const char *name;		/* extension name */
	Status (*version)(Display *, int *, int *);	/* how to get version */
	Bool have;			/* whether we have this extension */
	int major;			/* major version of extension */
	int minor;			/* minor version of extension */
	int opcode;			/* extension base for opcodes */
	int event;			/* extension base for event ids */
	int error;			/* extension base for error ids */
} ExtensionInfo;

enum {
	XkbBase,
	XfixesBase,
#ifdef XRANDR
	XrandrBase,
#endif
#ifdef XINERAMA
	XineramaBase,
#endif
#ifdef SYNC
	XsyncBase,
#endif
#ifdef RENDER
	XrenderBase,
#endif
#ifdef XCOMPOSITE
	XcompositeBase,
#endif
#ifdef DAMAGE
	XdamageBase,
#endif
#ifdef SHAPE
	XshapeBase,
#endif
	BaseLast
};					/* X11 extensions */

enum {
	LeftStrut,
	RightStrut,
	TopStrut,
	BotStrut,
	LastStrut
};					/* ewmh struts */

enum {
	ColFG,
	ColBG,
	ColBorder,
	ColButton,
	ColShadow,
	ColLast
};					/* colors */

enum {
	ClientWindow,
	ClientIcon,
	ClientTitle,
	ClientGrips,
	ClientFrame,
	ClientTimeWindow,
	ClientAppl,
	ClientClass,
	ClientGroup,
	ClientTransFor,
	ClientTransForGroup,
	ClientLeader,
	ClientSession,
	ClientAny,
	ClientColormap,
	SysTrayWindows,
	ScreenContext,
	PartLast
};					/* client parts */

typedef enum {
	MenuBtn,
	IconifyBtn,
	MaximizeBtn,
	CloseBtn,
	ShadeBtn,
	StickBtn,
	LHalfBtn,
	RHalfBtn,
	FillBtn,
	FloatBtn,
	SizeBtn,
	IconBtn,
	TitleTags,
	TitleName,
	TitleSep,
	LastElement,
	LastBtn = TitleTags
} ElementType;

typedef enum {
	OnClientTitle,
	OnClientGrips,
	OnClientFrame,
	OnClientDock,
	OnClientWindow,
	OnClientIcon,
	OnRoot,
	LastOn
} OnWhere;

typedef enum {
	ButtonImageDefault,
	ButtonImagePressed,
	ButtonImagePressedB1,
	ButtonImagePressedB2,
	ButtonImagePressedB3,
	ButtonImageToggledPressed,
	ButtonImageToggledPressedB1,
	ButtonImageToggledPressedB2,
	ButtonImageToggledPressedB3,
	ButtonImageHover,
	ButtonImageFocus,
	ButtonImageUnfocus,
	ButtonImageToggledHover,
	ButtonImageToggledFocus,
	ButtonImageToggledUnfocus,
	ButtonImageDisabledHover,
	ButtonImageDisabledFocus,
	ButtonImageDisabledUnfocus,
	ButtonImageToggledDisabledHover,
	ButtonImageToggledDisabledFocus,
	ButtonImageToggledDisabledUnfocus,
	LastButtonImageType
} ButtonImageType;

typedef enum {
	CauseDestroyed,
	CauseUnmapped,
	CauseReparented,
	CauseQuitting,
	CauseSwitching,
	CauseRestarting
} WithdrawCause;

typedef enum {
	ColSmartPlacement,
	RowSmartPlacement,
	MinOverlapPlacement,
	UnderMousePlacement,
	CascadePlacement,
	RandomPlacement
} WindowPlacement;

typedef enum {
	ModalModeless,
	ModalPrimary,
	ModalSystem,
	ModalGroup
} Modality;

typedef enum {
	NoInputModel,
	PassiveInputModel,
	GloballyActiveModel,
	LocallyActiveModel
} InputModel;

typedef enum {
	OrientLeft,
	OrientTop,
	OrientRight,
	OrientBottom
} LayoutOrientation;

typedef enum {
	PositionNone,
	PositionNorthWest,
	PositionNorth,
	PositionNorthEast,
	PositionWest,
	PositionCenter,
	PositionEast,
	PositionSouthWest,
	PositionSouth,
	PositionSouthEast,
	PositionStatic,
} LayoutPosition;

typedef enum {
	StrutsOn,
	StrutsOff,
	StrutsHide,
	StrutsDown
} StrutsPosition;

typedef enum {
	DockNone,
	DockEast,
	DockNorthEast,
	DockNorth,
	DockNorthWest,
	DockWest,
	DockSouthWest,
	DockSouth,
	DockSouthEast
} DockPosition;

typedef enum {
	DockSideEast,
	DockSideNorth,
	DockSideWest,
	DockSideSouth
} DockSide;

typedef enum {
	DockHorz,
	DockVert
} DockOrient;

typedef enum {
	RelativeNone,
	RelativeNorthWest,
	RelativeNorth,
	RelativeNorthEast,
	RelativeWest,
	RelativeCenter,
	RelativeEast,
	RelativeSouthWest,
	RelativeSouth,
	RelativeSouthEast,
	RelativeStatic,
	RelativeNext,
	RelativePrev,
	RelativeLast
} RelativeDirection;

typedef enum {
	SetFlagSetting,
	UnsetFlagSetting,
	ToggleFlagSetting
} FlagSetting;

typedef enum {
	IncCount,
	DecCount,
	SetCount
} ActionCount;

typedef enum {
	FocusClient,
	ActiveClient,
	PointerClient,
	AnyClient,
	AllClients,
	EveryClient
} WhichClient;

typedef enum {
	IncludeIcons,
	ExcludeIcons,
	OnlyIcons
} IconsIncluded;

enum {
	_NET_WM_ORIENTATION_HORZ,
	_NET_WM_ORIENTATION_VERT
};

enum {
	_NET_WM_TOPLEFT,
	_NET_WM_TOPRIGHT,
	_NET_WM_BOTTOMRIGHT,
	_NET_WM_BOTTOMLEFT
};

typedef enum {
	PatternSolid,
	PatternParent,
	PatternGradient,
	PatternPixmap
} Pattern;

typedef enum {
	ResizeTiled,
	ResizeScaled,
	ResizeStretched
} Resizing;

typedef enum {
	GradientDiagonal,
	GradientCrossDiagonal,
	GradientRectangle,
	GradientPyramid,
	GradientPipeCross,
	GradientElliptic,
	GradientMirrorHorizontal,
	GradientHorizontal,
	GradientSplitVertical,
	GradientVertical
} Gradient;

typedef enum {
	ReliefRaised,
	ReliefFlat,
	ReliefSunken
} Relief;

typedef enum {
	Bevel1,
	Bevel2
} Bevel;

typedef enum {
	JustifyLeft,
	JustifyCenter,
	JustifyRight
} Justify;

enum {
	CursorTopLeft,
	CursorTop,
	CursorTopRight,
	CursorRight,
	CursorBottomRight,
	CursorBottom,
	CursorBottomLeft,
	CursorLeft,
	CursorEvery,
	CursorNormal,
	CursorLast
};					/* cursor */

enum {
	Clk2Focus,
	SloppyFloat,
	AllSloppy,
	SloppyRaise
};					/* focus model */

typedef struct Tag Tag;
typedef struct View View;
typedef struct Monitor Monitor;
typedef struct Appl Appl;
typedef struct Class Class;
typedef struct Client Client;
typedef struct AScreen AScreen;
typedef struct Group Group;
typedef struct CycleList CycleList;
typedef struct Key Key;
typedef struct Node Node;
typedef struct Term Term;
typedef struct Leaf Leaf;
typedef union Container Container;
typedef struct Arrangement Arrangement;

#ifdef STARTUP_NOTIFICATION
typedef struct Notify Notify;
#endif

typedef struct {
	Pattern pattern;		/* default solid */
	Gradient gradient;		/* default none */
	Relief relief;			/* default raised */
	Resizing resize;		/* default tiled */
	Bevel bevel;			/* default bevel1 */
	Bool interlaced;		/* default False */
	Bool border;			/* default False */
} Appearance;

typedef struct {
	char *file;
	struct {
		Pixmap draw, mask;
		XImage *ximage;
#if defined IMLIB2
		Imlib_Image image;
#endif
#if defined RENDER
		Picture pict, alph;
#endif
#if defined XCAIRO
		cairo_surface_t *surf, *clip;
#endif
	}
#if defined IMLIB2 || defined PIXBUF || defined XPM || defined LIBPNG
	pixmap,
#endif
	bitmap;
	int x, y;
	unsigned w, h, b, d;
} AdwmPixmap;

typedef struct {
	Appearance appearance;		/* default 'flat solid' */
	XColor color;			/* color1 => color => default color */
	XColor colorTo;			/* color2 => colorTo => default color */
	XColor picColor;		/* picColor => color => default color */
	AdwmPixmap pixmap;		/* None */
	XftColor textColor;		/* textColor => black */
	int textOpacity;		/* 0 */
	XftColor textShadowColor;	/* textShadowColor => default color */
	int textShadowOpacity;		/* 0 */
	int textShadowXOffset;		/* 0 */
	int textShadowYOffset;		/* 0 */
	XColor borderColor;		/* borderColor => black */
	int borderWidth;		/* borderWidth => 1 */
	XColor backgroundColor;		/* backgroundColor => color => default color */
	XColor foregroundColor;		/* foregroundColor => picColor => opposite color */
	int opacity;			/* 0 */
	int borders[4];			/* left, right, top, bottom pixmap borders */
} Texture;

typedef struct {
	XftFont *font;
	XGlyphInfo extents;
	int ascent, descent, width, height;
} AdwmFont;

typedef struct {
	/* ordered for imlib DATA32 */
	unsigned blue:8;
	unsigned green:8;
	unsigned red:8;
	unsigned alpha:8;
} ARGB;

typedef struct {
	Bool shadow;
	XftColor color;
	unsigned transparency;
	int offset;
} TextShadow;

#if 0
typedef struct {
	Pixmap pixmap;
	int width, height;
} AdwmBitmap;
#endif

#define GIVE_FOCUS (1<<0)
#define TAKE_FOCUS (1<<1)

/* typedefs */
typedef struct {
	int x, y, w, h, b, g, n, s, th;
} LayoutArgs;

typedef struct {
	int x, y;
	unsigned int w, h;
} Extent;

typedef struct {
	int x, y, w, h, b;
} Geometry;

typedef struct {
	int x, y, w, h, b, t, g, v;
} ClientGeometry;

typedef struct {
	int x, y, w, h;
} Workarea;

typedef struct {
	int t, l, r, b;
} Extents;

struct Monitor {
	Workarea sc, wa;
	unsigned long struts[LastStrut];
	Monitor *next;
	int mx, my;
	View *curview;	    /* current view */
	View *preview;	    /* previous view */
	int num, index;
	Window veil;
	struct {
		Workarea wa;
		DockPosition position;
		DockOrient orient;
	} dock;
	int row, col;			/* row and column in monitor layout */
	PointerBarrier bars[8];
};

typedef struct {
	// void (*arrange) (View *v);
	Arrangement *arrange;
	char symbol;
	int features;
	LayoutOrientation major;	/* overall orientation */
	LayoutOrientation minor;	/* master area orientation */
} Layout;

typedef struct {
	AdwmPixmap px;
	XftColor bg;
	Bool present;
} ButtonImage;

typedef struct {
	ButtonImage *image;
	Bool (**action) (Client *, XEvent *);
} Element;

typedef struct {
	Bool present, hovered;
	unsigned pressed;
	Geometry eg;
} ElementClient;

typedef union {
	struct {
		unsigned taskbar:1;
		unsigned pager:1;
		unsigned winlist:1;
		unsigned cycle:1;
		unsigned focus:1;
		unsigned arrange:1;
		unsigned sloppy:1;
	};
	unsigned skip;
} SkipUnion;

extern char *skip_fields[32];		/* see parse.h */

typedef union {
	struct {
		unsigned border:1;
		unsigned grips:1;
		unsigned title:1;
		struct {
			unsigned menu:1;
			unsigned min:1;
			unsigned max:1;
			unsigned close:1;
			unsigned size:1;
			unsigned shade:1;
			unsigned stick:1;
			unsigned fill:1;
			unsigned floats:1;
			unsigned half:1;
		} but __attribute__ ((packed));
	};
	unsigned has;
} HasUnion;

extern char *has_fields[32];		/* see parse.h */

typedef union {
	struct {
		unsigned transient:1;
		unsigned grptrans:1;
		unsigned banned:1;
		unsigned max:1;
		unsigned floater:1;
		unsigned maxv:1;
		unsigned maxh:1;
		unsigned lhalf:1;
		unsigned rhalf:1;
		unsigned shaded:1;
		unsigned icon:1;
		unsigned fill:1;
		unsigned modal:2;
		unsigned above:1;
		unsigned below:1;
		unsigned attn:1;
		unsigned sticky:1;
		unsigned undec:1;
		unsigned closing:1;
		unsigned killing:1;
		unsigned pinging:1;
		unsigned hidden:1;
		unsigned bastard:1;
		unsigned full:1;
		unsigned focused:1;
		unsigned selected:1;
		unsigned dockapp:1;
		unsigned moveresize:1;
		unsigned managed:1;
		unsigned saving:1;
	};
	unsigned is;
} IsUnion;

extern char *is_fields[32];		/* see parse.h */

typedef union {
	struct {
		unsigned move:1;
		unsigned size:1;
		unsigned sizev:1;
		unsigned sizeh:1;
		unsigned min:1;
		unsigned max:1;
		unsigned maxv:1;
		unsigned maxh:1;
		unsigned close:1;
		unsigned shade:1;
		unsigned stick:1;
		unsigned full:1;
		unsigned above:1;
		unsigned below:1;
		unsigned fill:1;
		unsigned fillh:1;
		unsigned fillv:1;
		unsigned floats:1;
		unsigned hide:1;
		unsigned tag:1;
		unsigned arrange:1;
		unsigned undec:1;
		unsigned select:1;
		unsigned focus:2;
	};
	unsigned can;
} CanUnion;

extern char *can_fields[32];		/* see parse.h */

typedef union {
	struct {
		unsigned struts:1;
		unsigned time:1;
		unsigned boundary:1;
		unsigned clipping:1;
		unsigned wshape:1;
		unsigned bshape:1;
	};
	unsigned with;
} WithUnion;

extern char *with_fields[32];		/* see parse.h */

struct Appl {
	Appl *next;			/* next in list */
	char *appid;			/* application id (unique) */
	char *name;			/* application name (or NULL) */
	char *description;		/* description (or NULL) */
	char *wmclass;			/* wmclass (or NULL) */
	char *binary;			/* binary name (or NULL) */
	char *icon;			/* icon name (or NULL) */
	ButtonImage button;		/* titlebar icon for this appid */
	Window *members;		/* windows with this appid */
	unsigned count;			/* count of windows */
};

struct Class {
	Class *next;			/* next in list */
	XClassHint ch;			/* res_class and res_name */
	Window *members;		/* windows with this res_class and name */
	ButtonImage button;		/* titlebar icon for this class */
	unsigned count;			/* count of windows */
};

struct Client {
	XWMHints wmh;			/* initial state */
	XClassHint ch;
	XSizeHints sh;
	char *name;
	char *icon_name;
	char *wm_name;			/* name for session management */
	char *wm_role;			/* role for session management */
	int monitor;			/* initial monitor */
	Extents e;			/* copy of _NET_FRAME_EXTENTS */
	Geometry s, u;			/* static and initial */
	ClientGeometry c, r;		/* current, restore */
//	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
//	int minax, maxax, minay, maxay, gravity;
//	int ignoreunmap;
//	long flags;
	int wintype;			/* mask of _NET_WM_WINDOW_TYPE */
	int winstate;			/* copy of WM_STATE */
	int index;			/* holds index during session management */
	int breadcrumb;			/* holds state during layout */
	Bool wasfloating;		/* holds state during layout */
	unsigned long long tags;	/* on which views this client appears */
	int nonmodal;			/* holds state for modality */
	SkipUnion skip;
	IsUnion is;
	HasUnion has;
	HasUnion needs;
	WithUnion with;
	CanUnion can;
	View *cview;
	Leaf *leaves;
	Client *next;	/* tiling list order */
	Client *prev;	/* tiling list order (rev) */
	Client *snext;	/* stacking list order _NET_CLIENT_LIST_STACKING */
	Client *cnext;	/* client list order _NET_CLIENT_LIST */
	Client *fnext;	/* focus list order */
	Client *anext;	/* select list order */
	Window win;
	Window icon;
	Window title;
	Window grips;
	Window lgrip;
	Window rgrip;
	Window tgrip;
	Window frame;
#ifdef RENDER
	struct {
		Picture win;
		Picture icon;
		Picture title;
		Picture grips;
		Picture lgrip;
		Picture rgrip;
		Picture tgrip;
		Picture frame;
	} pict;
#endif
#ifdef XCAIRO
	struct {
		cairo_t *win;
		cairo_t *icon;
		cairo_t *title;
		cairo_t *grips;
		cairo_t *lgrip;
		cairo_t *rgrip;
		cairo_t *tgrip;
		cairo_t *frame;
	} cctx;
#endif
	Window time_window;
	Window leader;
	Window transfor;
	Window session;
	Window *cmapwins;
	Colormap cmap;
	ButtonImage button;
	ElementClient *element;
	Time user_time;
	Time save_time;
#ifdef SYNC
	struct {
		XID counter;
		Bool waiting;
		XSyncValue val;
		XSyncAlarm alarm;
		int w;
		int h;
	} sync;
#endif
#ifdef STARTUP_NOTIFICATION
	SnStartupSequence *seq;
#endif
	unsigned int opacity;
};

struct CycleList {
	Client *c;
	CycleList *next;
	CycleList *prev;
};

struct Group {
	Window *members;
	unsigned count;
	int modal_transients;
};

struct View {
	StrutsPosition barpos;		/* show struts? */
	Bool dectiled;			/* decorate tiled? */
	int nmaster;
	int ncolumns;
	double mwfact;			/* master width factor */
	double mhfact;			/* master height factor */
	LayoutOrientation major;	/* overall orientation */
	LayoutOrientation minor;	/* master area orientation */
	WindowPlacement placement;	/* float placement policy */
	Monitor *curmon;		/* monitor currently displaying this view */
	Layout *layout;
	Node *tree;			/* layout tree */
	int index;
	unsigned long long seltags;	/* tags selected for this view */
	int row, col;			/* row and column in desktop layout */
	Bool needarrange;		/* need to be rearranged */
	Client *lastsel;		/* last selected client for view */
	Time strut_time;		/* time that we entered a strut */
};					/* per-tag settings */

typedef enum {
	PlaceImageCenter,		/* center on screen at original size */
	PlaceImageScaled,		/* scaled to w or h of screen orig aspect */
	PlaceImageTiled,		/* tiled over the screen */
	PlaceImageFull,			/* scaled with aspect adjused to screen */
	PlaceImageClipped,		/* same as full but clip where necessary to
					   avoid changing aspect too drastically */
} PlaceImage;

struct Tag {
	Atom dt;			/* desktop atom for this tag */
	char name[64];			/* desktop name for this tag */
	unsigned long color;		/* color of desktop background */
	char *image;			/* image name for this desktop background */
	Pixmap pixmap;			/* pixmap of loaded image */
	Geometry w;			/* size and position of the pixmap */
	PlaceImage how;			/* how the pixmap is placed */
};

typedef enum {
	TreeTypeNode,			/* interior node (contains nodes or terms) */
	TreeTypeTerm,			/* terminal node (contains only leaves) */
	TreeTypeLeaf			/* leaf node */
} TreeType;

#define CONTAINER_COMMON_PORTION \
	TreeType type; \
	int view; \
	ClientGeometry t, f, c; \
	SkipUnion skip; \
	IsUnion is; \
	HasUnion has; \
	HasUnion needs; \
	WithUnion with; \
	CanUnion can

struct Node {
	CONTAINER_COMMON_PORTION;

	Node *parent;
	Node *next;			/* next sibling node */
	Node *prev;			/* prev sibling node */

	struct {
		int number;
		int active;
		Container *head;
		Container *tail;
		LayoutOrientation ori;
		LayoutPosition pos;
		Container *selected;		/* last selected child */
		Container *focused;		/* last focused child */
	} children;
};

struct Term {
	CONTAINER_COMMON_PORTION;

	Node *parent;
	Term *next;			/* next sibling node */
	Term *prev;			/* prev sibling node */

	struct {
		int number;
		int active;
		Leaf *head;
		Leaf *tail;
		LayoutOrientation ori;
		LayoutPosition pos;
		Leaf *selected;		/* last selected child */
		Leaf *focused;		/* last focused child */
	} children;
};

struct Leaf {
	CONTAINER_COMMON_PORTION;

	Term *parent;
	Leaf *next;			/* next sibling leaf */
	Leaf *prev;			/* prev sibling leaf */

	struct {
		Leaf *next;			/* next leaf for same client */
		Client *client;
		char *clientid;
		char *res_name;
		char *res_class;
		char *wm_name;
		char *wm_role;
		char *wm_command;
	} client;
};

union Container {
	struct {
		CONTAINER_COMMON_PORTION;

		Container *parent;
		Container *next;	/* next sibling */
		Container *prev;	/* prev sibling */
	};
	Node node;
	Term term;
	Leaf leaf;
};

struct _FontInfo {
	XGlyphInfo *extents;
	int ascent;
	int descent;
	int height;
	int width;
};

struct _DrawInfo {
#ifdef XCAIRO
	cairo_t *cctx;
#endif
#ifdef RENDER
	Picture pict;
#endif
	Pixmap pixmap;
	XftDraw *xft;
	int w;
	int h;
};

typedef struct {
	int x, y, w, h;
	struct _FontInfo font[3];
	GC gc;
	struct _DrawInfo draw;
} DC;					/* draw context */

typedef struct {
	int border;
	int margin;
	int outline;
	int spacing;
	int titleheight;
	int gripsheight;
	int gripswidth;
	Bool fullgrips;
	int opacity;
	char titlelayout[32];
#if defined USE_PANGO
	PangoFontDescription *font[3];
#elif defined USE_XFT
	XftFont *font[3];
#else
#error No font description format defined.
#endif
	int drop[3];
	union {
		struct {
			XftColor norm[ColLast];
			XftColor focu[ColLast];
			XftColor sele[ColLast];
		};
		XftColor hue[3][ColLast];
	} color;
} Style;

struct Key {
	unsigned long mod;
	KeySym keysym;
	void (*func) (XEvent *, Key *);
	void (*stop) (XEvent *, Key *);
	Key *chain;
	Key *cnext;
	char *arg;
	RelativeDirection dir;
	Bool wrap;
	ActionCount act;
	FlagSetting set;
	WhichClient any;
	WindowPlacement plc;
	IconsIncluded ico;
	int tag;
	CycleList *cycle, *where;
	int num;
	Bool cyc;
};					/* keyboard shortcuts */

typedef struct {
	char *home;
	char *runt;
	char *cach;
	struct {
		char *home;
		char *dirs;
	} conf, data;
} XdgDirs;

typedef struct {
	int debug;
	int output;
	Bool useveil;
	Bool attachaside;
	Bool dectiled;
	Bool decmax;
	Bool hidebastards;
	Bool strutsactive;
	Bool autoroll;
	Bool showdesk;
	Time strutsdelay;
	int focus;
	int snap;
	const char *command;
	const char *command2;
	const char *command3;
	const char *menucommand;
	DockPosition dockpos;
	DockOrient dockori;
	int dockmon;
	int dragdist;
	double mwfact;
	double mhfact;
	int nmaster;
	int ncolumns;
	char deflayout;
	WindowPlacement placement;
	int ntags;
	/* the following should probably be in theme file */
	const char *prependdirs; /* directories to prepend to icon search paths */
	const char *appenddirs;	 /* directories to append to icon search paths */
	const char *icontheme;	 /* the icon theme to use */
	const char *extensions;	 /* the list of icon filename extensions in order of preference */
} Options;

struct AScreen {
	Bool managed;
	Window root;
	Window selwin;
	Client *clients;
	Monitor *monitors;
	int last;
	int nmons;
	Client *stack;
	Client *clist;			/* client list in creation order */
	Client *flist;			/* client list in last focus order */
	Client *alist;			/* client list in last active order */
	int screen;
	int ntags;
	struct {
		Container *tree;	/* only one dock per screen for now... */
		Monitor *monitor;	/* monitor on which dock appears */
	} dock;
	View views[MAXTAGS];
	Tag tags[MAXTAGS];
	Key *keylist;
	struct {
		int orient;		/* orientation */
		int rows, cols;		/* rows and cols (one can be zero) */
		int start;		/* starting corner */
	} layout;
	struct {
		int rows, cols;		/* rows and cols in desk/monitor layout */
	} d, m;
	int sh, sw;
	Bool colormapnotified;
	DC dc;				/* draw context for frames, titles, grips */
	GC gc;				/* graphics context for dock apps */
	Element element[LastElement];
	Style style;
#ifdef IMLIB2
	Imlib_Context context;		/* context for 32-bit ARGB visuals */
#if 0
	Imlib_Context rootctx;		/* context for default root visuals */
#endif
#endif
	Window drawable;		/* proper drawable for GC creation */
	Visual *visual;
	Colormap colormap;
	unsigned int depth;
#ifdef RENDER
	XRenderPictFormat *format;
#endif
	Bool dither;
	unsigned int bpp;
	unsigned char *rctab;		/* red color table */
	unsigned char *gctab;		/* green color table */
	unsigned char *bctab;		/* blue color table */
	XColor *colors;			/* colormap */
	int ncolors;			/* number of colors in colormap */
	int cpc;
#ifdef STARTUP_NOTIFICATION
	SnMonitorContext *ctx;
#endif
	Options options;		/* screen-specific options */
};

typedef struct {
	char *prop;
	char *tags;
	struct {
		IsUnion is;
		IsUnion set;
	} is;
	struct {
		SkipUnion skip;
		SkipUnion set;
	} skip;
	struct {
		HasUnion has;
		HasUnion set;
	} has;
	struct {
		CanUnion can;
		CanUnion set;
	} can;
#if 0
	Bool isfloating;		/* deprecated */
	Bool hastitle;			/* deprecated */
#endif
	regex_t *propregex;
	regex_t *tagregex;
} Rule;					/* window matching rules */

#ifdef STARTUP_NOTIFICATION
struct Notify {
	Notify *next;
	SnStartupSequence *seq;
	char *id;
	char *launcher;
	char *launchee;
	char *hostname;
	pid_t pid;
	long sequence;
	Time timestamp;
	Bool complete;
	Bool assigned;
};
#endif

typedef struct {
	void *handle;
	char *name;
	char *clas;
	void (*initrcfile) (const char *, Bool);  /* locate and read primary rc databases */
	void (*initconfig) (Bool);  /* perform global configuration */
	void (*initscreen) (Bool);  /* perform per-screen configuration */
	void (*inittags) (Bool);    /* initialize per-screen tags */
	void (*initkeys) (Bool);    /* initialize per-screen key bindings */
	void (*initdock) (Bool);    /* initialize dock layouts */
	void (*initlayouts) (Bool); /* initialize views and layouts */
	void (*initstyle) (Bool);   /* initialize per-screen style */
	void (*inittheme) (Bool);   /* initialize per-screen theme */
	void (*deinitstyle) (void);
	void (*drawclient) (Client *);
} AdwmOperations;

/* main */
View *onview(Client *c);
View *clientview(Client *c);
View *selview();
View *nearview();
void *ecalloc(size_t nmemb, size_t size);
void *emallocz(size_t size);
void *erealloc(void *ptr, size_t size);
void eprint(const char *errstr, ...);
Client *findclient(Window w);
Client *findmanaged(Window w);
const char *getresource(const char *resource, const char *defval);
const char *getscreenres(const char *resource, const char *defval);
const char *getsessionres(const char *resource, const char *defval);
void putresource(const char *resource, const char *value);
void putscreenres(const char *resource, const char *value);
void putintscreenres(const char *resource, int value);
void putcharscreenres(const char *resource, char value);
void putfloatscreenres(const char *resource, double value);
void putviewres(unsigned v, const char *resource, const char *value);
void putintviewres(unsigned v, const char *resource, int value);
void putcharviewres(unsigned v, const char *resource, char value);
void putfloatviewres(unsigned v, const char *resource, double value);
void puttagsres(unsigned t, const char *resource, const char *value);
void putsessionres(const char *resource, const char *value);
Client *getclient(Window w, int part);
Client *getmanaged(Window w, int part);
View *getview(int x, int y);
Bool gettextprop(Window w, Atom atom, char **text);
void getpointer(int *x, int *y);
void hide(Client *c);
void hideall(View *v);
void iconify(Client *c);
void iconifyall(View *v);

void setborder(int px);
void incborder(int px);
void decborder(int px);
void setmargin(int px);
void incmargin(int px);
void decmargin(int px);
Client *findfocus(Client *not);
Client *findselect(Client *not);
void focus(Client *c);
void focusicon(void);
void focusnext(Client *c);
void focusprev(Client *c);
void focusmain(Client *c);
void focusurgent(Client *c);
AScreen *getscreen(Window win, Bool query);
AScreen *geteventscr(XEvent *ev);
void killclient(Client *c);
void killproc(Client *c);
void killxclient(Client *c);
Bool configurerequest(XEvent *e);
void pushtime(Time time);
void quit(const char *arg);
void restart(const char *arg);
void reload(void);
void spawn(const char *arg);
void togglestruts(View *v);
void togglemin(Client *c);
void togglepager(Client *c);
void toggletaskbar(Client *c);
void togglemodal(Client *c);
void togglemonitor(void);
void toggleshowing(void);
void togglehidden(Client *c);
Bool selectionclear(XEvent *e);

/* needed by tags.c */
Bool with_transients(Client *c, Bool (*each) (Client *, int), int data);

/* needed by layout.c */
void focuslockclient(Client *c);
Bool checkfocuslock(Client *c, unsigned long serial);
Bool canselect(Client *c);
Bool selectok(Client *c);
Group *getleader(Window leader, int group);
Class *getclass(Client *c);
void updategeom(Monitor *m);
extern Cursor cursor[CursorLast];
extern ExtensionInfo einfo[BaseLast];
Bool handle_event(XEvent *ev);
View *closestview(int x, int y);
Bool newsize(Client *c, int w, int h, Time time);
void ban(Client *c);
void unban(Client *c, View *v);
extern Group window_stack;
void unmanage(Client *c, WithdrawCause cause);

/* needed by ewmh.c */
#ifdef STARTUP_NOTIFICATION
void updateappl(Client *c);
#endif

/* needed by draw.c */
void installcolormaps(AScreen *s, Client *c, Window *w);
ButtonImage *getappbutton(Client *c);
ButtonImage *getresbutton(Client *c);
ButtonImage *getwinbutton(Client *c);
ButtonImage **getbuttons(Client *c);
ButtonImage *getbutton(Client *c);

void show_client_state(Client *c);

#define LENGTH(x)		(sizeof(x)/sizeof(*x))

#ifndef NAME
#define NAME "adwm"
#endif

#define _WCFMTS(_wc,_mask)	"%d%sx%d%s+%d%s+%d%s:%d%s (%d%s-0x%lx%s) "
#define _WCARGS(_wc,_mask)	(_wc).x, ((_mask) & CWX) ? "!" : "", \
				(_wc).y, ((_mask) & CWY) ? "!" : "", \
				(_wc).width, ((_mask) & CWWidth) ? "!" : "", \
				(_wc).height, ((_mask) & CWHeight) ? "!" : "", \
				(_wc).border_width, ((_mask) & CWBorderWidth) ? "!" : "", \
				(_wc).stack_mode, ((_mask) & CWStackMode) ? "!" : "", \
				(_wc).sibling, ((_mask) & CWSibling) ? "!" : ""

#define __GFMTS(_g)	"%dx%d+%d+%d:%d (%d:%d:%d) "
#define __GARGS(_g)	(_g)->w,(_g)->h,(_g)->x,(_g)->y,(_g)->b,(_g)->t,(_g)->g,(_g)->v
#define __CFMTS(_c)	"{%-8s,%-8s} [0x%08lx 0x%08lx 0x%08lx %-20s] "
#define __CARGS(_c)	(_c)->ch.res_class, (_c)->ch.res_name, (_c)->frame,(_c)->win,(_c)->icon,(_c)->name

#define __PTRACE(_num)		  do { if (options.debug >= _num) { \
		fprintf(stderr, NAME ": T: [%s] %12s: +%4d : %s()\n", _timestamp(), __FILE__, __LINE__, __func__); \
		fflush(stderr); } } while (0)

#define __DPRINTF(_num, _args...)  do { if (options.debug >= _num) { \
		fprintf(stderr, NAME ": D: [%s] %12s: +%4d : %s() : ", _timestamp(), __FILE__, __LINE__, __func__); \
		fprintf(stderr, _args); fflush(stderr); } } while (0)

#define __CPRINTF(_num,_c,_args...) do { if (options.debug >= _num) { \
		fprintf(stderr, NAME ": D: [%s] %12s: +%4d : %s() : " __CFMTS(_c), _timestamp(), __FILE__, __LINE__, __func__,__CARGS(_c)); \
		fprintf(stderr, _args); fflush(stderr); } } while (0)

#define __GPRINTF(_num,_g,_args...) do { if (options.debug >= _num) { \
		fprintf(stderr, NAME ": D: [%s] %12s: +%4d : %s() : " __GFMTS(_g), _timestamp(), __FILE__, __LINE__, __func__,__GARGS(_g)); \
		fprintf(stderr, _args); fflush(stderr); } } while (0)

#define __EPRINTF(_args...)	  do { \
		fprintf(stderr, NAME ": E: [%s] %12s: +%4d : %s() : ", _timestamp(), __FILE__, __LINE__, __func__); \
		fprintf(stderr, _args); fflush(stderr); } while (0)

#define __BKTRACE(_args...)	 do { \
		fprintf(stderr, NAME ": B: [%s] %12s: +%4d : %s() : ", _timestamp(), __FILE__, __LINE__, __func__); \
		fprintf(stderr, _args); dumpstack(__FILE__, __LINE__, __func__); fflush(stderr); } while (0)

#define __OPRINTF(_num, _args...)  do { if (options.debug >= _num) { \
		fprintf(stderr, NAME ": I: "); \
		fprintf(stderr, _args); fflush(stderr); } } while (0)

#define __XPRINTF(_num, _args...)  do { } while(0)

#define _DPRINT			__PTRACE(0)
#define _DPRINTF(args...)	__DPRINTF(0,args)
#define _CPRINTF(c,args...)	__CPRINTF(0,c,args)
#define _GPRINTF(g,args...)	__GPRINTF(0,g,args)
#define _EPRINTF(args...)	__EPRINTF(args)
#define _BKTRACE(args...)	__BKTRACE(args)
#define _OPRINTF(args...)	__OPRINTF(0,args)
#define _XPRINTF(args...)	__XPRINTF(0,args)

#ifdef DEBUG
#define DPRINT			_DPRINT
#define DPRINTF(args...)	_DPRINTF(args)
#define CPRINTF(c,args...)	_CPRINTF(c,args)
#define GPRINTF(g,args...)	_GPRINTF(g,args)
#else
#define DPRINT			do { } while(0)
#define DPRINTF(args...)	do { } while(0)
#define CPRINTF(c,args...)	do { } while(0)
#define GPRINTF(g,args...)	do { } while(0)
#endif

#define EPRINTF(args...)	_EPRINTF(args)
#define BKTRACE(args...)	_BKTRACE(args)
#define OPRINTF(args...)	_OPRINTF(args)
#define XPRINTF(args...)	_XPRINTF(args)

#define NONREENTRANT_ENTER \
static int _was_here = 0; \
do { \
	if (_was_here++) \
		_DPRINTF("entered %d times!\n", _was_here); \
} while(0)
#define NONREENTRANT_LEAVE \
do { \
	_was_here--; \
} while(0)

void _xtrap_push(Bool ignore, const char *time, const char *file, int line, const char *func, const char *fmt, ...)
     __attribute__((__format__(__printf__, 6, 7)));
void _xtrap_pop(int canary);

#define xtrap_push(ig,args...) int _xtrap_canary = 0; do { _xtrap_push(ig, _timestamp(), __FILE__, __LINE__, __func__, args); } while (0)
#define xtrap_pop()            do { _xtrap_pop (_xtrap_canary); } while (0)


#define OPAQUE			0xffffffff
#define RESNAME		       "adwm"
#define RESCLASS	       "Adwm"
#define STR(_s)			TOSTR(_s)
#define TOSTR(_s)		#_s
#define min(_a, _b)		((_a) < (_b) ? (_a) : (_b))
#define max(_a, _b)		((_a) > (_b) ? (_a) : (_b))

/* macros */
#define WINDOWMASK		(EnterWindowMask | LeaveWindowMask)
#define BUTTONMASK		(ButtonPressMask | ButtonReleaseMask)
#define CLEANMASK(mask)		(mask & ~(numlockmask | LockMask))
#define MOUSEMASK		(BUTTONMASK | PointerMotionMask)
#define CLIENTMASK	        (PropertyChangeMask | StructureNotifyMask | ColormapChangeMask | PointerMotionMask)
#define CLIENTNOPROPAGATEMASK 	(BUTTONMASK | ButtonMotionMask)
#define FRAMEMASK               (MOUSEMASK | WINDOWMASK | SubstructureRedirectMask | SubstructureNotifyMask | FocusChangeMask)
#define MAPPINGMASK		(StructureNotifyMask | SubstructureRedirectMask | SubstructureNotifyMask | EnterWindowMask)
#define ROOTMASK		(BUTTONMASK | WINDOWMASK | MAPPINGMASK | FocusChangeMask | ColormapChangeMask)

/* globals */
extern XdgDirs xdgdirs;
extern Options options;
extern Atom atom[NATOMS];
extern Display *dpy;
extern AScreen *scr;
extern AScreen *screens;
extern AScreen *event_scr;
extern Bool (*actions[LastOn][Button5-Button1+1][2]) (Client *, XEvent *);
extern Class *classes;
extern Client *sel;
extern Client *gave;			/* gave focus last */
extern Client *took;			/* took focus last */
extern int nscr;
extern int nrules;
extern Rule **rules;
extern unsigned modkey;
extern unsigned numlockmask;
extern unsigned scrlockmask;
extern XContext context[];
extern Time user_time;

extern char *clientId;

/* for debugging and error handling */
const char *_timestamp(void);
void dumpstack(const char *file, const int line, const char *func);
void ignorenext(void);

extern XrmDatabase xresdb, xrdb, srdb;
extern int cargc;
extern char **cargv;
#ifdef STARTUP_NOTIFICATION
extern SnDisplay *sn_dpy;
#endif
extern XErrorTrap *traps;

#endif				/* __LOCAL_ADWM_H__ */
