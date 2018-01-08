/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "ewmh.h"
#include "config.h"
#include "session.h"	/* verification */

static IceConn iceConn = 0;
static SmcConn smcConn = 0;
// static SmsConn smsConn = 0;

static Status
new_client_cb(SmsConn smsConn, SmPointer data, unsigned long *mask, SmsCallbacks *cb, char **reason)
{
	/* FIXME: write this function */
	return (0);
}

Bool
hostbased_auth_cb(char *hostname)
{
	return (False);		/* refuse host based authentication */
}

static int num_xprts = 0;
static IceListenObj *listen_objs;

static void
close_listeners(void)
{
	IceFreeListenObjs(num_xprts, listen_objs);
}

static IceIOErrorHandler prev_handler = NULL;

static void
manager_io_error_handler(IceConn ice)
{
	if (prev_handler)
		(*prev_handler) (ice);
}

static void
install_io_error_handler(void)
{
	IceIOErrorHandler default_handler = NULL;

	prev_handler = IceSetIOErrorHandler(NULL);
	default_handler = IceSetIOErrorHandler(manager_io_error_handler);
	if (prev_handler == default_handler)
		prev_handler = NULL;
}

IceAuthDataEntry *authDataEntries = NULL;
char *networkIds;

void
init_sm_manager(void)
{
	char err[256] = { 0, };

	install_io_error_handler();

	if (!SmsInitialize(NAME, VERSION, new_client_cb, NULL, hostbased_auth_cb, sizeof(err), err)) {
		EPRINTF("SmsInitialize: %s\n", err);
		exit(EXIT_FAILURE);
	}
	if (!IceListenForConnections(&num_xprts, &listen_objs, sizeof(err), err)) {
		EPRINTF("IceListenForConnections: %s\n", err);
		exit(EXIT_FAILURE);
	}
	atexit(close_listeners);
}

#if 0
static void
save_wmstate(void)
{
	/* Note: it is easiest for us to save the information as an X Resource
	   file because that is what we use to save the information; however,
	   the X resource database must be updated before writing it.  xrdb is
	   the name of our resource database. */
}
#endif

static void
save_client(AScreen *s, Client *c)
{
}

/** @brief save yourself phase 2 callback
  *
  * This is where all the work is donw in saving the state of the windows.  It
  * is not done in phase 1 because phase 1 is used for the other clients to make
  * sure that all the property information on their windows is correct and up to
  * date.
  */
static void
save_yourself_phase2_cb(SmcConn conn, SmPointer data)
{
	AScreen *save_screen = scr;
	Client *c;

	/* Step 1: set all the of the WM's session management client properties */

	/* Step 2: find out where to save, check SM_SAVE_DIR environment variable */

	/* Step 3: figure out he save file name */

	/* Step 4: write the file header */

	/* Step 5: write window configuration entries for each client */


	for (scr = screens; scr < screens + nscr; scr++)
		for (c = scr->clients; c; c = c->next)
			save_client(scr, c);

	scr = save_screen;

	/* Step 6: add -clientId and -restore options to our command line */

	/* Step 7: set the command line property, dicsarding previous */

	/* Step 8: send save yourself done */
	SmcSaveYourselfDone(conn, True);
}

static Bool sent_save_done = False;

void
send_save_yourself(Client *c)
{
	XEvent ev;

	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = False;
	ev.xclient.display = dpy;
	ev.xclient.window = c->win;
	ev.xclient.message_type = _XA_WM_PROTOCOLS;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = _XA_WM_SAVE_YOURSELF;
	ev.xclient.data.l[1] = user_time;
	ev.xclient.data.l[2] = 0;
	ev.xclient.data.l[3] = 0;
	ev.xclient.data.l[4] = 0;

	c->is.saving = True;
	c->save_time = user_time;

	XSendEvent(dpy, c->win, False, NoEventMask, (XEvent *) &ev);
	XSync(dpy, False);
}

