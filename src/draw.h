/* draw.c */

#ifndef __LOCAL_DRAW_H__
#define __LOCAL_DRAW_H__

enum { Normal, Focused, Selected };
enum { AlignLeft, AlignCenter, AlignRight };	/* title position */

extern Bool (*buttons[LastElement][Button5-Button1+1][2]) (Client *, XEvent *);

Bool createwmicon(Client *c);
Bool createkwmicon(Client *c, Pixmap *data, unsigned long n);
Bool createneticon(Client *c, long *data, unsigned long n);
Bool createappicon(Client *c);
void removebutton(ButtonImage *bi);
void drawclient(Client *c);
#ifdef DAMAGE
Bool drawdamage(Client *c, XDamageNotifyEvent *ev);
#endif
unsigned titleheight(AScreen *ds);
int elementw(AScreen *ds, Client *c, char which);
int drawelement(AScreen *ds, char which, int x, int position, Client *c);
ButtonImage *buttonimage(AScreen *ds, Client *c, ElementType type);
unsigned int textnw(AScreen *ds, const char *text, unsigned int len, int hilite);
unsigned int textw(AScreen *ds, const char *text, int hilite);
int gethilite(Client *c);
XftColor *gethues(AScreen *ds, Client *c);
XftColor *getcolor(AScreen *ds, Client *c, int type);
unsigned long getpixel(AScreen *ds, Client *c, int type);
void initelement(ElementType type, const char *name, const char *dev, Bool (**action) (Client *, XEvent *));
void freestyle();
void initstyle(Bool reload);

#endif				/* __LOCAL_DRAW_H__ */
