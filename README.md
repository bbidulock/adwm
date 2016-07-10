
### adwm

Package adwm-0.6.2 was released under GPL license 2016-07-08.

This was originally a fork of Echinus which in turn was a fork of dwm,
and borrows concepts from velox, awesome and spectrwm.  The source
for adwm is hosted on GitHub at [](https://github.com/bbidulock/adwm).
What it includes is a full rewrite with significant updates and additions
resulting in full EWMH (NetwM), WMH (WinWM), MWMH (CDE/Motif),
ICCCM 2.0 compliance and support.


### Release

This is the adwm-0.6.2 package, released 2016-07-08.  This release,
and the latest version, can be obtained from the GitHub repository at
[](https://github.com/bbidulock/adwm), using a command such as:

    $> git clone https://github.com/bbidulock/adwm.git

Please see the [NEWS](NEWS) file for release notes and history of user visible
changes for the current version, and the [ChangeLog](ChangeLog) file for a more
detailed history of implementation changes.  The [TODO](TODO) file lists
features not yet implemented and other outstanding items.  The file
[COMPLIANCE](COMPLIANCE) lists the current state of XDG compliance.

Please see the [INSTALL](INSTALL) file for installation instructions.

When working from ```git(1)```, follow the instructions in this file.  An
abbreviated installation procedure that works for most applications
appears below.

Echinus and dwm were published under the MIT/X Consortium License that
can be found in the file [LICENSE](LICENSE).  As over 90% of the source file
lines have been supplied or replaced and the remaining 10% is not
subject to copyright, this release is published under the GPL license
that can be found in the file [COPYING](COPYING).


### Quick Start:

The quickest and easiest way to get adwm up and running is to run
the following commands:

    $> git clone https://github.com/bbidulock/adwm.git adwm
    $> cd adwm
    $> ./autogen.sh
    $> ./configure --prefix=/usr --mandir=/usr/share/man
    $> make V=0
    $> make DESTDIR="$pkgdir" install

This will configure, compile and install adwm the quickest.  For
those who like to spend the extra 15 seconds reading ./configure --help,
some compile time options can be turned on and off before the build.
adwm, however, does not provide any compile time options of its
own.

For general information on GNU's ./configure, see the file [INSTALL](INSTALL).


### Running adwm

Read the manual page after installation: man adwm.


### Issues

Report issues to [](https://github.com/bbidulock/adwm/issues).
