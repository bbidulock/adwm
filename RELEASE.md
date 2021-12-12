[adwm -- release notes.  2021-12-11]: #

Maintenance Release 0.7.15
==========================

This is another release of the adwm window manager that provides a fully
XDG, WindowMaker, WMH (WinWM), MWMH (CDE/Motif), ICCM 2.0 compliant
tiling/floating window manager.  It borrows concepts from `dwm(1)`,
`velox(1)`, `awesome(1)`, `spectrwm(1)` and `wmii(1)`.

This is a maintenance release that updates gh-pages support and removes
a harmful type punning that was leading to indeterminate behaviour.
Session management has not progressed since the last release.  There are
no outstanding issues at the time of release, and the next release
should include some outstanding development from the [TODO](TODO) list.

Included in the release is an autoconf tarball for building the package
from source.  See the [NEWS](NEWS) and [TODO](TODO) file in the release
for more information.  Please report problems to the issues list on
[GitHub](https://github.com/bbidulock/adwm/issues).

[ vim: set ft=markdown sw=4 tw=72 nocin nosi fo+=tcqlorn spell: ]: #
