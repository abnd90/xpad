/*

Copyright (c) 2001-2007 Michael Terry
Copyright (c) 2009 Paul Ivanov
Copyright (c) 2011 Sergei Riaguzov
Copyright (c) 2011 Dennis Hilmar
Copytight (c) 2011 OBATA Akio

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
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "fio.h"
#include "help.h"
#include "xpad-app.h"
#include "xpad-pad.h"
#include "xpad-pad-properties.h"
#include "xpad-preferences.h"
#include "xpad-settings.h"
#include "xpad-text-buffer.h"
#include "xpad-text-view.h"
#include "xpad-toolbar.h"
#include "xpad-tray.h"

G_DEFINE_TYPE(XpadPad, xpad_pad, GTK_TYPE_WINDOW)
#define XPAD_PAD_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), XPAD_TYPE_PAD, XpadPadPrivate))

struct XpadPadPrivate 
{
	/* saved values */
	gint x, y, width, height;
	gboolean location_valid;
	gchar *infoname;
	gchar *contentname;
	gboolean sticky;
	
	/* selected child widgets */
	GtkWidget *textview;
	GtkWidget *scrollbar;
	
	/* toolbar stuff */
	GtkWidget *toolbar;
	guint toolbar_timeout;
	gint toolbar_height;
	gboolean toolbar_expanded;
	gboolean toolbar_pad_resized;
	
	/* properties window */
	GtkWidget *properties;
	
	/* menus */
	GtkWidget *menu;
	GtkWidget *highlight_menu;
	
	XpadPadGroup *group;
};

enum
{
	CLOSED,
	LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_GROUP,
  LAST_PROP
};

static void load_info (XpadPad *pad, gboolean *show);
static GtkWidget *menu_get_popup_highlight (XpadPad *pad, GtkAccelGroup *accel_group);
static GtkWidget *menu_get_popup_no_highlight (XpadPad *pad, GtkAccelGroup *accel_group);
static void xpad_pad_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_pad_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xpad_pad_dispose (GObject *object);
static void xpad_pad_finalize (GObject *object);
static void xpad_pad_show (XpadPad *pad);
static gboolean xpad_pad_configure_event (XpadPad *pad, GdkEventConfigure *event);
static gboolean xpad_pad_toolbar_size_allocate (XpadPad *pad, GtkAllocation *event);
static gboolean xpad_pad_window_state_event (XpadPad *pad, GdkEventWindowState *event);
static gboolean xpad_pad_delete_event (XpadPad *pad, GdkEvent *event);
static gboolean xpad_pad_popup_menu (XpadPad *pad);
static void xpad_pad_popup_deactivate (GtkWidget *menu, XpadPad *pad);
static gboolean xpad_pad_button_press_event (XpadPad *pad, GdkEventButton *event);
static gboolean xpad_pad_text_view_button_press_event (GtkWidget *text_view, GdkEventButton *event, XpadPad *pad);
static void xpad_pad_text_changed (XpadPad *pad, GtkTextBuffer *buffer);
static void xpad_pad_notify_has_scrollbar (XpadPad *pad);
static void xpad_pad_notify_has_decorations (XpadPad *pad);
static void xpad_pad_notify_has_toolbar (XpadPad *pad);
static void xpad_pad_notify_autohide_toolbar (XpadPad *pad);
static void xpad_pad_hide_toolbar (XpadPad *pad);
static void xpad_pad_show_toolbar (XpadPad *pad);
static void xpad_pad_popup (XpadPad *pad, GdkEventButton *event);
static void xpad_pad_spawn (XpadPad *pad);
static void xpad_pad_clear (XpadPad *pad);
static void xpad_pad_undo (XpadPad *pad);
static void xpad_pad_redo (XpadPad *pad);
static void xpad_pad_cut (XpadPad *pad);
static void xpad_pad_copy (XpadPad *pad);
static void xpad_pad_paste (XpadPad *pad);
static void xpad_pad_delete (XpadPad *pad);
static void xpad_pad_open_properties (XpadPad *pad);
static void xpad_pad_open_preferences (XpadPad *pad);
static void xpad_pad_quit (XpadPad *pad);
static void xpad_pad_close_all (XpadPad *pad);
static void xpad_pad_sync_title (XpadPad *pad);
static void xpad_pad_set_group (XpadPad *pad, XpadPadGroup *group);
static gboolean xpad_pad_leave_notify_event (GtkWidget *pad, GdkEventCrossing *event);
static gboolean xpad_pad_enter_notify_event (GtkWidget *pad, GdkEventCrossing *event);
static void xpad_pad_toolbar_popup (GtkWidget *toolbar, GtkMenu *menu, XpadPad *pad);
static void xpad_pad_toolbar_popdown (GtkWidget *toolbar, GtkMenu *menu, XpadPad *pad);
static XpadPadGroup *xpad_pad_get_group (XpadPad *pad);

static guint signals[LAST_SIGNAL] = { 0 };

GtkWidget *
xpad_pad_new (XpadPadGroup *group)
{
	return GTK_WIDGET (g_object_new (XPAD_TYPE_PAD, "group", group, NULL));
}

GtkWidget *
xpad_pad_new_with_info (XpadPadGroup *group, const gchar *info_filename, gboolean *show)
{
	GtkWidget *pad = GTK_WIDGET (g_object_new (XPAD_TYPE_PAD, "group", group, NULL));
	
	XPAD_PAD (pad)->priv->infoname = g_strdup (info_filename);
	load_info (XPAD_PAD (pad), show);
	xpad_pad_load_content (XPAD_PAD (pad));
	gtk_window_set_role (GTK_WINDOW (pad), XPAD_PAD (pad)->priv->infoname);
	
	return pad;
}

GtkWidget *
xpad_pad_new_from_file (XpadPadGroup *group, const gchar *filename)
{
	GtkWidget *pad = NULL;
	gchar *content;
	
	content = fio_get_file (filename);
	
	if (!content)
	{
		gchar *usertext = g_strdup_printf (_("Could not read file %s."), filename);
		xpad_app_error (NULL, usertext, NULL);
		g_free (usertext);
	}
	else
	{
		GtkTextBuffer *buffer;
		
		pad = GTK_WIDGET (g_object_new (XPAD_TYPE_PAD, "group", group, NULL));
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (XPAD_PAD (pad)->priv->textview));

		xpad_text_buffer_freeze_undo (XPAD_TEXT_BUFFER (buffer));
		g_signal_handlers_block_by_func (buffer, xpad_pad_text_changed, pad);
		
		xpad_text_buffer_set_text_with_tags (XPAD_TEXT_BUFFER (buffer), content ? content : "");
		g_free (content);
		
		g_signal_handlers_unblock_by_func (buffer, xpad_pad_text_changed, pad);
		xpad_text_buffer_thaw_undo (XPAD_TEXT_BUFFER (buffer));

		xpad_pad_text_changed(XPAD_PAD(pad), buffer);
	}
	
	return pad;
}

