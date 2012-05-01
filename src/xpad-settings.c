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

#include <string.h>
#include "xpad-settings.h"
#include "fio.h"

G_DEFINE_TYPE(XpadSettings, xpad_settings, G_TYPE_OBJECT)
#define XPAD_SETTINGS_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), XPAD_TYPE_SETTINGS, XpadSettingsPrivate))

#define DEFAULTS_FILENAME	"default-style"

struct XpadSettingsPrivate 
{
	guint width;
	guint height;
	gboolean has_decorations;
	gboolean confirm_destroy;
	gboolean edit_lock;
	gboolean sticky;
	gboolean has_toolbar;
	gboolean autohide_toolbar;
	gboolean has_scrollbar;
	GdkColor *back;
	GdkColor *text;
	gchar *fontname;
	GSList *toolbar_buttons;
};

enum
{
	CHANGE_BUTTONS,
	LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_HAS_DECORATIONS,
  PROP_CONFIRM_DESTROY,
  PROP_STICKY,
  PROP_EDIT_LOCK,
  PROP_HAS_TOOLBAR,
  PROP_AUTOHIDE_TOOLBAR,
  PROP_HAS_SCROLLBAR,
  PROP_BACK_COLOR,
  PROP_TEXT_COLOR,
  PROP_FONTNAME,
  LAST_PROP
};

static void load_from_file (XpadSettings *settings, const gchar *filename);
static void save_to_file (XpadSettings *settings, const gchar *filename);
static void xpad_settings_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_settings_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xpad_settings_finalize (GObject *object);

static XpadSettings *_xpad_settings = NULL;
static guint signals[LAST_SIGNAL] = { 0 };

XpadSettings *
xpad_settings (void)
{
	/* Singleton class */
	if (!_xpad_settings)
		_xpad_settings = XPAD_SETTINGS (g_object_new (XPAD_TYPE_SETTINGS, NULL));
	
	return _xpad_settings;
}

