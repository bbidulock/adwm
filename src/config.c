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
	union {
		struct {
			char *pdir;	/* private directory */
			char *rdir;	/* runtime directory ${XDG_RUNTIME_DIR}/adwm */
			char *xdir;	/* XDG directory ${XDG_CONFIG_DIR}/adwm */
			char *udir;	/* user directory ${HOME}/.adwm */
			char *sdir;	/* system directory /usr/share/adwm */
		};
		char *dirs[5];
	};
	union {
		struct {
			char *dockfile;	/* dockrc file */
			char *keysfile;	/* kerysrc file */
			char *themefile;	/* themerc file */
			char *stylefile;	/* stylerc file */
			char *rcfile;	/* rcfile */
		};
		char *files[5];
	};
} AdwmConfig;

AdwmConfig config = { NULL, };

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

	/* init debug first */
	strncpy(n, "debug", nlen);
	strncpy(c, "Debug", clen);
	options.debug = atoi(readres(name, clas, "0"));
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

char *
findrcfile(const char *file)
{
	char *path, *buf, *result = NULL;
	struct stat st;
	int i;

	if (*file == '/') {
		result = strdup(file);
		return (result);
	}
	path = ecalloc(PATH_MAX + 1, sizeof(*path));
	for (i = 0; i < 5; i++) {
		strncpy(path, config.dirs[i], PATH_MAX);
		strncat(path, file, PATH_MAX);
		if (!access(path, R_OK)) {
			if (!lstat(path, &st)) {
				if (S_ISLNK(st.st_mode)) {
					buf = ecalloc(PATH_MAX + 1, sizeof(*buf));
					if (readlink(path, buf, PATH_MAX) != -1) {
						if (*buf == '/')
							strncpy(path, buf, PATH_MAX);
						else {
							if (strrchr(path, '/'))
								*(strrchr(path, '/') + 1) = '\0';
							strncat(path, buf, PATH_MAX);
						}
					} else {
						free(buf);
						DPRINTF("Cannot readlink %s: %s\n", path, strerror(errno));
						continue;
					}
					free(buf);
				}
				break;
			} else {
				DPRINTF("Cannot stat %s: %s\n", path, strerror(errno));
				continue;
			}
		} else {
			DPRINTF("Cannot access %s: %s\n", path, strerror(errno));
			continue;
		}
	}
	if (i < 5)
		result = strdup(path);
	free(path);
	return (result);
}

char *
finddirpath(const char *rc, const char *file)
{
	char *path = NULL;
	int len;

	if (!file)
		return (path);
	if (*file == '/') {
		path = strdup(file);
		return (path);
	}
	if (!rc)
		return (path);
	len = strlen(rc) + strlen(file);
	path = ecalloc(len + 1, sizeof(*path));
	strncpy(path, rc, len);
	if (strrchr(path, '/'))
		*(strrchr(path, '/') + 1) = '\0';
	strncat(path, file, len);
	return (path);
}

char *
findthemepath(const char *file)
{
	return finddirpath(config.themefile, file);
}

char *
findstylepath(const char *file)
{
	return finddirpath(config.stylefile, file);
}

char *
findconfigpath(const char *file)
{
	return finddirpath(config.rcfile, file);
}

char *
findrcpath(const char *file)
{
	char *path = NULL;
	int i;

	if (!file)
		return (path);
	if (*file == '/') {
		path = strdup(file);
		return (path);
	}
	/* check relative to themefile, stylefile, rcfile directory */
	for (i = 2; i < 5; i++) {
		if (!config.files[i])
			continue;
		if ((path = finddirpath(config.files[i], file))) {
			if (!access(path, R_OK))
				return (path);
			free(path);
			path = NULL;
		}
	}
	/* check relative to pdir, rdir, xdir, udir, sdir */
	for (i = 0; i < 5; i++) {
		if (!config.dirs[i])
			continue;
		if ((path = finddirpath(config.dirs[i], file))) {
			if (!access(path, R_OK))
				return (path);
			free(path);
			path = NULL;
		}
	}
	return (path);
}

