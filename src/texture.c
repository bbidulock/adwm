/* See COPYING file for copyright and license details. */
#include <regex.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xft/Xft.h>
#include "adwm.h"
#include "config.h"
#ifdef IMLIB2
#include <Imlib2.h>
#endif
#ifdef XPM
#include <X11/xpm.h>
#endif

void dgradient(Texture *t, unsigned width, unsigned height, ARGB *data);
void egradient(Texture *t, unsigned width, unsigned height, ARGB *data);
void hgradient(Texture *t, unsigned width, unsigned height, ARGB *data);
void pgradient(Texture *t, unsigned width, unsigned height, ARGB *data);
void rgradient(Texture *t, unsigned width, unsigned height, ARGB *data);
void vgradient(Texture *t, XColor color, XColor colorTo, unsigned width,
	       unsigned height, ARGB *data, unsigned fromHeight, unsigned toHeight);
void cdgradient(Texture *t, unsigned width, unsigned height, ARGB *data);
void pcgradient(Texture *t, unsigned width, unsigned height, ARGB *data);
void svgradient(Texture *t, unsigned width, unsigned height, ARGB *data);
void raisedBevel(Texture *t, unsigned width, unsigned height, ARGB *data);
void sunkenBevel(Texture *t, unsigned width, unsigned height, ARGB *data);

/*
 * Notes: These gradient functions were pulled directly across from blackbox(1)
 * and are present in one form or another also in fluxbox(1), openbox(1) and
 * waimea(1).  wmaker(1) and icewm(1) also do this kind of (mostly linear)
 * gradient.  Strangely, many of the window managers use imlib2, but none of
 * them use the gradient features of imlib2, and imblib2 is mostly used by these
 * just to load png and xpm files and generate pixmaps from them.
 */

void
dgradient(Texture *t, unsigned width, unsigned height, ARGB *data)
{
	float drx, dgx, dbx, dry, dgy, dby;
	float yr = 0.0, yg = 0.0, yb = 0.0;
	float xr = t->color.red;
	float xg = t->color.green;
	float xb = t->color.blue;

	unsigned w = width * 2, h = height * 2;
	unsigned x, y;

	unsigned *xt[3], *yt[3];
	const unsigned dimension = max(width, height);
	unsigned *alloc = calloc(dimension * 6, sizeof(*alloc));

	ARGB *p = data;

	xt[0] = alloc + (dimension * 0);
	xt[1] = alloc + (dimension * 1);
	xt[2] = alloc + (dimension * 2);
	yt[0] = alloc + (dimension * 3);
	yt[1] = alloc + (dimension * 4);
	yt[2] = alloc + (dimension * 5);

	dry = drx = t->colorTo.red - t->color.red;
	dgy = dgx = t->colorTo.green - t->color.green;
	dby = dbx = t->colorTo.blue - t->color.blue;

	// Create X table
	drx /= w;
	dgx /= w;
	dbx /= w;

	for (x = 0; x < width; ++x) {
		xt[0][x] = xr;
		xt[1][x] = xg;
		xt[2][x] = xb;

		xr += drx;
		xg += dgx;
		xb += dbx;
	}

	// Create Y table
	dry /= h;
	dgy /= h;
	dby /= h;

	for (y = 0; y < height; ++y) {
		yt[0][y] = yr;
		yt[1][y] = yg;
		yt[2][y] = yb;

		yr += dry;
		yg += dgy;
		yb += dby;
	}

	// Combine tables to create gradient

	if (!t->appearance.interlaced) {
		// normal dgradient
		for (y = 0; y < height; ++y) {
			for (x = 0; x < width; ++x, ++p) {
				p->red = xt[0][x] + yt[0][y];
				p->green = xt[1][x] + yt[1][y];
				p->blue = xt[2][x] + yt[2][y];
				p->alpha = 0xff;
			}
		}
	} else {
		// interlacing effect
		for (y = 0; y < height; ++y) {
			for (x = 0; x < width; ++x, ++p) {
				p->red = xt[0][x] + yt[0][y];
				p->green = xt[1][x] + yt[1][y];
				p->blue = xt[2][x] + yt[2][y];
				p->alpha = 0xff;

				if (y & 1) {
					p->red = (p->red >> 1) + (p->red >> 2);
					p->green = (p->green >> 1) + (p->green >> 2);
					p->blue = (p->blue >> 1) + (p->blue >> 2);
				}
			}
		}
	}
	free(alloc);
}

