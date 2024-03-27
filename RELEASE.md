[adwm -- release notes.  2024-03-26]: #

Maintenance Release 0.7.17
===========================

This is another release of the adwm window manager that provides a fully
XDG, WindowMaker, WMH (WinWM), MWMH (CDE/Motif), ICCM 2.0 compliant
tiling/floating window manager.  It borrows concepts from `dwm(1)`,
`velox(1)`, `awesome(1)`, `spectrwm(1)` and `wmii(1)`.

This is a maintenance release that updates build flags, fights unused
direct dependencies, adds some XF86 keysyms and minor fixes for GCC.  It
also keeps adwm from setting zero width or height even when instructed
by the client.  Session management has not progressed since the last
release.  There are no outstanding issues at the time of release, and
the next release should include some outstanding development from the
[TODO][5] list.

Included in the release is an autoconf tarball for building the package
from source.  See the [NEWS][3] and [TODO][5] file in the release
for more information.  Please report problems to the issues list on
[GitHub][2].

[1]: https://github.com/openss7/adwm
[2]: https://github.com/openss7/adwm/issues
[3]: https://github.com/openss7/adwm/blob/0.7.17/NEWS
[4]: https://github.com/openss7/adwm/blob/0.7.17/ChangeLog
[5]: https://github.com/openss7/adwm/blob/0.7.17/TODO
[6]: https://github.com/openss7/adwm/blob/0.7.17/COMPLIANCE
[7]: https://github.com/openss7/adwm/blob/0.7.17/INSTALL
[8]: https://github.com/openss7/adwm/blob/0.7.17/LICENSE
[9]: https://github.com/openss7/adwm/blob/0.7.17/COPYING

[ vim: set ft=markdown sw=4 tw=72 nocin nosi fo+=tcqlorn spell: ]: #
