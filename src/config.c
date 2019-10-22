/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "resource.h"
#include "config.h" /* verification */

Options options;

AdwmPlaces config = { NULL, };

void
inittags(Bool reload)
{
	unsigned i;

	for (i = 0; i < MAXTAGS; i++) {
		const char *res;
		char resource[256], def[8];
		Tag *t = scr->tags + i;

		snprintf(def, sizeof(def), "%u", i);
		snprintf(resource, sizeof(resource), "tags.name%u", i);
		res = getscreenres(resource, def);
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
	unsigned i;
	Monitor *m;

	if (reload)
		return;

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
		char resource[256];
		Leaf *l;
		char *res_name, *res_class, *wm_command;

		snprintf(resource, sizeof(resource), "dock.app%u", i);
		res = getscreenres(resource, NULL);
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
initviews(Bool reload)
{
	unsigned i;

	if (reload)
		return;
	for (i = 0; i < MAXTAGS; i++) {
		const char *res;
		char resource[256], defl[2];
		Layout *l;
		View *v = scr->views + i;
		int strutsactive;

		snprintf(resource, sizeof(resource), "taqs.layout%u", i);
		defl[0] = scr->options.deflayout; defl[1] = '\0';
		res = getscreenres(resource, defl);
		v->layout = layouts;
		for (l = layouts; l->symbol; l++)
			if (l->symbol == *res) {
				v->layout = l;
				break;
			}
		l = v->layout;
		snprintf(resource, sizeof(resource), "view%u.strutsactive", i);
		strutsactive = (res = getscreenres(resource, NULL))
			? atoi(res) : scr->options.strutsactive;
		if (strutsactive)
			v->barpos = StrutsOn;
		else {
			if (scr->options.hidebastards == 2)
				v->barpos = StrutsDown;
			else if (scr->options.hidebastards)
				v->barpos = StrutsHide;
			else
				v->barpos = StrutsOff;
		}
		snprintf(resource, sizeof(resource), "view%u.decoratetiled", i);
		v->dectiled = (res = getscreenres(resource, NULL))
			?  atoi(res) : scr->options.dectiled;
		snprintf(resource, sizeof(resource), "view%u.nmaster", i);
		v->nmaster = (res = getscreenres(resource, NULL))
			? atoi(res) : scr->options.nmaster;
		snprintf(resource, sizeof(resource), "view%u.ncolumns", i);
		v->ncolumns = (res = getscreenres(resource, NULL))
			? atoi(res) : scr->options.ncolumns;
		snprintf(resource, sizeof(resource), "view%u.mwfact", i);
		v->mwfact = (res = getscreenres(resource, NULL))
			? atof(res) : scr->options.mwfact;
		snprintf(resource, sizeof(resource), "view%u.mhfact", i);
		v->mhfact = (res = getscreenres(resource, NULL))
			? atof(res) : scr->options.mhfact;
		snprintf(resource, sizeof(resource), "view%u.major", i);
		v->major = (res = getscreenres(resource, NULL))
			? atoi(res) : l->major;
		if (v->major < OrientLeft || v->major > OrientBottom)
			v->major = l->major;
		snprintf(resource, sizeof(resource), "view%u.minor", i);
		v->minor = (res = getscreenres(resource, NULL))
			? atoi(res) : l->minor;
		if (v->minor < OrientLeft || v->minor > OrientBottom)
			v->minor = l->minor;
		snprintf(resource, sizeof(resource), "view%u.placement", i);
		v->placement = (res = getscreenres(resource, NULL))
			? atoi(res) : scr->options.placement;
		if (v->placement < ColSmartPlacement || v->placement > RandomPlacement)
			v->placement = scr->options.placement;
		v->index = i;
		/* restore any saved view tags */
		v->seltags = (1ULL << i);
		snprintf(resource, sizeof(resource), "view%u.seltags", i);
		if ((res = getscreenres(resource, NULL))) {
			unsigned long long tags = 0;

			sscanf(res, "%Lx", &tags);
			if (tags)
				v->seltags = tags;
		}
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

	scr->options = options;

	if ((res = getscreenres("attachaside", NULL)))
		scr->options.attachaside = atoi(res) ? True : False;
	if ((res = getscreenres("command", NULL)))
		scr->options.command = res;
	if ((res = getscreenres("command2", NULL)))
		scr->options.command2 = res;
	if ((res = getscreenres("command3", NULL)))
		scr->options.command3 = res;
	if ((res = getscreenres("menucommand", NULL)))
		scr->options.menucommand = res;
	if ((res = getscreenres("decoratetiled", NULL)))
		scr->options.dectiled = atoi(res);
	if ((res = getscreenres("decoratemax", NULL)))
		scr->options.decmax = atoi(res) ? True : False;
	if ((res = getscreenres("hidebastards", NULL)))
		scr->options.hidebastards = atoi(res);
	if ((res = getscreenres("strutsactive", NULL)))
		scr->options.strutsactive = atoi(res) ? True : False;
	if ((res = getscreenres("autoroll", NULL)))
		scr->options.autoroll = atoi(res) ? True : False;
	if ((res = getscreenres("strutsdelay", NULL)))
		scr->options.strutsdelay = atoi(res);
	if ((res = getscreenres("sloppy", NULL)))
		scr->options.focus = atoi(res);
	if ((res = getscreenres("snap", NULL)))
		scr->options.snap = atoi(res);
	if ((res = getscreenres("dock.position", NULL)))
		scr->options.dockpos = atoi(res);
	if ((res = getscreenres("dock.orient", NULL)))
		scr->options.dockori = atoi(res);
	if ((res = getscreenres("dock.monitor", NULL)))
		scr->options.dockmon = atoi(res);
	if ((res = getscreenres("dragdistance", NULL)))
		scr->options.dragdist = atoi(res);
	if ((res = getscreenres("mwfact", NULL))) {
		scr->options.mwfact = atof(res);
		if (scr->options.mwfact < 0.10 || scr->options.mwfact > 0.90)
			scr->options.mwfact = options.mwfact;
	}
	if ((res = getscreenres("mhfact", NULL))) {
		scr->options.mhfact = atof(res);
		if (scr->options.mhfact < 0.10 || scr->options.mhfact > 0.90)
			scr->options.mhfact = options.mhfact;
	}
	if ((res = getscreenres("nmaster", NULL))) {
		scr->options.nmaster = atoi(res);
		if (scr->options.nmaster < 1 || scr->options.nmaster > 10)
			scr->options.nmaster = options.nmaster;
	}
	if ((res = getscreenres("ncolumns", NULL))) {
		scr->options.ncolumns = atoi(res);
		if (scr->options.ncolumns < 1 || scr->options.ncolumns > 10)
			scr->options.ncolumns = options.ncolumns;
	}
	if ((res = getscreenres("deflayout", NULL))) {
		if (strnlen(res, 2) == 1)
			scr->options.deflayout = res[0];
		else
			scr->options.deflayout = 'i';
	}
	if ((res = getscreenres("placement", NULL))) {
		scr->options.placement = atoi(res);
		if (scr->options.placement < 0 || scr->options.placement > RandomPlacement)
			scr->options.placement = options.placement;
	}
}

void
initconfig(Bool reload)
{
	const char *res;

	/* init debug first */
	options.debug = atoi(getsessionres("debug", "0"));
	/* init appearance */
	options.useveil = atoi(getsessionres("useveil", "0")) ? True : False;
	options.attachaside = atoi(getsessionres("attachaside", "1")) ? True : False;
	options.command = getsessionres("command", COMMAND);
	options.command2 = getsessionres("command2", COMMAND2);
	options.command3 = getsessionres("command3", COMMAND3);
	options.menucommand = getsessionres("menucommand", MENUCOMMAND);
	options.dectiled = atoi(getsessionres("decoratetiled", STR(DECORATETILED)));
	options.decmax = atoi(getsessionres("decoratemax", STR(DECORATEMAX)));
	options.hidebastards = atoi(getsessionres("hidebastards", "0"));
	options.strutsactive = atoi(getsessionres("strutsactive", "1")) ? True : False;
	options.autoroll = atoi(getsessionres("autoroll", "0")) ? True : False;
	options.showdesk = atoi(getsessionres("showdesk", "0")) ? True : False;
	options.strutsdelay = atoi(getsessionres("strutsdelay", "500"));
	options.focus = atoi(getsessionres("sloppy", "0"));
	options.snap = atoi(getsessionres("snap", STR(SNAP)));
	options.dockpos = atoi(getsessionres("dock.position", "1"));
	options.dockori = atoi(getsessionres("dock.orient", "1"));
	options.dockmon = atoi(getsessionres("dock.monitor", "0"));
	options.dragdist = atoi(getsessionres("dragdistance", "5"));
	options.mwfact = atof(getsessionres("mwfact", STR(DEFMWFACT)));
	if (options.mwfact < 0.10 || options.mwfact > 0.90)
		options.mwfact = DEFMWFACT;
	options.mhfact = atof(getsessionres("mhfact", STR(DEFMHFACT)));
	if (options.mhfact < 0.10 || options.mwfact > 0.90)
		options.mhfact = DEFMHFACT;
	options.nmaster = atoi(getsessionres("nmaster", STR(DEFNMASTER)));
	if (options.nmaster < 1 || options.nmaster > 10)
		options.nmaster = DEFNMASTER;
	options.ncolumns = atoi(getsessionres("ncolumns", STR(DEFNCOLUMNS)));
	if (options.ncolumns < 1 || options.ncolumns > 10)
		options.ncolumns = DEFNCOLUMNS;
	res = getsessionres("deflayout", "i");
	if (strlen(res) == 1)
		options.deflayout = res[0];
	else
		options.deflayout = 'i';
	options.placement = atoi(getsessionres("placement", "0"));
	if (options.placement < 0 || options.placement > RandomPlacement)
		options.placement = 0;
	options.ntags = strtoul(getsessionres("tags.number", "5"), NULL, 0);
	if (options.ntags < 1 || options.ntags > MAXTAGS)
		options.ntags = 5;
	options.prependdirs = getsessionres("icon.prependdirs", NULL);
	options.appenddirs = getsessionres("icon.appenddirs", NULL);
	options.icontheme = getsessionres("icon.theme", NULL);
#if defined LIBRSVG
	options.extensions = getsessionres("icon.extensions", "png,svg,xpm");
#else
	options.extensions = getsessionres("icon.extensions", "png,xpm");
#endif
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
						XPRINTF("Cannot readlink %s: %s\n", path, strerror(errno));
						continue;
					}
					free(buf);
				}
				break;
			} else {
				XPRINTF("Cannot stat %s: %s\n", path, strerror(errno));
				continue;
			}
		} else {
			XPRINTF("Cannot access %s: %s\n", path, strerror(errno));
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
		if (!access(path, R_OK))
			return (path);
		free(path);
		path = NULL;
	} else {
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
			XPRINTF("Could not find dock file for %s\n", file);
		if (config.dockfile && access(config.dockfile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.dockfile, strerror(errno));
			free(config.dockfile);
			config.dockfile = NULL;
		}
		if (!config.dockfile && strcmp(file, "dockrc")) {
			if (!(config.dockfile = findrcfile("dockrc")))
			XPRINTF("Could not find dock file for %s\n", "dockrc");
		}
		if (config.dockfile && access(config.dockfile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.dockfile, strerror(errno));
			free(config.dockfile);
			config.dockfile = NULL;
		}
	}
	if (!config.dockfile) {
		if (!(config.dockfile = findthemepath("dockrc")))
			XPRINTF("Could not find dock file for %s\n", file);
		if (config.dockfile && access(config.dockfile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.dockfile, strerror(errno));
			free(config.dockfile);
			config.dockfile = NULL;
		}
	}
	if (!config.dockfile) {
		if (!(config.dockfile = findstylepath("dockrc")))
			XPRINTF("Could not find dock file for %s\n", "dockrc");
		if (config.dockfile && access(config.dockfile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.dockfile, strerror(errno));
			free(config.dockfile);
			config.dockfile = NULL;
		}
	}
	if (!config.dockfile) {
		XPRINTF("Could not find readable dock file.\n");
		return;
	}
	XPRINTF("Reading databse file %s\n", config.dockfile);
	drdb = XrmGetFileDatabase(config.dockfile);
	if (!drdb) {
		XPRINTF("Could not read database file '%s'\n", config.dockfile);
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
			XPRINTF("Could not find keys file for %s\n", file);
		if (config.keysfile && access(config.keysfile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.keysfile, strerror(errno));
			free(config.keysfile);
			config.keysfile = NULL;
		}
		if (!config.keysfile && strcmp(file, "keysrc")) {
			if (!(config.keysfile = findrcfile("keysrc")))
				XPRINTF("Could not find keys file for %s\n", "keysrc");
		}
		if (config.keysfile && access(config.keysfile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.keysfile, strerror(errno));
			free(config.keysfile);
			config.keysfile = NULL;
		}
	}
	if (!config.keysfile) {
		if (!(config.keysfile = findthemepath("keysrc")))
			XPRINTF("Could not find keys file for %s\n", "keysrc");
		if (config.keysfile && access(config.keysfile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.keysfile, strerror(errno));
			free(config.keysfile);
			config.keysfile = NULL;
		}
	}
	if (!config.keysfile) {
		if (!(config.keysfile = findstylepath("keysrc")))
			XPRINTF("Could not find keys file for %s\n", "keysrc");
		if (config.keysfile && access(config.keysfile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.keysfile, strerror(errno));
			free(config.keysfile);
			config.keysfile = NULL;
		}
	}
	if (!config.keysfile) {
		XPRINTF("Could not find readable keys file.\n");
		return;
	}
	XPRINTF("Reading databse file %s\n", config.keysfile);
	krdb = XrmGetFileDatabase(config.keysfile);
	if (!krdb) {
		XPRINTF("Could not read database file '%s'\n", config.keysfile);
		return;
	}
	XrmMergeDatabases(krdb, &xrdb);
}

