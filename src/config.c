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
#include "adwm.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "resource.h"
#include "config.h" /* verification */

Options options;

typedef struct {
	char *udir;			/* user directory */
	char *pdir;			/* private directory */
	char *sdir;			/* system directory */
	char *rcfile;			/* rcfile */
	char *keysfile;			/* kerysrc file */
	char *stylefile;		/* stylerc file */
	char *themefile;		/* themerc file */
} AdwmConfig;

AdwmConfig config;

void
inittags(Bool reload)
{
	unsigned i, s = scr->screen;

	xresdb = xrdb;

	for (i = 0; i < MAXTAGS; i++) {
		const char *res;
		char name[256], clas[256], def[8];
		Tag *t = scr->tags + i;

		snprintf(def, sizeof(def), "%u", i);
		snprintf(name, sizeof(name), "adwm.screen%u.tags.name%u", s, i);
		snprintf(clas, sizeof(clas), "Adwm.Screen%u.Tags.Name%u", s, i);
		res = readres(name, clas, def);
		snprintf(t->name, sizeof(t->name), res);
	}
	scr->ntags = scr->options.ntags;

	ewmh_process_net_number_of_desktops();
	ewmh_process_net_desktop_names();

	if (reload) {
		int ntags = scr->ntags;

		for (; ntags > scr->ntags; ntags--)
			deltag();
		for (; ntags < scr->ntags; ntags++)
			addtag();

		ewmh_process_net_desktop_names();
		ewmh_update_net_number_of_desktops();
	}
}

static Bool
parsedockapp(const char *res, char **name, char **clas, char **cmd)
{
	const char *p, *q;
	size_t len;

	p = res;
	q = strchrnul(p, ' ');
	if ((len = q - p)) {
		*name = ecalloc(len + 1, sizeof(**name));
		strncpy(*name, p, len);
	} else
		*name = NULL;
	p = q + (*q ? 1 : 0);
	q = strchrnul(p, ' ');
	if ((len = q - p)) {
		*clas = ecalloc(len + 1, sizeof(**clas));
		strncpy(*clas, p, len);
	} else
		*clas = NULL;
	p = q + (*q ? 1 : 0);
	if ((len = strlen(p))) {
		*cmd = ecalloc(len + 1, sizeof(**cmd));
		strncpy(*cmd, p, len);
	} else
		*cmd = NULL;
	if (!*name && !*clas && !*cmd)
		return False;
	return True;
}

void
initdock(Bool reload)
{
	Container *t, *n;
	unsigned i, s;
	Monitor *m;

	if (reload)
		return;

	s = scr->screen;
	m = scr->dock.monitor ? : scr->monitors;

	t = scr->dock.tree = ecalloc(1, sizeof(*t));
	t->type = TreeTypeNode;
	t->view = m->num;
	t->is.dockapp = True;
	t->parent = NULL;

	switch (m->dock.position) {
	default:
	case DockNone:
	case DockEast:
		t->node.children.pos = PositionEast;
		t->node.children.ori = OrientRight;
		break;
	case DockNorthEast:
		t->node.children.pos = PositionNorthEast;
		t->node.children.ori =
		    (m->dock.orient == DockHorz) ? OrientTop : OrientRight;
		break;
	case DockNorth:
		t->node.children.pos = PositionNorth;
		t->node.children.ori = OrientTop;
		break;
	case DockNorthWest:
		t->node.children.pos = PositionNorthWest;
		t->node.children.ori =
		    (m->dock.orient == DockHorz) ? OrientTop : OrientLeft;
		break;
	case DockWest:
		t->node.children.pos = PositionWest;
		t->node.children.ori = OrientLeft;
		break;
	case DockSouthWest:
		t->node.children.pos = PositionSouthWest;
		t->node.children.ori =
		    (m->dock.orient == DockHorz) ? OrientBottom : OrientLeft;
		break;
	case DockSouth:
		t->node.children.pos = PositionSouth;
		t->node.children.ori = OrientBottom;
		break;
	case DockSouthEast:
		t->node.children.pos = PositionSouthEast;
		t->node.children.ori =
		    (m->dock.orient == DockHorz) ? OrientBottom : OrientRight;
		break;
	}

	n = adddocknode(t);

	xresdb = xrdb;

	for (i = 0; i < MAXTAGS; i++) {
		const char *res;
		char name[256], clas[256];
		Leaf *l;
		char *res_name, *res_class, *wm_command;

		snprintf(name, sizeof(name), "adwm.screen%u.dock.app%u", s, i);
		snprintf(clas, sizeof(clas), "Adwm.Screen%u.Dock.App%u", s, i);
		res = readres(name, clas, NULL);
		if (!res || !parsedockapp(res, &res_name, &res_class, &wm_command))
			continue;
		l = ecalloc(1, sizeof(*l));
		l->type = TreeTypeLeaf;
		l->view = n->view;
		l->is.dockapp = True;
		l->client.client = NULL;
		l->client.next = NULL;
		l->client.name = res_name;
		l->client.clas = res_class;
		l->client.command = wm_command;
		appleaf(n, l, False);
	}
}

