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
#include "xpad-pad-properties.h"

G_DEFINE_TYPE(XpadPadProperties, xpad_pad_properties, GTK_TYPE_DIALOG)
#define XPAD_PAD_PROPERTIES_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), XPAD_TYPE_PAD_PROPERTIES, XpadPadPropertiesPrivate))

struct XpadPadPropertiesPrivate 
{
	GtkWidget *fontcheck;
	GtkWidget *colorcheck;
	GtkWidget *colorbox;
	
	GtkWidget *textbutton;
	GdkColor texttmp;
	
	GtkWidget *backbutton;
	GdkColor backtmp;
	
	GtkWidget *fontbutton;
};

static void xpad_pad_properties_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_pad_properties_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xpad_pad_properties_response (GtkDialog *dialog, gint response);
static void change_color_check (GtkToggleButton *button, XpadPadProperties *prop);
static void change_font_check (GtkToggleButton *button, XpadPadProperties *prop);
static void change_text_color (GtkColorButton *button, XpadPadProperties *prop);
static void change_back_color (GtkColorButton *button, XpadPadProperties *prop);
static void change_font_face (GtkFontButton *button, XpadPadProperties *prop);

enum
{
  PROP_0,
  PROP_FOLLOW_FONT_STYLE,
  PROP_FOLLOW_COLOR_STYLE,
  PROP_BACK_COLOR,
  PROP_TEXT_COLOR,
  PROP_FONTNAME,
  LAST_PROP
};

GtkWidget *
xpad_pad_properties_new (void)
{
	return g_object_new (XPAD_TYPE_PAD_PROPERTIES, NULL);
}

