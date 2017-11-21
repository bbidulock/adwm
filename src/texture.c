/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "texture.h" /* verification */

#if !defined(IMLIB2) || !defined(USE_IMLIB2)
static void solid(const Texture *t, const unsigned width, const unsigned height,
		  ARGB *data, unsigned char alpha);
static void dgradient(const Texture *t, const unsigned width, const unsigned height,
		      ARGB *data, unsigned char alpha);
static void cdgradient(const Texture *t, const unsigned width, const unsigned height,
		       ARGB *data, unsigned char alpha);
static void hgradient(const Texture *t, const unsigned width, const unsigned height,
		      ARGB *data, unsigned char alpha);
static void mhgradient(const Texture *t, const unsigned width, const unsigned height,
		       ARGB *data, unsigned char alpha);
static void vgradient(const Texture *t, const unsigned width, const unsigned height,
		      ARGB *data, unsigned char alpha);
static void svgradient(const Texture *t, const unsigned width, const unsigned height,
		       ARGB *data, unsigned char alpha);
static void pgradient(const Texture *t, const unsigned width, const unsigned height,
		      ARGB *data, unsigned char alpha);
#endif				/* !defined(IMLIB2) || !defined(USE_IMLIB2) */

static void rgradient(const Texture *t, const unsigned width, const unsigned height,
		      ARGB *data, unsigned char alpha);
static void pcgradient(const Texture *t, const unsigned width, const unsigned height,
		       ARGB *data, unsigned char alpha);
static void egradient(const Texture *t, const unsigned width, const unsigned height,
		      ARGB *data, unsigned char alpha);

#if !defined(IMLIB2) || !defined(USE_IMLIB2)
static void interlace(const Texture *t, const unsigned width, const unsigned height,
		      ARGB *data);
static void border(const Texture *t, const unsigned width, const unsigned height,
		   ARGB *data);
static void raised(const Texture *t, const unsigned width, const unsigned height,
		   ARGB *data);
static void sunken(const Texture *t, const unsigned width, const unsigned height,
		   ARGB *data);
#endif				/* !defined(IMLIB2) || !defined(USE_IMLIB2) */

/*
 * Notes: These gradient functions were adapted from blackbox(1) and are present
 * in one form or another also in fluxbox(1), openbox(1) and waimea(1).
 * wmaker(1) and icewm(1) also do this kind of (mostly linear) gradient.
 * Strangely, many of the window managers use imlib2, but none of them use the
 * gradient features of imlib2, and imblib2 is mostly used by these just to load
 * png and xpm files and generate pixmaps from them.
 */

static void
mirrorpoints(ARGB *data, const unsigned h, const unsigned width, const unsigned height)
{
	register unsigned x;
	register ARGB *xp, *p;
	unsigned y;
	ARGB *yp;
	const unsigned w = (width + 1) / 2;
	const unsigned d = (w & 0x1);

	/* horizontally reflect points from 0 to (h-1) */
	for (y = 0, yp = data; y < h; y++, yp += width)
		for (x = w, p = yp + x, xp = yp + x - 1 - d; x < width; x++, xp--, p++)
			*p = *xp;
}

static void
reflectpoints(ARGB *data, const unsigned w, const unsigned width, const unsigned height)
{
	register unsigned y;
	register const ARGB *yp;
	register ARGB *p;
	const unsigned h = (height + 1) / 2;
	const unsigned d = (h & 0x1);
	const size_t stride = width * sizeof(*p);

	/* vertically reflect points from 0 to (w-1) */
	for (y = h, p = data + y * width, yp = data + (h - 1 - d) * width; y < height;
	     y++, yp -= width, p += width)
		memcpy(p, yp, stride);
}

static void
wheelpoints(ARGB *data, const unsigned width, const unsigned height)
{
	/* mirror quadrant I into IV */
	mirrorpoints(data, (height + 1) / 2, width, height);
	/* reflect top half into bottom half */
	reflectpoints(data, width, width, height);
}

#if !defined(IMLIB2) || !defined(USE_IMLIB2)

static void
sumpoint(ARGB *r, const ARGB *a, const ARGB *b)
{
	r->red = a->red + b->red;
	r->green = a->green + b->green;
	r->blue = a->blue + b->blue;
	r->alpha = a->alpha + b->alpha;
}

static void
solid(const Texture *t, const unsigned width, const unsigned height, ARGB *data,
      const unsigned char alpha)
{
	ARGB *p = data;
	int i, j;

	const unsigned char r = t->color.red;
	const unsigned char g = t->color.green;
	const unsigned char b = t->color.blue;

	for (i = 0; i < width; i++) {
		for (j = 0; j < height; j++) {
			p->red = r;
			p->green = g;
			p->blue = b;
			p->alpha = alpha;
			p++;
		}
	}
}

static void
dgradient(const Texture *t, const unsigned width, const unsigned height, ARGB *data,
	  unsigned char alpha)
{
	const ARGB *xt = ecalloc(width, sizeof(*xt));
	const ARGB *yt = ecalloc(height, sizeof(*yt));
	ARGB *p;
	unsigned x, y;

	{
		const double dr = t->colorTo.red - t->color.red;
		const double dg = t->colorTo.green - t->color.green;
		const double db = t->colorTo.blue - t->color.blue;

		/* Create X table */
		{
			double xr = t->color.red;
			double xg = t->color.green;
			double xb = t->color.blue;

			const unsigned w = width * 2;

			const double drx = dr / (double) w;
			const double dgx = dg / (double) w;
			const double dbx = db / (double) w;

			for (x = 0, p = (ARGB *) xt; x < width; x++, p++) {
				p->red = xr;
				p->green = xg;
				p->blue = xb;
				p->alpha = alpha;

				xr += drx;
				xg += dgx;
				xb += dbx;
			}
		}

		/* Create Y table */
		{
			double yr = 0.0;
			double yg = 0.0;
			double yb = 0.0;

			const unsigned h = height * 2;

			const double dry = dr / (double) h;
			const double dgy = dg / (double) h;
			const double dby = db / (double) h;

			for (y = 0, p = (ARGB *) yt; y < height; y++, p++) {
				p->red = yr;
				p->green = yg;
				p->blue = yb;
				p->alpha = 0;

				yr += dry;
				yg += dgy;
				yb += dby;
			}
		}
	}

	/* Combine tables to create gradient */
	for (p = data, y = 0; y < height; y++)
		for (x = 0; x < width; x++, p++)
			sumpoint(p, xt + x, yt + y);
	free((void *) xt);
	free((void *) yt);
}

