/* xlib.c */

#ifndef __LOCAL_XLIB_H__
#define __LOCAL_XLIB_H__
void xlib_removepixmap(AdwmPixmap *p);
Bool xlib_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h);
Bool xlib_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h, unsigned d);
Bool xlib_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data);
Bool xlib_createpngicon(AScreen *ds, Client *c, const char *file);
Bool xlib_createsvgicon(AScreen *ds, Client *c, const char *file);
Bool xlib_createxpmicon(AScreen *ds, Client *c, const char *file);
Bool xlib_createxbmicon(AScreen *ds, Client *c, const char *file);
#ifdef DAMAGE
Bool xlib_drawdamage(Client *c, XDamageNotifyEvent *ev);
#endif
int xlib_drawbutton(AScreen *ds, Client *c, ElementType type, XftColor *col, int x);
int xlib_drawtext(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int mw);
int xlib_drawsep(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int mw);
void xlib_drawdockapp(AScreen *ds, Client *c);
void xlib_drawnormal(AScreen *ds, Client *c);
Bool xlib_initpng(char *path, AdwmPixmap *p);
Bool xlib_initsvg(char *path, AdwmPixmap *p);
Bool xlib_initxpm(char *path, AdwmPixmap *p);
Bool xlib_initxbm(char *path, AdwmPixmap *p);
Bool xlib_initxbmdata(const unsigned char *bits, int width, int height, AdwmPixmap *px);
Bool xlib_initpixmap(char *path, AdwmPixmap *p);
Bool xlib_initbitmap(char *path, AdwmPixmap *p);
void xlib_getpixmap(const char *file, AdwmPixmap *p);
#endif				/* __LOCAL_XLIB_H__ */