static void
xpad_pad_properties_class_init (XpadPadPropertiesClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->set_property = xpad_pad_properties_set_property;
	gobject_class->get_property = xpad_pad_properties_get_property;
	
	/* Properties */
	
	g_object_class_install_property (gobject_class,
	                                 PROP_FOLLOW_FONT_STYLE,
	                                 g_param_spec_boolean ("follow-font-style",
	                                                       "Follow Font Style",
	                                                       "Whether to use the default xpad font style",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_FOLLOW_COLOR_STYLE,
	                                 g_param_spec_boolean ("follow-color-style",
	                                                       "Follow Color Style",
	                                                       "Whether to use the default xpad color style",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_TEXT_COLOR,
	                                 g_param_spec_boxed ("text-color",
	                                                     "Text Color",
	                                                     "The color of text in the pad",
	                                                     GDK_TYPE_COLOR,
	                                                     G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_BACK_COLOR,
	                                 g_param_spec_boxed ("back-color",
	                                                     "Back Color",
	                                                     "The color of the background in the pad",
	                                                     GDK_TYPE_COLOR,
	                                                     G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_FONTNAME,
	                                 g_param_spec_string ("fontname",
	                                                      "Font Name",
	                                                      "The name of the font for the pad",
	                                                      NULL,
	                                                      G_PARAM_READWRITE));
	
	g_type_class_add_private (gobject_class, sizeof (XpadPadPropertiesPrivate));
}

static void
xpad_pad_properties_init (XpadPadProperties *prop)
{
	GtkWidget *font_radio, *color_radio, *hbox, *font_hbox, *vbox;
	GtkWidget *label, *appearance_frame, *alignment, *appearance_vbox;
	gchar *text;
	GtkSizeGroup *size_group_labels = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	
	prop->priv = XPAD_PAD_PROPERTIES_GET_PRIVATE (prop);
	
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
		"border-width", 6,
		NULL));
	
	prop->priv->textbutton = gtk_color_button_new ();
	prop->priv->backbutton = gtk_color_button_new ();
	prop->priv->fontbutton = gtk_font_button_new ();
	
	font_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Use font from xpad preferences"));
	prop->priv->fontcheck = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (font_radio), _("Use this font:"));
	color_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Use colors from xpad preferences"));
	prop->priv->colorcheck = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (color_radio), _("Use these colors:"));
	
	font_hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (font_hbox), prop->priv->fontcheck, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (font_hbox), prop->priv->fontbutton, TRUE, TRUE, 0);
	
	prop->priv->colorbox = gtk_vbox_new (FALSE, 6);
	hbox = gtk_hbox_new (FALSE, 12);
	label = gtk_label_new_with_mnemonic (_("Background:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_size_group_add_widget (size_group_labels, label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), prop->priv->backbutton, TRUE, TRUE, 0);
	g_object_set (G_OBJECT (prop->priv->colorbox), "child", hbox, NULL);
	
	hbox = gtk_hbox_new (FALSE, 12);
	label = gtk_label_new_with_mnemonic (_("Foreground:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_size_group_add_widget (size_group_labels, label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), prop->priv->textbutton, TRUE, TRUE, 0);
	g_object_set (G_OBJECT (prop->priv->colorbox), "child", hbox, NULL);
	
	alignment = gtk_alignment_new (1, 1, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);
	gtk_container_add (GTK_CONTAINER (alignment), prop->priv->colorbox);
	
	
	gtk_dialog_add_button (GTK_DIALOG (prop), "gtk-close", GTK_RESPONSE_CLOSE);
	gtk_dialog_set_default_response (GTK_DIALOG (prop), GTK_RESPONSE_CLOSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (prop), FALSE);
	g_signal_connect (prop, "response", G_CALLBACK (xpad_pad_properties_response), NULL);
	
	gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (prop->priv->textbutton), FALSE);
	gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (prop->priv->backbutton), FALSE);
	
	gtk_color_button_set_title (GTK_COLOR_BUTTON (prop->priv->textbutton), _("Set Foreground Color"));
	gtk_color_button_set_title (GTK_COLOR_BUTTON (prop->priv->backbutton), _("Set Background Color"));
	gtk_font_button_set_title (GTK_FONT_BUTTON (prop->priv->fontbutton), _("Set Font"));
	
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), font_radio, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), font_hbox, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (appearance_vbox), vbox, FALSE, FALSE, 0);
	
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), color_radio, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), prop->priv->colorcheck, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (appearance_vbox), vbox, FALSE, FALSE, 0);
	
	g_signal_connect (prop->priv->colorcheck, "toggled", G_CALLBACK (change_color_check), prop);
	g_signal_connect (prop->priv->fontcheck, "toggled", G_CALLBACK (change_font_check), prop);
	g_signal_connect (prop->priv->textbutton, "color-set", G_CALLBACK (change_text_color), prop);
	g_signal_connect (prop->priv->backbutton, "color-set", G_CALLBACK (change_back_color), prop);
	g_signal_connect (prop->priv->fontbutton, "font-set", G_CALLBACK (change_font_face), prop);
	
	/* Setup initial state, which should never be seen, but just in case client doesn't set them
	   itself, we'll be consistent. */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (font_radio), TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (color_radio), TRUE);
	gtk_widget_set_sensitive (prop->priv->colorbox, FALSE);
	gtk_widget_set_sensitive (prop->priv->fontbutton, FALSE);
	
	g_object_unref (size_group_labels);
	
	g_object_set (G_OBJECT (GTK_DIALOG (prop)->vbox),
		"child", appearance_frame,
		NULL);
	gtk_widget_show_all (GTK_DIALOG (prop)->vbox);
}

static void
xpad_pad_properties_response (GtkDialog *dialog, gint response)
{
	if (response == GTK_RESPONSE_CLOSE)
		gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
change_color_check (GtkToggleButton *button, XpadPadProperties *prop)
{
	gtk_widget_set_sensitive (prop->priv->colorbox, gtk_toggle_button_get_active (button));
	
	g_object_notify (G_OBJECT (prop), "follow-color-style");
}

static void
change_font_check (GtkToggleButton *button, XpadPadProperties *prop)
{
	gtk_widget_set_sensitive (prop->priv->fontbutton, gtk_toggle_button_get_active (button));
	
	g_object_notify (G_OBJECT (prop), "follow-font-style");
}

static void
change_text_color (GtkColorButton *button, XpadPadProperties *prop)
{
	g_object_notify (G_OBJECT (prop), "text-color");
}

static void
change_back_color (GtkColorButton *button, XpadPadProperties *prop)
{
	g_object_notify (G_OBJECT (prop), "back-color");
}

static void
change_font_face (GtkFontButton *button, XpadPadProperties *prop)
{
	g_object_notify (G_OBJECT (prop), "fontname");
}

void
xpad_pad_properties_set_follow_font_style (XpadPadProperties *prop, gboolean follow)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prop->priv->fontcheck), !follow);
	
	g_object_notify (G_OBJECT (prop), "follow_font_style");
}

