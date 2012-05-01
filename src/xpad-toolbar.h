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

#ifndef __XPAD_TOOLBAR_H__
#define __XPAD_TOOLBAR_H__

#include <gtk/gtk.h>
#include "xpad-pad.h"

G_BEGIN_DECLS

#define XPAD_TYPE_TOOLBAR          (xpad_toolbar_get_type ())
#define XPAD_TOOLBAR(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_TOOLBAR, XpadToolbar))
#define XPAD_TOOLBAR_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_TOOLBAR, XpadToolbarClass))
#define XPAD_IS_TOOLBAR(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_TOOLBAR))
#define XPAD_IS_TOOLBAR_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_TOOLBAR))
#define XPAD_TOOLBAR_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_TOOLBAR, XpadToolbarClass))

typedef struct XpadToolbarClass XpadToolbarClass;
typedef struct XpadToolbarPrivate XpadToolbarPrivate;
typedef struct XpadToolbar XpadToolbar;

struct XpadToolbar
{
	GtkToolbar parent;
	
	/* private */
	XpadToolbarPrivate *priv;
};

struct XpadToolbarClass
{
	GtkToolbarClass parent_class;
	
	void (*activate_clear) (XpadToolbar *toolbar);
	void (*activate_close) (XpadToolbar *toolbar);
	void (*activate_undo) (XpadToolbar *toolbar);
	void (*activate_redo) (XpadToolbar *toolbar);
	void (*activate_cut) (XpadToolbar *toolbar);
	void (*activate_copy) (XpadToolbar *toolbar);
	void (*activate_paste) (XpadToolbar *toolbar);
	void (*activate_delete) (XpadToolbar *toolbar);
	void (*activate_new) (XpadToolbar *toolbar);
	void (*activate_preferences) (XpadToolbar *toolbar);
	void (*activate_properties) (XpadToolbar *toolbar);
	void (*activate_quit) (XpadToolbar *toolbar);
	void (*popup) (XpadToolbar *toolbar, GtkMenu *menu);
	void (*popdown) (XpadToolbar *toolbar, GtkMenu *menu);
};

GType xpad_toolbar_get_type (void);

GtkWidget *xpad_toolbar_new (XpadPad *pad);

void xpad_toolbar_enable_undo_button (XpadToolbar *toolbar, gboolean enable);
void xpad_toolbar_enable_redo_button (XpadToolbar *toolbar, gboolean enable);
void xpad_toolbar_enable_cut_button (XpadToolbar *toolbar, gboolean enable);
void xpad_toolbar_enable_copy_button (XpadToolbar *toolbar, gboolean enable);
void xpad_toolbar_enable_paste_button (XpadToolbar *toolbar, gboolean enable);

G_END_DECLS

#endif /* __XPAD_TOOLBAR_H__ */
