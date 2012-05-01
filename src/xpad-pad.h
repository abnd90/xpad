/*

Copyright (c) 2001-2007 Michael Terry
Copyright (c) 2009 Paul Ivanov
Copyright (c) 2011 Sergei Riaguzov

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

#ifndef __XPAD_PAD_H__
#define __XPAD_PAD_H__

#include <gtk/gtk.h>
#include "xpad-pad-group.h"

G_BEGIN_DECLS

#define XPAD_TYPE_PAD          (xpad_pad_get_type ())
#define XPAD_PAD(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_PAD, XpadPad))
#define XPAD_PAD_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_PAD, XpadPadClass))
#define XPAD_IS_PAD(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_PAD))
#define XPAD_IS_PAD_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_PAD))
#define XPAD_PAD_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_PAD, XpadPadClass))

typedef struct XpadPadClass XpadPadClass;
typedef struct XpadPadPrivate XpadPadPrivate;
typedef struct XpadPad XpadPad;

struct XpadPad
{
   /* private */
   GtkWindow parent;
   XpadPadPrivate *priv;
};

struct XpadPadClass
{
   GtkWindowClass parent_class;
   
   void (*closed) (XpadPad *pad);
};

GType xpad_pad_get_type (void);

GtkWidget *xpad_pad_new (XpadPadGroup *group);
GtkWidget *xpad_pad_new_with_info (XpadPadGroup *group, const gchar *info_filename, gboolean *show);
GtkWidget *xpad_pad_new_from_file (XpadPadGroup *group, const gchar *filename);
void xpad_pad_close (XpadPad *pad);
void xpad_pad_toggle (XpadPad *pad);
void xpad_pad_save_info (XpadPad *pad);

void xpad_pad_load_content (XpadPad *pad);
void xpad_pad_save_content (XpadPad *pad);

void xpad_pad_notify_has_selection (XpadPad *pad);
void xpad_pad_notify_clipboard_owner_changed (XpadPad *pad);
void xpad_pad_notify_undo_redo_changed (XpadPad *pad);

G_END_DECLS

#endif /* __XPAD_PAD_H__ */
