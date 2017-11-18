/* draw.c */

#ifndef __LOCAL_DRAW_H__
#define __LOCAL_DRAW_H__

Bool createwmicon(Client *c);
Bool createkwmicon(Client *c, Pixmap *data, unsigned long n);
Bool createneticon(Client *c, long *data, unsigned long n);
Bool createappicon(Client *c);
void drawclient(Client *c);
void freestyle();
void initstyle(Bool reload);

#endif				/* __LOCAL_DRAW_H__ */
