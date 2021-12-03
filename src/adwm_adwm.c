/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "resource.h"
#include "actions.h"
#include "parse.h"
#include "config.h"

/*
 * The purpose of this file is to provide a loadable module that provides
 * functions for providing the behaviour specified to adwm.  This module reads
 * the adwm configuration file to determine configuration information, and
 * access an adwm(1) style file.  It also reads the adwm(1) keys configuration
 * file to find key bindings.
 *
 * This module is really just intenteded to provide the default adwm(1)
 * behaviour as a separate loadable module (because it is not needed when
 * adwm(1) is mimicing the behaviour of another window manager).
 */

typedef struct {
	char *udir;			/* user directory */
	char *pdir;			/* private directory */
	char *sdir;			/* system directory */
	char *rcfile;			/* rcfile */
	char *keysfile;			/* kerysrc file */
	char *btnsfile;			/* buttonrc file */
	char *rulefile;			/* rulerc file */
	char *stylefile;		/* stylerc file */
	char *themefile;		/* themerc file */
} AdwmConfig;

Options options;

static AdwmConfig config;

static XrmDatabase xconfigdb;		/* general configuration */
static XrmDatabase xkeysdb;		/* key binding configuration */
static XrmDatabase xbtnsdb;		/* button binding configuration */
static XrmDatabase xruledb;		/* window rule configuration */
static XrmDatabase xstyledb;		/* current style configuration */
static XrmDatabase xthemedb;		/* current theme configuration */

typedef struct {
	struct {
		struct {
			XColor border;
			XColor bg;
			XColor fg;
			XColor button;
			XftColor font;
			XftColor shadow;
		} color;
		AdwmFont font;
		unsigned drop;

	} normal, selected;
	unsigned border;
	unsigned margin;
	unsigned opacity;
	Bool outline;
	unsigned spacing;
	const char *titlelayout;
	unsigned grips;
	unsigned title;
	Texture *elements;
} AdwmStyle;

static AdwmStyle *styles;

typedef struct {
} AdwmTheme;

static AdwmTheme *themes;

/** @brief initkeysfile_ADWM: locate and load runtime keys file
  *
  * @{ */
static void
initkeysfile_ADWM(void)
{
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
	if (xkeysdb) {
		XrmDestroyDatabase(xkeysdb);
		xkeysdb = NULL;
	}
	xkeysdb = XrmGetFileDatabase(config.keysfile);
	if (!xkeysdb) {
		XPRINTF("Could not find database file '%s'\n", config.keysfile);
		return;
	}
	XrmMergeDatabases(xkeysdb, &xconfigdb);
}
/** @} */

/** @brief initbtnsfile_ADWM: locate and load runtime buttons file
  *
  * @{ */
static void
initbtnsfile_ADWM(void)
{
	const char *file;
	struct stat st;
	char *path;

	file = getresource("buttonFile", "buttonrc");
	path = ecalloc(PATH_MAX + 1, sizeof(*path));
	strncpy(path, file, PATH_MAX);
	if (!lstat(file, &st) && S_ISLNK(st.st_mode)) {
		if (readlink(file, path, PATH_MAX) == -1)
			eprint("%s: %s\n", file, strerror(errno));
	}
	free(config.btnsfile);
	config.btnsfile = strdup(path);
	if (*config.btnsfile != '/') {
		strncpy(path, config.rcfile, PATH_MAX);
		if (strrchr(path, '/'))
			*strrchr(path, '/') = '\0';
		strncat(path, "/", PATH_MAX);
		strncat(path, config.btnsfile, PATH_MAX);
		free(config.btnsfile);
		config.btnsfile = strdup(path);
	}
	free(path);
	if (xbtnsdb) {
		XrmDestroyDatabase(xbtnsdb);
		xbtnsdb = NULL;
	}
	xbtnsdb = XrmGetFileDatabase(config.btnsfile);
	if (!xbtnsdb) {
		XPRINTF("Could not find database file '%s'\n", config.btnsfile);
		return;
	}
	XrmMergeDatabases(xbtnsdb, &xconfigdb);
}
/** @} */

/** @brief initrulefile_ADWM: locate and load runtime rules file
  *
  * @{ */
static void
initrulefile_ADWM(void)
{
	const char *file;
	struct stat st;
	char *path;

	file = getresource("ruleFile", "rulerc");
	path = ecalloc(PATH_MAX + 1, sizeof(*path));
	strncpy(path, file, PATH_MAX);
	if (!lstat(file, &st) && S_ISLNK(st.st_mode)) {
		if (readlink(file, path, PATH_MAX) == -1)
			eprint("%s: %s\n", file, strerror(errno));
	}
	free(config.rulefile);
	config.rulefile = strdup(path);
	if (*config.rulefile != '/') {
		strncpy(path, config.rcfile, PATH_MAX);
		if (strrchr(path, '/'))
			*strrchr(path, '/') = '\0';
		strncat(path, "/", PATH_MAX);
		strncat(path, config.rulefile, PATH_MAX);
		free(config.rulefile);
		config.rulefile = strdup(path);
	}
	free(path);
	if (xruledb) {
		XrmDestroyDatabase(xruledb);
		xruledb = NULL;
	}
	xruledb = XrmGetFileDatabase(config.rulefile);
	if (!xruledb) {
		XPRINTF("Could not find database file '%s'\n", config.rulefile);
		return;
	}
	XrmMergeDatabases(xruledb, &xconfigdb);
}
/** @} */

