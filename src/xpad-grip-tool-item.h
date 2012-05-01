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

#ifndef __XPAD_GRIP_TOOL_ITEM_H__
#define __XPAD_GRIP_TOOL_ITEM_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XPAD_TYPE_GRIP_TOOL_ITEM          (xpad_grip_tool_item_get_type ())
#define XPAD_GRIP_TOOL_ITEM(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_GRIP_TOOL_ITEM, XpadGripToolItem))
#define XPAD_GRIP_TOOL_ITEM_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_GRIP_TOOL_ITEM, XpadGripToolItemClass))
#define XPAD_IS_GRIP_TOOL_ITEM(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_GRIP_TOOL_ITEM))
#define XPAD_IS_GRIP_TOOL_ITEM_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_GRIP_TOOL_ITEM))
#define XPAD_GRIP_TOOL_ITEM_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_GRIP_TOOL_ITEM, XpadGripToolItemClass))

typedef struct XpadGripToolItemClass XpadGripToolItemClass;
typedef struct XpadGripToolItemPrivate XpadGripToolItemPrivate;
typedef struct XpadGripToolItem XpadGripToolItem;

struct XpadGripToolItem
{
	GtkToolItem parent;
	
	/* private */
	XpadGripToolItemPrivate *priv;
};

struct XpadGripToolItemClass
{
	GtkToolItemClass parent_class;
};

GType xpad_grip_tool_item_get_type (void);

GtkToolItem *xpad_grip_tool_item_new (void);

G_END_DECLS

#endif /* __XPAD_GRIP_TOOL_ITEM_H__ */