static void
xpad_pad_class_init (XpadPadClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->dispose = xpad_pad_dispose;
	gobject_class->finalize = xpad_pad_finalize;
	gobject_class->set_property = xpad_pad_set_property;
	gobject_class->get_property = xpad_pad_get_property;
	
	signals[CLOSED] =
		g_signal_new ("closed",
						  G_OBJECT_CLASS_TYPE (gobject_class),
						  G_SIGNAL_RUN_FIRST,
						  G_STRUCT_OFFSET (XpadPadClass, closed),
						  NULL, NULL,
						  g_cclosure_marshal_VOID__VOID,
						  G_TYPE_NONE,
						  0);
	
	/* Properties */
	
	g_object_class_install_property (gobject_class,
												PROP_GROUP,
												g_param_spec_pointer ("group",
																			 "Pad Group",
																			 "Pad group for this pad",
																			 G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	
	g_type_class_add_private (gobject_class, sizeof (XpadPadPrivate));
}

static void
xpad_pad_init (XpadPad *pad)
{
	GtkWidget *vbox;
	GtkAccelGroup *accel_group;
	GtkClipboard *clipboard;
	
	pad->priv = XPAD_PAD_GET_PRIVATE (pad);
	
	pad->priv->x = 0;
	pad->priv->y = 0;
	pad->priv->location_valid = FALSE;
	pad->priv->width = xpad_settings_get_width (xpad_settings ());
	pad->priv->height = xpad_settings_get_height (xpad_settings ());
	pad->priv->infoname = NULL;
	pad->priv->contentname = NULL;
	pad->priv->sticky = xpad_settings_get_sticky (xpad_settings ());
	pad->priv->textview = NULL;
	pad->priv->scrollbar = NULL;
	pad->priv->toolbar = NULL;
	pad->priv->toolbar_timeout = 0;
	pad->priv->toolbar_height = 0;
	pad->priv->toolbar_expanded = FALSE;
	pad->priv->toolbar_pad_resized = TRUE;
	pad->priv->properties = NULL;
	pad->priv->group = NULL;

	XpadTextView *text_view = g_object_new (XPAD_TYPE_TEXT_VIEW,
		"follow-font-style", TRUE,
		"follow-color-style", TRUE,
		NULL);
	
	xpad_text_view_set_pad (text_view, pad);

	pad->priv->textview = GTK_WIDGET (text_view);
	
	pad->priv->scrollbar = GTK_WIDGET (g_object_new (GTK_TYPE_SCROLLED_WINDOW,
		"hadjustment", NULL,
		"hscrollbar-policy", GTK_POLICY_NEVER,
		"shadow-type", GTK_SHADOW_NONE,
		"vadjustment", NULL,
		"vscrollbar-policy", GTK_POLICY_NEVER,
		"child", pad->priv->textview,
		NULL));
	
	pad->priv->toolbar = GTK_WIDGET ( xpad_toolbar_new (pad));
	
	accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (pad), accel_group);
	g_object_unref (G_OBJECT (accel_group));
	pad->priv->menu = menu_get_popup_no_highlight (pad, accel_group);
	pad->priv->highlight_menu = menu_get_popup_highlight (pad, accel_group);
	gtk_accel_group_connect (accel_group, GDK_Q, GDK_CONTROL_MASK, 0,
									 g_cclosure_new_swap (G_CALLBACK (xpad_pad_quit), pad, NULL));

	
	vbox = GTK_WIDGET (g_object_new (GTK_TYPE_VBOX,
		"homogeneous", FALSE,
		"spacing", 0,
		"child", pad->priv->scrollbar,
		"child", pad->priv->toolbar,
		NULL));
	gtk_container_child_set (GTK_CONTAINER (vbox), pad->priv->toolbar, "expand", FALSE, NULL);
	
	gtk_window_set_decorated (GTK_WINDOW(pad), xpad_settings_get_has_decorations (xpad_settings ()));
	gtk_window_set_default_size (GTK_WINDOW(pad), xpad_settings_get_width (xpad_settings ()), xpad_settings_get_height (xpad_settings ()));
	gtk_window_set_gravity (GTK_WINDOW(pad),  GDK_GRAVITY_STATIC); /* static gravity makes saving pad x,y work */
	gtk_window_set_skip_pager_hint (GTK_WINDOW(pad),xpad_settings_get_has_decorations (xpad_settings ()));
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW(pad), !xpad_settings_get_has_decorations (xpad_settings ()));
	gtk_window_set_type_hint (GTK_WINDOW(pad), GDK_WINDOW_TYPE_HINT_NORMAL);
	gtk_window_set_position (GTK_WINDOW(pad), GTK_WIN_POS_MOUSE);

	g_object_set (G_OBJECT (pad),
		"child", vbox,
		NULL);
	
	xpad_pad_notify_has_scrollbar (pad);
	xpad_pad_notify_has_selection (pad);
	xpad_pad_notify_clipboard_owner_changed (pad);
	xpad_pad_notify_undo_redo_changed (pad);

 	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	
	/* Set up signals */
	gtk_widget_add_events (GTK_WIDGET (pad), GDK_BUTTON_PRESS_MASK | GDK_PROPERTY_CHANGE_MASK);
	gtk_widget_add_events (pad->priv->toolbar, GDK_ALL_EVENTS_MASK);
	g_signal_connect (pad->priv->textview, "button-press-event", G_CALLBACK (xpad_pad_text_view_button_press_event), pad);
	g_signal_connect_swapped (pad->priv->textview, "popup-menu", G_CALLBACK (xpad_pad_popup_menu), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "size-allocate", G_CALLBACK (xpad_pad_toolbar_size_allocate), pad);
	g_signal_connect (pad, "button-press-event", G_CALLBACK (xpad_pad_button_press_event), NULL);
	g_signal_connect (pad, "configure-event", G_CALLBACK (xpad_pad_configure_event), NULL);
	g_signal_connect (pad, "delete-event", G_CALLBACK (xpad_pad_delete_event), NULL);
	g_signal_connect (pad, "popup-menu", G_CALLBACK (xpad_pad_popup_menu), NULL);
	g_signal_connect (pad, "show", G_CALLBACK (xpad_pad_show), NULL);
	g_signal_connect (pad, "window-state-event", G_CALLBACK (xpad_pad_window_state_event), NULL);
	g_signal_connect_swapped (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)), "changed", G_CALLBACK (xpad_pad_text_changed), pad);
	
	g_signal_connect (pad, "enter-notify-event", G_CALLBACK (xpad_pad_enter_notify_event), NULL);
	g_signal_connect (pad, "leave-notify-event", G_CALLBACK (xpad_pad_leave_notify_event), NULL);
	
	g_signal_connect_swapped (xpad_settings (), "notify::has-decorations", G_CALLBACK (xpad_pad_notify_has_decorations), pad);
	g_signal_connect_swapped (xpad_settings (), "notify::has-toolbar", G_CALLBACK (xpad_pad_notify_has_toolbar), pad);
	g_signal_connect_swapped (xpad_settings (), "notify::autohide-toolbar", G_CALLBACK (xpad_pad_notify_autohide_toolbar), pad);
	g_signal_connect_swapped (xpad_settings (), "notify::has-scrollbar", G_CALLBACK (xpad_pad_notify_has_scrollbar), pad);
	g_signal_connect_swapped (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)), "notify::has-selection", G_CALLBACK (xpad_pad_notify_has_selection), pad);
	g_signal_connect_swapped (clipboard, "owner-change", G_CALLBACK (xpad_pad_notify_clipboard_owner_changed), pad);
	
	g_signal_connect_swapped (pad->priv->toolbar, "activate-new", G_CALLBACK (xpad_pad_spawn), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-clear", G_CALLBACK (xpad_pad_clear), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-close", G_CALLBACK (xpad_pad_close), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-undo", G_CALLBACK (xpad_pad_undo), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-redo", G_CALLBACK (xpad_pad_redo), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-cut", G_CALLBACK (xpad_pad_cut), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-copy", G_CALLBACK (xpad_pad_copy), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-paste", G_CALLBACK (xpad_pad_paste), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-delete", G_CALLBACK (xpad_pad_delete), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-properties", G_CALLBACK (xpad_pad_open_properties), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-preferences", G_CALLBACK (xpad_pad_open_preferences), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-quit", G_CALLBACK (xpad_pad_close_all), pad);
	
	g_signal_connect (pad->priv->toolbar, "popup", G_CALLBACK (xpad_pad_toolbar_popup), pad);
	g_signal_connect (pad->priv->toolbar, "popdown", G_CALLBACK (xpad_pad_toolbar_popdown), pad);
	
	g_signal_connect (pad->priv->menu, "deactivate", G_CALLBACK (xpad_pad_popup_deactivate), pad);
	g_signal_connect (pad->priv->highlight_menu, "deactivate", G_CALLBACK (xpad_pad_popup_deactivate), pad);
	
	if (pad->priv->sticky)
		gtk_window_stick (GTK_WINDOW (pad));
	else
		gtk_window_unstick (GTK_WINDOW (pad));
	
	xpad_pad_sync_title (pad);
	
	gtk_widget_show_all (vbox);
	
	gtk_widget_hide (pad->priv->toolbar);
	xpad_pad_notify_has_toolbar (pad);
}

static void
xpad_pad_show (XpadPad *pad)
{
	/* Some wm's might not acknowledge our request for a specific
		location before we are shown.  What we do here is a little gimpy
		and not very respectful of wms' sovereignty, but it has the effect
		of making pads' locations very dependable.  We just move the pad
		again here after being shown.  This may create a visual effect if 
		the wm did ignore us, but is better than being in the wrong
		place, I guess. */
	if (pad->priv->location_valid)
		gtk_window_move (GTK_WINDOW (pad), pad->priv->x, pad->priv->y);
		
	if (pad->priv->sticky)
		gtk_window_stick (GTK_WINDOW (pad));
	else
		gtk_window_unstick (GTK_WINDOW (pad));
	
/* xpad_pad_sync_title (pad);*/
}