static void
xpad_settings_class_init (XpadSettingsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->finalize = xpad_settings_finalize;
	gobject_class->set_property = xpad_settings_set_property;
	gobject_class->get_property = xpad_settings_get_property;
	
	/* Properties */
	
	g_object_class_install_property (gobject_class,
	                                 PROP_WIDTH,
	                                 g_param_spec_uint ("width",
	                                                    "Default Width of Pads",
	                                                    "Window width of pads on creation",
	                                                    0,
	                                                    G_MAXUINT,
	                                                    200,
	                                                    G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_HEIGHT,
	                                 g_param_spec_uint ("height",
	                                                    "Default Height of Pads",
	                                                    "Window height of pads on creation",
	                                                    0,
	                                                    G_MAXUINT,
	                                                    200,
	                                                    G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_HAS_DECORATIONS,
	                                 g_param_spec_boolean ("has_decorations",
	                                                       "Has Decorations",
	                                                       "Whether pads have window decorations",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_CONFIRM_DESTROY,
	                                 g_param_spec_boolean ("confirm_destroy",
	                                                       "Confirm Destroy",
	                                                       "Whether destroying a pad requires user confirmation",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_STICKY,
	                                 g_param_spec_boolean ("sticky",
	                                                       "Default Stickiness",
	                                                       "Whether pads are sticky on creation",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_EDIT_LOCK,
	                                 g_param_spec_boolean ("edit_lock",
	                                                       "Edit Lock",
	                                                       "Whether edit lock mode is enabled",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_HAS_TOOLBAR,
	                                 g_param_spec_boolean ("has_toolbar",
	                                                       "Has Toolbar",
	                                                       "Whether pads have toolbars",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_AUTOHIDE_TOOLBAR,
	                                 g_param_spec_boolean ("autohide_toolbar",
	                                                       "Autohide Toolbar",
	                                                       "Whether toolbars hide when not used",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_HAS_SCROLLBAR,
	                                 g_param_spec_boolean ("has_scrollbar",
	                                                       "Has Scrollbar",
	                                                       "Whether pads have scrollbars",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_FONTNAME,
	                                 g_param_spec_string ("fontname",
	                                                      "Font Name",
	                                                      "Default name of pad font",
	                                                      NULL,
	                                                      G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_TEXT_COLOR,
	                                 g_param_spec_boxed ("text_color",
	                                                     "Text Color",
	                                                     "Default color of pad text",
	                                                     GDK_TYPE_COLOR,
	                                                     G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class,
	                                 PROP_BACK_COLOR,
	                                 g_param_spec_boxed ("back_color",
	                                                     "Back Color",
	                                                     "Default color of pad background",
	                                                     GDK_TYPE_COLOR,
	                                                     G_PARAM_READWRITE));
	
	/* Signals */
	
	signals[CHANGE_BUTTONS] = 
		g_signal_new ("change_buttons",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (XpadSettingsClass, change_buttons),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	g_type_class_add_private (gobject_class, sizeof (XpadSettingsPrivate));
}

static void
xpad_settings_init (XpadSettings *settings)
{
	GdkColor back, text;
	
	settings->priv = XPAD_SETTINGS_GET_PRIVATE (settings);
	
	/* A pleasant light yellow color, similar to 
	   commercial sticky notes. */
	back.pixel = 0;
	back.red = 65535;
	back.green = 61166;
	back.blue = 39321;
	settings->priv->back = gdk_color_copy (&back);
	
	/* Black */
	text.pixel = 0;
	text.red = 0;
	text.green = 0;
	text.blue = 0;
	settings->priv->text = gdk_color_copy (&text);
	
	settings->priv->width = 200;
	settings->priv->height = 200;
	settings->priv->has_decorations = TRUE;
	settings->priv->confirm_destroy = TRUE;
	settings->priv->sticky = FALSE;
	settings->priv->edit_lock = FALSE;
	settings->priv->fontname = NULL;
	settings->priv->has_toolbar = TRUE;
	settings->priv->autohide_toolbar = TRUE;
	settings->priv->has_scrollbar = TRUE;
	
	settings->priv->toolbar_buttons = NULL;
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("New"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Delete"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Close"));
	
	load_from_file (settings, DEFAULTS_FILENAME);
}

static void
xpad_settings_finalize (GObject *object)
{
	XpadSettings *settings = XPAD_SETTINGS (object);
	
	g_slist_free (settings->priv->toolbar_buttons);
	gdk_color_free (settings->priv->text);
	gdk_color_free (settings->priv->back);
	g_free (settings->priv->fontname);
	
	G_OBJECT_CLASS (xpad_settings_parent_class)->finalize (object);
}

void xpad_settings_set_width (XpadSettings *settings, guint width)
{
	if (settings->priv->width == width)
		return;
	
	settings->priv->width = width;
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_object_notify (G_OBJECT (settings), "width");
}

guint xpad_settings_get_width (XpadSettings *settings)
{
	return settings->priv->width;
}

void xpad_settings_set_height (XpadSettings *settings, guint height)
{
	if (settings->priv->height == height)
		return;
	
	settings->priv->height = height;
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_object_notify (G_OBJECT (settings), "height");
}

guint xpad_settings_get_height (XpadSettings *settings)
{
	return settings->priv->height;
}

void xpad_settings_set_has_decorations (XpadSettings *settings, gboolean decorations)
{
	if (settings->priv->has_decorations == decorations)
		return;
	
	settings->priv->has_decorations = decorations;
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_object_notify (G_OBJECT (settings), "has_decorations");
}

gboolean xpad_settings_get_has_decorations (XpadSettings *settings)
{
	return settings->priv->has_decorations;
}

void xpad_settings_set_confirm_destroy (XpadSettings *settings, gboolean confirm)
{
	if (settings->priv->confirm_destroy == confirm)
		return;
	
	settings->priv->confirm_destroy = confirm;
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_object_notify (G_OBJECT (settings), "confirm_destroy");
}

gboolean xpad_settings_get_confirm_destroy (XpadSettings *settings)
{
	return settings->priv->confirm_destroy;
}

void xpad_settings_set_edit_lock (XpadSettings *settings, gboolean lock)
{
	if (settings->priv->edit_lock == lock)
		return;
	
	settings->priv->edit_lock = lock;
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_object_notify (G_OBJECT (settings), "edit_lock");
}

gboolean xpad_settings_get_edit_lock (XpadSettings *settings)
{
	return settings->priv->edit_lock;
}

void xpad_settings_set_has_toolbar (XpadSettings *settings, gboolean toolbar)
{
	if (settings->priv->has_toolbar == toolbar)
		return;
	
	settings->priv->has_toolbar = toolbar;
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_object_notify (G_OBJECT (settings), "has_toolbar");
}

gboolean xpad_settings_get_has_toolbar (XpadSettings *settings)
{
	return settings->priv->has_toolbar;
}

void xpad_settings_set_sticky (XpadSettings *settings, gboolean sticky)
{
	if (settings->priv->sticky == sticky)
		return;
	
	settings->priv->sticky = sticky;
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_object_notify (G_OBJECT (settings), "sticky");
}

gboolean xpad_settings_get_sticky (XpadSettings *settings)
{
	return settings->priv->sticky;
}

void xpad_settings_set_autohide_toolbar (XpadSettings *settings, gboolean hide)
{
	if (settings->priv->autohide_toolbar == hide)
		return;
	
	settings->priv->autohide_toolbar = hide;
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_object_notify (G_OBJECT (settings), "autohide_toolbar");
}

gboolean xpad_settings_get_autohide_toolbar (XpadSettings *settings)
{
	return settings->priv->autohide_toolbar;
}

void xpad_settings_set_has_scrollbar (XpadSettings *settings, gboolean scrollbar)
{
	if (settings->priv->has_scrollbar == scrollbar)
		return;
	
	settings->priv->has_scrollbar = scrollbar;
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_object_notify (G_OBJECT (settings), "has_scrollbar");
}

gboolean xpad_settings_get_has_scrollbar (XpadSettings *settings)
{
	return settings->priv->has_scrollbar;
}

void xpad_settings_add_toolbar_button (XpadSettings *settings, const gchar *button)
{
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup (button));
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_signal_emit (settings, signals[CHANGE_BUTTONS], 0);
}

gboolean xpad_settings_move_toolbar_button (XpadSettings *settings, gint button, gint new)
{
	GSList *element;
	gpointer data;
	
	if (button == new)
		return FALSE;
	
	if (new >= g_slist_length (settings->priv->toolbar_buttons) || 0 > new )
		return FALSE;
	
	element = g_slist_nth (settings->priv->toolbar_buttons, button);
	if (!element)
		return FALSE;
	
	data = element->data;
	settings->priv->toolbar_buttons = g_slist_delete_link (settings->priv->toolbar_buttons, element);
	settings->priv->toolbar_buttons = g_slist_insert (settings->priv->toolbar_buttons, data, new);
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_signal_emit (settings, signals[CHANGE_BUTTONS], 0);
	
	return TRUE;
}

static void xpad_settings_remove_toolbar_list_element (XpadSettings *settings, GSList *element)
{
	g_free (element->data);
	settings->priv->toolbar_buttons = g_slist_delete_link (settings->priv->toolbar_buttons, element);
}

gboolean xpad_settings_remove_toolbar_button (XpadSettings *settings, gint button)
{
	GSList *element;
	
	element = g_slist_nth (settings->priv->toolbar_buttons, button);
	
	if (!element)
		return FALSE;
	
	xpad_settings_remove_toolbar_list_element (settings, element);
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_signal_emit (settings, signals[CHANGE_BUTTONS], 0);
	
	return TRUE;
}

gboolean xpad_settings_remove_all_toolbar_buttons (XpadSettings *settings)
{
	if (settings->priv->toolbar_buttons == NULL)
		return FALSE;

	while (settings->priv->toolbar_buttons)
	{
		g_free (settings->priv->toolbar_buttons->data);
		settings->priv->toolbar_buttons = 
			g_slist_delete_link (settings->priv->toolbar_buttons,
					settings->priv->toolbar_buttons);
	}

	settings->priv->toolbar_buttons = NULL;

	g_signal_emit (settings, signals[CHANGE_BUTTONS], 0);

	return TRUE;
}

gboolean xpad_settings_remove_last_toolbar_button (XpadSettings *settings)
{
	GSList *element;
	
	element = g_slist_last (settings->priv->toolbar_buttons);
	
	if (!element)
		return FALSE;
	
	xpad_settings_remove_toolbar_list_element (settings, element);
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_signal_emit (settings, signals[CHANGE_BUTTONS], 0);
	
	return TRUE;
}

G_CONST_RETURN GSList *xpad_settings_get_toolbar_buttons (XpadSettings *settings)
{
	return settings->priv->toolbar_buttons;
}

void xpad_settings_set_back_color (XpadSettings *settings, const GdkColor *back)
{
	if (settings->priv->back)
		gdk_color_free (settings->priv->back);
	
	if (back)
		settings->priv->back = gdk_color_copy (back);
	else
		settings->priv->back = NULL;
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_object_notify (G_OBJECT (settings), "back_color");
}

G_CONST_RETURN GdkColor *xpad_settings_get_back_color (XpadSettings *settings)
{
	return settings->priv->back;
}

void xpad_settings_set_text_color (XpadSettings *settings, const GdkColor *text)
{
	if (settings->priv->text)
		gdk_color_free (settings->priv->text);
	
	if (text)
		settings->priv->text = gdk_color_copy (text);
	else
		settings->priv->text = NULL;
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_object_notify (G_OBJECT (settings), "text_color");
}

G_CONST_RETURN GdkColor *xpad_settings_get_text_color (XpadSettings *settings)
{
	return settings->priv->text;
}

void xpad_settings_set_fontname (XpadSettings *settings, const gchar *fontname)
{
	g_free (settings->priv->fontname);
	settings->priv->fontname = g_strdup (fontname);
	
	save_to_file (settings, DEFAULTS_FILENAME);
	
	g_object_notify (G_OBJECT (settings), "fontname");
}

G_CONST_RETURN gchar *xpad_settings_get_fontname (XpadSettings *settings)
{
	return settings->priv->fontname;
}

static void
xpad_settings_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadSettings *settings;
	
	settings = XPAD_SETTINGS (object);
	
	switch (prop_id)
	{
	case PROP_HEIGHT:
		xpad_settings_set_height (settings, g_value_get_uint (value));
		break;
	
	case PROP_WIDTH:
		xpad_settings_set_width (settings, g_value_get_uint (value));
		break;
	
	case PROP_HAS_DECORATIONS:
		xpad_settings_set_has_decorations (settings, g_value_get_boolean (value));
		break;
	
	case PROP_CONFIRM_DESTROY:
		xpad_settings_set_confirm_destroy (settings, g_value_get_boolean (value));
		break;
	
	case PROP_STICKY:
		xpad_settings_set_sticky (settings, g_value_get_boolean (value));
		break;
	
	case PROP_EDIT_LOCK:
		xpad_settings_set_edit_lock (settings, g_value_get_boolean (value));
		break;
	
	case PROP_HAS_TOOLBAR:
		xpad_settings_set_has_toolbar (settings, g_value_get_boolean (value));
		break;
	
	case PROP_AUTOHIDE_TOOLBAR:
		xpad_settings_set_autohide_toolbar (settings, g_value_get_boolean (value));
		break;
	
	case PROP_HAS_SCROLLBAR:
		xpad_settings_set_has_scrollbar (settings, g_value_get_boolean (value));
		break;
	
	case PROP_BACK_COLOR:
		xpad_settings_set_back_color (settings, g_value_get_boxed (value));
		break;
	
	case PROP_TEXT_COLOR:
		xpad_settings_set_text_color (settings, g_value_get_boxed (value));
		break;
	
	case PROP_FONTNAME:
		xpad_settings_set_fontname (settings, g_value_get_string (value));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xpad_settings_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadSettings *settings;
	
	settings = XPAD_SETTINGS (object);
	
	switch (prop_id)
	{
	case PROP_HEIGHT:
		g_value_set_uint (value, xpad_settings_get_height (settings));
		break;
	
	case PROP_WIDTH:
		g_value_set_uint (value, xpad_settings_get_width (settings));
		break;
	
	case PROP_HAS_DECORATIONS:
		g_value_set_boolean (value, xpad_settings_get_has_decorations (settings));
		break;
	
	case PROP_CONFIRM_DESTROY:
		g_value_set_boolean (value, xpad_settings_get_confirm_destroy (settings));
		break;
	
	case PROP_STICKY:
		g_value_set_boolean (value, xpad_settings_get_sticky (settings));
		break;
	
	case PROP_EDIT_LOCK:
		g_value_set_boolean (value, xpad_settings_get_edit_lock (settings));
		break;
	
	case PROP_HAS_TOOLBAR:
		g_value_set_boolean (value, xpad_settings_get_has_toolbar (settings));
		break;
	
	case PROP_AUTOHIDE_TOOLBAR:
		g_value_set_boolean (value, xpad_settings_get_autohide_toolbar (settings));
		break;
	
	case PROP_HAS_SCROLLBAR:
		g_value_set_boolean (value, xpad_settings_get_has_scrollbar (settings));
		break;
	
	case PROP_BACK_COLOR:
		g_value_set_static_boxed (value, xpad_settings_get_back_color (settings));
		break;
	
	case PROP_TEXT_COLOR:
		g_value_set_static_boxed (value, xpad_settings_get_text_color (settings));
		break;
	
	case PROP_FONTNAME:
		g_value_set_string (value, xpad_settings_get_fontname (settings));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
load_from_file (XpadSettings *settings, const gchar *filename)
{
	/**
	 * We need to set up int values for all these to take the value from the file.
	 * These will be assigned back to the appropriate values after load.
	 */
	gchar *buttons = NULL;
	GdkColor text = {0}, back = {0};
	gboolean use_text, use_back;
	
	use_text = settings->priv->text ? TRUE : FALSE;
	if (settings->priv->text)
		text = *settings->priv->text;
	
	use_back = settings->priv->back ? TRUE : FALSE;
	if (settings->priv->back)
		back = *settings->priv->back;
	
	if (fio_get_values_from_file (filename, 
		"b|decorations", &settings->priv->has_decorations,
		"u|height", &settings->priv->height,
		"u|width", &settings->priv->width,
		"b|confirm_destroy", &settings->priv->confirm_destroy,
		"b|edit_lock", &settings->priv->edit_lock,
		"b|sticky_on_start", &settings->priv->sticky,
		"h|back_red", &back.red,
		"h|back_green", &back.green,
		"h|back_blue", &back.blue,
		"b|use_back", &use_back,
		"h|text_red", &text.red,
		"h|text_green", &text.green,
		"h|text_blue", &text.blue,
		"b|use_text", &use_text,
		"s|fontname", &settings->priv->fontname,
		"b|toolbar", &settings->priv->has_toolbar,
		"b|auto_hide_toolbar", &settings->priv->autohide_toolbar,
		"b|scrollbar", &settings->priv->has_scrollbar,
		"s|buttons", &buttons,
		NULL))
		return;
	
	if (use_text)
	{
		gdk_color_free (settings->priv->text);
		settings->priv->text = gdk_color_copy (&text);
	}
	
	if (use_back)
	{
		gdk_color_free (settings->priv->back);
		settings->priv->back = gdk_color_copy (&back);
	}
	
	if (settings->priv->fontname &&
	    strcmp (settings->priv->fontname, "NULL") == 0)
	{
		g_free (settings->priv->fontname);
		settings->priv->fontname = NULL;
	}
	
	if (buttons)
	{
		gint i;
		gchar **button_names;
		
		button_names = g_strsplit (buttons, ",", 0);
		
		while (settings->priv->toolbar_buttons)
		{
			g_free (settings->priv->toolbar_buttons->data);
			settings->priv->toolbar_buttons = 
				g_slist_delete_link (settings->priv->toolbar_buttons,
				settings->priv->toolbar_buttons);
		}
		
		for (i = 0; button_names[i]; ++i)
		{
			settings->priv->toolbar_buttons = 
				g_slist_append (settings->priv->toolbar_buttons,
				g_strstrip (button_names[i])); /* takes ownership of string */
		}
		
		g_free (button_names);
		g_free (buttons);
	}
}


static void
save_to_file (XpadSettings *settings, const gchar *filename)
{
	gchar *buttons = g_strdup ("");
	GSList *tmp;
	
	tmp = settings->priv->toolbar_buttons;
	
	while (tmp)
	{
		gchar *tmpstr = buttons;
		
		if (tmp->next)
			buttons = g_strconcat (buttons, tmp->data, ", ", NULL);
		else
			buttons = g_strconcat (buttons, tmp->data, NULL);
		
		g_free (tmpstr);
		tmp = tmp->next;
	}
	
	fio_set_values_to_file (filename, 
		"b|decorations", settings->priv->has_decorations,
		"u|height", settings->priv->height,
		"u|width", settings->priv->width,
		"b|confirm_destroy", settings->priv->confirm_destroy,
		"b|edit_lock", settings->priv->edit_lock,
		"b|sticky_on_start", settings->priv->sticky,
		"h|back_red", settings->priv->back ? settings->priv->back->red : 0,
		"h|back_green", settings->priv->back ? settings->priv->back->green : 0,
		"h|back_blue", settings->priv->back ? settings->priv->back->blue : 0,
		"b|use_back", settings->priv->back ? TRUE : FALSE,
		"h|text_red", settings->priv->text ? settings->priv->text->red : 0,
		"h|text_green", settings->priv->text ? settings->priv->text->green : 0,
		"h|text_blue", settings->priv->text ? settings->priv->text->blue : 0,
		"b|use_text", settings->priv->text ? TRUE : FALSE,
		"s|fontname", settings->priv->fontname ? settings->priv->fontname : "NULL",
		"b|toolbar", settings->priv->has_toolbar,
		"b|auto_hide_toolbar", settings->priv->autohide_toolbar,
		"b|scrollbar", settings->priv->has_scrollbar,
		"s|buttons", buttons,
		NULL);
	
	g_free (buttons);
}