/** @brief initstylefile_ADWM: locate and load runtime style file
  *
  * @{ */
static void
initstylefile_ADWM(void)
{
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
	if (xstyledb) {
		XrmDestroyDatabase(xstyledb);
		xstyledb = NULL;
	}
	xstyledb = XrmGetFileDatabase(config.stylefile);
	if (!xstyledb) {
		XPRINTF("Could not find database file '%s'\n", config.stylefile);
		return;
	}
	XrmMergeDatabases(xstyledb, &xconfigdb);
}
/** @} */

/** @brief initthemefile_ADWM: locate and load runtime theme file
  *
  * @{ */
static void
initthemefile_ADWM(void)
{
	const char *file;
	struct stat st;
	char *path;

	file = getresource("styleFile", "stylerc");
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
	if (xthemedb) {
		XrmDestroyDatabase(xthemedb);
		xthemedb = NULL;
	}
	xthemedb = XrmGetFileDatabase(config.themefile);
	if (!xthemedb) {
		XPRINTF("Could not find database file '%s'\n", config.themefile);
		return;
	}
	XrmMergeDatabases(xthemedb, &xconfigdb);
}
/** @} */

/** @brief initrcfile_ADWM: locate and load runtime configuration file
  *
  * @{ */
static void
initrcfile_ADWM(const char *conf __attribute__((unused)), Bool reload __attribute__((unused)))
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
	if (xconfigdb) {
		XrmDestroyDatabase(xconfigdb);
		xconfigdb = NULL;
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

	xconfigdb = XrmGetFileDatabase(config.rcfile);
	if (!xconfigdb) {
		free(config.rcfile);
		len = strlen(config.pdir) + strlen("/adwmrc") + 2;
		config.rcfile = ecalloc(len + 1, sizeof(*config.rcfile));
		strncpy(config.rcfile, config.pdir, len);
		strncat(config.rcfile, "/adwmrc", len);
		xconfigdb = XrmGetFileDatabase(config.rcfile);
	}
	if (!xconfigdb) {
		free(config.rcfile);
		len = strlen(config.udir) + strlen("/adwmrc") + 2;
		config.rcfile = ecalloc(len + 1, sizeof(*config.rcfile));
		strncpy(config.rcfile, config.udir, len);
		strncat(config.rcfile, "/adwmrc", len);
		xconfigdb = XrmGetFileDatabase(config.rcfile);
	}
	if (!xconfigdb) {
		free(config.rcfile);
		len = strlen(config.sdir) + strlen("/adwmrc") + 2;
		config.rcfile = ecalloc(len + 1, sizeof(*config.rcfile));
		strncpy(config.rcfile, config.sdir, len);
		strncat(config.rcfile, "/adwmrc", len);
		xconfigdb = XrmGetFileDatabase(config.rcfile);
	}
	if (xconfigdb) {
		char *dir;

		dir = strdup(config.rcfile);
		if (strrchr(dir, '/'))
			*strrchr(dir, '/') = '\0';
		if (chdir(dir))
			XPRINTF("Could not change directory to %s: %s\n", dir, strerror(errno));
		free(dir);
	} else {
		XPRINTF("Couldn't find database file '%s'\n", config.rcfile);
		EPRINTF("Could not find usable database, using defaults\n");
		if (chdir(config.udir))
			XPRINTF("Could not change directory to %s: %s\n", config.udir, strerror(errno));
	}
	initkeysfile_ADWM();
	initbtnsfile_ADWM();
	initrulefile_ADWM();
	initstylefile_ADWM();
	initthemefile_ADWM();
}
/** @} */


/** @brief initconfig_ADWM: perform global configuration
  *
  * @{ */
static void
initconfig_ADWM(Bool reload __attribute__((unused)))
{
	const char *res;
	char name[256], clas[256], *n, *c;
	size_t nlen, clen;

	xresdb = xconfigdb;

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
	strncpy(n, "Useveil", clen);
	options.useveil  = atoi(readres(name, clas, "0")) ? True : False;
	strncpy(n, "attachaside", nlen);
	strncpy(c, "Attachaside", clen);
	options.attachaside  = atoi(readres(name, clas, "1")) ? True : False;
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
	strcpy(n, "strutsactive");
	strcpy(c, "StrutsActive");
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
	if (options.mhfact < 0.10 || options.mhfact > 0.90)
		options.mhfact = DEFMHFACT;

	strncpy(n, "nmaster", nlen);
	strncpy(c, "Nmaster", clen);
	options.nmaster = atof(readres(name, clas, STR(DEFNMASTER)));
	if (options.nmaster < 1 || options.nmaster > 10)
		options.nmaster = DEFNMASTER;

	strncpy(n, "ncolumns", nlen);
	strncpy(c, "Ncolumns", clen);
	options.ncolumns = atof(readres(name, clas, STR(DEFNCOLUMNS)));
	if (options.ncolumns < 1 || options.ncolumns > 10)
		options.ncolumns = DEFNCOLUMNS;

	strncpy(n, "deflayout", nlen);
	strncpy(c, "Deflayout", clen);
	res = readres(name, clas, "i");
	if (strlen(res) == 1)
		options.deflayout = res[0];
	else
		options.deflayout = 'i';

	strncpy(n, "tags.number", nlen);
	strncpy(c, "Tags.Number", clen);
	options.ntags = atoi(readres(name, clas, "5"));
	if (options.ntags < 1 || options.ntags > MAXTAGS)
		options.ntags = 5;
}
/** @} */

