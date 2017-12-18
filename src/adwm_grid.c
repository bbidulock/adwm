/* See COPYING file for copyright and license details. */

#include "adwm.h"

/*
 * This is a grid layout ala velox, both in a tiled and floating version.
 */

static void
arrange_GRID(Monitor *m)
{
	Client *c;
	Workarea wa;
	View *v;
	int rows, cols, n, i, *rc, *rh, col, row;
	int cw, cl, *rl;
	int gap;

	if (!(c = nexttiled(scr->clients, m)))
		return;

	v = &scr->views[m->curtag];

	getworkarea(m, &wa);

	for (n = 0, c = nexttiled(scr->clients, m); c; c = nexttiled(c->next, m), n++) ;

	for (cols = 1; cols < n && cols < v->ncolumns; cols++) ;

	for (rows = 1; (rows * cols) < n; rows++) ;

	/* number of rows per column */
	rc = ecalloc(cols, sizeof(*rc));
	for (col = 0, i = 0; i < n; rc[col]++, i++, col = i % cols) ;

	/* average height per client in column */
	rh = ecalloc(cols, sizeof(*rh));
	for (col = 0; col < cols; rh[col] = wa.h / rc[col], col++) ;

	/* height of last client in column */
	rl = ecalloc(cols, sizeof(*rl));
	for (col = 0; col < cols; rl[col] = wa.h - (rc[col] - 1) * rh[col], col++) ;

	/* average width of column */
	cw = wa.w / cols;

	/* width of last column */
	cl = wa.w - (cols - 1) * cw;

	gap =
	    scr->style.margin >
	    scr->style.border ? scr->style.margin - scr->style.border : 0;

	for (i = 0, col = 0, row = 0, c = nexttiled(scr->clients, m); c && i < n;
	     c = nexttiled(c->next, m), i++, col = i % cols, row = i / cols) {
		ClientGeometry n;

		n.x = gap + wa.x + (col * cw);
		n.y = gap + wa.y + (row * rh[col]);
		n.w = (col == cols - 1) ? cl : cw;
		n.h = (row == rows - 1) ? rl[col] : rh[col];
		n.b = scr->style.border;
		if (col > 0) {
			n.w += n.b;
			n.x -= n.b;
		}
		if (row > 0) {
			n.h += n.b;
			n.y -= n.b;
		}
		n.w -= 2 * (gap + n.b);
		n.h -= 2 * (gap + n.b);
		n.t = (v->dectiled && c->has.title) ? scr->style.titleheight : 0;
		n.g = (v->dectiled && c->has.grips) ? scr->style.gripsheight : 0;
		if (!c->is.moveresize) {
			XPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &n);
		} else {
			ClientGeometry C = n;

			/* center it where it was before */
			C.x = (c->c.x + c->c.w / 2) - C.w / 2;
			C.y = (c->c.y + c->c.h / 2) - C.h / 2;
			XPRINTF("CALLING reconfigure()\n");
			reconfigure(c, &C);
		}
	}
	free(rc);
	free(rh);
	free(rl);
}

Layout adwm_layouts[] = {
	/* *INDENT-OFF* */
	/* function		symbol	features				major		minor		placement	 */
	{ &arrange_GRID,	'g',	NCOLUMNS | ROTL | MMOVE,		OrientLeft,	OrientTop,	ColSmartPlacement },
	{ NULL,			'\0',	0,					0,		0,		0		  }
	/* *INDENT-ON* */
};

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS foldmarker=@{,@} foldmethod=marker:
