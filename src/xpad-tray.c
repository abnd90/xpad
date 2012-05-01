/*

Copyright (c) 2002 Jamis Buck
Copyright (c) 2003-2007 Michael Terry

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
#include <gtk/gtk.h>
#include "fio.h"
#include "xpad-app.h"
#include "xpad-pad.h"
#include "xpad-pad-group.h"
#include "xpad-preferences.h"
#include "xpad-tray.h"

static void xpad_tray_activate_cb (GtkStatusIcon *icon);
static void xpad_tray_popup_menu_cb (GtkStatusIcon *icon, guint button, guint time);

static GtkStatusIcon  *docklet = NULL;

void
xpad_tray_open (void)
{
	GtkIconTheme *theme;
	
	xpad_tray_close ();
	
	theme = gtk_icon_theme_get_default ();
	if (!gtk_icon_theme_has_icon (theme, PACKAGE))
		return;
	
	docklet = gtk_status_icon_new_from_icon_name (PACKAGE);
	
	if (docklet)
	{
		g_signal_connect (docklet, "activate", G_CALLBACK (xpad_tray_activate_cb), NULL);
		g_signal_connect (docklet, "popup-menu", G_CALLBACK (xpad_tray_popup_menu_cb), NULL);
	}
}

void
xpad_tray_close (void)
{
	if (docklet) {
		g_object_unref (docklet);
		docklet = NULL;
	}
}

gboolean
xpad_tray_is_open (void)
{
	if (docklet)
		return gtk_status_icon_is_embedded (docklet);
	else
		return FALSE;
}

static gint
menu_title_compare (GtkWindow *a, GtkWindow *b)
{
	gchar *title_a = g_utf8_casefold (gtk_window_get_title (a), -1);
	gchar *title_b = g_utf8_casefold (gtk_window_get_title (b), -1);
	
	gint rv = g_utf8_collate (title_a, title_b);
	
	g_free (title_a);
	g_free (title_b);
	
	return rv;
}

static void
menu_show_all (XpadPadGroup *group)
{
	GSList *pads = xpad_pad_group_get_pads (xpad_app_get_pad_group ());
	g_slist_foreach (pads, (GFunc) gtk_window_present, NULL);
	g_slist_free (pads);
}

static void
menu_spawn (XpadPadGroup *group)
{
	GtkWidget *pad = xpad_pad_new (group);
	gtk_widget_show (pad);
}

static void
xpad_tray_popup_menu_cb (GtkStatusIcon *icon, guint button, guint time)
{
	GtkWidget *menu, *item;
	GSList *pads, *l;
	gint n;
	
	menu = gtk_menu_new ();
	pads = xpad_pad_group_get_pads (xpad_app_get_pad_group ());
	
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
	g_signal_connect_swapped (item, "activate", G_CALLBACK (menu_spawn), xpad_app_get_pad_group ());
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	
	item = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	
	item = gtk_menu_item_new_with_mnemonic (_("_Show All"));
	g_signal_connect_swapped (item, "activate", G_CALLBACK (menu_show_all), xpad_app_get_pad_group ());
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	if (!pads)
		gtk_widget_set_sensitive (item, FALSE);
	
	item = gtk_image_menu_item_new_with_mnemonic (_("_Close All"));
	g_signal_connect_swapped (item, "activate", G_CALLBACK (xpad_pad_group_close_all), xpad_app_get_pad_group ());
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	if (!pads)
		gtk_widget_set_sensitive (item, FALSE);
	
	item = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	
	/**
	 * Order pads according to title.
	 */
	pads = g_slist_sort (pads, (GCompareFunc) menu_title_compare);
	
	/**
	 * Populate list of windows.
	 */
	for (l = pads, n = 1; l; l = l->next, n++)
	{
		gchar *title;
		gchar *tmp_title;
		
		tmp_title = g_strdup (gtk_window_get_title (GTK_WINDOW (l->data)));
		str_replace_tokens (&tmp_title, '_', "__");
		if (n < 10)
			title = g_strdup_printf ("_%i. %s", n, tmp_title);
		else
			title = g_strdup_printf ("%i. %s", n, tmp_title);
		g_free (tmp_title);
		
		item = gtk_menu_item_new_with_mnemonic (title);
		g_signal_connect_swapped (item, "activate", G_CALLBACK (gtk_window_present), l->data);
		gtk_container_add (GTK_CONTAINER (menu), item);
		gtk_widget_show (item);
		
		g_free (title);
	}
	g_slist_free (pads);
	
	if (pads)
	{
		item = gtk_separator_menu_item_new ();
		gtk_container_add (GTK_CONTAINER (menu), item);
		gtk_widget_show (item);
	}
	
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES, NULL);
	g_signal_connect (item, "activate", G_CALLBACK (xpad_preferences_open), NULL);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
	g_signal_connect (item, "activate", G_CALLBACK (gtk_main_quit), NULL);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_show (item);
	
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, gtk_status_icon_position_menu, icon, button, time);
}

static void
xpad_tray_activate_cb (GtkStatusIcon *icon)
{
	GSList *pads = xpad_pad_group_get_pads (xpad_app_get_pad_group ());
	g_slist_foreach (pads, (GFunc) gtk_window_present, NULL);
	g_slist_free (pads);
}