void
egradient(Texture *t, unsigned width, unsigned height, ARGB *data)
{
	// elliptic gradient - based on original dgradient, written by
	// Mosfet (mosfet@kde.org)
	// adapted from kde sources for Blackbox by Brad Hughes

	float drx, dgx, dbx, dry, dgy, dby, yr, yg, yb, xr, xg, xb;
	int rsign, gsign, bsign;
	unsigned tr = t->colorTo.red;
	unsigned tg = t->colorTo.green;
	unsigned tb = t->colorTo.blue;
	unsigned x, y;

	const unsigned dimension = max(width, height);
	unsigned *alloc = calloc(dimension * 6, sizeof(*alloc));
	unsigned *xt[3], *yt[3];

	ARGB *p = data;

	xt[0] = alloc + (dimension * 0);
	xt[1] = alloc + (dimension * 1);
	xt[2] = alloc + (dimension * 2);
	yt[0] = alloc + (dimension * 3);
	yt[1] = alloc + (dimension * 4);
	yt[2] = alloc + (dimension * 5);

	dry = drx = t->colorTo.red - t->color.red;
	dgy = dgx = t->colorTo.green - t->color.green;
	dby = dbx = t->colorTo.blue - t->color.blue;

	rsign = (drx < 0) ? -1 : 1;
	gsign = (dgx < 0) ? -1 : 1;
	bsign = (dbx < 0) ? -1 : 1;

	xr = yr = drx / 2;
	xg = yg = dgx / 2;
	xb = yb = dbx / 2;

	// Create X table
	drx /= width;
	dgx /= width;
	dbx /= width;

	for (x = 0; x < width; x++) {
		xt[0][x] = xr * xr;
		xt[1][x] = xg * xg;
		xt[2][x] = xb * xb;

		xr -= drx;
		xg -= dgx;
		xb -= dbx;
	}

	// Create Y table
	dry /= height;
	dgy /= height;
	dby /= height;

	for (y = 0; y < height; y++) {
		yt[0][y] = yr * yr;
		yt[1][y] = yg * yg;
		yt[2][y] = yb * yb;

		yr -= dry;
		yg -= dgy;
		yb -= dby;
	}

	// Combine tables to create gradient

	if (!t->appearance.interlaced) {
		// normal egradient
		for (y = 0; y < height; ++y) {
			for (x = 0; x < width; ++x, ++p) {
				p->red =
				    (tr - (rsign * (int) (sqrt(xt[0][x] + yt[0][y]))));
				p->green =
				    (tg - (gsign * (int) (sqrt(xt[1][x] + yt[1][y]))));
				p->blue =
				    (tb - (bsign * (int) (sqrt(xt[2][x] + yt[2][y]))));
				p->alpha = 0xff;
			}
		}
	} else {
		// interlacing effect
		for (y = 0; y < height; ++y) {
			for (x = 0; x < width; ++x, ++p) {
				p->red =
				    (tr - (rsign * (int) (sqrt(xt[0][x] + yt[0][y]))));
				p->green =
				    (tg - (gsign * (int) (sqrt(xt[1][x] + yt[1][y]))));
				p->blue =
				    (tb - (bsign * (int) (sqrt(xt[2][x] + yt[2][y]))));
				p->alpha = 0xff;

				if (y & 1) {
					p->red = (p->red >> 1) + (p->red >> 2);
					p->green = (p->green >> 1) + (p->green >> 2);
					p->blue = (p->blue >> 1) + (p->blue >> 2);
				}
			}
		}
	}
	free(alloc);
}

