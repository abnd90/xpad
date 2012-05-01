/**
 * Copyright (c) 2004-2007 Michael Terry
 * Copyright (c) 2009 Paul Ivanov
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* required by socket stuff */
/* define _GNU_SOURCE here because that makes our sockets work nice
 Unfortunately, we lose portability... */
#define _GNU_SOURCE	1
#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <string.h>
#include <stdlib.h> /* for exit */

#include "../config.h"
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "fio.h" /* for fio_get_info_from_file */
#include "help.h"
#include "prefix.h"
#include "xpad-app.h"
#include "xpad-pad.h"
#include "xpad-pad-group.h"
#include "xpad-session-manager.h"
#include "xpad-tray.h"

/* Seems that some systems (sun-sparc-solaris2.8 at least), need the following three #defines. 
   These were provided by Alan Mizrahi <alan@cesma.usb.ve>.
*/
#ifndef PF_LOCAL
#define PF_LOCAL PF_UNIX
#endif

#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif

#ifndef SUN_LEN
#define SUN_LEN(sunp) ((size_t)((struct sockaddr_un *)0)->sun_path + strlen((sunp)->sun_path))
#endif


static gint xpad_argc;
static gchar **xpad_argv;
static gboolean option_nonew;
static gboolean option_new;
static gboolean option_hide;
static gboolean option_show;
static gboolean option_toggle;
static gboolean option_version;
static gboolean option_quit;
static gchar **option_files;
static gchar *option_smid;
static gchar *config_dir;
static gchar *program_path;
static gchar *server_filename;
static gint server_fd;
static FILE *output;
static gboolean xpad_translucent = FALSE;
static XpadPadGroup *pad_group;
static gint pads_loaded_on_start = 0;

static gboolean  process_local_args         (gint *argc, gchar **argv[]);
static gboolean  process_remote_args        (gint *argc, gchar **argv[], gboolean have_gtk);

static gboolean  config_dir_exists          (void);
static gchar    *make_config_dir            (void);
static void      register_stock_icons       (void);
static gint      xpad_app_load_pads         (void);
static gboolean  xpad_app_quit_if_no_pads   (XpadPadGroup *group);
static gboolean  xpad_app_first_idle_check  (XpadPadGroup *group);
static gboolean  xpad_app_pass_args         (void);
static gboolean  xpad_app_open_proc_file    (void);


static void
xpad_app_init (int argc, char **argv)
{
	gboolean first_time;
	gboolean have_gtk;
/*	GdkVisual *visual;*/
	
	/* Set up i18n */
#ifdef ENABLE_NLS
	gtk_set_locale ();
	bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
	
	have_gtk = gtk_init_check (&argc, &argv);
	xpad_argc = argc;
	xpad_argv = argv;
	output = stdout;
	
	/* Set up config directory. */
	first_time = !config_dir_exists ();
	config_dir = make_config_dir ();
	
	/* create master socket name */
	server_filename = g_build_filename (xpad_app_get_config_dir (), "server", NULL);
	
	if (!have_gtk)
	{
		/* We don't have GTK+, but we can still do
		   --version or --help and such.  Plus, we
		   can pass commands to a remote instance. */
		process_local_args (&xpad_argc, &xpad_argv);
		if (!xpad_app_pass_args ())
		{
			process_remote_args (&xpad_argc, &xpad_argv, FALSE);
			fprintf (output, "%s\n", _("Xpad is a graphical program.  Please run it from your desktop."));
		}
		exit (0);
	}
	
	g_set_application_name (_("Xpad"));
	gdk_set_program_class (PACKAGE);
	
	/* Set up translucency. */
/*	visual = gdk_visual_get_best_with_depth (32);
	if (visual)
	{
		GdkColormap *colormap;
		colormap = gdk_colormap_new (visual, TRUE);
		gtk_widget_set_default_colormap (colormap);
		xpad_translucent = TRUE;
	}*/
	
	/* Set up program path. */
	if (xpad_argc > 0)
		program_path = g_find_program_in_path (xpad_argv[0]);
	else
		program_path = NULL;
	
	process_local_args (&xpad_argc, &xpad_argv);
	
	if (xpad_app_pass_args ())
		exit (0);
	
	/* Race condition here, between calls */
	xpad_app_open_proc_file ();
	
	register_stock_icons ();
	gtk_window_set_default_icon_name (PACKAGE);
	
	pad_group = xpad_pad_group_new();
	process_remote_args (&xpad_argc, &xpad_argv, TRUE);
	
	xpad_tray_open ();
	xpad_session_manager_init ();
	
	/* load all pads */
	pads_loaded_on_start = xpad_app_load_pads ();
	if (pads_loaded_on_start == 0 && !option_new) {
		if (!option_nonew) {
			GtkWidget *pad = xpad_pad_new (pad_group);
			gtk_widget_show (pad);
		}
	}
	
	g_idle_add ((GSourceFunc)xpad_app_first_idle_check, pad_group);
	
	if (first_time)
		show_help ();
	
	g_free (server_filename);
	server_filename = NULL;
}