static void
xpad_pad_dispose (GObject *object)
{
	XpadPad *pad = XPAD_PAD (object);
	
	if (pad->priv->toolbar_timeout)
	{
		g_source_remove (pad->priv->toolbar_timeout);
		pad->priv->toolbar_timeout = 0;
	}
	
	if (pad->priv->properties)
		gtk_widget_destroy (pad->priv->properties);
	
	gtk_widget_destroy (pad->priv->menu);
	gtk_widget_destroy (pad->priv->highlight_menu);
	
	G_OBJECT_CLASS (xpad_pad_parent_class)->dispose (object);
}

static void
xpad_pad_finalize (GObject *object)
{
	XpadPad *pad = XPAD_PAD (object);
	
	g_free (pad->priv->infoname);
	g_free (pad->priv->contentname);
	
	g_signal_handlers_disconnect_matched (xpad_settings (), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, pad);
	
	G_OBJECT_CLASS (xpad_pad_parent_class)->finalize (object);
}

static void
xpad_pad_notify_has_scrollbar (XpadPad *pad)
{
	if (xpad_settings_get_has_scrollbar (xpad_settings ()))
	{
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pad->priv->scrollbar), 
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	}
	else
	{
		GtkAdjustment *v, *h;
		
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pad->priv->scrollbar), 
			GTK_POLICY_NEVER, GTK_POLICY_NEVER);
		
		/* now we need to adjust view so that user can see whole pad */
		h = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (pad->priv->scrollbar));
		v = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (pad->priv->scrollbar));
		
		gtk_adjustment_set_value (h, 0);
		gtk_adjustment_set_value (v, 0);
	}
}

static void
xpad_pad_notify_has_decorations (XpadPad *pad)
{
	gboolean decorations = xpad_settings_get_has_decorations (xpad_settings ());
	
	/**
	 *  There are two modes of operation:  a normal mode and a 'stealth' mode.
	 *  If decorations are disabled, we also don't show up in the taskbar or pager. 
	 */
	gtk_window_set_decorated (GTK_WINDOW (pad), decorations);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (pad), !decorations);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (pad), !decorations);
	
	/* reshow_with_initial_size() seems to set the window back to a never-shown state.
		This is good, as some WMs don't like us changing the above parameters mid-run,
		even if we do a hide/show cycle. */
	gtk_window_set_default_size (GTK_WINDOW (pad), pad->priv->width, pad->priv->height);
	gtk_window_reshow_with_initial_size (GTK_WINDOW (pad));
}

static gint
xpad_pad_text_and_toolbar_height (XpadPad *pad)
{
	GdkRectangle rec;
	gint textx, texty, x, y;
	GtkTextIter iter;
	
	gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(pad->priv->textview), &rec);
	gtk_text_buffer_get_end_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(pad->priv->textview)), &iter);
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(pad->priv->textview), &iter, &rec);
	gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(pad->priv->textview),
		GTK_TEXT_WINDOW_WIDGET, rec.x + rec.width, rec.y + rec.height,
		&textx, &texty);
	gtk_widget_translate_coordinates(pad->priv->textview, GTK_WIDGET(pad), textx, texty, &x, &y);

	return y + pad->priv->toolbar_height + gtk_container_get_border_width(GTK_CONTAINER(pad->priv->textview));
}

static void
xpad_pad_show_toolbar (XpadPad *pad)
{
	if (!GTK_WIDGET_VISIBLE (pad->priv->toolbar))
	{
		GtkRequisition req;
		
		if (GTK_WIDGET (pad)->window)
			gdk_window_freeze_updates (GTK_WIDGET (pad)->window);
		gtk_widget_show (pad->priv->toolbar);
		if (!pad->priv->toolbar_height)
		{
			gtk_widget_size_request (pad->priv->toolbar, &req);
			pad->priv->toolbar_height = req.height;
		}

		/* Do we have room for the toolbar without covering text? */
		if (xpad_pad_text_and_toolbar_height (pad) > pad->priv->height)
		{
			pad->priv->toolbar_expanded = TRUE;
			pad->priv->height += pad->priv->toolbar_height;
			gtk_window_resize (GTK_WINDOW (pad), pad->priv->width, pad->priv->height);
		}
		else
			pad->priv->toolbar_expanded = FALSE;
		
		pad->priv->toolbar_pad_resized = FALSE;
		
		if (GTK_WIDGET (pad)->window)
			gdk_window_thaw_updates (GTK_WIDGET (pad)->window);
	}
}

static void
xpad_pad_hide_toolbar (XpadPad *pad)
{
	if (GTK_WIDGET_VISIBLE (pad->priv->toolbar))
	{
		if (GTK_WIDGET (pad)->window)
			gdk_window_freeze_updates (GTK_WIDGET (pad)->window);
		gtk_widget_hide (pad->priv->toolbar);
		
		if (pad->priv->toolbar_expanded ||
			 (pad->priv->toolbar_pad_resized && xpad_pad_text_and_toolbar_height (pad) >= pad->priv->height))
		{
				pad->priv->height -= pad->priv->toolbar_height;
				gtk_window_resize (GTK_WINDOW (pad), pad->priv->width, pad->priv->height);
				pad->priv->toolbar_expanded = FALSE;
		}
		if (GTK_WIDGET (pad)->window)
			gdk_window_thaw_updates (GTK_WIDGET (pad)->window);
	}
}

static void
xpad_pad_notify_has_toolbar (XpadPad *pad)
{
	if (xpad_settings_get_has_toolbar (xpad_settings ()))
	{
		if (!xpad_settings_get_autohide_toolbar (xpad_settings ()))
			xpad_pad_show_toolbar (pad);
	}
	else
		xpad_pad_hide_toolbar (pad);
}

static gboolean
toolbar_timeout (XpadPad *pad)
{
	if (pad->priv->toolbar_timeout &&
		 xpad_settings_get_autohide_toolbar (xpad_settings ()) &&
		 xpad_settings_get_has_toolbar (xpad_settings ()))
		xpad_pad_hide_toolbar (pad);
	
	pad->priv->toolbar_timeout = 0;
	
	return FALSE;
}

static void
xpad_pad_notify_autohide_toolbar (XpadPad *pad)
{
	if (xpad_settings_get_autohide_toolbar (xpad_settings ()))
	{
		/* Likely not to be in pad when turning setting on */
		if (!pad->priv->toolbar_timeout)
			pad->priv->toolbar_timeout = g_timeout_add (1000, (GSourceFunc) toolbar_timeout, pad);
	}
	else
	{
		if (xpad_settings_get_has_toolbar (xpad_settings ()))
			xpad_pad_show_toolbar(pad);
	}
}

void
xpad_pad_notify_has_selection (XpadPad *pad)
{
	g_return_if_fail (pad);

	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview));
	gboolean has_selection = gtk_text_buffer_get_has_selection (buffer);

	XpadToolbar *toolbar = NULL;
	toolbar = XPAD_TOOLBAR (pad->priv->toolbar);
	g_return_if_fail (toolbar);

	xpad_toolbar_enable_cut_button (toolbar, has_selection);
	xpad_toolbar_enable_copy_button (toolbar, has_selection);
}

void
xpad_pad_notify_clipboard_owner_changed (XpadPad *pad)
{
	g_return_if_fail (pad);

	XpadToolbar *toolbar = NULL;
	toolbar = XPAD_TOOLBAR (pad->priv->toolbar);
	g_return_if_fail (toolbar);

	GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	xpad_toolbar_enable_paste_button (toolbar, gtk_clipboard_wait_is_text_available (clipboard));
}

void
xpad_pad_notify_undo_redo_changed (XpadPad *pad)
{
	g_return_if_fail (pad);

	XpadTextBuffer *buffer = NULL;
	buffer = XPAD_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));
	g_return_if_fail (buffer);

	XpadToolbar *toolbar = NULL;
	toolbar = XPAD_TOOLBAR (pad->priv->toolbar);
	g_return_if_fail (toolbar);

	xpad_toolbar_enable_undo_button (toolbar, xpad_text_buffer_undo_available (buffer));
	xpad_toolbar_enable_redo_button (toolbar, xpad_text_buffer_redo_available (buffer));
}

