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
#include "config.h"
#include "icons.h" /* verification */

char **xdgs = NULL;
char **dirs = NULL;
char **exts = NULL;

typedef struct IconTheme IconTheme;
typedef struct IconDirectory IconDirectory;

struct IconDirectory {
	IconTheme *theme;
	IconDirectory *next;
	char *directory;
	int size;
	int minsize;
	int maxsize;
	int threshold;
	int scale;
	char *context;
	char *type;
};

struct IconTheme {
	IconTheme *next;
	char *name;
	char *path;
	char *inherits;
	char *dnames;
	char *snames;
	IconDirectory *dirs;
};

IconTheme *themes = NULL;

Bool
DirectoryMatchesSize(IconTheme *theme, IconDirectory *subdir, int iconsize)
{
	if (!subdir->minsize)
		subdir->minsize = subdir->size;
	if (!subdir->maxsize)
		subdir->maxsize = subdir->size;
	if (!subdir->threshold)
		subdir->threshold = 2;
	if (!strcmp(subdir->type, "Fixed"))
		return (subdir->size == iconsize);
	if (!strcmp(subdir->type, "Scaled"))
		return (subdir->minsize <= iconsize && iconsize <= subdir->maxsize);
	if (!strcmp(subdir->type, "Threshold"))
		return ((subdir->size - subdir->threshold) <= iconsize &&
				iconsize <= (subdir->size + subdir->threshold));
	return (False);
}

int
DirectorySizeDistance(IconTheme *theme, IconDirectory *subdir, int iconsize)
{
	if (!subdir->minsize)
		subdir->minsize = subdir->size;
	if (!subdir->maxsize)
		subdir->maxsize = subdir->size;
	if (!subdir->threshold)
		subdir->threshold = 2;
	if (!strcmp(subdir->type, "Fixed")) {
		return abs(subdir->size - iconsize);
	} else
	if (!strcmp(subdir->type, "Scaled")) {
		if (iconsize < subdir->minsize)
			return (subdir->minsize - iconsize);
		if (iconsize > subdir->maxsize)
			return (iconsize - subdir->maxsize);
		return (0);
	} else
	if (!strcmp(subdir->type, "Threshold")) {
		if (iconsize < (subdir->size - subdir->threshold))
			return (subdir->minsize - iconsize);
		if (iconsize > (subdir->size + subdir->threshold))
			return (iconsize - subdir->maxsize);
		return (0);
	}
	return (0);
}

char *
_LookupIcon(const char *iconname, int size, IconTheme *theme, char **fexts)
{
	IconDirectory *id;
	char **ext, **dir;
	static char buf[PATH_MAX + 1] = { 0, };

	if (!fexts)
		fexts = exts;
	for (id = theme->dirs; id; id = id->next) {
		if (DirectoryMatchesSize(theme, id, size)) {
			for (dir = dirs; dir && *dir; dir++) {
				for (ext = exts; ext && *ext; ext++) {
					snprintf(buf, sizeof(buf), "%s/%s/%s/%s.%s", *dir, theme->name, id->directory, iconname, *ext);
					if (!access(buf, R_OK))
						return (strdup(buf));
				}
			}
		}
	}
	int minimal_size = 0x7fffffff, dist;
	char *closest = NULL;

	for (id = theme->dirs; id; id = id->next) {
		for (dir = dirs; dir && *dir; dir++) {
			for (ext = exts; ext && *ext; ext++) {
				snprintf(buf, sizeof(buf), "%s/%s/%s/%s.%s", *dir, theme->name, id->directory, iconname, *ext);
				if (!access(buf, R_OK) && (dist = DirectorySizeDistance(theme, id, size)) < minimal_size) {
					free(closest);
					closest = strdup(buf);
					minimal_size = dist;
				}
			}
		}
	}
	if (closest)
		return (closest);
	return (NULL);
}

char *
_FindBestIconHelper(const char **iconlist, int size, const char *name, char **fexts)
{
	IconTheme *theme;
	const char **icon;
	char *file;


	for (theme = themes; theme; theme = theme->next)
		if (!strcmp(theme->name, name))
			break;
	if (!theme)
		return (NULL);
	for (icon = iconlist; icon && *icon; icon++)
		if ((file = _LookupIcon(*icon, size, theme, fexts)))
			return (file);
	if (theme->inherits)
		if ((file = _FindBestIconHelper(iconlist, size, theme->inherits, fexts)))
			return (file);
	return (NULL);
}