gint main (gint argc, gchar **argv)
{
	xpad_app_init (argc, argv);
	
	gtk_main ();
	
	return 0;
}




/* parent and secondary may be NULL.
 * Returns when user dismisses error.
 */
void
xpad_app_error (GtkWindow *parent, const gchar *primary, const gchar *secondary)
{
	GtkWidget *dialog;
	
	if (!xpad_session_manager_start_interact (TRUE))
		return;
	
	g_printerr ("%s\n", primary);
	
	dialog = xpad_app_alert_new (parent, GTK_STOCK_DIALOG_ERROR, primary, secondary);
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_OK, 1, NULL);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	
	xpad_session_manager_stop_interact (FALSE);
}



G_CONST_RETURN gchar *
xpad_app_get_config_dir (void)
{
	return config_dir;
}


/* Returns absolute path to our own executable. May be NULL. */
G_CONST_RETURN gchar *
xpad_app_get_program_path (void)
{
	return program_path;
}


XpadPadGroup *
xpad_app_get_pad_group (void)
{
	return pad_group;
}

gboolean
xpad_app_get_translucent (void)
{
	return xpad_translucent;
}

static gboolean
config_dir_exists (void)
{
	gchar *dir = NULL;
	gboolean exists = FALSE;
	
	dir = g_build_filename (g_get_user_config_dir (), PACKAGE, NULL);
	exists = g_file_test (dir, G_FILE_TEST_EXISTS);
	g_free (dir);
	
	if (!exists)
	{
		/* For backwards-compatibility, we see if the old location for
		   configuration files exists.  It will be moved in make_config_dir */
		dir = g_build_filename (g_get_home_dir (), "." PACKAGE, NULL);
		exists = g_file_test (dir, G_FILE_TEST_EXISTS);
		g_free (dir);
	}
	
	return exists;
}

static void
make_path (const gchar *path)
{
	GSList *dirs = NULL, *i;
	gchar *dirname;
	
	dirname = g_strdup (path);
	while (!g_file_test (dirname, G_FILE_TEST_EXISTS))
	{
		dirs = g_slist_prepend (dirs, dirname);
		dirname = g_path_get_dirname (dirname);
	}
	g_free (dirname);
	
	for (i = dirs; i; i = i->next)
	{
		g_mkdir ((gchar *) i->data, 0700);
		g_free (i->data);
	}
	g_slist_free (dirs);
}

/**
 * Creates the directory if it does not exist.
 * Returns newly allocated dir name, NULL if an error occurred.
 */
static gchar *
make_config_dir (void)
{
	gchar *dir = NULL;
	
	make_path (g_get_user_config_dir ());
	
	dir = g_build_filename (g_get_user_config_dir (), PACKAGE, NULL);
	
	if (!g_file_test (dir, G_FILE_TEST_EXISTS))
	{
		gchar *olddir;
		
		/* For backwards-compatibility, we see if the old location for
		   configuration files exists.  If so, we move it. */
		olddir = g_build_filename (g_get_home_dir (), "." PACKAGE, NULL);
		
		if (g_file_test (olddir, G_FILE_TEST_EXISTS))
			g_rename (olddir, dir);
		else
			g_mkdir (dir, 0700); /* give user all rights */
		
		g_free (olddir);
	}
	
	return dir;
}


/**
 * Creates an alert with 'stock' used to create an icon and parent text of 'parent',
 * secondary text of 'secondary'.  No buttons are added.
 */
GtkWidget *
xpad_app_alert_new (GtkWindow *parent, const gchar *stock,
                    const gchar *primary, const gchar *secondary)
{
	GtkWidget *dialog, *hbox, *image, *label;
	gchar *buf;
	
	dialog = gtk_dialog_new_with_buttons (
		"",
		parent,
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
		NULL);
	
	hbox = gtk_hbox_new (FALSE, 12);
	image = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_DIALOG);
	label = gtk_label_new (NULL);
	
	if (secondary)
		buf = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s\n</span>\n%s", primary, secondary);
	else
		buf = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>", primary);
	
	gtk_label_set_markup (GTK_LABEL (label), buf);
	g_free (buf);
	
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
	gtk_container_add (GTK_CONTAINER (hbox), image);
	gtk_container_add (GTK_CONTAINER (hbox), label);
	
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	
	gtk_widget_show_all (hbox);
	
	return dialog;
}