static gboolean
xpad_pad_enter_notify_event (GtkWidget *pad, GdkEventCrossing *event)
{
	if (xpad_settings_get_has_toolbar (xpad_settings ()) &&
		 xpad_settings_get_autohide_toolbar (xpad_settings ()) &&
		 event->detail != GDK_NOTIFY_INFERIOR &&
		 event->mode == GDK_CROSSING_NORMAL)
	{
		XPAD_PAD (pad)->priv->toolbar_timeout = 0;
		xpad_pad_show_toolbar (XPAD_PAD (pad));
	}
	
	return FALSE;
}

static gboolean
xpad_pad_leave_notify_event (GtkWidget *pad, GdkEventCrossing *event)
{
	if (xpad_settings_get_has_toolbar (xpad_settings ()) &&
		 xpad_settings_get_autohide_toolbar (xpad_settings ()) &&
		 event->detail != GDK_NOTIFY_INFERIOR &&
		 event->mode == GDK_CROSSING_NORMAL)
	{
		if (!XPAD_PAD (pad)->priv->toolbar_timeout)
			XPAD_PAD (pad)->priv->toolbar_timeout = g_timeout_add (1000, (GSourceFunc) toolbar_timeout, pad);
	}
	
	return FALSE;
}

static void
xpad_pad_spawn (XpadPad *pad)
{
	GtkWidget *newpad = xpad_pad_new (pad->priv->group);
	gtk_widget_show (newpad);
}

static void
xpad_pad_clear (XpadPad *pad)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview));
	gtk_text_buffer_set_text (buffer, "", -1);
}

void
xpad_pad_close (XpadPad *pad)
{
	gtk_widget_hide (GTK_WIDGET (pad));
	
	/* If no tray and this is the last pad, we don't want to record this
		pad as closed, we want to start with just this pad next open.  So
		quit before we record. */
	if (!xpad_tray_is_open () &&
		 xpad_pad_group_num_visible_pads (pad->priv->group) == 0)
	{
		xpad_pad_quit (pad);
		return;
	}
	
	if (pad->priv->properties)
		gtk_widget_destroy (pad->priv->properties);
	
	xpad_pad_save_info (pad);
	
	g_signal_emit (pad, signals[CLOSED], 0);
}

void
xpad_pad_toggle(XpadPad *pad)
{
	 if (GTK_WIDGET_VISIBLE (pad)) 
		  xpad_pad_close (pad);
	 else
		  gtk_widget_show (GTK_WIDGET (pad));
}

static gboolean
should_confirm_delete (XpadPad *pad)
{
	GtkTextBuffer *buffer;
	GtkTextIter s, e;
	gchar *content;
	gboolean confirm;
	
	if (!xpad_settings_get_confirm_destroy (xpad_settings ()))
		return FALSE;
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview));
	gtk_text_buffer_get_bounds (buffer, &s, &e);
	content = gtk_text_buffer_get_text (buffer, &s, &e, FALSE);
	
	confirm = strcmp (g_strstrip (content), "") != 0;
	
	g_free (content);
	
	return confirm;
}

static void
xpad_pad_delete (XpadPad *pad)
{
	if (should_confirm_delete (pad))
	{
		GtkWidget *dialog;
		gint response;
		
		dialog = xpad_app_alert_new (GTK_WINDOW (pad), GTK_STOCK_DIALOG_WARNING,
			_("Delete this pad?"),
			_("All text of this pad will be irrevocably lost."));
		
		if (!dialog)
			return;
		
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, 1, GTK_STOCK_DELETE, 2, NULL);
		
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		
		gtk_widget_destroy (dialog);
		
		if (response != 2)
			return;
	}
	
	if (pad->priv->infoname)
		fio_remove_file (pad->priv->infoname);
	if (pad->priv->contentname)
		fio_remove_file (pad->priv->contentname);
	
	gtk_widget_destroy (GTK_WIDGET (pad));
}

static void
pad_properties_sync_title (XpadPad *pad)
{
	gchar *title;
	
	if (!pad->priv->properties)
		return;
	
	title = g_strdup_printf (_("'%s' Properties"), gtk_window_get_title (GTK_WINDOW (pad)));
	gtk_window_set_title (GTK_WINDOW (pad->priv->properties), title);
	g_free (title);
}

static void
pad_properties_destroyed (XpadPad *pad)
{
	if (!pad->priv->properties)
		return;
	
	g_signal_handlers_disconnect_by_func (pad, (gpointer) pad_properties_sync_title, NULL);
	pad->priv->properties = NULL;
}

static void
prop_notify_follow_font (XpadPad *pad)
{
	XpadPadProperties *prop = XPAD_PAD_PROPERTIES (pad->priv->properties);
	
	xpad_text_view_set_follow_font_style (XPAD_TEXT_VIEW (pad->priv->textview), xpad_pad_properties_get_follow_font_style (prop));
	
	if (!xpad_pad_properties_get_follow_font_style (prop))
	{
		const gchar *font = xpad_pad_properties_get_fontname (prop);
		PangoFontDescription *fontdesc;
		
		fontdesc = font ? pango_font_description_from_string (font) : NULL;
		gtk_widget_modify_font (pad->priv->textview, fontdesc);
		if (fontdesc)
			pango_font_description_free (fontdesc);
	}
	
	xpad_pad_save_info (pad);
}

static void
prop_notify_follow_color (XpadPad *pad)
{
	XpadPadProperties *prop = XPAD_PAD_PROPERTIES (pad->priv->properties);
	
	xpad_text_view_set_follow_color_style (XPAD_TEXT_VIEW (pad->priv->textview), xpad_pad_properties_get_follow_color_style (prop));
	
	if (!xpad_pad_properties_get_follow_color_style (prop))
	{
		gtk_widget_modify_base (pad->priv->textview, GTK_STATE_NORMAL, xpad_pad_properties_get_back_color (prop));
		gtk_widget_modify_text (pad->priv->textview, GTK_STATE_NORMAL, xpad_pad_properties_get_text_color (prop));
	}
	
	xpad_pad_save_info (pad);
}

static void
prop_notify_text (XpadPad *pad)
{
	XpadPadProperties *prop = XPAD_PAD_PROPERTIES (pad->priv->properties);
	
	gtk_widget_modify_text (pad->priv->textview, GTK_STATE_NORMAL, xpad_pad_properties_get_text_color (prop));
	
	xpad_pad_save_info (pad);
}

static void
prop_notify_back (XpadPad *pad)
{
	XpadPadProperties *prop = XPAD_PAD_PROPERTIES (pad->priv->properties);
	
	gtk_widget_modify_base (pad->priv->textview, GTK_STATE_NORMAL, xpad_pad_properties_get_back_color (prop));
	
	xpad_pad_save_info (pad);
}

static void
prop_notify_font (XpadPad *pad)
{
	XpadPadProperties *prop = XPAD_PAD_PROPERTIES (pad->priv->properties);
	
	const gchar *font = xpad_pad_properties_get_fontname (prop);
	PangoFontDescription *fontdesc;
	
	fontdesc = font ? pango_font_description_from_string (font) : NULL;
	gtk_widget_modify_font (pad->priv->textview, fontdesc);
	if (fontdesc)
		pango_font_description_free (fontdesc);
	
	xpad_pad_save_info (pad);
}

static void
xpad_pad_open_properties (XpadPad *pad)
{
	GtkStyle *style;
	gchar *fontname;
	
	if (pad->priv->properties)
	{
		gtk_window_present (GTK_WINDOW (pad->priv->properties));
		return;
	}
	
	pad->priv->properties = xpad_pad_properties_new ();
	
	gtk_window_set_transient_for (GTK_WINDOW (pad->priv->properties), GTK_WINDOW (pad));
	gtk_window_set_resizable (GTK_WINDOW (pad->priv->properties), FALSE);
	
	g_signal_connect_swapped (pad->priv->properties, "destroy", G_CALLBACK (pad_properties_destroyed), pad);
	g_signal_connect (pad, "notify::title", G_CALLBACK (pad_properties_sync_title), NULL);
	
	style = gtk_widget_get_style (pad->priv->textview);
	fontname = style->font_desc ? pango_font_description_to_string (style->font_desc) : NULL;
	g_object_set (G_OBJECT (pad->priv->properties),
		"follow-font-style", xpad_text_view_get_follow_font_style (XPAD_TEXT_VIEW (pad->priv->textview)),
		"follow-color-style", xpad_text_view_get_follow_color_style (XPAD_TEXT_VIEW (pad->priv->textview)),
		"back-color", &style->base[GTK_STATE_NORMAL],
		"text-color", &style->text[GTK_STATE_NORMAL],
		"fontname", fontname,
		NULL);
	g_free (fontname);
	
	g_signal_connect_swapped (pad->priv->properties, "notify::follow-font-style", G_CALLBACK (prop_notify_follow_font), pad);
	g_signal_connect_swapped (pad->priv->properties, "notify::follow-color-style", G_CALLBACK (prop_notify_follow_color), pad);
	g_signal_connect_swapped (pad->priv->properties, "notify::text-color", G_CALLBACK (prop_notify_text), pad);
	g_signal_connect_swapped (pad->priv->properties, "notify::back-color", G_CALLBACK (prop_notify_back), pad);
	g_signal_connect_swapped (pad->priv->properties, "notify::fontname", G_CALLBACK (prop_notify_font), pad);
	
	pad_properties_sync_title (pad);
	
	gtk_widget_show (pad->priv->properties);
}

