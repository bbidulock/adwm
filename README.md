[adwm -- read me first file.  2021-05-08]: #

adwm
===============

Package `adwm-0.7.12` was released under GPLv3 license 2021-05-08.

This was originally a fork of Echinus which in turn was a fork of `dwm(1)`, and
borrows concepts from `velox(1)`, `awesome(1)`, `spectrwm(1)` and `wmii(1)`.
The source for `adwm` is hosted on [GitHub][1].  What it includes is a full
rewrite with significant updates and additions resulting in full EWMH (NetwM),
WMH (WinWM), MWMH (CDE/Motif), ICCCM 2.0 compliance and support.

Release
-------

This is the `adwm-0.7.12` package, released 2021-05-08.  This release, and
the latest version, can be obtained from [GitHub][1], using a command such as:

    $> git clone https://github.com/bbidulock/adwm.git

Please see the [NEWS][3] file for release notes and history of user visible
changes for the current version, and the [ChangeLog][4] file for a more
detailed history of implementation changes.  The [TODO][5] file lists features
not yet implemented and other outstanding items.

The file [COMPLIANCE][6] lists the current state of EWMH/ICCCM compliance.

Please see the [INSTALL][7] file for installation instructions.

When working from `git(1)`, please use this file.  An abbreviated
installation procedure that works for most applications appears below.

`echinus(1)` and `dwm(1)` were published under the MIT/X Consortium
License that can be found in the file [LICENSE][8].  As over 90% of the source
file lines have been supplied or replaced and the remaining 10% is not subject
to copyright.
This release is published under GPLv3.  Please see the license
in the file [COPYING][9].


Quick Start
-----------

The quickest and easiest way to get `adwm` up and running is to run the
following commands:

    $> git clone https://github.com/bbidulock/adwm.git
    $> cd adwm
    $> ./autogen.sh
    $> ./configure
    $> make
    $> make DESTDIR="$pkgdir" install

This will configure, compile and install `adwm` the quickest.  For those who
like to spend the extra 15 seconds reading `./configure --help`, some compile
time options can be turned on and off before the build.

For general information on GNU's `./configure`, see the file [INSTALL][7].


Running
-------

Read the manual page after installation:

    man adwm


Features
--------

Following are some of the features provided by `adwm(1)` not provided by similar
window managers:

General:

- full NetWM/EWMH, WinWM/WMH (gnome), OSF/Motif and ICCCM 2.0 compliance;
- full support for ICCCM 2.0 client and global modality;
- full RANDR and Xinerama compliance with full support for multi-head setups;
- full support for Window Maker dock applications in floating and tiling modes;
- window manager based auto-hide for panels and docks (struts);
- three focus models: click-to-focus, sloppy and all-sloppy;
- soon to come X11R6 session management;
- very wide array of available key-binding actions;
- key-binding defaults consistent with a wide array of light-weight and popular
  window managers.

Views, Workspaces and Desktops:

- view/tag based workspaces and desktops;
- scroll wheel support for switching desktops;
- key bindings for switching desktops observes EMWH desktop layout.

Floating/stacking mode:

- smart cascading placement algorithm;
- window decorations: title bar, resize grips, wide array of configurable
  buttons.;
- themed buttons include hovered, toggled, pressed, focused and unfocused
  pixmaps.;
- multiple mouse actions per title bar button based on mouse button pressed.;
- separate click and click-drag actions for title bar, borders and grips.;
- windows can snap to other windows and workspace/monitor boundaries.;
- key bindings for moving and resizing windows without using the mouse.
- drop shadowed XFT text in title bars, with separate color selection for
  active and inactive windows;
- dragging of windows between monitor;
- `metacity(1)`-like drag-to-left, -right or -top functions to optimize
  single-desktop operation.

Tiling modes:

- per-monitor tiling layouts for multi-head setups;
- multiple tiling modes including master/stacking, monocle and grid (but, no
  dwindle or centered-master modes, yet);
- adjustable window borders and inter-window margins;
- one pixel open space around dynamic desktop area for clicking on the root
  window in tiled modes;
- ability to drag-and-swap windows in tiling layouts (ala `awesome(1)`,
  `wmii(1)` and `spectrwm(1)`);

Issues
------

Report issues on GitHub [here][2].



[1]: https://github.com/bbidulock/adwm
[2]: https://github.com/bbidulock/adwm/issues
[3]: https://github.com/bbidulock/adwm/blob/0.7.12/NEWS
[4]: https://github.com/bbidulock/adwm/blob/0.7.12/ChangeLog
[5]: https://github.com/bbidulock/adwm/blob/0.7.12/TODO
[6]: https://github.com/bbidulock/adwm/blob/0.7.12/COMPLIANCE
[7]: https://github.com/bbidulock/adwm/blob/0.7.12/INSTALL
[8]: https://github.com/bbidulock/adwm/blob/0.7.12/LICENSE
[9]: https://github.com/bbidulock/adwm/blob/0.7.12/COPYING

[ vim: set ft=markdown sw=4 tw=80 nocin nosi fo+=tcqlorn spell: ]: #