char *
_LookupFallbackIcon(const char *iconname, char **fexts)
{
	char **dir, **ext;
	static char buf[PATH_MAX + 1] = { 0, };

	if (!fexts)
		fexts = exts;
	for (dir = dirs; dir && *dir; dir++) {
		for (ext = exts; ext && *ext; ext++) {
			snprintf(buf, sizeof(buf), "%s/%s.%s", *dir, iconname, *ext);
			if (!access(buf, R_OK)) {
				return strdup(buf);
			}
		}
	}
	return (NULL);
}

char *
FindBestIcon(const char **iconlist, int size, char **fexts)
{
	char *file;
	const char **icon;

	if ((file = _FindBestIconHelper(iconlist, size, options.icontheme, fexts)))
		return (file);
	if ((file = _FindBestIconHelper(iconlist, size, "hicolor", fexts)))
		return (file);

	for (icon = iconlist; icon && *icon; icon++)
		if ((file = _LookupFallbackIcon(*icon, fexts)))
			return (file);
	return (NULL);
}

char *
_FindIconHelper(const char *icon, int size, const char *name, char **fexts)
{
	IconTheme *theme;
	char *file;

	for (theme = themes; theme; theme = theme->next)
		if (!strcmp(theme->name, name))
			break;
	if (!theme)
		return (NULL);
	if ((file = _LookupIcon(icon, size, theme, fexts)))
		return (file);
	if (theme->inherits)
		if ((file = _FindIconHelper(icon, size, theme->inherits, fexts)))
			return (file);
	return (NULL);
}

char *
FindIcon(const char *icon, int size, char **fexts)
{
	char *file;

	if ((file = _FindIconHelper(icon, size, options.icontheme, fexts)))
		return (file);
	if ((file = _FindIconHelper(icon, size, "hicolor", fexts)))
		return (file);
	return _LookupFallbackIcon(icon, fexts);
}

IconDirectory *
allocicondir(IconTheme *theme, char *subdir)
{
	IconDirectory *dir;

	dir = ecalloc(1, sizeof(*dir));
	dir->theme = theme;
	dir->directory = subdir;
	dir->next = theme->dirs;
	theme->dirs = dir;
	return (dir);
}

void
freeicontheme(IconTheme *it)
{
	IconDirectory *dir;

	while ((dir = it->dirs)) {
		it->dirs = dir->next;
		free(dir->directory);
		free(dir->context);
		free(dir->type);
		free(dir);
	}
	free(it->name);
	free(it->path);
	free(it->inherits);
	free(it->dnames);
	free(it->snames);
	free(it);
}

void
freeiconthemes(void)
{
	IconTheme *it;

	while ((it = themes)) {
		themes = it->next;
		it->next = NULL;
		freeicontheme(it);
	}
}

void
removeicontheme(IconTheme *it)
{
	IconTheme **pit;

	for (pit = &themes; *pit && *pit != it; pit = &(*pit)->next) ;
	if (*pit) {
		*pit = it->next;
		it->next = NULL;
	}
	freeicontheme(it);
}

IconTheme *
allocicontheme(char *name, char *path)
{
	IconTheme *it;

	/* We go backward through directories, so if we have a duplicate we want to
	   overwrite it with the new directory data of the same name. */

	for (it = themes; it; it = it->next) {
		if (!strcmp(name, it->name)) {
			removeicontheme(it);
			break;
		}
	}

	it = ecalloc(1, sizeof(*it));
	it->name = name;
	it->path = path;
	it->next = themes;
	it->dirs = NULL;
	it->dnames = NULL;
	themes = it;
	return (it);
}

