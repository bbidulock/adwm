/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "resource.h"
#include "config.h"
#include "icons.h" /* verification */

extern AdwmPlaces config;

char **xdgs = NULL;
char **dirs = NULL;
char **exts = NULL;

typedef struct IconDirectory IconDirectory;
typedef struct IconTheme IconTheme;
typedef struct IconThemeName IconThemeName;

struct IconDirectory {
	IconTheme *theme;
	IconDirectory *next;
	char *subdir;
	int size;
	int scale;
	char *context;
	char *type;
	int maxsize;
	int minsize;
	int threshold;
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

struct IconThemeName {
	IconThemeName *next;
	char *name;
};

IconTheme *themes = NULL;
IconThemeName *themenames = NULL;

void
pushthemename(const char *name)
{
	IconThemeName *itn;

	OPRINTF("adding theme %s to scan list\n", name);
	itn = ecalloc(1, sizeof(*itn));
	itn->name = strdup(name);
	itn->next = themenames;
	themenames = itn;
}

char *
popthemename(void)
{
	IconThemeName *itn;
	char *name = NULL;

	if ((itn = themenames)) {
		themenames = itn->next;
		name = itn->name;
		free(itn);
	}
	return (name);
}

static char **
freestringlist(char **list)
{
	char **str;

	for (str = list; str && *str; str++)
		free(*str);
	free(list);
	return (NULL);
}

static char *
_FindAnyIconHelper(char **files, const char *iconname, int size, const char *ext)
{
	char **file, **good = NULL, *p, **maybe, *best = NULL;
	int n = 0;

	for (file = files; file && *file; file++) {
		if (!strncmp(*file, iconname, strlen(iconname))
		    && (p = strrchr(*file, '.'))
		    && !strcmp(p + 1, ext)) {
			good = reallocarray(good, n + 2, sizeof(*good));
			good[n++] = strdup(*file);
			good[n] = NULL;
		}
	}
	if (n > 0)
		goto found;
	/* try without attention to case */
	for (file = files; file && *file; file++) {
		if (!strncasecmp(*file, iconname, strlen(iconname))
		    && (p = strrchr(*file, '.'))
		    && !strcasecmp(p + 1, ext)) {
			good = reallocarray(good, n + 2, sizeof(*good));
			good[n++] = strdup(*file);
			good[n] = NULL;
		}
	}
      found:
	if (!good)
		return (NULL);
	if (n == 1)
		best = good[0];
#if defined IMLIB2 && defined USE_IMLIB2
	if (!best) {
		int hdiff = 0x7fffffff;

		imlib_context_push(scr->context);
		for (maybe = good; maybe && *maybe; maybe++) {
			Imlib_Image image;

			if ((image = imliib_load_image(*maybe))) {
				int h, d;
				imlib_context_set_image(image);
				h = imlib_image_get_height();
				d = abs(size - h);
				if (d < hdiff) {
					hdiff = d;
					best = *maybe;
				}
				imlib_free_image();
			}
		}
		imlib_context_pop();
	}
#else
#if defined PIXBUF && defined USE_PIXBUF
	if (!best) {
		int hdiff = 0x7fffffff;

		for (maybe = good; maybe && *maybe; maybe++) {
			GdkPixbufFormat *format;
			int w = 0, h = 0;

			if ((format = gdk_pixbuf_get_file_info(*maybe, &w, &h))) {
				int d = abs(size - h);
				if (d < hdiff) {
					hdiff = d;
					best = *maybe;
				}
			}
		}
	}
#else
	if (!best) {
		if (!strcmp(ext, "xpm")) {
#ifdef XPM
			int hdiff = 0x7fffffff;

			for (maybe = good; maybe && *maybe; maybe++) {
				XImage *xicon = NULL, *xmask = NULL;
				XpmAttributes xa = { 0, };

				if (XpmReadFileToImage(dpy, *maybe, &xicon, &xmask, &xa) == XpmSuccess) {
					int h = xa.height, d = abs(size -h);
					if (d < hdiff) {
						hdiff = d;
						best = *maybe;
					}
					if (xicon)
						XDestroyImage(xicon);
					if (xmask)
						XDestroyImage(xmask);
					XpmFreeAttributes(&xa);
				}
			}
#endif
		} else
		if (!strcmp(ext, "png")) {
#ifdef LIBPNG
			int hdiff = 0x7fffffff;

			for (maybe = good; maybe && *maybe; maybe++) {
				FILE *f;

				if ((f = fopen(*maybe, "rb"))) {
					png_structp png_ptr;

					if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) {
						png_infop info_ptr;

						if ((info_ptr = png_create_info_struct(png_ptr))) {
							if (setjmp(png_jmpbuf(png_ptr)) == 0) {
								png_uint_32 width, height;
								int dummy = 0;

								png_init_io(png_ptr, f);
								png_read_info(png_ptr, info_ptr);
								if (png_get_IHDR(png_ptr, info_ptr, &width, &height, &dummy, &dummy, &dummy, &dummy, &dummy)) {
									if (height < 0x7fffffff) {
										int h = height, d = abs(h - size);
										
										if (d < hdiff) {
											hdiff = d;
											best = *maybe;
										}
									}
								} else
									png_error(png_ptr, "png_get_IHDR failed");
							}
							png_destroy_info_struct(png_ptr, &info_ptr);
						}
						png_destroy_read_struct(&png_ptr, NULL, NULL);
					}
					fclose(f);
				}
			}
#endif
		}
	}
#endif
#endif
	if (!best) {
		off_t max = 0;

		/* just pick the bigest (st_size) one */
		for (maybe = good; maybe && *maybe; maybe++) {
			struct stat st;

			if (stat(*maybe, &st))
				continue;
			if (st.st_size > max) {
				max = st.st_size;
				best = *maybe;
			}
		}
	}
	if (!best)
		/* just pick one! */
		best = good[0];

