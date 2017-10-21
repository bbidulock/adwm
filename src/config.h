/* config.c */

#ifndef __LOCAL_CONFIG_H__
#define __LOCAL_CONFIG_H__

void inittags(Bool reload);
void initdock(Bool reload);
void initlayouts(Bool reload);
void initscreen(Bool reload);
void initconfig(Bool reload);
void initrcfile(const char *file, Bool reload);
char *findrcpath(const char *file);

#endif				/* __LOCAL_CONFIG_H__ */