static void
initdockfile(void)
{
	XrmDatabase drdb;
	const char *file;

	free(config.dockfile);
	config.dockfile = NULL;

	if (!config.dockfile && (file = getresource("dockFile", "dockrc"))) {
		if (!(config.dockfile = findrcfile(file)))
			DPRINTF("Could not find dock file for %s\n", file);
		if (config.dockfile && access(config.dockfile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.dockfile, strerror(errno));
			free(config.dockfile);
			config.dockfile = NULL;
		}
		if (!config.dockfile && strcmp(file, "dockrc")) {
			if (!(config.dockfile = findrcfile("dockrc")))
			DPRINTF("Could not find dock file for %s\n", "dockrc");
		}
		if (config.dockfile && access(config.dockfile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.dockfile, strerror(errno));
			free(config.dockfile);
			config.dockfile = NULL;
		}
	}
	if (!config.dockfile) {
		if (!(config.dockfile = findthemepath("dockrc")))
			DPRINTF("Could not find dock file for %s\n", file);
		if (config.dockfile && access(config.dockfile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.dockfile, strerror(errno));
			free(config.dockfile);
			config.dockfile = NULL;
		}
	}
	if (!config.dockfile) {
		if (!(config.dockfile = findstylepath("dockrc")))
			DPRINTF("Could not find dock file for %s\n", "dockrc");
		if (config.dockfile && access(config.dockfile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.dockfile, strerror(errno));
			free(config.dockfile);
			config.dockfile = NULL;
		}
	}
	if (!config.dockfile) {
		DPRINTF("Could not find readable dock file.\n");
		return;
	}
	DPRINTF("Reading databse file %s\n", config.dockfile);
	drdb = XrmGetFileDatabase(config.dockfile);
	if (!drdb) {
		DPRINTF("Could not read database file '%s'\n", config.dockfile);
		return;
	}
	XrmMergeDatabases(drdb, &xrdb);
}

static void
initkeysfile(void)
{
	XrmDatabase krdb;
	const char *file;

	free(config.keysfile);
	config.keysfile = NULL;

	if (!config.keysfile && (file = getresource("keysFile", "keysrc"))) {
		if (!(config.keysfile = findrcfile(file)))
			DPRINTF("Could not find keys file for %s\n", file);
		if (config.keysfile && access(config.keysfile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.keysfile, strerror(errno));
			free(config.keysfile);
			config.keysfile = NULL;
		}
		if (!config.keysfile && strcmp(file, "keysrc")) {
			if (!(config.keysfile = findrcfile("keysrc")))
				DPRINTF("Could not find keys file for %s\n", "keysrc");
		}
		if (config.keysfile && access(config.keysfile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.keysfile, strerror(errno));
			free(config.keysfile);
			config.keysfile = NULL;
		}
	}
	if (!config.keysfile) {
		if (!(config.keysfile = findthemepath("keysrc")))
			DPRINTF("Could not find keys file for %s\n", "keysrc");
		if (config.keysfile && access(config.keysfile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.keysfile, strerror(errno));
			free(config.keysfile);
			config.keysfile = NULL;
		}
	}
	if (!config.keysfile) {
		if (!(config.keysfile = findstylepath("keysrc")))
			DPRINTF("Could not find keys file for %s\n", "keysrc");
		if (config.keysfile && access(config.keysfile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.keysfile, strerror(errno));
			free(config.keysfile);
			config.keysfile = NULL;
		}
	}
	if (!config.keysfile) {
		DPRINTF("Could not find readable keys file.\n");
		return;
	}
	DPRINTF("Reading databse file %s\n", config.keysfile);
	krdb = XrmGetFileDatabase(config.keysfile);
	if (!krdb) {
		DPRINTF("Could not read database file '%s'\n", config.keysfile);
		return;
	}
	XrmMergeDatabases(krdb, &xrdb);
}

