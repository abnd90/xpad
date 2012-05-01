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

#ifndef __XPAD_PAD_GROUP_H__
#define __XPAD_PAD_GROUP_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XPAD_TYPE_PAD_GROUP          (xpad_pad_group_get_type ())
#define XPAD_PAD_GROUP(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_PAD_GROUP, XpadPadGroup))
#define XPAD_PAD_GROUP_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_PAD_GROUP, XpadPadGroupClass))
#define XPAD_IS_PAD_GROUP(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_PAD_GROUP))
#define XPAD_IS_PAD_GROUP_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_PAD_GROUP))
#define XPAD_PAD_GROUP_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_PAD_GROUP, XpadPadGroupClass))

typedef struct XpadPadGroupClass XpadPadGroupClass;
typedef struct XpadPadGroup XpadPadGroup;
typedef struct XpadPadGroupPrivate XpadPadGroupPrivate;

struct XpadPadGroup
{
	GObject parent;
	
	/*< private >*/
	XpadPadGroupPrivate *priv;
};

struct XpadPadGroupClass
{
	GObjectClass parent_class;
	
	/* Signals */
	void (* pad_added)      (XpadPadGroup *group,
	                         GtkWidget *pad);
};

GType    xpad_pad_group_get_type (void);

XpadPadGroup *xpad_pad_group_new      (void);

void     xpad_pad_group_add      (XpadPadGroup *group, GtkWidget *pad);
void     xpad_pad_group_remove   (XpadPadGroup *group, GtkWidget *pad);

void     xpad_pad_group_close_all        (XpadPadGroup *group);
void     xpad_pad_group_show_all         (XpadPadGroup *group);
void     xpad_pad_group_toggle_hide      (XpadPadGroup *group);
GSList * xpad_pad_group_get_pads         (XpadPadGroup *group);
gint     xpad_pad_group_num_visible_pads (XpadPadGroup *group);

G_END_DECLS

#endif /* __XPAD_PAD_GROUP_H__ */
