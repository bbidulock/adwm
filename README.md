[adwm -- read me first file.  2017-09-10]: #

adwm
===============

Package `adwm-0.6.9` was released under GPLv3 license 2017-09-10.

This was originally a fork of Echinus which in turn was a fork of `dwm(1)`, and
borrows concepts from `velox(1)`, `awesome(1)` and `spectrwm(1)`.  The source
for __`adwm`__ is hosted on [GitHub][1].  What it includes is a full
rewrite with significant updates and additions resulting in full EWMH (NetwM),
WMH (WinWM), MWMH (CDE/Motif), ICCCM 2.0 compliance and support.


Release
-------

This is the `adwm-0.6.9` package, released 2017-09-10.  This release, and
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

The quickest and easiest way to get __`adwm`__ up and running is to run the
following commands:

    $> git clone https://github.com/bbidulock/adwm.git
    $> cd adwm
    $> ./autogen.sh
    $> ./configure
    $> make
    $> make DESTDIR="$pkgdir" install

This will configure, compile and install __`adwm`__ the quickest.  For those who
like to spend the extra 15 seconds reading `./configure --help`, some compile
time options can be turned on and off before the build.

For general information on GNU's `./configure`, see the file [INSTALL][7].


Running
-------

Read the manual page after installation:

    man adwm


Issues
------

Report issues on GitHub [here][2].



[1]: https://github.com/bbidulock/adwm
[2]: https://github.com/bbidulock/adwm/issues
[3]: https://github.com/bbidulock/adwm/blob/0.6.9/NEWS
[4]: https://github.com/bbidulock/adwm/blob/0.6.9/ChangeLog
[5]: https://github.com/bbidulock/adwm/blob/0.6.9/TODO
[6]: https://github.com/bbidulock/adwm/blob/0.6.9/COMPLIANCE
[7]: https://github.com/bbidulock/adwm/blob/0.6.9/INSTALL
[8]: https://github.com/bbidulock/adwm/blob/0.6.9/LICENSE
[9]: https://github.com/bbidulock/adwm/blob/0.6.9/COPYING

[ vim: set ft=markdown sw=4 tw=80 nocin nosi fo+=tcqlorn spell: ]: #