static void
register_stock_icons (void)
{
	GtkIconTheme *theme;
	
	theme = gtk_icon_theme_get_default ();
	gtk_icon_theme_prepend_search_path (theme, THEME_DIR);
}


static gboolean
xpad_app_quit_if_no_pads (XpadPadGroup *group)
{
	if (!xpad_tray_is_open ())
	{
		gint num_pads = xpad_pad_group_num_visible_pads (group);
		if (num_pads == 0)
		{
			if (gtk_main_level () > 0)
				gtk_main_quit ();
			else
				exit (0);
		}
	}
	
	return FALSE;
}


static gboolean
xpad_app_first_idle_check (XpadPadGroup *group)
{
	/* We do this check at the first idle rather than immediately during
	   start because we want to give the tray time to become embedded. */
	if (!xpad_tray_is_open () &&
	    xpad_pad_group_num_visible_pads (group) == 0)
	{
		if (pads_loaded_on_start > 0)
			/* So we loaded xpad, there's no tray, and there's only hidden
			   pads...  Probably previously had tray open but we failed
			   this time.  Show all pads as a last resort.  This shouldn't
			   happen in normal operation. */
			xpad_pad_group_show_all (group);
		else
		{
			if (gtk_main_level () > 0)
				gtk_main_quit ();
			else
				exit (0);
		}
	}
	
	return FALSE;
}


static void
xpad_app_pad_added (XpadPadGroup *group, XpadPad *pad)
{
	g_signal_connect_swapped (pad, "closed", G_CALLBACK (xpad_app_quit_if_no_pads), group);
	g_signal_connect_swapped (pad, "destroy", G_CALLBACK (xpad_app_quit_if_no_pads), group);
}


/* Scans config directory for pad files and loads them. */
static gint
xpad_app_load_pads (void)
{
	gint opened = 0;
	GDir *dir;
	G_CONST_RETURN gchar *name;
	
	g_signal_connect (pad_group, "pad-added", G_CALLBACK (xpad_app_pad_added), NULL);
	
	dir = g_dir_open (xpad_app_get_config_dir (), 0, NULL);
	
	if (!dir)
	{
		gchar *errtext;
		
		errtext = g_strdup_printf (_("Could not open directory %s."), xpad_app_get_config_dir ());
		
		xpad_app_error (NULL, errtext,
			_("This directory is needed to store preference and pad information.  Xpad will close now."));
		g_free (errtext);
		
		exit (1);
	}
	
	while ((name = g_dir_read_name (dir)))
	{
		/* if it's an info file, but not a backup info file... */
		if (!strncmp (name, "info-", 5) &&
		    name[strlen (name) - 1] != '~')
		{
			gboolean show = TRUE;
			GtkWidget *pad = xpad_pad_new_with_info (pad_group, name, &show);
			if ((show || option_show) && !option_hide)
				gtk_widget_show (pad);
		  else if (show) /* pad thought it would show, we should save that it didn't */
		    xpad_pad_save_info (XPAD_PAD (pad));
			
			opened ++;
		}
	}
	
	g_dir_close (dir);
	
	return opened;
}












/*
converts main program arguments into one long string.
puts allocated string in dest, and returns size
*/
static gint
args_to_string (int argc, char **argv, char **dest)
{
	gint i;
	gint size = 0;
	gchar *p;
	
	for (i = 0; i < argc; i++)
		size += strlen (argv[i]) + 1;
	
	*dest = g_malloc (size);
	
	p = *dest;
	
	for (i = 0; i < argc; i++)
	{
		strcpy (p, argv[i]);
		p += strlen (argv[i]);
		p[0] = ' ';
		p += 1;
	}
	
	p --;
	p[0] = '\0';
	
	return size;
}


/*
returns number of strings in newly allocated argv
*/
static gint
string_to_args (const char *string, char ***argv)
{
	gint num, i;
	const gchar *tmp;
	char **list;
	
	/* first, find out how many arguments we have */
	num = 1;
	for (tmp = strchr (string, ' '); tmp; tmp = strchr (tmp+1, ' '))
	  num++;
	
	list = (char **) g_malloc (sizeof (char *) * (num + 1));
	
	for (i = 0; i < num; i++)
	{
		size_t len;
		
		/* string points to beginning of current arg */
		tmp = strchr (string, ' '); /* NULL or end of this arg */
		
		if (tmp) len = tmp - string;
		else   len = strlen (string);
		
		list[i] = g_malloc (len + 1);
		strncpy (list[i], string, len);
		list[i][len] = '\0';
		
		/* make string point to beginning of next arg */
		string = tmp + 1;
	}
	
	list[i] = NULL;	/* null terminate list */
	
	*argv = list;
	
	return num;
}