/** @brief save yourself callback
  *
  * The session manager sends a SaveYourself message to the client either to
  * checkpoint it or so that it can save its state.  The client responds with
  * zero or more calls to SmcSetProperties to update the properties indicating
  * how to restart the client.  When all the properties have been set, the
  * client calls SmcSaveYourselfDone.
  *
  * If #interactStyle is SmInteractStyleNone, the client must not interact with
  * the user while saving state.  If #interactStyle is SmInteractStyleErrors,
  * the client may interact with the user only if an error condition arises.  If
  * #interactStyle is SmInteractStyleAny, then the client may interact with the
  * user at any time, the client must call SmcInteractRequest and wait for an
  * Interact message from the session manager.  When the client is done
  * inteacting with the user, it calls SmcInteractDone.  The client may only
  * call SmcInteractRequest after it receives a SaveYourself message and before
  * it calls SmcSaveYourselfDone.
  *
  * If #saveType is SmSaveLocal, the client must update the properties to
  * reflect its current state.  Specifically, it should save enough information
  * to restore the state as seen by the user of this client.  It should not
  * affect the state as seen by other users.  If #saveType is SmSaveGlobal, the
  * user wants the client to commit all of its data to permanent, globally
  * accessible storage.  If #saveType is SmSaveBoth, the client should do both
  * of these (it should first commit the data to permanent storage before
  * updating its properies).
  *
  * The #shutdown argument specifies whether the system is being shut down.  The
  * interaction is different depending on whether or not shutdown is set.  If
  * not shutting down, the client should save its state and wait for a "Save
  * Complete" message.  If shutting down, the client must save state and then
  * prevent interaction until it receives either a "Die" or a "Shutdown
  * Cancelled".
  *
  * The #fast argument specifies that the client should save its state as
  * quickly as possible.  For example, if the session manager knows that power
  * is about to fail, it sould set #fast to True.
  *
  * During Phase 1 we need to ask any clients that only do X11R5 session
  * management and advertize WM_SAVE_YOURSELF in WM_PROTOCOLS to save themselves
  * and update their WM_COMMAND properties.  We will not be able to send
  * RequestSaveYourselfPhase2 until we are complete the former.  X11R5 clients
  * were not allowed to interact with the user during the WM_SAVE_YOURSELF
  * protocol, so there should be no problem with unwanted user interaction
  * occurring.
  */
void
save_yourself_cb(SmcConn conn, SmPointer data, int saveType, Bool shutdown, int interactStyle, Bool fast)
{
	AScreen *s;
	Client *c;
	int i, saving_clients = 0;

	for (i = 0, s = screens; i < nscr; i++, s++) {
		for (c = s->clients; c; c = c->next) {
			Atom *atoms;
			unsigned long j, n;

			if (!c->win)
				continue;
			if (c->is.saving) {
				saving_clients++;
				continue;
			}
			if ((atoms = getatom(c->win, _XA_WM_PROTOCOLS, &n))) {
				for (j = 0; j < n; j++) {
					if (atoms[j] == _XA_WM_SAVE_YOURSELF) {
						send_save_yourself(c);
						saving_clients++;
					}
				}
				XFree(atoms);
			}
		}
	}
	if (saving_clients) {
		/* FIXME: wait for clients to update WM_COMMAND. */
	}
	if (!SmcRequestSaveYourselfPhase2(conn, save_yourself_phase2_cb, NULL)) {
		SmcSaveYourselfDone(conn, False);
		sent_save_done = True;
	} else
		sent_save_done = False;
}

/** @brief die callback
  *
  * The session manager sends a Die message to the client when it wants it to
  * die.  The client should respond by calling SmcCloseConnection.  A session
  * manager that behaves properly will send a SaveYourself message before a Die
  * message.
  */
void
die_cb(SmcConn conn, SmPointer data)
{
	SmcCloseConnection(conn, 0, NULL);
	exit(EXIT_SUCCESS);
}

void
save_complete_cb(SmcConn conn, SmPointer data)
{
	/* doesn't really do anything */
}

void
shutdown_cancelled_cb(SmcConn conn, SmPointer data)
{
	if (!sent_save_done) {
		SmcSaveYourselfDone(conn, False);
		sent_save_done = True;
	}
}

static unsigned long cb_mask =
    SmcSaveYourselfProcMask | SmcDieProcMask |
    SmcSaveCompleteProcMask | SmcShutdownCancelledProcMask;

static SmcCallbacks cb = {
	/* *INDENT-OFF* */
	.save_yourself		= { .callback = save_yourself_cb,	.client_data = NULL, },
	.die			= { .callback = die_cb,			.client_data = NULL, },
	.save_complete		= { .callback = save_complete_cb,	.client_data = NULL, },
	.shutdown_cancelled	= { .callback = shutdown_cancelled_cb,	.client_data = NULL, },
	/* *INDENT-ON* */
};

void
init_sm_client(void)
{
	char *env;
	char err[256] = { 0, };
	if (smcConn) {
		DPRINTF("already connected\n");
		return;
	}
	if (!(env = getenv("SESSION_MANAGER"))) {
		if (clientId)
			EPRINTF("clientId specified but no SESSION_MANAGER\n");
		return;
	}
	smcConn = SmcOpenConnection(env, NULL, SmProtoMajor, SmProtoMinor,
			cb_mask, &cb, clientId, &clientId,
			sizeof(err), err);
	if (!smcConn) {
		EPRINTF("SmcOpenConnection: %s\n", err);
		exit(EXIT_FAILURE);
	}
	iceConn = SmcGetIceConnection(smcConn);
}

void
init_sm(void)
{
	init_sm_manager();
	init_sm_client();
}