static void
initstylefile(void)
{
	XrmDatabase yrdb;
	const char *file, *name;
	char *path, *p, *q;
	int len;

	free(config.stylefile);
	config.stylefile = NULL;
	free(config.stylename);
	config.stylename = NULL;

	if ((name = getresource("style.name", NULL))) {
		config.stylename = strdup(name);
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
			XPRINTF("Could not find style file for %s\n", file);
		if (config.stylefile && access(config.stylefile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.stylefile, strerror(errno));
			free(config.stylefile);
			config.stylefile = NULL;
		}
		if (!config.stylefile && strcmp(file, "stylerc")) {
			if (!(config.stylefile = findrcfile("stylerc")))
				XPRINTF("Could not find style file for %s\n", "stylerc");
		}
		if (config.stylefile && access(config.stylefile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.stylefile, strerror(errno));
			free(config.stylefile);
			config.stylefile = NULL;
		}
	}
	if (!config.stylefile && (name = getresource("styleName", "Default"))) {
		config.stylename = strdup(name);
		len = strlen("styles/") + strlen(name) + strlen("/stylerc");
		path = ecalloc(len + 1, sizeof(*path));
		strncpy(path, "styles/", len);
		strncat(path, name, len);
		strncat(path, "/stylerc", len);
		if (!(config.stylefile = findrcfile(path)))
			XPRINTF("Could not find style file for %s\n", path);
		free(path);
		if (config.stylefile && access(config.stylefile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.stylefile, strerror(errno));
			free(config.stylefile);
			config.stylefile = NULL;
		}
		if (!config.stylefile && strcmp(name, "Default")) {
			if (!(config.stylefile = findrcfile("styles/Default/stylerc")))
				XPRINTF("Could not find style file for %s\n", "styles/Default/stylerc");
		}
		if (config.stylefile && access(config.stylefile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.stylefile, strerror(errno));
			free(config.stylefile);
			config.stylefile = NULL;
		}
	}
	if (!config.stylefile) {
		XPRINTF("Could not find readable style file.\n");
		return;
	}
	if (!config.stylename && (p = q = config.stylefile) && (q = strrchr(q, '/'))) {
		*q = '\0';
		if ((p = strrchr(p, '/')))
			config.stylename = strdup(p + 1);
		*q = '/';
	}
	XPRINTF("Reading database file %s\n", config.stylefile);
	yrdb = XrmGetFileDatabase(config.stylefile);
	if (!yrdb) {
		XPRINTF("Could not read database file '%s'\n", config.stylefile);
		return;
	}
	XrmMergeDatabases(yrdb, &xrdb);
}