static void
cdgradient(const Texture *t, const unsigned width, const unsigned height, ARGB *data,
	   unsigned char alpha)
{
	const ARGB *xt = ecalloc(width, sizeof(*xt));
	const ARGB *yt = ecalloc(height, sizeof(*yt));
	ARGB *p;
	unsigned x, y;

	{
		const double dr = t->colorTo.red - t->color.red;
		const double dg = t->colorTo.green - t->color.green;
		const double db = t->colorTo.blue - t->color.blue;

		/* Create X table */
		{
			double xr = t->colorTo.red;
			double xg = t->colorTo.green;
			double xb = t->colorTo.blue;

			const unsigned w = width * 2;

			const double drx = dr / (double) w;
			const double dgx = dg / (double) w;
			const double dbx = db / (double) w;

			for (x = 0, p = (ARGB *) xt; x < width; x++, p++) {
				p->red = xr;
				p->green = xg;
				p->blue = xb;
				p->alpha = alpha;

				xr -= drx;
				xg -= dgx;
				xb -= dbx;
			}
		}

		/* Create Y table */
		{
			double yr = 0.0;
			double yg = 0.0;
			double yb = 0.0;

			const unsigned h = height * 2;

			const double dry = dr / (double) h;
			const double dgy = dg / (double) h;
			const double dby = db / (double) h;

			for (y = 0, p = (ARGB *) yt; y < height; y++, p++) {
				p->red = yr;
				p->green = yg;
				p->blue = yb;
				p->alpha = 0;

				yr += dry;
				yg += dgy;
				yb += dby;
			}
		}
	}

	/* Combine tables to create gradient */
	for (p = data, y = 0; y < height; y++)
		for (x = 0; x < width; x++, p++)
			sumpoint(p, xt + x, yt + y);
	free((void *) xt);
	free((void *) yt);
}

static void
extendpoints(ARGB *data, register const ARGB *xt, const unsigned width,
	     const unsigned height)
{
	register unsigned y;
	register ARGB *p;
	register const size_t stride = width * sizeof(*p);

	/* copy row xt to all rows */
	for (p = data, y = 0; y < height; y++, p += width)
		memcpy(p, xt, stride);
}

static void
hgradient(const Texture *t, const unsigned width, const unsigned height, ARGB *data,
	  unsigned char alpha)
{
	const ARGB *xt = ecalloc(width, sizeof(*xt));
	ARGB *p;
	unsigned x;

	/* a gradient from the left edge to the right edge */
	{
		const double dr = t->colorTo.red - t->color.red;
		const double dg = t->colorTo.green - t->color.green;
		const double db = t->colorTo.blue - t->color.blue;

		/* Create x table */
		{
			const unsigned w = width;

			const double drx = dr / (double) w;
			const double dgx = dg / (double) w;
			const double dbx = db / (double) w;

			double xr = t->color.red;
			double xg = t->color.green;
			double xb = t->color.blue;

			for (x = 0, p = (ARGB *) xt; x < width; x++, p++) {
				p->red = xr;
				p->green = xg;
				p->blue = xb;
				p->alpha = alpha;

				xr += drx;
				xg += dgx;
				xb += dbx;
			}
		}
	}

	/* fill all rows with x table data */
	extendpoints(data, xt, width, height);

	free((void *) xt);
}

static void
mhgradient(const Texture *t, const unsigned width, const unsigned height, ARGB *data,
	   unsigned char alpha)
{
	const ARGB *xt = ecalloc(width, sizeof(*xt));
	ARGB *p;
	unsigned x;

	/* a gradient from the left edge to the right edge */
	{
		const double dr = t->colorTo.red - t->color.red;
		const double dg = t->colorTo.green - t->color.green;
		const double db = t->colorTo.blue - t->color.blue;

		/* Create x table */
		{
			const unsigned w = (width + 1) / 2;

			const double drx = dr / (double) w;
			const double dgx = dg / (double) w;
			const double dbx = db / (double) w;

			double xr = t->color.red;
			double xg = t->color.green;
			double xb = t->color.blue;

			for (x = 0, p = (ARGB *) xt; x < w; x++, p++) {
				p->red = xr;
				p->green = xg;
				p->blue = xb;
				p->alpha = alpha;

				xr += drx;
				xg += dgx;
				xb += dbx;
			}
			if (w & 0x1) {
				xr -= drx;
				xg -= dgx;
				xb -= dbx;
			}
			for (x = w; x < width; x++, p++) {
				p->red = xr;
				p->green = xg;
				p->blue = xb;
				p->alpha = alpha;

				xr -= drx;
				xg -= dgx;
				xb -= dbx;
			}
		}
	}

	/* fill all rows with x table data */
	extendpoints(data, xt, width, height);

	free((void *) xt);
}

static void
smearpoints(ARGB *data, const ARGB *yt, const unsigned width, const unsigned height)
{
	register unsigned x, y;
	register const ARGB *yp;
	register ARGB *p;

	/* copy column yt to all columns */
	for (p = data, y = 0, yp = yt; y < height; y++, yp++)
		for (x = 0; x < width; x++, p++)
			*p = *yp;
}

static void
vgradient(const Texture *t, const unsigned width, const unsigned height, ARGB *data,
	  unsigned char alpha)
{
	const ARGB *yt = ecalloc(height, sizeof(*yt));
	ARGB *p;
	unsigned y;

	/* a gradient from the top to the bottom edge */
	{
		const double dr = t->colorTo.red - t->color.red;
		const double dg = t->colorTo.green - t->color.green;
		const double db = t->colorTo.blue - t->color.blue;

		/* Create y table */
		{
			const unsigned h = height;

			const double dry = dr / (double) h;
			const double dgy = dg / (double) h;
			const double dby = db / (double) h;

			double yr = t->color.red;
			double yg = t->color.green;
			double yb = t->color.blue;

			for (y = 0, p = (ARGB *) yt; y < height; y++, p++) {
				p->red = yr;
				p->green = yg;
				p->blue = yb;
				p->alpha = alpha;

				yr += dry;
				yg += dgy;
				yb += dby;
			}
		}
	}

	/* fill all columns with y table data */
	smearpoints(data, yt, width, height);

	free((void *) yt);
}

