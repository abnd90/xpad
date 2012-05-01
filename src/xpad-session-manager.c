/*

Copyright (c) 2001-2007 Michael Terry

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include "xpad-session-manager.h"

#ifndef X_DISPLAY_MISSING

#include <X11/SM/SMlib.h>
#include <stdio.h>	/* only for printf */
#include <sys/types.h> /* for getuid and getpwuid */
#include <sys/select.h>	/* for select */
#include <stdlib.h>	/* for free */
#include <string.h>	/* for strlen */
#include <unistd.h>	/* for getuid */
#include <pwd.h>	/* for getpwuid */
#include <sys/types.h>	/* for getuid and getpwuid */
#include <sys/time.h>	/* for struct timeval */
#include "xpad-app.h"

static SmcConn xpad_session_manager_conn = NULL;
static int xpad_interact_style;
static gboolean xpad_shutdown;
static gboolean xpad_saving;
static int xpad_ice_fd;
static gchar *client_id = NULL;
static gboolean set_props = TRUE;
static gboolean blocking = FALSE;

static void xpad_session_manager_save_yourself (SmcConn smc_conn, SmPointer client_data,
                                                int save_type, Bool shutdown,
                                                int interact_style, Bool fast);
static void xpad_session_manager_die (SmcConn smc_conn, SmPointer client_data);
static void xpad_session_manager_shutdown_cancelled (SmcConn smc_conn, SmPointer client_data);
static void xpad_session_manager_save_complete (SmcConn smc_conn, SmPointer client_data);

#define RETURN_IF_BAD_CONN(conn)	{if (xpad_session_manager_conn != conn) {return;}}
#define RETURN_IF_NOT_SAVING()		{if (!xpad_saving) {return;}}

static gboolean
xpad_session_manager_cycle (GIOChannel *source, GIOCondition condition,
                            gpointer data)
{	
  gboolean rv = TRUE;
	IceConn ice_conn;
	
	ice_conn = SmcGetIceConnection (xpad_session_manager_conn);
	
	switch (IceProcessMessages (ice_conn, NULL, NULL)) {
	default:
	case IceProcessMessagesSuccess:
		break;
	
	case IceProcessMessagesIOError:
		xpad_session_manager_shutdown ();
		rv = FALSE;
		break;
	
	case IceProcessMessagesConnectionClosed:
		xpad_session_manager_conn = NULL;
		rv = FALSE;
		break;
	}
	
	return rv;
}

static gboolean
xpad_session_manager_add_cycle_to_main_loop ()
{
  GIOChannel *channel;
  
	if (!xpad_session_manager_conn)
		return FALSE;
	
	if (xpad_ice_fd == -1)
	  return FALSE;
	
	channel = g_io_channel_unix_new (xpad_ice_fd);
	g_io_add_watch (channel, G_IO_IN, xpad_session_manager_cycle, NULL);
	g_io_channel_unref (channel);
	
	return FALSE;
}

static void
xpad_session_manager_start_interact_callback (SmcConn smc_conn, SmPointer client_data)
{
  if (blocking)
    gtk_main_quit ();
	blocking = FALSE;
}

gboolean
xpad_session_manager_start_interact (gboolean error)
{
	Status status;
	
	if (!xpad_session_manager_conn)
		return TRUE;
	
	if (!xpad_saving)
		return TRUE;
	
	switch (xpad_interact_style) {
	case SmInteractStyleNone:
		return FALSE;
	
	case SmInteractStyleErrors:
		if (!error)
			return FALSE;
		break;
	
	case SmInteractStyleAny:
		break;
	
	default:
		return TRUE;
	}
	
	blocking = TRUE;
	status = SmcInteractRequest (xpad_session_manager_conn, 
		error ? SmDialogError : SmDialogNormal,
		xpad_session_manager_start_interact_callback, NULL);
	
	if (status)
		gtk_main ();
	else
		blocking = FALSE;
	
	return status ? TRUE : FALSE;
}

void
xpad_session_manager_stop_interact (gboolean stop_shutdown)
{
	if (!xpad_session_manager_conn)
		return;
	
	if (!blocking)
		return;
	
	SmcInteractDone (xpad_session_manager_conn, stop_shutdown);
}

static void
xpad_session_manager_ice_connection_watch (IceConn ice_conn,
	IcePointer client_data, Bool opening, IcePointer *watch_data)
{
	int fd = IceConnectionNumber (ice_conn);
	
	if (opening)
		xpad_ice_fd = fd;
	else
		xpad_ice_fd = -1;
  
	xpad_session_manager_add_cycle_to_main_loop ();
}

static void
xpad_session_manager_set_properties (void)
{
	struct {
		SmPropValue clone[1];
		SmPropValue program[1];
		SmPropValue restart[3];
		SmPropValue user[1];
		SmPropValue process[1];
	} vals;
	SmProp prop[] = {
		{SmCloneCommand, SmLISTofARRAY8, 1, NULL},
		{SmProgram, SmARRAY8, 1, NULL},
		{SmRestartCommand, SmLISTofARRAY8, 3, NULL},
		{SmUserID, SmARRAY8, 1, NULL},
		{SmProcessID, SmARRAY8, 1, NULL}
	};
	SmProp *props[G_N_ELEMENTS (prop)];
	struct passwd *pw;
	int i;
	gchar *pid_str;
	gchar *command = g_strdup (xpad_app_get_program_path ());
	
	prop[0].vals = vals.clone;
	prop[1].vals = vals.program;
	prop[2].vals = vals.restart;
	prop[3].vals = vals.user;
	prop[4].vals = vals.process;
	
	for (i = 0; i < G_N_ELEMENTS (prop); i++) {
		props[i] = &prop[i];
	}
	
	pw = getpwuid (getuid ());
	
	vals.user->value = pw ? pw->pw_name : "";
	vals.user->length = strlen (vals.user->value);
	
	vals.program->value = command;
	vals.program->length = strlen (vals.program->value);
	
	vals.clone->value = command;
	vals.clone->length = strlen (vals.clone->value);
	
	vals.restart[0].value = command;
	vals.restart[0].length = strlen (vals.restart[0].value);
	
	vals.restart[1].value = "--sm-client-id";
	vals.restart[1].length = strlen (vals.restart[1].value);
	
	vals.restart[2].value = client_id;
	vals.restart[2].length = strlen (vals.restart[2].value);
	
	pid_str = g_strdup_printf ("%i", getpid ());
	vals.process->value = pid_str;
	vals.process->length = strlen (vals.process->value);
	
	SmcSetProperties (xpad_session_manager_conn, 4, (SmProp **) &props);
	
	g_free (pid_str);
	g_free (command);
}