static void
initthemefile(void)
{
	XrmDatabase trdb;
	const char *file, *name;
	char *path, *p, *q;
	int len;

	free(config.themefile);
	config.themefile = NULL;
	free(config.themename);
	config.themename = NULL;

	if ((name = getresource("theme.name", NULL))) {
		/* already loaded a theme, must be included in adwmrc */
		config.themename = strdup(name);
		if (config.rcfile)
			config.themefile = strdup(config.rcfile);
		return;
	}
	/* haven't loaded a theme file yet */
	if (!config.themefile && (file = getresource("themeFile", "themerc"))) {
		if (!(config.themefile = findrcfile(file)))
			XPRINTF("Could not find theme file for %s\n", file);
		if (config.themefile && access(config.themefile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.themefile, strerror(errno));
			free(config.themefile);
			config.themefile = NULL;
		}
		if (!config.themefile && strcmp(file, "themerc")) {
			if (!(config.themefile = findrcfile("themerc")))
				XPRINTF("Could not find theme file for %s\n", "themerc");
		}
		if (config.themefile && access(config.themefile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.themefile, strerror(errno));
			free(config.themefile);
			config.themefile = NULL;
		}
	}
	if (!config.themefile && (name = getresource("themeName", "Default"))) {
		config.themename = strdup(name);
		len = strlen("themes/") + strlen(name) + strlen("/themerc");
		path = ecalloc(len + 1, sizeof(*path));
		strncpy(path, "themes/", len);
		strncat(path, name, len);
		strncat(path, "/themerc", len);
		if (!(config.themefile = findrcfile(path)))
			XPRINTF("Could not find theme file for %s\n", path);
		free(path);
		if (config.themefile && access(config.themefile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.themefile, strerror(errno));
			free(config.themefile);
			config.themefile = NULL;
		}
		if (!config.themefile && strcmp(name, "Default")) {
			if (!(config.themefile = findrcfile("themes/Default/themerc")))
				XPRINTF("Could not find theme file for %s\n", "themes/Default/themerc");
		}
		if (config.themefile && access(config.themefile, R_OK)) {
			XPRINTF("Could not access %s: %s\n", config.themefile, strerror(errno));
			free(config.themefile);
			config.themefile = NULL;
		}
	}
	if (!config.themefile) {
		XPRINTF("Could not find readable theme file.\n");
		return;
	}
	if (!config.themename && (p = q = config.themefile) && (q = strrchr(q, '/'))) {
		*q = '\0';
		if ((p = strrchr(p, '/')))
			config.themename = strdup(p + 1);
		*q = '/';
	}
	XPRINTF("Reading databse file %s\n", config.themefile);
	trdb = XrmGetFileDatabase(config.themefile);
	if (!trdb) {
		XPRINTF("Could not read database file '%s'\n", config.themefile);
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
	OPRINTF("config.rdir = %s\n", config.rdir);

	free(config.xdir);
	len = strlen(xdgdirs.conf.home) + strlen("/adwm/");
	config.xdir = ecalloc(len + 1, sizeof(*config.xdir));
	strncpy(config.xdir, xdgdirs.conf.home, len);
	strncat(config.xdir, "/adwm/", len);
	OPRINTF("config.xdir = %s\n", config.xdir);

	free(config.udir);
	len = strlen(xdgdirs.home) + strlen("/.adwm/");
	config.udir = ecalloc(len + 1, sizeof(*config.udir));
	strncpy(config.udir, xdgdirs.home, len);
	strncat(config.udir, "/.adwm/", len);
	OPRINTF("config.udir = %s\n", config.udir);

	free(config.sdir);
	len = strlen(SYSCONFPATH) + 1;
	config.sdir = ecalloc(len + 1, sizeof(*config.sdir));
	strncpy(config.sdir, SYSCONFPATH, len);
	strncat(config.sdir, "/", len);
	OPRINTF("config.sdir = %s\n", config.sdir);

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
	OPRINTF("config.pdir = %s\n", config.pdir);
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
		XPRINTF("Could not change directory to %s: %s\n", dir, strerror(errno));
	XPRINTF("Reading databse file %s\n", rcfile);
	xrdb = XrmGetFileDatabase(rcfile);
	if (!xrdb) {
		XPRINTF("Couldn't find database file '%s', using defaults\n", rcfile);
		free(dir);
		dir = strdup(config.sdir);
		free(rcfile);
		len = strlen(dir) + strlen("adwmrc");
		rcfile = ecalloc(len + 1, sizeof(*rcfile));
		strncpy(rcfile, dir, len);
		strncat(rcfile, "adwmrc", len);
		if (chdir(dir))
			XPRINTF("Could not change directory to %s: %s\n", dir, strerror(errno));
		XPRINTF("Reading databse file %s\n", rcfile);
		xrdb = XrmGetFileDatabase(rcfile);
		if (!xrdb)
			XPRINTF("Couldn't find database file '%s', using defaults\n", rcfile);
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