/** @brief initscreen_ADWM: perform per-screen configuration
  *
  * @{ */
static void
initscreen_ADWM(Bool reload __attribute__((unused)))
{
	const char *res;
	char name[256], clas[256], *n, *c;
	size_t nlen, clen;
	unsigned s = scr->screen;

	xresdb = xconfigdb;

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
			scr->options.deflayout = res[0];
		else
			scr->options.deflayout = 'i';
	}
}
/** @} */

/** @brief inittags_ADWM: initialize tags
  *
  * Set each tag to the default name
  *
  * @{ */
static void
inittags_ADWM(Bool reload)
{
	unsigned i, s = scr->screen;

	xresdb = xconfigdb;

	for (i = 0; i < MAXTAGS; i++) {
		const char *res;
		char name[256], clas[256], def[8];
		Tag *t = scr->tags + i;

		snprintf(def, sizeof(def), "%u", i);
		snprintf(name, sizeof(name), "adwm.screen%u.tags.name%u", s, i);
		snprintf(clas, sizeof(clas), "Adwm.Screen%u.Tags.Name%u", s, i);
		res = readres(name, clas, def);
		strncpy(t->name, res, sizeof(t->name) - 1);
	}
	scr->ntags = scr->options.ntags;

	ewmh_process_net_number_of_desktops();
	ewmh_process_net_desktop_names();

	if (reload) {
		unsigned ntags = scr->ntags;

		for (; ntags > scr->ntags; ntags--)
			deltag();
		for (; ntags < scr->ntags; ntags++)
			addtag();

		ewmh_process_net_desktop_names();
		ewmh_update_net_number_of_desktops();
	}
}
/** @} */

typedef struct {
	const char *name;
	void (*action) (XEvent *e, Key *k);
	char *arg;
} KeyItem;

static KeyItem KeyItems[] = {
	/* *INDENT-OFF* */
	{ "viewprevtag",	k_viewprevtag,	 NULL		},
	{ "quit",		k_quit,		 NULL		}, /* arg is new command */
	{ "restart",		k_restart,	 NULL		}, /* arg is new command */
	{ "reload",		k_reload,	 NULL		},
	{ "killclient",		k_killclient,	 NULL		},
	{ "focusmain",		k_focusmain,	 NULL		},
	{ "focusurgent",	k_focusurgent,	 NULL		},
	{ "zoom",		k_zoom,		 NULL		},
	{ "moveright",		k_moveresizekb,	 "+5 +0 +0 +0"	}, /* arg is delta geometry */
	{ "moveleft",		k_moveresizekb,	 "-5 +0 +0 +0"	}, /* arg is delta geometry */
	{ "moveup",		k_moveresizekb,	 "+0 -5 +0 +0"	}, /* arg is delta geometry */
	{ "movedown",		k_moveresizekb,	 "+0 +5 +0 +0"	}, /* arg is delta geometry */
	{ "resizedecx",		k_moveresizekb,	 "+3 +0 -6 +0"	}, /* arg is delta geometry */
	{ "resizeincx",		k_moveresizekb,	 "-3 +0 +6 +0"	}, /* arg is delta geometry */
	{ "resizedecy",		k_moveresizekb,	 "+0 +3 +0 -6"	}, /* arg is delta geometry */
	{ "resizeincy",		k_moveresizekb,	 "+0 -3 +0 +6"	}, /* arg is delta geometry */
	{ "togglemonitor",	k_togglemonitor, NULL		},
	{ "appendtag",		k_appendtag,	 NULL		},
	{ "rmlasttag",		k_rmlasttag,	 NULL		},
	{ "flipview",		k_flipview,	 NULL		},
	{ "rotateview",		k_rotateview,	 NULL		},
	{ "unrotateview",	k_unrotateview,	 NULL		},
	{ "flipzone",		k_flipzone,	 NULL		},
	{ "rotatezone",		k_rotatezone,	 NULL		},
	{ "unrotatezone",	k_unrotatezone,	 NULL		},
	{ "flipwins",		k_flipwins,	 NULL		},
	{ "rotatewins",		k_rotatewins,	 NULL		},
	{ "unrotatewins",	k_unrotatewins,	 NULL		},
	{ "raise",		k_raise,	 NULL		},
	{ "lower",		k_lower,	 NULL		},
	{ "raiselower",		k_raiselower,	 NULL		}
	/* *INDENT-ON* */
};

static const struct {
	const char *prefix;
	ActionCount act;
} inc_prefix[] = {
	/* *INDENT-OFF* */
	{ "set", SetCount},	/* set count to arg or reset no arg */
	{ "inc", IncCount},	/* increment count by arg (or 1) */
	{ "dec", DecCount}	/* decrement count by arg (or 1) */
	/* *INDENT-ON* */
};

static KeyItem KeyItemsByAmt[] = {
	/* *INDENT-OFF* */
	{ "mwfact",	k_setmwfactor,	NULL },
	{ "nmaster",	k_setnmaster,	NULL },
	{ "ncolumns",	k_setncolumns,	NULL },
	{ "margin",	k_setmargin,	NULL },
	{ "border",	k_setborder,	NULL }
	/* *INDENT-ON* */
};

