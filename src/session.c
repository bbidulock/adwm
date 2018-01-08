/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "config.h"
#include "session.h"	/* verification */

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

void
init_sm_client(void)
{
}

void
init_sm(void)
{
	init_sm_manager();
	init_sm_client();
}