#include <errno.h>
/* This reads a line from the proc file.  This line will contain arguments to process. */
static void
xpad_app_read_from_proc_file (void)
{
	gint client_fd, size;
	gint argc;
	gchar **argv;
	gchar *args;
	struct sockaddr_un client;
	socklen_t client_len;
	size_t bytes;
	
	/* accept waiting connection */
	client_len = sizeof (client);
	client_fd = accept (server_fd, (struct sockaddr *) &client, &client_len);
	if (client_fd == -1)
		return;
	
	/* get size of args */
	bytes = read (client_fd, &size, sizeof (size));
	if (bytes != sizeof(size))
		goto close_client_fd;
	
	/* alloc memory */
	args = (gchar *) g_malloc (size);
	if (!args)
		goto close_client_fd;
	
	/* read args */
	bytes = read (client_fd, args, size);
	if (bytes < size)
		goto close_client_fd;
	
	argc = string_to_args (args, &argv);
	
	g_free (args);
	
	/* here we redirect singleton->priv->output to the socket */
	output = fdopen (client_fd, "w");
	
	if (!process_remote_args (&argc, &argv, TRUE))
	{
		/* if there were no non-local arguments, insert --new as argument */
		gint c = 2;
		gchar **v = g_malloc (sizeof (gchar *) * c);
		v[0] = PACKAGE;
		v[1] = "--new";
		
		process_remote_args (&c, &v, TRUE);
		
		g_free (v);
	}
	
	/* restore standard singleton->priv->output */
	fclose (output);
	output = stdout;
	
	g_strfreev (argv);
	return;
	
close_client_fd:
	close (client_fd);
}


static gboolean
can_read_from_server_fd (GIOChannel *source, GIOCondition condition, gpointer data)
{
	xpad_app_read_from_proc_file ();
	
	return TRUE;
}

static gboolean
xpad_app_open_proc_file (void)
{
	GIOChannel *channel;
	struct sockaddr_un master;
	
	g_unlink (server_filename);
	
	/* create the socket */
	server_fd = socket (PF_LOCAL, SOCK_STREAM, 0);
	bzero (&master, sizeof (master)); 
	master.sun_family = AF_LOCAL;
	strcpy (master.sun_path, server_filename);
	
	if (bind (server_fd, (struct sockaddr *) &master, SUN_LEN (&master)))
		return FALSE;
	
	/* listen for connections */
	if (listen (server_fd, 5))
		return FALSE;
	
	/* set up input loop, waiting for read */
	channel = g_io_channel_unix_new (server_fd);
	g_io_add_watch (channel, G_IO_IN, can_read_from_server_fd, NULL);
	g_io_channel_unref (channel);
	
	return TRUE;
}


static gboolean
xpad_app_pass_args (void)
{
	int client_fd;
	struct sockaddr_un master;
	fd_set fdset;
	gchar buf [129];
	gchar *args = NULL;
	gint size;
	gint bytesRead;
	gboolean connected = FALSE;
	
	/* create master socket */
	client_fd = socket (PF_LOCAL, SOCK_STREAM, 0);
	master.sun_family = AF_LOCAL;
	strcpy (master.sun_path, server_filename);
	
	/* connect to master socket */
	if (connect (client_fd, (struct sockaddr *) &master, SUN_LEN (&master)))
		goto done;
	connected = TRUE;
	
	size = args_to_string (xpad_argc, xpad_argv, &args) + 1;
	
	/* first, write length of string */
	write (client_fd, &size, sizeof (size));
	
	/* now, write string */
	write (client_fd, args, size);
	
	do
	{
		/* wait for response */
		FD_ZERO (&fdset);
		FD_SET (client_fd, &fdset);	
		/* block until we are answered, or an error occurs */
		select (client_fd + 1, &fdset, NULL, &fdset, NULL);
		
		do
		{
			bytesRead = read (client_fd, buf, 128);
			
			if (bytesRead < 0)
			{
			  goto done;
			}

			buf[bytesRead] = '\0';
			printf ("%s", buf);
		}
		while (bytesRead > 0);
	}
	while (bytesRead > 0);
	
done:
	close (client_fd);
	
	g_free (args);
	
	return connected;
}






















