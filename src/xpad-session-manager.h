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

#ifndef __XPAD_SESSION_MANAGER_H__
#define __XPAD_SESSION_MANAGER_H__

#include <gtk/gtk.h>

gboolean xpad_session_manager_start_interact (gboolean error);
void     xpad_session_manager_stop_interact (gboolean stop_shutdown);
void     xpad_session_manager_init (void);
void     xpad_session_manager_shutdown (void);
void     xpad_session_manager_set_id (const gchar *id);

#endif /* __XPAD_SESSION_MANAGER_H__ */