static const struct {
	const char *prefix;
	FlagSetting set;
} set_prefix[] = {
	/* *INDENT-OFF* */
	{ "",		SetFlagSetting	    },
	{ "set",	SetFlagSetting	    },
	{ "un",		UnsetFlagSetting    },
	{ "de",		UnsetFlagSetting    },
	{ "unset",	UnsetFlagSetting    },
	{ "toggle",	ToggleFlagSetting   }
	/* *INDENT-ON* */
};

static const struct {
	const char *suffix;
	WhichClient any;
} set_suffix[] = {
	/* *INDENT-OFF* */
	{ "",		FocusClient	},
	{ "sel",	ActiveClient	},
	{ "ptr",	PointerClient	},
	{ "all",	AllClients	},	/* all on current monitor */
	{ "any",	AnyClient	},	/* clients on any monitor */
	{ "every",	EveryClient	}	/* clients on every workspace */
	/* *INDENT-ON* */
};

static KeyItem KeyItemsByState[] = {
	/* *INDENT-OFF* */
	{ "floating",	k_setfloating,	NULL },
	{ "fill",	k_setfill,	NULL },
	{ "full",	k_setfull,	NULL },
	{ "max",	k_setmax,	NULL },
	{ "maxv",	k_setmaxv,	NULL },
	{ "maxh",	k_setmaxh,	NULL },
	{ "lhalf",	k_setlhalf,	NULL },
	{ "rhalf",	k_setrhalf,	NULL },
	{ "shade",	k_setshade,	NULL },
	{ "shaded",	k_setshade,	NULL },
	{ "hide",	k_sethidden,	NULL },
	{ "hidden",	k_sethidden,	NULL },
	{ "iconify",	k_setmin,	NULL },
	{ "min",	k_setmin,	NULL },
	{ "above",	k_setabove,	NULL },
	{ "below",	k_setbelow,	NULL },
	{ "pager",	k_setpager,	NULL },
	{ "taskbar",	k_settaskbar,	NULL },
	{ "showing",	k_setshowing,	NULL },
	{ "struts",	k_setstruts,	NULL },
	{ "dectiled",	k_setdectiled,	NULL },
	{ "sticky",	k_setsticky,	NULL }
	/* *INDENT-ON* */
};

static const struct {
	const char *suffix;
	RelativeDirection dir;
} rel_suffix[] = {
	/* *INDENT-OFF* */
	{ "NW",	RelativeNorthWest	},
	{ "N",	RelativeNorth		},
	{ "NE",	RelativeNorthEast	},
	{ "W",	RelativeWest		},
	{ "C",	RelativeCenter		},
	{ "E",	RelativeEast		},
	{ "SW",	RelativeSouthWest	},
	{ "S",	RelativeSouth		},
	{ "SE",	RelativeSouthEast	},
	{ "R",	RelativeStatic		},
	{ "L",	RelativeLast		}
	/* *INDENT-ON* */
};

static KeyItem KeyItemsByDir[] = {
	/* *INDENT-OFF* */
	{ "moveto",	k_moveto,	NULL }, /* arg is position */
	{ "snapto",	k_snapto,	NULL }, /* arg is direction */
	{ "edgeto",	k_edgeto,	NULL }, /* arg is direction */
	{ "moveby",	k_moveby,	NULL }  /* arg is direction and amount */
	/* *INDENT-ON* */
};

static const struct {
	const char *which;
	WhichClient any;
} list_which[] = {
	/* *INDENT-OFF* */
	{ "",		FocusClient	},	/* focusable, same monitor */
	{ "act",	ActiveClient	},	/* activatable (incl. icons), same monitor */
	{ "all",	AllClients	},	/* all clients (incl. icons), same mon */
	{ "any",	AnyClient	},	/* any client on any monitor */
	{ "every",	EveryClient	}	/* all clients, all desktops */
	/* *INDENT-ON* */
};

static const struct {
	const char *suffix;
	RelativeDirection dir;
} lst_suffix[] = {
	/* *INDENT-OFF* */
	{ "",		RelativeNone		},
	{ "icon",	RelativeCenter		},
	{ "next",	RelativeNext		},
	{ "prev",	RelativePrev		},
	{ "last",	RelativeLast		},
	{ "up",		RelativeNorth		},
	{ "down",	RelativeSouth		},
	{ "left",	RelativeWest		},
	{ "right",	RelativeEast		},
	{ "NW",		RelativeNorthWest	},
	{ "N",		RelativeNorth		},
	{ "NE",		RelativeNorthEast	},
	{ "W",		RelativeWest		},
	{ "E",		RelativeEast		},
	{ "SW",		RelativeSouthWest	},
	{ "S",		RelativeSouth		},
	{ "SE",		RelativeSouthEast	}
	/* *INDENT-ON* */
};

static const struct {
	const char *suffix;
	RelativeDirection dir;
} cyc_suffix[] = {
	/* *INDENT-OFF* */
	{ "icon",	RelativeCenter		},
	{ "next",	RelativeNext		},
	{ "prev",	RelativePrev		}
	/* *INDENT-ON* */
};

static KeyItem KeyItemsByList[] = {
	/* *INDENT-OFF* */
	{ "focus",	k_focus,	NULL },
	{ "client",	k_client,	NULL },
	{ "stack",	k_stack,	NULL },
	{ "group",	k_group,	NULL },
	{ "tab",	k_tab,		NULL },
	{ "panel",	k_panel,	NULL },
	{ "dock",	k_dock,		NULL },
	{ "swap",	k_swap,		NULL }
	/* *INDENT-ON* */
};

