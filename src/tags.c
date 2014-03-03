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
#include "config.h"

static void
_tag(Client *c, int index)
{
	c->tags |= (index == -1) ? ((1ULL << scr->ntags) - 1) : (1ULL << index);
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

	if (!c || (!c->can.stick && c->is.managed) || !(m = c->curmon))
		return;

	c->is.sticky = !c->is.sticky;
	if (c->is.managed) {
		if (c->is.sticky)
			tag(c, -1);
		else
			tag(c, m->curtag);
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
	else if (c->curmon)
		/* at least one tag must be enabled */
		c->tags = (1ULL << c->curmon->curtag);
	ewmh_update_net_window_desktop(c);
	drawclient(c);
	arrange(NULL);
}

void
toggleview(Monitor *cm, int index)
{
	unsigned int i, j;
	Monitor *m;
	unsigned long long tags;

	/* Typically from a keyboard command.  Again, we should use the monitor with the
	   keyboard focus before the monitor with the pointer in it. */

	i = (index == -1) ? 0 : index;
	tags = (1ULL << i);
	cm->prevtags = cm->seltags;
	cm->seltags ^= tags;
	for (m = scr->monitors; m; m = m->next) {
		if ((m->seltags & tags) && m != cm) {
			m->prevtags = m->seltags;
			m->seltags &= ~tags;
			for (j = 0; j < scr->ntags && !(m->seltags & (1ULL << j)); j++) ;
			if (j == scr->ntags) {
				m->seltags |= tags;	/* at least one tag must be
							   viewed */
				cm->seltags &= ~tags;	/* can't toggle */
				j = i;
			}
			if (m->curtag == i)
				m->curtag = j;
			arrange(m);
		}
	}
	arrange(cm);
	focus(NULL);
	ewmh_update_net_current_desktop();
}

void
focusview(Monitor *cm, int index)
{
	unsigned long long tags;
	unsigned int i;
	Client *c;

	i = (index == -1) ? 0 : index;
	tags = (1ULL << i);
	if (!(cm->seltags & tags))
		toggleview(cm, i);
	if (!(cm->seltags & tags))
		return;
	for (c = scr->stack; c; c = c->snext)
		if ((c->tags & tags) && !c->is.bastard && !c->is.dockapp)
			/* XXX: c->can.focus? */
			break;
	if (c)
		focus(c);
	// restack(); /* XXX: really necessary? */
}

void
view(int index)
{
	unsigned long long tags;
	int i;
	Monitor *m, *cm;
	int prevtag;

	i = (index == -1) ? 0 : index;
	tags = (1ULL << i);
	if (!(cm = selmonitor()))
		cm = nearmonitor();

	if (cm->seltags & tags)
		return;

	cm->prevtags = cm->seltags;
	cm->seltags = tags;
	prevtag = cm->curtag;
	cm->curtag = i;
	for (m = scr->monitors; m; m = m->next) {
		if ((m->seltags & tags) && m != cm) {
			m->curtag = prevtag;
			m->prevtags = m->seltags;
			m->seltags = cm->prevtags;
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
viewprev(Monitor *m)
{
	unsigned long long tmptags, tag;
	unsigned int i;
	int prevcurtag;

	for (tag = 1, i = 0; i < scr->ntags - 1 && !(m->prevtags & tag); i++, tag <<= 1) ;
	prevcurtag = m->curtag;
	m->curtag = i;

	tmptags = m->seltags;
	m->seltags = m->prevtags;
	m->prevtags = tmptags;
	if (scr->views[prevcurtag].barpos != scr->views[m->curtag].barpos)
		updategeom(m);
	arrange(NULL);
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

void
viewleft(Monitor *m)
{
	unsigned long long tag;
	unsigned int i;

	/* wrap around: TODO: do full _NET_DESKTOP_LAYOUT */
	if (m->seltags & 1ULL) {
		view(scr->ntags - 1);
		return;
	}
	for (tag = 2ULL, i = 1; i < scr->ntags; i++, tag <<= 1) {
		if (m->seltags & tag) {
			view(i - 1);
			return;
		}
	}
}

void
viewlefttag()
{
	Monitor *cm;

	if (!(cm = selmonitor()))
		return;
	return viewleft(cm);
}

void
viewright(Monitor *m)
{
	unsigned long long tags;
	unsigned int i;

	for (tags = 1ULL, i = 0; i < scr->ntags - 1; i++, tags <<= 1) {
		if (m->seltags & tags) {
			view(i + 1);
			return;
		}
	}
	/* wrap around: TODO: do full _NET_DESKTOP_LAYOUT */
	if (i == scr->ntags - 1) {
		view(0);
		return;
	}
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
	view(index);
}

void
taketoprev(Client *c)
{
	Monitor *m;

	if (!c || !(m = c->curmon))
		return;
	viewprev(m);
	taketo(c, m->curtag);
}

void
taketoleft(Client *c)
{
	Monitor *m;

	if (!c || !(m = c->curmon))
		return;
	viewleft(m);
	taketo(c, m->curtag);
}

void
taketoright(Client *c)
{
	Monitor *m;

	if (!c || !(m = c->curmon))
		return;
	viewright(m);
	taketo(c, m->curtag);
}


static void
initview(unsigned int i)
{
	char conf[32], ltname;
	Layout *l;
	View *v = scr->views + i;

	v->layout = layouts;
	snprintf(conf, sizeof(conf), "tags.layout%d", i);
	strncpy(&ltname, getresource(conf, scr->options.deflayout), 1);
	for (l = layouts; l->symbol; l++)
		if (l->symbol == ltname) {
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
	if (scr->d.rows && scr->d.cols) {
		v->row = i / scr->d.cols;
		v->col = i - v->row * scr->d.cols;
	} else {
		v->row = -1;
		v->col = -1;
	}
}

static void
newview(unsigned int i)
{
	initview(i);
}

static void
inittag(unsigned i)
{
	char conf[32], def[8];

	snprintf(conf, sizeof(conf), "tags.name%d", i);
	snprintf(def, sizeof(def), "%u", i);
	snprintf(scr->tags[i].name, sizeof(scr->tags[i].name), "%s",
		 getresource(conf, def));
	scr->tags[i].dt = XInternAtom(dpy, scr->tags[i].name, False);
	DPRINTF("Assigned name '%s' to tag %u\n", scr->tags[i].name, i);
}

static void
newtag(unsigned i)
{
	inittag(i);
}

void
inittags()
{
	scr->ntags = scr->options.ntags;
	ewmh_process_net_number_of_desktops();
	scr->views = ecalloc(scr->ntags, sizeof(*scr->views));
	scr->tags = ecalloc(scr->ntags, sizeof(*scr->tags));
	ewmh_process_net_desktop_names();
}

void
initlayouts()
{
	unsigned i;

	for (i = 0; i < scr->ntags; i++)
		initview(i);
	ewmh_update_net_desktop_modes();
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
	if (cm->curtag == last)
		view(last - 1);

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
	Monitor *m;
	unsigned int n;

	if (scr->ntags >= sizeof(c->tags) * 8)
		return;		/* stop the insanity, go organic */

	n = scr->ntags + 1;
	scr->views = erealloc(scr->views, n * sizeof(*scr->views));
	newview(scr->ntags);
	scr->tags = erealloc(scr->tags, n * sizeof(*scr->tags));
	newtag(scr->ntags);

	for (c = scr->clients; c; c = c->next)
		if ((c->tags & ((1ULL << scr->ntags) - 1)) == ((1ULL << scr->ntags) - 1)
		    || c->is.sticky)
			c->tags |= (1ULL << scr->ntags);

	for (m = scr->monitors; m; m = m->next) {
		/* probably unnecessary */
		m->prevtags &= ~(1ULL << scr->ntags);
		m->seltags &= ~(1ULL << scr->ntags);
	}

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
