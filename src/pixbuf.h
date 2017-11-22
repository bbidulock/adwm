/* pixbuf.c */

#ifndef __LOCAL_PIXBUF_H__
#define __LOCAL_PIXBUF_H__
#if defined PIXBUF
void pixbuf_removepixmap(AdwmPixmap *p);
Bool pixbuf_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h);
Bool pixbuf_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h, unsigned d);
Bool pixbuf_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data);
Bool pixbuf_createpngicon(AScreen *ds, Client *c, const char *file);
Bool pixbuf_createsvgicon(AScreen *ds, Client *c, const char *file);
Bool pixbuf_createxpmicon(AScreen *ds, Client *c, const char *file);
Bool pixbuf_createxbmicon(AScreen *ds, Client *c, const char *file);
#ifdef DAMAGE
Bool pixbuf_drawdamage(Client *c, XDamageNotifyEvent *ev);
#endif
int pixbuf_drawbutton(AScreen *ds, Client *c, ElementType type, XftColor *col, int x);
int pixbuf_drawtext(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int mw);
int pixbuf_drawsep(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int w);
void pixbuf_drawdockapp(AScreen *ds, Client *c);
void pixbuf_drawnormal(AScreen *ds, Client *c);
Bool pixbuf_initpng(char *path, AdwmPixmap *p);
Bool pixbuf_initsvg(char *path, AdwmPixmap *p);
Bool pixbuf_initxpm(char *path, AdwmPixmap *p);
Bool pixbuf_initxbm(char *path, AdwmPixmap *p);
Bool pixbuf_initxbmdata(const unsigned char *bits, int width, int height, AdwmPixmap *px);
Bool pixbuf_initpixmap(char *path, AdwmPixmap *p);
Bool pixbuf_initbitmap(char *path, AdwmPixmap *p);
void pixbuf_getpixmap(const char *file, AdwmPixmap *p);
#endif				/* defined PIXBUF */
#endif				/* __LOCAL_PIXBUF_H__ */
