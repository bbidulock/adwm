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
#include "layout.h"
#include "draw.h"
#include "ewmh.h"
#include "config.h"
#include "tags.h" /* verification */

static Bool
_tag(Client *c, int index)
{
	unsigned long long tags;
	View *oldv, *newv;

	tags = (index == -1) ? ((1ULL << scr->ntags) - 1) : (1ULL << index);
	if ((c->tags & tags) == tags)
		return False;

	oldv = clientview(c);
	c->tags |= tags;
	newv = clientview(c);

	if (c->is.managed)
		ewmh_update_net_window_desktop(c);

	if (newv != oldv) {
		if (newv)
			needarrange(newv);
		if (oldv)
			needarrange(oldv);
	}
	return True;
}

void
tag(Client *c, int index)
{
	if (!c)
		return;
	if (!c->can.tag && c->is.managed)
		return;
	if (with_transients(c, &_tag, index)) {
		arrangeneeded();
		focus(sel);
	}
}

static Bool
_tagonly(Client *c, int index)
{
	unsigned long long tags;
	View *oldv, *newv;

	tags = (index == -1) ? ((1ULL << scr->ntags) - 1) : (1ULL << index);
	if (c->tags == tags)
		return False;
	oldv = clientview(c);
	if (!(c->tags = tags))
		c->tags = (1ULL << c->cview->index);
	newv = clientview(c);

	if (c->is.managed)
		ewmh_update_net_window_desktop(c);

	if (newv != oldv) {
		if (newv)
			needarrange(newv);
		if (oldv)
			needarrange(oldv);
	}
	return True;
}

void
tagonly(Client *c, int index)
{
	if (!c)
		return;
	if (!c->can.tag && c->is.managed)
		return;
	if (with_transients(c, &_tagonly, index)) {
		arrangeneeded();
		focus(sel);
	}
}

void
togglesticky(Client *c)
{
	View *v;

	if (!c || (!c->can.stick && c->is.managed) || !(v = c->cview))
		return;

	c->is.sticky = !c->is.sticky;
	if (c->is.managed) {
		if (c->is.sticky)
			tag(c, -1);
		else
			tagonly(c, v->index);
		ewmh_update_net_window_state(c);
		drawclient(c);
		arrange(NULL);
	}
}

void
toggletag(Client *c, int index)
{
	unsigned long long tags;

	if (!c || (!c->can.tag && c->is.managed))
		return;
	tags = c->tags;
	tags ^= (index == -1) ? ((1ULL << scr->ntags) - 1) : (1ULL << index);
	if ((tags & ((1ULL << scr->ntags) - 1)) || c->is.sticky)
		c->tags = tags;
	else if (c->cview)
		/* at least one tag must be enabled */
		c->tags = (1ULL << c->cview->index);
	if (c->is.managed)
		ewmh_update_net_window_desktop(c);
	drawclient(c);
	arrange(NULL);
}

Client *
lastselected(View *v)
{
	return (v ? v->lastsel : NULL);
}

void
toggleview(View *cv, int index)
{
	Client *c;
	View *v;
	unsigned long long tags;
	unsigned i;

	/* Typically from a keyboard command.  Again, we should use the monitor with the
	   keyboard focus before the monitor with the pointer in it. */

	/* This function used to worry about having a selected tag for each monitor, but
	   this has now been moved to the view.  We don't care if there exists views that
	   have no selected tag because they can still be assigned to a monitor and tags
	   toggled from there. */
	if (-1 > index || index >= scr->ntags)
		return;
	tags = (index == -1) ? ((1ULL << scr->ntags) - 1) : (1ULL << index);
	cv->seltags ^= tags;
	for (v = scr->views, i = 0; i < scr->ntags; i++, v++)
		if ((v->seltags & tags) && v != cv)
			arrange(v);
	arrange(cv);
	c = lastselected(cv);
	focus(c);
	discardcrossing(c);
	ewmh_update_net_current_desktop();
}

void
focusview(View *v, int index)
{
	unsigned long long tags;
	Client *c;

	if (-1 > index || index >= scr->ntags)
		return;
	tags = (index == -1) ? ((1ULL << scr->ntags) - 1) : (1ULL << index);
	if (!(v->seltags & tags))
		toggleview(v, index);
	for (c = scr->stack; c; c = c->snext)
		if (((c->tags & tags) || c->is.sticky) &&
		    !c->is.bastard && !c->is.dockapp && c->can.focus)
			break;
	focus(c);
}

