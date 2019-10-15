/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "resource.h"
#include "config.h"
#include "restore.h" /* verification */

/*
 * Steps involved in restoring window manager state in response to a -restore SAVEFILE specified on
 * as an argument to the wm launch command:
 *
 * 1. Create a temporary Xresource configuration databases for startup and read configuration from
 *    the specified SAVEFILE.
 *
 * 2. Locate and read the client state state file, one at a time, using the following method:
 *    a. If the client is an X11R6 SM supporting client, save the client state for use when the
 *       client maps it's top-level window with the appropriate X11R6 SM properties.
 *    b. If the client is an X11R5 SM supporting client, launch the WM_COMMAND associated with the
 *       client, with startup notification, and save the client state for use when the client maps
 *       it's top-level window without the appropriate X11R6 SM properties.  Also save the startup
 *       notification id used, for use matching the mapped window.
 *    c. If the client does not support X11R5 SM, but had an explicit WM_COMMAND, launch the
 *       WM_COMMAND associated with the client, with startup notification, and save the client state
 *       for use when the client maps it's top-level window without the appropriate X11R6 SM
 *       properties.  Also save the startup notification id used, for use matching the mapped
 *       window.
 * The startup process of the window manager must be altered as follows:
 *
 * 1. During startup, if a restore file has been read, skip reading configuration from the various
 *    configuration files associated with the window manager.
 *
 * 2. When clients are to be managed and have X11R6 SM properties set, restore the state of the
 *    client from the corresponding saved client state entry and delete the entry.
 *
 * 3. When clients are to be managed and do not have X11R6 SM properties set, perform startup
 *    notification assistance first and then use the characteristics of the client to match it to a
 *    launched non-X11R6 client and restore the state of the client from the corresponding saved
 *    client state entry and delete the entry.
 *
 * Note that when restoring client state, the focus order, stacking order, and client order must be
 * restored to the same order that it was previously.  This likely means that we will have to ba
 * able to insert placeholders in these lists and adjust the corresponding routines to perform
 * layouts (such as tiled) containing blank positions (positions that will be filled in later).
 * The cardinal order in each list can be saved in the client and then the lists resorted once the
 * client has been restored.
 *
 * Note also that the window manager should not allow user interaction with the window manager until
 * all of the clients have been restored, or after some time delay since the last mapped client.
 *
 * Note also, that the startup process should warp the pointer to its previous location (desktop and
 * monitor) and disable pointer and keyboard events (with grab) until the startup process has
 * completed as well.  Then the appropriate client should be focused and all pointer and keyboard
 * events discarded before ungrabbing pointer and keyboard.
 */