void
hgradient(Texture *t, unsigned width, unsigned height, ARGB *data)
{
	float drx, dgx, dbx;
	float xr = t->color.red;
	float xg = t->color.green;
	float xb = t->color.blue;
	unsigned total = width * (height - 2);
	unsigned x;

	ARGB *p = data;

	drx = t->colorTo.red - t->color.red;
	dgx = t->colorTo.green - t->color.green;
	dbx = t->colorTo.blue - t->color.blue;

	drx /= width;
	dgx /= width;
	dbx /= width;

	if (t->appearance.interlaced && height > 1) {
		// interlacing effect

		// first line
		for (x = 0; x < width; ++x, ++p) {
			p->red = xr;
			p->green = xg;
			p->blue = xb;
			p->alpha = 0xff;

			xr += drx;
			xg += dgx;
			xb += dbx;
		}

		// second line
		xr = t->color.red;
		xg = t->color.green;
		xb = t->color.blue;

		for (x = 0; x < width; ++x, ++p) {
			p->red = xr;
			p->green = xg;
			p->blue = xb;
			p->alpha = 0xff;

			p->red = (p->red >> 1) + (p->red >> 2);
			p->green = (p->green >> 1) + (p->green >> 2);
			p->blue = (p->blue >> 1) + (p->blue >> 2);

			xr += drx;
			xg += dgx;
			xb += dbx;
		}
	} else {
		// first line
		for (x = 0; x < width; ++x, ++p) {
			p->red = xr;
			p->green = xg;
			p->blue = xb;
			p->alpha = 0xff;

			xr += drx;
			xg += dgx;
			xb += dbx;
		}

		if (height > 1) {
			// second line
			memcpy(p, data, width * sizeof(*p));
			p += width;
		}
	}

	if (height > 2) {
		// rest of the gradient
		for (x = 0; x < total; ++x)
			p[x] = data[x];
	}
}

void
pgradient(Texture *t, unsigned width, unsigned height, ARGB *data)
{
	// pyramid gradient - based on original dgradient, written by
	// Mosfet (mosfet@kde.org)
	// adapted from kde sources for Blackbox by Brad Hughes

	float yr, yg, yb, drx, dgx, dbx, dry, dgy, dby, xr, xg, xb;
	int rsign, gsign, bsign;
	unsigned tr = t->colorTo.red;
	unsigned tg = t->colorTo.green;
	unsigned tb = t->colorTo.blue;
	unsigned x, y;

	const unsigned dimension = max(width, height);
	unsigned *alloc = calloc(dimension * 6, sizeof(*alloc));
	unsigned *xt[3], *yt[3];

	ARGB *p = data;

	xt[0] = alloc + (dimension * 0);
	xt[1] = alloc + (dimension * 1);
	xt[2] = alloc + (dimension * 2);
	yt[0] = alloc + (dimension * 3);
	yt[1] = alloc + (dimension * 4);
	yt[2] = alloc + (dimension * 5);

	dry = drx = t->colorTo.red - t->color.red;
	dgy = dgx = t->colorTo.green - t->color.green;
	dby = dbx = t->colorTo.blue - t->color.blue;

	rsign = (drx < 0) ? -1 : 1;
	gsign = (dgx < 0) ? -1 : 1;
	bsign = (dbx < 0) ? -1 : 1;

	xr = yr = drx / 2;
	xg = yg = dgx / 2;
	xb = yb = dbx / 2;

	// Create X table
	drx /= width;
	dgx /= width;
	dbx /= width;

	for (x = 0; x < width; ++x) {
		xt[0][x] = fabs(xr);
		xt[1][x] = fabs(xg);
		xt[2][x] = fabs(xb);

		xr -= drx;
		xg -= dgx;
		xb -= dbx;
	}

	// Create Y table
	dry /= height;
	dgy /= height;
	dby /= height;

	for (y = 0; y < height; ++y) {
		yt[0][y] = fabs(yr);
		yt[1][y] = fabs(yg);
		yt[2][y] = fabs(yb);

		yr -= dry;
		yg -= dgy;
		yb -= dby;
	}

	// Combine tables to create gradient

	if (!t->appearance.interlaced) {
		// normal pgradient
		for (y = 0; y < height; ++y) {
			for (x = 0; x < width; ++x, ++p) {
				p->red = tr - (rsign * (xt[0][x] + yt[0][y]));
				p->green = tg - (gsign * (xt[1][x] + yt[1][y]));
				p->blue = tb - (bsign * (xt[2][x] + yt[2][y]));
				p->alpha = 0xff;
			}
		}
	} else {
		// interlacing effect
		for (y = 0; y < height; ++y) {
			for (x = 0; x < width; ++x, ++p) {
				p->red = tr - (rsign * (xt[0][x] + yt[0][y]));
				p->green = tg - (gsign * (xt[1][x] + yt[1][y]));
				p->blue = tb - (bsign * (xt[2][x] + yt[2][y]));
				p->alpha = 0xff;

				if (y & 1) {
					p->red = (p->red >> 1) + (p->red >> 2);
					p->green = (p->green >> 1) + (p->green >> 2);
					p->blue = (p->blue >> 1) + (p->blue >> 2);
				}
			}
		}
	}
	free(alloc);
}

