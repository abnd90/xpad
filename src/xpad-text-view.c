/**
 * Copyright (c) 2004-2007 Michael Terry
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

#include "xpad-text-view.h"
#include "xpad-text-buffer.h"
#include "xpad-settings.h"

G_DEFINE_TYPE(XpadTextView, xpad_text_view, GTK_TYPE_TEXT_VIEW)
#define XPAD_TEXT_VIEW_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), XPAD_TYPE_TEXT_VIEW, XpadTextViewPrivate))

struct XpadTextViewPrivate 
{
	gboolean follow_font_style;
	gboolean follow_color_style;
	gulong notify_text_handler;
	gulong notify_back_handler;
	gulong notify_font_handler;
	XpadTextBuffer *buffer;
};

static void xpad_text_view_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_text_view_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xpad_text_view_realize (XpadTextView *widget);
static void xpad_text_view_finalize (GObject *object);
static gboolean xpad_text_view_button_press_event (GtkWidget *widget, GdkEventButton *event);
static gboolean xpad_text_view_focus_out_event (GtkWidget *widget, GdkEventFocus *event);
static void xpad_text_view_notify_edit_lock (XpadTextView *view);
static void xpad_text_view_notify_editable (XpadTextView *view);
static void xpad_text_view_notify_fontname (XpadTextView *view);
static void xpad_text_view_notify_text_color (XpadTextView *view);
static void xpad_text_view_notify_back_color (XpadTextView *view);
static void xpad_text_view_style_set (GtkWidget *widget, GtkStyle *previous_style);

enum
{
  PROP_0,
  PROP_FOLLOW_FONT_STYLE,
  PROP_FOLLOW_COLOR_STYLE,
  LAST_PROP
};

GtkWidget *
xpad_text_view_new (void)
{
	return GTK_WIDGET (g_object_new (XPAD_TYPE_TEXT_VIEW, NULL));
}

static void
xpad_text_view_class_init (XpadTextViewClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->finalize = xpad_text_view_finalize;
	gobject_class->set_property = xpad_text_view_set_property;
	gobject_class->get_property = xpad_text_view_get_property;
	
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
	
	g_type_class_add_private (gobject_class, sizeof (XpadTextViewPrivate));
}

static void
xpad_text_view_init (XpadTextView *view)
{
	gchar *name;
	
	view->priv = XPAD_TEXT_VIEW_GET_PRIVATE (view);
	
	view->priv->follow_font_style = TRUE;
	view->priv->follow_color_style = TRUE;
	
	view->priv->buffer = xpad_text_buffer_new (NULL);
	gtk_text_view_set_buffer (GTK_TEXT_VIEW (view), GTK_TEXT_BUFFER (view->priv->buffer));
	
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD);
	gtk_container_set_border_width (GTK_CONTAINER (view), 5);
	
	name = g_strdup_printf ("%p", (void *) view);
	gtk_widget_set_name (GTK_WIDGET (view), name);
	g_free (name);
	
	g_signal_connect (view, "button-press-event", G_CALLBACK (xpad_text_view_button_press_event), NULL);
	g_signal_connect_after (view, "focus-out-event", G_CALLBACK (xpad_text_view_focus_out_event), NULL);
	g_signal_connect (view, "realize", G_CALLBACK (xpad_text_view_realize), NULL);
	g_signal_connect (view, "notify::editable", G_CALLBACK (xpad_text_view_notify_editable), NULL);
	g_signal_connect (view, "style-set", G_CALLBACK (xpad_text_view_style_set), NULL);
	g_signal_connect_swapped (xpad_settings (), "notify::edit-lock", G_CALLBACK (xpad_text_view_notify_edit_lock), view);
	view->priv->notify_font_handler = g_signal_connect_swapped (xpad_settings (), "notify::fontname", G_CALLBACK (xpad_text_view_notify_fontname), view);
	view->priv->notify_text_handler = g_signal_connect_swapped (xpad_settings (), "notify::text-color", G_CALLBACK (xpad_text_view_notify_text_color), view);
	view->priv->notify_back_handler = g_signal_connect_swapped (xpad_settings (), "notify::back-color", G_CALLBACK (xpad_text_view_notify_back_color), view);
	
	xpad_text_view_notify_text_color (view);
	xpad_text_view_notify_back_color (view);
	xpad_text_view_notify_fontname (view);
}

static void
xpad_text_view_finalize (GObject *object)
{
	XpadTextView *view = XPAD_TEXT_VIEW (object);

	if (view->priv->buffer)
	{
		g_object_unref (view->priv->buffer);
		view->priv->buffer = NULL;
	}
	
	g_signal_handlers_disconnect_matched (xpad_settings (), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, view);
	
	G_OBJECT_CLASS (xpad_text_view_parent_class)->finalize (object);
}

static void
xpad_text_view_realize (XpadTextView *view)
{
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), !xpad_settings_get_edit_lock (xpad_settings ()));
}

static gboolean
xpad_text_view_focus_out_event (GtkWidget *widget, GdkEventFocus *event)
{
	if (xpad_settings_get_edit_lock (xpad_settings ()))
	{
		gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), FALSE);
		return TRUE;
	}
	
	return FALSE;
}

static gboolean
xpad_text_view_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	if (event->button == 1 &&
	    xpad_settings_get_edit_lock (xpad_settings ()) &&
	    !gtk_text_view_get_editable (GTK_TEXT_VIEW (widget)))
	{
		if (event->type == GDK_2BUTTON_PRESS)
		{
			gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), TRUE);
			return TRUE;
		}
		else if (event->type == GDK_BUTTON_PRESS)
		{
			gtk_window_begin_move_drag (GTK_WINDOW (gtk_widget_get_toplevel (widget)), event->button, event->x_root, event->y_root, event->time);
			return TRUE;
		}
	}
	
	return FALSE;
}

static void
xpad_text_view_notify_edit_lock (XpadTextView *view)
{
	/* chances are good that they don't have the text view focused while it changed, so make non-editable if edit lock turned on */
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), !xpad_settings_get_edit_lock (xpad_settings ()));
}

