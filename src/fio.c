/*

Copyright (c) 2001-2007 Michael Terry
Copytight (c) 2011 David Hull

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

#include "../config.h"
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "fio.h"
#include "xpad-app.h"

/* Sets filename to full path of filename (prepends xpad_app_get_config_dir ()
   to it).  Returns a GFile representing the file. */
static GFile *
fio_fill_filename (const gchar *filename)
{
	if (g_path_is_absolute (filename))
		return g_file_new_for_path (filename);
	else {
		gchar *full_path;
		GFile *file;
		
		full_path = g_build_filename (xpad_app_get_config_dir (), filename, NULL);
		file = g_file_new_for_path (full_path);
		
		g_free (full_path);
		return file;
	}
}

/* This function returns 'string' with all instances
   of 'obj' replaced with instances of 'replacement'
   It modifies string and re-allocs it.
   */
gchar *str_replace_tokens (gchar **string, gchar obj, gchar *replacement)
{
	gchar *p;
	gint rsize = strlen (replacement);
	gint osize = 1;
	gint diff = rsize - osize;
	
	p = *string;
	while ((p = strchr (p, obj)))
	{
		gint offset = p - *string;
		*string = g_realloc (*string, strlen (*string) + diff + 1);
		p = *string + offset;
		g_memmove (p + rsize, p + osize, strlen (p + osize) + 1);
		
		memcpy (p, replacement, rsize);
		
		p = p + rsize;
	}
	
	return *string;
}

gchar *
fio_unique_name (const gchar *prefix)
{
	int fd;
	gchar *name, *base, *pattern;
	
	pattern = g_strconcat (prefix, "XXXXXX", NULL);
	name = g_build_filename (xpad_app_get_config_dir (), pattern, NULL);
	g_free (pattern);
	fd = g_mkstemp (name);
	if (fd == -1)
	{
		g_free (name);
		return NULL;
	}
	
	close (fd);
	base = g_path_get_basename (name);
	
	g_free (name);
	
	return base;
}

/* This callously overwrites name~ -- but this is fine since this function is for
   our private .xpad directory anyway */
gboolean fio_set_file (const gchar *name, const gchar *value)
{
	GFile *file;
	GFileOutputStream *stream;
	GError *error = NULL;
	
	file = fio_fill_filename (name);
	
	stream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_PRIVATE, NULL, &error);
	
	if (stream)
	{
		g_output_stream_write_all (G_OUTPUT_STREAM (stream), value, strlen (value),
		                           NULL, NULL, &error);
		g_object_unref (stream);
	}
	
	if (error)
	{
		gchar *usertext;
		gchar *parse_name;
		
		parse_name = g_file_get_parse_name (file);
		usertext = g_strdup_printf (_("Could not write to file %s: %s"), parse_name, error->message);
		
		xpad_app_error (NULL, usertext, NULL);
		
		g_error_free (error);
		g_free (usertext);
		g_free (parse_name);
	}
	
	g_object_unref (file);
	return !error;
}


/**
 * Returned gchar * must be g_free'd.
 */
gchar *fio_get_file (const gchar *name)
{
	GFile *file;
	gchar *contents = NULL;
	
	file = fio_fill_filename (name);
	g_file_load_contents (file, NULL, &contents, NULL, NULL, NULL);
	g_object_unref (file);
	
	return contents;
}


/* list is a variable number of (gchar *) / (gchar ** or gint *) groups, 
	terminated by a NULL variable */
/**
 * each gchar ** pointer will hereafter point to memory that must be g_free'd.
 */
gint fio_get_values_from_file (const gchar *filename, ...)
{
	gchar *buf;
	const gchar *item;
	va_list ap;
	size_t len;
	
	buf = fio_get_file (filename);
	
	if (!buf)
		return 1;
	
	/* because of the way we look for a matching variable name, which is
		to look for an endline, the variable name, and a space, we insert a
		newline at the beginning, so that the first variable name is caught. */
	len = strlen (buf);
	buf = g_realloc (buf, len + 2);
	g_memmove (buf + 1, buf, len + 1);
	buf[0] = '\n';
	
	va_start (ap, filename);

	while ((item = va_arg (ap, gchar *)))
	{
		gchar *fullitem;
		gint *value;
		gchar *where;
		gint size;
		gchar type;
		
		type = item[0];
		item = &item[2]; /* skip type and '|' */
		fullitem = g_strdup_printf ("\n%s ", item);
		value = va_arg (ap, void *);
		where  = strstr (buf, fullitem);
		
		if (where)
		{
			gchar *temp;

			where = strstr (where, " ") + 1;
			size = strcspn (where, "\n");
		
			temp = g_malloc (size + 1);
			strncpy (temp, where, size);
			temp[size] = '\0';
			
			switch (type)
			{
			case 'i':
				*((gint *) value) = atoi (temp);
				break;
			case 'u':
				*((guint *) value) = (guint) strtoul (temp, NULL, 0);
				break;
			case 'h':
				*((guint16 *) value) = (guint16) strtoul (temp, NULL, 0);
				break;
			case 's':
				g_free (*((gchar **) value));
				*((gchar **) value) = g_strdup (temp);
				break;
			case 'b':
				*((gboolean *) value) = atoi (temp) ? TRUE : FALSE;
				break;
			default:
				g_warning ("Bad type to fio_get_values_from_file: %c\n", type);
				break;
			}
		
			g_free (temp);
		}
		g_free (fullitem);
	}

	va_end (ap);
	g_free (buf);

	return 0;
}

/* list is a variable number of (gchar *) / (gchar ** or gint *) groups, 
	terminated by a NULL variable */
gint fio_set_values_to_file (const gchar *filename, ...)
{
	gchar *buf, *tmpbuf;
	const gchar *item;
	va_list ap;
	
	va_start (ap, filename);
	
	buf = g_strdup ("");
	while ((item = va_arg (ap, gchar *)))
	{
		gchar *final_string;
		gchar *value_string;
		gchar type;
		
		type = item[0];
		item = &item[2]; /* skip type and '|' */
		
		/* translate our types to printf types */
		switch (type)
		{
		case 'b':
			value_string = g_strdup_printf ("%i", va_arg (ap, gboolean));
			break;
		case 'h':
		case 'i':
			value_string = g_strdup_printf ("%i", va_arg (ap, gint));
			break;
		case 'u':
			value_string = g_strdup_printf ("%u", va_arg (ap, guint));
			break;
		case 's':
			value_string = g_strdup_printf ("%s", va_arg (ap, gchar *));
			break;
		default:
			g_warning ("Bad type to fio_set_values_to_file: %c\n", type);
			value_string = g_strdup ("");
			break;
		}
		
		final_string = g_strdup_printf ("%s%s %s", (buf[0] == 0) ? "" : "\n", item, value_string);
		g_free (value_string);
		
		tmpbuf = buf;
		buf = g_strconcat (buf, final_string, NULL);
		g_free (tmpbuf);
		g_free (final_string);
	}
	
	va_end (ap);
	
	tmpbuf = buf;
	buf = g_strconcat (buf, "\n", NULL);
	g_free (tmpbuf);
	
	if (!fio_set_file (filename, buf))
	{
		g_free (buf);
		return 1;
	}
	
	g_free (buf);
	return 0;
}

void fio_remove_file (const gchar *filename)
{
	GFile *file;
	file = fio_fill_filename (filename);
	g_file_delete (file, NULL, NULL);
	g_object_unref (file);
}

