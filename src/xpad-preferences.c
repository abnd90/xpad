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
#include "xpad-app.h"
#include "xpad-preferences.h"
#include "xpad-settings.h"

G_DEFINE_TYPE(XpadPreferences, xpad_preferences, GTK_TYPE_DIALOG)
#define XPAD_PREFERENCES_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), XPAD_TYPE_PREFERENCES, XpadPreferencesPrivate))

struct XpadPreferencesPrivate 
{
	GtkWidget *fontcheck;
	GtkWidget *antifontcheck;
	GtkWidget *colorcheck;
	GtkWidget *anticolorcheck;
	GtkWidget *colorbox;
	
	GtkWidget *editcheck;
	GtkWidget *stickycheck;
	GtkWidget *confirmcheck;
	
	GtkWidget *textbutton;
	GtkWidget *backbutton;
	GtkWidget *fontbutton;
	
	guint notify_edit_handler;
	guint notify_sticky_handler;
	guint notify_confirm_handler;
	guint notify_font_handler;
	guint notify_back_handler;
	guint notify_text_handler;
	guint font_handler;
	guint back_handler;
	guint text_handler;
	guint colorcheck_handler;
	guint fontcheck_handler;
	guint editcheck_handler;
	guint stickycheck_handler;
	guint confirmcheck_handler;
};

static void change_edit_check (GtkToggleButton *button, XpadPreferences *pref);
static void change_sticky_check (GtkToggleButton *button, XpadPreferences *pref);
static void change_confirm_check (GtkToggleButton *button, XpadPreferences *pref);
static void change_color_check (GtkToggleButton *button, XpadPreferences *pref);
static void change_font_check (GtkToggleButton *button, XpadPreferences *pref);
static void change_text_color (GtkColorButton *button, XpadPreferences *pref);
static void change_back_color (GtkColorButton *button, XpadPreferences *pref);
static void change_font_face (GtkFontButton *button, XpadPreferences *pref);
static void notify_edit (XpadPreferences *pref);
static void notify_sticky (XpadPreferences *pref);
static void notify_confirm (XpadPreferences *pref);
static void notify_fontname (XpadPreferences *pref);
static void notify_text_color (XpadPreferences *pref);
static void notify_back_color (XpadPreferences *pref);
static void xpad_preferences_finalize (GObject *object);
static void xpad_preferences_response (GtkDialog *dialog, gint response);

static GtkWidget *_xpad_preferences = NULL;

void
xpad_preferences_open (void)
{
	if (_xpad_preferences)
	{
		gtk_window_present (GTK_WINDOW (_xpad_preferences));
	}
	else
	{
		_xpad_preferences = GTK_WIDGET (g_object_new (XPAD_TYPE_PREFERENCES, NULL));
		g_signal_connect_swapped (_xpad_preferences, "destroy", G_CALLBACK (g_nullify_pointer), &_xpad_preferences);
		gtk_widget_show (_xpad_preferences);
	}
}

static void
xpad_preferences_class_init (XpadPreferencesClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->finalize = xpad_preferences_finalize;
	
	g_type_class_add_private (gobject_class, sizeof (XpadPreferencesPrivate));
}

