/* config.c */

#ifndef __LOCAL_CONFIG_H__
#define __LOCAL_CONFIG_H__

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
			char *iconfile;	    /* icon index.theme file */
			char *themefile;	/* themerc file */
			char *stylefile;	/* stylerc file */
			char *rcfile;	/* rcfile */
		};
		char *files[6];
	};
	union {
		struct {
			char *dockname; /* dock theme name */
			char *keynames; /* keys theme name */
			char *iconname; /* icon theme name */
			char *themename; /* theme name */
			char *stylename; /* style name */
		};
		char *names[5];
	};
} AdwmPlaces;

void inittags(Bool reload);
void initdock(Bool reload);
void initlayouts(Bool reload);
void initscreen(Bool reload);
void initconfig(Bool reload);
void initrcfile(const char *file, Bool reload);
char *findrcpath(const char *file);

#endif				/* __LOCAL_CONFIG_H__ */