void
view(View *ov, int index)
{
	Monitor *cm, *om;
	Client *c;
	View *cv;

	if (0 > index || index >= scr->ntags) {
		XPRINTF("WARNING: bad index %d\n", index);
		return;
	}
	if (ov->index == index) {
		XPRINTF("WARNING: view already index %d\n", index);
		return;
	}
	if (!(cm = ov->curmon)) {
		XPRINTF("WARNING: view %d has no monitor\n", ov->index);
		return;
	}
	XPRINTF("VIEW: disassociating monitor %d from view %d\n", cm->num, ov->index);
	ov->curmon = NULL;
	XPRINTF("VIEW: setting previous view for monitor %d to view %d\n", cm->num, ov->index);
	cm->preview = ov;
	XPRINTF("VIEW: new view is %d\n", index);
	cv = scr->views + index;
	XPRINTF("VIEW: associating monitor %d with view %d\n", cm->num, cv->index);
	cv->curmon = cm;
	XPRINTF("VIEW: associating view %d with monitor %d\n", cv->index, cm->num);
	cm->curview = cv;
	for (om = scr->monitors; om; om = om->next) {
		if (om == cm)
			continue;
		if (om->curview == cv) {
			XPRINTF("VIEW: switching monitor %d from view %d to %d\n",
					om->num, om->curview->index, ov->index);
			om->curview = ov;
			ov->curmon = om;
			updategeom(om);
			XPRINTF("VIEW: arranging view %d\n", ov->index);
			arrange(ov);
			/* only one can match */
			break;
		}
	}
	updategeom(cm);
	XPRINTF("VIEW: arranging view %d\n", cv->index);
	arrange(cv);
	c = lastselected(cv);
	focus(c);
	discardcrossing(c);
	ewmh_update_net_current_desktop();
}



static void
viewprev(View *ov)
{
	Monitor *cm;

	if (!ov || !(cm = ov->curmon) || !cm->preview || cm->preview == cm->curview)
		return;
	view(ov, cm->preview->index);
}

void
viewprevtag()
{
	return viewprev(selview());
}

static void
viewleft(View *v)
{
	/* wrap around: TODO: do full _NET_DESKTOP_LAYOUT */
	if (v->index == 0)
		view(v, scr->ntags - 1);
	else
		view(v, v->index - 1);
}

void
viewlefttag()
{
	View *v;

	if ((v = selview()))
		viewleft(v);
}

static void
viewright(View *v)
{
	/* wrap around: TODO: do full _NET_DESKTOP_LAYOUT */
	if (v->index == scr->ntags - 1)
		view(v, 0);
	else
		view(v, v->index + 1);
}

void
viewrighttag()
{
	View *v;

	if ((v = selview()))
		viewright(v);
}

void
taketo(Client *c, int index)
{
	tagonly(c, index);
	view(c->cview, index);
}

void
taketoprev(Client *c)
{
	Monitor *cm;
	View *v;

	if (!c || !(v = c->cview) || !(cm = v->curmon))
		return;
	viewprev(v);
	taketo(c, cm->curview->index);
}

void
taketoleft(Client *c)
{
	Monitor *cm;
	View *v;

	if (!c || !(v = c->cview) || !(cm = v->curmon))
		return;
	viewleft(v);
	taketo(c, cm->curview->index);
}

void
taketoright(Client *c)
{
	Monitor *cm;
	View *v;

	if (!c || !(v = c->cview) || !(cm = v->curmon))
		return;
	viewright(v);
	taketo(c, cm->curview->index);
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
deltag(void)
{
	Client *c;
	unsigned long long tags;
	unsigned int last;
	View *v;

	if (!(v = selview()) || !v->curmon)
		return;
	/* can't have fewer tags than monitors */
	if (scr->ntags <= 1 || scr->ntags <= scr->nmons)
		return;
	last = scr->ntags - 1;
	tags = (1ULL << last);

	/* move off the desktop being deleted */
	if (v->index == last)
		view(v, last - 1);

	/* move windows off the desktop being deleted */
	for (c = scr->clients; c; c = c->next) {
		if (isomni(c)) {
			c->tags &= ~tags;
			continue;
		}
		if (!(c->tags & (tags - 1)) && !c->is.sticky)
			tag(c, last - 1);
		else
			c->tags &= ~tags;
	}

	--scr->ntags;
}

void
addtag(void)
{
	Client *c;

	if (scr->ntags >= MAXTAGS)
		/* stop the insanity, go organic */
		return;
	for (c = scr->clients; c; c = c->next)
		if (isomni(c))
			c->tags |= (1ULL << scr->ntags);
	scr->ntags++;
}

void
settags(unsigned int numtags)
{
	if (1 > numtags || scr->nmons > numtags || numtags > MAXTAGS)
		return;
	while (scr->ntags < numtags)
		addtag();
	while (scr->ntags > numtags)
		deltag();
	ewmh_update_net_number_of_desktops();
	ewmh_process_net_desktop_names();
	ewmh_process_net_desktop_layout();
}

void
appendtag()
{
	addtag();
	ewmh_update_net_number_of_desktops();
	ewmh_process_net_desktop_names();
	ewmh_process_net_desktop_layout();
}

void
rmlasttag()
{
	deltag();
	ewmh_update_net_number_of_desktops();
	ewmh_process_net_desktop_names();
	ewmh_process_net_desktop_layout();
}

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
