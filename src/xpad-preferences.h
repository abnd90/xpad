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

#ifndef __XPAD_PREFERENCES_H__
#define __XPAD_PREFERENCES_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XPAD_TYPE_PREFERENCES          (xpad_preferences_get_type ())
#define XPAD_PREFERENCES(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_PREFERENCES, XpadPreferences))
#define XPAD_PREFERENCES_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_PREFERENCES, XpadPreferencesClass))
#define XPAD_IS_PREFERENCES(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_PREFERENCES))
#define XPAD_IS_PREFERENCES_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_PREFERENCES))
#define XPAD_PREFERENCES_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_PREFERENCES, XpadPreferencesClass))

typedef struct XpadPreferencesClass XpadPreferencesClass;
typedef struct XpadPreferencesPrivate XpadPreferencesPrivate;
typedef struct XpadPreferences XpadPreferences;

struct XpadPreferences
{
	/* private */
	GtkDialog parent;
	XpadPreferencesPrivate *priv;
};

struct XpadPreferencesClass
{
	GtkDialogClass parent_class;
};

GType xpad_preferences_get_type (void);

void xpad_preferences_open (void);

G_END_DECLS

#endif /* __XPAD_PREFERENCES_H__ */