void
rgradient(Texture *t, unsigned width, unsigned height, ARGB *data)
{
	// rectangle gradient - based on original dgradient, written by
	// Mosfet (mosfet@kde.org)
	// adapted from kde sources for Blackbox by Brad Hughes

	float drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
	int rsign, gsign, bsign;
	unsigned tr = t->colorTo.red;
	unsigned tg = t->colorTo.green;
	unsigned tb = t->colorTo.blue;
	unsigned x, y;

	const unsigned dimension = max(width, height);
	unsigned *alloc = calloc(dimension * 6, sizeof(*alloc));
	unsigned *xt[3], *yt[3];

	ARGB *p = data;

	xt[0] = alloc + (dimension * 0);
	xt[1] = alloc + (dimension * 1);
	xt[2] = alloc + (dimension * 2);
	yt[0] = alloc + (dimension * 3);
	yt[1] = alloc + (dimension * 4);
	yt[2] = alloc + (dimension * 5);

	dry = drx = t->colorTo.red - t->color.red;
	dgy = dgx = t->colorTo.green - t->color.green;
	dby = dbx = t->colorTo.blue - t->color.blue;

	rsign = (drx < 0) ? -2 : 2;
	gsign = (dgx < 0) ? -2 : 2;
	bsign = (dbx < 0) ? -2 : 2;

	xr = yr = drx / 2;
	xg = yg = dgx / 2;
	xb = yb = dbx / 2;

	// Create X table
	drx /= width;
	dgx /= width;
	dbx /= width;

	for (x = 0; x < width; ++x) {
		xt[0][x] = fabs(xr);
		xt[1][x] = fabs(xg);
		xt[2][x] = fabs(xb);

		xr -= drx;
		xg -= dgx;
		xb -= dbx;
	}

	// Create Y table
	dry /= height;
	dgy /= height;
	dby /= height;

	for (y = 0; y < height; ++y) {
		yt[0][y] = fabs(yr);
		yt[1][y] = fabs(yg);
		yt[2][y] = fabs(yb);

		yr -= dry;
		yg -= dgy;
		yb -= dby;
	}

	// Combine tables to create gradient

	if (!t->appearance.interlaced) {
		// normal rgradient
		for (y = 0; y < height; ++y) {
			for (x = 0; x < width; ++x, ++p) {
				p->red = tr - (rsign * max(xt[0][x], yt[0][y]));
				p->green = tg - (gsign * max(xt[1][x], yt[1][y]));
				p->blue = tb - (bsign * max(xt[2][x], yt[2][y]));
				p->alpha = 0xff;
			}
		}
	} else {
		// interlacing effect
		for (y = 0; y < height; ++y) {
			for (x = 0; x < width; ++x, ++p) {
				p->red = tr - (rsign * max(xt[0][x], yt[0][y]));
				p->green = tg - (gsign * max(xt[1][x], yt[1][y]));
				p->blue = tb - (bsign * max(xt[2][x], yt[2][y]));
				p->alpha = 0xff;

				if (y & 1) {
					p->red = (p->red >> 1) + (p->red >> 2);
					p->green = (p->green >> 1) + (p->green >> 2);
					p->blue = (p->blue >> 1) + (p->blue >> 2);
				}
			}
		}
	}
	free(alloc);
}