static void
xpad_pad_open_preferences (XpadPad *pad)
{
	xpad_preferences_open ();
}

static void
xpad_pad_quit (XpadPad *pad)
{
	gtk_main_quit ();
}

static void
xpad_pad_text_changed (XpadPad *pad, GtkTextBuffer *buffer)
{
	/* set title */
	xpad_pad_sync_title (pad);
	
	/* record change */
	xpad_pad_save_content (pad);
}

static gboolean
xpad_pad_toolbar_size_allocate (XpadPad *pad, GtkAllocation *event)
{
	pad->priv->toolbar_height = event->height;
	return FALSE;
}

static gboolean
xpad_pad_configure_event (XpadPad *pad, GdkEventConfigure *event)
{
	if (!GTK_WIDGET_VISIBLE (pad))
		return FALSE;
	
	if (pad->priv->width != event->width || pad->priv->height != event->height)
		pad->priv->toolbar_pad_resized = TRUE;
	
	pad->priv->x = event->x;
	pad->priv->y = event->y;
	pad->priv->width = event->width;
	pad->priv->height = event->height;
	pad->priv->location_valid = TRUE;
	
	xpad_pad_save_info (pad);
	
	/* Sometimes when moving, if the toolbar tries to hide itself,
		the window manager will not resize it correctly.  So, we make
		sure not to end the timeout while moving. */
	if (pad->priv->toolbar_timeout)
	{
		g_source_remove (pad->priv->toolbar_timeout);
		pad->priv->toolbar_timeout = g_timeout_add (1000, (GSourceFunc) toolbar_timeout, pad);
	}
	
	return FALSE;
}

static gboolean
xpad_pad_window_state_event (XpadPad *pad, GdkEventWindowState *event)
{
	if (event->changed_mask & GDK_WINDOW_STATE_STICKY) {
		if (GTK_WIDGET_VISIBLE (pad))
		{
			pad->priv->sticky = (event->new_window_state & GDK_WINDOW_STATE_STICKY) ? TRUE : FALSE;
			xpad_pad_save_info (pad);
		}
	}
	
	return FALSE;
}

static gboolean
xpad_pad_delete_event (XpadPad *pad, GdkEvent *event)
{
	xpad_pad_close (pad);
	
	return TRUE;
}

static gboolean
xpad_pad_popup_menu (XpadPad *pad)
{
	xpad_pad_popup (pad, NULL);
	
	return TRUE;
}

static gboolean
xpad_pad_text_view_button_press_event (GtkWidget *text_view, GdkEventButton *event, XpadPad *pad)
{
	if (event->type == GDK_BUTTON_PRESS)
	{
		switch (event->button)
		{
		case 1:
			if ((event->state & gtk_accelerator_get_default_mod_mask ()) == GDK_CONTROL_MASK)
			{
				gtk_window_begin_move_drag (GTK_WINDOW (pad), event->button, event->x_root, event->y_root, event->time);
				return TRUE;
			}
			break;
		
		case 3:
			if ((event->state & gtk_accelerator_get_default_mod_mask ()) == GDK_CONTROL_MASK)
			{
				GdkWindowEdge edge;
				
				if (gtk_widget_get_direction (GTK_WIDGET (pad)) == GTK_TEXT_DIR_LTR)
					edge = GDK_WINDOW_EDGE_SOUTH_EAST;
				else
					edge = GDK_WINDOW_EDGE_SOUTH_WEST;
				
				gtk_window_begin_resize_drag (GTK_WINDOW (pad), edge, event->button, event->x_root, event->y_root, event->time);
			}
			else
			{
				xpad_pad_popup (pad, event);
			}
			return TRUE;
		}
	}
	
	return FALSE;
}

static gboolean
xpad_pad_button_press_event (XpadPad *pad, GdkEventButton *event)
{
	if (event->type == GDK_BUTTON_PRESS)
	{
		switch (event->button)
		{
		case 1:
			gtk_window_begin_move_drag (GTK_WINDOW (pad), event->button, event->x_root, event->y_root, event->time);
			return TRUE;
		
		case 3:
			if ((event->state & gtk_accelerator_get_default_mod_mask ()) == GDK_CONTROL_MASK)
			{
				GdkWindowEdge edge;
				
				if (gtk_widget_get_direction (GTK_WIDGET (pad)) == GTK_TEXT_DIR_LTR)
					edge = GDK_WINDOW_EDGE_SOUTH_EAST;
				else
					edge = GDK_WINDOW_EDGE_SOUTH_WEST;
				
				gtk_window_begin_resize_drag (GTK_WINDOW (pad), edge, event->button, event->x_root, event->y_root, event->time);
			}
			else
			{
				xpad_pad_popup (pad, event);
			}
			return TRUE;
		}
	}
	
	return FALSE;
}

static void
xpad_pad_sync_title (XpadPad *pad)
{
	GtkTextBuffer *buffer;
	GtkTextIter s, e;
	gchar *content, *end;
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview));
	gtk_text_buffer_get_bounds (buffer, &s, &e);
	content = gtk_text_buffer_get_text (buffer, &s, &e, FALSE);
	end = g_utf8_strchr (content, -1, '\n');
	if (end)
		*end = '\0';
	
	gtk_window_set_title (GTK_WINDOW (pad), g_strstrip (content));
	
	g_free (content);
}

static void
xpad_pad_set_group (XpadPad *pad, XpadPadGroup *group)
{
	pad->priv->group = group;
	if (group)
		xpad_pad_group_add (group, GTK_WIDGET (pad));
}

static XpadPadGroup *
xpad_pad_get_group (XpadPad *pad)
{
	return pad->priv->group;
}

static void
xpad_pad_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadPad *pad;
	
	pad = XPAD_PAD (object);
	
	switch (prop_id)
	{
	case PROP_GROUP:
		xpad_pad_set_group (pad, g_value_get_pointer (value));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xpad_pad_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadPad *pad;
	
	pad = XPAD_PAD (object);
	
	switch (prop_id)
	{
	case PROP_GROUP:
		g_value_set_pointer (value, xpad_pad_get_group (pad));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

void
xpad_pad_load_content (XpadPad *pad)
{
	g_return_if_fail (pad);

	gchar *content;
	GtkTextBuffer *buffer;
	
	if (!pad->priv->contentname)
		return;
	
	content = fio_get_file (pad->priv->contentname);
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview));
	
	xpad_text_buffer_freeze_undo (XPAD_TEXT_BUFFER (buffer));
	g_signal_handlers_block_by_func (buffer, xpad_pad_text_changed, pad);
	
	xpad_text_buffer_set_text_with_tags (XPAD_TEXT_BUFFER (buffer), content ? content : "");
	g_free (content);
	
	g_signal_handlers_unblock_by_func (buffer, xpad_pad_text_changed, pad);
	xpad_text_buffer_thaw_undo (XPAD_TEXT_BUFFER (buffer));

	xpad_pad_text_changed(pad, buffer);
}

void
xpad_pad_save_content (XpadPad *pad)
{
	g_return_if_fail (pad);

	gchar *content;
	GtkTextBuffer *buffer;
	
	/* create content file if it doesn't exist yet */
	if (!pad->priv->contentname)
	{
		pad->priv->contentname = fio_unique_name ("content-");
		if (!pad->priv->contentname)
			return;
	}
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview));
	content = xpad_text_buffer_get_text_with_tags (XPAD_TEXT_BUFFER (buffer));
	
	fio_set_file (pad->priv->contentname, content);
	
	g_free (content);
}

