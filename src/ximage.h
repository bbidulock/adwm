/* ximage.c */

#ifndef __LOCAL_XIMAGE_H__
#define __LOCAL_XIMAGE_H__
void ximage_removebutton(ButtonImage *bi);
Bool ximage_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h);
Bool ximage_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h, unsigned d);
Bool ximage_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data);
Bool ximage_createpngicon(AScreen *ds, Client *c, const char *file);
Bool ximage_createsvgicon(AScreen *ds, Client *c, const char *file);
Bool ximage_createxpmicon(AScreen *ds, Client *c, const char *file);
Bool ximage_createxbmicon(AScreen *ds, Client *c, const char *file);
#ifdef DAMAGE
Bool ximage_drawdamage(Client *c, XDamageNotifyEvent *ev);
#endif
int ximage_drawbutton(AScreen *ds, Client *c, ElementType type, XftColor *col, int x);
int ximage_drawtext(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int mw);
int ximage_drawsep(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int w);
void ximage_drawdockapp(AScreen *ds, Client *c);
void ximage_drawnormal(AScreen *ds, Client *c);
Bool ximage_initpng(char *path, ButtonImage *bi);
Bool ximage_initsvg(char *path, ButtonImage *bi);
Bool ximage_initxpm(char *path, ButtonImage *bi);
Bool ximage_initxbm(char *path, ButtonImage *bi);
Bool ximage_initpixmap(char *path, ButtonImage *bi);
Bool ximage_initbitmap(char *path, ButtonImage *bi);
#endif				/* __LOCAL_XIMAGE_H__ */