	p = strdup(best);
	good = freestringlist(good);
	return (p);
}

static char *
_LookupIconInDirectory(const char *directory, const char *iconname, int size, const char *ext)
{
	DIR *dir;
	char *file = NULL;
	int nr = 0, nd = 0;
	char **s_reg = NULL, **s_dir = NULL, **search;

	if ((dir = opendir(directory))) {
		struct dirent *d;
		char path[PATH_MAX + 1] = { 0, };
		struct stat st = { 0, };

		while ((d = readdir(dir))) {
			if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
				continue;
			strncpy(path, directory, PATH_MAX);
			strncat(path, "/", PATH_MAX);
			strncat(path, d->d_name, PATH_MAX);
			if (stat(path, &st))
				continue;
			if (S_ISDIR(st.st_mode)) {
				s_dir = reallocarray(s_dir, nd + 2, sizeof(*s_dir));
				s_dir[nd] = ecalloc(1, sizeof(*s_dir[nd]));
				s_dir[nd++] = strdup(path);
				s_dir[nd] = NULL;
				continue;
			}
			if (S_ISREG(st.st_mode) && !access(path, R_OK)) {
				s_reg = reallocarray(s_reg, nr + 2, sizeof(*s_reg));
				s_reg[nr] = ecalloc(1, sizeof(*s_reg[nr]));
				s_reg[nr++] = strdup(path);
				s_reg[nr] = NULL;
				continue;
			}
		}
		closedir(dir);
		/* search files in this directory first */
		file = _FindAnyIconHelper(s_reg, iconname, size, ext);
		s_reg = freestringlist(s_reg);
		if (!file)
			/* search files in subdirectories next (no specific order) */
			for (search = s_dir; search && *search; search++)
				if ((file = _LookupIconInDirectory(*search, iconname, size, ext)))
					break;
		s_dir = freestringlist(s_dir);
	}
	return (file);
}

char *
_LookupAnyIcon(const char *iconname, int size, const char **fexts)
{
	char **search;

	/* look in some other places */
	if (!fexts)
		fexts = (const char **) exts;
	for (search = dirs; search && *search; search++) {
		const char **ext;

		for (ext = fexts; ext && *ext; ext++) {
			char *file;

			if ((file = _LookupIconInDirectory(*search, iconname, size, *ext)))
				return (file);
		}
	}
	return (NULL);
}

