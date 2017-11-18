/* icon.c */

#ifndef __LOCAL_ICON_H__
#define __LOCAL_ICON_H__

void initicons(Bool reload);
char *FindBestIcon(const char **iconlist, int size, const char **fexts);
char *FindIcon(const char *icon, int size, const char **fexts);

#endif				/* __LOCAL_ICON_H__ */