static const struct {
	const char *suffix;
	RelativeDirection dir;
	Bool wrap;
} tag_suffix[] = {
	/* *INDENT-OFF* */
	{ "",		RelativeNone,		False	},
	{ "next",	RelativeNext,		True	},
	{ "prev",	RelativePrev,		True	},
	{ "last",	RelativeLast,		False	},
	{ "up",		RelativeNorth,		False	},
	{ "down",	RelativeSouth,		False	},
	{ "left",	RelativeWest,		False	},
	{ "right",	RelativeEast,		False	},
	{ "NW",		RelativeNorthWest,	True	},
	{ "N",		RelativeNorth,		True	},
	{ "NE",		RelativeNorthEast,	True	},
	{ "W",		RelativeWest,		True	},
	{ "E",		RelativeEast,		True	},
	{ "SW",		RelativeSouthWest,	True	},
	{ "S",		RelativeSouth,		True	},
	{ "SE",		RelativeSouthEast,	True	}
	/* *INDENT-ON* */
};

static KeyItem KeyItemsByTag[] = {
	/* *INDENT-OFF* */
	{"view",	k_view,		NULL },
	{"toggleview",	k_toggleview,	NULL },
	{"focusview",	k_focusview,	NULL },
	{"tag",		k_tag,		NULL },
	{"toggletag",	k_toggletag,	NULL },
	{"taketo",	k_taketo,	NULL },
	{"sendto",	k_sendto,	NULL }
	/* *INDENT-ON* */
};

static void
initmodkey()
{
	const char *res;

	res = readres("adwm.modkey", "Adwm.Modkey", "A");
	switch (*res) {
	case 'S':
		modkey = ShiftMask;
		break;
	case 'C':
		modkey = ControlMask;
		break;
	case 'W':
		modkey = Mod4Mask;
		break;
	default:
		modkey = Mod1Mask;
	}
}

static char *
capclass(char *clas)
{
	char *p;

	for (p = clas; *p != '\0'; p = strchrnul(p, '.'), p += ((*p == '\0') ? 0 : 1))
		*p = toupper(*p);
	return clas;
}

