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
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "config.h"

Options options;

void
inittags(void)
{
	unsigned i;

	for (i = 0; i < MAXTAGS; i++) {
		const char *res;
		char name[256], def[8];
		Tag *t = scr->tags + i;

		snprintf(def, sizeof(def), "%u", i);
		snprintf(name, sizeof(name), "screen%d.tags.name%u", scr->screen, i);
		res = getresource(name, def);
		snprintf(t->name, sizeof(t->name), res);
	}
	scr->ntags = scr->options.ntags;
	ewmh_process_net_number_of_desktops();
	ewmh_process_net_desktop_names();
}

void
initlayouts(void)
{
	unsigned i;

	for (i = 0; i < MAXTAGS; i++) {
		const char *res;
		char name[256];
		Layout *l;
		View *v = scr->views + i;

		snprintf(name, sizeof(name), "screen%d.tags.layout%u", scr->screen, i);
		res = getresource(name, scr->options.deflayout);
		v->layout = layouts;
		for (l = layouts; l->symbol; l++)
			if (l->symbol == *res) {
				v->layout = l;
				break;
			}
		l = v->layout;
		v->barpos = StrutsOn;
		v->dectiled = scr->options.dectiled;
		v->nmaster = scr->options.nmaster;
		v->ncolumns = scr->options.ncolumns;
		v->mwfact = scr->options.mwfact;
		v->mhfact = scr->options.mhfact;
		v->major = l->major;
		v->minor = l->minor;
		v->placement = l->placement;
		v->index = i;
		v->seltags = (1ULL << i);
		if (scr->d.rows && scr->d.cols) {
			v->row = i / scr->d.cols;
			v->col = i - v->row * scr->d.cols;
		} else {
			v->row = -1;
			v->col = -1;
		}
	}
	ewmh_update_net_desktop_modes();
}

void
initscreen(void)
{
	const char *res;
	char name[256], *n;
	size_t nlen;

	snprintf(name, sizeof(name), "screen%d.", scr->screen);
	nlen = strnlen(name, sizeof(name));
	n = name + nlen;
	nlen = sizeof(name) - nlen;

	scr->options = options;

	strncpy(n, "attachside", nlen);
	if ((res = getresource(name, NULL)))
		scr->options.attachaside = atoi(res) ? True : False;
	strncpy(n, "command", nlen);
	if ((res = getresource(name, NULL)))
		scr->options.command = res;
	strncpy(n, "decoratetiled", nlen);
	if ((res = getresource(name, NULL)))
		scr->options.dectiled = atoi(res);
	strncpy(n, "decoratemax", nlen);
	if ((res = getresource(name, NULL)))
		scr->options.decmax = atoi(res) ? True : False;
	strncpy(n, "hidebastards", nlen);
	if ((res = getresource(name, NULL)))
		scr->options.hidebastards = atoi(res) ? True : False;
	strncpy(n, "autoroll", nlen);
	if ((res = getresource(name, NULL)))
		scr->options.autoroll = atoi(res) ? True : False;
	strncpy(n, "sloppy", nlen);
	if ((res = getresource(name, NULL)))
		scr->options.focus = atoi(res);
	strncpy(n, "snap", nlen);
	if ((res = getresource(name, NULL)))
		scr->options.snap = atoi(res);
	strncpy(n, "dock.position", nlen);
	if ((res = getresource(name, NULL)))
		scr->options.dockpos = atoi(res);
	strncpy(n, "dock.orient", nlen);
	if ((res = getresource(name, NULL)))
		scr->options.dockori = atoi(res);
	strncpy(n, "dragdistance", nlen);
	if ((res = getresource(name, NULL)))
		scr->options.dragdist = atoi(res);
	strncpy(n, "mwfact", nlen);
	if ((res = getresource(name, NULL))) {
		scr->options.mwfact = atof(res);
		if (scr->options.mwfact < 0.10 || scr->options.mwfact > 0.90)
			scr->options.mwfact = options.mwfact;
	}
	strncpy(n, "mhfact", nlen);
	if ((res = getresource(name, NULL))) {
		scr->options.mhfact = atof(res);
		if (scr->options.mhfact < 0.10 || scr->options.mhfact > 0.90)
			scr->options.mhfact = options.mhfact;
	}
	strncpy(n, "nmaster", nlen);
	if ((res = getresource(name, NULL))) {
		scr->options.nmaster = atoi(res);
		if (scr->options.nmaster < 1 || scr->options.nmaster > 10)
			scr->options.nmaster = options.nmaster;
	}
	strncpy(n, "ncolumns", nlen);
	if ((res = getresource(name, NULL))) {
		scr->options.ncolumns = atoi(res);
		if (scr->options.ncolumns < 1 || scr->options.ncolumns > 10)
			scr->options.ncolumns = options.ncolumns;
	}
	strncpy(n, "deflayout", nlen);
	if ((res = getresource(name, NULL))) {
		if (strnlen(res, 2) == 1)
			scr->options.deflayout = res;
	}
}

void
initconfig(void)
{
	const char *res;

	/* init appearance */
	options.attachaside = atoi(getresource("attachaside", "1")) ? True : False;
	options.command = getresource("command", COMMAND);
	options.dectiled = atoi(getresource("decoratetiled", STR(DECORATETILED)));
	options.decmax = atoi(getresource("decoratemax", STR(DECORATEMAX)));
	options.hidebastards = atoi(getresource("hidebastards", "0")) ? True : False;
	options.autoroll = atoi(getresource("autoroll", "0")) ? True : False;
	options.focus = atoi(getresource("sloppy", "0"));
	options.snap = atoi(getresource("snap", STR(SNAP)));
	options.dockpos = atoi(getresource("dock.position", "1"));
	options.dockori = atoi(getresource("dock.orient", "1"));
	options.dragdist = atoi(getresource("dragdistance", "5"));

	options.mwfact = atof(getresource("mwfact", STR(DEFMWFACT)));
	if (options.mwfact < 0.10 || options.mwfact > 0.90)
		options.mwfact = DEFMWFACT;

	options.mhfact = atof(getresource("mhfact", STR(DEFMHFACT)));
	if (options.mhfact < 0.10 || options.mwfact > 0.90)
		options.mhfact = DEFMHFACT;

	options.nmaster = atoi(getresource("nmaster", STR(DEFNMASTER)));
	if (options.nmaster < 1 || options.nmaster > 10)
		options.nmaster = DEFNMASTER;

	options.ncolumns = atoi(getresource("ncolumns", STR(DEFNCOLUMNS)));
	if (options.ncolumns < 1 || options.ncolumns > 10)
		options.ncolumns = DEFNCOLUMNS;

	res = getresource("deflayout", "i");
	if (strlen(res) == 1)
		options.deflayout = res;
	else
		options.deflayout = "i";

	options.ntags = strtoul(getresource("tags.number", "5"), NULL, 0);
	if (options.ntags < 1 || options.ntags > MAXTAGS)
		options.ntags = 5;
}
