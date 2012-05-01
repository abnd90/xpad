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

#ifndef __XPAD_TEXT_BUFFER_H__
#define __XPAD_TEXT_BUFFER_H__

#include <gtk/gtk.h>
#include "xpad-pad.h"

G_BEGIN_DECLS

#define XPAD_TYPE_TEXT_BUFFER          (xpad_text_buffer_get_type ())
#define XPAD_TEXT_BUFFER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_TEXT_BUFFER, XpadTextBuffer))
#define XPAD_TEXT_BUFFER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_TEXT_BUFFER, XpadTextBufferClass))
#define XPAD_IS_TEXT_BUFFER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_TEXT_BUFFER))
#define XPAD_IS_TEXT_BUFFER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_TEXT_BUFFER))
#define XPAD_TEXT_BUFFER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_TEXT_BUFFER, XpadTextBufferClass))

typedef struct XpadTextBufferClass XpadTextBufferClass;
typedef struct XpadTextBufferPrivate XpadTextBufferPrivate;
typedef struct XpadTextBuffer XpadTextBuffer;

struct XpadTextBuffer
{
	GtkTextBuffer parent;

	/* private */
	XpadTextBufferPrivate *priv;
};

struct XpadTextBufferClass
{
	GtkTextBufferClass parent_class;
};

GType xpad_text_buffer_get_type (void);

XpadTextBuffer *xpad_text_buffer_new (XpadPad *pad);

void xpad_text_buffer_set_text_with_tags (XpadTextBuffer *buffer, const gchar *text);
gchar *xpad_text_buffer_get_text_with_tags (XpadTextBuffer *buffer);

void xpad_text_buffer_insert_text (XpadTextBuffer *buffer, gint pos, const gchar *text, gint len);
void xpad_text_buffer_delete_range (XpadTextBuffer *buffer, gint start, gint end);
void xpad_text_buffer_toggle_tag (XpadTextBuffer *buffer, const gchar *name);

gboolean xpad_text_buffer_undo_available (XpadTextBuffer *buffer);
gboolean xpad_text_buffer_redo_available (XpadTextBuffer *buffer);
void xpad_text_buffer_undo (XpadTextBuffer *buffer);
void xpad_text_buffer_redo (XpadTextBuffer *buffer);
void xpad_text_buffer_freeze_undo (XpadTextBuffer *buffer);
void xpad_text_buffer_thaw_undo (XpadTextBuffer *buffer);

XpadPad *xpad_text_buffer_get_pad (XpadTextBuffer *buffer);
void xpad_text_buffer_set_pad (XpadTextBuffer *buffer, XpadPad *pad);

G_END_DECLS

#endif /* __XPAD_TEXT_BUFFER_H__ */
