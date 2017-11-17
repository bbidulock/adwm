/* icon.c */

#ifndef __LOCAL_ICON_H__
#define __LOCAL_ICON_H__

void initicons(void);
char *FindBestIcon(const char **iconlist, int size, char **fexts);
char *FindIcon(const char *icon, int size, char **fexts);

#endif				/* __LOCAL_ICON_H__ */
