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

#ifndef __XPAD_PAD_PROPERTIES_H__
#define __XPAD_PAD_PROPERTIES_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XPAD_TYPE_PAD_PROPERTIES          (xpad_pad_properties_get_type ())
#define XPAD_PAD_PROPERTIES(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_PAD_PROPERTIES, XpadPadProperties))
#define XPAD_PAD_PROPERTIES_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_PAD_PROPERTIES, XpadPadPropertiesClass))
#define XPAD_IS_PAD_PROPERTIES(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_PAD_PROPERTIES))
#define XPAD_IS_PAD_PROPERTIES_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_PAD_PROPERTIES))
#define XPAD_PAD_PROPERTIES_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_PAD_PROPERTIES, XpadPadPropertiesClass))

typedef struct XpadPadPropertiesClass XpadPadPropertiesClass;
typedef struct XpadPadPropertiesPrivate XpadPadPropertiesPrivate;
typedef struct XpadPadProperties XpadPadProperties;

struct XpadPadProperties
{
	/* private */
	GtkDialog parent;
	XpadPadPropertiesPrivate *priv;
};

struct XpadPadPropertiesClass
{
	GtkDialogClass parent_class;
};

GType xpad_pad_properties_get_type (void);

GtkWidget *xpad_pad_properties_new (void);

void xpad_pad_properties_set_follow_font_style (XpadPadProperties *pad_properties, gboolean follow);
gboolean xpad_pad_properties_get_follow_font_style (XpadPadProperties *pad_properties);

void xpad_pad_properties_set_follow_color_style (XpadPadProperties *pad_properties, gboolean follow);
gboolean xpad_pad_properties_get_follow_color_style (XpadPadProperties *pad_properties);

void xpad_pad_properties_set_back_color (XpadPadProperties *pad_properties, const GdkColor *back);
G_CONST_RETURN GdkColor *xpad_pad_properties_get_back_color (XpadPadProperties *pad_properties);

void xpad_pad_properties_set_text_color (XpadPadProperties *pad_properties, const GdkColor *text);
G_CONST_RETURN GdkColor *xpad_pad_properties_get_text_color (XpadPadProperties *pad_properties);

void xpad_pad_properties_set_fontname (XpadPadProperties *pad_properties, const gchar *fontname);
G_CONST_RETURN gchar *xpad_pad_properties_get_fontname (XpadPadProperties *pad_properties);

G_END_DECLS

#endif /* __XPAD_PAD_PROPERTIES_H__ */