void
newicontheme(const char *name, const char *path)
{
	FILE *file;

	if ((file = fopen(path, "r"))) {
		IconTheme *it;
		IconDirectory *dir = NULL;
		size_t len;
		char *p, *q;
		Bool parsed_theme = False;
		Bool parsing_theme = False;
		Bool parsing_dir = False;
		char *key, *val;
		char *section = NULL;
		static char buf[BUFSIZ + 1] = { 0, };

		it = allocicontheme(strdup(name), strdup(path));

		while ((p = fgets(buf, sizeof(buf), file))) {
			if ((q = strchr(p, '\r')))
				*q = '\0';
			if ((q = strchr(p, '\n')))
				*q = '\0';
			if (*p == '[' && (q = strchr(p, ']'))) {
				*q = '\0';
				free(section);
				section = strdup(p + 1);
				if (!strcmp(p + 1, "Icon Theme")) {
					if (!parsed_theme)
						parsing_theme = True;
					parsing_dir = False;
				} else if ((it->dnames && (p = strstr(it->dnames, section))
					    && (q = p + strlen(section)) && *(p - 1) == ',' && *q == ',') ||
				           (it->snames && (p = strstr(it->snames, section))
					    && (q = p + strlen(section)) && *(p - 1) == ',' && *q == ',')) {
					if (parsing_theme) {
						parsed_theme = True;
						parsing_theme = False;
					}
					parsing_dir = True;
					dir = allocicondir(it, section);
					section = NULL;
				} else
					EPRINTF("do not now what to do with section %s in icon.theme %s\n", section, path);

			} else if (parsing_theme) {
				if ((key = p) && (val = strchr(key, '=')) && !strchr(key, '[')) {
					*val++ = '\0';
					if (!strcmp(key, "Directories")) {
						len = strlen(val) + 3;
						it->dnames = ecalloc(len + 1, sizeof(*it->dnames));
						strncpy(it->dnames, ",", len);
						strncat(it->dnames, val, len);
						strncat(it->dnames, ",", len);
					} else if (!strcmp(key, "ScaledDirectories")) {
						len = strlen(val) + 3;
						it->snames = ecalloc(len + 1, sizeof(*it->snames));
						strncpy(it->snames, ",", len);
						strncat(it->snames, val, len);
						strncat(it->snames, ",", len);
					} else if (!strcmp(key, "Inherits"))
						it->inherits = strdup(val);
					else
						EPRINTF("do not know what to do with key %s in %s\n", key, section);
				}
			} else if (parsing_dir) {
				if ((key = p) && (val = strchr(key, '=')) && !strchr(key, '[')) {
					*val++ = '\0';
					if (!strcmp(key, "Size"))
						dir->size = atoi(val);
					else if (!strcmp(key, "Scale"))
						dir->scale = atoi(val);
					else if (!strcmp(key, "Context"))
						dir->context = strdup(val);
					else if (!strcmp(key, "Type"))
						dir->type = strdup(val);
					else if (!strcmp(key, "MinSize"))
						dir->minsize = atoi(val);
					else if (!strcmp(key, "MaxSize"))
						dir->maxsize = atoi(val);
					else if (!strcmp(key, "Threshold"))
						dir->threshold = atoi(val);
					else
						EPRINTF("do not know what to do with key %s in %s\n", key, section);
				}
			}
		}
		fclose(file);
	}
}

void
rescanicons(void)
{
	char **xdg;
	DIR *dir;

	freeiconthemes();

	for (xdg = xdgs; xdg && *xdg; xdg++) ;
	for (xdg--; xdg >= xdgs; xdg--) {
		if ((dir = opendir(*xdg))) {
			struct dirent *d;
			static char path[PATH_MAX + 1] = { 0, };

			while ((d = readdir(dir))) {
				if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
					continue;
				snprintf(path, PATH_MAX, "%s/%s/index.theme", *xdg, d->d_name);
				if (!access(path, R_OK))
					newicontheme(d->d_name, path);
			}
			closedir(dir);
		}
	}
}

