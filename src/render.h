/* render.c */

#ifndef __LOCAL_RENDER_H__
#define __LOCAL_RENDER_H__
#if defined RENDER
void render_removepixmap(AdwmPixmap *p);
Bool render_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h);
Bool render_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h, unsigned d);
Bool render_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data);
Bool render_createpngicon(AScreen *ds, Client *c, const char *file);
Bool render_createsvgicon(AScreen *ds, Client *c, const char *file);
Bool render_createxpmicon(AScreen *ds, Client *c, const char *file);
Bool render_createxbmicon(AScreen *ds, Client *c, const char *file);
#ifdef DAMAGE
Bool render_drawdamage(Client *c, XDamageNotifyEvent *ev);
#endif
int render_drawbutton(AScreen *ds, Client *c, ElementType type, XftColor *col, int x);
int render_drawtext(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int mw);
int render_drawsep(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int w);
void render_drawdockapp(AScreen *ds, Client *c);
void render_drawnormal(AScreen *ds, Client *c);
Bool render_initpng(char *path, AdwmPixmap *p);
Bool render_initsvg(char *path, AdwmPixmap *p);
Bool render_initxpm(char *path, AdwmPixmap *p);
Bool render_initxbm(char *path, AdwmPixmap *p);
Bool render_initxbmdata(const unsigned char *bits, int width, int height, AdwmPixmap *px);
Bool render_initpixmap(char *path, AdwmPixmap *p);
Bool render_initbitmap(char *path, AdwmPixmap *p);
void render_getpixmap(const char *file, AdwmPixmap *p);
#endif				/* defined RENDER */
#endif				/* __LOCAL_RENDER_H__ */
