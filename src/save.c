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

XrmDatabase
save(Bool permanent)
{
	XrmDatabase srdb = NULL;
	AScreen *save_screen = scr;
	char line[BUFSIZ] = { 0, };
	const char *str;
	unsigned i;

#if 0
	if (srdb) {
		XrmDestroyDatabase(srdb);
		srdb = NULL;
	}
#endif
	if (!(srdb = XrmGetStringDatabase(line))) {
		EPRINTF("could not create Xrm database\n");
		return (srdb);
	}

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

		if ((str = config.themefile) && strcmp(str, "themerc")) {
			snprintf(line, sizeof(line), "Adwm*themeFile:\t\t%s\n", str);
			XrmPutLineResource(&srdb, line);
		}
		if ((str = config.themename) && strcmp(str, "Default")) {
			snprintf(line, sizeof(line), "Adwm*themeName:\t\t%s\n", str);
			XrmPutLineResource(&srdb, line);
		}
		if ((str = config.stylefile) && strcmp(str, "stylerc")) {
			snprintf(line, sizeof(line), "Adwm*styleFile:\t\t%s\n", str);
			XrmPutLineResource(&srdb, line);
		}
		if ((str = config.stylename) && strcmp(str, "Default")) {
			snprintf(line, sizeof(line), "Adwm*styleName:\t\t%s\n", str);
			XrmPutLineResource(&srdb, line);
		}
		if ((str = config.keysfile) && strcmp(str, "keysrc")) {
			snprintf(line, sizeof(line), "Adwm*keysFile:\t\t%s\n", str);
			XrmPutLineResource(&srdb, line);
		}

		/* should probably save the dock definitions in the main file */

		if ((str = config.dockfile) && strcmp(str, "dockrc")) {
			snprintf(line, sizeof(line), "Adwm*dockFile:\t\t%s\n", str);
			XrmPutLineResource(&srdb, line);
		}
	} else {

		/* The intention of the non-permanent form is to simply save the main
		   adwmrc file and just link it relatively to the ancilliary files that
		   it references. This is to simply save the configuration so that each
		   time adwm boots it uses the saved configuration.  This is not used by
		   X11R6 session management. */

		if ((str = getresource("themeFile", NULL)) && strcmp(str, "themerc")) {
			snprintf(line, sizeof(line), "Adwm*themeFile:\t\t%s\n", str);
			XrmPutLineResource(&srdb, line);
		}
		if ((str = getresource("themeName", NULL)) && strcmp(str, "Default")) {
			snprintf(line, sizeof(line), "Adwm*themeName:\t\t%s\n", str);
			XrmPutLineResource(&srdb, line);
		}
		if ((str = getresource("styleFile", NULL)) && strcmp(str, "stylerc")) {
			snprintf(line, sizeof(line), "Adwm*styleFile:\t\t%s\n", str);
			XrmPutLineResource(&srdb, line);
		}
		if ((str = getresource("styleName", NULL)) && strcmp(str, "Default")) {
			snprintf(line, sizeof(line), "Adwm*styleName:\t\t%s\n", str);
			XrmPutLineResource(&srdb, line);
		}
		if ((str = getresource("keysFile", NULL)) && strcmp(str, "keysrc")) {
			snprintf(line, sizeof(line), "Adwm*keysFile:\t\t%s\n", str);
			XrmPutLineResource(&srdb, line);
		}
		if ((str = getresource("dockFile", NULL)) && strcmp(str, "dockrc")) {
			snprintf(line, sizeof(line), "Adwm*dockFile:\t\t%s\n", str);
			XrmPutLineResource(&srdb, line);
		}
	}

	/* GENERAL DIRECTIVES */
	if (options.debug) {
		snprintf(line, sizeof(line), "Adwm*debug:\t\t%d\n", options.debug);
		XrmPutLineResource(&srdb, line);
	}
	if (options.useveil) {
		snprintf(line, sizeof(line), "Adwm*useveil:\t\t%d\n", options.useveil);
		XrmPutLineResource(&srdb, line);
	}
	if (!options.attachaside) {
		snprintf(line, sizeof(line), "Adwm*attachaside:\t\t%d\n", options.attachaside);
		XrmPutLineResource(&srdb, line);
	}
	if (options.command && COMMAND && strcmp(options.command, COMMAND)) {
		snprintf(line, sizeof(line), "Adwm*command:\t\t%s\n", options.command);
		XrmPutLineResource(&srdb, line);
	}
	if (options.command2) {
		snprintf(line, sizeof(line), "Adwm*command2:\t\t%s\n", options.command2);
		XrmPutLineResource(&srdb, line);
	}
	if (options.command3) {
		snprintf(line, sizeof(line), "Adwm*command3:\t\t%s\n", options.command3);
		XrmPutLineResource(&srdb, line);
	}
	if (options.menucommand) {
		snprintf(line, sizeof(line), "Adwm*menucommand:\t\t%s\n", options.menucommand);
		XrmPutLineResource(&srdb, line);
	}
	if (options.dectiled != DECORATETILED) {
		snprintf(line, sizeof(line), "Adwm*decoratetiled:\t\t%d\n", options.dectiled);
		XrmPutLineResource(&srdb, line);
	}
	if (options.decmax != DECORATEMAX) {
		snprintf(line, sizeof(line), "Adwm*decoratemax:\t\t%d\n", options.decmax);
		XrmPutLineResource(&srdb, line);
	}
	if (options.hidebastards) {
		snprintf(line, sizeof(line), "Adwm*hidebastards:\t\t%d\n", options.hidebastards);
		XrmPutLineResource(&srdb, line);
	}
	if (options.strutsactive) {
		snprintf(line, sizeof(line), "Adwm*strutsactive:\t\t%d\n", options.strutsactive);
		XrmPutLineResource(&srdb, line);
	}
	if (options.autoroll) {
		snprintf(line, sizeof(line), "Adwm*autoroll:\t\t%d\n", options.autoroll);
		XrmPutLineResource(&srdb, line);
	}
	if (options.showdesk) {
		snprintf(line, sizeof(line), "Adwm*showdesk:\t\t%d\n", options.showdesk);
		XrmPutLineResource(&srdb, line);
	}
	if (options.strutsdelay != 500) {
		snprintf(line, sizeof(line), "Adwm*strutsdelay:\t\t%lu\n", options.strutsdelay);
		XrmPutLineResource(&srdb, line);
	}
	if (options.focus) {
		snprintf(line, sizeof(line), "Adwm*sloppy:\t\t%d\n", options.focus);
		XrmPutLineResource(&srdb, line);
	}
	if (options.snap != SNAP) {
		snprintf(line, sizeof(line), "Adwm*snap:\t\t%d\n", options.snap);
		XrmPutLineResource(&srdb, line);
	}
	if (options.dockpos != 1) {
		snprintf(line, sizeof(line), "Adwm*dock.position:\t\t%d\n", options.dockpos);
		XrmPutLineResource(&srdb, line);
	}
	if (options.dockori != 1) {
		snprintf(line, sizeof(line), "Adwm*dock.orient:\t\t%d\n", options.dockori);
		XrmPutLineResource(&srdb, line);
	}
	if (options.dockmon != 0) {
		snprintf(line, sizeof(line), "Adwm*dock.monitor:\t\t%d\n", options.dockmon);
		XrmPutLineResource(&srdb, line);
	}
	if (options.dragdist != 5) {
		snprintf(line, sizeof(line), "Adwm*dragdistance:\t\t%d\n", options.dragdist);
		XrmPutLineResource(&srdb, line);
	}
	if (options.mwfact != DEFMWFACT) {
		snprintf(line, sizeof(line), "Adwm*mwfact:\t\t%f\n", options.mwfact);
		XrmPutLineResource(&srdb, line);
	}
	if (options.mhfact != DEFMHFACT) {
		snprintf(line, sizeof(line), "Adwm*mhfact:\t\t%f\n", options.mhfact);
		XrmPutLineResource(&srdb, line);
	}
	if (options.nmaster != DEFNMASTER) {
		snprintf(line, sizeof(line), "Adwm*nmaster:\t\t%d\n", options.nmaster);
		XrmPutLineResource(&srdb, line);
	}
	if (options.ncolumns != DEFNCOLUMNS) {
		snprintf(line, sizeof(line), "Adwm*ncolumns:\t\t%d\n", options.ncolumns);
		XrmPutLineResource(&srdb, line);
	}
	if (options.deflayout != 'i') {
		snprintf(line, sizeof(line), "Adwm*deflayout:\t\t%c\n", options.deflayout);
		XrmPutLineResource(&srdb, line);
	}
	if (options.placement != ColSmartPlacement) {
		snprintf(line, sizeof(line), "Adwm*placement:\t\t%c\n", options.placement);
		XrmPutLineResource(&srdb, line);
	}
	if (options.ntags != 5) {
		snprintf(line, sizeof(line), "Adwm*tags.number:\t\t%d\n", options.ntags);
		XrmPutLineResource(&srdb, line);
	}
	if (options.prependdirs) {
		snprintf(line, sizeof(line), "Adwm*icon.prependdirs:\t\t%s\n", options.prependdirs);
		XrmPutLineResource(&srdb, line);
	}
	if (options.appenddirs) {
		snprintf(line, sizeof(line), "Adwm*icon.appenddirs:\t\t%s\n", options.appenddirs);
		XrmPutLineResource(&srdb, line);
	}
	if (options.icontheme) {
		snprintf(line, sizeof(line), "Adwm*icon.theme:\t\t%s\n", options.icontheme);
		XrmPutLineResource(&srdb, line);
	}
	if (options.extensions) {
		snprintf(line, sizeof(line), "Adwm*icon.extensions:\t\t%s\n", options.extensions);
		XrmPutLineResource(&srdb, line);
	}

	/* SCREEN DIRECTIVES */
	for (scr = screens; scr < screens + nscr; scr++) {

		if (!scr->managed)
			continue;
		if (scr->options.attachaside != options.attachaside) {
			snprintf(line, sizeof(line), "Adwm.screen%u.attachaside:\t\t%d\n", scr->screen, scr->options.attachaside);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.command && options.command && strcmp(scr->options.command, options.command)) {
			snprintf(line, sizeof(line), "Adwm.screen%u.command:\t\t%d\n", scr->screen, scr->options.attachaside);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.command2 && options.command2 && strcmp(scr->options.command2, options.command2)) {
			snprintf(line, sizeof(line), "Adwm.screen%u.command2:\t\t%d\n", scr->screen, scr->options.attachaside);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.command3 && options.command3 && strcmp(scr->options.command3, options.command3)) {
			snprintf(line, sizeof(line), "Adwm.screen%u.command3:\t\t%d\n", scr->screen, scr->options.attachaside);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.menucommand && options.menucommand && strcmp(scr->options.menucommand, options.menucommand)) {
			snprintf(line, sizeof(line), "Adwm.screen%u.menucommand:\t\t%d\n", scr->screen, scr->options.attachaside);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.dectiled != options.dectiled) {
			snprintf(line, sizeof(line), "Adwm.screen%u.decoratetiled:\t\t%d\n", scr->screen, scr->options.dectiled);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.decmax != options.decmax) {
			snprintf(line, sizeof(line), "Adwm.screen%u.decoratemax:\t\t%d\n", scr->screen, scr->options.decmax);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.hidebastards != options.hidebastards) {
			snprintf(line, sizeof(line), "Adwm.screen%u.hidebastards:\t\t%d\n", scr->screen, scr->options.hidebastards);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.strutsactive != options.strutsactive) {
			snprintf(line, sizeof(line), "Adwm.screen%u.strutsactive:\t\t%d\n", scr->screen, scr->options.strutsactive);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.autoroll != options.autoroll) {
			snprintf(line, sizeof(line), "Adwm.screen%u.autoroll:\t\t%d\n", scr->screen, scr->options.autoroll);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.showdesk != options.showdesk) {
			snprintf(line, sizeof(line), "Adwm.screen%u.showdesk:\t\t%d\n", scr->screen, scr->options.showdesk);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.strutsdelay != options.strutsdelay) {
			snprintf(line, sizeof(line), "Adwm.screen%u.strutsdelay:\t\t%lu\n", scr->screen, scr->options.strutsdelay);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.focus != options.focus) {
			snprintf(line, sizeof(line), "Adwm.screen%u.sloppy:\t\t%d\n", scr->screen, scr->options.focus);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.snap != options.snap) {
			snprintf(line, sizeof(line), "Adwm.screen%u.snap:\t\t%d\n", scr->screen, scr->options.snap);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.dockpos != options.dockpos) {
			snprintf(line, sizeof(line), "Adwm.screen%u.dock.position:\t\t%d\n", scr->screen, scr->options.dockpos);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.dockori != options.dockori) {
			snprintf(line, sizeof(line), "Adwm.screen%u.dock.orient:\t\t%d\n", scr->screen, scr->options.dockori);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.dockmon != options.dockmon) {
			snprintf(line, sizeof(line), "Adwm.screen%u.dock.monitor:\t\t%d\n", scr->screen, scr->options.dockmon);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.dragdist != options.dragdist) {
			snprintf(line, sizeof(line), "Adwm.screen%u.dragdistance:\t\t%d\n", scr->screen, scr->options.dragdist);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.mwfact != options.mwfact) {
			snprintf(line, sizeof(line), "Adwm.screen%u.mwfact:\t\t%f\n", scr->screen, scr->options.mwfact);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.mhfact != options.mhfact) {
			snprintf(line, sizeof(line), "Adwm.screen%u.mhfact:\t\t%f\n", scr->screen, scr->options.mhfact);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.nmaster != options.nmaster) {
			snprintf(line, sizeof(line), "Adwm.screen%u.nmaster:\t\t%d\n", scr->screen, scr->options.nmaster);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.ncolumns != options.ncolumns) {
			snprintf(line, sizeof(line), "Adwm.screen%u.ncolumns:\t\t%d\n", scr->screen, scr->options.ncolumns);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.deflayout != options.deflayout) {
			snprintf(line, sizeof(line), "Adwm.screen%u.deflayout:\t\t%c\n", scr->screen, scr->options.deflayout);
			XrmPutLineResource(&srdb, line);
		}
		if (scr->options.placement != options.placement) {
			snprintf(line, sizeof(line), "Adwm.screen%u.placement:\t\t%c\n", scr->screen, scr->options.placement);
			XrmPutLineResource(&srdb, line);
		}

		/* TAG DIRECTIVES */
		if (options.ntags) {
			snprintf(line, sizeof(line), "Adwm*tags.number:\t\t%d\n", options.ntags);
			XrmPutLineResource(&srdb, line);
		}
		for (i = 0; i < MAXTAGS; i++) {
			Tag *t = scr->tags + i;

			if (t->name) {
				snprintf(line, sizeof(line), "Adwm*tags.name%u:\t\t%s\n", i, t->name);
				XrmPutLineResource(&srdb, line);
			}
		}

		/* VIEW DIRECTIVES */
		for (i = 0; i < MAXTAGS; i++) {
			View *v = scr->views + i;
			Layout *l = v->layout;

			/* this one was poorly named */
			if (l->symbol != scr->options.deflayout) {
				snprintf(line, sizeof(line), "Adwm.screen%u.tags.layout%u:\t\t%c\n", scr->screen, i, l->symbol);
				XrmPutLineResource(&srdb, line);
			}
			if (v->major != l->major) {
				snprintf(line, sizeof(line), "Adwm.screen%u.view%u.major:\t\t%d\n", scr->screen, i, v->major);
				XrmPutLineResource(&srdb, line);
			}
			if (v->minor != l->minor) {
				snprintf(line, sizeof(line), "Adwm.screen%u.view%u.minor:\t\t%d\n", scr->screen, i, v->minor);
				XrmPutLineResource(&srdb, line);
			}
			if (v->placement != scr->options.placement) {
				snprintf(line, sizeof(line), "Adwm.screen%u.view%u.placement:\t\t%d\n", scr->screen, i, v->placement);
				XrmPutLineResource(&srdb, line);
			}
			if ((v->barpos == StrutsOn) != !!scr->options.strutsactive) {
				snprintf(line, sizeof(line), "Adwm.screen%u.view%u.strutsactive:\t\t%d\n", scr->screen, i, (v->barpos == StrutsOn) ?  1 : 0);
				XrmPutLineResource(&srdb, line);
			}
			if (v->dectiled != scr->options.dectiled) {
				snprintf(line, sizeof(line), "Adwm.screen%u.view%u.decoratetiled:\t\t%d\n", scr->screen, i, v->dectiled);
				XrmPutLineResource(&srdb, line);
			}
			if (v->nmaster != scr->options.nmaster) {
				snprintf(line, sizeof(line), "Adwm.screen%u.view%u.nmaster:\t\t%d\n", scr->screen, i, v->nmaster);
				XrmPutLineResource(&srdb, line);
			}
			if (v->ncolumns != scr->options.ncolumns) {
				snprintf(line, sizeof(line), "Adwm.screen%u.view%u.ncolumns:\t\t%d\n", scr->screen, i, v->ncolumns);
				XrmPutLineResource(&srdb, line);
			}
			if (v->mwfact != scr->options.mwfact) {
				snprintf(line, sizeof(line), "Adwm.screen%u.view%u.mwfact:\t\t%f\n", scr->screen, i, v->mwfact);
				XrmPutLineResource(&srdb, line);
			}
			if (v->mhfact != scr->options.mhfact) {
				snprintf(line, sizeof(line), "Adwm.screen%u.view%u.mhfact:\t\t%f\n", scr->screen, i, v->mhfact);
				XrmPutLineResource(&srdb, line);
			}
			if (v->seltags != (1ULL << i)) {
				snprintf(line, sizeof(line), "Adwm.screen%u.view%u.seltags:\t\t0x%llx\n", scr->screen, i, v->seltags);
				XrmPutLineResource(&srdb, line);
			}
		}

	}
	/* RULE DIRECTIVES */
	for (i = 0; i < 64; i++) {
		Rule *r;

		if ((r = rules[i])) {
			snprintf(line, sizeof(line), "Adwm.rule%u:\t\t%s %s %d %d\n", i, r->prop, r->tags, r->isfloating, r->hastitle);
			XrmPutLineResource(&srdb, line);
		}
	}

	scr = save_screen;
	return (srdb);
}

int
save_state(FILE *f)
{
	AScreen *save_screen = scr;
	Client *c;
	int index;

	/* mark an index on each client */
	for (scr = screens; scr < screens + nscr; scr++)
		for (index = 0, c = scr->clients; c; c = c->next, index++)
			c->breadcrumb = index;

	scr = save_screen;
	return (0);
}