gboolean
xpad_pad_properties_get_follow_font_style (XpadPadProperties *prop)
{
	return !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prop->priv->fontcheck));
}

void
xpad_pad_properties_set_follow_color_style (XpadPadProperties *prop, gboolean follow)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prop->priv->colorcheck), !follow);
	
	g_object_notify (G_OBJECT (prop), "follow_color_style");
}

gboolean
xpad_pad_properties_get_follow_color_style (XpadPadProperties *prop)
{
	return !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prop->priv->colorcheck));
}

void
xpad_pad_properties_set_back_color (XpadPadProperties *prop, const GdkColor *back)
{
	gtk_color_button_set_color (GTK_COLOR_BUTTON (prop->priv->backbutton), back);
	
	g_object_notify (G_OBJECT (prop), "back_color");
}

G_CONST_RETURN GdkColor *
xpad_pad_properties_get_back_color (XpadPadProperties *prop)
{
	gtk_color_button_get_color (GTK_COLOR_BUTTON (prop->priv->backbutton), &prop->priv->backtmp);
	return &prop->priv->backtmp;
}

void
xpad_pad_properties_set_text_color (XpadPadProperties *prop, const GdkColor *text)
{
	gtk_color_button_set_color (GTK_COLOR_BUTTON (prop->priv->textbutton), text);
	
	g_object_notify (G_OBJECT (prop), "text_color");
}

G_CONST_RETURN GdkColor *
xpad_pad_properties_get_text_color (XpadPadProperties *prop)
{
	gtk_color_button_get_color (GTK_COLOR_BUTTON (prop->priv->textbutton), &prop->priv->texttmp);
	return &prop->priv->texttmp;
}

void
xpad_pad_properties_set_fontname (XpadPadProperties *prop, const gchar *fontname)
{
	gtk_font_button_set_font_name (GTK_FONT_BUTTON (prop->priv->fontbutton), fontname);
	
	g_object_notify (G_OBJECT (prop), "fontname");
}

G_CONST_RETURN gchar *xpad_pad_properties_get_fontname (XpadPadProperties *prop)
{
	return gtk_font_button_get_font_name (GTK_FONT_BUTTON (prop->priv->fontbutton));
}

static void
xpad_pad_properties_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadPadProperties *prop;
	
	prop = XPAD_PAD_PROPERTIES (object);
	
	switch (prop_id)
	{
	case PROP_FOLLOW_FONT_STYLE:
		xpad_pad_properties_set_follow_font_style (prop, g_value_get_boolean (value));
		break;
	
	case PROP_FOLLOW_COLOR_STYLE:
		xpad_pad_properties_set_follow_color_style (prop, g_value_get_boolean (value));
		break;
	
	case PROP_BACK_COLOR:
		xpad_pad_properties_set_back_color (prop, g_value_get_boxed (value));
		break;
	
	case PROP_TEXT_COLOR:
		xpad_pad_properties_set_text_color (prop, g_value_get_boxed (value));
		break;
	
	case PROP_FONTNAME:
		xpad_pad_properties_set_fontname (prop, g_value_get_string (value));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xpad_pad_properties_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadPadProperties *prop;
	
	prop = XPAD_PAD_PROPERTIES (object);
	
	switch (prop_id)
	{
	case PROP_FOLLOW_FONT_STYLE:
		g_value_set_boolean (value, xpad_pad_properties_get_follow_font_style (prop));
		break;
	
	case PROP_FOLLOW_COLOR_STYLE:
		g_value_set_boolean (value, xpad_pad_properties_get_follow_color_style (prop));
		break;
	
	case PROP_BACK_COLOR:
		g_value_set_static_boxed (value, xpad_pad_properties_get_back_color (prop));
		break;
	
	case PROP_TEXT_COLOR:
		g_value_set_static_boxed (value, xpad_pad_properties_get_text_color (prop));
		break;
	
	case PROP_FONTNAME:
		g_value_set_string (value, xpad_pad_properties_get_fontname (prop));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}