static void
load_info (XpadPad *pad, gboolean *show)
{
	gboolean locked = FALSE, follow_font = TRUE, follow_color = TRUE;
	gboolean hidden = FALSE;
	GdkColor text = {0}, back = {0};
	gchar *fontname = NULL, *oldcontentprefix;
	
	if (!pad->priv->infoname)
		return;
	
	if (fio_get_values_from_file (pad->priv->infoname, 
		"i|width", &pad->priv->width,
		"i|height", &pad->priv->height,
		"i|x", &pad->priv->x,
		"i|y", &pad->priv->y,
		"b|locked", &locked,
		"b|follow_font", &follow_font,
		"b|follow_color", &follow_color,
		"b|sticky", &pad->priv->sticky,
		"b|hidden", &hidden,
		"h|back_red", &back.red,
		"h|back_green", &back.green,
		"h|back_blue", &back.blue,
		"h|text_red", &text.red,
		"h|text_green", &text.green,
		"h|text_blue", &text.blue,
		"s|fontname", &fontname,
		"s|content", &pad->priv->contentname,
		NULL))
		return;
	
	pad->priv->location_valid = TRUE;
	if (xpad_settings_get_has_toolbar (xpad_settings ()) &&
		 !xpad_settings_get_autohide_toolbar (xpad_settings ()))
	{
		pad->priv->toolbar_height = 0;
		xpad_pad_hide_toolbar (pad);
		xpad_pad_show_toolbar (pad); /* these will resize pad at correct height */
	}
	else
		gtk_window_resize (GTK_WINDOW (pad), pad->priv->width, pad->priv->height);
	gtk_window_move (GTK_WINDOW (pad), pad->priv->x, pad->priv->y);
	
	xpad_text_view_set_follow_font_style (XPAD_TEXT_VIEW (pad->priv->textview), follow_font);
	xpad_text_view_set_follow_color_style (XPAD_TEXT_VIEW (pad->priv->textview), follow_color);
	
	/* obsolete setting, no longer written as of xpad-2.0-b2 */
	if (locked)
	{
		xpad_text_view_set_follow_font_style (XPAD_TEXT_VIEW (pad->priv->textview), FALSE);
		xpad_text_view_set_follow_color_style (XPAD_TEXT_VIEW (pad->priv->textview), FALSE);
	}
	
	if (!xpad_text_view_get_follow_color_style (XPAD_TEXT_VIEW (pad->priv->textview)))
	{
		gtk_widget_modify_text (pad->priv->textview, GTK_STATE_NORMAL, &text);
		gtk_widget_modify_base (pad->priv->textview, GTK_STATE_NORMAL, &back);
	}
	
	if (!xpad_text_view_get_follow_font_style (XPAD_TEXT_VIEW (pad->priv->textview)))
	{
		PangoFontDescription *font_desc = pango_font_description_from_string (fontname);
		gtk_widget_modify_font (pad->priv->textview, font_desc);
		pango_font_description_free (font_desc);
	}
	
	if (pad->priv->sticky)
		gtk_window_stick (GTK_WINDOW (pad));
	else
		gtk_window_unstick (GTK_WINDOW (pad));
	
	/* Special check for contentname being absolute.  A while back,
		xpad had absolute pathnames, pointing to ~/.xpad/content-*.
		Now, files are kept in ~/.config/xpad, so using old config
		files with a new xpad will break pads.  We check to see if
		contentname is old pointer and then make it relative. */
	oldcontentprefix = g_build_filename (g_get_home_dir (), ".xpad", "content-", NULL);
	if (g_str_has_prefix (pad->priv->contentname, oldcontentprefix))
	{
		gchar *oldcontent = pad->priv->contentname;
		pad->priv->contentname = g_path_get_basename (oldcontent);
		g_free (oldcontent);
	}
	g_free (oldcontentprefix);
	
	if (show)
		*show = !hidden;
}

void
xpad_pad_save_info (XpadPad *pad)
{
	gint height;
	GtkStyle *style;
	gchar *fontname;
	
	/* Must create pad info file if it doesn't exist yet */
	if (!pad->priv->infoname)
	{
		pad->priv->infoname = fio_unique_name ("info-");
		if (!pad->priv->infoname)
			return;
		gtk_window_set_role (GTK_WINDOW (pad), pad->priv->infoname);
	}
	/* create content file if it doesn't exist yet */
	if (!pad->priv->contentname)
	{
		pad->priv->contentname = fio_unique_name ("content-");
		if (!pad->priv->contentname)
			return;
	}
	
	height = pad->priv->height;
	if (GTK_WIDGET_VISIBLE (pad->priv->toolbar) && pad->priv->toolbar_expanded)
		height -= pad->priv->toolbar_height;
	
	style = gtk_widget_get_style (pad->priv->textview);
	fontname = pango_font_description_to_string (style->font_desc);
	
	fio_set_values_to_file (pad->priv->infoname,
		"i|width", pad->priv->width,
		"i|height", height,
		"i|x", pad->priv->x,
		"i|y", pad->priv->y,
		"b|follow_font", xpad_text_view_get_follow_font_style (XPAD_TEXT_VIEW (pad->priv->textview)),
		"b|follow_color", xpad_text_view_get_follow_color_style (XPAD_TEXT_VIEW (pad->priv->textview)),
		"b|sticky", pad->priv->sticky,
		"b|hidden", !GTK_WIDGET_VISIBLE (pad),
		"h|back_red", style->base[GTK_STATE_NORMAL].red,
		"h|back_green", style->base[GTK_STATE_NORMAL].green,
		"h|back_blue", style->base[GTK_STATE_NORMAL].blue,
		"h|text_red", style->text[GTK_STATE_NORMAL].red,
		"h|text_green", style->text[GTK_STATE_NORMAL].green,
		"h|text_blue", style->text[GTK_STATE_NORMAL].blue,
		"s|fontname", fontname,
		"s|content", pad->priv->contentname,
		NULL);
	
	g_free (fontname);
}

static void
menu_about (XpadPad *pad)
{
	const gchar *artists[] = {"Michael Terry <mike@mterry.name>", NULL};
	const gchar *authors[] = {"Jeroen Vermeulen <jtv@xs4all.nl>", "Michael Terry <mike@mterry.name>", "Paul Ivanov <pivanov@berkeley.edu>", NULL};
	const gchar *comments = _("Sticky notes");
	const gchar *copyright = "© 2001-2007 Michael Terry";
	/* we use g_strdup_printf because C89 has size limits on static strings */
	gchar *license = g_strdup_printf ("%s\n%s\n%s",
"This program is free software; you can redistribute it and/or\n"
"modify it under the terms of the GNU General Public License\n"
"as published by the Free Software Foundation; either version 3\n"
"of the License, or (at your option) any later version.\n"
,
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
,
"You should have received a copy of the GNU General Public License\n"
"along with this program; if not, write to the Free Software\n"
"Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.");
	/* Translators: please translate this as your own name and optionally email
		like so: "Your Name <your@email.com>" */
	const gchar *translator_credits = _("translator-credits");
	const gchar *website = "http://xpad.sourceforge.net/";
	
	gtk_show_about_dialog (GTK_WINDOW (pad),
		"artists", artists,
		"authors", authors,
		"comments", comments,
		"copyright", copyright,
		"license", license,
		"logo-icon-name", PACKAGE,
		"translator-credits", translator_credits,
		"version", VERSION,
		"website", website,
		NULL);
	
	g_free (license);
}

void
xpad_pad_cut (XpadPad *pad)
{
	gtk_text_buffer_cut_clipboard (
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)),
		gtk_clipboard_get (GDK_SELECTION_CLIPBOARD),
		TRUE);
}

static void
menu_cut (XpadPad *pad)
{
	xpad_pad_cut (pad);
}

void
xpad_pad_copy (XpadPad *pad)
{
	gtk_text_buffer_copy_clipboard (
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)),
		gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
}

static void
menu_copy (XpadPad *pad)
{
	xpad_pad_copy (pad);
}

void
xpad_pad_paste (XpadPad *pad)
{
	gtk_text_buffer_paste_clipboard (
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)),
		gtk_clipboard_get (GDK_SELECTION_CLIPBOARD),
		NULL,
		TRUE);
}