static void
initkeys_ADWM(Bool reload __attribute__((unused)))
{
	unsigned int i, j, l;
	const char *res;
	char name[256], clas[256];

	xresdb = xkeysdb;

	freekeys();
	initmodkey();
	/* global functions */
	for (i = 0; i < LENGTH(KeyItems); i++) {
		Key key = { 0, };

		snprintf(name, sizeof(name), "%s", KeyItems[i].name);
		snprintf(clas, sizeof(clas), "%s", KeyItems[i].name);
		XPRINTF("Check for key item '%s'\n", name);
		if (!(res = readres(name, capclass(clas), NULL)))
			continue;
		key.func = KeyItems[i].action;
		key.arg = KeyItems[i].arg;
		parsekeys(res, &key);
	}
	/* increment, decrement and set functions */
	for (j = 0; j < LENGTH(KeyItemsByAmt); j++) {
		for (i = 0; i < LENGTH(inc_prefix); i++) {
			Key key = { 0, };

			snprintf(name, sizeof(name), "%s%s", inc_prefix[i].prefix,
				 KeyItemsByAmt[j].name);
			snprintf(clas, sizeof(clas), "%s%s", inc_prefix[i].prefix,
				 KeyItemsByAmt[j].name);
			XPRINTF("Check for key item '%s'\n", name);
			if (!(res = readres(name, capclass(clas), NULL)))
				continue;
			key.func = KeyItemsByAmt[j].action;
			key.arg = NULL;
			key.act = inc_prefix[i].act;
			parsekeys(res, &key);
		}
	}
	/* client or screen state set, unset and toggle functions */
	for (j = 0; j < LENGTH(KeyItemsByState); j++) {
		for (i = 0; i < LENGTH(set_prefix); i++) {
			for (l = 0; l < LENGTH(set_suffix); l++) {
				Key key = { 0, };

				snprintf(name, sizeof(name), "%s%s%s",
					 set_prefix[i].prefix, KeyItemsByState[j].name,
					 set_suffix[l].suffix);
				snprintf(clas, sizeof(clas), "%s%s%s",
					 set_prefix[i].prefix, KeyItemsByState[j].name,
					 set_suffix[l].suffix);
				XPRINTF("Check for key item '%s'\n", name);
				if (!(res = readres(name, capclass(clas), NULL)))
					continue;
				key.func = KeyItemsByState[j].action;
				key.arg = NULL;
				key.set = set_prefix[i].set;
				key.any = set_suffix[l].any;
				parsekeys(res, &key);
			}
		}
	}
	/* functions with a relative direction */
	for (j = 0; j < LENGTH(KeyItemsByDir); j++) {
		for (i = 0; i < LENGTH(rel_suffix); i++) {
			Key key = { 0, };

			snprintf(name, sizeof(name), "%s%s", KeyItemsByDir[j].name,
				 rel_suffix[i].suffix);
			snprintf(clas, sizeof(clas), "%s%s", KeyItemsByDir[j].name,
				 rel_suffix[i].suffix);
			XPRINTF("Check for key item '%s'\n", name);
			if (!(res = readres(name, capclass(clas), NULL)))
				continue;
			key.func = KeyItemsByDir[j].action;
			key.arg = NULL;
			key.dir = rel_suffix[i].dir;
			parsekeys(res, &key);
		}
	}
	/* per tag functions */
	for (j = 0; j < LENGTH(KeyItemsByTag); j++) {
		for (i = 0; i < MAXTAGS; i++) {
			Key key = { 0, };

			snprintf(name, sizeof(name), "%s%d", KeyItemsByTag[j].name, i);
			snprintf(clas, sizeof(clas), "%s%d", KeyItemsByTag[j].name, i);
			XPRINTF("Check for key item '%s'\n", name);
			if (!(res = readres(name, capclass(clas), NULL)))
				continue;
			key.func = KeyItemsByTag[j].action;
			key.arg = NULL;
			key.tag = i;
			key.dir = RelativeNone;
			parsekeys(res, &key);
		}
		for (i = 0; i < LENGTH(tag_suffix); i++) {
			Key key = { 0, };

			snprintf(name, sizeof(name), "%s%s", KeyItemsByTag[j].name,
				 tag_suffix[i].suffix);
			snprintf(clas, sizeof(clas), "%s%s", KeyItemsByTag[j].name,
				 tag_suffix[i].suffix);
			XPRINTF("Check for key item '%s'\n", name);
			if (!(res = readres(name, capclass(clas), NULL)))
				continue;
			key.func = KeyItemsByTag[j].action;
			key.arg = NULL;
			key.tag = 0;
			key.dir = tag_suffix[i].dir;
			key.wrap = tag_suffix[i].wrap;
			parsekeys(res, &key);
		}
	}
	/* list settings */
	for (j = 0; j < LENGTH(KeyItemsByList); j++) {
		for (i = 0; i < 32; i++) {
			Key key = { 0, };

			snprintf(name, sizeof(name), "%s%d", KeyItemsByList[j].name, i);
			snprintf(clas, sizeof(clas), "%s%d", KeyItemsByList[j].name, i);
			XPRINTF("Check for key item '%s'\n", name);
			if (!(res = readres(name, capclass(clas), NULL)))
				continue;
			key.func = KeyItemsByList[j].action;
			key.arg = NULL;
			key.tag = i;
			key.any = AllClients;
			key.dir = RelativeNone;
			key.cyc = False;
			parsekeys(res, &key);
		}
		for (i = 0; i < LENGTH(lst_suffix); i++) {
			for (l = 0; l < LENGTH(list_which); l++) {
				Key key = { 0, };

				snprintf(name, sizeof(name), "%s%s%s",
					 KeyItemsByList[j].name, lst_suffix[i].suffix,
					 list_which[l].which);
				snprintf(clas, sizeof(clas), "%s%s%s",
					 KeyItemsByList[j].name, lst_suffix[i].suffix,
					 list_which[l].which);
				XPRINTF("Check for key item '%s'\n", name);
				if (!(res = readres(name, capclass(clas), NULL)))
					continue;
				key.func = KeyItemsByList[j].action;
				key.arg = NULL;
				key.tag = 0;
				key.any = list_which[l].any;
				key.dir = lst_suffix[i].dir;
				key.cyc = False;
				parsekeys(res, &key);
			}
		}
		for (i = 0; i < LENGTH(cyc_suffix); i++) {
			for (l = 0; l < LENGTH(list_which); l++) {
				Key key = { 0, };

				snprintf(name, sizeof(name), "cycle%s%s%s",
					 KeyItemsByList[j].name, cyc_suffix[i].suffix,
					 list_which[l].which);
				snprintf(clas, sizeof(clas), "cycle%s%s%s",
					 KeyItemsByList[j].name, cyc_suffix[i].suffix,
					 list_which[l].which);
				XPRINTF("Check for key item '%s'\n", name);
				if (!(res = readres(name, capclass(clas), NULL)))
					continue;
				key.func = KeyItemsByList[j].action;
				key.arg = NULL;
				key.tag = 0;
				key.any = list_which[l].any;
				key.dir = cyc_suffix[i].dir;
				key.cyc = True;
				parsekeys(res, &key);
			}
		}
	}
	/* layout setting */
	for (i = 0; layouts[i].symbol != '\0'; i++) {
		Key key = { 0, };

		snprintf(name, sizeof(name), "setlayout%c", layouts[i].symbol);
		snprintf(clas, sizeof(clas), "setlayout%c", layouts[i].symbol);
		XPRINTF("Check for key item '%s'\n", name);
		if (!(res = readres(name, capclass(clas), NULL)))
			continue;
		key.func = k_setlayout;
		key.arg = name + 9;
		parsekeys(res, &key);
	}
	/* spawn */
	for (i = 0; i < 64; i++) {
		Key key = { 0, };

		snprintf(name, sizeof(name), "spawn%d", i);
		snprintf(clas, sizeof(clas), "spawn%d", i);
		XPRINTF("Check for key item '%s'\n", name);
		if (!(res = readres(name, capclass(clas), NULL)))
			continue;
		key.func = k_spawn;
		key.arg = NULL;
		parsekeys(res, &key);
	}
}

/** @brief initlayouts_ADWM: perform per-screen view and layout initialization
  *
  * @{ */
