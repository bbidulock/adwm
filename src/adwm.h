#ifndef __LOCAL_ADWM_H__
#define __LOCAL_ADWM_H__

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xfixes.h>
#ifdef SYNC
#include <X11/extensions/sync.h>
#endif
#ifdef IMLIB2
#include <Imlib2.h>
#endif
#ifdef XPM
#include <X11/xpm.h>
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
	StrutPartial, Strut, WindowPid, WindowName, WindowNameVisible, WindowIconName,
	WindowIconNameVisible, WindowUserTime, UserTimeWindow, NetStartupId,
	StartupInfo, StartupInfoBegin,
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
	ColLast
};					/* colors */

enum {
	ClientWindow,
	ClientIcon,
	ClientTitle,
	ClientGrips,
	ClientFrame,
	ClientTimeWindow,
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
	const char *file;
	Pixmap pixmap, mask;
	int x, y;
	unsigned depth, width, height;
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

typedef struct {
	Pixmap pixmap;
	int width, height;
} AdwmBitmap;

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
	WindowPlacement placement;	/* float placement policy */
} Layout;

typedef struct {
	Bool present, hovered;
	unsigned pressed;
	struct {
		int x, y, w, h, b;
	} g;
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
	};
	unsigned is;
} IsUnion;

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

struct Client {
	char *name;
	char *icon_name;
	int monitor;			/* initial monitor */
	ClientGeometry c, r, s, u;	/* current, restore, static, supplied */
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int minax, maxax, minay, maxay, gravity;
	int ignoreunmap;
	long flags;
	int wintype;
	int winstate;
	int breadcrumb;
	Bool wasfloating;
	unsigned long long tags;
	int nonmodal;
	SkipUnion skip;
	IsUnion is;
	HasUnion has;
	HasUnion needs;
	WithUnion with;
	CanUnion can;
	View *cview;
	Leaf *leaves;
	Client *next;
	Client *prev;
	Client *snext;
	Client *cnext;
	Client *fnext;
	Client *anext;
	Window win;
	Window icon;
	Window title;
	Window grips;
	Window frame;
	Window time_window;
	Window leader;
	Window transfor;
	Window session;
	Window *cmapwins;
	Colormap cmap;
	ElementClient *element;
	Time user_time;
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
		char *name;
		char *clas;
		char *command;
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

typedef struct {
	int x, y, w, h;
	struct {
		XGlyphInfo *extents;
		int ascent;
		int descent;
		int height;
		int width;
	} font[3];
	GC gc;
	struct {
		Pixmap pixmap;
		XftDraw *xft;
		int w;
		int h;
	} draw;
} DC;					/* draw context */

typedef struct {
#if defined IMLIB2 || defined XPM
	Pixmap pixmap, mask;
#endif
	Pixmap bitmap;
	Bool present;
	int x, y;
	unsigned w, h, b;

} ButtonImage;

typedef struct {
	ButtonImage *image;
	Bool (**action) (Client *, XEvent *);
} Element;

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
	XftFont *font[3];
	int drop[3];
	struct {
		unsigned long norm[ColLast];
		unsigned long focu[ColLast];
		unsigned long sele[ColLast];
		XftColor *font[3];
		XftColor *shadow[3];
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
	Bool useveil;
	Bool attachaside;
	Bool dectiled;
	Bool decmax;
	Bool hidebastards;
	Bool strutsactive;
	Bool autoroll;
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
	const char *deflayout;
	int ntags;
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
	Bool showing_desktop;
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
	Imlib_Context context;
#endif
	Window drawable;		/* proper drawable for GC creation */
	Visual *visual;
	Colormap colormap;
	unsigned int depth;
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
	Bool isfloating;
	Bool hastitle;
	regex_t *propregex;
	regex_t *tagregex;
} Rule;					/* window matching rules */

#ifdef STARTUP_NOTIFICATION
struct Notify {
	Notify *next;
	SnStartupSequence *seq;
	char **names;
	char **values;
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
const char *getresource(const char *resource, const char *defval);
const char *getscreenres(const char *resource, const char *defval);
const char *getsessionres(const char *resource, const char *defval);
Client *getclient(Window w, int part);
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
void discardcrossing(Client *c);
Bool canselect(Client *c);
Bool selectok(Client *c);
Group *getleader(Window leader, int group);
void updategeom(Monitor *m);
extern Cursor cursor[CursorLast];
extern ExtensionInfo einfo[BaseLast];
Bool handle_event(XEvent *ev);
View *closestview(int x, int y);
Bool newsize(Client *c, int w, int h, Time time);
void send_configurenotify(Client *c, Window above);
void ban(Client *c);
void unban(Client *c, View *v);
extern Group window_stack;
void unmanage(Client *c, WithdrawCause cause);

/* needed by draw.c */
void installcolormaps(AScreen *s, Client *c, Window *w);

void show_client_state(Client *c);

#define LENGTH(x)		(sizeof(x)/sizeof(*x))

#ifndef NAME
#define NAME "adwm"
#endif

#define __PTRACE(_num)		  do { if (options.debug >= _num) { \
		fprintf(stderr, NAME ": T: [%s] %12s: +%4d : %s()\n", _timestamp(), __FILE__, __LINE__, __func__); \
		fflush(stderr); } } while (0)