void
vgradient(Texture *t, XColor color, XColor colorTo, unsigned width,
	  unsigned height, ARGB *data, unsigned fromHeight, unsigned toHeight)
{
	float yr = color.red;
	float yg = color.green;
	float yb = color.blue;

	float dry = colorTo.red - color.red;
	float dgy = colorTo.green - color.green;
	float dby = colorTo.blue - color.blue;

	unsigned deltaHeight = toHeight - fromHeight;
	unsigned x, y;

	ARGB *p = data + width * fromHeight;

	dry /= deltaHeight;
	dgy /= deltaHeight;
	dby /= deltaHeight;

	if (t->appearance.interlaced) {
		// faked interlacing effect
		for (y = fromHeight; y < toHeight; ++y) {
			const ARGB rgb = {
				.red = ((y & 1) ? (yr * 3. / 4.) : yr),
				.green = ((y & 1) ? (yg * 3. / 4.) : yg),
				.blue = ((y & 1) ? (yb * 3. / 4.) : yb),
				.alpha = 0xff,
			};
			for (x = 0; x < width; ++x, ++p)
				*p = rgb;

			yr += dry;
			yg += dgy;
			yb += dby;
		}
	} else {
		// normal vgradient
		for (y = fromHeight; y < toHeight; ++y) {
			const ARGB rgb = {
				.red = (yr),
				.green = (yg),
				.blue = (yb),
				.alpha = 0xff,
			};
			for (x = 0; x < width; ++x, ++p)
				*p = rgb;

			yr += dry;
			yg += dgy;
			yb += dby;
		}
	}
}

void
cdgradient(Texture *t, unsigned width, unsigned height, ARGB *data)
{
	// cross diagonal gradient - based on original dgradient, written by
	// Mosfet (mosfet@kde.org)
	// adapted from kde sources for Blackbox by Brad Hughes

	float drx, dgx, dbx, dry, dgy, dby;
	float yr = 0.0, yg = 0.0, yb = 0.0;
	float xr = t->color.red;
	float xg = t->color.green;
	float xb = t->color.blue;
	unsigned w = width * 2, h = height * 2;
	unsigned x, y;

	const unsigned dimension = max(width, height);
	unsigned *alloc = calloc(dimension * 6, sizeof(*alloc));
	unsigned *xt[3], *yt[3];

	ARGB *p = data;

	xt[0] = alloc + (dimension * 0);
	xt[1] = alloc + (dimension * 1);
	xt[2] = alloc + (dimension * 2);
	yt[0] = alloc + (dimension * 3);
	yt[1] = alloc + (dimension * 4);
	yt[2] = alloc + (dimension * 5);

	dry = drx = t->colorTo.red - t->color.red;
	dgy = dgx = t->colorTo.green - t->color.green;
	dby = dbx = t->colorTo.blue - t->color.blue;

	// Create X table
	drx /= w;
	dgx /= w;
	dbx /= w;

	for (x = width - 1; x != 0; --x) {
		xt[0][x] = xr;
		xt[1][x] = xg;
		xt[2][x] = xb;

		xr += drx;
		xg += dgx;
		xb += dbx;
	}

	xt[0][x] = xr;
	xt[1][x] = xg;
	xt[2][x] = xb;

	// Create Y table
	dry /= h;
	dgy /= h;
	dby /= h;

	for (y = 0; y < height; ++y) {
		yt[0][y] = yr;
		yt[1][y] = yg;
		yt[2][y] = yb;

		yr += dry;
		yg += dgy;
		yb += dby;
	}

	// Combine tables to create gradient

	if (!t->appearance.interlaced) {
		// normal dgradient
		for (y = 0; y < height; ++y) {
			for (x = 0; x < width; ++x, ++p) {
				p->red = xt[0][x] + yt[0][y];
				p->green = xt[1][x] + yt[1][y];
				p->blue = xt[2][x] + yt[2][y];
				p->alpha = 0xff;
			}
		}
	} else {
		// interlacing effect
		for (y = 0; y < height; ++y) {
			for (x = 0; x < width; ++x, ++p) {
				p->red = xt[0][x] + yt[0][y];
				p->green = xt[1][x] + yt[1][y];
				p->blue = xt[2][x] + yt[2][y];
				p->alpha = 0xff;

				if (y & 1) {
					p->red = (p->red >> 1) + (p->red >> 2);
					p->green = (p->green >> 1) + (p->green >> 2);
					p->blue = (p->blue >> 1) + (p->blue >> 2);
				}
			}
		}
	}
	free(alloc);
}

