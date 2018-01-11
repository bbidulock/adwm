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
	char line[BUFSIZ] = { 0, };

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
		unsigned i;

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
			// View *v = scr->views + i;
		}

	}
	scr = save_screen;
}

