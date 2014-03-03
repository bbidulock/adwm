#include <regex.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include "adwm.h"
#include "layout.h"
#include "tags.h"
#include "config.h"

Options options;

void
initconfig(void)
{
	/* init appearance */
	options.attachaside = atoi(getresource("attachaside", "1")) ? True : False;
	strncpy(options.command, getresource("command", COMMAND),
		LENGTH(options.command));
	options.command[LENGTH(options.command) - 1] = '\0';
	options.dectiled = atoi(getresource("decoratetiled", STR(DECORATETILED)));
	options.decmax = atoi(getresource("decoratemax", STR(DECORATEMAX)));
	options.hidebastards = atoi(getresource("hidebastards", "0")) ? True : False;
	options.autoroll = atoi(getresource("autoroll", "0")) ? True : False;
	options.focus = atoi(getresource("sloppy", "0"));
	options.snap = atoi(getresource("snap", STR(SNAP)));
	options.dockpos = atoi(getresource("dock.position", "1"));
	options.dockori = atoi(getresource("dock.orient", "1"));
	options.dragdist = atoi(getresource("dragdistance", "5"));
}
