/**
 * Copyright (c) 2004-2007 Michael Terry
 * Copyright (c) 2011 Sergei Riaguzov
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

#include "xpad-text-buffer.h"
#include "xpad-undo.h"
#include "xpad-pad.h"

G_DEFINE_TYPE(XpadTextBuffer, xpad_text_buffer, GTK_TYPE_TEXT_BUFFER)
#define XPAD_TEXT_BUFFER_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), XPAD_TYPE_TEXT_BUFFER, XpadTextBufferPrivate))

struct XpadTextBufferPrivate 
{
	XpadUndo *undo;
	XpadPad *pad;
};

/* Unicode chars in the Private Use Area. */
static gunichar TAG_CHAR = 0xe000;

static GtkTextTagTable *create_tag_table (void);

static GtkTextTagTable *global_text_tag_table = NULL;

enum
{
	PROP_0,
	PROP_PAD,
	LAST_PROP
};

XpadTextBuffer *
xpad_text_buffer_new (XpadPad *pad)
{
	if (!global_text_tag_table)
		global_text_tag_table = create_tag_table (); /* FIXME: never freed */
	
	return XPAD_TEXT_BUFFER (g_object_new (XPAD_TYPE_TEXT_BUFFER, "tag_table", global_text_tag_table, "pad", pad, NULL));
}

static void
xpad_text_buffer_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadTextBuffer *text_buffer = XPAD_TEXT_BUFFER (object);

	switch (prop_id)
	{
	case PROP_PAD:
		if (text_buffer->priv->pad && G_IS_OBJECT (text_buffer->priv->pad))
			g_object_unref (text_buffer->priv->pad);
		if (G_VALUE_HOLDS_POINTER (value) && G_IS_OBJECT (g_value_get_pointer (value)))
		{
			text_buffer->priv->pad = g_value_get_pointer (value);
			g_object_ref (text_buffer->priv->pad);
		}
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	 }
}

static void
xpad_text_buffer_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadTextBuffer *text_buffer = XPAD_TEXT_BUFFER (object);

	switch (prop_id)
	{
	case PROP_PAD:
		g_value_set_pointer (value, text_buffer->priv->pad);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
xpad_text_buffer_finalize (GObject *object)
{
	XpadTextBuffer *text_buffer = XPAD_TEXT_BUFFER (object);

	if (text_buffer->priv->pad)
	{
		g_object_unref (text_buffer->priv->pad);
		text_buffer->priv->pad = NULL;
	}
	
	if (text_buffer->priv->undo)
	{
		g_free (text_buffer->priv->undo);
		text_buffer->priv->undo = NULL;
	}
	
	G_OBJECT_CLASS (xpad_text_buffer_parent_class)->finalize (object);
}

static void
xpad_text_buffer_class_init (XpadTextBufferClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->finalize = xpad_text_buffer_finalize;
	gobject_class->set_property = xpad_text_buffer_set_property;
	gobject_class->get_property = xpad_text_buffer_get_property;

	g_object_class_install_property (gobject_class,
					 PROP_PAD,
					 g_param_spec_pointer ("pad",
									"Pad",
									"Pad connected to this buffer",
									G_PARAM_READWRITE));
	
	g_type_class_add_private (gobject_class, sizeof (XpadTextBufferPrivate));
}

static void
xpad_text_buffer_init (XpadTextBuffer *buffer)
{
	buffer->priv = XPAD_TEXT_BUFFER_GET_PRIVATE (buffer);

	buffer->priv->undo = xpad_undo_new (buffer);
}

void
xpad_text_buffer_set_text_with_tags (XpadTextBuffer *buffer, const gchar *text)
{
	GtkTextIter start, end;
	GList *tags = NULL;
	gchar **tokens;
	gint count;
	gchar tag_char_utf8[7] = {0};
	
	if (!text)
		return;
	
	gtk_text_buffer_begin_user_action (GTK_TEXT_BUFFER (buffer));
	
	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer), &start, &end);
	gtk_text_buffer_delete (GTK_TEXT_BUFFER (buffer), &start, &end);
	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer), &start, &end);
	
	g_unichar_to_utf8 (TAG_CHAR, tag_char_utf8);
	
	tokens = g_strsplit (text, tag_char_utf8, 0);
	
	for (count = 0; tokens[count]; count++)
	{
		if (count % 2 == 0)
		{
			gint offset;
			GList *j;
			
			offset = gtk_text_iter_get_offset (&end);
			gtk_text_buffer_insert (GTK_TEXT_BUFFER (buffer), &end, tokens[count], -1);
			gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (buffer), &start, offset);
			
			for (j = tags; j; j = j->next)
			{
				gtk_text_buffer_apply_tag_by_name (GTK_TEXT_BUFFER (buffer), j->data, &start, &end);
			}
		}
		else
		{
			if (tokens[count][0] != '/')
			{
				tags = g_list_prepend (tags, tokens[count]);
			}
			else
			{
				GList *element = g_list_find_custom (tags, &(tokens[count][1]), (GCompareFunc) g_ascii_strcasecmp);
				
				if (element)
				{
					tags = g_list_delete_link (tags, element);
				}
			}
		}
	}
	
	gtk_text_buffer_end_user_action (GTK_TEXT_BUFFER (buffer));
	
	g_strfreev (tokens);
}