static void
svgradient(const Texture *t, const unsigned width, const unsigned height, ARGB *data,
	   unsigned char alpha)
{
	const ARGB *yt = ecalloc(height, sizeof(*yt));
	ARGB *p;
	unsigned y;

	/* a gradient from the middle to the top and bottom edge */
	{
		const double dr = t->color.red - t->colorTo.red;
		const double dg = t->color.green - t->colorTo.green;
		const double db = t->color.blue - t->colorTo.blue;

		/* Create y table */
		{
			const unsigned h = (height + 1) / 2;

			const double dry = dr / (double) h;
			const double dgy = dg / (double) h;
			const double dby = db / (double) h;

			double yr = t->colorTo.red;
			double yg = t->colorTo.green;
			double yb = t->colorTo.blue;

			for (y = 0, p = (ARGB *) yt; y < h; y++, p++) {
				p->red = yr;
				p->green = yg;
				p->blue = yb;
				p->alpha = alpha;

				yr += dry;
				yg += dgy;
				yb += dby;
			}
			if (h & 0x1) {
				yr -= dry;
				yg -= dgy;
				yb -= dby;
			}
			for (y = h; y < height; y++, p++) {
				p->red = yr;
				p->green = yg;
				p->blue = yb;
				p->alpha = alpha;

				yr -= dry;
				yg -= dgy;
				yb -= dby;
			}
		}
	}

	/* fill all columns with y table data */
	smearpoints(data, yt, width, height);

	free((void *) yt);
}

static void
pgradient(const Texture *t, const unsigned width, const unsigned height, ARGB *data,
	  unsigned char alpha)
{
	/* pyramid gradient - based on original dgradient, written by Mosfet
	   (mosfet@kde.org), adapted from kde sources for Blackbox by Brad Hughes,
	   rewritten 4 times more efficient by bidulock@openss7.org for adwm. */

	const unsigned w = (width + 1) / 2;
	const unsigned h = (height + 1) / 2;

	const ARGB *xt = ecalloc(w, sizeof(*xt));
	const ARGB *yt = ecalloc(h, sizeof(*yt));
	ARGB *p = data;
	unsigned x, y;

	const double dr = t->colorTo.red - t->color.red;
	const double dg = t->colorTo.green - t->color.green;
	const double db = t->colorTo.blue - t->color.blue;

	/* Create X table */
	{
		const double drx = dr / (double) width;
		const double dgx = dg / (double) width;
		const double dbx = db / (double) width;

		double xr = dr / 2.0;
		double xg = dg / 2.0;
		double xb = db / 2.0;

		for (x = 0, p = (ARGB *) xt; x < w; x++, p++) {
			p->red = fabs(xr);
			p->green = fabs(xg);
			p->blue = fabs(xb);

			xr -= drx;
			xg -= dgx;
			xb -= dbx;
		}
	}

	/* Create Y table */
	{
		const double dry = dr / (double) height;
		const double dgy = dg / (double) height;
		const double dby = db / (double) height;

		double yr = dr / 2.0;
		double yg = dg / 2.0;
		double yb = db / 2.0;

		for (y = 0, p = (ARGB *) yt; y < h; y++, p++) {
			p->red = fabs(yr);
			p->green = fabs(yg);
			p->blue = fabs(yb);

			yr -= dry;
			yg -= dgy;
			yb -= dby;
		}
	}

	/* Combine tables to create gradient in quadrant I */
	{
		const int rsign = (dr < 0) ? -1 : 1;
		const int gsign = (dg < 0) ? -1 : 1;
		const int bsign = (db < 0) ? -1 : 1;

		const ARGB *yp, *xp, to = {
			.red = t->colorTo.red,
			.green = t->colorTo.green,
			.blue = t->colorTo.blue,
			.alpha = alpha,
		};
		ARGB *rp;

		for (rp = data, y = 0, yp = yt; y < h; y++, yp++, rp += width) {
			for (p = rp, x = 0, xp = xt; x < w; x++, xp++, p++) {
				const unsigned char sr = xp->red + yp->red;
				const unsigned char sg = xp->green + yp->green;
				const unsigned char sb = xp->blue + yp->blue;

				p->red = to.red + ((rsign < 0) ? sr : -sr);
				p->green = to.green + ((gsign < 0) ? sg : -sg);
				p->blue = to.blue + ((bsign < 0) ? sb : -sb);
				p->alpha = to.alpha;
			}
		}
	}

	/* Complete other quadrants */
	wheelpoints(data, width, height);

	free((void *) xt);
	free((void *) yt);
}

#endif				/* !defined(IMLIB2) || !defined(USE_IMLIB2) */

static void
rgradient(const Texture *t, const unsigned width, const unsigned height, ARGB *data,
	  unsigned char alpha)
{
	/* rectangle gradient - based on original dgradient, written by Mosfet
	   (mosfet@kde.org), adapted from kde sources for Blackbox by Brad Hughes,
	   rewritten 4 times more efficient by bidulock@openss7.org for adwm. */

	const unsigned w = (width + 1) / 2;
	const unsigned h = (height + 1) / 2;

	const ARGB *xt = ecalloc(w, sizeof(*xt));
	const ARGB *yt = ecalloc(h, sizeof(*yt));
	ARGB *p;
	unsigned x, y;

	const double dr = t->colorTo.red - t->color.red;
	const double dg = t->colorTo.green - t->color.green;
	const double db = t->colorTo.blue - t->color.blue;

	/* Create X table */
	{
		const double drx = dr / (double) width;
		const double dgx = dg / (double) width;
		const double dbx = db / (double) width;

		double xr = drx / 2;
		double xg = dgx / 2;
		double xb = dbx / 2;

		for (x = 0, p = (ARGB *) xt; x < w; x++, p++) {
			p->red = fabs(xr);
			p->green = fabs(xg);
			p->blue = fabs(xb);

			xr -= drx;
			xg -= dgx;
			xb -= dbx;
		}
	}

	/* Create Y table */
	{
		const double dry = dr / (double) height;
		const double dgy = dg / (double) height;
		const double dby = db / (double) height;

		double yr = dry / 2;
		double yg = dgy / 2;
		double yb = dby / 2;

		for (y = 0, p = (ARGB *) yt; y < h; y++, p++) {
			p->red = fabs(yr);
			p->green = fabs(yg);
			p->blue = fabs(yb);

			yr -= dry;
			yg -= dgy;
			yb -= dby;
		}
	}

	/* Combine tables to create gradient in quadrant I */
	{

		const int rsign = (dr < 0) ? -2 : 2;
		const int gsign = (dg < 0) ? -2 : 2;
		const int bsign = (db < 0) ? -2 : 2;

		const ARGB *yp, *xp, to = {
			.red = t->colorTo.red,
			.green = t->colorTo.green,
			.blue = t->colorTo.blue,
			.alpha = alpha,
		};
		ARGB *rp;

		for (rp = data, y = 0, yp = yt; y < h; y++, yp++, rp += width) {
			for (p = rp, x = 0, xp = xt; x < w; x++, p++, xp++) {
				const unsigned char mr = max(xp->red, yp->red);
				const unsigned char mg = max(xp->green, yp->green);
				const unsigned char mb = max(xp->blue, yp->blue);

				p->red = to.red + 2 * ((rsign < 0) ? mr : -mr);
				p->green = to.green + 2 * ((gsign < 0) ? mg : -mg);
				p->blue = to.blue + 2 * ((bsign < 0) ? mb : -mb);
				p->alpha = to.alpha;
			}
		}
	}

	/* Complete other quadrants */
	wheelpoints(data, width, height);

	free((void *) xt);
	free((void *) yt);
}