void
initicons(void)
{
	const char *p, *q, *env;
	char *l, *e;
	int i = 0, j = 0, k = 0;
	size_t len;
	const char *home = getenv("HOME") ? : "~";

	for (p = options.prependdirs; p && (q = strchrnul(p, ':')); p = q + 1) {
		dirs = reallocarray(dirs, i + 2, sizeof(*dirs));
		dirs[i + 1] = NULL;
		dirs[i] = strndup(p, q - p);
		i++;
		if (!*q)
			break;
	}
	{
		dirs = reallocarray(dirs, i + 2, sizeof(*dirs));
		dirs[i + 1] = NULL;
		len = strlen(home) + strlen("/.icons") + 1;
		dirs[i] = ecalloc(len + 1, sizeof(*dirs[i]));
		strncpy(dirs[i], home, len);
		strncat(dirs[i], "/.icons", len);
		i++;
	}
	if ((env = getenv("XDG_DATA_HOME"))) {
		xdgs = reallocarray(xdgs, j + 2, sizeof(*xdgs));
		xdgs[j + 1] = NULL;
		xdgs[j] = strdup(env);
		j++;
	} else {
		xdgs = reallocarray(xdgs, j + 2, sizeof(*xdgs));
		xdgs[j + 1] = NULL;
		len = strlen(home) + strlen("/.local/share") + 1;
		xdgs[j] = ecalloc(len + 1, sizeof(*xdgs[j]));
		strncpy(xdgs[j], home, len);
		strncat(xdgs[j], "/.local/share", len);
		j++;
	}
	env = getenv("XDG_DATA_DIRS") ? : "/usr/local/share:/usr/share";
	for (p = env; (q = strchrnul(p, ':')); p = q + 1) {
		xdgs = reallocarray(xdgs, j + 2, sizeof(*xdgs));
		xdgs[j + 1] = NULL;
		xdgs[j] = strndup(p, q - p);
		j++;
		if (!*q)
			break;
	}
	if ((env = getenv("XDG_DATA_HOME"))) {
		dirs = reallocarray(dirs, i + 2, sizeof(*dirs));
		dirs[i + 1] = NULL;
		len = strlen(env) + strlen("/icons") + 1;
		dirs[i] = ecalloc(len + 1, sizeof(*dirs[i]));
		strncpy(dirs[i], env, len);
		strncat(dirs[i], "/icons", len);
		i++;
	} else {
		dirs = reallocarray(dirs, i + 2, sizeof(*dirs));
		dirs[i + 1] = NULL;
		len = strlen(home) + strlen("/.local/share/icons") + 1;
		dirs[i] = ecalloc(len + 1, sizeof(*dirs[i]));
		strncpy(dirs[i], home, len);
		strncpy(dirs[i], home, len);
		strncat(dirs[i], "/.local/share/icons", len);
		i++;
	}
	env = getenv("XDG_DATA_DIRS") ? : "/usr/local/share:/usr/share";
	for (p = env; (q = strchrnul(p, ':')); p = q + 1) {
		dirs = reallocarray(dirs, i + 2, sizeof(*dirs));
		dirs[i + 1] = NULL;
		len = (q - p) + strlen("/icons") + 1;
		dirs[i] = ecalloc(len + 1, sizeof(*dirs[i]));
		strncpy(dirs[i], p, q - p);
		strncat(dirs[i], "/icons", len);
		i++;
		if (!*q)
			break;
	}
	if ((env = getenv("XDG_DATA_HOME"))) {
		dirs = reallocarray(dirs, i + 2, sizeof(*dirs));
		dirs[i + 1] = NULL;
		len = strlen(env) + strlen("/pixmaps") + 1;
		dirs[i] = ecalloc(len + 1, sizeof(*dirs[i]));
		strncpy(dirs[i], env, len);
		strncat(dirs[i], "/pixmaps", len);
		i++;
	} else {
		dirs = reallocarray(dirs, i + 2, sizeof(*dirs));
		dirs[i + 1] = NULL;
		len = strlen(home) + strlen("/.local/share/pixmaps") + 1;
		dirs[i] = ecalloc(len + 1, sizeof(*dirs[i]));
		strncpy(dirs[i], home, len);
		strncat(dirs[i], "/.local/share/pixmaps", len);
		i++;
	}
	env = getenv("XDG_DATA_DIRS") ? : "/usr/local/share:/usr/share";
	for (p = env; (q = strchrnul(p, ':')); p = q + 1) {
		dirs = reallocarray(dirs, i + 2, sizeof(*dirs));
		dirs[i + 1] = NULL;
		len = (q - p) + strlen("/pixmaps") + 1;
		dirs[i] = ecalloc(len + 1, sizeof(*dirs[i]));
		strncpy(dirs[i], p, (q - p));
		strncat(dirs[i], "/pixmaps", len);
		i++;
		if (!*q)
			break;
	}
	for (p = options.appenddirs; p && (q = strchrnul(p, ':')); p = q + 1) {
		dirs = reallocarray(dirs, i + 2, sizeof(*dirs));
		dirs[i + 1] = NULL;
		dirs[i] = strndup(p, q - p);
		i++;
		if (!*q)
			break;
	}
	for (p = options.extensions; p && (q = strchrnul(p, ',')); p = q + 1) {
		exts = reallocarray(exts, k + 2, sizeof(*exts));
		exts[k + 1] = NULL;
		exts[k] = strndup(p, q - p);
		k++;
		if (!*q)
			break;
	}
	if (!options.icontheme) {
		if ((env = getenv("XDG_ICON_THEME")))
			options.icontheme = strdup(env);
		else {
			static char buf[BUFSIZ + 1] = { 0, };
			l = NULL;
			strncpy(buf, home, BUFSIZ);
			strncat(buf, "/.gtkrc-2.0.xde", BUFSIZ);
			if (!access(buf, R_OK))
				l = buf;
			else {
				strncpy(buf, home, BUFSIZ);
				strncat(buf, "/.gtkrc-2.0", BUFSIZ);
				if (!access(buf, R_OK))
					l = buf;
			}
			if (l) {
				FILE *file;

				if ((file = fopen(l, "r"))) {
					while ((l = fgets(buf, sizeof(buf), file))) {
						if ((e = strchr(l, '\r')))
							*e = '\0';
						if ((e = strchr(l, '\n')))
							*e = '\0';
						if (!strncmp(buf, "gtk-icon-theme-name=", 20)) {
							l = buf + 20;
							if (*l == '"')
								l++;
							e = strchrnul(l, '"');
							options.icontheme = strndup(l, e - l);
						}
					}
					fclose(file);
				}
			}

		}
		if (!options.icontheme)
			options.icontheme = strdup("hicolor");
	}
	rescanicons();
}
