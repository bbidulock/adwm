/* xlib.c */

#ifndef __LOCAL_XLIB_H__
#define __LOCAL_XLIB_H__
void xlib_removebutton(ButtonImage *bi);
Bool xlib_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h);
Bool xlib_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h, unsigned d);
Bool xlib_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data);
Bool xlib_createpngicon(AScreen *ds, Client *c, const char *file);
Bool xlib_createsvgicon(AScreen *ds, Client *c, const char *file);
Bool xlib_createxpmicon(AScreen *ds, Client *c, const char *file);
Bool xlib_createxbmicon(AScreen *ds, Client *c, const char *file);
int xlib_drawbutton(AScreen *ds, Client *c, ElementType type, XftColor *col, int x);
int xlib_drawtext(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int mw);
int xlib_drawsep(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int mw);
void xlib_drawdockapp(AScreen *ds, Client *c);
void xlib_drawnormal(AScreen *ds, Client *c);
Bool xlib_initpng(char *path, ButtonImage *bi);
Bool xlib_initsvg(char *path, ButtonImage *bi);
Bool xlib_initxpm(char *path, ButtonImage *bi);
Bool xlib_initxbm(char *path, ButtonImage *bi);
Bool xlib_initpixmap(char *path, ButtonImage *bi);
Bool xlib_initbitmap(char *path, ButtonImage *bi);
#endif				/* __LOCAL_XLIB_H__ */

