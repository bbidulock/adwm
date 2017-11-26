/* image.c */

#ifndef __LOCAL_IMAGE_H__
#define __LOCAL_IMAGE_H__

const char *xbm_status_string(int status);
#ifdef XPM
const char *xpm_status_string(int status);
#endif

XImage *combine_bitmap_and_mask(AScreen *ds, XImage *xdraw, XImage *xmask);
XImage *combine_pixmap_and_mask(AScreen *ds, XImage *xdraw, XImage *xmask);

int XReadBitmapFileImage(Display *display, Visual * visual, const char *file,
			 unsigned *width, unsigned *height, XImage **image_return,
			 int *x_hot, int *y_hot);
#ifdef LIBPNG
XImage *png_read_file_to_ximage(Display *display, Visual* visual, const char *file);
#endif
void renderimage(AScreen *ds, const ARGB *argb, const unsigned width,
		 const unsigned height);
void initimage(void);

#endif				/* __LOCAL_IMAGE_H__ */
