/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "resource.h"
#include "config.h"
#include "save.h" /* verification */


void
save(void)
{
	AScreen *save_screen = scr;
	char value[256] = { 0, };

	for (scr = screens; scr < screens + nscr; scr++) {
		if (!scr->managed)
			continue;
		putscreenres("attachaside", scr->options.attachaside ? "1" : "0");
		if (scr->options.command)
			putscreenres("command", scr->options.command);
		if (scr->options.command2)
			putscreenres("command2", scr->options.command2);
		if (scr->options.command3)
			putscreenres("command3", scr->options.command3);
		if (scr->options.menucommand)
			putscreenres("menucommand", scr->options.menucommand);
		putscreenres("decoratetiled", scr->options.decoratetiled ? "1" : "0");
		snprintf(value, sizeof(value), "%d", scr->options.hidebastards);
		putscreenres("hidebastards", value);
		putscreenres("strutsactive", scr->options.strutsactive ? "1" : "0");
		putscreenres("autoroll", scr->options.autoroll ? "1" : "0");
		snprintf(value, sizeof(value), "%d", scr->options.strutsdelay);
		putscreenres("strutsdelay", value);
		snprintf(value, sizeof(value), "%d", scr->options.sloppy);
		putscreenres("sloppy", value);
		snprintf(value, sizeof(value), "%d", scr->options.snap);
		putscreenres("snap", value);
		snprintf(value, sizeof(value), "%d", scr->options.dockpos);
		putscreenres("dock.position", value);
		snprintf(value, sizeof(value), "%d", scr->options.dockori);
		putscreenres("dock.orient", value);
		snprintf(value, sizeof(value), "%d", scr->options.dockmon);
		putscreenres("dock.monitor", value);
		snprintf(value, sizeof(value), "%d", scr->options.dragdist);
		putscreenres("dragdistance", value);
		snprintf(value, sizeof(value), "%f", scr->options.mwfact);
		putscreenres("mwfact", value);
		snprintf(value, sizeof(value), "%f", scr->options.mhfact);
		putscreenres("mhfact", value);

	}
	scr = save_screen;
}