static void
xpad_preferences_init (XpadPreferences *pref)
{
	GtkWidget *hbox, *font_hbox, *vbox;
	const GdkColor *color;
	const gchar *fontname;
	GtkStyle *style;
	GtkWidget *label, *appearance_frame, *alignment, *appearance_vbox;
	GtkWidget *options_frame, *options_vbox, *global_vbox;
	gchar *text;
	GtkSizeGroup *size_group_labels = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	GtkRequisition req;
	
	pref->priv = XPAD_PREFERENCES_GET_PRIVATE (pref);
	
	text = g_strconcat ("<b>", _("Appearance"), "</b>", NULL);
	label = GTK_WIDGET (g_object_new (GTK_TYPE_LABEL,
		"label", text,
		"use-markup", TRUE,
		"xalign", 0.0,
		NULL));
	g_free (text);
	appearance_vbox = GTK_WIDGET (g_object_new (GTK_TYPE_VBOX,
		"homogeneous", FALSE,
		"spacing", 18,
		NULL));
	alignment = gtk_alignment_new (1, 1, 1, 1);
	g_object_set (G_OBJECT (alignment),
		"left-padding", 12,
		"top-padding", 12,
		"child", appearance_vbox,
		NULL);
	appearance_frame = GTK_WIDGET (g_object_new (GTK_TYPE_FRAME,
		"label-widget", label,
		"shadow-type", GTK_SHADOW_NONE,
		"child", alignment,
		NULL));
	
	pref->priv->textbutton = gtk_color_button_new ();
	pref->priv->backbutton = gtk_color_button_new ();
	pref->priv->fontbutton = gtk_font_button_new ();
	
	pref->priv->antifontcheck = gtk_radio_button_new_with_mnemonic (NULL, _("Use font from theme"));
	pref->priv->fontcheck = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pref->priv->antifontcheck), _("Use this font:"));
	pref->priv->anticolorcheck = gtk_radio_button_new_with_mnemonic (NULL, _("Use colors from theme"));
	pref->priv->colorcheck = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pref->priv->anticolorcheck), _("Use these colors:"));
	
	font_hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (font_hbox), pref->priv->fontcheck, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (font_hbox), pref->priv->fontbutton, TRUE, TRUE, 0);
	
	pref->priv->colorbox = gtk_vbox_new (FALSE, 6);
	hbox = gtk_hbox_new (FALSE, 12);
	label = gtk_label_new_with_mnemonic (_("Background:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_size_group_add_widget (size_group_labels, label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), pref->priv->backbutton, TRUE, TRUE, 0);
	g_object_set (G_OBJECT (pref->priv->colorbox), "child", hbox, NULL);
	
	hbox = gtk_hbox_new (FALSE, 12);
	label = gtk_label_new_with_mnemonic (_("Foreground:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_size_group_add_widget (size_group_labels, label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), pref->priv->textbutton, TRUE, TRUE, 0);
	g_object_set (G_OBJECT (pref->priv->colorbox), "child", hbox, NULL);
	
	alignment = gtk_alignment_new (1, 1, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);
	gtk_container_add (GTK_CONTAINER (alignment), pref->priv->colorbox);
	
	pref->priv->editcheck = gtk_check_button_new_with_mnemonic (_("_Edit lock"));
	pref->priv->stickycheck = gtk_check_button_new_with_mnemonic (_("_Pads start on all workspaces"));
	pref->priv->confirmcheck = gtk_check_button_new_with_mnemonic (_("_Confirm pad deletion"));
	
	gtk_dialog_add_button (GTK_DIALOG (pref), "gtk-close", GTK_RESPONSE_CLOSE);
	gtk_dialog_set_default_response (GTK_DIALOG (pref), GTK_RESPONSE_CLOSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (pref), FALSE);
	g_signal_connect (pref, "response", G_CALLBACK (xpad_preferences_response), NULL);
	gtk_window_set_title (GTK_WINDOW (pref), _("Xpad Preferences"));
	
	gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (pref->priv->textbutton), FALSE);
	gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (pref->priv->backbutton), xpad_app_get_translucent ());
	
	gtk_color_button_set_title (GTK_COLOR_BUTTON (pref->priv->textbutton), _("Set Foreground Color"));
	gtk_color_button_set_title (GTK_COLOR_BUTTON (pref->priv->backbutton), _("Set Background Color"));
	gtk_font_button_set_title (GTK_FONT_BUTTON (pref->priv->fontbutton), _("Set Font"));
	
	/* Set current state */
	style = gtk_widget_get_default_style ();
	
	color = xpad_settings_get_back_color (xpad_settings ());
	if (color)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->colorcheck), TRUE);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (pref->priv->backbutton), color);
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->anticolorcheck), TRUE);
		gtk_widget_set_sensitive (pref->priv->colorbox, FALSE);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (pref->priv->backbutton), &style->base[GTK_STATE_NORMAL]);
	}
	
	color = xpad_settings_get_text_color (xpad_settings ());
	if (color)
		gtk_color_button_set_color (GTK_COLOR_BUTTON (pref->priv->textbutton), color);
	else
		gtk_color_button_set_color (GTK_COLOR_BUTTON (pref->priv->textbutton), &style->text[GTK_STATE_NORMAL]);
	
	fontname = xpad_settings_get_fontname (xpad_settings ());
	if (fontname)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->fontcheck), TRUE);
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (pref->priv->fontbutton), fontname);
	}
	else
	{
		gchar *str;
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->antifontcheck), TRUE);
		gtk_widget_set_sensitive (pref->priv->fontbutton, FALSE);
		
		str = pango_font_description_to_string (style->font_desc);
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (pref->priv->fontbutton), str);
		g_free (str);
	}
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->editcheck), xpad_settings_get_edit_lock (xpad_settings ()));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->stickycheck), xpad_settings_get_sticky (xpad_settings ()));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->confirmcheck), xpad_settings_get_confirm_destroy (xpad_settings ()));
	
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), pref->priv->antifontcheck, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), font_hbox, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (appearance_vbox), vbox, FALSE, FALSE, 0);
	
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), pref->priv->anticolorcheck, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), pref->priv->colorcheck, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (appearance_vbox), vbox, FALSE, FALSE, 0);
	
	
	text = g_strconcat ("<b>", _("Options"), "</b>", NULL);
	label = GTK_WIDGET (g_object_new (GTK_TYPE_LABEL,
		"label", text,
		"use-markup", TRUE,
		"xalign", 0.0,
		NULL));
	g_free (text);
	options_vbox = GTK_WIDGET (g_object_new (GTK_TYPE_VBOX,
		"homogeneous", FALSE,
		"spacing", 6,
		NULL));
	alignment = gtk_alignment_new (1, 1, 1, 1);
	g_object_set (G_OBJECT (alignment),
		"left-padding", 12,
		"top-padding", 12,
		"child", options_vbox,
		NULL);
	options_frame = GTK_WIDGET (g_object_new (GTK_TYPE_FRAME,
		"label-widget", label,
		"shadow-type", GTK_SHADOW_NONE,
		"child", alignment,
		NULL));
	
	
	gtk_box_pack_start (GTK_BOX (options_vbox), pref->priv->editcheck, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (options_vbox), pref->priv->stickycheck, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (options_vbox), pref->priv->confirmcheck, FALSE, FALSE, 0);	
	
	global_vbox = g_object_new (GTK_TYPE_VBOX,
		"border-width", 6,
		"homogeneous", FALSE,
		"spacing", 18,
		"child", appearance_frame,
		"child", options_frame,
		NULL);
	
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pref)->vbox), global_vbox, FALSE, FALSE, 0);
	
	pref->priv->editcheck_handler = g_signal_connect (pref->priv->editcheck, "toggled", G_CALLBACK (change_edit_check), pref);
	pref->priv->stickycheck_handler = g_signal_connect (pref->priv->stickycheck, "toggled", G_CALLBACK (change_sticky_check), pref);
	pref->priv->confirmcheck_handler = g_signal_connect (pref->priv->confirmcheck, "toggled", G_CALLBACK (change_confirm_check), pref);
	pref->priv->colorcheck_handler = g_signal_connect (pref->priv->colorcheck, "toggled", G_CALLBACK (change_color_check), pref);
	pref->priv->fontcheck_handler = g_signal_connect (pref->priv->fontcheck, "toggled", G_CALLBACK (change_font_check), pref);
	pref->priv->text_handler = g_signal_connect (pref->priv->textbutton, "color-set", G_CALLBACK (change_text_color), pref);
	pref->priv->back_handler = g_signal_connect (pref->priv->backbutton, "color-set", G_CALLBACK (change_back_color), pref);
	pref->priv->font_handler = g_signal_connect (pref->priv->fontbutton, "font-set", G_CALLBACK (change_font_face), pref);
	pref->priv->notify_font_handler = g_signal_connect_swapped (xpad_settings (), "notify::fontname", G_CALLBACK (notify_fontname), pref);
	pref->priv->notify_text_handler = g_signal_connect_swapped (xpad_settings (), "notify::text-color", G_CALLBACK (notify_text_color), pref);
	pref->priv->notify_back_handler = g_signal_connect_swapped (xpad_settings (), "notify::back-color", G_CALLBACK (notify_back_color), pref);
	pref->priv->notify_sticky_handler = g_signal_connect_swapped (xpad_settings (), "notify::sticky", G_CALLBACK (notify_sticky), pref);
	pref->priv->notify_edit_handler = g_signal_connect_swapped (xpad_settings (), "notify::edit-lock", G_CALLBACK (notify_edit), pref);
	pref->priv->notify_confirm_handler = g_signal_connect_swapped (xpad_settings (), "notify::confirm-destroy", G_CALLBACK (notify_confirm), pref);
	
	g_object_unref (size_group_labels);
	
	gtk_widget_show_all (GTK_DIALOG (pref)->vbox);
	
	/* Make window not so squished */
	gtk_widget_size_request (GTK_WIDGET (pref), &req);
	g_object_set (G_OBJECT (pref), "default-width", (gint) (req.height * 0.8), NULL);
}

