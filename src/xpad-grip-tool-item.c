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

#include "xpad-grip-tool-item.h"

G_DEFINE_TYPE(XpadGripToolItem, xpad_grip_tool_item, GTK_TYPE_TOOL_ITEM)
#define XPAD_GRIP_TOOL_ITEM_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), XPAD_TYPE_GRIP_TOOL_ITEM, XpadGripToolItemPrivate))

struct XpadGripToolItemPrivate
{
	GtkWidget *drawbox;
};

/*static void xpad_grip_tool_item_size_request (GtkWidget *widget, GtkRequisition *requisition);*/
static gboolean xpad_grip_tool_item_event_box_expose (GtkWidget *widget, GdkEventExpose *event);
static void xpad_grip_tool_item_event_box_realize (GtkWidget *widget);
static gboolean xpad_grip_tool_item_button_pressed_event (GtkWidget *widget, GdkEventButton *event);

GtkToolItem *
xpad_grip_tool_item_new (void)
{
	return GTK_TOOL_ITEM (g_object_new (XPAD_TYPE_GRIP_TOOL_ITEM, NULL));
}

static void
xpad_grip_tool_item_class_init (XpadGripToolItemClass *klass)
{
	GObjectClass *gobject_class;
	GtkContainerClass *container_class;
	GtkWidgetClass *widget_class;
	
	gobject_class = (GObjectClass *)klass;
	container_class = (GtkContainerClass *)klass;
	widget_class = (GtkWidgetClass *)klass;
	
	g_type_class_add_private (gobject_class, sizeof (XpadGripToolItemPrivate));
}

static void
xpad_grip_tool_item_init (XpadGripToolItem *grip)
{
	GtkWidget *alignment;
	gboolean right;
	
	grip->priv = XPAD_GRIP_TOOL_ITEM_GET_PRIVATE (grip);
	
	grip->priv->drawbox = gtk_drawing_area_new ();
	gtk_widget_add_events (grip->priv->drawbox, GDK_BUTTON_PRESS_MASK | GDK_EXPOSURE_MASK);
	g_signal_connect (grip->priv->drawbox, "button-press-event", G_CALLBACK (xpad_grip_tool_item_button_pressed_event), NULL);
	g_signal_connect (grip->priv->drawbox, "realize", G_CALLBACK (xpad_grip_tool_item_event_box_realize), NULL);
	g_signal_connect (grip->priv->drawbox, "expose-event", G_CALLBACK (xpad_grip_tool_item_event_box_expose), NULL);
	gtk_widget_set_size_request (grip->priv->drawbox, 18, 18);
	
	right =	gtk_widget_get_direction (grip->priv->drawbox) == GTK_TEXT_DIR_LTR;
	alignment = gtk_alignment_new (right ? 1 : 0, 1, 0, 0);
	
	gtk_container_add (GTK_CONTAINER (alignment), grip->priv->drawbox);
	gtk_container_add (GTK_CONTAINER (grip), alignment);
}

static gboolean
xpad_grip_tool_item_button_pressed_event (GtkWidget *widget, GdkEventButton *event)
{
	if (event->button == 1)
	{
		GdkWindowEdge edge;
		
		if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
			edge = GDK_WINDOW_EDGE_SOUTH_EAST;
		else
			edge = GDK_WINDOW_EDGE_SOUTH_WEST;
	
		gtk_window_begin_resize_drag (GTK_WINDOW (gtk_widget_get_toplevel (widget)),
			edge, event->button, event->x_root, event->y_root, event->time);
		
		return TRUE;
	}
	
	return FALSE;
}

static void
xpad_grip_tool_item_event_box_realize (GtkWidget *widget)
{
	GdkDisplay *display = gtk_widget_get_display (widget);
	GdkCursorType cursor_type;
	GdkCursor *cursor;
	
	if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
		cursor_type = GDK_BOTTOM_RIGHT_CORNER;
	else
		cursor_type = GDK_BOTTOM_LEFT_CORNER;
	
	cursor = gdk_cursor_new_for_display (display, cursor_type);
	gdk_window_set_cursor (widget->window, cursor);
	gdk_cursor_unref (cursor);
}

static gboolean
xpad_grip_tool_item_event_box_expose (GtkWidget *widget, GdkEventExpose *event)
{
	GdkWindowEdge edge;
	
	if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
		edge = GDK_WINDOW_EDGE_SOUTH_EAST;
	else
		edge = GDK_WINDOW_EDGE_SOUTH_WEST;
	
	gtk_paint_resize_grip (
		widget->style,
		widget->window,
		GTK_WIDGET_STATE (widget),
		NULL,
		widget,
		"xpad-grip-tool-item",
		edge,
		0, 0,
		widget->allocation.width, widget->allocation.height);
	
	return FALSE;
}