void
pcgradient(Texture *t, unsigned width, unsigned height, ARGB *data)
{
	// pipe cross gradient - based on original dgradient, written by
	// Mosfet (mosfet@kde.org)
	// adapted from kde sources for Blackbox by Brad Hughes

	float drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
	int rsign, gsign, bsign;
	unsigned tr = t->colorTo.red;
	unsigned tg = t->colorTo.green;
	unsigned tb = t->colorTo.blue;
	unsigned x, y;

	const unsigned dimension = max(width, height);
	unsigned *xt[3], *yt[3];

	unsigned *alloc = calloc(dimension * 6, sizeof(*alloc));

	ARGB *p = data;

	xt[0] = alloc + (dimension * 0);
	xt[1] = alloc + (dimension * 1);
	xt[2] = alloc + (dimension * 2);
	yt[0] = alloc + (dimension * 3);
	yt[1] = alloc + (dimension * 4);
	yt[2] = alloc + (dimension * 5);

	dry = drx = t->colorTo.red - t->color.red;
	dgy = dgx = t->colorTo.green - t->color.green;
	dby = dbx = t->colorTo.blue - t->color.blue;

	rsign = (drx < 0) ? -2 : 2;
	gsign = (dgx < 0) ? -2 : 2;
	bsign = (dbx < 0) ? -2 : 2;

	xr = yr = drx / 2;
	xg = yg = dgx / 2;
	xb = yb = dbx / 2;

	// Create X table
	drx /= width;
	dgx /= width;
	dbx /= width;

	for (x = 0; x < width; ++x) {
		xt[0][x] = fabs(xr);
		xt[1][x] = fabs(xg);
		xt[2][x] = fabs(xb);

		xr -= drx;
		xg -= dgx;
		xb -= dbx;
	}

	// Create Y table
	dry /= height;
	dgy /= height;
	dby /= height;

	for (y = 0; y < height; ++y) {
		yt[0][y] = fabs(yr);
		yt[1][y] = fabs(yg);
		yt[2][y] = fabs(yb);

		yr -= dry;
		yg -= dgy;
		yb -= dby;
	}

	// Combine tables to create gradient

	if (!t->appearance.interlaced) {
		// normal rgradient
		for (y = 0; y < height; ++y) {
			for (x = 0; x < width; ++x, ++p) {
				p->red = tr - (rsign * min(xt[0][x], yt[0][y]));
				p->green = tg - (gsign * min(xt[1][x], yt[1][y]));
				p->blue = tb - (bsign * min(xt[2][x], yt[2][y]));
				p->alpha = 0xff;
			}
		}
	} else {
		// interlacing effect
		for (y = 0; y < height; ++y) {
			for (x = 0; x < width; ++x, ++p) {
				p->red = tr - (rsign * min(xt[0][x], yt[0][y]));
				p->green = tg - (gsign * min(xt[1][x], yt[1][y]));
				p->blue = tb - (bsign * min(xt[2][x], yt[2][y]));
				p->alpha = 0xff;

				if (y & 1) {
					p->red = (p->red >> 1) + (p->red >> 2);
					p->green = (p->green >> 1) + (p->green >> 2);
					p->blue = (p->blue >> 1) + (p->blue >> 2);
				}
			}
		}
	}
	free(alloc);
}

/*
 * Adapted from a patch by David Barr, http://david.chalkskeletons.com
 * split grad: h1 -> from | to -> h2
 */

#define SAT_SHIFT(dest, input, shift) \
  dest = input; dest += input >> shift; if (dest > 0xff) dest = 0xff;

