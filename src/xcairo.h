/* xcairo.c */

#ifndef __LOCAL_XCAIRO_H__
#define __LOCAL_XCAIRO_H__
#if defined XCAIRO
void xcairo_removepixmap(AdwmPixmap *p);
Bool xcairo_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h);
Bool xcairo_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h, unsigned d);
Bool xcairo_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data);
Bool xcairo_createpngicon(AScreen *ds, Client *c, const char *file);
Bool xcairo_createsvgicon(AScreen *ds, Client *c, const char *file);
Bool xcairo_createxpmicon(AScreen *ds, Client *c, const char *file);
Bool xcairo_createxbmicon(AScreen *ds, Client *c, const char *file);
#ifdef DAMAGE
Bool xcairo_drawdamage(Client *c, XDamageNotifyEvent * ev);
#endif
int xcairo_drawbutton(AScreen *ds, Client *c, ElementType type, XftColor *col, int x);
int xcairo_drawtext(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int mw);
int xcairo_drawsep(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int w);
void xcairo_drawdockapp(AScreen *ds, Client *c);
void xcairo_drawnormal(AScreen *ds, Client *c);
Bool xcairo_initpng(char *path, AdwmPixmap *p);
Bool xcairo_initsvg(char *path, AdwmPixmap *p);
Bool xcairo_initxpm(char *path, AdwmPixmap *p);
Bool xcairo_initxbm(char *path, AdwmPixmap *p);
Bool xcairo_initxbmdata(const unsigned char *bits, int width, int height, AdwmPixmap *px);
Bool xcairo_initpixmap(char *path, AdwmPixmap *p);
Bool xcairo_initbitmap(char *path, AdwmPixmap *p);
void xcairo_getpixmap(const char *file, AdwmPixmap *p);
#endif				/* defined XCAIRO */
#endif				/* __LOCAL_XCAIRO_H__ */