static void
pcgradient(const Texture *t, const unsigned width, const unsigned height, ARGB *data,
	   unsigned char alpha)
{
	/* pipe cross gradient - based on original dgradient, written by Mosfet
	   (mosfet@kde.org) adapted from kde sources for Blackbox by Brad Hughes */

	double drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
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

	/* Create X table */
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

	/* Create Y table */
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

	/* Combine tables to create gradient */
	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x, ++p) {
			p->red = tr - (rsign * min(xt[0][x], yt[0][y]));
			p->green = tg - (gsign * min(xt[1][x], yt[1][y]));
			p->blue = tb - (bsign * min(xt[2][x], yt[2][y]));
			p->alpha = 0xff;
		}
	}
	free(alloc);
}

static void
egradient(const Texture *t, const unsigned width, const unsigned height, ARGB *data,
	  unsigned char alpha)
{
	/* elliptic gradient - based on original dgradient, written by Mosfet
	   (mosfet@kde.org), adapted from kde sources for Blackbox by Brad Hughes,
	   rewritten 4 times more efficient by bidulock@openss7.org for adwm. */

	const unsigned w = (width + 1) / 2;
	const unsigned h = (height + 1) / 2;

	const ARGB *xt = ecalloc(w, sizeof(*xt));
	const ARGB *yt = ecalloc(h, sizeof(*yt));
	ARGB *p;
	unsigned x, y;

	const double dr = t->colorTo.red - t->color.red;
	const double dg = t->colorTo.green - t->color.green;
	const double db = t->colorTo.blue - t->color.blue;

	/* Create X table */
	{
		const double drx = dr / (double) width;
		const double dgx = dg / (double) width;
		const double dbx = db / (double) width;

		double xr = dr / 2.0;
		double xg = dg / 2.0;
		double xb = db / 2.0;

		for (x = 0, p = (ARGB *) xt; x < w; x++, p++) {
			p->red = xr * xr;
			p->green = xg * xg;
			p->blue = xb * xb;

			xr -= drx;
			xg -= dgx;
			xb -= dbx;
		}
	}

	/* Create Y table */
	{
		const double dry = dr / (double) height;
		const double dgy = dg / (double) height;
		const double dby = db / (double) height;

		double yr = dr / 2.0;
		double yg = dg / 2.0;
		double yb = db / 2.0;

		for (y = 0, p = (ARGB *) yt; y < h; y++, p++) {
			p->red = yr * yr;
			p->green = yg * yg;
			p->blue = yb * yb;

			yr -= dry;
			yg -= dgy;
			yb -= dby;
		}
	}

	/* Combine tables to create gradient in quadrant I */
	{
		const int rsign = (dr < 0) ? -1 : 1;
		const int gsign = (dg < 0) ? -1 : 1;
		const int bsign = (db < 0) ? -1 : 1;

		const ARGB *xp, *yp, to = {
			.red = t->colorTo.red,
			.green = t->colorTo.green,
			.blue = t->colorTo.blue,
			.alpha = alpha,
		};
		ARGB *rp;

		for (rp = data, y = 0, yp = yt; y < h; y++, yp++, rp += width) {
			for (p = rp, x = 0, xp = xt; x < w; x++, xp++, p++) {
				const unsigned char sr = sqrt(xp->red + yp->red);
				const unsigned char sg = sqrt(xp->green + yp->green);
				const unsigned char sb = sqrt(xp->blue + yp->blue);

				p->red = to.red + ((rsign < 0) ? sr : -sr);
				p->green = to.green + ((gsign < 0) ? sg : -sg);
				p->blue = to.blue + ((bsign < 0) ? sb : -sb);
				p->alpha = to.alpha;
			}
		}
	}

	/* Complete other quadrants */
	wheelpoints(data, width, height);

	free((void *) xt);
	free((void *) yt);
}

#if 0
static void
raisedBevel(const Texture *t, const unsigned width, const unsigned height, ARGB *data)
{
	ARGB *p = data + (t->borderWidth * width) + t->borderWidth;
	unsigned w = width - (t->borderWidth * 2);
	unsigned h = height - (t->borderWidth * 2) - 2;

	if (width <= 2 || height <= 2 ||
	    width <= (t->borderWidth * 4) || height <= (t->borderWidth * 4))
		return;

	/* top of the bevel */
	do {
		raisepoint(p);
		++p;
	} while (--w);

	p += t->borderWidth + t->borderWidth;
	w = width - (t->borderWidth * 2);

	/* left and right of the bevel */
	do {
		raisepoint(p);
		p += w - 1;
		lowerpoint(p);
		p += t->borderWidth + t->borderWidth + 1;
	} while (--h);

	w = width - (t->borderWidth * 2);

	/* bottom of the bevel */
	do {
		lowerpoint(p);
		++p;
	} while (--w);
}

static void
sunkenBevel(const Texture *t, const unsigned width, const unsigned height, ARGB *data)
{
	ARGB *p = data + (t->borderWidth * width) + t->borderWidth;
	unsigned w = width - (t->borderWidth * 2);
	unsigned h = height - (t->borderWidth * 2) - 2;

	if (width <= 2 || height <= 2 ||
	    width <= (t->borderWidth * 4) || height <= (t->borderWidth * 4))
		return;

	/* top of the bevel */
	do {
		lowerpoint(p);
		++p;
	} while (--w);

	p += t->borderWidth + t->borderWidth;
	w = width - (t->borderWidth * 2);

	/* left and right of the bevel */
	do {
		lowerpoint(p);
		p += w - 1;
		raisepoint(p);
		p += t->borderWidth + t->borderWidth + 1;
	} while (--h);

	w = width - (t->borderWidth * 2);

	/* bottom of the bevel */
	do {
		raisepoint(p);
		++p;
	} while (--w);
}