void
xpad_session_manager_init (void)
{
	char error_string_net[100];
	char *client_id_ret;
	unsigned long mask = 
		SmcSaveYourselfProcMask |
		SmcDieProcMask |
		SmcSaveCompleteProcMask |
		SmcShutdownCancelledProcMask;
	SmcCallbacks callbacks = {
		{xpad_session_manager_save_yourself, NULL},
		{xpad_session_manager_die, NULL},
		{xpad_session_manager_save_complete, NULL},
		{xpad_session_manager_shutdown_cancelled, NULL}};
	
	if (xpad_session_manager_conn) {
		/* we were already connected... */
		xpad_session_manager_shutdown ();
	}
	
	xpad_saving = FALSE;
	xpad_interact_style = SmInteractStyleAny;
	xpad_session_manager_conn = SmcOpenConnection (NULL, NULL, 
		SmProtoMajor, SmProtoMinor,
		mask, &callbacks, client_id, &client_id_ret,
		100, error_string_net);
	
	client_id = g_strdup (client_id_ret);
	free (client_id_ret);
	
	xpad_ice_fd = -1;
	IceAddConnectionWatch (xpad_session_manager_ice_connection_watch, NULL);
	
	gdk_set_sm_client_id (client_id);
}

void
xpad_session_manager_set_id (const gchar *id)
{
	gboolean alread_set_up = xpad_session_manager_conn ? TRUE : FALSE;
	
	if (alread_set_up)
		xpad_session_manager_shutdown ();
	
	client_id = g_strdup (id);
	
	if (alread_set_up)
		xpad_session_manager_init ();
}

void
xpad_session_manager_shutdown (void)
{
	if (!xpad_session_manager_conn)
		return;
	
	if (client_id)
	{
		g_free (client_id);
		client_id = NULL;
	}
	
	SmcCloseConnection (xpad_session_manager_conn, 0, NULL);
	xpad_session_manager_conn = NULL;
}

static void
xpad_session_manager_save_global (Bool fast)
{
	/* No need to do anything.  Currently, all xpad pads are always current with
	hard drive. */
}

static void
xpad_session_manager_save_local (Bool fast)
{
	/* should also save cursor positions and open accessory windows */
	
	if (set_props)
	{
		xpad_session_manager_set_properties ();
		set_props = FALSE;
	}
}

static void
xpad_session_manager_save_yourself (SmcConn smc_conn, SmPointer client_data,
                                    int save_type, Bool shutdown, int interact_style,
                                    Bool fast)
{
	RETURN_IF_BAD_CONN (smc_conn);
	
	xpad_interact_style = interact_style;
	xpad_shutdown = shutdown ? TRUE : FALSE;
	xpad_saving = TRUE;
	
	switch (save_type) {
	case SmSaveLocal:
		xpad_session_manager_save_local (fast);
		break;
	
	case SmSaveGlobal:
		xpad_session_manager_save_global (fast);
		break;
	
	case SmSaveBoth:
		xpad_session_manager_save_global (fast);
		xpad_session_manager_save_local (fast);
		break;
	}
	
	SmcSaveYourselfDone (smc_conn, True);
	
	if (shutdown)
		xpad_interact_style = SmInteractStyleNone;
}

static void
xpad_session_manager_die (SmcConn smc_conn, SmPointer client_data)
{
	RETURN_IF_BAD_CONN (smc_conn);
	xpad_shutdown = True;
	
	if (blocking)
	  gtk_main_quit ();
	blocking = FALSE;
	xpad_saving = FALSE;
	xpad_interact_style = SmInteractStyleAny;
	
	gtk_main_quit ();
}

static void
xpad_session_manager_shutdown_cancelled (SmcConn smc_conn, SmPointer client_data)
{
	RETURN_IF_BAD_CONN (smc_conn);
	RETURN_IF_NOT_SAVING ();
	xpad_shutdown = False;
	
	if (blocking)
	  gtk_main_quit ();
	blocking = FALSE;
	xpad_saving = FALSE;
	xpad_interact_style = SmInteractStyleAny;
}

static void
xpad_session_manager_save_complete (SmcConn smc_conn, SmPointer client_data)
{
	RETURN_IF_BAD_CONN (smc_conn);
	RETURN_IF_NOT_SAVING ();
	
	if (blocking)
	  gtk_main_quit ();
	blocking = FALSE;
	xpad_saving = FALSE;
	xpad_interact_style = SmInteractStyleAny;
}

#else 

gboolean
xpad_session_manager_start_interact (gboolean error)
{
	return TRUE;
}

void
xpad_session_manager_stop_interact (gboolean stop_shutdown)
{
}

void
xpad_session_manager_init (void)
{
}

void xpad_session_manager_shutdown (void)
{
}

void
xpad_session_manager_set_id (const gchar *id)
{
}

#endif /* X_DISPLAY_MISSING */