void
svgradient(Texture *t, unsigned width, unsigned height, ARGB *data)
{
	int rt, gt, bt;

	SAT_SHIFT(rt, t->color.red, 2);
	SAT_SHIFT(gt, t->color.green, 2);
	SAT_SHIFT(bt, t->color.blue, 2);

	XColor h1 = {
		.red = rt,
		.green = gt,
		.blue = bt,
	};

	SAT_SHIFT(rt, t->colorTo.red, 4);
	SAT_SHIFT(gt, t->colorTo.green, 4);
	SAT_SHIFT(bt, t->colorTo.blue, 4);

	XColor h2 = {
		.red = rt,
		.green = gt,
		.blue = bt,
	};

	vgradient(t, h1, t->color, width, height, data, 0, height / 2);
	vgradient(t, t->colorTo, h2, width, height, data, height / 2, height);
}

void
raisedBevel(Texture *t, unsigned width, unsigned height, ARGB *data)
{
	ARGB *p = data + (t->borderWidth * width) + t->borderWidth;
	unsigned w = width - (t->borderWidth * 2);
	unsigned h = height - (t->borderWidth * 2) - 2;
	unsigned char rr, gg, bb;

	if (width <= 2 || height <= 2 ||
	    width <= (t->borderWidth * 4) || height <= (t->borderWidth * 4))
		return;

	// top of the bevel
	do {
		rr = p->red + (p->red >> 1);
		gg = p->green + (p->green >> 1);
		bb = p->blue + (p->blue >> 1);

		if (rr < p->red)
			rr = ~0;
		if (gg < p->green)
			gg = ~0;
		if (bb < p->blue)
			bb = ~0;

		p->red = rr;
		p->green = gg;
		p->blue = bb;
		p->alpha = 0xff;

		++p;
	} while (--w);

	p += t->borderWidth + t->borderWidth;
	w = width - (t->borderWidth * 2);

	// left and right of the bevel
	do {
		rr = p->red + (p->red >> 1);
		gg = p->green + (p->green >> 1);
		bb = p->blue + (p->blue >> 1);

		if (rr < p->red)
			rr = ~0;
		if (gg < p->green)
			gg = ~0;
		if (bb < p->blue)
			bb = ~0;

		p->red = rr;
		p->green = gg;
		p->blue = bb;
		p->alpha = 0xff;

		p += w - 1;

		rr = (p->red >> 2) + (p->red >> 1);
		gg = (p->green >> 2) + (p->green >> 1);
		bb = (p->blue >> 2) + (p->blue >> 1);

		if (rr > p->red)
			rr = 0;
		if (gg > p->green)
			gg = 0;
		if (bb > p->blue)
			bb = 0;

		p->red = rr;
		p->green = gg;
		p->blue = bb;
		p->alpha = 0xff;

		p += t->borderWidth + t->borderWidth + 1;
	} while (--h);

	w = width - (t->borderWidth * 2);

	// bottom of the bevel
	do {
		rr = (p->red >> 2) + (p->red >> 1);
		gg = (p->green >> 2) + (p->green >> 1);
		bb = (p->blue >> 2) + (p->blue >> 1);

		if (rr > p->red)
			rr = 0;
		if (gg > p->green)
			gg = 0;
		if (bb > p->blue)
			bb = 0;

		p->red = rr;
		p->green = gg;
		p->blue = bb;
		p->alpha = 0xff;

		++p;
	} while (--w);
}

