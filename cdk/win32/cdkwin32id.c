/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"
#include <cdk/cdk.h>

#include "cdkprivate-win32.h"

static GHashTable *handle_ht = NULL;

static guint
cdk_handle_hash (HANDLE *handle)
{
#ifdef _WIN64
  return ((guint *) handle)[0] ^ ((guint *) handle)[1];
#else
  return (guint) *handle;
#endif
}

static gint
cdk_handle_equal (HANDLE *a,
		  HANDLE *b)
{
  return (*a == *b);
}

void
cdk_win32_handle_table_insert (HANDLE  *handle,
			       gpointer data)
{
  g_return_if_fail (handle != NULL);

  if (!handle_ht)
    handle_ht = g_hash_table_new ((GHashFunc) cdk_handle_hash,
				  (GEqualFunc) cdk_handle_equal);

  g_hash_table_insert (handle_ht, handle, data);
}

void
cdk_win32_handle_table_remove (HANDLE handle)
{
  if (!handle_ht)
    handle_ht = g_hash_table_new ((GHashFunc) cdk_handle_hash,
				  (GEqualFunc) cdk_handle_equal);

  g_hash_table_remove (handle_ht, &handle);
}

gpointer
cdk_win32_handle_table_lookup (HWND handle)
{
  gpointer data = NULL;

  if (handle_ht)
    data = g_hash_table_lookup (handle_ht, &handle);

  return data;
}