static void
render(AScreen *ds, Drawable d, Texture *t, int x, int y, unsigned w, unsigned h,
       unsigned char alpha)
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
		dgradient(t, w, h, data, alpha);
		break;
	case GradientElliptic:
		egradient(t, w, h, data, alpha);
		break;
	case GradientHorizontal:
		hgradient(t, w, h, data, alpha);
		break;
	case GradientPyramid:
		pgradient(t, w, h, data, alpha);
		break;
	case GradientRectangle:
		rgradient(t, w, h, data, alpha);
		break;
	case GradientVertical:
		vgradient(t, w, h, data, alpha);
		break;
	case GradientCrossDiagonal:
		cdgradient(t, w, h, data, alpha);
		break;
	case GradientPipeCross:
		pcgradient(t, w, h, data, alpha);
		break;
	case GradientSplitVertical:
		svgradient(t, w, h, data, alpha);
		break;
	case GradientMirrorHorizontal:
		mhgradient(t, w, h, data, alpha);
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

	/* rendertodrawable(ds, d, x, y, w, h, data); */

	if (t->appearance.border) {
		unsigned i;

		XSetForeground(dpy, ds->dc.gc, t->borderColor.pixel);
		XSetLineAttributes(dpy, ds->dc.gc, 0, LineSolid, CapNotLast, JoinMiter);
		for (i = 0; i < t->borderWidth; i++)
			XDrawRectangle(dpy, d, ds->dc.gc,
				       x + i, y + i, w - (i * 2) - 1, h - (i * 2) - 1);
	}
}
#endif

#if !defined(IMLIB2) || !defined(USE_IMLIB2)

static void
setpoint(ARGB *p, unsigned char r, unsigned char g, unsigned char b)
{
	p->red = r;
	p->green = g;
	p->blue = b;
}

static void
raisepoint(ARGB *p)
{
	unsigned char r, g, b;

	r = p->red + (p->red >> 1);
	if (r < p->red)
		r = ~0;
	g = p->green + (p->green >> 1);
	if (g < p->green)
		g = ~0;
	b = p->blue + (p->blue >> 1);
	if (b < p->blue)
		b = ~0;
	p->red = r;
	p->green = g;
	p->blue = b;
}

static void
lowerpoint(ARGB *p)
{
	unsigned char r, g, b;

	r = (p->red >> 2) + (p->red >> 1);
	if (r > p->red)
		r = 0;
	g = (p->green >> 2) + (p->green >> 1);
	if (g > p->green)
		g = 0;
	b = (p->blue >> 2) + (p->blue >> 1);
	if (b > p->blue)
		b = 0;
	p->red = r;
	p->green = g;
	p->blue = b;
}

static void
interlace(const Texture *t, const unsigned width, const unsigned height, ARGB *data)
{
	register unsigned i, j;
	register ARGB *p;

	for (p = data + width, i = 1; i < height; i += 2, p += width)
		for (j = 0; j < width; j++, p++)
			lowerpoint(p);
}

static void
border(const Texture *t, const unsigned width, const unsigned height, ARGB *data)
{
	int i, j, x, y, w, h, l;

	const unsigned char r = t->borderColor.red;
	const unsigned char g = t->borderColor.green;
	const unsigned char b = t->borderColor.blue;

	switch (t->appearance.bevel) {
	case Bevel1:
	default:
		x = 0;
		y = 0;
		w = width;
		h = height;
		break;
	case Bevel2:
		x = 1;
		y = 1;
		w = width - 2;
		h = height - 2;
		break;
	}
	l = t->borderWidth;
	if (l > w / 4)
		l = w / 4;
	if (l > h / 4)
		l = h / 4;
	for (i = 0; i < l && w > 0 && h > 0; i++, x++, y++, w -= 2, h -= 2) {
		/* top */
		for (j = x + y * width; j < x + y * width + w; j++)
			setpoint(data + j, r, g, b);
		/* sides */
		for (j = x + (y + 1) * width; j < x + (y + h - 1) * width; j += width) {
			setpoint(data + j, r, g, b);
			setpoint(data + j + w - 1, r, g, b);
		}
		/* bottom */
		for (j = x + (y + h) * width; j < x + (y + h) * width + w; j++)
			setpoint(data + j, r, g, b);
	}
}

static void
raised(const Texture *t, const unsigned width, const unsigned height, ARGB *data)
{
	int i, j, x, y, w, h, l;

	switch (t->appearance.bevel) {
	case Bevel1:
	default:
		x = 0;
		y = 0;
		w = width;
		h = height;
		break;
	case Bevel2:
		x = 1;
		y = 1;
		w = width - 2;
		h = height - 2;
		break;
	}
	l = t->borderWidth;
	if (l > w / 4)
		l = w / 4;
	if (l > h / 4)
		l = h / 4;
	for (i = 0; i < l && w > 0 && h > 0; i++, x++, y++, w -= 2, h -= 2) {
		/* top */
		for (j = x + y * width; j < x + y * width + w; j++) {
			raisepoint(data + j);
		}
		/* sides */
		for (j = x + (y + 1) * width; j < x + (y + h - 1) * width; j += width) {
			raisepoint(data + j);
			lowerpoint(data + j + w - 1);
		}
		/* bottom */
		for (j = x + (y + h) * width; j < x + (y + h) * width + w; j++) {
			lowerpoint(data + j);
		}
	}
}

static void
sunken(const Texture *t, const unsigned width, const unsigned height, ARGB *data)
{
	int i, j, x, y, w, h, l;

	switch (t->appearance.bevel) {
	case Bevel1:
	default:
		x = 0;
		y = 0;
		w = width;
		h = height;
		break;
	case Bevel2:
		x = 1;
		y = 1;
		w = width - 2;
		h = height - 2;
		break;
	}
	l = t->borderWidth;
	if (l > w / 4)
		l = w / 4;
	if (l > h / 4)
		l = h / 4;
	for (i = 0; i < l && w > 0 && h > 0; i++, x++, y++, w -= 2, h -= 2) {
		/* top */
		for (j = x + y * width; j < x + y * width + w; j++) {
			lowerpoint(data + j);
		}
		/* sides */
		for (j = x + (y + 1) * width; j < x + (y + h - 1) * width; j += width) {
			lowerpoint(data + j);
			raisepoint(data + j + w - 1);
		}
		/* bottom */
		for (j = x + (y + h) * width; j < x + (y + h) * width + w; j++) {
			raisepoint(data + j);
		}
	}
}
#endif				/* !defined(IMLIB2) || !defined(USE_IMLIB2) */

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

#ifdef IMLIB2

#ifdef USE_IMLIB2

static void
drawsolid(const Texture *t, const unsigned width, const unsigned height,
	  const unsigned char alpha)
{
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_context_set_blend(0);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, alpha);
	imlib_image_fill_rectangle(0, 0, width, height);
}