static Bool
DirectoryMatchesSize(IconDirectory *subdir, int iconsize)
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

static int
DirectorySizeDistance(IconDirectory *subdir, int iconsize)
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

static char *
_LookupIcon(const char *iconname, int size, IconTheme *theme, const char **fexts)
{
	IconDirectory *id;
	char **ext, **xdg, *p;
	char buf[PATH_MAX + 1] = { 0, };

	if (!fexts)
		fexts = (const char **) exts;
	for (id = theme->dirs; id; id = id->next) {
		if (DirectoryMatchesSize(id, size)) {
			for (xdg = xdgs; xdg && *xdg; xdg++) {
				for (ext = exts; ext && *ext; ext++) {
					snprintf(buf, sizeof(buf), "%s/%s/%s/%s.%s", *xdg, theme->name, id->subdir, iconname, *ext);
					if (!access(buf, R_OK))
						return (strdup(buf));
					/* try lower-casing the file name */
					for (p = strrchr(buf, '/'); p && *p; p++)
						*p = tolower(*p);
					if (!access(buf, R_OK))
						return (strdup(buf));
				}
			}
		}
	}
	int minimal_size = 0x7fffffff, dist, missing;
	char *closest = NULL;

	for (id = theme->dirs; id; id = id->next) {
		for (xdg = xdgs; xdg && *xdg; xdg++) {
			for (ext = exts; ext && *ext; ext++) {
				snprintf(buf, sizeof(buf), "%s/%s/%s/%s.%s", *xdg, theme->name, id->subdir, iconname, *ext);
				if ((missing = access(buf, R_OK))) {
					/* try lower-casing the file name */
					for (p = strrchr(buf, '/'); p && *p; p++)
						*p = tolower(*p);
					missing = access(buf, R_OK);
				}
				if (!missing && (dist = DirectorySizeDistance(id, size)) < minimal_size) {
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

static char *
_FindBestIconHelper(const char **iconlist, int size, const char *name, const char **fexts)
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

static char *
_LookupFallbackIcon(const char *iconname, const char **fexts)
{
	char **dir, **ext, *p;
	char buf[PATH_MAX + 1] = { 0, };

	if (!fexts)
		fexts = (const char **) exts;
	for (dir = dirs; dir && *dir; dir++) {
		for (ext = exts; ext && *ext; ext++) {
			snprintf(buf, sizeof(buf), "%s/%s.%s", *dir, iconname, *ext);
			if (!access(buf, R_OK))
				return strdup(buf);
			/* try lower-casing the file name */
			for (p = strrchr(buf, '/'); p && *p; p++)
				*p = tolower(*p);
			if (!access(buf, R_OK))
				return strdup(buf);
		}
	}
	return (NULL);
}

char *
FindBestIcon(const char **iconlist, int size, const char **fexts)
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
#if 0
	for (icon = iconlist; icon && *icon; icon++)
		if ((file = _LookupAnyIcon(*icon, size, fexts)))
			return (file);
#endif
	return (NULL);
}

static char *
_FindIconHelper(const char *icon, int size, const char *name, const char **fexts)
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
FindIcon(const char *icon, int size, const char **fexts)
{
	char *file;

	if ((file = _FindIconHelper(icon, size, options.icontheme, fexts)))
		return (file);
	if ((file = _FindIconHelper(icon, size, "hicolor", fexts)))
		return (file);
	if ((file = _LookupFallbackIcon(icon, fexts)))
		return (file);
#if 0
	if ((file = _LookupAnyIcon(icon, size, fexts)))
		return (file);
#endif
	return (NULL);
}

static IconDirectory *
allocicondir(IconTheme *theme, char *subdir)
{
	IconDirectory *dir;

	dir = ecalloc(1, sizeof(*dir));
	dir->theme = theme;
	dir->subdir = subdir;
	dir->next = theme->dirs;
	theme->dirs = dir;
	return (dir);
}

static void
freeicontheme(IconTheme *it)
{
	IconDirectory *dir;

	while ((dir = it->dirs)) {
		it->dirs = dir->next;
		free(dir->subdir);
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

static void
freeiconthemes(void)
{
	IconTheme *it;

	while ((it = themes)) {
		themes = it->next;
		it->next = NULL;
		freeicontheme(it);
	}
}

static void
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

static IconTheme *
allocicontheme(char *name, char *path)
{
	IconTheme *it;

	/* We go backward through directories, so if we have a duplicate we want to
	   overwrite it with the new directory data of the same name. */

	OPRINTF("creating new icon theme %s: %s\n", name, path);
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

static Bool
issubdir(const char *path, const char *subdir)
{
	size_t len = strlen(path) + 1 + strlen(subdir) + 1;
	char *dir = ecalloc(len + 1, sizeof(*dir));
	struct stat st;

	strncpy(dir, path, len);
	if (strrchr(dir, '/'))
		*strrchr(dir, '/') = '\0';
	strncat(dir, "/", len);
	strncat(dir, subdir, len);

	if (stat(dir, &st)) {
		EPRINTF("%s: %s\n", strerror(errno), dir);
		free(dir);
		return (False);
	}
	free(dir);
	if (!S_ISDIR(st.st_mode)) {
		EPRINTF("%s: not a directory\n", dir);
		return (False);
	}
	return (True);
}

static IconTheme *
newicontheme(const char *name, const char *path)
{
	FILE *file;

	if ((file = fopen(path, "r"))) {
		IconTheme *it;
		IconDirectory *dir = NULL;
		char *p, *q;
		Bool parsed_theme = False;
		Bool parsing_theme = False;
		Bool parsing_dir = False;
		char *key, *val;
		char *section = NULL;
		static char buf[8*BUFSIZ + 1] = { 0, };

		it = allocicontheme(strdup(name), strdup(path));

		while ((p = fgets(buf, sizeof(buf), file))) {
			if (p[0] == '#')
				continue;
			if ((q = strchr(p, '\r')))
				*q = '\0';
			if ((q = strchr(p, '\n')))
				*q = '\0';
			if (*p == '[' && (q = strchr(p, ']'))) {
				Bool listed;

				*q = '\0';
				free(section);
				section = strdup(p + 1);

				{
					size_t len = strlen(section) + 3;
					char *target = ecalloc(len, sizeof(*target));

					strncpy(target, ",", len);
					strncat(target, section, len);
					strncat(target, ",", len);
					listed = (it->dnames && strstr(it->dnames, target)) || (it->snames && strstr(it->snames, target)) ? True : False;
					free(target);
				}

				if (!strcmp(p + 1, "Icon Theme")) {
					if (!parsed_theme)
						parsing_theme = True;
					parsing_dir = False;
				} else if (listed || issubdir(path, section)) {
					if (!listed)
						EPRINTF("theme %s: subdir %s exists but is not listed in %s\n", name, section, path);
					if (parsing_theme) {
						parsed_theme = True;
						parsing_theme = False;
					}
					if (parsed_theme) {
						parsing_dir = True;
						dir = allocicondir(it, section);
						section = NULL;
					} else {
						parsing_theme = False;
						parsing_dir = False;
					}
				} else {
					EPRINTF("theme %s: unknown section %s in %s\n", name, section, path);
					parsing_theme = False;
					parsing_dir = False;
					dir = NULL;
				}
			} else if (parsing_theme) {
				if ((key = p) && (val = strchr(key, '=')) && !strchr(key, '[')) {
					*val++ = '\0';
					if (!strcmp(key, "Directories")) {
						size_t len = strlen(val) + 3;
						it->dnames = ecalloc(len + 1, sizeof(*it->dnames));
						strncpy(it->dnames, ",", len);
						strncat(it->dnames, val, len);
						strncat(it->dnames, ",", len);
					} else if (!strcmp(key, "ScaledDirectories")) {
						size_t len = strlen(val) + 3;
						it->snames = ecalloc(len + 1, sizeof(*it->snames));
						strncpy(it->snames, ",", len);
						strncat(it->snames, val, len);
						strncat(it->snames, ",", len);
					} else if (!strcmp(key, "Inherits"))
						it->inherits = strdup(val);
					else if (!strcmp(key, "DisplayDepth")) { }
					else if (!strcmp(key, "DesktopDefault")) { }
					else if (!strcmp(key, "DesktopSizes")) { }
					else if (!strcmp(key, "ToolbarDefault")) { }
					else if (!strcmp(key, "ToolbarSizes")) { }
					else if (!strcmp(key, "MainToolbarDefault")) { }
					else if (!strcmp(key, "MainToolbarSizes")) { }
					else if (!strcmp(key, "SmallDefault")) { }
					else if (!strcmp(key, "SmallSizes")) { }
					else if (!strcmp(key, "PanelDefault")) { }
					else if (!strcmp(key, "PanelSizes")) { }
					else if (!strcmp(key, "DialogDefault")) { }
					else if (!strcmp(key, "DialogSizes")) { }
					else if (!strcmp(key, "Name")) { }
					else if (!strcmp(key, "Example")) { }
					else if (!strcmp(key, "Comment")) { }
					else if (!strcmp(key, "Hidden")) { }
					else if (!strcmp(key, "LinkOverlay")) { }
					else if (!strcmp(key, "LockOverlay")) { }
					else if (!strcmp(key, "ShareOverlay")) { }
					else if (!strcmp(key, "ZipOverlay")) { }
					else if (strstr(key, "X-") == key) { }
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
					else if (!strcasecmp(key, "MinSize"))
						dir->minsize = atoi(val);
					else if (!strcasecmp(key, "MaxSize"))
						dir->maxsize = atoi(val);
					else if (!strcmp(key, "Threshold"))
						dir->threshold = atoi(val);
					else if (strstr(key, "X-") == key) { }
					else
						EPRINTF("do not know what to do with key %s in %s\n", key, section);
				}
			}
		}
		fclose(file);
		for (dir = it->dirs; dir; dir = dir->next) {
			if (!dir->type)
				dir->type = strdup("Threshold");
			if (strcmp(dir->type, "Threshold") && strcmp(dir->type, "Fixed") && strcmp(dir->type, "Scalable")) {
				EPRINTF("%s %s unknown Type=%s\n", dir->theme->name, dir->subdir, dir->type);
				free(dir->type);
				dir->type = strdup("Threshold");
			}
			if (!dir->size)
				EPRINTF("%s %s invalid Size=%d\n", dir->theme->name, dir->subdir, dir->size);
			if (!strcmp(dir->type, "Threshold")) {
				if (!dir->threshold)
					dir->threshold = 2;
			} else
			if (!strcmp(dir->type, "Scalable")) {
				if (!dir->maxsize)
					dir->maxsize = dir->size;
				if (!dir->minsize)
					dir->minsize = dir->size;
			}
		}
		return (it);
	}
	return (NULL);
}

static void
rescanicons(void)
{
	char *name;

	freeiconthemes();

	while ((name = popthemename())) {
		size_t nlen = strlen(name);
		char **xdg;

		OPRINTF("searching for theme %s\n", name);
		for (xdg = xdgs; xdg && *xdg; xdg++) ;
		for (xdg--; xdg >= xdgs; xdg--) {
			IconTheme *it;
			size_t len = strlen(*xdg) + nlen + 13 + 1;
			char *path = ecalloc(len, sizeof(*path));

			strncpy(path, *xdg, len);
			strncat(path, "/", len);
			strncat(path, name, len);
			strncat(path, "/index.theme", len);
			if (!access(path, R_OK) && (it = newicontheme(name, path))
			    && it->inherits && strcmp(it->inherits, name)
			    && strcmp(it->inherits, "hicolor"))
				pushthemename(it->inherits);
			free(path);
		}
		free(name);
	}
}

static Bool
already(char **list, char *item)
{
	char **p;

	for (p = list; p && *p && *p != item && strcmp(*p, item); p++) ;
	if (p && *p && *p != item)
		return True;
	return False;
}

void
initicons(Bool reload __attribute__((unused)))
{
	const char *p, *q, *env;
	char *l, *e;
	int i = 0, j = 0, k = 0;
	size_t len;
	const char *home = getenv("HOME") ? : "~";
	struct stat st;
	IconTheme *it;

	dirs = freestringlist(dirs);
	xdgs = freestringlist(xdgs);
	exts = freestringlist(exts);
	
	free(config.iconname);
	config.iconname = NULL;
	free(config.iconfile);
	config.iconfile = NULL;

	OPRINTF("initializing icon theme\n");
	for (p = options.prependdirs; p && (q = strchrnul(p, ':')); p = q + 1) {
		dirs = reallocarray(dirs, i + 2, sizeof(*dirs));
		dirs[i + 1] = NULL;
		dirs[i] = strndup(p, q - p);
		if (!already(dirs, dirs[i]) && !stat(dirs[i], &st) && S_ISDIR(st.st_mode)) {
			OPRINTF("added directory to search paths: %s\n", dirs[i]);
			i++;
		} else {
			free(dirs[i]);
			dirs[i] = NULL;
		}
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
		if (!already(dirs, dirs[i]) && !stat(dirs[i], &st) && S_ISDIR(st.st_mode)) {
			OPRINTF("added directory to search paths: %s\n", dirs[i]);
			i++;
		} else {
			free(dirs[i]);
			dirs[i] = NULL;
		}
	}
	if ((env = getenv("XDG_DATA_HOME"))) {
		xdgs = reallocarray(xdgs, j + 2, sizeof(*xdgs));
		xdgs[j + 1] = NULL;
		len = strlen(env) + strlen("/icons") + 1;
		xdgs[j] = ecalloc(len + 1, sizeof(*xdgs[j]));
		strncpy(xdgs[j], env, len);
		strncat(xdgs[j], "/icons", len);
		if (!already(xdgs, xdgs[j]) && !stat(xdgs[j], &st) && S_ISDIR(st.st_mode)) {
			OPRINTF("added directory to XDG paths: %s\n", xdgs[j]);
			j++;
		} else {
			free(xdgs[j]);
			xdgs[j] = NULL;
		}
	} else {
		xdgs = reallocarray(xdgs, j + 2, sizeof(*xdgs));
		xdgs[j + 1] = NULL;
		len = strlen(home) + strlen("/.local/share/icons") + 1;
		xdgs[j] = ecalloc(len + 1, sizeof(*xdgs[j]));
		strncpy(xdgs[j], home, len);
		strncpy(xdgs[j], home, len);
		strncat(xdgs[j], "/.local/share/icons", len);
		if (!already(xdgs, xdgs[j]) && !stat(xdgs[j], &st) && S_ISDIR(st.st_mode)) {
			OPRINTF("added directory to XDG paths: %s\n", xdgs[j]);
			j++;
		} else {
			free(xdgs[j]);
			xdgs[j] = NULL;
		}
	}
	env = getenv("XDG_DATA_DIRS") ? : "/usr/local/share:/usr/share";
	for (p = env; (q = strchrnul(p, ':')); p = q + 1) {
		xdgs = reallocarray(xdgs, j + 2, sizeof(*xdgs));
		xdgs[j + 1] = NULL;
		len = (q - p) + strlen("/icons") + 1;
		xdgs[j] = ecalloc(len + 1, sizeof(*xdgs[j]));
		strncpy(xdgs[j], p, q - p);
		strncat(xdgs[j], "/icons", len);
		if (!already(xdgs, xdgs[j]) && !stat(xdgs[j], &st) && S_ISDIR(st.st_mode)) {
			OPRINTF("added directory to XDG paths: %s\n", xdgs[j]);
			j++;
		} else {
			free(xdgs[j]);
			xdgs[j] = NULL;
		}
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
		if (!already(dirs, dirs[i]) && !stat(dirs[i], &st) && S_ISDIR(st.st_mode)) {
			OPRINTF("added directory to search paths: %s\n", dirs[i]);
			i++;
		} else {
			free(dirs[i]);
			dirs[i] = NULL;
		}
	} else {
		dirs = reallocarray(dirs, i + 2, sizeof(*dirs));
		dirs[i + 1] = NULL;
		len = strlen(home) + strlen("/.local/share/pixmaps") + 1;
		dirs[i] = ecalloc(len + 1, sizeof(*dirs[i]));
		strncpy(dirs[i], home, len);
		strncat(dirs[i], "/.local/share/pixmaps", len);
		if (!already(dirs, dirs[i]) && !stat(dirs[i], &st) && S_ISDIR(st.st_mode)) {
			OPRINTF("added directory to search paths: %s\n", dirs[i]);
			i++;
		} else {
			free(dirs[i]);
			dirs[i] = NULL;
		}
	}
	env = getenv("XDG_DATA_DIRS") ? : "/usr/local/share:/usr/share";
	for (p = env; (q = strchrnul(p, ':')); p = q + 1) {
		dirs = reallocarray(dirs, i + 2, sizeof(*dirs));
		dirs[i + 1] = NULL;
		len = (q - p) + strlen("/pixmaps") + 1;
		dirs[i] = ecalloc(len + 1, sizeof(*dirs[i]));
		strncpy(dirs[i], p, (q - p));
		strncat(dirs[i], "/pixmaps", len);
		if (!already(dirs, dirs[i]) && !stat(dirs[i], &st) && S_ISDIR(st.st_mode)) {
			OPRINTF("added directory to search paths: %s\n", dirs[i]);
			i++;
		} else {
			free(dirs[i]);
			dirs[i] = NULL;
		}
		if (!*q)
			break;
	}
	for (p = options.appenddirs; p && (q = strchrnul(p, ':')); p = q + 1) {
		dirs = reallocarray(dirs, i + 2, sizeof(*dirs));
		dirs[i + 1] = NULL;
		dirs[i] = strndup(p, q - p);
		if (!already(dirs, dirs[i]) && !stat(dirs[i], &st) && S_ISDIR(st.st_mode)) {
			OPRINTF("added directory to search paths: %s\n", dirs[i]);
			i++;
		} else {
			free(dirs[i]);
			dirs[i] = NULL;
		}
		if (!*q)
			break;
	}
	for (p = options.extensions; p && (q = strchrnul(p, ',')); p = q + 1) {
		exts = reallocarray(exts, k + 2, sizeof(*exts));
		exts[k + 1] = NULL;
		exts[k] = strndup(p, q - p);
		if (!already(exts, exts[k])) {
			OPRINTF("added file extension preference list: %s\n", exts[k]);
			k++;
		} else {
			free(exts[k]);
			exts[k] = NULL;
		}
		if (!*q)
			break;
	}
	if (!options.icontheme) {
		if ((env = getenv("XDG_ICON_THEME")))
			options.icontheme = strdup(env);
		else {
			char buf[PATH_MAX + 1] = { 0, };
			int missing;
			FILE *file;
			strncpy(buf, home, PATH_MAX);
			strncat(buf, "/.gtkrc-2.0.xde", PATH_MAX);
			if ((missing = access(buf, R_OK))) {
				strncpy(buf, home, PATH_MAX);
				strncat(buf, "/.gtkrc-2.0", PATH_MAX);
				missing = access(buf, R_OK);
			}
			if (!missing && (file = fopen(buf, "r"))) {
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
		if (!options.icontheme)
			options.icontheme = strdup("hicolor");
	}
	OPRINTF("using icon theme %s\n", options.icontheme);
	pushthemename("hicolor");
	if (options.icontheme && strcmp(options.icontheme, "hicolor"))
		pushthemename(options.icontheme);
	if (options.icontheme)
		config.iconname = strdup(options.icontheme);
	rescanicons();

	for (it = themes; it && strcmp(it->name, options.icontheme); it = it->next) ;
	if (it && it->path) 
		config.iconfile = strdup(it->path);
}