void
initlayouts(Bool reload)
{
	unsigned i, s = scr->screen;;

	xresdb = xrdb;

	for (i = 0; i < MAXTAGS; i++) {
		const char *res;
		char name[256], clas[256];
		Layout *l;
		View *v = scr->views + i;

		snprintf(name, sizeof(name), "adwm.screen%u.tags.layout%u", s, i);
		snprintf(clas, sizeof(clas), "Adwm.Screen%u.Tags.Layout%u", s, i);
		res = readres(name, clas, scr->options.deflayout);
		v->layout = layouts;
		for (l = layouts; l->symbol; l++)
			if (l->symbol == *res) {
				v->layout = l;
				break;
			}
		l = v->layout;
		if (scr->options.strutsactive)
			v->barpos = StrutsOn;
		else {
			if (scr->options.hidebastards == 2)
				v->barpos = StrutsDown;
			else if (scr->options.hidebastards)
				v->barpos = StrutsHide;
			else
				v->barpos = StrutsOff;
		}
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
		if (l->arrange && l->arrange->initlayout)
			l->arrange->initlayout(v);
	}

	ewmh_update_net_desktop_modes();

	ewmh_process_net_desktop_layout();
	ewmh_update_net_desktop_layout();
	ewmh_update_net_number_of_desktops();
	ewmh_update_net_current_desktop();
	ewmh_update_net_virtual_roots();
}