/**
 * Here are the functions called when arguments are passed to us.
 */

static GOptionEntry local_options[] =
{
	{"version", 'v', 0, G_OPTION_ARG_NONE, &option_version, N_("Show version number and quit"), NULL},
	{"no-new", 'N', 0, G_OPTION_ARG_NONE, &option_nonew, N_("Don't create a new pad on startup if no previous pads exist"), NULL},
	{NULL}
};

static GOptionEntry remote_options[] =
{
	{"new", 'n', 0, G_OPTION_ARG_NONE, &option_new, N_("Create a new pad on startup even if pads already exist"), NULL},
	{"hide", 'h', 0, G_OPTION_ARG_NONE, &option_hide, N_("Hide all pads"), NULL},
	{"show", 's', 0, G_OPTION_ARG_NONE, &option_show, N_("Show all pads"), NULL},
	{"toggle", 't', 0, G_OPTION_ARG_NONE, &option_toggle, N_("Toggle between show and hide all pads"), NULL},
	{"new-from-file", 'f', 0, G_OPTION_ARG_FILENAME_ARRAY, &option_files, N_("Create a new pad with the contents of a file"), N_("FILE")},
	{"quit", 'q', 0, G_OPTION_ARG_NONE, &option_quit, N_("Close all pads"), NULL},
	{"sm-client-id", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &option_smid, NULL, NULL},
	{NULL}
};

static gboolean
process_local_args (gint *argc, gchar **argv[])
{
	GError *error = NULL;
	GOptionContext *context;
	gint argc_copy;
	gchar **argv_copy;
	
	option_version = FALSE;
	option_nonew = FALSE;
	
	/* We make copies of argc and argv because we actually don't want the 
	   behavior of g_option_context_parse() that removes entries from the
	   array. */
	argc_copy = *argc;
	argv_copy = g_strdupv (*argv);
	
	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, local_options, GETTEXT_PACKAGE);
	/* We do remote here as well, because we want --help to pick them up.  It
	   can't hurt since they only set the global values that we reset later. */
	g_option_context_add_main_entries (context, remote_options, GETTEXT_PACKAGE);
	if (g_option_context_parse (context, &argc_copy, &argv_copy, &error))
	{
		if (option_version)
		{
			fprintf (output, _("Xpad %s"), PACKAGE_VERSION);
			fprintf (output, "\n");
			exit (0);
		}
	}
	else
	{
		fprintf (output, "%s\n", error->message);
		exit (1);
	}
	
	g_option_context_free (context);
	g_strfreev(argv_copy);
	
	return(option_version || option_nonew);
}

static gboolean
process_remote_args (gint *argc, gchar **argv[], gboolean have_gtk)
{
	GError *error = NULL;
	GOptionContext *context;
	
	option_new = FALSE;
	option_files = NULL;
	option_quit = FALSE;
	option_smid = NULL;
	option_hide = FALSE;
	option_show = FALSE;
	option_toggle = FALSE;
	
	context = g_option_context_new (NULL);
	g_option_context_set_ignore_unknown_options (context, TRUE);
	g_option_context_set_help_enabled (context, FALSE);
	g_option_context_add_main_entries (context, remote_options, GETTEXT_PACKAGE);
	if (g_option_context_parse (context, argc, argv, &error))
	{
		if (have_gtk && option_smid)
			xpad_session_manager_set_id (option_smid);
		
		if (have_gtk && option_new)
		{
			GtkWidget *pad = xpad_pad_new (pad_group);
			gtk_widget_show (pad);
		}

		if (have_gtk && option_show)
		  xpad_pad_group_show_all (pad_group);
		
		if (have_gtk && option_hide)
		  xpad_pad_group_close_all (pad_group);
		
		if (have_gtk && option_toggle)
		  xpad_pad_group_toggle_hide (pad_group);

		if (have_gtk && option_files)
		{
			int i;
			for (i = 0; option_files[i]; i++)
			{
				GtkWidget *pad = xpad_pad_new_from_file (pad_group, option_files[i]);
				if (pad)
					gtk_widget_show (pad);
			}
		}
		
		if (option_quit)
		{
			if (have_gtk && gtk_main_level () > 0)
				gtk_main_quit ();
			else
				exit (0);
		}
	}
	else
	{
		fprintf (output, "%s\n", error->message);
		/* Don't quit.  Bad options passed to the main xpad program by other
		   iterations shouldn't close the main one. */
	}
	
	g_option_context_free (context);
	
	return(option_new || option_quit || option_smid || option_files ||
	       option_hide || option_show || option_toggle);
}