static void
xpad_preferences_finalize (GObject *object)
{
	XpadPreferences *pref = XPAD_PREFERENCES (object);
	
	g_signal_handlers_disconnect_matched (xpad_settings (), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, pref);
	
	G_OBJECT_CLASS (xpad_preferences_parent_class)->finalize (object);
}

static void
xpad_preferences_response (GtkDialog *dialog, gint response)
{
	if (response == GTK_RESPONSE_CLOSE)
		gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
change_color_check (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (xpad_settings (), pref->priv->notify_back_handler);
	g_signal_handler_block (xpad_settings (), pref->priv->notify_text_handler);
	
	if (!gtk_toggle_button_get_active (button))
	{
		xpad_settings_set_text_color (xpad_settings (), NULL);
		xpad_settings_set_back_color (xpad_settings (), NULL);
	}
	else
	{
		GdkColor color;
		gtk_color_button_get_color (GTK_COLOR_BUTTON (pref->priv->textbutton), &color);
		xpad_settings_set_text_color (xpad_settings (), &color);
		gtk_color_button_get_color (GTK_COLOR_BUTTON (pref->priv->backbutton), &color);
		xpad_settings_set_back_color (xpad_settings (), &color);
	}
	
	gtk_widget_set_sensitive (pref->priv->colorbox, gtk_toggle_button_get_active (button));
	
	g_signal_handler_unblock (xpad_settings (), pref->priv->notify_back_handler);
	g_signal_handler_unblock (xpad_settings (), pref->priv->notify_text_handler);
}

static void
change_font_check (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (xpad_settings (), pref->priv->notify_font_handler);
	
	if (!gtk_toggle_button_get_active (button))
		xpad_settings_set_fontname (xpad_settings (), NULL);
	else
		xpad_settings_set_fontname (xpad_settings (), gtk_font_button_get_font_name (GTK_FONT_BUTTON (pref->priv->fontbutton)));
	
	gtk_widget_set_sensitive (pref->priv->fontbutton, gtk_toggle_button_get_active (button));
	
	g_signal_handler_unblock (xpad_settings (), pref->priv->notify_font_handler);
}

static void
change_edit_check (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (xpad_settings (), pref->priv->notify_edit_handler);
	xpad_settings_set_edit_lock (xpad_settings (), gtk_toggle_button_get_active (button));
	g_signal_handler_unblock (xpad_settings (), pref->priv->notify_edit_handler);
}

static void
change_sticky_check (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (xpad_settings (), pref->priv->notify_sticky_handler);
	xpad_settings_set_sticky (xpad_settings (), gtk_toggle_button_get_active (button));
	g_signal_handler_unblock (xpad_settings (), pref->priv->notify_sticky_handler);
}

static void
change_confirm_check (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (xpad_settings (), pref->priv->notify_confirm_handler);
	xpad_settings_set_confirm_destroy (xpad_settings (), gtk_toggle_button_get_active (button));
	g_signal_handler_unblock (xpad_settings (), pref->priv->notify_confirm_handler);
}

static void
change_text_color (GtkColorButton *button, XpadPreferences *pref)
{
	GdkColor color;
	gtk_color_button_get_color (button, &color);
	g_signal_handler_block (xpad_settings (), pref->priv->notify_text_handler);
	xpad_settings_set_text_color (xpad_settings (), &color);
	g_signal_handler_unblock (xpad_settings (), pref->priv->notify_text_handler);
}

static void
change_back_color (GtkColorButton *button, XpadPreferences *pref)
{
	GdkColor color;
	gtk_color_button_get_color (button, &color);
	g_signal_handler_block (xpad_settings (), pref->priv->notify_back_handler);
	xpad_settings_set_back_color (xpad_settings (), &color);
	g_signal_handler_unblock (xpad_settings (), pref->priv->notify_back_handler);
}

static void
change_font_face (GtkFontButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (xpad_settings (), pref->priv->notify_font_handler);
	xpad_settings_set_fontname (xpad_settings (), gtk_font_button_get_font_name (button));
	g_signal_handler_unblock (xpad_settings (), pref->priv->notify_font_handler);
}

static void
notify_back_color (XpadPreferences *pref)
{
	const GdkColor *color = xpad_settings_get_back_color (xpad_settings ());
	
	g_signal_handler_block (pref->priv->backbutton, pref->priv->back_handler);
	g_signal_handler_block (pref->priv->colorcheck, pref->priv->colorcheck_handler);
	
	if (color)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->colorcheck), TRUE);
		gtk_widget_set_sensitive (pref->priv->colorbox, TRUE);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (pref->priv->backbutton), color);
	}
	else
	{
		gtk_widget_set_sensitive (pref->priv->colorbox, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->anticolorcheck), TRUE);
	}
	
	g_signal_handler_unblock (pref->priv->colorcheck, pref->priv->colorcheck_handler);
	g_signal_handler_unblock (pref->priv->backbutton, pref->priv->back_handler);
}