#define __DPRINTF(_num, _args...)  do { if (options.debug >= _num) { \
		fprintf(stderr, NAME ": D: [%s] %12s: +%4d : %s() : ", _timestamp(), __FILE__, __LINE__, __func__); \
		fprintf(stderr, _args); fflush(stderr); } } while (0)

#define __CPRINTF(_num,c,_args...) do { if (options.debug >= _num) { \
		fprintf(stderr, NAME ": D: [%s] %12s: +%4d : %s() : [0x%08lx 0x%08lx 0x%08lx %-20s] ", _timestamp(), __FILE__, __LINE__, __func__,(c)->frame,(c)->win,(c)->icon,(c)->name); \
		fprintf(stderr, _args); fflush(stderr); } } while (0)

#define __GPRINTF(_num,c,_args...) do { if (options.debug >= _num) { \
		fprintf(stderr, NAME ": D: [%s] %12s: +%4d : %s() : %dx%d+%d+%d:%d (%d:%d:%d) ", _timestamp(), __FILE__, __LINE__, __func__,(_g)->w,(_g)->h,(_g)->x,(_g)->y,(_g)->b,(_g)->t,(_g)->g,(_g)->v); \
		fprintf(stderr, _args); fflush(stderr); } } while (0)

#define __EPRINTF(_args...)	  do { \
		fprintf(stderr, NAME ": E: [%s] %12s: +%4d : %s() : ", _timestamp(), __FILE__, __LINE__, __func__); \
		fprintf(stderr, _args); fflush(stderr); } while (0)

#define __OPRINTF(_num, _args...)  do { if (options.debug >= _num) { \
		fprintf(stdout, NAME ": I: "); \
		fprintf(stdout, _args); fflush(stdout); } } while (0)

#define __XPRINTF(_num, _args...)  do { } while(0)

#define _DPRINT			__PTRACE(0)
#define _DPRINTF(args...)	__DPRINTF(0,args)
#define _CPRINTF(c,args...)	__CPRINTF(0,c,args)
#define _GPRINTF(g,args...)	__GPRINTF(0,g,args)
#define _EPRINTF(args...)	__EPRINTF(args)
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
#define CLIENTMASK	        (PropertyChangeMask | StructureNotifyMask| ColormapChangeMask)
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
extern unsigned long ignore_request;

/* for debugging and error handling */
const char *_timestamp(void);
void dumpstack(const char *file, const int line, const char *func);
void ignorenext(void);

extern XrmDatabase xresdb, xrdb;
extern int cargc;
extern char **cargv;
#ifdef STARTUP_NOTIFICATION
extern SnDisplay *sn_dpy;
#endif

#endif				/* __LOCAL_ADWM_H__ */