static void
menu_paste (XpadPad *pad)
{
	xpad_pad_paste (pad);
}

void
xpad_pad_undo (XpadPad *pad)
{
	g_return_if_fail (pad->priv->textview);
	XpadTextBuffer *buffer = NULL;
	buffer = XPAD_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));
	g_return_if_fail (buffer);
	xpad_text_buffer_undo (buffer);
}

static void
menu_undo (XpadPad *pad)
{
	xpad_pad_undo (pad);
}

void
xpad_pad_redo (XpadPad *pad)
{
	g_return_if_fail (pad->priv->textview);
	XpadTextBuffer *buffer = NULL;
	buffer = XPAD_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));
	g_return_if_fail (buffer);
	xpad_text_buffer_redo (buffer);
}

static void
menu_redo (XpadPad *pad)
{
	xpad_pad_redo (pad);
}

static void
menu_show_all (XpadPad *pad)
{
	GSList *pads, *i;
	
	if (!pad->priv->group)
		return;
	
	pads = xpad_pad_group_get_pads (pad->priv->group);
	
	for (i = pads; i; i = i->next)
	{
		if (XPAD_PAD (i->data) != pad)
			gtk_window_present (GTK_WINDOW (i->data));
	}
	gtk_window_present (GTK_WINDOW (pad));
	
	g_slist_free (pads);
}

static void
xpad_pad_close_all (XpadPad *pad)
{
	if (!pad->priv->group)
		return;
	
	/**
	 * The logic is different here depending on whether the tray is open.
	 * If it is open, we just close each pad individually.  If it isn't
	 * open, we do a quit.  This way, when xpad is run again, only the
	 * pads open during the last 'close all' will open again.
	 */
	if (xpad_tray_is_open ())
		xpad_pad_group_close_all (pad->priv->group);
	else
		xpad_pad_quit (pad);
}

static void
menu_show (XpadPad *pad)
{
	gtk_window_present (GTK_WINDOW (pad));
}

static void
menu_toggle_tag (XpadPad *pad, const gchar *name)
{
	g_return_if_fail (pad->priv->textview);
	XpadTextBuffer *buffer = NULL;
	buffer = XPAD_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));
	xpad_text_buffer_toggle_tag (buffer, name);
	xpad_pad_save_content (pad);
}

static void
menu_bold (XpadPad *pad)
{
	menu_toggle_tag (pad, "bold");
}

static void
menu_italic (XpadPad *pad)
{
	menu_toggle_tag (pad, "italic");
}

static void
menu_underline (XpadPad *pad)
{
	menu_toggle_tag (pad, "underline");
}

static void
menu_strikethrough (XpadPad *pad)
{
	menu_toggle_tag (pad, "strikethrough");
}

static void
menu_sticky (XpadPad *pad, GtkCheckMenuItem *check)
{
	if (gtk_check_menu_item_get_active (check))
		gtk_window_stick (GTK_WINDOW (pad));
	else
		gtk_window_unstick (GTK_WINDOW (pad));
}

static void
menu_toolbar (XpadPad *pad, GtkCheckMenuItem *check)
{
	xpad_settings_set_has_toolbar (xpad_settings (), gtk_check_menu_item_get_active (check));
}

static void
menu_scrollbar (XpadPad *pad, GtkCheckMenuItem *check)
{
	xpad_settings_set_has_scrollbar (xpad_settings (), gtk_check_menu_item_get_active (check));
}

static void
menu_autohide (XpadPad *pad, GtkCheckMenuItem *check)
{
	xpad_settings_set_autohide_toolbar (xpad_settings (), gtk_check_menu_item_get_active (check));
}

static void
menu_decorated (XpadPad *pad, GtkCheckMenuItem *check)
{
	xpad_settings_set_has_decorations (xpad_settings (), gtk_check_menu_item_get_active (check));
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

#define MENU_ADD(mnemonic, image, key, mask, callback) {\
	item = gtk_image_menu_item_new_with_mnemonic (mnemonic);\
	if (image) {\
		GtkWidget *imgwidget = gtk_image_new_from_stock (image, GTK_ICON_SIZE_MENU);\
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), imgwidget);\
	}\
	g_signal_connect_swapped (item, "activate", G_CALLBACK (callback), pad);\
	if (key)\
		gtk_widget_add_accelerator(item, "activate", accel_group, key, mask, GTK_ACCEL_VISIBLE);\
	gtk_container_add (GTK_CONTAINER (menu), item);\
	gtk_widget_show (item);\
	}

#define MENU_ADD_STOCK(stock, callback) {\
	item = gtk_image_menu_item_new_from_stock (stock, accel_group);\
	g_signal_connect_swapped (item, "activate", G_CALLBACK (callback), pad);\
	gtk_container_add (GTK_CONTAINER (menu), item);\
	gtk_widget_show (item);\
	}

#define MENU_ADD_STOCK_WITH_ACCEL(stock, callback, key, mask) {\
	item = gtk_image_menu_item_new_from_stock (stock, accel_group);\
	g_signal_connect_swapped (item, "activate", G_CALLBACK (callback), pad);\
	if (key)\
		gtk_widget_add_accelerator(item, "activate", accel_group, key, mask, GTK_ACCEL_VISIBLE);\
	gtk_container_add (GTK_CONTAINER (menu), item);\
	gtk_widget_show (item);\
	}

#define MENU_ADD_CHECK(mnemonic, active, callback) {\
	item = gtk_check_menu_item_new_with_mnemonic (mnemonic);\
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), active);\
	g_signal_connect_swapped (item, "toggled", G_CALLBACK (callback), pad);\
	gtk_container_add (GTK_CONTAINER (menu), item);\
	gtk_widget_show (item);\
	}

#define MENU_ADD_SEP() {\
	item = gtk_separator_menu_item_new ();\
	gtk_container_add (GTK_CONTAINER (menu), item);\
	gtk_widget_show (item);\
	}

static GtkWidget *
menu_get_popup_no_highlight (XpadPad *pad, GtkAccelGroup *accel_group)
{
	GtkWidget *uppermenu, *menu, *item;
	
	uppermenu = gtk_menu_new ();
	gtk_menu_set_accel_group (GTK_MENU (uppermenu), accel_group);
	
	item = gtk_menu_item_new_with_mnemonic (_("_Pad"));
	gtk_container_add (GTK_CONTAINER (uppermenu), item);
	gtk_widget_show (item);
	
	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	
	MENU_ADD_STOCK (GTK_STOCK_NEW, xpad_pad_spawn);
	MENU_ADD_SEP ();
	MENU_ADD_CHECK (_("Show on _All Workspaces"), pad->priv->sticky, menu_sticky);
	g_object_set_data (G_OBJECT (uppermenu), "sticky", item);
	MENU_ADD_STOCK (GTK_STOCK_PROPERTIES, xpad_pad_open_properties);
	MENU_ADD_SEP ();
	MENU_ADD_STOCK (GTK_STOCK_CLOSE, xpad_pad_close);
	MENU_ADD_STOCK (GTK_STOCK_DELETE, xpad_pad_delete);
	
	
	item = gtk_menu_item_new_with_mnemonic (_("_Edit"));
	gtk_container_add (GTK_CONTAINER (uppermenu), item);
	gtk_widget_show (item);
	
	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	 
	MENU_ADD_STOCK_WITH_ACCEL (GTK_STOCK_UNDO, menu_undo, GDK_Z, GDK_CONTROL_MASK);
	g_object_set_data (G_OBJECT (uppermenu), "undo", item);
	 
	MENU_ADD_STOCK_WITH_ACCEL (GTK_STOCK_REDO, menu_redo, GDK_R, GDK_CONTROL_MASK);
	g_object_set_data (G_OBJECT (uppermenu), "redo", item);

	MENU_ADD_SEP();
	
	MENU_ADD_STOCK (GTK_STOCK_PASTE, menu_paste);
	g_object_set_data (G_OBJECT (uppermenu), "paste", item);

	MENU_ADD_SEP ();

	MENU_ADD_STOCK (GTK_STOCK_PREFERENCES, xpad_pad_open_preferences);
	
	
	item = gtk_menu_item_new_with_mnemonic (_("_View"));
	gtk_container_add (GTK_CONTAINER (uppermenu), item);
	gtk_widget_show (item);
	
	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	
	MENU_ADD_CHECK (_("_Toolbar"), xpad_settings_get_has_toolbar (xpad_settings ()), menu_toolbar);
	MENU_ADD_CHECK (_("_Autohide Toolbar"), xpad_settings_get_autohide_toolbar (xpad_settings ()), menu_autohide);
	gtk_widget_set_sensitive (item, xpad_settings_get_has_toolbar (xpad_settings ()));
	MENU_ADD_CHECK (_("_Scrollbar"), xpad_settings_get_has_scrollbar (xpad_settings ()), menu_scrollbar);
	MENU_ADD_CHECK (_("_Window Decorations"), xpad_settings_get_has_decorations (xpad_settings ()), menu_decorated);
	
	
	item = gtk_menu_item_new_with_mnemonic (_("_Notes"));
	gtk_container_add (GTK_CONTAINER (uppermenu), item);
	gtk_widget_show (item);
	
	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	g_object_set_data (G_OBJECT (uppermenu), "notes-menu", menu);
	
	MENU_ADD (_("_Show All"), NULL, 0, 0, menu_show_all);
	MENU_ADD (_("_Close All"), NULL, 0, 0, xpad_pad_close_all);
	
	/* The rest of the notes menu will get set up in the prep function below */
	
	item = gtk_menu_item_new_with_mnemonic (_("_Help"));
	gtk_container_add (GTK_CONTAINER (uppermenu), item);
	gtk_widget_show (item);
	
	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	
	MENU_ADD (_("_Contents"), GTK_STOCK_HELP, GDK_F1, 0, show_help);
	MENU_ADD (_("_About"), GTK_STOCK_ABOUT, 0, 0, menu_about);
	
	return uppermenu;
}