static void
xpad_text_view_notify_editable (XpadTextView *view)
{
	GdkCursor *cursor;
	gboolean editable;
	
	editable = gtk_text_view_get_editable (GTK_TEXT_VIEW (view));
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), editable);
	
	cursor = editable ? gdk_cursor_new (GDK_XTERM) : NULL;
	
	gdk_window_set_cursor (gtk_text_view_get_window (GTK_TEXT_VIEW (view), GTK_TEXT_WINDOW_TEXT), cursor);
	
	if (cursor)
		gdk_cursor_unref (cursor);
}

/* Adjust the cursor to match the text color */
static void
xpad_text_view_style_set (GtkWidget *widget, GtkStyle *previous_style)
{
	GdkColor c;
	
	/* text color changes */
	c = gtk_widget_get_style (widget)->text[GTK_STATE_NORMAL];
	if (!previous_style || !gdk_color_equal (&c, &previous_style->text[GTK_STATE_NORMAL]))
	{
		const gchar *name = gtk_widget_get_name (widget);
		gchar *style_string = g_strdup_printf ("style '%s' {GtkWidget::cursor_color = {%" G_GUINT16_FORMAT ", %" G_GUINT16_FORMAT ", %" G_GUINT16_FORMAT "}} widget '*%s' style '%s'", name, c.red, c.green, c.blue, name, name);
		gtk_rc_parse_string (style_string);
		g_free (style_string);
		gtk_widget_reset_rc_styles (widget);
	}
	
	/* base color changes */
	c = gtk_widget_get_style (widget)->base[GTK_STATE_NORMAL];
	if (!previous_style || !gdk_color_equal (&c, &previous_style->base[GTK_STATE_NORMAL]))
	{
		gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, &c);
	}
}

static void
xpad_text_view_notify_fontname (XpadTextView *view)
{
	const gchar *font = xpad_settings_get_fontname (xpad_settings ());
	PangoFontDescription *fontdesc;
	
	fontdesc = font ? pango_font_description_from_string (font) : NULL;
	gtk_widget_modify_font (GTK_WIDGET (view), fontdesc);
	if (fontdesc)
		pango_font_description_free (fontdesc);
}

static void
xpad_text_view_notify_text_color (XpadTextView *view)
{
	gtk_widget_modify_text (GTK_WIDGET (view), GTK_STATE_NORMAL, xpad_settings_get_text_color (xpad_settings ()));
}

static void
xpad_text_view_notify_back_color (XpadTextView *view)
{
	gtk_widget_modify_base (GTK_WIDGET (view), GTK_STATE_NORMAL, xpad_settings_get_back_color (xpad_settings ()));
}

void
xpad_text_view_set_follow_font_style (XpadTextView *view, gboolean follow)
{
	if (follow != view->priv->follow_font_style)
	{
		if (follow)
		{
			g_signal_handler_unblock (xpad_settings (), view->priv->notify_font_handler);
			xpad_text_view_notify_fontname (view);
		}
		else
		{
			g_signal_handler_block (xpad_settings (), view->priv->notify_font_handler);
		}
	}
	
	view->priv->follow_font_style = follow;
	
	g_object_notify (G_OBJECT (view), "follow_font_style");
}

gboolean
xpad_text_view_get_follow_font_style (XpadTextView *view)
{
	return view->priv->follow_font_style;
}

void
xpad_text_view_set_follow_color_style (XpadTextView *view, gboolean follow)
{
	if (follow != view->priv->follow_color_style)
	{
		if (follow)
		{
			g_signal_handler_unblock (xpad_settings (), view->priv->notify_text_handler);
			g_signal_handler_unblock (xpad_settings (), view->priv->notify_back_handler);
			xpad_text_view_notify_text_color (view);
			xpad_text_view_notify_back_color (view);
		}
		else
		{
			g_signal_handler_block (xpad_settings (), view->priv->notify_text_handler);
			g_signal_handler_block (xpad_settings (), view->priv->notify_back_handler);
		}
	}
	
	view->priv->follow_color_style = follow;
	
	g_object_notify (G_OBJECT (view), "follow_color_style");
}

gboolean
xpad_text_view_get_follow_color_style (XpadTextView *view)
{
	return view->priv->follow_color_style;
}

static void
xpad_text_view_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadTextView *view;
	
	view = XPAD_TEXT_VIEW (object);
	
	switch (prop_id)
	{
	case PROP_FOLLOW_FONT_STYLE:
		xpad_text_view_set_follow_font_style (view, g_value_get_boolean (value));
		break;
	
	case PROP_FOLLOW_COLOR_STYLE:
		xpad_text_view_set_follow_color_style (view, g_value_get_boolean (value));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xpad_text_view_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadTextView *view;
	
	view = XPAD_TEXT_VIEW (object);
	
	switch (prop_id)
	{
	case PROP_FOLLOW_FONT_STYLE:
		g_value_set_boolean (value, xpad_text_view_get_follow_font_style (view));
		break;
	
	case PROP_FOLLOW_COLOR_STYLE:
		g_value_set_boolean (value, xpad_text_view_get_follow_color_style (view));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

XpadPad *xpad_text_view_get_pad (XpadTextView *view)
{
	return xpad_text_buffer_get_pad (view->priv->buffer);
}

void xpad_text_view_set_pad (XpadTextView *view, XpadPad *pad)
{
	xpad_text_buffer_set_pad (view->priv->buffer, pad);
}