static void
initstylefile(void)
{
	XrmDatabase srdb;
	const char *file, *name;
	char *path;
	int len;

	free(config.stylefile);
	config.stylefile = NULL;

	if ((name = getresource("style.name", NULL))) {
		/* already loaded a style, must be included in themerc */
		if (config.themefile)
			config.stylefile = strdup(config.themefile);
		else if (config.rcfile)
			config.stylefile = strdup(config.rcfile);
		return;
	}
	/* haven't loaded a style file yet */
	if (!config.stylefile && (file = getresource("styleFile", "stylerc"))) {
		if (!(config.stylefile = findrcfile(file)))
			DPRINTF("Could not find style file for %s\n", file);
		if (config.stylefile && access(config.stylefile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.stylefile, strerror(errno));
			free(config.stylefile);
			config.stylefile = NULL;
		}
		if (!config.stylefile && strcmp(file, "stylerc")) {
			if (!(config.stylefile = findrcfile("stylerc")))
				DPRINTF("Could not find style file for %s\n", "stylerc");
		}
		if (config.stylefile && access(config.stylefile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.stylefile, strerror(errno));
			free(config.stylefile);
			config.stylefile = NULL;
		}
	}
	if (!config.stylefile && (name = getresource("styleName", "Default"))) {
		len = strlen("styles/") + strlen(name) + strlen("/stylerc");
		path = ecalloc(len + 1, sizeof(*path));
		strncpy(path, "styles/", len);
		strncat(path, name, len);
		strncat(path, "/stylerc", len);
		if (!(config.stylefile = findrcfile(path)))
			DPRINTF("Could not find style file for %s\n", path);
		free(path);
		if (config.stylefile && access(config.stylefile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.stylefile, strerror(errno));
			free(config.stylefile);
			config.stylefile = NULL;
		}
		if (!config.stylefile && strcmp(name, "Default")) {
			if (!(config.stylefile = findrcfile("styles/Default/stylerc")))
				DPRINTF("Could not find style file for %s\n", "styles/Default/stylerc");
		}
		if (config.stylefile && access(config.stylefile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.stylefile, strerror(errno));
			free(config.stylefile);
			config.stylefile = NULL;
		}
	}
	if (!config.stylefile) {
		DPRINTF("Could not find readable style file.\n");
		return;
	}
	DPRINTF("Reading databse file %s\n", config.stylefile);
	srdb = XrmGetFileDatabase(config.stylefile);
	if (!srdb) {
		DPRINTF("Could not read database file '%s'\n", config.stylefile);
		return;
	}
	XrmMergeDatabases(srdb, &xrdb);
}

static void
initthemefile(void)
{
	XrmDatabase trdb;
	const char *file, *name;
	char *path;
	int len;

	free(config.themefile);
	config.themefile = NULL;

	if ((name = getresource("theme.name", NULL))) {
		/* already loaded a theme, must be included in adwmrc */
		if (config.rcfile)
			config.themefile = strdup(config.rcfile);
		return;
	}
	/* haven't loaded a theme file yet */
	if (!config.themefile && (file = getresource("themeFile", "themerc"))) {
		if (!(config.themefile = findrcfile(file)))
			DPRINTF("Could not find theme file for %s\n", file);
		if (config.themefile && access(config.themefile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.themefile, strerror(errno));
			free(config.themefile);
			config.themefile = NULL;
		}
		if (!config.themefile && strcmp(file, "themerc")) {
			if (!(config.themefile = findrcfile("themerc")))
				DPRINTF("Could not find theme file for %s\n", "themerc");
		}
		if (config.themefile && access(config.themefile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.themefile, strerror(errno));
			free(config.themefile);
			config.themefile = NULL;
		}
	}
	if (!config.themefile && (name = getresource("themeName", "Default"))) {
		len = strlen("themes/") + strlen(name) + strlen("/themerc");
		path = ecalloc(len + 1, sizeof(*path));
		strncpy(path, "themes/", len);
		strncat(path, name, len);
		strncat(path, "/themerc", len);
		if (!(config.themefile = findrcfile(path)))
			DPRINTF("Could not find theme file for %s\n", path);
		free(path);
		if (config.themefile && access(config.themefile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.themefile, strerror(errno));
			free(config.themefile);
			config.themefile = NULL;
		}
		if (!config.themefile && strcmp(name, "Default")) {
			if (!(config.themefile = findrcfile("themes/Default/themerc")))
				DPRINTF("Could not find theme file for %s\n", "themes/Default/themerc");
		}
		if (config.themefile && access(config.themefile, R_OK)) {
			DPRINTF("Could not access %s: %s\n", config.themefile, strerror(errno));
			free(config.themefile);
			config.themefile = NULL;
		}
	}
	if (!config.themefile) {
		DPRINTF("Could not find readable theme file.\n");
		return;
	}
	DPRINTF("Reading databse file %s\n", config.themefile);
	trdb = XrmGetFileDatabase(config.themefile);
	if (!trdb) {
		DPRINTF("Could not read database file '%s'\n", config.themefile);
		return;
	}
	XrmMergeDatabases(trdb, &xrdb);
}