static void
drawdiagonal(const Texture *t, const unsigned width, const unsigned height,
	     const unsigned char alpha)
{
	Imlib_Color_Range rangeh, rangev;

	/* a gradient from the top left corner to the bottom right corner */
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_context_set_blend(1);

	rangev = imlib_create_color_range();
	imlib_context_set_color_range(rangev);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, alpha);
	imlib_add_color_to_color_range(0);
	imlib_context_set_color(t->colorTo.red, t->colorTo.green, t->colorTo.blue, alpha);
	imlib_add_color_to_color_range(height);
	imlib_image_fill_color_range_rectangle(0, 0, width, height, 0.0);
	imlib_free_color_range();

	rangeh = imlib_create_color_range();
	imlib_context_set_color_range(rangeh);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, 127);
	imlib_add_color_to_color_range(0);
	imlib_context_set_color(t->colorTo.red, t->colorTo.green, t->colorTo.blue, 127);
	imlib_add_color_to_color_range(width);
	imlib_image_fill_color_range_rectangle(0, 0, width, height, -90.0);
	imlib_free_color_range();
}

static void
drawcrossdiagonal(const Texture *t, const unsigned width, const unsigned height,
		  const unsigned char alpha)
{
	Imlib_Color_Range rangeh, rangev;

	/* a gradient from the top right corner to the bottom left corner */
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_context_set_blend(1);

	rangev = imlib_create_color_range();
	imlib_context_set_color_range(rangev);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, alpha);
	imlib_add_color_to_color_range(0);
	imlib_context_set_color(t->colorTo.red, t->colorTo.green, t->colorTo.blue, alpha);
	imlib_add_color_to_color_range(height);
	imlib_image_fill_color_range_rectangle(0, 0, width, height, 0.0);
	imlib_free_color_range();

	rangeh = imlib_create_color_range();
	imlib_context_set_color_range(rangeh);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, 127);
	imlib_add_color_to_color_range(0);
	imlib_context_set_color(t->colorTo.red, t->colorTo.green, t->colorTo.blue, 127);
	imlib_add_color_to_color_range(width);
	imlib_image_fill_color_range_rectangle(0, 0, width, height, 90.0);
	imlib_free_color_range();
}

static void
drawpyramid(const Texture *t, const unsigned width, const unsigned height, const
	    unsigned char alpha)
{
	Imlib_Color_Range rangeh, rangev;

	/* a gradient that starts in all four corners and smooths to the center of the
	   texture */
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_context_set_blend(1);

	rangev = imlib_create_color_range();
	imlib_context_set_color_range(rangev);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, alpha);
	imlib_add_color_to_color_range(0);
	imlib_context_set_color(t->colorTo.red, t->colorTo.green, t->colorTo.blue, alpha);
	imlib_add_color_to_color_range((height + 1) / 2);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, alpha);
	imlib_add_color_to_color_range(height);
	imlib_image_fill_color_range_rectangle(0, 0, width, height, 0.0);
	imlib_free_color_range();

	rangeh = imlib_create_color_range();
	imlib_context_set_color_range(rangeh);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, 127);
	imlib_add_color_to_color_range(0);
	imlib_context_set_color(t->colorTo.red, t->colorTo.green, t->colorTo.blue, 127);
	imlib_add_color_to_color_range(width / 2);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, 127);
	imlib_add_color_to_color_range(width);
	imlib_image_fill_color_range_rectangle(0, 0, width, height, 90.0);
	imlib_free_color_range();
}

static void
drawrectangle(const Texture *t, const unsigned width, const unsigned height,
	      const unsigned char alpha)
{
	Imlib_Image image, buffer;
	ARGB *data;

	data = ecalloc(width * height, sizeof(*data));
	rgradient(t, width, height, data, alpha);
	buffer = imlib_create_image_using_data(width, height, (DATA32 *) data);
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_blend_image_onto_image(buffer, False, 0, 0, width, height, 0, 0, width,
				     height);
	image = imlib_context_get_image();
	imlib_context_set_image(buffer);
	imlib_free_image_and_decache();
	imlib_context_set_image(image);
	free(data);
}

static void
drawpipecross(const Texture *t, const unsigned width, const unsigned height,
	      const unsigned char alpha)
{
	Imlib_Image image, buffer;
	ARGB *data;

	data = ecalloc(width * height, sizeof(*data));
	pcgradient(t, width, height, data, alpha);
	buffer = imlib_create_image_using_data(width, height, (DATA32 *) data);
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_blend_image_onto_image(buffer, False, 0, 0, width, height, 0, 0, width,
				     height);
	image = imlib_context_get_image();
	imlib_context_set_image(buffer);
	imlib_free_image_and_decache();
	imlib_context_set_image(image);
	free(data);
}

static void
drawelliptic(const Texture *t, const unsigned width, const unsigned height,
	     const unsigned char alpha)
{
	Imlib_Image image, buffer;
	ARGB *data;

	data = ecalloc(width * height, sizeof(*data));
	egradient(t, width, height, data, alpha);
	buffer = imlib_create_image_using_data(width, height, (DATA32 *) data);
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_blend_image_onto_image(buffer, False, 0, 0, width, height, 0, 0, width,
				     height);
	image = imlib_context_get_image();
	imlib_context_set_image(buffer);
	imlib_free_image_and_decache();
	imlib_context_set_image(image);
	free(data);
}

static void
drawmirrorhorizontal(const Texture *t, const unsigned width, const unsigned height,
		     const unsigned char alpha)
{
	Imlib_Color_Range range;

	/* a gradient from the left edge to the middle and then reversed to the right
	   edge */
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_context_set_blend(0);

	range = imlib_create_color_range();
	imlib_context_set_color_range(range);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, alpha);
	imlib_add_color_to_color_range(0);
	imlib_context_set_color(t->colorTo.red, t->colorTo.green, t->colorTo.blue, alpha);
	imlib_add_color_to_color_range((width + 1) / 2);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, alpha);
	imlib_add_color_to_color_range(width - 1);
	imlib_image_fill_color_range_rectangle(0, 0, width, height, 90.0);
	imlib_free_color_range();
}

static void
drawhorizontal(const Texture *t, const unsigned width, const unsigned height,
	       const unsigned char alpha)
{
	Imlib_Color_Range range;

	/* a gradient from the left edge to the right */
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_context_set_blend(0);

	range = imlib_create_color_range();
	imlib_context_set_color_range(range);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, alpha);
	imlib_add_color_to_color_range(0);
	imlib_context_set_color(t->colorTo.red, t->colorTo.green, t->colorTo.blue, alpha);
	imlib_add_color_to_color_range(width);
	imlib_image_fill_color_range_rectangle(0, 0, width, height, 90.0);
	imlib_free_color_range();
}

