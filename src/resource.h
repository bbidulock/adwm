/* resource.c */

#ifndef __LOCAL_RESOURCE_H__
#define __LOCAL_RESOURCE_H__

const char *readres(const char *name, const char *clas, const char *defval);
void getbitmap(const unsigned char *bits, int width, int height, AdwmBitmap * bitmap);
void getappearance(const char *descrip, Appearance *appear);
void getxcolor(const char *color, const char *defcol, XColor *xcol);
void getxftcolor(const char *color, const char *defcol, XftColor *xftcol);
void freexftcolor(XftColor *xftcol);
void getbool(const char *name, const char *clas, const char *true_val, Bool defval,
	     Bool *result);
void getfont(const char *font, const char *deffont, AdwmFont *afont);
void freefont(AdwmFont *afont);
void readtexture(const char *name, const char *clas, Texture *t, const char *defcol,
		 const char *oppcol);
void freetexture(Texture *t);
void getpixmap(const char *file, AdwmPixmap *p);
void getshadow(const char *descrip, TextShadow *shadow);

#endif				/* __LOCAL_RESOURCE_H__ */