void
sunkenBevel(Texture *t, unsigned width, unsigned height, ARGB *data)
{
	ARGB *p = data + (t->borderWidth * width) + t->borderWidth;
	unsigned w = width - (t->borderWidth * 2);
	unsigned h = height - (t->borderWidth * 2) - 2;
	unsigned char rr, gg, bb;

	if (width <= 2 || height <= 2 ||
	    width <= (t->borderWidth * 4) || height <= (t->borderWidth * 4))
		return;

	// top of the bevel
	do {
		rr = (p->red >> 2) + (p->red >> 1);
		gg = (p->green >> 2) + (p->green >> 1);
		bb = (p->blue >> 2) + (p->blue >> 1);

		if (rr > p->red)
			rr = 0;
		if (gg > p->green)
			gg = 0;
		if (bb > p->blue)
			bb = 0;

		p->red = rr;
		p->green = gg;
		p->blue = bb;
		p->alpha = 0xff;

		++p;
	} while (--w);

	p += t->borderWidth + t->borderWidth;
	w = width - (t->borderWidth * 2);

	// left and right of the bevel
	do {
		rr = (p->red >> 2) + (p->red >> 1);
		gg = (p->green >> 2) + (p->green >> 1);
		bb = (p->blue >> 2) + (p->blue >> 1);

		if (rr > p->red)
			rr = 0;
		if (gg > p->green)
			gg = 0;
		if (bb > p->blue)
			bb = 0;

		p->red = rr;
		p->green = gg;
		p->blue = bb;
		p->alpha = 0xff;

		p += w - 1;

		rr = p->red + (p->red >> 1);
		gg = p->green + (p->green >> 1);
		bb = p->blue + (p->blue >> 1);

		if (rr < p->red)
			rr = ~0;
		if (gg < p->green)
			gg = ~0;
		if (bb < p->blue)
			bb = ~0;

		p->red = rr;
		p->green = gg;
		p->blue = bb;
		p->alpha = 0xff;

		p += t->borderWidth + t->borderWidth + 1;
	} while (--h);

	w = width - (t->borderWidth * 2);

	// bottom of the bevel
	do {
		rr = p->red + (p->red >> 1);
		gg = p->green + (p->green >> 1);
		bb = p->blue + (p->blue >> 1);

		if (rr < p->red)
			rr = ~0;
		if (gg < p->green)
			gg = ~0;
		if (bb < p->blue)
			bb = ~0;

		p->red = rr;
		p->green = gg;
		p->blue = bb;
		p->alpha = 0xff;

		++p;
	} while (--w);
}

void
render(AScreen *ds, Drawable d, Texture *t, int x, int y, unsigned w, unsigned h)
{
	ARGB *data;

	switch (t->appearance.pattern) {
	case PatternParent:
		return;
	case PatternSolid:
		return;
	case PatternGradient:
		break;
	}

	data = calloc(w * h, sizeof(*data));

	switch (t->appearance.gradient) {
	case GradientDiagonal:
		dgradient(t, w, h, data);
		break;
	case GradientElliptic:
		egradient(t, w, h, data);
		break;
	case GradientHorizontal:
		hgradient(t, w, h, data);
		break;
	case GradientPyramid:
		pgradient(t, w, h, data);
		break;
	case GradientRectangle:
		rgradient(t, w, h, data);
		break;
	case GradientVertical:
		vgradient(t, t->color, t->colorTo, w, h, data, 0, h);
		break;
	case GradientCrossDiagonal:
		cdgradient(t, w, h, data);
		break;
	case GradientPipeCross:
		pcgradient(t, w, h, data);
		break;
	case GradientSplitVertical:
		svgradient(t, w, h, data);
		break;
	case GradientMirrorHorizontal:
		// mhgradient(t, w, h, data); /* FIXME: port this from waimea */
		break;
	}
	switch (t->appearance.relief) {
	case ReliefRaised:
		raisedBevel(t, w, h, data);
		break;
	case ReliefSunken:
		sunkenBevel(t, w, h, data);
		break;
	case ReliefFlat:
		break;
	}

	// rendertodrawable(ds, d, x, y, w, h, data);

	if (t->appearance.border) {
		unsigned i;

		XSetForeground(dpy, ds->dc.gc, t->borderColor.pixel);
		XSetLineAttributes(dpy, ds->dc.gc, 0, LineSolid, CapNotLast, JoinMiter);
		for (i = 0; i < t->borderWidth; i++)
			XDrawRectangle(dpy, d, ds->dc.gc,
				       x + i, y + i, w - (i * 2) - 1, h - (i * 2) - 1);
	}
}

void
rendertodrawable(AScreen *ds, Drawable d, int x, int y, unsigned width, unsigned height,
		 ARGB *data)
{
	Imlib_Image image;

	imlib_context_push(ds->context);

	image = imlib_create_image_using_data(width, height, (DATA32 *) data);
	if (image) {
		imlib_context_set_image(image);
		imlib_context_set_mask(None);
		imlib_context_set_drawable(d);
		imlib_render_image_on_drawable(x, y);
		imlib_free_image_and_decache();
	}
	free(data);

	imlib_context_pop();
}

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