static void
drawsplitvertical(const Texture *t, const unsigned width, const unsigned height,
		  const unsigned char alpha)
{
	Imlib_Color_Range range;

	/* a gradient split in the midde that goes out toward the top and bottom edges */
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_context_set_blend(0);

	range = imlib_create_color_range();
	imlib_context_set_color_range(range);
	imlib_context_set_color(t->colorTo.red, t->colorTo.green, t->colorTo.blue, alpha);
	imlib_add_color_to_color_range(0);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, alpha);
	imlib_add_color_to_color_range((height + 1) / 2);
	imlib_context_set_color(t->colorTo.red, t->colorTo.green, t->colorTo.blue, alpha);
	imlib_add_color_to_color_range(height - 1);
	imlib_image_fill_color_range_rectangle(0, 0, width, height, 0.0);
	imlib_free_color_range();
}

static void
drawvertical(const Texture *t, const unsigned width, const unsigned height,
	     const unsigned char alpha)
{
	Imlib_Color_Range range;

	/* a gradient from the top edge to the bottom */
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_context_set_blend(0);

	range = imlib_create_color_range();
	imlib_context_set_color_range(range);
	imlib_context_set_color(t->color.red, t->color.green, t->color.blue, alpha);
	imlib_add_color_to_color_range(0);
	imlib_context_set_color(t->colorTo.red, t->colorTo.green, t->colorTo.blue, alpha);
	imlib_add_color_to_color_range(height - 1);
	imlib_image_fill_color_range_rectangle(0, 0, width, height, 0.0);
	imlib_free_color_range();
}

static void
drawsunken(const Texture *t, unsigned width, unsigned height)
{
	Imlib_Image image, rbuffer, lbuffer;
	DATA32 *data;
	ARGB *rdata, *ldata;

	data = imlib_image_get_data_for_reading_only();
	rdata = ecalloc(width * height, sizeof(*rdata));
	ldata = ecalloc(width * height, sizeof(*ldata));
	memcpy(rdata, data, width * height * sizeof(*rdata));
	memcpy(ldata, data, width * height * sizeof(*ldata));

	rbuffer = imlib_create_image_using_data(width, height, (DATA32 *) rdata);
	imlib_context_set_operation(IMLIB_OP_ADD);
	imlib_context_set_blend(1);
	imlib_blend_image_onto_image(rbuffer, False, 0, 0, width, height, 0, 0, width,
				     height);
	lbuffer = imlib_create_image_using_data(width, height, (DATA32 *) ldata);
	imlib_context_set_operation(IMLIB_OP_SUBTRACT);
	imlib_context_set_blend(1);
	imlib_blend_image_onto_image(lbuffer, False, 0, 0, width, height, 0, 0, width,
				     height);
	image = imlib_context_get_image();
	imlib_context_set_image(rbuffer);
	imlib_free_image();
	free(rdata);
	imlib_context_set_image(lbuffer);
	imlib_free_image();
	free(ldata);
	imlib_context_set_image(image);
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_context_set_blend(1);
}

static void
drawraised(const Texture *t, unsigned width, unsigned height)
{
	Imlib_Image image, rbuffer, lbuffer;
	DATA32 *data;
	ARGB *rdata, *ldata;

	data = imlib_image_get_data_for_reading_only();
	rdata = ecalloc(width * height, sizeof(*rdata));
	ldata = ecalloc(width * height, sizeof(*ldata));
	memcpy(rdata, data, width * height * sizeof(*rdata));
	memcpy(ldata, data, width * height * sizeof(*ldata));

	rbuffer = imlib_create_image_using_data(width, height, (DATA32 *) rdata);
	imlib_context_set_operation(IMLIB_OP_ADD);
	imlib_context_set_blend(1);
	imlib_blend_image_onto_image(rbuffer, False, 0, 0, width, height, 0, 0, width,
				     height);
	lbuffer = imlib_create_image_using_data(width, height, (DATA32 *) ldata);
	imlib_context_set_operation(IMLIB_OP_SUBTRACT);
	imlib_context_set_blend(1);
	imlib_blend_image_onto_image(lbuffer, False, 0, 0, width, height, 0, 0, width,
				     height);
	image = imlib_context_get_image();
	imlib_context_set_image(rbuffer);
	imlib_free_image();
	free(rdata);
	imlib_context_set_image(lbuffer);
	imlib_free_image();
	free(ldata);
	imlib_context_set_image(image);
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_context_set_blend(1);
}

static void
drawborder(const Texture *t, unsigned width, unsigned height)
{
	int i;
	int x, y, w, h, b;

	b = t->borderWidth;

	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_context_set_blend(0);
	imlib_context_set_color(t->borderColor.red, t->borderColor.green,
				t->borderColor.blue, 255);
	switch (t->appearance.bevel) {
	case Bevel1:
	default:
		x = 0;
		y = 0;
		w = width;
		h = height;
		break;
	case Bevel2:
		x = 1;
		y = 1;
		w = width - 2;
		h = height - 2;
		break;
	}
	for (i = 0; i < b && w > 0 && h > 0; i++, x++, y++, w -= 2, h -= 2)
		imlib_image_draw_rectangle(x, y, w, h);
}

#endif				/* USE_IMLIB2 */

#ifdef USE_IMLIB2

static void
drawinterlace(const Texture *t, const unsigned width, const unsigned height)
{
	Imlib_Image image, buffer;
	DATA32 *data32;
	unsigned x, y;
	ARGB *p, *data;

	data32 = imlib_image_get_data_for_reading_only();
	data = ecalloc(width * height, sizeof(*data));
	memcpy(data, data32, width * height * sizeof(*data));

	for (p = data, y = 0; y < height; y++)
		if (y & 0x1)
			for (x = 0; x < width; x++, p++)
				p->alpha = 64;
		else
			for (x = 0; x < width; x++, p++)
				p->alpha = 0;

	buffer = imlib_create_image_using_data(width, height, (DATA32 *) data);
	imlib_context_set_operation(IMLIB_OP_SUBTRACT);
	imlib_context_set_blend(1);
	imlib_blend_image_onto_image(buffer, False, 0, 0, width, height, 0, 0, width,
				     height);
	image = imlib_context_get_image();
	imlib_context_set_image(buffer);
	imlib_free_image();
	imlib_context_set_image(image);
	free(data);
}

