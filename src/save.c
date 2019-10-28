/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "resource.h"
#include "config.h"
#include "save.h" /* verification */

extern AdwmPlaces config;

/*
 * Steps involved in saving window manager state in response to SaveYourself Phase 1:
 *
 * 1. Create an Xresource configuration database.
 * 2. Save all current configuration parameters (config, styles, themes, keys, mouse, desktops) to the resource database.
 * 3. Create a file for client state and write filename to configuration database.
 * 4. Save the database to a new global or local unique filename.
 * 5. Issue WM_SAVE_YOURSELF client messages to all X11R5 session management observing clients.
 * 6. Request SaveYourself Phase 2.
 *
 * Steps involved in saving window manager state in response to SaveYourself Phase 2:
 *
 * 1. Write all X11R6 SM observing clients to the client state file.
 * 2. Write all X11R5 SM observing clients to the client state file.  (Noting startup notification
 *    observance.)
 * 3. Write all non-SM observing clients with WM_COMMAND set to client state file.  (Noting startup
 *    notification observance.)
 * 4. Write all non-SM observing clients with startup notification assistance to client state file
 *    if WM_COMMAND can be determined or was implied by startup notification.  For example, if
 *    _NET_WM_PID is specified on a window, the /proc filesystem may be used to determine the
 *    command that was used to launch the client.
 * 5. Issue SaveYourselfComplete to session manager.
 *
 * Note that when dock applications are saved, they should be saved in the order in which they
 * appear in the dock and their dock app position saved as well.  Note also that stacking order,
 * focus order, client order, must be saved in addition to the full state of the clients saved.
 * Tiled windows must also have their saved floating geometries and positions saved as well.
 *
 */

const char *
show_geometry(Geometry *g)
{
	static char buf[128] = { 0, };

	snprintf(buf, sizeof(buf), "%ux%u%c%u%c%u:%u", (unsigned) g->w, (unsigned) g->h,
		 g->x >= 0 ? '+' : '-', (unsigned) abs(g->x),
		 g->y >= 0 ? '+' : '-', (unsigned) abs(g->y), (unsigned) g->b);
	return (buf);
}

const char *
show_client_geometry(ClientGeometry *g)
{
	static char buf[128] = { 0, };

	snprintf(buf, sizeof(buf), "%s(%u,%u,%u)", show_geometry((Geometry *)g),
			(unsigned) g->t,
			(unsigned) g->g,
			(unsigned) g->v);
	return (buf);
}

