#include <unistd.h>
#include <errno.h>
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
#include <X11/Xresource.h>
#include <X11/Xft/Xft.h>
#ifdef IMLIB2
#include <Imlib2.h>
#endif
#ifdef XPM
#include <X11/xpm.h>
#endif
#include "adwm.h"
#include "layout.h"
#include "tags.h"
#include "config.h"

Options options;

typedef struct {
	char *rcfile;			/* rcfile */
	char *udir;			/* user directory */
	char *pdir;			/* private directory */
	char *sdir;			/* system directory */
} AdwmConfig;

static AdwmConfig config;

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
		/* probably unnecessary: will be done by
		   ewmh_process_net_desktop_layout() */
		if (scr->d.rows && scr->d.cols) {
			v->row = i / scr->d.cols;
			v->col = i - v->row * scr->d.cols;
		} else {
			v->row = -1;
			v->col = -1;
		}
	}
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

void
initrcfile(void)
{
	const char *home = getenv("HOME") ? : ".";
	const char *file = NULL;
	char *pos;
	int i, len;
	char *owd;
	struct stat st;

	/* init resource database */
	XrmInitialize();

	owd = calloc(PATH_MAX, sizeof(*owd));
	if (!getcwd(owd, PATH_MAX))
		strcpy(owd, "/");

	for (i = 0; i < cargc - 1; i++)
		if (!strcmp(cargv[i], "-f"))
			file = cargv[i + 1];
	free(config.rcfile);
	if (file) {
		if (*file == '/')
			config.rcfile = strdup(file);
		else {
			len = strlen(home) + strlen(file) + 2;
			config.rcfile = ecalloc(len, sizeof(*config.rcfile));
			strcpy(config.rcfile, home);
			strcat(config.rcfile, "/");
			strcat(config.rcfile, file);
		}
	} else {
		len = strlen(home) + strlen("/.adwm/adwmrc") + 1;
		config.rcfile = ecalloc(len, sizeof(*config.rcfile));
		strcpy(config.rcfile, home);
		strcat(config.rcfile, "/.adwm/adwmrc");
		if (!lstat(config.rcfile, &st) && S_ISLNK(st.st_mode)) {
			char *buf = ecalloc(PATH_MAX + 1, sizeof(*buf));

			if (readlink(config.rcfile, buf, PATH_MAX) == -1)
				eprint("%s: %s\n", config.rcfile, strerror(errno));
			if (*buf == '/') {
				free(config.rcfile);
				config.rcfile = strdup(buf);
			} else if (*buf) {
				free(config.rcfile);
				len = strlen(home) + strlen(buf) + 2;
				config.rcfile = ecalloc(len, sizeof(*config.rcfile));
				strcpy(config.rcfile, home);
				strcat(config.rcfile, "/");
				strcat(config.rcfile, buf);
			}
			free(buf);
		}

	}
	free(config.pdir);
	config.pdir = strdup(config.rcfile);
	if ((pos = strrchr(config.pdir, '/')))
		*pos = '\0';
	free(config.udir);
	config.udir = ecalloc(strlen(home) + strlen("/.adwm") + 1, sizeof(*config.udir));
	strcpy(config.udir, home);
	strcat(config.udir, "/.adwm");
	free(config.sdir);
	config.sdir = strdup("/usr/share/adwm");
	if (!strncmp(home, config.pdir, strlen(home))) {
		free(config.pdir);
		config.pdir = strdup(config.udir);
	}

	xrdb = XrmGetFileDatabase(config.rcfile);
	if (!xrdb) {
		DPRINTF("Couldn't find database file '%s'\n", config.rcfile);
		free(config.rcfile);
		len = strlen(config.pdir) + strlen("/adwmrc") + 2;
		config.rcfile = ecalloc(len, sizeof(*config.rcfile));
		strcpy(config.rcfile, config.pdir);
		strcat(config.rcfile, "/adwmrc");
		xrdb = XrmGetFileDatabase(config.rcfile);
	}
	if (!xrdb) {
		DPRINTF("Couldn't find database file '%s'\n", config.rcfile);
		free(config.rcfile);
		len = strlen(config.udir) + strlen("/adwmrc") + 2;
		config.rcfile = ecalloc(len, sizeof(*config.rcfile));
		strcpy(config.rcfile, config.udir);
		strcat(config.rcfile, "/adwmrc");
		xrdb = XrmGetFileDatabase(config.rcfile);
	}
	if (!xrdb) {
		DPRINTF("Couldn't find database file '%s'\n", config.rcfile);
		free(config.rcfile);
		len = strlen(config.sdir) + strlen("/adwmrc") + 2;
		config.rcfile = ecalloc(len, sizeof(*config.rcfile));
		strcpy(config.rcfile, config.sdir);
		strcat(config.rcfile, "/adwmrc");
		xrdb = XrmGetFileDatabase(config.rcfile);
	}
	if (!xrdb) {
		DPRINTF("Couldn't find database file '%s'\n", config.rcfile);
		fprintf(stderr, "adwm: Could not find usable database, using defaults\n");
	}
}
