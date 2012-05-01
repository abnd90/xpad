/**
 * Copyright (c) 2004-2007 Michael Terry
 * Copyright (c) 2009 Paul Ivanov
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

#include "xpad-pad-group.h"
#include "xpad-pad.h"

G_DEFINE_TYPE(XpadPadGroup, xpad_pad_group, G_TYPE_OBJECT)

#define XPAD_PAD_GROUP_GET_PRIVATE(object)  (G_TYPE_INSTANCE_GET_PRIVATE ((object), XPAD_TYPE_PAD_GROUP, XpadPadGroupPrivate))

struct XpadPadGroupPrivate
{
	GSList *pads;
};

static void     xpad_pad_group_dispose           (GObject *object);

static void     xpad_pad_group_destroy_pads      (XpadPadGroup *group);

enum {
	PROP_0
};

enum
{
	PAD_ADDED,
	PAD_REMOVED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

XpadPadGroup *
xpad_pad_group_new (void)
{
	return XPAD_PAD_GROUP (g_object_new (XPAD_TYPE_PAD_GROUP, NULL));
}

static void
xpad_pad_group_class_init (XpadPadGroupClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->dispose = xpad_pad_group_dispose;
	
	signals[PAD_ADDED] =
		g_signal_new ("pad_added",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (XpadPadGroupClass, pad_added),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE,
		              1,
		              GTK_TYPE_WIDGET);
	
	signals[PAD_REMOVED] =
		g_signal_new ("pad_removed",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (XpadPadGroupClass, pad_added),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE,
		              1,
		              GTK_TYPE_WIDGET);
	
	g_type_class_add_private (object_class, sizeof (XpadPadGroupPrivate));
}

static void
xpad_pad_group_dispose (GObject *object)
{
	XpadPadGroup *group = XPAD_PAD_GROUP (object);

	xpad_pad_group_destroy_pads (group);
}

static void
xpad_pad_group_init (XpadPadGroup *group)
{
	group->priv = XPAD_PAD_GROUP_GET_PRIVATE (group);
	
	group->priv->pads = NULL;
}


GSList *
xpad_pad_group_get_pads (XpadPadGroup *group)
{
	return g_slist_copy (group->priv->pads);
}


/* Subsumes a pad into this group */
void
xpad_pad_group_add (XpadPadGroup *group, GtkWidget *pad)
{
	g_object_ref(pad);
	g_object_ref_sink(GTK_OBJECT(pad));
	
	group->priv->pads = g_slist_append (group->priv->pads, XPAD_PAD (pad));
	g_signal_connect_swapped (pad, "destroy", G_CALLBACK (xpad_pad_group_remove), group);
	
	g_signal_emit (group, signals[PAD_ADDED], 0, pad);
}


/* Removes a pad from this group */
void
xpad_pad_group_remove (XpadPadGroup *group, GtkWidget *pad)
{
	group->priv->pads = g_slist_remove (group->priv->pads, XPAD_PAD (pad));
	
	g_signal_emit (group, signals[PAD_REMOVED], 0, pad);
	
	g_object_unref(pad);
}


/* Deletes all the current pads in the group */
static void
xpad_pad_group_destroy_pads (XpadPadGroup *group)
{
	g_slist_foreach (group->priv->pads, (GFunc) gtk_widget_destroy, NULL);
	g_slist_free (group->priv->pads);
	group->priv->pads = NULL;
}


gint
xpad_pad_group_num_visible_pads (XpadPadGroup *group)
{
	gint num = 0;
	if (group)
	{
		GSList *i;
		for (i = group->priv->pads; i; i = i->next)
		{
			if (GTK_WIDGET_VISIBLE(GTK_WIDGET(i->data)))
				num ++;
		}
	}
	return num;
}


void
xpad_pad_group_close_all (XpadPadGroup *group)
{
	if (group)
		g_slist_foreach (group->priv->pads, (GFunc) xpad_pad_close, NULL);
}


void
xpad_pad_group_show_all (XpadPadGroup *group)
{
	if (group)
		g_slist_foreach (group->priv->pads, (GFunc) gtk_widget_show, NULL);
}


void
xpad_pad_group_toggle_hide(XpadPadGroup *group)
{
	if (group)
		g_slist_foreach (group->priv->pads, (GFunc) xpad_pad_toggle, NULL);
}

