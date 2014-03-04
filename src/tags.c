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

static void
_tag(Client *c, int index)
{
	c->tags |= (index == -1) ? ((1ULL << scr->ntags) - 1) : (1ULL << index);
	if (c->is.managed)
		ewmh_update_net_window_desktop(c);
	arrange(NULL);
	focus(NULL);
}

void
tag(Client *c, int index)
{
	if (!c || (!c->can.tag && c->is.managed))
		return;
	return with_transients(c, &_tag, index);
}

void
togglesticky(Client *c)
{
	Monitor *m;

	if (!c || (!c->can.stick && c->is.managed) || !(m = c->cmon))
		return;

	c->is.sticky = !c->is.sticky;
	if (c->is.managed) {
		if (c->is.sticky)
			tag(c, -1);
		else
			tag(c, m->curview->index);
		ewmh_update_net_window_state(c);
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
	if (tags & ((1ULL << scr->ntags) - 1))
		c->tags = tags;
	else if (c->cmon)
		/* at least one tag must be enabled */
		c->tags = (1ULL << c->cmon->curview->index);
	if (c->is.managed)
		ewmh_update_net_window_desktop(c);
	drawclient(c);
	arrange(NULL);
}

void
toggleview(Monitor *cm, int index)
{
	Monitor *m;
	unsigned long long tags;

	/* Typically from a keyboard command.  Again, we should use the monitor with the
	   keyboard focus before the monitor with the pointer in it. */

	/* This function used to worry about having a selected tag for each monitor, but
	   this has now been moved to the view.  We don't care if there exists views that 
	   have no selected tag because they can still be assigned to a monitor and tags
	   toggled from there. */
	if (-1 > index || index >= scr->ntags)
		return;
	tags = (index == -1) ? ((1ULL << scr->ntags) - 1) : (1ULL << index);
	cm->curview->seltags ^= tags;
	for (m = scr->monitors; m; m = m->next)
		if ((m->curview->seltags & tags) && m != cm)
			arrange(m);
	arrange(cm);
	focus(NULL);
	ewmh_update_net_current_desktop();
}

void
focusview(Monitor *cm, int index)
{
	unsigned long long tags;
	Client *c;

	if (-1 > index || index >= scr->ntags)
		return;
	tags = (index == -1) ? ((1ULL << scr->ntags) - 1) : (1ULL << index);
	if (!(cm->curview->seltags & tags))
		toggleview(cm, index);
	for (c = scr->stack; c; c = c->snext)
		if ((c->tags & tags) && !c->is.bastard && !c->is.dockapp && c->can.focus)
			break;
	focus(c);
}

void
view(Monitor *cm, int index)
{
	Monitor *m;

	if (0 > index || index >= scr->ntags)
		return;

	if (cm->curview->index == index)
		return;

	/* FIXME: this is a swapping arrangment, we should follow the desktop layout and
	   shift all of the view on all of the monitors of this screen */
	cm->preview = cm->curview;
	cm->curview = scr->views + index;

	for (m = scr->monitors; m; m = m->next) {
		if (m == cm)
			continue;
		if (m->curview == cm->curview) {
			m->preview = m->curview;
			m->curview = cm->preview;
			updategeom(m);
			arrange(m);
		}
	}
	updategeom(cm);
	arrange(cm);
	focus(NULL);
	ewmh_update_net_current_desktop();
}

static void
viewprev(Monitor *cm)
{
	View *preview;
	Monitor *m;

	preview = cm->curview;
	cm->curview = cm->preview;
	cm->preview = preview;

	/* FIXME: this is a swapping arrangment, we should follow the desktop layout and
	   shift all of the view on all of the monitors of this screen */
	for (m = scr->monitors; m; m = m->next) {
		if (m == cm)
			continue;
		if (m->curview == cm->curview) {
			m->curview = m->preview;
			m->preview = cm->curview;
			updategeom(m);
			arrange(m);
		}
	}
	updategeom(cm);
	arrange(cm);
	focus(NULL);
	ewmh_update_net_current_desktop();
}

void
viewprevtag()
{
	Monitor *cm;

	if (!(cm = selmonitor()))
		return;
	return viewprev(cm);
}

static void
viewleft(Monitor *m)
{
	/* wrap around: TODO: do full _NET_DESKTOP_LAYOUT */
	if (m->curview->index == 0)
		view(m, scr->ntags - 1);
	else
		view(m, m->curview->index - 1);
}

void
viewlefttag()
{
	Monitor *cm;

	if (!(cm = selmonitor()))
		return;
	return viewleft(cm);
}

static void
viewright(Monitor *m)
{
	/* wrap around: TODO: do full _NET_DESKTOP_LAYOUT */
	if (m->curview->index == scr->ntags - 1)
		view(m, 0);
	else
		view(m, m->curview->index + 1);
}

void
viewrighttag()
{
	Monitor *cm;

	if (!(cm = selmonitor()))
		return;
	return viewright(cm);
}

void
taketo(Client *c, int index)
{
	if (!c || (!c->can.tag && c->is.managed))
		return;
	with_transients(c, &_tag, index);
	view(c->cmon, index);
}

void
taketoprev(Client *c)
{
	Monitor *m;

	if (!c || !(m = c->cmon))
		return;
	viewprev(m);
	taketo(c, m->curview->index);
}

void
taketoleft(Client *c)
{
	Monitor *m;

	if (!c || !(m = c->cmon))
		return;
	viewleft(m);
	taketo(c, m->curview->index);
}

void
taketoright(Client *c)
{
	Monitor *m;

	if (!c || !(m = c->cmon))
		return;
	viewright(m);
	taketo(c, m->curview->index);
}

static Bool
isomni(Client *c)
{
	if (!c->is.sticky)
		if ((c->tags & ((1ULL << scr->ntags) - 1)) != ((1ULL << scr->ntags) - 1))
			return False;
	return True;
}

static void
deltag()
{
	Client *c;
	unsigned long long tags;
	unsigned int last;
	Monitor *cm;

	if (!(cm = selmonitor()))
		return;
	if (scr->ntags < 2)
		return;
	last = scr->ntags - 1;
	tags = (1ULL << last);

	/* move off the desktop being deleted */
	if (cm->curview->index == last)
		view(cm, last - 1);

	/* move windows off the desktop being deleted */
	for (c = scr->clients; c; c = c->next) {
		if (isomni(c)) {
			c->tags &= ~tags;
			continue;
		}
		if (!(c->tags & (tags - 1)))
			tag(c, last - 1);
		else
			c->tags &= ~tags;
	}

	--scr->ntags;
}

static void
addtag()
{
	Client *c;
	unsigned long long alltags;

	if (scr->ntags >= MAXTAGS)
		/* stop the insanity, go organic */
		return;
	alltags = ((1ULL << scr->ntags) - 1);
	for (c = scr->clients; c; c = c->next)
		if (((c->tags & alltags) == alltags) || c->is.sticky)
			c->tags |= (1ULL << scr->ntags);
	scr->ntags++;
}

void
settags(unsigned int numtags)
{
	if (1 > numtags || numtags > MAXTAGS)
		return;
	while (scr->ntags < numtags)
		addtag();
	while (scr->ntags > numtags)
		deltag();
	ewmh_process_net_desktop_names();
	ewmh_update_net_number_of_desktops();
}

void
appendtag()
{
	addtag();
	ewmh_process_net_desktop_names();
	ewmh_update_net_number_of_desktops();
}

void
rmlasttag()
{
	deltag();
	ewmh_process_net_desktop_names();
	ewmh_update_net_number_of_desktops();
}

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