static void
drawpattern(const Texture *t, unsigned width, unsigned height, const unsigned char alpha)
{
	switch (t->appearance.pattern) {
	case PatternSolid:
	default:
		drawsolid(t, width, height, alpha);
		break;
	case PatternParent:
		break;
	case PatternGradient:
		switch (t->appearance.gradient) {
		case GradientDiagonal:
		default:
			drawdiagonal(t, width, height, alpha);
			break;
		case GradientCrossDiagonal:
			drawcrossdiagonal(t, width, height, alpha);
			break;
		case GradientRectangle:
			drawrectangle(t, width, height, alpha);
			break;
		case GradientPyramid:
			drawpyramid(t, width, height, alpha);
			break;
		case GradientPipeCross:
			drawpipecross(t, width, height, alpha);
			break;
		case GradientElliptic:
			drawelliptic(t, width, height, alpha);
			break;
		case GradientMirrorHorizontal:
			drawmirrorhorizontal(t, width, height, alpha);
			break;
		case GradientHorizontal:
			drawhorizontal(t, width, height, alpha);
			break;
		case GradientSplitVertical:
			drawsplitvertical(t, width, height, alpha);
			break;
		case GradientVertical:
			drawvertical(t, width, height, alpha);
			break;
		}
		break;
	}

	if (t->appearance.interlaced)
		drawinterlace(t, width, height);

	switch (t->appearance.relief) {
	case ReliefFlat:
		if (t->appearance.border)
			drawborder(t, width, height);
		break;
	case ReliefSunken:
		drawsunken(t, width, height);
		break;
	case ReliefRaised:
	default:
		drawraised(t, width, height);
		break;
	}
}

#else				/* USE_IMLIB2 */

static void
drawpattern(const Texture *t, unsigned width, unsigned height, const unsigned char alpha)
{
	Imlib_Image image, buffer;
	ARGB *data;

	data = ecalloc(width * height, sizeof(*data));

	switch (t->appearance.pattern) {
	case PatternSolid:
	default:
		solid(t, width, height, data, alpha);
		break;
	case PatternParent:
		break;
	case PatternGradient:
		switch (t->appearance.gradient) {
		case GradientDiagonal:
		default:
			dgradient(t, width, height, data, alpha);
			break;
		case GradientCrossDiagonal:
			cdgradient(t, width, height, data, alpha);
			break;
		case GradientRectangle:
			rgradient(t, width, height, data, alpha);
			break;
		case GradientPyramid:
			pgradient(t, width, height, data, alpha);
			break;
		case GradientPipeCross:
			pcgradient(t, width, height, data, alpha);
			break;
		case GradientElliptic:
			egradient(t, width, height, data, alpha);
			break;
		case GradientMirrorHorizontal:
			mhgradient(t, width, height, data, alpha);
			break;
		case GradientHorizontal:
			hgradient(t, width, height, data, alpha);
			break;
		case GradientSplitVertical:
			svgradient(t, width, height, data, alpha);
			break;
		case GradientVertical:
			vgradient(t, width, height, data, alpha);
			break;
		}

		break;
	}

	if (t->appearance.interlaced)
		interlace(t, width, height, data);

	switch (t->appearance.relief) {
	case ReliefFlat:
		if (t->appearance.border)
			border(t, width, height, data);
		break;
	case ReliefSunken:
		sunken(t, width, height, data);
		break;
	case ReliefRaised:
	default:
		raised(t, width, height, data);
		break;
	}

	buffer = imlib_create_image_using_data(width, height, (DATA32 *) data);
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_context_set_blend(1);
	imlib_blend_image_onto_image(buffer, False, 0, 0, width, height, 0, 0, width,
				     height);
	image = imlib_context_get_image();
	imlib_context_set_image(buffer);
	imlib_free_image();
	imlib_context_set_image(image);
	free(data);
}

#endif				/* USE_IMLIB2 */

void
drawtexture(const AScreen *ds, const Texture *t, const Drawable d, const int x,
	    const int y, const unsigned width, const unsigned height,
	    const unsigned char alpha)
{
	Imlib_Image image;

	imlib_context_push(ds->context);

	image = imlib_create_image(width, height);

	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_context_set_anti_alias(1);
	imlib_context_set_dither(1);
	imlib_context_set_blend(1);
	imlib_context_set_image(image);
	imlib_context_set_mask(None);

	drawpattern(t, width, height, alpha);

	imlib_context_set_drawable(d);
	imlib_context_set_image(image);
	imlib_context_set_mask(None);
	imlib_render_image_on_drawable(x, y);

	imlib_free_image();

	imlib_context_pop();
}

#else				/* IMLIB2 */

static void
drawpattern(const Texture *t, const unsigned width, const unsigned height, ARGB *data,
	    const unsigned char alpha)
{
	switch (t->appearance.pattern) {
	case PatternSolid:
	default:
		solid(t, width, height, data, alpha);
		break;
	case PatternParent:
		break;
	case PatternGradient:
		switch (t->appearance.gradient) {
		case GradientDiagonal:
		default:
			dgradient(t, width, height, data, alpha);
			break;
		case GradientCrossDiagonal:
			cdgradient(t, width, height, data, alpha);
			break;
		case GradientRectangle:
			rgradient(t, width, height, data, alpha);
			break;
		case GradientPyramid:
			pgradient(t, width, height, data, alpha);
			break;
		case GradientPipeCross:
			pcgradient(t, width, height, data, alpha);
			break;
		case GradientElliptic:
			egradient(t, width, height, data, alpha);
			break;
		case GradientMirrorHorizontal:
			mhgradient(t, width, height, data, alpha);
			break;
		case GradientHorizontal:
			hgradient(t, width, height, data, alpha);
			break;
		case GradientSplitVertical:
			svgradient(t, width, height, data, alpha);
			break;
		case GradientVertical:
			vgradient(t, width, height, data, alpha);
			break;
		}
		break;
	}

	if (t->appearance.interlaced)
		interlace(t, width, height, data);

	switch (t->appearance.relief) {
	case ReliefFlat:
		if (t->appearance.border)
			border(t, width, height, data);
		break;
	case ReliefSunken:
		sunken(t, width, height, data);
		break;
	case ReliefRaised:
	default:
		raised(t, width, height, data);
		break;
	}
}

void
rendertexture(const AScreen *ds, const Texture *t, const Drawable d, const int x,
	      const int y, const unsigned width, const unsigned height)
{
}

void
drawtexture(const AScreen *ds, const Texture *t, const Drawable d, const int x,
	    const int y, const unsigned width, const unsigned height,
	    const unsigned char alpha)
{
	ARGB *data;

	data = ecalloc(width * height, sizeof(*data));
	drawpattern(t, width, height, data, alpha);
	rendertexture(ds, t, d, x, y, width, height);
	free(data);
}

#endif				/* IMLIB2 */

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
