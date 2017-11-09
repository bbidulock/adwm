/* draw.c */

#ifndef __LOCAL_DRAW_H__
#define __LOCAL_DRAW_H__

Bool createneticon(Client *c, long *data, unsigned long n);
Bool createwmicon(Client *c);
void drawclient(Client *c);
void freestyle();
void initstyle(Bool reload);

#endif				/* __LOCAL_DRAW_H__ */