void
save(FILE *file, Bool permanent)
{
	AScreen *save_screen = scr;
	const char *str;
	unsigned i;

	/* CONFIGURATION DIRECTIVES */
	if (permanent) {

		/* The permanent form is used for X11R6 session management and saves the
		   complete path to ancilliary configuration files, themes, styles, keys
		   and dock.  Once we provide ability to change key definitions while
		   running, we should likely store the keys and remove the reference to
		   the keys file.  Likely we should already be saving the dock
		   configuration in the main file and remove the reference to the dock
		   file.  Themes and style, however, are normally loaded on the system and 
		   should only be referenced. */

		if ((str = config.themefile) && strcmp(str, "themerc"))
			fprintf(file, "Adwm*themeFile:\t\t%s\n", str);
		if ((str = config.themename) && strcmp(str, "Default"))
			fprintf(file, "Adwm*themeName:\t\t%s\n", str);
		if ((str = config.stylefile) && strcmp(str, "stylerc"))
			fprintf(file, "Adwm*styleFile:\t\t%s\n", str);
		if ((str = config.stylename) && strcmp(str, "Default"))
			fprintf(file, "Adwm*styleName:\t\t%s\n", str);
		if ((str = config.keysfile) && strcmp(str, "keysrc"))
			fprintf(file, "Adwm*keysFile:\t\t%s\n", str);
		if ((str = config.btnsfile) && strcmp(str, "buttonrc"))
			fprintf(file, "Adwm*buttonFile:\t\t%s\n", str);
		if ((str = config.rulefile) && strcmp(str, "rulerc"))
			fprintf(file, "Adwm*ruleFile:\t\t%s\n", str);

		/* should probably save the dock definitions in the main file */

		if ((str = config.dockfile) && strcmp(str, "dockrc"))
			fprintf(file, "Adwm*dockFile:\t\t%s\n", str);
	} else {

		/* The intention of the non-permanent form is to simply save the main
		   adwmrc file and just link it relatively to the ancilliary files that
		   it references. This is to simply save the configuration so that each
		   time adwm boots it uses the saved configuration.  This is not used by
		   X11R6 session management. */

		if ((str = getresource("themeFile", NULL)) && strcmp(str, "themerc"))
			fprintf(file, "Adwm*themeFile:\t\t%s\n", str);
		if ((str = getresource("themeName", NULL)) && strcmp(str, "Default"))
			fprintf(file, "Adwm*themeName:\t\t%s\n", str);
		if ((str = getresource("styleFile", NULL)) && strcmp(str, "stylerc"))
			fprintf(file, "Adwm*styleFile:\t\t%s\n", str);
		if ((str = getresource("styleName", NULL)) && strcmp(str, "Default"))
			fprintf(file, "Adwm*styleName:\t\t%s\n", str);
		if ((str = getresource("keysFile", NULL)) && strcmp(str, "keysrc"))
			fprintf(file, "Adwm*keysFile:\t\t%s\n", str);
		if ((str = getresource("buttonFile", NULL)) && strcmp(str, "buttonrc"))
			fprintf(file, "Adwm*buttonFile:\t\t%s\n", str);
		if ((str = getresource("ruleFile", NULL)) && strcmp(str, "rulerc"))
			fprintf(file, "Adwm*ruleFile:\t\t%s\n", str);
		if ((str = getresource("dockFile", NULL)) && strcmp(str, "dockrc"))
			fprintf(file, "Adwm*dockFile:\t\t%s\n", str);
	}

	/* GENERAL DIRECTIVES */
	if (options.debug)
		fprintf(file, "Adwm*debug:\t\t%d\n", options.debug);
	if (options.useveil)
		fprintf(file, "Adwm*useveil:\t\t%d\n", options.useveil);
	if (!options.attachaside)
		fprintf(file, "Adwm*attachaside:\t\t%d\n", options.attachaside);
	if (options.command && COMMAND && strcmp(options.command, COMMAND))
		fprintf(file, "Adwm*command:\t\t%s\n", options.command);
	if (options.command2)
		fprintf(file, "Adwm*command2:\t\t%s\n", options.command2);
	if (options.command3)
		fprintf(file, "Adwm*command3:\t\t%s\n", options.command3);
	if (options.menucommand)
		fprintf(file, "Adwm*menucommand:\t\t%s\n", options.menucommand);
	if (options.dectiled != DECORATETILED)
		fprintf(file, "Adwm*decoratetiled:\t\t%d\n", options.dectiled);
	if (options.decmax != DECORATEMAX)
		fprintf(file, "Adwm*decoratemax:\t\t%d\n", options.decmax);
	if (options.hidebastards)
		fprintf(file, "Adwm*hidebastards:\t\t%d\n", options.hidebastards);
	if (options.strutsactive)
		fprintf(file, "Adwm*strutsactive:\t\t%d\n", options.strutsactive);
	if (options.autoroll)
		fprintf(file, "Adwm*autoroll:\t\t%d\n", options.autoroll);
	if (options.showdesk)
		fprintf(file, "Adwm*showdesk:\t\t%d\n", options.showdesk);
	if (options.strutsdelay != 500)
		fprintf(file, "Adwm*strutsdelay:\t\t%lu\n", options.strutsdelay);
	if (options.focus)
		fprintf(file, "Adwm*sloppy:\t\t%d\n", options.focus);
	if (options.snap != SNAP)
		fprintf(file, "Adwm*snap:\t\t%d\n", options.snap);
	if (options.dockpos != 1)
		fprintf(file, "Adwm*dock.position:\t\t%d\n", options.dockpos);
	if (options.dockori != 1)
		fprintf(file, "Adwm*dock.orient:\t\t%d\n", options.dockori);
	if (options.dockmon != 0)
		fprintf(file, "Adwm*dock.monitor:\t\t%d\n", options.dockmon);
	if (options.dragdist != 5)
		fprintf(file, "Adwm*dragdistance:\t\t%d\n", options.dragdist);
	if (options.mwfact != DEFMWFACT)
		fprintf(file, "Adwm*mwfact:\t\t%f\n", options.mwfact);
	if (options.mhfact != DEFMHFACT)
		fprintf(file, "Adwm*mhfact:\t\t%f\n", options.mhfact);
	if (options.nmaster != DEFNMASTER)
		fprintf(file, "Adwm*nmaster:\t\t%d\n", options.nmaster);
	if (options.ncolumns != DEFNCOLUMNS)
		fprintf(file, "Adwm*ncolumns:\t\t%d\n", options.ncolumns);
	if (options.deflayout != 'i')
		fprintf(file, "Adwm*deflayout:\t\t%c\n", options.deflayout);
	if (options.placement != ColSmartPlacement)
		fprintf(file, "Adwm*placement:\t\t%c\n", options.placement);
	if (options.ntags != 5)
		fprintf(file, "Adwm*tags.number:\t\t%d\n", options.ntags);
	if (options.prependdirs)
		fprintf(file, "Adwm*icon.prependdirs:\t\t%s\n", options.prependdirs);
	if (options.appenddirs)
		fprintf(file, "Adwm*icon.appenddirs:\t\t%s\n", options.appenddirs);
	if (options.icontheme)
		fprintf(file, "Adwm*icon.theme:\t\t%s\n", options.icontheme);
	if (options.extensions)
		fprintf(file, "Adwm*icon.extensions:\t\t%s\n", options.extensions);

	/* SCREEN DIRECTIVES */
	for (scr = screens; scr < screens + nscr; scr++) {

		if (!scr->managed)
			continue;
		if (scr->options.attachaside != options.attachaside)
			fprintf(file, "Adwm.screen%u.attachaside:\t\t%d\n", scr->screen, scr->options.attachaside);
		if (scr->options.command && options.command && strcmp(scr->options.command, options.command))
			fprintf(file, "Adwm.screen%u.command:\t\t%d\n", scr->screen, scr->options.attachaside);
		if (scr->options.command2 && options.command2 && strcmp(scr->options.command2, options.command2))
			fprintf(file, "Adwm.screen%u.command2:\t\t%d\n", scr->screen, scr->options.attachaside);
		if (scr->options.command3 && options.command3 && strcmp(scr->options.command3, options.command3))
			fprintf(file, "Adwm.screen%u.command3:\t\t%d\n", scr->screen, scr->options.attachaside);
		if (scr->options.menucommand && options.menucommand && strcmp(scr->options.menucommand, options.menucommand))
			fprintf(file, "Adwm.screen%u.menucommand:\t\t%d\n", scr->screen, scr->options.attachaside);
		if (scr->options.dectiled != options.dectiled)
			fprintf(file, "Adwm.screen%u.decoratetiled:\t\t%d\n", scr->screen, scr->options.dectiled);
		if (scr->options.decmax != options.decmax)
			fprintf(file, "Adwm.screen%u.decoratemax:\t\t%d\n", scr->screen, scr->options.decmax);
		if (scr->options.hidebastards != options.hidebastards)
			fprintf(file, "Adwm.screen%u.hidebastards:\t\t%d\n", scr->screen, scr->options.hidebastards);
		if (scr->options.strutsactive != options.strutsactive)
			fprintf(file, "Adwm.screen%u.strutsactive:\t\t%d\n", scr->screen, scr->options.strutsactive);
		if (scr->options.autoroll != options.autoroll)
			fprintf(file, "Adwm.screen%u.autoroll:\t\t%d\n", scr->screen, scr->options.autoroll);
		if (scr->options.showdesk != options.showdesk)
			fprintf(file, "Adwm.screen%u.showdesk:\t\t%d\n", scr->screen, scr->options.showdesk);
		if (scr->options.strutsdelay != options.strutsdelay)
			fprintf(file, "Adwm.screen%u.strutsdelay:\t\t%lu\n", scr->screen, scr->options.strutsdelay);
		if (scr->options.focus != options.focus)
			fprintf(file, "Adwm.screen%u.sloppy:\t\t%d\n", scr->screen, scr->options.focus);
		if (scr->options.snap != options.snap)
			fprintf(file, "Adwm.screen%u.snap:\t\t%d\n", scr->screen, scr->options.snap);
		if (scr->options.dockpos != options.dockpos)
			fprintf(file, "Adwm.screen%u.dock.position:\t\t%d\n", scr->screen, scr->options.dockpos);
		if (scr->options.dockori != options.dockori)
			fprintf(file, "Adwm.screen%u.dock.orient:\t\t%d\n", scr->screen, scr->options.dockori);
		if (scr->options.dockmon != options.dockmon)
			fprintf(file, "Adwm.screen%u.dock.monitor:\t\t%d\n", scr->screen, scr->options.dockmon);
		if (scr->options.dragdist != options.dragdist)
			fprintf(file, "Adwm.screen%u.dragdistance:\t\t%d\n", scr->screen, scr->options.dragdist);
		if (scr->options.mwfact != options.mwfact)
			fprintf(file, "Adwm.screen%u.mwfact:\t\t%f\n", scr->screen, scr->options.mwfact);
		if (scr->options.mhfact != options.mhfact)
			fprintf(file, "Adwm.screen%u.mhfact:\t\t%f\n", scr->screen, scr->options.mhfact);
		if (scr->options.nmaster != options.nmaster)
			fprintf(file, "Adwm.screen%u.nmaster:\t\t%d\n", scr->screen, scr->options.nmaster);
		if (scr->options.ncolumns != options.ncolumns)
			fprintf(file, "Adwm.screen%u.ncolumns:\t\t%d\n", scr->screen, scr->options.ncolumns);
		if (scr->options.deflayout != options.deflayout)
			fprintf(file, "Adwm.screen%u.deflayout:\t\t%c\n", scr->screen, scr->options.deflayout);
		if (scr->options.placement != options.placement)
			fprintf(file, "Adwm.screen%u.placement:\t\t%c\n", scr->screen, scr->options.placement);

		/* Note: we should only save monitor information in display/home or display/current
		   save files.  The only information that can be restored, really, is which view was
		   active or previously active on which monitors.  When restoring, this information
		   can be set for the monitors that are present. */

		/* MONITOR DIRECTIVES */
		fprintf(file, "Adwm.screen%u.monitors:\t\t%d\n", scr->screen, scr->nmons);
		for (i = 0; i < scr->nmons; i++) {
			Monitor *m = scr->monitors + i;

			fprintf(file, "Adwm.screen%u.monitor%u.col:\t\t%d\n", scr->screen, i, m->col); /* XXX: calculated? */
			fprintf(file, "Adwm.screen%u.monitor%u.row:\t\t%d\n", scr->screen, i, m->row); /* XXX: calculated? */
			if (m->curview)
				fprintf(file, "Adwm.screen%u.monitor%u.curview:\t\t%d\n", scr->screen, i, m->curview->index);
			if (m->preview)
				fprintf(file, "Adwm.screen%u.monitor%u.preview:\t\t%d\n", scr->screen, i, m->preview->index);
		}

		/* TAG DIRECTIVES */
		if (scr->ntags != options.ntags)
			fprintf(file, "Adwm.screen%u.tags.number:\t\t%d\n", scr->screen, scr->ntags);
		for (i = 0; i < scr->ntags; i++) {
			Tag *t = scr->tags + i;

			if (t->name)
				fprintf(file, "Adwm.screen%u.tags.name%u:\t\t%s\n", scr->screen, i, t->name);
		}

		/* VIEW DIRECTIVES */
		for (i = 0; i < scr->ntags; i++) {
			View *v = scr->views + i;
			Layout *l = v->layout;

			/* this one was poorly named */
			if (l->symbol != scr->options.deflayout)
				fprintf(file, "Adwm.screen%u.tags.layout%u:\t\t%c\n", scr->screen, i, l->symbol);
			if (v->major != l->major)
				fprintf(file, "Adwm.screen%u.view%u.major:\t\t%d\n", scr->screen, i, v->major);
			if (v->minor != l->minor)
				fprintf(file, "Adwm.screen%u.view%u.minor:\t\t%d\n", scr->screen, i, v->minor);
			if (v->placement != scr->options.placement)
				fprintf(file, "Adwm.screen%u.view%u.placement:\t\t%d\n", scr->screen, i, v->placement);
			if ((v->barpos == StrutsOn) != !!scr->options.strutsactive)
				fprintf(file, "Adwm.screen%u.view%u.strutsactive:\t\t%d\n", scr->screen, i, (v->barpos == StrutsOn) ?  1 : 0);
			if (v->dectiled != scr->options.dectiled)
				fprintf(file, "Adwm.screen%u.view%u.decoratetiled:\t\t%d\n", scr->screen, i, v->dectiled);
			if (v->nmaster != scr->options.nmaster)
				fprintf(file, "Adwm.screen%u.view%u.nmaster:\t\t%d\n", scr->screen, i, v->nmaster);
			if (v->ncolumns != scr->options.ncolumns)
				fprintf(file, "Adwm.screen%u.view%u.ncolumns:\t\t%d\n", scr->screen, i, v->ncolumns);
			if (v->mwfact != scr->options.mwfact)
				fprintf(file, "Adwm.screen%u.view%u.mwfact:\t\t%f\n", scr->screen, i, v->mwfact);
			if (v->mhfact != scr->options.mhfact)
				fprintf(file, "Adwm.screen%u.view%u.mhfact:\t\t%f\n", scr->screen, i, v->mhfact);
			if (v->seltags != (1ULL << i))
				fprintf(file, "Adwm.screen%u.view%u.seltags:\t\t0x%llx\n", scr->screen, i, v->seltags);
		}

	}
	/* RULE DIRECTIVES */
	for (i = 0; i < 64; i++) {
		Rule *r;

		if ((r = rules[i])) {
			int f;
			unsigned mask;
#if 0
			/* old deprecated format */
			fprintf(file, "Adwm.rule%u:\t\t%s %s %d %d\n", i, r->prop, r->tags, r->isfloating, r->hastitle);
#else
			if (r->prop)
				fprintf(file, "Adwm.rule%u.prop:\t\t%s\n", i, r->prop);
			if (r->tags)
				fprintf(file, "Adwm.rule%u.tags:\t\t%s\n", i, r->tags);
			if (r->is.set.is)
				for (f = 0, mask = 1; f < 32; f++, mask <<= 1)
					if ((r->is.set.is & is_mask.is & mask) && is_fields[f])
						fprintf(file, "Adwm.rule%u.is.%s:\t\t%d\n", i, is_fields[f], (r->is.is.is & mask) ?  1 : 0);
			if (r->skip.set.skip)
				for (f = 0, mask = 1; f < 32; f++, mask <<= 1)
					if ((r->skip.set.skip & skip_mask.skip & mask) && skip_fields[f])
						fprintf(file, "Adwm.rule%u.skip.%s:\t\t%d\n", i, skip_fields[f], (r->skip.skip.skip & mask) ?  1 : 0);
			if (r->has.set.has)
				for (f = 0, mask = 1; f < 32; f++, mask <<= 1)
					if ((r->has.set.has & has_mask.has & mask) && has_fields[f])
						fprintf(file, "Adwm.rule%u.has.%s:\t\t%d\n", i, has_fields[f], (r->has.has.has & mask) ?  1 : 0);
			if (r->can.set.can)
				for (f = 0, mask = 1; f < 32; f++, mask <<= 1)
					if ((r->can.set.can & can_mask.can & mask) && can_fields[f])
						fprintf(file, "Adwm.rule%u.can.%s:\t\t%d\n", i, can_fields[f], (r->can.can.can & mask) ?  1 : 0);
#endif
		}
	}

	if (!permanent)
		goto done;

	/* CLIENT DIRECTIVES */
	Client *c;

	/* mark an index on each client */
	for (scr = screens; scr < screens + nscr; scr++) {
		unsigned n;

		for (n = 0, c = scr->clients; c; c = c->next, n++)
			c->index = n;
		fprintf(file, "Adwm.screen%u.clients:\t\t%u\n", scr->screen, n);

		/* creation order list */
		/* We want to restore creation order: this is window list order for panels */
		fprintf(file, "Adwm.screen%u.clist:\t\t", scr->screen);
		for (c = scr->clist; c; c = c->cnext)
			fprintf(file, "%u%s", c->index, c->cnext ? ", " : "");
		fputc('\n', file);

		/* this one should just be 0..(n-1) */
		/* tiling order list */
		/* We want to restore tiling order: this determines layout for tiling mode views */
		fprintf(file, "Adwm.screen%u.tiles:\t\t", scr->screen);
		for (c = scr->clients; c; c = c->next)
			fprintf(file, "%u%s", c->index, c->snext ? ", " : "");
		fputc('\n', file);

		/* stacking order list */
		/* We want to restore stacking order: this determines layout for stacking mode views */
		fprintf(file, "Adwm.screen%u.stack:\t\t", scr->screen);
		for (c = scr->stack; c; c = c->snext)
			fprintf(file, "%u%s", c->index, c->snext ? ", " : "");
		fputc('\n', file);

		/* focus order list */
		/* It might not be necessary to restore focus order; but, then, it does determine
		   last focussed window for a view. */
		fprintf(file, "Adwm.screen%u.flist:\t\t", scr->screen);
		for (c = scr->flist; c; c = c->fnext)
			fprintf(file, "%u%s", c->index, c->fnext ? ", " : "");
		fputc('\n', file);

		/* active order list */
		/* It might not be necessary to restore active order; but, then, it does determine
		   last active window for a view. */
		fprintf(file, "Adwm.screen%u.alist:\t\t", scr->screen);
		for (c = scr->alist; c; c = c->anext)
			fprintf(file, "%u%s", c->index, c->anext ? ", " : "");
		fputc('\n', file);

		/* in creation order */
		for (c = scr->clist; c; c = c->cnext) {
			char *clientid, *res_name = NULL, *res_class = NULL, *wm_command = NULL;
			Bool strings;

			clientid = getclientid(c);
			if (clientid)
				fprintf(file, "Adwm.screen%u.client%u.clientid:\t\t%s\n", scr->screen, c->index, clientid);
			/* cannot identify windows without client strings */
			if ((strings = getclientstrings(c, &res_name, &res_class, &wm_command))) {
				if (res_name)
					fprintf(file, "Adwm.screen%u.client%u.res_name:\t\t%s\n", scr->screen, c->index, res_name);
				if (res_class)
					fprintf(file, "Adwm.screen%u.client%u.res_class:\t\t%s\n", scr->screen, c->index, res_class);
				if (wm_command)
					fprintf(file, "Adwm.screen%u.client%u.wm_command:\t\t%s\n", scr->screen, c->index, wm_command);
				if (c->wm_name)
					fprintf(file, "Adwm.screen%u.client%u.wm_name:\t\t%s\n", scr->screen, c->index, c->wm_name);
				if (c->wm_role)
					fprintf(file, "Adwm.screen%u.client%u.wm_role:\t\t%s\n", scr->screen, c->index, c->wm_role);

				fprintf(file, "Adwm.screen%u.client%u.monitor:\t\t%d\n", scr->screen, c->index, c->monitor);
				fprintf(file, "Adwm.screen%u.client%u.geometry.static:\t\t%s\n", scr->screen, c->index, show_geometry(&c->s));
				fprintf(file, "Adwm.screen%u.client%u.geometry.initial:\t\t%s\n", scr->screen, c->index, show_geometry(&c->u));
				fprintf(file, "Adwm.screen%u.client%u.geometry.current:\t\t%s\n", scr->screen, c->index, show_client_geometry(&c->c));
				fprintf(file, "Adwm.screen%u.client%u.geometry.restore:\t\t%s\n", scr->screen, c->index, show_client_geometry(&c->r));
				fprintf(file, "Adwm.screen%u.client%u.wm_state:\t\t%d\n", scr->screen, c->index, c->winstate);
				fprintf(file, "Adwm.screen%u.client%u.tags:\t\t0x%llx\n", scr->screen, c->index, c->tags);

				fprintf(file, "Adwm.screen%u.client%u.skip_flags:\t\t0x%x\n", scr->screen, c->index, c->skip.skip & skip_mask.skip);
				fprintf(file, "Adwm.screen%u.client%u.is_flags:\t\t0x%x\n", scr->screen, c->index, c->is.is & is_mask.is);
				fprintf(file, "Adwm.screen%u.client%u.has_flags:\t\t0x%x\n", scr->screen, c->index, c->has.has & has_mask.has);
				fprintf(file, "Adwm.screen%u.client%u.with_flags:\t\t0x%x\n", scr->screen, c->index, c->with.with & with_mask.with);
				fprintf(file, "Adwm.screen%u.client%u.can_flags:\t\t0x%x\n", scr->screen, c->index, c->can.can & can_mask.can);

				fprintf(file, "Adwm.screen%u.client%u.opacity:\t\t%u\n", scr->screen, c->index, c->opacity);

				if (clientid) {
					/*
					 * X11R6,7 Properties to identify client windows: (from ICCCM)
					 *
					 * SM_CLIENT_ID :-
					 * WM_CLIENT_LEADER (or WM_TRANSIENT_FOR) :-
					 * WM_WINDOW_ROLE (or WM_CLASS and WM_NAME) :- 
					 *
					 * Identify windows with WM_CLASS, WM_NAME and possibly
					 * WM_WINDOW_ROLE.
					 */
				} else {
					/*
					 * X11R5 Properties to identify client windows: (from ICCCM)
					 *
					 * WM_HINTS (window_group)
					 * WM_CLASS
					 * WM_NAME
					 * WM_COMMAND (non-zero length)
					 * WM_CLIENT_MACHINE
					 */
				}
			}

		}
	}

done:
	scr = save_screen;
}