static void
initlayouts_ADWM(Bool reload __attribute__((unused)))
{
	unsigned i, s = scr->screen;

	xresdb = xconfigdb;

	for (i = 0; i < MAXTAGS; i++) {
		const char *res;
		char name[256], clas[256], defl[2];
		Layout *l;
		View *v = scr->views + i;

		snprintf(name, sizeof(name), "adwm.screen%u.tags.layout%u", s, i);
		snprintf(clas, sizeof(clas), "Adwm.Screen%u.Tags.Layout%u", s, i);
		defl[0] = scr->options.deflayout; defl[1] = 0;
		res = readres(name, clas, defl);
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
		v->placement = scr->options.placement;
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
/** @} */

static void
initbuttons_ADWM(AdwmStyle *style, Bool reload __attribute__((unused)))
{
	static const struct {
		const char *name;
		const char *clas;
		const char *def;
	} elements[LastElement] = {
		/* *INDENT-OFF* */
		[MenuBtn]	= { "button.menu",	MENUPIXMAP,	NULL },
		[IconifyBtn]	= { "button.iconify",	ICONPIXMAP,	NULL },
		[MaximizeBtn]	= { "button.maximize",	MAXPIXMAP,	NULL },
		[CloseBtn]	= { "button.close",	CLOSEPIXMAP,	NULL },
		[ShadeBtn]	= { "button.shade",	SHADEPIXMAP,	NULL },
		[StickBtn]	= { "button.stick",	STICKPIXMAP,	NULL },
		[LHalfBtn]	= { "button.lhalf",	LHALFPIXMAP,	NULL },
		[RHalfBtn]	= { "button.rhalf",	RHALFPIXMAP,	NULL },
		[FillBtn]	= { "button.fill",	FILLPIXMAP,	NULL },
		[FloatBtn]	= { "button.float",	FLOATPIXMAP,	NULL },
		[SizeBtn]	= { "button.resize",	SIZEPIXMAP,	NULL },
		[IconBtn]	= { "button.icon",	WINPIXMAP,	NULL },
		[TitleTags]	= { "title.tags",	NULL,		NULL },
		[TitleName]	= { "title.name",	NULL,		NULL },
		[TitleSep]	= { "title.sep",	NULL,		NULL },
		/* *INDENT-ON* */
	};
	static const struct {
		const char *name;
		const char *clas;
	} kind[LastButtonImageType] = {
		/* *INDENT-OFF* */
		[ButtonImageDefault]		    = { "",				""			    },
		[ButtonImagePressed]		    = { ".pressed",			".Pressed"		    },
		[ButtonImagePressedB1]		    = { ".pressed.b1",			".Pressed.B1"		    },
		[ButtonImagePressedB2]		    = { ".pressed.b2",			".Pressed.B2"		    },
		[ButtonImagePressedB3]		    = { ".pressed.b3",			".Pressed.B3"		    },
		[ButtonImageToggledPressed]	    = { ".toggled.pressed",		".Toggled.Pressed"	    },
		[ButtonImageToggledPressedB1]	    = { ".toggled.pressed.b1",		".Toggled.Pressed.B1"	    },
		[ButtonImageToggledPressedB2]	    = { ".toggled.pressed.b2",		".Toggled.Pressed.B2"	    },
		[ButtonImageToggledPressedB3]	    = { ".toggled.pressed.b3",		".Toggled.Pressed.B3"	    },
		[ButtonImageHover]		    = { ".hovered",			".Hovered"		    },
		[ButtonImageFocus]		    = { ".focused",			".Focused"		    },
		[ButtonImageUnfocus]		    = { ".unfocus",			".Unfocus"		    },
		[ButtonImageToggledHover]	    = { ".toggled.hovered",		".Toggled.Hovered"	    },
		[ButtonImageToggledFocus]	    = { ".toggled.focused",		".Toggled.Focused"	    },
		[ButtonImageToggledUnfocus]	    = { ".toggled.unfocus",		".Toggled.Unfocus"	    },
		[ButtonImageDisabledHover]	    = { ".disabled.hovered",		".Disabled.Hovered"	    },
		[ButtonImageDisabledFocus]	    = { ".disabled.focused",		".Disabled.Focused"	    },
		[ButtonImageDisabledUnfocus]	    = { ".disabled.unfocus",		".Disabled.Unfocus"	    },
		[ButtonImageToggledDisabledHover]   = { ".toggled.disabled.hovered",	".Toggled.Disabled.Hovered" },
		[ButtonImageToggledDisabledFocus]   = { ".toggled.disabled.focused",	".Toggled.Disabled.Focused" },
		[ButtonImageToggledDisabledUnfocus] = { ".toggled.disabled.unfocus",	".Toggled.Disabled.Unfocus" },
		/* *INDENT-ON* */
	};
	int i, j, n;
	char name[256], clas[256];

	n = LastElement * LastButtonImageType;
	free(style->elements);
	style->elements = calloc(n, sizeof(*style->elements));

	for (n = 0, i = 0; i < LastElement; i++) {
		for (j = 0; j < LastButtonImageType; j++, n++) {
			snprintf(name, sizeof(name), "adwm.style.%s%s",
				 elements[i].name, kind[j].name);
			snprintf(clas, sizeof(clas), "Adwm.Style.%s%s",
				 elements[i].clas, kind[j].clas);
			readtexture(name, clas, &style->elements[n], "black",
				    "white");
			if (!style->elements[n].pixmap.file && elements[i].def)
				getpixmap(elements[i].def,
					  &style->elements[i].pixmap);
		}
	}
}

static void
initstyle_ADWM(Bool reload)
{
	const char *res;
	AdwmStyle *style;

	xresdb = xstyledb;

	if (!styles)
		styles = ecalloc(nscr, sizeof(*styles));
	style = styles + scr->screen;

	res = readres("adwm.style.normal.border", "Adwm.Style.Normal.Border",
		      NORMBORDERCOLOR);
	getxcolor(res, NORMBORDERCOLOR, &style->normal.color.border);
	res = readres("adwm.style.normal.bg", "Adwm.Style.Normal.Bg", NORMBGCOLOR);
	getxcolor(res, NORMBGCOLOR, &style->normal.color.bg);
	res = readres("adwm.style.normal.fg", "Adwm.Style.Normal.Fg", NORMFGCOLOR);
	getxcolor(res, NORMFGCOLOR, &style->normal.color.fg);
	res = readres("adwm.style.normal.button", "Adwm.Style.Normal.Button",
		      NORMBUTTONCOLOR);
	getxcolor(res, NORMBUTTONCOLOR, &style->normal.color.button);

	res = readres("adwm.style.selected.border", "Adwm.Style.Selected.Border",
		      SELBORDERCOLOR);
	getxcolor(res, SELBORDERCOLOR, &style->selected.color.border);
	res = readres("adwm.style.selected.bg", "Adwm.Style.Selected.Bg", SELBGCOLOR);
	getxcolor(res, SELBGCOLOR, &style->selected.color.bg);
	res = readres("adwm.style.selected.fg", "Adwm.Style.Selected.Fg", SELFGCOLOR);
	getxcolor(res, SELFGCOLOR, &style->selected.color.fg);
	res = readres("adwm.style.selected.button", "Adwm.Style.Selected.Button",
		      SELBUTTONCOLOR);
	getxcolor(res, SELBUTTONCOLOR, &style->selected.color.button);

	res = readres("adwm.style.normal.font", "Adwm.Style.Normal.Font", FONT);
	getfont(res, FONT, &style->normal.font);
	res = readres("adwm.style.normal.fg", "Adwm.Style.Normal.Fg", NORMFGCOLOR);
	getxftcolor(res, NORMFGCOLOR, &style->normal.color.font);
	res = readres("adwm.style.normal.drop", "Adwm.Style.Normal.Drop", "0");
	style->normal.drop = strtoul(res, NULL, 0);
	res = readres("adwm.style.normal.shadow", "Adwm.Style.Normal.Shadow",
		      NORMBORDERCOLOR);
	getxftcolor(res, NORMBORDERCOLOR, &style->normal.color.shadow);

	res = readres("adwm.style.selected.font", "Adwm.Style.Selected.Font", FONT);
	getfont(res, FONT, &style->selected.font);
	res = readres("adwm.style.selected.fg", "Adwm.Style.Selected.Fg", SELFGCOLOR);
	getxftcolor(res, SELFGCOLOR, &style->selected.color.font);
	res = readres("adwm.style.selected.drop", "Adwm.Style.Selected.Drop", "0");
	style->selected.drop = strtoul(res, NULL, 0);
	res = readres("adwm.style.selected.shadow", "Adwm.Style.Selected.Shadow",
		      SELBORDERCOLOR);
	getxftcolor(res, SELBORDERCOLOR, &style->selected.color.shadow);

	res = readres("adwm.style.border", "Adwm.Style.Border", STR(BORDERPX));
	style->border = strtoul(res, NULL, 0);
	res = readres("adwm.style.margin", "Adwm.Style.Margin", STR(MARGINPX));
	style->margin = strtoul(res, NULL, 0);
	res = readres("adwm.style.opacity", "Adwm.Style.Opacity", STR(NF_OPACITY));
	style->opacity = strtod(res, NULL) * OPAQUE;
	getbool("adwm.style.outline", "Adwm.Style.Outline", "1", False, &style->outline);
	res = readres("adwm.style.spacing", "Adwm.Style.Spacing", "1");
	style->spacing = strtoul(res, NULL, 0);
	res = readres("adwm.style.titlelayout", "Adwm.Style.TitleLayout", "N  IMC");
	style->titlelayout = res;
	res = readres("adwm.style.grips", "Adwm.Style.Grips", STR(GRIPHEIGHT));
	style->grips = strtoul(res, NULL, 0);
	res = readres("adwm.style.title", "Adwm.Style.Title", STR(TITLEHEIGHT));
	style->title = strtoul(res, NULL, 0);

	initbuttons_ADWM(style, reload);
}

static void
inittheme_ADWM(Bool reload __attribute__((unused)))
{
	const char *res;
	AdwmTheme *theme;

	xresdb = xthemedb;

	if (!themes)
		themes = ecalloc(nscr, sizeof(*themes));
	theme = themes + scr->screen;
	(void) theme;
	(void) res;

	/* FIXME: read theme elements */

}

static void
deinitstyle_ADWM(void)
{
}

static void
drawclient_ADWM(Client *c)
{
	(void) c;
}

AdwmOperations adwm_ops = {
	.name = "adwm",
	.clas = "Adwm",
	.initrcfile = &initrcfile_ADWM,
	.initconfig = &initconfig_ADWM,
	.initscreen = &initscreen_ADWM,
	.inittags = &inittags_ADWM,
	.initkeys = &initkeys_ADWM,
	.initlayouts = &initlayouts_ADWM,
	.initstyle = &initstyle_ADWM,
	.inittheme = &inittheme_ADWM,
	.deinitstyle = &deinitstyle_ADWM,
	.drawclient = &drawclient_ADWM,
};

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