static void
menu_prep_popup_no_highlight (XpadPad *current_pad, GtkWidget *uppermenu)
{
	GtkWidget *menu, *item;

	GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

	XpadTextBuffer *buffer = XPAD_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (current_pad->priv->textview)));
	
	item = g_object_get_data (G_OBJECT (uppermenu), "paste");
	if (item)
		gtk_widget_set_sensitive (item, gtk_clipboard_wait_is_text_available (clipboard));

	item = g_object_get_data (G_OBJECT (uppermenu), "undo");
	if (item)
		gtk_widget_set_sensitive (item, xpad_text_buffer_undo_available (buffer));

	item = g_object_get_data (G_OBJECT (uppermenu), "redo");
	if (item)
		gtk_widget_set_sensitive (item, xpad_text_buffer_redo_available (buffer));
	
	item = g_object_get_data (G_OBJECT (uppermenu), "sticky");
	if (item) {
		g_signal_handlers_block_by_func (item, menu_sticky, current_pad);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), current_pad->priv->sticky);
		g_signal_handlers_unblock_by_func (item, menu_sticky, current_pad);
	}
	
	menu = g_object_get_data (G_OBJECT (uppermenu), "notes-menu");
	if (menu)
	{
		gint n = 1;
		gchar *key;
		
		/* Remove old menu */
		item = g_object_get_data (G_OBJECT (menu), "notes-sep");
		while (item)
		{
			gtk_container_remove (GTK_CONTAINER (menu), item);
			key = g_strdup_printf ("notes-%i", n++);
			item = g_object_get_data (G_OBJECT (menu), key);
			g_free (key);
		}
	}
	if (menu && current_pad->priv->group)
	{
		GSList *pads, *l;
		gint n;
		GtkAccelGroup *accel_group = gtk_menu_get_accel_group (GTK_MENU (uppermenu));
		
		MENU_ADD_SEP ();
		g_object_set_data (G_OBJECT (menu), "notes-sep", item);
		
		/**
		 * Order pads according to title.
		 */
		pads = xpad_pad_group_get_pads (current_pad->priv->group);
		
		pads = g_slist_sort (pads, (GCompareFunc) menu_title_compare);
		
		/**
		 * Populate list of windows.
		 */
		for (l = pads, n = 1; l; l = l->next, n++)
		{
			gchar *title;
			gchar *tmp_title;
			gchar *key;
			GtkWidget *pad = GTK_WIDGET (l->data);
			
			key = g_strdup_printf ("notes-%i", n);
			tmp_title = g_strdup (gtk_window_get_title (GTK_WINDOW (pad)));
			str_replace_tokens (&tmp_title, '_', "__");
			if (n < 10)
				title = g_strdup_printf ("_%i. %s", n, tmp_title);
			else
				title = g_strdup_printf ("%i. %s", n, tmp_title);
			g_free (tmp_title);
			
			MENU_ADD (title, NULL, 0, 0, menu_show);
			g_object_set_data (G_OBJECT (menu), key, item);
			
			g_free (title);
			g_free (key);
		}
		g_slist_free (pads);
	}
}

static GtkWidget *
menu_get_popup_highlight (XpadPad *pad, GtkAccelGroup *accel_group)
{
	GtkWidget *menu, *item;
	
	menu = gtk_menu_new ();
	gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);
	
	MENU_ADD_STOCK (GTK_STOCK_CUT, menu_cut);
	MENU_ADD_STOCK (GTK_STOCK_COPY, menu_copy);
	MENU_ADD_STOCK (GTK_STOCK_PASTE, menu_paste);
	g_object_set_data (G_OBJECT (menu), "paste", item);
	MENU_ADD_SEP ();
	MENU_ADD_STOCK_WITH_ACCEL (GTK_STOCK_BOLD, menu_bold, GDK_b, GDK_CONTROL_MASK);
	MENU_ADD_STOCK_WITH_ACCEL (GTK_STOCK_ITALIC, menu_italic, GDK_i, GDK_CONTROL_MASK);
	MENU_ADD_STOCK_WITH_ACCEL (GTK_STOCK_UNDERLINE, menu_underline, GDK_u, GDK_CONTROL_MASK);
	MENU_ADD_STOCK (GTK_STOCK_STRIKETHROUGH, menu_strikethrough);
	
	return menu;
}

static void
menu_prep_popup_highlight (XpadPad *pad, GtkWidget *menu)
{
	GtkWidget *item;
	GtkClipboard *clipboard;
	
	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	
	item = g_object_get_data (G_OBJECT (menu), "paste");
	if (item)
		gtk_widget_set_sensitive (item, gtk_clipboard_wait_is_text_available (clipboard));
}

static void
menu_popup (GtkWidget *menu, XpadPad *pad)
{
	g_signal_handlers_block_matched (pad, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) xpad_pad_leave_notify_event, NULL);
	pad->priv->toolbar_timeout = 0;
}

static void
menu_popdown (GtkWidget *menu, XpadPad *pad)
{
	GdkRectangle rect;
	
	g_signal_handlers_unblock_matched (pad, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) xpad_pad_leave_notify_event, NULL);
	
	/**
	 * We must check if we disabled off of pad and start the timeout if so.
	 */
	gdk_window_get_pointer (GTK_WIDGET (pad)->window, &rect.x, &rect.y, NULL);
	rect.width = 1;
	rect.height = 1;
	
	if (!pad->priv->toolbar_timeout &&
		 !gtk_widget_intersect (GTK_WIDGET (pad), &rect, NULL))
		pad->priv->toolbar_timeout = g_timeout_add (1000, (GSourceFunc) toolbar_timeout, pad);
}

static void
xpad_pad_popup_deactivate (GtkWidget *menu, XpadPad *pad)
{
	menu_popdown (GTK_WIDGET (menu), pad);
}

static void
xpad_pad_toolbar_popup (GtkWidget *toolbar, GtkMenu *menu, XpadPad *pad)
{
	menu_popup (GTK_WIDGET (menu), pad);
}

static void
xpad_pad_toolbar_popdown (GtkWidget *toolbar, GtkMenu *menu, XpadPad *pad)
{
	menu_popdown (GTK_WIDGET (menu), pad);
}

static void
xpad_pad_popup (XpadPad *pad, GdkEventButton *event)
{
	GtkTextBuffer *buffer;
	GtkWidget *menu;
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview));
	
	if (gtk_text_buffer_get_selection_bounds (buffer, NULL, NULL))
	{
		menu = pad->priv->highlight_menu;
		menu_prep_popup_highlight (pad, menu);
	}
	else
	{
		menu = pad->priv->menu;
		menu_prep_popup_no_highlight (pad, menu);
	}
	
	if (!menu)
		return;
	
	menu_popup (GTK_WIDGET (menu), pad);
	
	if (event)
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);
	else
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
}
