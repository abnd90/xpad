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

#ifndef __XPAD_APP_H__
#define __XPAD_APP_H__

#include <gtk/gtk.h>
#include "xpad-pad-group.h"

G_BEGIN_DECLS

void       xpad_app_error     (GtkWindow *parent, const gchar *primary, const gchar *secondary);
GtkWidget *xpad_app_alert_new (GtkWindow *parent, const gchar *stock, const gchar *primary, const gchar *secondary);

G_CONST_RETURN gchar *xpad_app_get_config_dir (void);
G_CONST_RETURN gchar *xpad_app_get_program_path (void);
XpadPadGroup         *xpad_app_get_pad_group (void);
gboolean              xpad_app_get_translucent (void);

G_END_DECLS

#endif /* __XPAD_APP_H__ */