void
initscreen(Bool reload)
{
	const char *res;
	char name[256], clas[256], *n, *c;
	size_t nlen, clen;
	unsigned s = scr->screen;

	xresdb = xrdb;

	snprintf(name, sizeof(name), "adwm.screen%u.", s);
	snprintf(clas, sizeof(clas), "Adwm.Screen%u.", s);
	nlen = strnlen(name, sizeof(name));
	clen = strnlen(clas, sizeof(clas));
	n = name + nlen;
	c = clas + clen;
	nlen = sizeof(name) - nlen;
	clen = sizeof(clas) - clen;

	scr->options = options;

	strncpy(n, "attachaside", nlen);
	strncpy(c, "Attachaside", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.attachaside = atoi(res) ? True : False;
	strncpy(n, "command", nlen);
	strncpy(c, "Command", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.command = res;
	strncpy(n, "command2", nlen);
	strncpy(c, "Command2", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.command2 = res;
	strncpy(n, "command3", nlen);
	strncpy(c, "Command3", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.command3 = res;
	strncpy(n, "menucommand", nlen);
	strncpy(c, "Menucommand", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.menucommand = res;
	strncpy(n, "decoratetiled", nlen);
	strncpy(c, "Decoratetiled", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.dectiled = atoi(res);
	strncpy(n, "decoratemax", nlen);
	strncpy(c, "Decoratemax", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.decmax = atoi(res) ? True : False;
	strncpy(n, "hidebastards", nlen);
	strncpy(c, "Hidebastards", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.hidebastards = atoi(res);
	strncpy(n, "strutsactive", nlen);
	strncpy(c, "StrutsActive", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.strutsactive = atoi(res) ? True : False;
	strncpy(n, "autoroll", nlen);
	strncpy(c, "Autoroll", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.autoroll = atoi(res) ? True : False;
	strncpy(n, "sloppy", nlen);
	strncpy(c, "Sloppy", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.focus = atoi(res);
	strncpy(n, "snap", nlen);
	strncpy(c, "Snap", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.snap = atoi(res);
	strncpy(n, "dock.position", nlen);
	strncpy(c, "Dock.Position", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.dockpos = atoi(res);
	strncpy(n, "dock.orient", nlen);
	strncpy(c, "Dock.Orient", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.dockori = atoi(res);
	strncpy(n, "dock.monitor", nlen);
	strncpy(c, "Dock.Monitor", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.dockmon = atoi(res);
	strncpy(n, "dragdistance", nlen);
	strncpy(c, "Dragdistance", clen);
	if ((res = readres(name, clas, NULL)))
		scr->options.dragdist = atoi(res);
	strncpy(n, "mwfact", nlen);
	strncpy(c, "Mwfact", clen);
	if ((res = readres(name, clas, NULL))) {
		scr->options.mwfact = atof(res);
		if (scr->options.mwfact < 0.10 || scr->options.mwfact > 0.90)
			scr->options.mwfact = options.mwfact;
	}
	strncpy(n, "mhfact", nlen);
	strncpy(c, "Mhfact", clen);
	if ((res = readres(name, clas, NULL))) {
		scr->options.mhfact = atof(res);
		if (scr->options.mhfact < 0.10 || scr->options.mhfact > 0.90)
			scr->options.mhfact = options.mhfact;
	}
	strncpy(n, "nmaster", nlen);
	strncpy(c, "Nmaster", clen);
	if ((res = readres(name, clas, NULL))) {
		scr->options.nmaster = atoi(res);
		if (scr->options.nmaster < 1 || scr->options.nmaster > 10)
			scr->options.nmaster = options.nmaster;
	}
	strncpy(n, "ncolumns", nlen);
	strncpy(c, "Ncolumns", clen);
	if ((res = readres(name, clas, NULL))) {
		scr->options.ncolumns = atoi(res);
		if (scr->options.ncolumns < 1 || scr->options.ncolumns > 10)
			scr->options.ncolumns = options.ncolumns;
	}
	strncpy(n, "deflayout", nlen);
	strncpy(c, "Deflayout", clen);
	if ((res = readres(name, clas, NULL))) {
		if (strnlen(res, 2) == 1)
			scr->options.deflayout = res;
	}
}

void
initconfig(Bool reload)
{
	const char *res;
	char name[256], clas[256], *n, *c;
	size_t nlen, clen;

	xresdb = xrdb;

	strcpy(name, "adwm.session.");
	strcpy(clas, "Adwm.Session.");
	nlen = strlen(name);
	clen = strlen(clas);
	n = name + nlen;
	c = clas + clen;
	nlen = sizeof(name) - nlen;
	clen = sizeof(clas) - clen;

	/* init appearance */
	strncpy(n, "useveil", nlen);
	strncpy(c, "Useveil", clen);
	options.useveil = atoi(readres(name, clas, "0")) ? True : False;
	strncpy(n, "attachaside", nlen);
	strncpy(c, "Attachaside", clen);
	options.attachaside = atoi(readres(name, clas, "1")) ? True : False;
	strncpy(n, "command", nlen);
	strncpy(c, "Command", clen);
	options.command = readres(name, clas, COMMAND);
	strncpy(n, "command2", nlen);
	strncpy(c, "Command2", clen);
	options.command2 = readres(name, clas, COMMAND2);
	strncpy(n, "command3", nlen);
	strncpy(c, "Command3", clen);
	options.command3 = readres(name, clas, COMMAND3);
	strncpy(n, "menucommand", nlen);
	strncpy(c, "Menucommand", clen);
	options.menucommand = readres(name, clas, MENUCOMMAND);
	strncpy(n, "decoratetiled", nlen);
	strncpy(c, "Decoratetiled", clen);
	options.dectiled = atoi(readres(name, clas, STR(DECORATETILED)));
	strncpy(n, "decoratemax", nlen);
	strncpy(c, "Decoratemax", clen);
	options.decmax = atoi(readres(name, clas, STR(DECORATEMAX)));
	strncpy(n, "hidebastards", nlen);
	strncpy(c, "Hidebastards", clen);
	options.hidebastards = atoi(readres(name, clas, "0"));
	strncpy(n, "strutsactive", nlen);
	strncpy(c, "StrutsActive", clen);
	options.strutsactive = atoi(readres(name, clas, "1")) ? True : False;
	strncpy(n, "autoroll", nlen);
	strncpy(c, "Autoroll", clen);
	options.autoroll = atoi(readres(name, clas, "0")) ? True : False;
	strncpy(n, "sloppy", nlen);
	strncpy(c, "Sloppy", clen);
	options.focus = atoi(readres(name, clas, "0"));
	strncpy(n, "snap", nlen);
	strncpy(c, "Snap", clen);
	options.snap = atoi(readres(name, clas, STR(SNAP)));
	strncpy(n, "dock.position", nlen);
	strncpy(c, "Dock.Position", clen);
	options.dockpos = atoi(readres(name, clas, "1"));
	strncpy(n, "dock.orient", nlen);
	strncpy(c, "Dock.Orient", clen);
	options.dockori = atoi(readres(name, clas, "1"));
	strncpy(n, "dock.monitor", nlen);
	strncpy(c, "Dock.Monitor", clen);
	options.dockmon = atoi(readres(name, clas, "0"));
	strncpy(n, "dragdistance", nlen);
	strncpy(c, "Dragdistance", clen);
	options.dragdist = atoi(readres(name, clas, "5"));

	strncpy(n, "mwfact", nlen);
	strncpy(c, "Mwfact", clen);
	options.mwfact = atof(readres(name, clas, STR(DEFMWFACT)));
	if (options.mwfact < 0.10 || options.mwfact > 0.90)
		options.mwfact = DEFMWFACT;

	strncpy(n, "mhfact", nlen);
	strncpy(c, "Mhfact", clen);
	options.mhfact = atof(readres(name, clas, STR(DEFMHFACT)));
	if (options.mhfact < 0.10 || options.mwfact > 0.90)
		options.mhfact = DEFMHFACT;

	strncpy(n, "nmaster", nlen);
	strncpy(c, "Nmaster", clen);
	options.nmaster = atoi(readres(name, clas, STR(DEFNMASTER)));
	if (options.nmaster < 1 || options.nmaster > 10)
		options.nmaster = DEFNMASTER;

	strncpy(n, "ncolumns", nlen);
	strncpy(c, "Ncolumns", clen);
	options.ncolumns = atoi(readres(name, clas, STR(DEFNCOLUMNS)));
	if (options.ncolumns < 1 || options.ncolumns > 10)
		options.ncolumns = DEFNCOLUMNS;

	strncpy(n, "deflayout", nlen);
	strncpy(c, "Deflayout", clen);
	res = readres(name, clas, "i");
	if (strlen(res) == 1)
		options.deflayout = res;
	else
		options.deflayout = "i";

	strncpy(n, "tags.number", nlen);
	strncpy(c, "Tags.number", clen);
	options.ntags = strtoul(readres(name, clas, "5"), NULL, 0);
	if (options.ntags < 1 || options.ntags > MAXTAGS)
		options.ntags = 5;
}

static void
initkeysfile(void)
{
	XrmDatabase krdb;
	const char *file;
	struct stat st;
	char *path;

	file = getresource("keysFile", "keysrc");
	path = ecalloc(PATH_MAX + 1, sizeof(*path));
	strncpy(path, file, PATH_MAX);
	if (!lstat(file, &st) && S_ISLNK(st.st_mode)) {
		if (readlink(file, path, PATH_MAX) == -1)
			eprint("%s: %s\n", file, strerror(errno));
	}
	free(config.keysfile);
	config.keysfile = strdup(path);
	if (*config.keysfile != '/') {
		strncpy(path, config.rcfile, PATH_MAX);
		if (strrchr(path, '/'))
			*strrchr(path, '/') = '\0';
		strncat(path, "/", PATH_MAX);
		strncat(path, config.keysfile, PATH_MAX);
		free(config.keysfile);
		config.keysfile = strdup(path);
	}
	free(path);
	krdb = XrmGetFileDatabase(config.keysfile);
	if (!krdb) {
		DPRINTF("Could not find database file '%s'\n", config.keysfile);
		return;
	}
	XrmMergeDatabases(krdb, &xrdb);
}

static void
initstylefile(void)
{
	XrmDatabase srdb;
	const char *file;
	struct stat st;
	char *path;

	file = getresource("styleFile", "stylerc");
	path = ecalloc(PATH_MAX + 1, sizeof(*path));
	strncpy(path, file, PATH_MAX);
	if (!lstat(file, &st) && S_ISLNK(st.st_mode))
		if (readlink(file, path, PATH_MAX) == -1)
			eprint("%s: %s\n", file, strerror(errno));
	free(config.stylefile);
	config.stylefile = strdup(path);
	if (*config.stylefile != '/') {
		strncpy(path, config.rcfile, PATH_MAX);
		if (strrchr(path, '/'))
			*strrchr(path, '/') = '\0';
		strncat(path, "/", PATH_MAX);
		strncat(path, config.stylefile, PATH_MAX);
		free(config.stylefile);
		config.stylefile = strdup(path);
	}
	free(path);
	srdb = XrmGetFileDatabase(config.stylefile);
	if (!srdb) {
		DPRINTF("Could not find database file '%s'\n", config.stylefile);
		return;
	}
	XrmMergeDatabases(srdb, &xrdb);
}

static void
initthemefile(void)
{
	XrmDatabase trdb;
	const char *file;
	struct stat st;
	char *path;

	file = getresource("themeFile", "themerc");
	path = ecalloc(PATH_MAX + 1, sizeof(*path));
	strncpy(path, file, PATH_MAX);
	if (!lstat(file, &st) && S_ISLNK(st.st_mode))
		if (readlink(file, path, PATH_MAX) == -1)
			eprint("%s: %s\n", file, strerror(errno));
	free(config.themefile);
	config.themefile = strdup(path);
	if (*config.themefile != '/') {
		strncpy(path, config.rcfile, PATH_MAX);
		if (strrchr(path, '/'))
			*strrchr(path, '/') = '\0';
		strncat(path, "/", PATH_MAX);
		strncat(path, config.themefile, PATH_MAX);
		free(config.themefile);
		config.themefile = strdup(path);
	}
	free(path);
	trdb = XrmGetFileDatabase(config.themefile);
	if (!trdb) {
		DPRINTF("Could not find database file '%s'\n", config.themefile);
		return;
	}
	XrmMergeDatabases(trdb, &xrdb);
}

char *
findrcpath(const char *file)
{
	char *path, *result;

	path = ecalloc(PATH_MAX + 1, sizeof(*path));
	strncpy(path, file, PATH_MAX);
	if (*path != '/') {
		if (config.stylefile) {
			strncpy(path, config.stylefile, PATH_MAX);
			if (strrchr(path, '/'))
				*strrchr(path, '/') = '\0';
			strncat(path, "/", PATH_MAX);
			strncat(path, file, PATH_MAX);
			if (!access(path, R_OK)) {
				result = strdup(path);
				free(path);
				return (result);
			}
		}
		if (config.rcfile) {
			strncpy(path, config.rcfile, PATH_MAX);
			if (strrchr(path, '/'))
				*strrchr(path, '/') = '\0';
			strncat(path, "/", PATH_MAX);
			strncat(path, file, PATH_MAX);
			if (!access(path, R_OK)) {
				result = strdup(path);
				free(path);
				return (result);
			}
		}
		if (config.pdir) {
			strncpy(path, config.pdir, PATH_MAX);
			strncat(path, "/", PATH_MAX);
			strncat(path, file, PATH_MAX);
			if (!access(path, R_OK)) {
				result = strdup(path);
				free(path);
				return (result);
			}
		}
		if (config.udir) {
			strncpy(path, config.udir, PATH_MAX);
			strncat(path, "/", PATH_MAX);
			strncat(path, file, PATH_MAX);
			if (!access(path, R_OK)) {
				result = strdup(path);
				free(path);
				return (result);
			}
		}
		if (config.sdir) {
			strncpy(path, config.sdir, PATH_MAX);
			strncat(path, "/", PATH_MAX);
			strncat(path, file, PATH_MAX);
			if (!access(path, R_OK)) {
				result = strdup(path);
				free(path);
				return (result);
			}
		}
	}
	if (!access(path, R_OK)) {
		result = strdup(path);
		free(path);
		return (result);
	}
	free(path);
	return (NULL);
}

void
initrcfile(const char *conf, Bool reload)
{
	const char *home = getenv("HOME") ? : ".";
	const char *file = NULL;
	char *pos;
	int i, len;
	struct stat st;
	static int initialized = 0;

	/* init resource database */
	if (!initialized) {
		XrmInitialize();
		initialized = 1;
	}
	if (xrdb) {
		XrmDestroyDatabase(xrdb);
		xresdb = xrdb = NULL;
	}
	for (i = 0; i < cargc - 1; i++)
		if (!strcmp(cargv[i], "-f"))
			file = cargv[i + 1];
	free(config.rcfile);
	if (file) {
		if (*file == '/')
			config.rcfile = strdup(file);
		else {
			len = strlen(home) + strlen(file) + 2;
			config.rcfile = ecalloc(len + 1, sizeof(*config.rcfile));
			strncpy(config.rcfile, home, len);
			strncat(config.rcfile, "/", len);
			strncat(config.rcfile, file, len);
		}
	} else {
		len = strlen(home) + strlen("/.adwm/adwmrc") + 1;
		config.rcfile = ecalloc(len + 1, sizeof(*config.rcfile));
		strncpy(config.rcfile, home, len);
		strncat(config.rcfile, "/.adwm/adwmrc", len);
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
				config.rcfile = ecalloc(len + 1, sizeof(*config.rcfile));
				strncpy(config.rcfile, home, len);
				strncat(config.rcfile, "/", len);
				strncat(config.rcfile, buf, len);
			}
			free(buf);
		}

	}
	free(config.pdir);
	config.pdir = strdup(config.rcfile);
	if ((pos = strrchr(config.pdir, '/')))
		*pos = '\0';
	free(config.udir);
	len = strlen(home) + strlen("/.adwm") + 1;
	config.udir = ecalloc(len + 1, sizeof(*config.udir));
	strncpy(config.udir, home, len);
	strncat(config.udir, "/.adwm", len);
	free(config.sdir);
	config.sdir = strdup(SYSCONFPATH);
	if (!strncmp(home, config.pdir, strlen(home))) {
		free(config.pdir);
		config.pdir = strdup(config.udir);
	}

	xrdb = XrmGetFileDatabase(config.rcfile);
	if (!xrdb) {
		DPRINTF("Couldn't find database file '%s'\n", config.rcfile);
		free(config.rcfile);
		len = strlen(config.pdir) + strlen("/adwmrc") + 2;
		config.rcfile = ecalloc(len + 1, sizeof(*config.rcfile));
		strncpy(config.rcfile, config.pdir, len);
		strncat(config.rcfile, "/adwmrc", len);
		xrdb = XrmGetFileDatabase(config.rcfile);
	}
	if (!xrdb) {
		DPRINTF("Couldn't find database file '%s'\n", config.rcfile);
		free(config.rcfile);
		len = strlen(config.udir) + strlen("/adwmrc") + 2;
		config.rcfile = ecalloc(len + 1, sizeof(*config.rcfile));
		strncpy(config.rcfile, config.udir, len);
		strncat(config.rcfile, "/adwmrc", len);
		xrdb = XrmGetFileDatabase(config.rcfile);
	}
	if (!xrdb) {
		DPRINTF("Couldn't find database file '%s'\n", config.rcfile);
		free(config.rcfile);
		len = strlen(config.sdir) + strlen("/adwmrc") + 2;
		config.rcfile = ecalloc(len + 1, sizeof(*config.rcfile));
		strncpy(config.rcfile, config.sdir, len);
		strncat(config.rcfile, "/adwmrc", len);
		xrdb = XrmGetFileDatabase(config.rcfile);
	}
	if (xrdb) {
		char *dir;

		dir = strdup(config.rcfile);
		if (strrchr(dir, '/'))
			*strrchr(dir, '/') = '\0';
		if (chdir(dir))
			DPRINTF("Could not change directory to %s: %s\n", dir,
				strerror(errno));
		free(dir);
	} else {
		DPRINTF("Couldn't find database file '%s'\n", config.rcfile);
		_DPRINTF("Could not find usable database, using defaults\n");
		if (chdir(config.udir))
			DPRINTF("Could not change directory to %s: %s\n", config.udir,
				strerror(errno));
	}
	initkeysfile();		/* read key bindings into the database */
	initstylefile();	/* read style elements into the database */
	initthemefile();	/* read theme elements into the database */
}
