/* CDK - The GIMP Drawing Kit
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

/* needs to be first because any header might include gdk-pixbuf.h otherwise */
#define GDK_PIXBUF_ENABLE_BACKEND
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "cdkcursor.h"
#include "cdkcursorprivate.h"

#include "cdkprivate-broadway.h"
#include "cdkdisplay-broadway.h"

#include <string.h>
#include <errno.h>

struct _CdkBroadwayCursor
{
  CdkCursor cursor;
};

struct _CdkBroadwayCursorClass
{
  CdkCursorClass cursor_class;
};

/*** CdkBroadwayCursor ***/

G_DEFINE_TYPE (CdkBroadwayCursor, cdk_broadway_cursor, CDK_TYPE_CURSOR)

static cairo_surface_t * cdk_broadway_cursor_get_surface (CdkCursor *cursor,
							  gdouble   *x_hot,
							  gdouble   *y_hot);

static void
cdk_broadway_cursor_finalize (GObject *object)
{
  G_OBJECT_CLASS (cdk_broadway_cursor_parent_class)->finalize (object);
}

static void
cdk_broadway_cursor_class_init (CdkBroadwayCursorClass *xcursor_class)
{
  CdkCursorClass *cursor_class = CDK_CURSOR_CLASS (xcursor_class);
  GObjectClass *object_class = G_OBJECT_CLASS (xcursor_class);

  object_class->finalize = cdk_broadway_cursor_finalize;

  cursor_class->get_surface = cdk_broadway_cursor_get_surface;
}

static void
cdk_broadway_cursor_init (CdkBroadwayCursor *cursor)
{
}

/* Called by cdk_display_broadway_finalize to flush any cached cursors
 * for a dead display.
 */
void
_cdk_broadway_cursor_display_finalize (CdkDisplay *display)
{
}

CdkCursor*
_cdk_broadway_display_get_cursor_for_type (CdkDisplay    *display,
					   CdkCursorType  cursor_type)
{
  CdkBroadwayCursor *private;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  private = g_object_new (CDK_TYPE_BROADWAY_CURSOR,
                          "cursor-type", cursor_type,
                          "display", display,
			  NULL);

  return CDK_CURSOR (private);
}

static cairo_surface_t *
cdk_broadway_cursor_get_surface (CdkCursor *cursor,
				 gdouble *x_hot,
				 gdouble *y_hot)
{
  g_return_val_if_fail (cursor != NULL, NULL);

  return NULL;
}

void
_cdk_broadway_cursor_update_theme (CdkCursor *cursor)
{
  g_return_if_fail (cursor != NULL);
}

CdkCursor *
_cdk_broadway_display_get_cursor_for_surface (CdkDisplay *display,
					      cairo_surface_t *surface,
					      gdouble     x,
					      gdouble     y)
{
  CdkBroadwayCursor *private;
  CdkCursor *cursor;

  private = g_object_new (CDK_TYPE_BROADWAY_CURSOR, 
                          "cursor-type", CDK_CURSOR_IS_PIXMAP,
                          "display", display,
                          NULL);
  cursor = (CdkCursor *) private;

  return cursor;
}

CdkCursor*
_cdk_broadway_display_get_cursor_for_name (CdkDisplay  *display,
					   const gchar *name)
{
  CdkBroadwayCursor *private;

  private = g_object_new (CDK_TYPE_BROADWAY_CURSOR,
                          "cursor-type", CDK_CURSOR_IS_PIXMAP,
                          "display", display,
                          NULL);

  return CDK_CURSOR (private);
}

gboolean
_cdk_broadway_display_supports_cursor_alpha (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return TRUE;
}

gboolean
_cdk_broadway_display_supports_cursor_color (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return TRUE;
}

void
_cdk_broadway_display_get_default_cursor_size (CdkDisplay *display,
					       guint      *width,
					       guint      *height)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));

  *width = *height = 20;
}

void
_cdk_broadway_display_get_maximal_cursor_size (CdkDisplay *display,
					       guint       *width,
					       guint       *height)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));

  *width = 128;
  *height = 128;
}