gchar *
xpad_text_buffer_get_text_with_tags (XpadTextBuffer *buffer)
{
	GtkTextIter start, prev;
	GSList *tags = NULL, *i;
	gchar tag_char_utf8[7] = {0};
	gchar *text = g_strdup (""), *oldtext = NULL, *tmp;
	gboolean done = FALSE;
	
	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &start);
	
	g_unichar_to_utf8 (TAG_CHAR, tag_char_utf8);
	
	prev = start;
	
	while (!done)
	{
		tmp = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (buffer), &prev, &start, TRUE);
		oldtext = text;
		text = g_strconcat (text, tmp, NULL);
		g_free (oldtext);
		g_free (tmp);
		
		tags = gtk_text_iter_get_toggled_tags (&start, TRUE);
		for (i = tags; i; i = i->next)
		{
			gchar *name;
			g_object_get (G_OBJECT (i->data), "name", &name, NULL);
			oldtext = text;
			text = g_strconcat (text, tag_char_utf8, name, tag_char_utf8, NULL);
			g_free (oldtext);
			g_free (name);
		}
		g_slist_free (tags);
		
		tags = gtk_text_iter_get_toggled_tags (&start, FALSE);
		for (i = tags; i; i = i->next)
		{
			gchar *name;
			g_object_get (G_OBJECT (i->data), "name", &name, NULL);
			oldtext = text;
			text = g_strconcat (text, tag_char_utf8, "/", name, tag_char_utf8, NULL);
			g_free (oldtext);
			g_free (name);
		}
		g_slist_free (tags);
		
		if (gtk_text_iter_is_end (&start))
			done = TRUE;
		prev = start;
		gtk_text_iter_forward_to_tag_toggle (&start, NULL);
	}
	
	return text;
}

void
xpad_text_buffer_insert_text (XpadTextBuffer *buffer, gint pos, const gchar *text, gint len)
{
    GtkTextBuffer *parent = (GtkTextBuffer*) buffer;
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_offset (parent, &iter, pos);
	gtk_text_buffer_insert (parent, &iter, text, len);
	gtk_text_buffer_place_cursor (parent, &iter);
}

void
xpad_text_buffer_delete_range (XpadTextBuffer *buffer, gint start, gint end)
{
    GtkTextBuffer *parent = (GtkTextBuffer*) buffer;

	GtkTextIter start_iter;
	GtkTextIter end_iter;

	gtk_text_buffer_get_iter_at_offset (parent, &start_iter, start);

	if (end < 0)
		gtk_text_buffer_get_end_iter (parent, &end_iter);
	else
		gtk_text_buffer_get_iter_at_offset (parent, &end_iter, end);

	gtk_text_buffer_place_cursor (parent, &start_iter);
	gtk_text_buffer_delete (parent, &start_iter, &end_iter);
}

