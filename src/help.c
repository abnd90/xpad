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

#include "../config.h"
#include <glib/gi18n.h>
#include <string.h>
#include <gtk/gtk.h>
#include "help.h"

GtkWidget *help_window = NULL;

static void help_close (void)
{
	help_window = NULL;
}


static void show_help_at_page (gint page);

static GtkWidget *create_help (gint page)
{
	GtkWidget *dialog, *helptext, *button;
	gchar *helptextbuf;
	
	/* Create the widgets */
	
	dialog = gtk_dialog_new ();
	helptext = gtk_label_new ("");
	
	/* we use g_strdup_printf because C89 has size limits on static strings */
	helptextbuf = g_strdup_printf ("%s\n\n%s\n\n%s\n\n%s\n\n%s\n\n%s",
_("Each xpad session consists of one or more open pads.  "
"These pads are basically sticky notes on your desktop in which "
"you can write memos."),
_("<b>To move a pad</b>, left drag on the toolbar, right drag "
"on the resizer in the bottom right, or hold down CTRL "
"while left dragging anywhere on the pad."),
_("<b>To resize a pad</b>, left drag on the resizer or hold down "
"CTRL while right dragging anywhere on the pad."),
_("<b>To change color settings</b>, right click on a pad "
"and choose Edit->Preferences."),
_("Most actions are available throught the popup menu "
"that appears when you right click on a pad.  Try it out and "
"enjoy."),
_("Please send comments or bug reports to "
"xpad-devel@lists.sourceforge.net"));
	
	gtk_label_set_markup (GTK_LABEL (helptext), helptextbuf);
	
	g_free (helptextbuf);
	
	gtk_misc_set_padding (GTK_MISC (helptext), 12, 12);
	gtk_misc_set_alignment (GTK_MISC (helptext), 0, 0);
	gtk_label_set_line_wrap (GTK_LABEL (helptext), TRUE);
	
	gtk_window_set_title (GTK_WINDOW (dialog), _("Help"));
	
	/* Add the label, and show everything we've added to the dialog. */
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), helptext);
	button = gtk_dialog_add_button (GTK_DIALOG(dialog), "gtk-close", 1);
	
	gtk_window_set_position (GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	
	g_signal_connect (GTK_OBJECT (dialog), "destroy", 
		G_CALLBACK (help_close), NULL);
	g_signal_connect_swapped (GTK_OBJECT (button), "clicked", 
		G_CALLBACK (gtk_widget_destroy), dialog);
	
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_widget_show_all (dialog);
	
	return dialog;
}

void show_help (void)
{
	show_help_at_page (0);
}

static void show_help_at_page (gint page)
{
	if (help_window == NULL)
		help_window = create_help (page);
	else
		gtk_window_present (GTK_WINDOW (help_window));
}