static void
notify_text_color (XpadPreferences *pref)
{
	const GdkColor *color = xpad_settings_get_text_color (xpad_settings ());
	
	g_signal_handler_block (pref->priv->textbutton, pref->priv->text_handler);
	g_signal_handler_block (pref->priv->colorcheck, pref->priv->colorcheck_handler);
	
	if (color)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->colorcheck), TRUE);
		gtk_widget_set_sensitive (pref->priv->colorbox, TRUE);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (pref->priv->textbutton), color);
	}
	else
	{
		gtk_widget_set_sensitive (pref->priv->colorbox, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->anticolorcheck), TRUE);
	}
	
	g_signal_handler_unblock (pref->priv->colorcheck, pref->priv->colorcheck_handler);
	g_signal_handler_unblock (pref->priv->textbutton, pref->priv->text_handler);
}

static void
notify_fontname (XpadPreferences *pref)
{
	const gchar *fontname = xpad_settings_get_fontname (xpad_settings ());
	
	g_signal_handler_block (pref->priv->fontbutton, pref->priv->font_handler);
	g_signal_handler_block (pref->priv->fontcheck, pref->priv->fontcheck_handler);
	
	if (fontname)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->fontcheck), TRUE);
		gtk_widget_set_sensitive (pref->priv->fontbutton, TRUE);
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (pref->priv->fontbutton), fontname);
	}
	else
	{
		gtk_widget_set_sensitive (pref->priv->fontbutton, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->antifontcheck), TRUE);
	}
	
	g_signal_handler_unblock (pref->priv->fontcheck, pref->priv->fontcheck_handler);
	g_signal_handler_unblock (pref->priv->fontbutton, pref->priv->font_handler);
}

static void
notify_edit (XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->editcheck, pref->priv->editcheck_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->editcheck), xpad_settings_get_edit_lock (xpad_settings ()));
	g_signal_handler_unblock (pref->priv->editcheck, pref->priv->editcheck_handler);
}

static void
notify_sticky (XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->stickycheck, pref->priv->stickycheck_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->stickycheck), xpad_settings_get_sticky (xpad_settings ()));
	g_signal_handler_unblock (pref->priv->stickycheck, pref->priv->stickycheck_handler);
}

static void
notify_confirm (XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->confirmcheck, pref->priv->confirmcheck_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->confirmcheck), xpad_settings_get_confirm_destroy (xpad_settings ()));
	g_signal_handler_unblock (pref->priv->confirmcheck, pref->priv->confirmcheck_handler);
}