void
xpad_text_buffer_toggle_tag (XpadTextBuffer *buffer, const gchar *name)
{
	GtkTextTagTable *table;
	GtkTextTag *tag;
	GtkTextIter start, end, i;
	gboolean all_tagged;
	
	table = gtk_text_buffer_get_tag_table ( GTK_TEXT_BUFFER (buffer));
	tag = gtk_text_tag_table_lookup (table, name);
	gtk_text_buffer_get_selection_bounds ( GTK_TEXT_BUFFER (buffer), &start, &end);
	
	if (!tag)
	{
		g_print ("Tag not found in table %p\n", (void *) table);
		return;
	}
	
	for (all_tagged = TRUE, i = start; !gtk_text_iter_equal (&i, &end); gtk_text_iter_forward_char (&i))
	{
		if (!gtk_text_iter_has_tag (&i, tag))
		{
			all_tagged = FALSE;
			break;
		}
	}
	
	if (all_tagged)
	{
		gtk_text_buffer_remove_tag ( GTK_TEXT_BUFFER (buffer), tag, &start, &end);
		xpad_undo_remove_tag (buffer->priv->undo, name, &start, &end);
	}
	else
	{
		gtk_text_buffer_apply_tag ( GTK_TEXT_BUFFER (buffer), tag, &start, &end);
		xpad_undo_apply_tag (buffer->priv->undo, name, &start, &end);
	}
}

static GtkTextTagTable *
create_tag_table (void)
{
	GtkTextTagTable *table;
	GtkTextTag *tag;
	
	table = gtk_text_tag_table_new ();
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "bold", "weight", PANGO_WEIGHT_BOLD, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "italic", "style", PANGO_STYLE_ITALIC, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "strikethrough", "strikethrough", TRUE, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "underline", "underline", PANGO_UNDERLINE_SINGLE, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "small-xx", "scale", PANGO_SCALE_XX_SMALL, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "small-x", "scale", PANGO_SCALE_X_SMALL, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "small", "scale", PANGO_SCALE_SMALL, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "medium", "scale", PANGO_SCALE_MEDIUM, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "large", "scale", PANGO_SCALE_LARGE, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "large-x", "scale", PANGO_SCALE_X_LARGE, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	tag = GTK_TEXT_TAG (g_object_new (GTK_TYPE_TEXT_TAG, "name", "large-xx", "scale", PANGO_SCALE_XX_LARGE, NULL));
	gtk_text_tag_table_add (table, tag);
	g_object_unref (tag);
	
	return table;
}

gboolean
xpad_text_buffer_undo_available (XpadTextBuffer *buffer)
{
	return xpad_undo_undo_available (buffer->priv->undo);
}

gboolean
xpad_text_buffer_redo_available (XpadTextBuffer *buffer)
{
	return xpad_undo_redo_available (buffer->priv->undo);
}

void
xpad_text_buffer_undo (XpadTextBuffer *buffer)
{
	xpad_undo_exec_undo (buffer->priv->undo);
}

void
xpad_text_buffer_redo (XpadTextBuffer *buffer)
{
	xpad_undo_exec_redo (buffer->priv->undo);
}

void xpad_text_buffer_freeze_undo (XpadTextBuffer *buffer)
{
	xpad_undo_freeze (buffer->priv->undo);
}

void xpad_text_buffer_thaw_undo (XpadTextBuffer *buffer)
{
	xpad_undo_thaw (buffer->priv->undo);
}

XpadPad *xpad_text_buffer_get_pad (XpadTextBuffer *buffer)
{
	if (buffer == NULL)
		return NULL;

	XpadPad *pad = NULL;
	g_object_get (G_OBJECT (buffer), "pad", &pad, NULL);
	return pad;
}

void xpad_text_buffer_set_pad (XpadTextBuffer *buffer, XpadPad *pad)
{
	g_return_if_fail (buffer);

	g_object_set (G_OBJECT (buffer), "pad", pad, NULL);
}