static void
initrcdirs(const char *conf, Bool reload)
{
	int i, len;

	free(config.rdir);
	len = strlen(xdgdirs.runt) + strlen("/adwm/");
	config.rdir = ecalloc(len + 1, sizeof(*config.rdir));
	strncpy(config.rdir, xdgdirs.runt, len);
	strncat(config.rdir, "/adwm/", len);
	DPRINTF("config.rdir = %s\n", config.rdir);

	free(config.xdir);
	len = strlen(xdgdirs.conf.home) + strlen("/adwm/");
	config.xdir = ecalloc(len + 1, sizeof(*config.xdir));
	strncpy(config.xdir, xdgdirs.conf.home, len);
	strncat(config.xdir, "/adwm/", len);
	DPRINTF("config.xdir = %s\n", config.xdir);

	free(config.udir);
	len = strlen(xdgdirs.home) + strlen("/.adwm/");
	config.udir = ecalloc(len + 1, sizeof(*config.udir));
	strncpy(config.udir, xdgdirs.home, len);
	strncat(config.udir, "/.adwm/", len);
	DPRINTF("config.udir = %s\n", config.udir);

	free(config.sdir);
	len = strlen(SYSCONFPATH) + 1;
	config.sdir = ecalloc(len + 1, sizeof(*config.sdir));
	strncpy(config.sdir, SYSCONFPATH, len);
	strncat(config.sdir, "/", len);
	DPRINTF("config.sdir = %s\n", config.sdir);

	free(config.pdir);
	config.pdir = NULL;

	free(config.rcfile);
	config.rcfile = NULL;
	if (conf) {
		if (*conf == '/')
			config.rcfile = strdup(conf);
		else {
			len = strlen(xdgdirs.home) + strlen(conf) + 1;
			config.rcfile = ecalloc(len + 1, sizeof(*config.rcfile));
			strncpy(config.rcfile, xdgdirs.home, len);
			strncat(config.rcfile, "/", len);
			strncat(config.rcfile, conf, len);
		}
		config.pdir = strdup(config.rcfile);
		if (strrchr(config.pdir, '/'))
			*(strrchr(config.pdir, '/') + 1) = '\0';
	} else {
		for (i = 1; i < 4; i++) {
			free(config.pdir);
			config.pdir = strdup(config.dirs[i]);
			len = strlen(config.pdir) + strlen("adwmrc") + 1;
			free(config.rcfile);
			config.rcfile = ecalloc(len + 1, sizeof(*config.rcfile));
			strncpy(config.rcfile, config.pdir, len);
			strncat(config.rcfile, "adwmrc", len);
			if (!access(config.rcfile, F_OK))
				break;
		}
	}
	DPRINTF("config.pdir = %s\n", config.pdir);
	/* XXX: if config.rcfile or its directory, config.pdir, doesn't exist, we will
	   read config.sdir/adwmrc when it comes to reading, and will create config.pdir
	   and write config.rcfile when it comes to it. */
}

void
initrcfile(const char *conf, Bool reload)
{
	const char *file = NULL;
	char *rcfile, *dir;
	int i, len;
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
	initrcdirs(file, reload);
	dir = strdup(config.pdir);
	rcfile = strdup(config.rcfile);
	if (chdir(dir))
		DPRINTF("Could not change directory to %s: %s\n", dir, strerror(errno));
	DPRINTF("Reading databse file %s\n", rcfile);
	xrdb = XrmGetFileDatabase(rcfile);
	if (!xrdb) {
		DPRINTF("Couldn't find database file '%s', using defaults\n", rcfile);
		free(dir);
		dir = strdup(config.sdir);
		free(rcfile);
		len = strlen(dir) + strlen("adwmrc");
		rcfile = ecalloc(len + 1, sizeof(*rcfile));
		strncpy(rcfile, dir, len);
		strncat(rcfile, "adwmrc", len);
		if (chdir(dir))
			DPRINTF("Could not change directory to %s: %s\n", dir, strerror(errno));
		DPRINTF("Reading databse file %s\n", rcfile);
		xrdb = XrmGetFileDatabase(rcfile);
		if (!xrdb)
			DPRINTF("Couldn't find database file '%s', using defaults\n", rcfile);
	}

	initthemefile();	/* read theme elements into the database */
	initstylefile();	/* read style elements into the database */
	initkeysfile();		/* read key bindings into the database */
	initdockfile();		/* read dock elements into the database */

	/* might want to pass these to above, instead of changing directories */
	/* except we should probably search pdir, rdir, xdir, udir, sdir for
	 * everything. */
	free(dir);
	free(rcfile);
}
