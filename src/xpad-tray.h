/*

Copyright (c) 2002 Jamis Buck
Copyright (c) 2003-2007 Michael Terry

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

#ifndef __XPAD_TRAY_H__
#define __XPAD_TRAY_H__

#include <gtk/gtk.h>

void     xpad_tray_open    (void);
void     xpad_tray_close   (void);
gboolean xpad_tray_is_open (void);

#endif /* __TRAY_H__ */
