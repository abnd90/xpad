/*

Copyright (c) 2001-2007 Michael Terry
Copyright (c) 2010 Sergei Riaguzov

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

#ifndef __XPAD_UNDO_H__
#define __XPAD_UNDO_H__

#include <gtk/gtk.h>
#include "xpad-text-buffer.h"
#include "xpad-pad.h"

G_BEGIN_DECLS

#define XPAD_TYPE_UNDO          (xpad_undo_get_type ())
#define XPAD_UNDO(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_UNDO, XpadUndo))
#define XPAD_UNDO_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_UNDO, XpadUndoClass))
#define XPAD_IS_UNDO(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_UNDO))
#define XPAD_IS_UNDO_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_UNDO))
#define XPAD_UNDO_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_UNDO, XpadUndoClass))

typedef struct XpadUndoClass XpadUndoClass;
typedef struct XpadUndoPrivate XpadUndoPrivate;
typedef struct XpadUndo XpadUndo;

struct XpadUndo
{
	GObject parent;

	/* private */
	XpadUndoPrivate *priv;
};

struct XpadUndoClass
{
	GObjectClass parent_class;
};

GType xpad_undo_get_type (void);

XpadUndo* xpad_undo_new (XpadTextBuffer *buffer);

gboolean xpad_undo_undo_available (XpadUndo *undo);
gboolean xpad_undo_redo_available (XpadUndo *undo);
void xpad_undo_exec_undo (XpadUndo *undo);
void xpad_undo_exec_redo (XpadUndo *undo);
void xpad_undo_freeze (XpadUndo *undo);
void xpad_undo_thaw (XpadUndo *undo);

void xpad_undo_apply_tag (XpadUndo *undo, const gchar *name, GtkTextIter *start, GtkTextIter *end);
void xpad_undo_remove_tag (XpadUndo *undo, const gchar *name, GtkTextIter *start, GtkTextIter *end);

G_END_DECLS

#endif /* __XPAD_UNDO_H__ */
