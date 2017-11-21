/* See COPYING file for copyright and license details. */

#include "adwm.h"

/*
 * The purpose of this file is to provie a loadable module that provides
 * functions to mimic the behaviour and style of icewm.  This module reads the
 * icewm configuration file to determine configuration information, and accesses
 * an icewm(1) style file.  It also reads the icewm preferences and prefoverride
 * configurations file to find key bindings.
 *
 * Note that icewm(1) allows its theme files to override pretty much anything in
 * the 'preferences' file but cannot override settings provided in the
 * 'prefoverride' file.  There are some global key bindings for starting
 * applications in the 'keys' file.
 *
 * This modules is really just intended to access a large volume of icewm
 * styles.
 */
