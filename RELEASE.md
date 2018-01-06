[adwm -- release notes.  2018-01-06]: #

Stable Release 0.7.2
====================

This is a stable release that includes more todo's completed.  Yes, there are
still some outstanding, but nothing major.  This release fixes one obnoxious
behaviour: auto-exposing docks and panels when you overshoot browser scrollbars:
there is now a default 500 millisecond delay before they are exposed.  You need
to rub the edge of the monitor with the pointer to get them to expose now.

There are a lot of preparations in the code now for handling more graphics file
formats and advanced rendering, but it is not yet complete.  The stable ximage
approach works well (if not the most efficient).  I wanted to released the other
fixes and improvements before working further on the graphics formats and
rendering.

As usual, Included is an autoconf tarball for building the package from source.
Please report problems to the issues list on github.


[ vim: set ft=markdown sw=4 tw=80 nocin nosi fo+=tcqlorn spell: ]: #
