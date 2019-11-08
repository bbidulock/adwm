/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "draw.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "actions.h"
#include "config.h"
#include "image.h" /* verification */

#define ALPHAMAX

const char *
xbm_status_string(int status)
{
	static char buf[64] = { 0, };

	switch (status) {
	case BitmapSuccess:
		return ("success");
	case BitmapOpenFailed:
		return ("open failed");
	case BitmapFileInvalid:
		return ("file invalid");
	case BitmapNoMemory:
		return ("no memory");
	default:
		snprintf(buf, sizeof(buf), "unknown %d", status);
		return (buf);
	}
}

#ifdef XPM
const char *
xpm_status_string(int status)
{
	static char buf[64] = { 0, };

	switch (status) {
	case XpmColorError:
		return ("color error");
	case XpmSuccess:
		return ("success");
	case XpmOpenFailed:
		return ("open failed");
	case XpmFileInvalid:
		return ("file invalid");
	case XpmNoMemory:
		return ("no memory");
	case XpmColorFailed:
		return ("color failed");
	default:
		snprintf(buf, sizeof(buf), "unknown %d", status);
		return (buf);
	}
}
#endif

int
XReadBitmapFileImage(Display *display, Visual *visual, const char *file, unsigned *width,
		     unsigned *height, XImage **image_return, int *x_hot, int *y_hot)
{
	XImage *ximage = NULL;
	unsigned char *data = NULL;
	unsigned w, h;
	int x, y, status;

	status = XReadBitmapFileData(file, &w, &h, &data, &x, &y);
	if (status != BitmapSuccess || !data)
		goto error;
	if (!(ximage = XCreateImage(display, visual, 1, XYBitmap, 0, NULL, w, h, 8, 0))) {
		status = BitmapNoMemory;
		goto error;
	}
	ximage->data = (char *) data;
	data = NULL;
	if (width)
		*width = w;
	if (height)
		*height = h;
	if (image_return)
		*image_return = ximage;
	else {
		free(ximage->data);
		ximage->data = NULL;
		XDestroyImage(ximage);
	}
	if (x_hot)
		*x_hot = x;
	if (y_hot)
		*y_hot = y;
      error:
	if (data)
		free(data);
	return (status);
}

XImage *
combine_bitmap_and_mask(AScreen *ds, XImage *xdraw, XImage *xmask)
{
	XImage *ximage = NULL;
	unsigned i, j;
	unsigned w = xdraw->width;
	unsigned h = xdraw->height;

	if (!(ximage = XCreateImage(dpy, ds->visual, 8, XYPixmap, 0, NULL, w, h, 8, 0))) {
		EPRINTF("could not create image %ux%ux%u\n", w, h, 32);
		return (NULL);
	}
	if (!(ximage->data = calloc(ximage->bytes_per_line, ximage->height))) {
		XDestroyImage(ximage);
		return (NULL);
	}
	for (j = 0; j < h; j++)
		for (i = 0; i < w; i++)
			if (!xmask || XGetPixel(xmask, i, j))
				XPutPixel(ximage, i, j, XGetPixel(xdraw, i, j) ? 255 : 0);
			else
				XPutPixel(ximage, i, j, 0);
	return (ximage);
}

XImage *
combine_pixmap_and_mask(AScreen *ds, XImage *xdraw, XImage *xmask)
{
	XImage *ximage = NULL;
	unsigned i, j;
	unsigned w = xdraw->width;
	unsigned h = xdraw->height;
	unsigned long bits = 0xff000000;

	if (!(ximage = XCreateImage(dpy, ds->visual, ds->depth, ZPixmap, 0, NULL, w, h, 8, 0))) {
		EPRINTF("could not create image %ux%ux%u\n", w, h, ds->depth);
		return (NULL);
	}
	if (!(ximage->data = calloc(ximage->bytes_per_line, ximage->height))) {
		XDestroyImage(ximage);
		return (NULL);
	}
	for (j = 0; j < h; j++)
		for (i = 0; i < w; i++)
			if (!xmask || XGetPixel(xmask, i, j))
				XPutPixel(ximage, i, j, XGetPixel(xdraw, i, j) | bits);
			else
				XPutPixel(ximage, i, j, XGetPixel(xdraw, i, j) & ~bits);
	return (ximage);
}

#ifdef LIBPNG
XImage *
png_read_file_to_ximage(Display *display, Visual *visual, const char *file)
{
	XImage *xicon = NULL;
	png_structp png_ptr;
	png_infop info_ptr;
	png_byte buf[8];
	int ret;
	FILE *f;

	if (!(f = fopen(file, "rb"))) {
		EPRINTF("cannot open %s: %s\n", file, strerror(errno));
		goto nofile;
	}
	if ((ret = fread(buf, 1, 8, f)) != 8)
		goto noread;
	if (png_sig_cmp(buf, 0, 8))
		goto noread;

	if (!(png_ptr = png_create_read_struct(png_get_libpng_ver(NULL), NULL, NULL, NULL)))
		goto noread;
	if (!(info_ptr = png_create_info_struct(png_ptr)))
		goto noinfo;
	if (setjmp(png_jmpbuf(png_ptr)))
		goto pngerr;

	png_uint_32 width, height, row_bytes;
	int bit_depth, color_type, channels;

	png_init_io(png_ptr, f);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_expand(png_ptr);
	if (bit_depth == 16)
		png_set_strip_16(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	png_read_update_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
	if (color_type == PNG_COLOR_TYPE_GRAY)
		channels = 1;
	else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		channels = 2;
	else if (color_type == PNG_COLOR_TYPE_RGB)
		channels = 3;
	else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
		channels = 4;
	else
		channels = 0;	/* should never happen */
	row_bytes = png_get_rowbytes(png_ptr, info_ptr);
	png_byte *png_pixels = ecalloc(row_bytes * height, sizeof(*png_pixels));
	volatile void *vol_png_pixels = png_pixels;
	png_byte **row_pointers = ecalloc(height, sizeof(*row_pointers));
	volatile void *vol_row_pointers = row_pointers;
	unsigned int i;
	for (i = 0; i < height; i++)
		row_pointers[i] = png_pixels + i * row_bytes;
	png_read_image(png_ptr, row_pointers);
	png_read_end(png_ptr, info_ptr);
	if (!(xicon = XCreateImage(display, visual, 32, ZPixmap, 0, NULL, width, height, 32, 0)))
		goto pngerr;
	volatile void *vol_xicon = xicon;
	xicon->data = ecalloc(xicon->bytes_per_line, xicon->height);
	{
		unsigned int j = 0;

		(void) j;
		for (png_byte *p = png_pixels, j = 0; j < height; j++) {
			for (i = 0; i < width; i++, p += channels) {
				unsigned long A, R, G, B;
				switch(color_type) {
				case PNG_COLOR_TYPE_GRAY:
					R = G = B = p[0];
					A = 255;
					break;
				case PNG_COLOR_TYPE_GRAY_ALPHA:
					R = G = B = p[0];
					A = p[1];
					break;
				case PNG_COLOR_TYPE_RGB:
					R = p[0];
					G = p[1];
					B = p[2];
					A = 255;
					break;
				case PNG_COLOR_TYPE_RGB_ALPHA:
					R = p[0];
					G = p[1];
					B = p[2];
					A = p[3];
					break;
				}
				if (!A)
					R = G = B = 0;
				unsigned long pixel = (A << 24)|(R <<16)|(G<<8)|(B<<0);
				XPutPixel(xicon, i, j, pixel);
			}
		}
	}
      pngerr:
	xicon = (typeof(xicon)) vol_xicon;
	png_pixels = (typeof(png_pixels)) vol_png_pixels;
	free(png_pixels);
	row_pointers = (typeof(row_pointers)) vol_row_pointers;
	free(row_pointers);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	goto noread;
      noinfo:
	png_destroy_read_struct(&png_ptr, NULL, NULL);
      noread:
	fclose(f);
      nofile:
	return (xicon);
}
#endif

XImage *
dn_scale_image(AScreen *ds, XImage *ximage, unsigned nw, unsigned nh, Bool bitmap)
{
	XImage *xscale = NULL;
	size_t elements = nw * nh;
	double *counts = NULL;
	double *colors = NULL;

	if (!(xscale = XCreateImage(dpy, ds->visual, 32, ZPixmap, 0, NULL, nw, nh, 8, 0))) {
		EPRINTF("could not create ximage\n");
		goto error;
	}
	if (!(xscale->data = calloc(xscale->bytes_per_line, xscale->height))) {
		EPRINTF("could not allocate ximage data\n");
		goto error;
	}
	if (!(counts = ecalloc(elements, sizeof(*counts)))) {
		EPRINTF("could not allocate %zu counts\n", elements);
		goto error;
	}
	if (!(colors = ecalloc(elements, sizeof(*colors) << 2))) {
		EPRINTF("could not allocate %zu colors\n", elements<<2);
		goto error;
	}

	unsigned w = ximage->width;
	unsigned h = ximage->height;
	unsigned i, j, k, l, m, n;

	double pppx = (double) nw / (double) w;
	double pppy = (double) nh / (double) h;

	double ty, by, lx, rx, xf, yf, ff;

	unsigned long A, R, G, B, pixel;

	XPRINTF("downscaling image %ux%ux%u to %ux%ux%u\n",
			ximage->width, ximage->height, ximage->depth,
			xscale->width, xscale->height, xscale->depth);

	for (ty = 0.0, by = pppy, l = 0; l < h; l++, ty = by, by = (l + 1) * pppy) {

		for (j = floor(ty); j < by; j++) {

			if (ty < (j + 1) && (j + 1) < by) yf = (j + 1) - ty;
			else if (ty < j && j < by) yf = by - j;
			else yf = 1.0;

			for (lx = 0.0, rx = pppx, k = 0; k < w; k++, lx = rx, rx = (k + 1) * pppx) {

				for (i = floor(lx); i < rx; i++) {

					if (lx < (i + 1) && (i + 1) < rx) xf = (i + 1) - lx;
					else if (lx < i && i < rx) xf = rx - i;
					else xf = 1.0;

					ff = xf * yf;
					m = j * nw + i;
					n = m << 2;
					counts[m] += ff;
					pixel = XGetPixel(ximage, k, l);
					if (bitmap && (pixel & 0x00ffffff))
						pixel |= 0x00ffffff;
					A = (pixel >> 24) & 0xff;
					R = (pixel >> 16) & 0xff;
					G = (pixel >> 8) & 0xff;
					B = (pixel >> 0) & 0xff;
					colors[n + 0] += A * ff;
					colors[n + 1] += R * ff;
					colors[n + 2] += G * ff;
					colors[n + 3] += B * ff;
				}
			}
		}
	}
	unsigned long amax = 0;

	for (j = 0; j < nh; j++) {
		for (i = 0; i < nw; i++) {
			m = j * nw + i;
			n = m << 2;
			if (counts[m]) {
				pixel = ((A = min(255, lround(colors[n + 0] / counts[m]))) << 24) |
					((R = min(255, lround(colors[n + 1] / counts[m]))) << 16) |
					((G = min(255, lround(colors[n + 2] / counts[m]))) <<  8) |
					((B = min(255, lround(colors[n + 3] / counts[m]))) <<  0);
				XPutPixel(xscale, i, j, pixel);
				amax = max(amax, A);
			}
		}
	}
	/* no opacity, add some */
	if (!amax)
		for (j = 0; j < nh; j++)
			for (i = 0; i < nw; i++)
				XPutPixel(xscale, i, j, XGetPixel(xscale, i, j) | 0xff000000);
#ifdef ALPHAMAX
	else if (amax < 255UL) {
		double bump = (double) 255 / (double) amax;

		for (j = 0; j < nh; j++) {
			for (i = 0; i < nw; i++) {
				pixel = XGetPixel(xscale, i, j);
				A = (pixel >> 24) & 0xff;
				A = min(255, lround(A * bump));
				pixel = (pixel & 0x00ffffff) | (A << 24);
				XPutPixel(xscale, i, j, pixel);
			}
		}
	}
#endif
	free(counts);
	free(colors);
	XDestroyImage(ximage);
	return (xscale);
      error:
	if (xscale)
		XDestroyImage(xscale);
	if (counts)
		free(counts);
	if (colors)
		free(colors);
	return (ximage);
}

XImage *
up_scale_image(AScreen *ds, XImage *ximage, unsigned nw, unsigned nh, Bool bitmap)
{
	XImage *xscale = NULL;
	size_t elements = nw * nh;

	xscale = XCreateImage(dpy, ds->visual, 32, ZPixmap, 0, NULL, nw, nh, 8, 0);
	if (!xscale)
		return (ximage);
	xscale->data = ecalloc(xscale->bytes_per_line, xscale->height);
	if (!xscale->data) {
		XDestroyImage(xscale);
		return (ximage);
	}
	double *counts = ecalloc(elements, sizeof(*counts));
	double *colors = ecalloc(elements, sizeof(*colors) << 2);

	unsigned w = ximage->width;
	unsigned h = ximage->height;
	unsigned i, j, k, l, m, n;

	double pppx = (double) w / (double) nw;
	double pppy = (double) h / (double) nh;
	double ty, by, lx, rx, xf, yf, ff;
	unsigned long A, R, G, B, pixel;

	for (ty = 0.0, by = pppy, l = 0; l < nh; l++, ty = by, by = (l + 1) *pppy) {

		for (j = floor(ty); j < by; j++) {

			if (ty < (j + 1) && (j + 1) < by)
				yf = (j + 1) - ty;
			else if (ty < j && j < by)
				yf = by - j;
			else
				yf = 1.0;

			for (lx = 0.0, rx = pppx, k = 0; k < nw; k++, lx = rx, rx = (k + 1) * pppx) {

				for (i = floor(lx); i < rx; i++) {

					if (lx < (i + 1) && (i + 1) < rx)
						xf = (i + 1) - lx;
					else if (lx < i && i < rx)
						xf = rx - i;
					else
						xf = 1.0;

					ff = xf * yf;

					m = l * nw + k;
					n = m << 2;
					counts[m] += ff;
					pixel = XGetPixel(ximage, i, j);
					if (bitmap && (pixel & 0x00ffffff))
						pixel |= 0x00ffffff;
					A = (pixel >> 24) & 0xff;
					R = (pixel >> 16) & 0xff;
					G = (pixel >> 8) & 0xff;
					B = (pixel >> 0) & 0xff;
					colors[n + 0] += A * ff;
					colors[n + 1] += R * ff;
					colors[n + 2] += G * ff;
					colors[n + 3] += B * ff;
				}
			}
		}
	}
	unsigned long amax = 0;

	for (j = 0; j < nh; j++) {
		for (i = 0; i < nw; i++) {
			m = j * nw + i;
			n = m << 2;
			if (counts[m]) {
				pixel = ((A = min(255, lround(colors[n + 0] / counts[m]))) << 24) |
					((R = min(255, lround(colors[n + 1] / counts[m]))) << 16) |
					((G = min(255, lround(colors[n + 2] / counts[m]))) <<  8) |
					((B = min(255, lround(colors[n + 3] / counts[m]))) <<  0);
				XPutPixel(xscale, i, j, pixel);
				amax = max(amax, A);
			}
		}
	}
	/* no opacity, add some */
	if (!amax)
		for (j = 0; j < nh; j++)
			for (i = 0; i < nw; i++)
				XPutPixel(xscale, i, j, XGetPixel(xscale, i, j) | 0xff000000);
#ifdef ALPHAMAX
	else if (amax < 255UL) {
		double bump = (double) 255 / (double) amax;

		for (j = 0; j < nh; j++) {
			for (i = 0; i < nw; i++) {
				pixel = XGetPixel(xscale, i, j);
				A = (pixel >> 24) & 0xff;
				A = min(255, lround(A * bump));
				pixel = (pixel & 0x00ffffff) | (A << 24);
				XPutPixel(xscale, i, j, pixel);
			}
		}
	}
#endif
	free(counts);
	free(colors);
	XDestroyImage(ximage);
	return (xscale);
}

/* upscaling width, downscaling height */
static XImage *
wh_scale_image(AScreen *ds, XImage *ximage, unsigned nw, unsigned nh, Bool bitmap)
{
	XImage *xscale = NULL;
	size_t elements = nw * nh;

	xscale = XCreateImage(dpy, ds->visual, 32, ZPixmap, 0, NULL, nw, nh, 8, 0);
	if (!xscale)
		return(ximage);
	xscale->data = ecalloc(xscale->bytes_per_line, xscale->height);
	if (!xscale->data) {
		XDestroyImage(xscale);
		return (ximage);
	}
	double *counts = ecalloc(elements, sizeof(*counts));
	double *colors = ecalloc(elements, sizeof(*colors) << 2);

	unsigned w = ximage->width;
	unsigned h = ximage->height;
	unsigned i, j, k, l, m, n;

	double pppx = (double) w / (double) nw;
	double pppy = (double) nh / (double) h;
	double ty, by, lx, rx, xf, yf, ff;
	unsigned long A, R, G, B, pixel;

	for (ty = 0.0, by = pppy, l = 0; l < h; l++, ty = by, by = (l + 1) * pppy) {

		for (j = floor(ty); j < by; j++) {

			if (ty < (j + 1) && (j + 1) < by)
				yf = (j + 1) - ty;
			else if (ty < j && j < by)
				yf = by - j;
			else
				yf = 1.0;

			for (lx = 0.0, rx = pppx, k = 0; k < nw; k++, lx = rx, rx = (k + 1) * pppx) {

				for (i = floor(lx); i < rx; i++) {

					if (lx < (i + 1) && (i + 1) < rx)
						xf = (i + 1) - lx;
					else if (lx < i && i < rx)
						xf = rx - i;
					else
						xf = 1.0;

					ff = xf * yf;

					m = l * nw + k;
					n = m << 2;
					counts[m] += ff;
					pixel = XGetPixel(ximage, i, j);
					if (bitmap && (pixel & 0x00ffffff))
						pixel |= 0x00ffffff;
					A = (pixel >> 24) & 0xff;
					R = (pixel >> 16) & 0xff;
					G = (pixel >> 8) & 0xff;
					B = (pixel >> 0) & 0xff;
					colors[n + 0] += A * ff;
					colors[n + 1] += R * ff;
					colors[n + 2] += G * ff;
					colors[n + 3] += B * ff;
				}
			}
		}
	}
	unsigned long amax = 0;

	for (j = 0; j < nh; j++) {
		for (i = 0; i < nw; i++) {
			m = j * nw + i;
			n = m << 2;
			if (counts[m]) {
				pixel = ((A = min(255, lround(colors[n + 0] / counts[m]))) << 24) |
					((R = min(255, lround(colors[n + 1] / counts[m]))) << 16) |
					((G = min(255, lround(colors[n + 2] / counts[m]))) <<  8) |
					((B = min(255, lround(colors[n + 3] / counts[m]))) <<  0);
				XPutPixel(xscale, i, j, pixel);
				amax = max(amax, A);
			}
		}
	}
	/* no opacity, add some */
	if (!amax)
		for (j = 0; j < nh; j++)
			for (i = 0; i < nw; i++)
				XPutPixel(xscale, i, j, XGetPixel(xscale, i, j) | 0xff000000);
#ifdef ALPHAMAX
	else if (amax < 255UL) {
		double bump = (double) 255 / (double) amax;

		for (j = 0; j < nh; j++) {
			for (i = 0; i < nw; i++) {
				pixel = XGetPixel(xscale, i, j);
				A = (pixel >> 24) & 0xff;
				A = min(255, lround(A * bump));
				pixel = (pixel & 0x00ffffff) | (A << 24);
				XPutPixel(xscale, i, j, pixel);
			}
		}
	}
#endif
	free(counts);
	free(colors);
	XDestroyImage(ximage);
	return (xscale);
}

/* downscaling width, upscaling height */
static XImage *
hw_scale_image(AScreen *ds, XImage *ximage, unsigned nw, unsigned nh, Bool bitmap)
{
	XImage *xscale = NULL;
	size_t elements = nw * nh;

	xscale = XCreateImage(dpy, ds->visual, 32, ZPixmap, 0, NULL, nw, nh, 8, 0);
	if (!xscale)
		return(ximage);
	xscale->data = ecalloc(xscale->bytes_per_line, xscale->height);
	if (!xscale->data) {
		XDestroyImage(xscale);
		return (ximage);
	}
	double *counts = ecalloc(elements, sizeof(*counts));
	double *colors = ecalloc(elements, sizeof(*colors) << 2);

	unsigned w = ximage->width;
	unsigned h = ximage->height;
	unsigned i, j, k, l, m, n;

	double pppx = (double) nw / (double) w;
	double pppy = (double) h / (double) nh;
	double ty, by, lx, rx, xf, yf, ff;
	unsigned long A, R, G, B, pixel;

	for (ty = 0.0, by = pppy, l = 0; l < nh; l++, ty = by, by = (l + 1) *pppy) {

		for (j = floor(ty); j < by; j++) {

			if (ty < (j + 1) && (j + 1) < by)
				yf = (j + 1) - ty;
			else if (ty < j && j < by)
				yf = by - j;
			else
				yf = 1.0;

			for (lx = 0.0, rx = pppx, k = 0; k < w; k++, lx = rx, rx = (k + 1) * pppx) {

				for (i = floor(lx); i < rx; i++) {

					if (lx < (i + 1) && (i + 1) < rx)
						xf = (i + 1) - lx;
					else if (lx < i && i < rx)
						xf = rx - i;
					else
						xf = 1.0;

					ff = xf * yf;

					m = j * nw + i;
					n = m << 2;
					counts[m] += ff;
					pixel = XGetPixel(ximage, i, j);
					if (bitmap && (pixel & 0x00ffffff))
						pixel |= 0x00ffffff;
					A = (pixel >> 24) & 0xff;
					R = (pixel >> 16) & 0xff;
					G = (pixel >> 8) & 0xff;
					B = (pixel >> 0) & 0xff;
					colors[n + 0] += A * ff;
					colors[n + 1] += R * ff;
					colors[n + 2] += G * ff;
					colors[n + 3] += B * ff;
				}
			}
		}
	}
	unsigned long amax = 0;

	for (j = 0; j < nh; j++) {
		for (i = 0; i < nw; i++) {
			m = j * nw + i;
			n = m << 2;
			if (counts[m]) {
				pixel = ((A = min(255, lround(colors[n + 0] / counts[m]))) << 24) |
					((R = min(255, lround(colors[n + 1] / counts[m]))) << 16) |
					((G = min(255, lround(colors[n + 2] / counts[m]))) <<  8) |
					((B = min(255, lround(colors[n + 3] / counts[m]))) <<  0);
				XPutPixel(xscale, i, j, pixel);
				amax = max(amax, A);
			}
		}
	}
	/* no opacity, add some */
	if (!amax)
		for (j = 0; j < nh; j++)
			for (i = 0; i < nw; i++)
				XPutPixel(xscale, i, j, XGetPixel(xscale, i, j) | 0xff000000);
#ifdef ALPHAMAX
	else if (amax < 255UL) {
		double bump = (double) 255 / (double) amax;

		for (j = 0; j < nh; j++) {
			for (i = 0; i < nw; i++) {
				pixel = XGetPixel(xscale, i, j);
				A = (pixel >> 24) & 0xff;
				A = min(255, lround(A * bump));
				pixel = (pixel & 0x00ffffff) | (A << 24);
				XPutPixel(xscale, i, j, pixel);
			}
		}
	}
#endif
	free(counts);
	free(colors);
	XDestroyImage(ximage);
	return (xscale);
}

/* crop ARGB image down to non-transparent extents, consuming passed image */
XImage *
crop_image(AScreen *ds, XImage *ximage)
{
	XImage *xcrop;
	int i, j;
	XRectangle r = { 0, };
	Bool opacity = False;
	int xmin = ximage->width;
	int ymin = ximage->height;;
	int xmax = 0;
	int ymax = 0;

	(void) ds;	/* XXX */

	for (j = 0; j < ximage->height; j++) {
		for (i = 0; i < ximage->width; i++) {
			if (XGetPixel(ximage, i, j) & 0xff000000UL) {
				xmin = min(xmin, i);
				xmax = max(xmax, i);
				ymin = min(ymin, j);
				ymax = max(ymax, j);
				opacity = True;
			}
		}
	}
	if (opacity) {
		assert(xmin <= xmax && ymin <= ymax);
		r.x = (int)xmin;
		r.y = (int)ymin;
		r.width = (int)xmax + 1 - (int)xmin;
		r.height = (int)ymax + 1 - (int)ymin;
	} else {
		XPRINTF("data icon has no alpha!\n");
		for (j = 0; j < ximage->height; j++)
			for (i = 0; i < ximage->width; i++)
				XPutPixel(ximage, i, j, XGetPixel(ximage, i, j) | 0xff000000);
		return (ximage);
	}
	xcrop = XSubImage(ximage, r.x, r.y, r.width, r.height);
	if (xcrop) {
		XPRINTF("cropped image %ux%ux%u to %ux%u+%d+%d\n",
				ximage->width, ximage->height, ximage->depth,
				r.width, r.height, r.x, r.y);
		XDestroyImage(ximage);
		return (xcrop);
	}
	return (ximage);
}

/* scale image, consuming passed in image */
XImage *
scale_image(AScreen *ds, XImage *ximage, unsigned nw, unsigned nh, Bool bitmap, Bool crop)
{
	unsigned w = ximage->width;
	unsigned h = ximage->height;

	if (crop)
		ximage = crop_image(ds, ximage);

	if (nw == w && nh == h) {
		return (ximage);
	} else if (nw <= w && nh <= h) {
		return dn_scale_image(ds, ximage, nw, nh, bitmap);
	} else if (nw >= w && nh >= h) {
		return up_scale_image(ds, ximage, nw, nh, bitmap);
	} else if (nw >= w && nh <= h) {
		return wh_scale_image(ds, ximage, nw, nh, bitmap);
	} else if (nw <= w && nh >= h) {
		return hw_scale_image(ds, ximage, nw, nh, bitmap);
	} else {
		/* can't happen */
		return (NULL);
	}
}

#if 0
static const unsigned char dither4[4][4] = {
	{0, 4, 1, 5},
	{6, 2, 7, 3},
	{1, 5, 0, 4},
	{7, 3, 6, 2}
};

static const unsigned char dither8[8][8] = {
	{0, 32, 8, 40, 2, 34, 10, 42},
	{48, 16, 56, 24, 50, 18, 58, 26},
	{12, 44, 4, 36, 14, 46, 6, 38},
	{60, 28, 52, 20, 62, 30, 54, 22},
	{3, 35, 11, 43, 1, 33, 9, 41},
	{51, 19, 59, 27, 49, 17, 57, 25},
	{15, 47, 7, 39, 13, 45, 5, 37},
	{63, 31, 55, 23, 61, 29, 53, 21}
};
#endif

void
renderimage(AScreen *ds, const ARGB *argb, const unsigned width, const unsigned height)
{
	XImage *image;
	unsigned long r, g, b, pixel;
	unsigned char *p, *pp, *data;
	unsigned x, y;
	const ARGB *c;

	image = XCreateImage(dpy, ds->visual, ds->bpp, ZPixmap, 0, NULL, width, height, 32, 0);
	if (!image) {
		XPRINTF("Could not create image\n");
		return;
	}
	image->data = NULL;
	p = pp = data = calloc(image->bytes_per_line * (height + 1), sizeof(*data));

	switch (ds->visual->class) {
	case StaticColor:
	case PseudoColor:
		for (c = argb, y = 0; y < height; y++) {
			for (x = 0; x < width; x++, c++) {
				r = ds->rctab[c->red];
				g = ds->gctab[c->green];
				b = ds->bctab[c->blue];

				pixel = (r * ds->cpc * ds->cpc) + (g * ds->cpc) + b;
				*p++ = ds->colors[pixel].pixel;
			}
			p = (pp += image->bytes_per_line);
		}
		break;
	case TrueColor:
		for (c = argb, y = 0; y < height; y++) {
			for (x = 0; x < width; x++, c++) {
				r = ds->rctab[c->red];
				g = ds->gctab[c->green];
				b = ds->bctab[c->blue];

				pixel = (((r << ds->visual->bits_per_rgb) | g) << ds->visual->bits_per_rgb) | b;
				switch (ds->bpp + ((image->byte_order == MSBFirst) ? 1 : 0)) {
				case 8:	/* 8bpp */
					*p++ = pixel;
					break;
				case 16:	/* 16bpp LSB */
					*p++ = pixel;
					*p++ = pixel >> 8;
					break;
				case 17:	/* 16bpp MSB */
					*p++ = pixel >> 8;
					*p++ = pixel;
					break;
				case 24:	/* 24bpp LSB */
					*p++ = pixel;
					*p++ = pixel >> 8;
					*p++ = pixel >> 16;
					break;
				case 25:	/* 24bpp MSB */
					*p++ = pixel >> 16;
					*p++ = pixel >> 8;
					*p++ = pixel;
					break;
				case 32:	/* 32bpp LSB */
					*p++ = pixel;
					*p++ = pixel >> 8;
					*p++ = pixel >> 16;
					*p++ = pixel >> 24;
					break;
				case 33:	/* 32bpp MSB */
					*p++ = pixel >> 24;
					*p++ = pixel >> 16;
					*p++ = pixel >> 8;
					*p++ = pixel;
					break;
				}
			}
			p = (pp += image->bytes_per_line);
		}
		break;
	case StaticGray:
	case GrayScale:
		for (c = argb, y = 0; y < height; y++) {
			for (x = 0; x < width; x++, c++) {
				r = ds->rctab[c->red];
				g = ds->gctab[c->green];
				b = ds->bctab[c->blue];

				g = ((r * 30) + (g * 59) + (b * 11)) / 100;
				*p++ = ds->colors[g].pixel;
			}
			p = (pp += image->bytes_per_line);
		}
		break;
	default:
		XPRINTF("Unsupported visual class\n");
		free(data);
		XDestroyImage(image);
		return;
	}
	image->data = (char *) data;
}

void
initimage(void)
{
	int i, count, nvi;
	XPixmapFormatValues *pmv;
	XRenderPictFormat *format;
	XVisualInfo *xvi, templ = { .screen = scr->screen, .depth = 32, .class = TrueColor, };
	Visual *visual = NULL;

	if ((xvi = XGetVisualInfo(dpy, VisualScreenMask|VisualDepthMask|VisualClassMask, &templ, &nvi))) {
		for (i = 0; i < nvi; i++) {
			format = XRenderFindVisualFormat(dpy, xvi[i].visual);
			if (format->type == PictTypeDirect && format->direct.alphaMask) {
				visual = xvi[i].visual;
				break;
			}
		}
	}
	XFree(xvi);

	xtrap_push(0,NULL);
	if (visual) {
		XSetWindowAttributes wa = { 0, };
		unsigned long mask = 0;

		scr->depth = 32;
		scr->visual = visual;
		scr->colormap = XCreateColormap(dpy, scr->root, visual, AllocNone);
		scr->format = XRenderFindStandardFormat(dpy, PictStandardARGB32);

		/* make a window with proper depth for creating GCs */
		wa.override_redirect = True;
		mask |= CWOverrideRedirect;
		wa.colormap = scr->colormap;
		mask |= CWColormap;
		/* cannot inherit pixels but can set them to anything */
		wa.background_pixel = BlackPixel(dpy, scr->screen);
		mask |= CWBackPixel;
		wa.border_pixel = BlackPixel(dpy, scr->screen);
		mask |= CWBorderPixel;
		wa.background_pixmap = None;
		mask |= CWBackPixmap;
		scr->drawable = XCreateWindow(dpy, scr->root, 0, 0, 1, 1, 0,
				scr->depth, InputOutput, scr->visual,
				mask, &wa);
	} else {
		scr->depth = DefaultDepth(dpy, scr->screen);
		scr->visual = DefaultVisual(dpy, scr->screen);
		scr->colormap = DefaultColormap(dpy, scr->screen);
		scr->format = XRenderFindVisualFormat(dpy, scr->visual);
		scr->drawable = scr->root;
	}
	xtrap_pop();
#ifdef PIXBUF
	gdk_pixbuf_xlib_init_with_depth(dpy, scr->screen, scr->depth);
#endif
#ifdef IMLIB2
	scr->context = imlib_context_new();
	imlib_context_push(scr->context);
	imlib_context_set_display(dpy);
	imlib_context_set_drawable(None);
	imlib_context_set_visual(scr->visual);
	imlib_context_set_colormap(scr->colormap);
	imlib_context_set_drawable(scr->drawable);
	imlib_context_set_dither_mask(1);
	imlib_context_set_anti_alias(1);
	imlib_context_set_dither(1);
	imlib_context_set_blend(1);
	imlib_context_set_mask(None);
	imlib_context_pop();

#if 0
	scr->rootctx = imlib_context_new();
	imlib_context_push(scr->rootctx);
	imlib_context_set_display(dpy);
	imlib_context_set_drawable(None);
	imlib_context_set_visual(DefaultVisual(dpy, scr->screen));
	imlib_context_set_colormap(DefaultColormap(dpy, scr->screen));
	imlib_context_set_drawable(scr->root);
	imlib_context_set_dither_mask(1);
	imlib_context_set_anti_alias(1);
	imlib_context_set_dither(1);
	imlib_context_set_blend(1);
	imlib_context_set_mask(None);
	imlib_context_pop();
#endif
#endif
	scr->dither = True;
	scr->bpp = 0;
#if 0
	scr->cpc = options.colorsPerChannel;
#else
	scr->cpc = (1<<16);
#endif

	free(scr->rctab);
	free(scr->gctab);
	free(scr->bctab);

	scr->rctab = ecalloc(256, sizeof(*scr->rctab));
	scr->gctab = ecalloc(256, sizeof(*scr->gctab));
	scr->bctab = ecalloc(256, sizeof(*scr->bctab));

	if ((pmv = XListPixmapFormats(dpy, &count))) {
		for (i = 0; i < count; i++) {
			if (pmv[i].depth == (int) scr->depth) {
				scr->bpp = pmv[i].bits_per_pixel;
				break;
			}
		}
		XFree(pmv);
	}
	if (scr->bpp == 0)
		scr->bpp = scr->depth;
	if (scr->bpp >= 24)
		scr->dither = False;

	switch (scr->visual->class) {
	case TrueColor:{
		unsigned long rmask = scr->visual->red_mask;
		unsigned long gmask = scr->visual->green_mask;
		unsigned long bmask = scr->visual->blue_mask;

		int roff = 0, rbits;
		int goff = 0, gbits;
		int boff = 0, bbits;

		for (; !(rmask & 0x1); rmask >>= 1, roff++) ;
		for (; !(gmask & 0x1); gmask >>= 1, goff++) ;
		for (; !(bmask & 0x1); bmask >>= 1, boff++) ;

		rbits = 255 / rmask;
		gbits = 255 / gmask;
		bbits = 255 / bmask;

		for (i = 0; i < 256; i++) {
			scr->rctab[i] = i / rbits;
			scr->gctab[i] = i / gbits;
			scr->bctab[i] = i / bbits;
		}

		break;
	}
	case PseudoColor:
	case StaticColor:{
		XColor icolors[256];
		int incolors;
		int rbits;
		int gbits;
		int bbits;
		int i, ii, p, r, g, b, bits;

		scr->ncolors = scr->cpc * scr->cpc * scr->cpc;
		if (scr->ncolors > (1 << scr->depth)) {
			scr->cpc = (1 << scr->depth) / 3;
			scr->ncolors = scr->cpc * scr->cpc * scr->cpc;
		}
		if (scr->cpc < 2 || scr->ncolors > (1 << scr->depth)) {
			XPRINTF("invalid color map size\n");
			scr->cpc = (1 << scr->depth) / 3;
		}
		scr->colors = calloc(scr->ncolors, sizeof(*scr->colors));
#ifdef ORDEREDPSEUDO
		bits = 256 / scr->cpc;
#else				/* ORDEREDPSEUDO */
		bits = 256 / (scr->cpc - 1);
#endif				/* ORDEREDPSEUDO */
		rbits = gbits = bbits = bits;

		for (i = 0; i < 256; i++) {
			scr->rctab[i] = i / rbits;
			scr->gctab[i] = i / gbits;
			scr->bctab[i] = i / bbits;
		}
		for (r = 0, i = 0; r < scr->cpc; r++) {
			for (g = 0; g < scr->cpc; g++) {
				for (b = 0; b < scr->cpc; b++) {
					scr->colors[i].red = (r * 0xffff) / (scr->cpc - 1);
					scr->colors[i].green = (g * 0xffff) / (scr->cpc - 1);
					scr->colors[i].blue = (b * 0xffff) / (scr->cpc - 1);
					scr->colors[i].flags = DoRed | DoGreen | DoBlue;
				}
			}
		}
		XGrabServer(dpy);
		{
			for (i = 0; i < scr->ncolors; i++) {
				if (!XAllocColor(dpy, scr->colormap, &scr->colors[i])) {
					XPRINTF("could not allocate color\n");
					scr->colors[i].flags = 0;
				} else
					scr->colors[i].flags = DoRed | DoGreen | DoBlue;
			}
		}
		XUngrabServer(dpy);

		incolors = ((1 << scr->depth) > 256) ? 256 : (1 << scr->depth);

		for (i = 0; i < incolors; i++)
			icolors[i].pixel = i;

		XQueryColors(dpy, scr->colormap, icolors, incolors);
		for (i = 0; i < scr->ncolors; i++) {
			if (!scr->colors[i].flags) {
				unsigned long chk = 0xffffffff, pixel, close = 0;

				p = 2;
				while (p--) {
					for (ii = 0; ii < incolors; i++) {
						r = (scr->colors[i].red - icolors[i].red) >> 8;
						g = (scr->colors[i].green - icolors[i].green) >> 8;
						b = (scr->colors[i].blue - icolors[i].blue) >> 8;
						pixel = (r * r) + (g * g) + (b * b);

						if (pixel < chk) {
							chk = pixel;
							close = ii;
						}

						scr->colors[i].red = icolors[close].red;
						scr->colors[i].green = icolors[close].green;
						scr->colors[i].blue = icolors[close].blue;

						if (XAllocColor(dpy, scr->colormap, &scr->colors[i])) {
							scr->colors[i].flags = DoRed | DoGreen | DoBlue;
							break;
						}
					}
				}
			}
		}
		break;
	}
	case GrayScale:
	case StaticGray:{
		int i, ii, p, bits;
		XColor icolors[256];
		int incolors;

		int rbits;
		int gbits;
		int bbits;

		if (scr->visual->class == StaticGray) {
			scr->ncolors = 1 << scr->depth;
		} else {
			scr->ncolors = scr->cpc * scr->cpc * scr->cpc;

			if (scr->ncolors > (1 << scr->depth)) {
				scr->cpc = (1 << scr->depth) / 3;
				scr->ncolors = scr->cpc * scr->cpc * scr->cpc;
			}
		}
		if (scr->cpc < 2 || scr->ncolors > (1 << scr->depth)) {
			XPRINTF("invalid colormap size\n");
			scr->cpc = (1 << scr->depth) / 3;
		}
		scr->colors = ecalloc(scr->ncolors, sizeof(*scr->colors));
		bits = 255 / (scr->cpc - 1);

		rbits = gbits = bbits = bits;

		for (i = 0; i < 256; i++) {
			scr->rctab[i] = i / rbits;
			scr->gctab[i] = i / gbits;
			scr->bctab[i] = i / bbits;
		}

		XGrabServer(dpy);
		{
			for (i = 0; i < scr->ncolors; i++) {
				scr->colors[i].red = (i * 0xffff) / (scr->cpc - 1);
				scr->colors[i].green = (i * 0xffff) / (scr->cpc - 1);
				scr->colors[i].blue = (i * 0xffff) / (scr->cpc - 1);
				scr->colors[i].flags = DoRed | DoGreen | DoBlue;
			}
			if (!XAllocColor(dpy, scr->colormap, &scr->colors[i])) {
				XPRINTF("could not allocate color\n");
				scr->colors[i].flags = 0;
			} else
				scr->colors[i].flags = DoRed | DoGreen | DoBlue;
		}
		XUngrabServer(dpy);
		incolors = ((1 << scr->depth) > 256) ? 256 : (1 << scr->depth);

		for (i = 0; i < incolors; i++)
			icolors[i].pixel = i;

		XQueryColors(dpy, scr->colormap, icolors, incolors);
		for (i = 0; i < scr->ncolors; i++) {
			if (!scr->colors[i].flags) {
				unsigned long chk = 0xffffffff, pixel, close = 0;

				p = 2;
				while (p--) {
					for (ii = 0; ii < incolors; ii++) {
						int r = (scr->colors[i].red - icolors[i].red) >> 8;
						int g = (scr->colors[i].green - icolors[i].green) >> 8;
						int b = (scr->colors[i].blue - icolors[i].blue) >> 8;

						pixel = (r * r) + (g * g) + (b * b);
						if (pixel < chk) {
							chk = pixel;
							close = ii;
						}
						scr->colors[i].red = icolors[close].red;
						scr->colors[i].green = icolors[close].green;
						scr->colors[i].blue = icolors[close].blue;

						if (XAllocColor(dpy, scr->colormap, &scr->colors[i])) {
							scr->colors[i].flags = DoRed | DoGreen | DoBlue;
							break;
						}
					}
				}
			}
		}
		break;
	}
	default:
		eprint("Unsuppoted visual class\n");
		break;
	}
}

