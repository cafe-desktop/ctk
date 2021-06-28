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

#include "cdkprivate-x11.h"
#include "cdkdisplay-x11.h"

#include <stdio.h>

static guint
cdk_xid_hash (XID *xid)
{
  return *xid;
}

static gboolean
cdk_xid_equal (XID *a, XID *b)
{
  return (*a == *b);
}

void
_cdk_x11_display_add_window (CdkDisplay *display,
                             XID        *xid,
                             CdkWindow  *data)
{
  CdkX11Display *display_x11;

  g_return_if_fail (xid != NULL);
  g_return_if_fail (GDK_IS_DISPLAY (display));

  display_x11 = GDK_X11_DISPLAY (display);

  if (!display_x11->xid_ht)
    display_x11->xid_ht = g_hash_table_new ((GHashFunc) cdk_xid_hash,
                                            (GEqualFunc) cdk_xid_equal);

  if (g_hash_table_lookup (display_x11->xid_ht, xid))
    g_warning ("XID collision, trouble ahead");

  g_hash_table_insert (display_x11->xid_ht, xid, data);
}

void
_cdk_x11_display_remove_window (CdkDisplay *display,
                                XID         xid)
{
  CdkX11Display *display_x11;

  g_return_if_fail (GDK_IS_DISPLAY (display));

  display_x11 = GDK_X11_DISPLAY (display);

  if (display_x11->xid_ht)
    g_hash_table_remove (display_x11->xid_ht, &xid);
}

/**
 * cdk_x11_window_lookup_for_display:
 * @display: (type CdkX11Display): the #CdkDisplay corresponding to the
 *           window handle
 * @window: an Xlib Window
 *
 * Looks up the #CdkWindow that wraps the given native window handle.
 *
 * Returns: (transfer none) (type CdkX11Window): the #CdkWindow wrapper for the native
 *    window, or %NULL if there is none.
 *
 * Since: 2.24
 */
CdkWindow *
cdk_x11_window_lookup_for_display (CdkDisplay *display,
                                   Window      window)
{
  CdkX11Display *display_x11;
  CdkWindow *data = NULL;

  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);

  display_x11 = GDK_X11_DISPLAY (display);

  if (display_x11->xid_ht)
    data = g_hash_table_lookup (display_x11->xid_ht, &window);

  return data;
}
