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

#define GDK_PIXBUF_ENABLE_BACKEND
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "cdkcursor.h"
#include "cdkcursorprivate.h"
#include "cdkdisplayprivate.h"
#include "cdkintl.h"
#include "cdkinternals.h"

#include <math.h>
#include <errno.h>

/**
 * SECTION:cursors
 * @Short_description: Standard and pixmap cursors
 * @Title: Cursors
 *
 * These functions are used to create and destroy cursors.
 * There is a number of standard cursors, but it is also
 * possible to construct new cursors from pixbufs. There
 * may be limitations as to what kinds of cursors can be
 * constructed on a given display, see
 * cdk_display_supports_cursor_alpha(),
 * cdk_display_supports_cursor_color(),
 * cdk_display_get_default_cursor_size() and
 * cdk_display_get_maximal_cursor_size().
 *
 * Cursors by themselves are not very interesting, they must be be
 * bound to a window for users to see them. This is done with
 * cdk_window_set_cursor() or by setting the cursor member of the
 * #CdkWindowAttr passed to cdk_window_new().
 */

/**
 * CdkCursor:
 *
 * A #CdkCursor represents a cursor. Its contents are private.
 */

enum {
  PROP_0,
  PROP_CURSOR_TYPE,
  PROP_DISPLAY
};

G_DEFINE_ABSTRACT_TYPE (CdkCursor, cdk_cursor, G_TYPE_OBJECT)

static void
cdk_cursor_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  CdkCursor *cursor = CDK_CURSOR (object);

  switch (prop_id)
    {
    case PROP_CURSOR_TYPE:
      g_value_set_enum (value, cursor->type);
      break;
    case PROP_DISPLAY:
      g_value_set_object (value, cursor->display);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
cdk_cursor_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  CdkCursor *cursor = CDK_CURSOR (object);

  switch (prop_id)
    {
    case PROP_CURSOR_TYPE:
      cursor->type = g_value_get_enum (value);
      break;
    case PROP_DISPLAY:
      cursor->display = g_value_get_object (value);
      /* check that implementations actually provide the display when constructing */
      g_assert (cursor->display != NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
cdk_cursor_class_init (CdkCursorClass *cursor_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (cursor_class);

  object_class->get_property = cdk_cursor_get_property;
  object_class->set_property = cdk_cursor_set_property;

  g_object_class_install_property (object_class,
				   PROP_CURSOR_TYPE,
				   g_param_spec_enum ("cursor-type",
                                                      P_("Cursor type"),
                                                      P_("Standard cursor type"),
                                                      CDK_TYPE_CURSOR_TYPE, CDK_X_CURSOR,
                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
				   PROP_DISPLAY,
				   g_param_spec_object ("display",
                                                        P_("Display"),
                                                        P_("Display of this cursor"),
                                                        CDK_TYPE_DISPLAY,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
cdk_cursor_init (CdkCursor *cursor G_GNUC_UNUSED)
{
}

/**
 * cdk_cursor_ref:
 * @cursor: a #CdkCursor
 *
 * Adds a reference to @cursor.
 *
 * Returns: (transfer full): Same @cursor that was passed in
 *
 * Deprecated: 3.0: Use g_object_ref() instead
 */
CdkCursor*
cdk_cursor_ref (CdkCursor *cursor)
{
  g_return_val_if_fail (cursor != NULL, NULL);

  return g_object_ref (cursor);
}

/**
 * cdk_cursor_unref:
 * @cursor: a #CdkCursor
 *
 * Removes a reference from @cursor, deallocating the cursor
 * if no references remain.
 *
 * Deprecated: 3.0: Use g_object_unref() instead
 */
void
cdk_cursor_unref (CdkCursor *cursor)
{
  g_return_if_fail (cursor != NULL);

  g_object_unref (cursor);
}

/**
 * cdk_cursor_new:
 * @cursor_type: cursor to create
 *
 * Creates a new cursor from the set of builtin cursors for the default display.
 * See cdk_cursor_new_for_display().
 *
 * To make the cursor invisible, use %CDK_BLANK_CURSOR.
 *
 * Returns: a new #CdkCursor
 *
 * Deprecated: 3.16: Use cdk_cursor_new_for_display() instead.
 */
CdkCursor*
cdk_cursor_new (CdkCursorType cursor_type)
{
  return cdk_cursor_new_for_display (cdk_display_get_default (), cursor_type);
}

/**
 * cdk_cursor_get_cursor_type:
 * @cursor:  a #CdkCursor
 *
 * Returns the cursor type for this cursor.
 *
 * Returns: a #CdkCursorType
 *
 * Since: 2.22
 **/
CdkCursorType
cdk_cursor_get_cursor_type (CdkCursor *cursor)
{
  g_return_val_if_fail (cursor != NULL, CDK_BLANK_CURSOR);

  return cursor->type;
}

/**
 * cdk_cursor_new_for_display:
 * @display: the #CdkDisplay for which the cursor will be created
 * @cursor_type: cursor to create
 *
 * Creates a new cursor from the set of builtin cursors.
 *
 * Returns: (nullable) (transfer full): a new #CdkCursor, or %NULL on failure
 *
 * Since: 2.2
 **/
CdkCursor*
cdk_cursor_new_for_display (CdkDisplay    *display,
                            CdkCursorType  cursor_type)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return CDK_DISPLAY_GET_CLASS (display)->get_cursor_for_type (display, cursor_type);
}

/**
 * cdk_cursor_new_from_name:
 * @display: the #CdkDisplay for which the cursor will be created
 * @name: the name of the cursor
 *
 * Creates a new cursor by looking up @name in the current cursor
 * theme.
 *
 * A recommended set of cursor names that will work across different
 * platforms can be found in the CSS specification:
 * - "none"
 * - ![](default_cursor.png) "default"
 * - ![](help_cursor.png) "help"
 * - ![](pointer_cursor.png) "pointer"
 * - ![](context_menu_cursor.png) "context-menu"
 * - ![](progress_cursor.png) "progress"
 * - ![](wait_cursor.png) "wait"
 * - ![](cell_cursor.png) "cell"
 * - ![](crosshair_cursor.png) "crosshair"
 * - ![](text_cursor.png) "text"
 * - ![](vertical_text_cursor.png) "vertical-text"
 * - ![](alias_cursor.png) "alias"
 * - ![](copy_cursor.png) "copy"
 * - ![](no_drop_cursor.png) "no-drop"
 * - ![](move_cursor.png) "move"
 * - ![](not_allowed_cursor.png) "not-allowed"
 * - ![](grab_cursor.png) "grab"
 * - ![](grabbing_cursor.png) "grabbing"
 * - ![](all_scroll_cursor.png) "all-scroll"
 * - ![](col_resize_cursor.png) "col-resize"
 * - ![](row_resize_cursor.png) "row-resize"
 * - ![](n_resize_cursor.png) "n-resize"
 * - ![](e_resize_cursor.png) "e-resize"
 * - ![](s_resize_cursor.png) "s-resize"
 * - ![](w_resize_cursor.png) "w-resize"
 * - ![](ne_resize_cursor.png) "ne-resize"
 * - ![](nw_resize_cursor.png) "nw-resize"
 * - ![](sw_resize_cursor.png) "sw-resize"
 * - ![](se_resize_cursor.png) "se-resize"
 * - ![](ew_resize_cursor.png) "ew-resize"
 * - ![](ns_resize_cursor.png) "ns-resize"
 * - ![](nesw_resize_cursor.png) "nesw-resize"
 * - ![](nwse_resize_cursor.png) "nwse-resize"
 * - ![](zoom_in_cursor.png) "zoom-in"
 * - ![](zoom_out_cursor.png) "zoom-out"
 *
 *
 * Returns: (nullable): a new #CdkCursor, or %NULL if there is no
 *   cursor with the given name
 *
 * Since: 2.8
 */
CdkCursor*
cdk_cursor_new_from_name (CdkDisplay  *display,
                          const gchar *name)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return CDK_DISPLAY_GET_CLASS (display)->get_cursor_for_name (display, name);
}

/**
 * cdk_cursor_new_from_pixbuf:
 * @display: the #CdkDisplay for which the cursor will be created
 * @pixbuf: the #GdkPixbuf containing the cursor image
 * @x: the horizontal offset of the “hotspot” of the cursor.
 * @y: the vertical offset of the “hotspot” of the cursor.
 *
 * Creates a new cursor from a pixbuf.
 *
 * Not all CDK backends support RGBA cursors. If they are not
 * supported, a monochrome approximation will be displayed.
 * The functions cdk_display_supports_cursor_alpha() and
 * cdk_display_supports_cursor_color() can be used to determine
 * whether RGBA cursors are supported;
 * cdk_display_get_default_cursor_size() and
 * cdk_display_get_maximal_cursor_size() give information about
 * cursor sizes.
 *
 * If @x or @y are `-1`, the pixbuf must have
 * options named “x_hot” and “y_hot”, resp., containing
 * integer values between `0` and the width resp. height of
 * the pixbuf. (Since: 3.0)
 *
 * On the X backend, support for RGBA cursors requires a
 * sufficently new version of the X Render extension.
 *
 * Returns: a new #CdkCursor.
 *
 * Since: 2.4
 */
CdkCursor *
cdk_cursor_new_from_pixbuf (CdkDisplay *display,
                            GdkPixbuf  *pixbuf,
                            gint        x,
                            gint        y)
{
  cairo_surface_t *surface;
  const char *option;
  char *end;
  gint64 value;
  CdkCursor *cursor;
 
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  if (x == -1 && (option = gdk_pixbuf_get_option (pixbuf, "x_hot")))
    {
      errno = 0;
      end = NULL;
      value = g_ascii_strtoll (option, &end, 10);
      if (errno == 0 &&
          end != option &&
          value >= 0 && value < G_MAXINT)
        x = (gint) value;
    }
  
  if (y == -1 && (option = gdk_pixbuf_get_option (pixbuf, "y_hot")))
    {
      errno = 0;
      end = NULL;
      value = g_ascii_strtoll (option, &end, 10);
      if (errno == 0 &&
          end != option &&
          value >= 0 && value < G_MAXINT)
        y = (gint) value;
    }

  surface = cdk_cairo_surface_create_from_pixbuf (pixbuf, 1, NULL);
  
  cursor = CDK_DISPLAY_GET_CLASS (display)->get_cursor_for_surface (display, surface, x, y);

  cairo_surface_destroy (surface);

  return cursor;
}

/**
 * cdk_cursor_new_from_surface:
 * @display: the #CdkDisplay for which the cursor will be created
 * @surface: the cairo image surface containing the cursor pixel data
 * @x: the horizontal offset of the “hotspot” of the cursor
 * @y: the vertical offset of the “hotspot” of the cursor
 *
 * Creates a new cursor from a cairo image surface.
 *
 * Not all CDK backends support RGBA cursors. If they are not
 * supported, a monochrome approximation will be displayed.
 * The functions cdk_display_supports_cursor_alpha() and
 * cdk_display_supports_cursor_color() can be used to determine
 * whether RGBA cursors are supported;
 * cdk_display_get_default_cursor_size() and
 * cdk_display_get_maximal_cursor_size() give information about
 * cursor sizes.
 *
 * On the X backend, support for RGBA cursors requires a
 * sufficently new version of the X Render extension.
 *
 * Returns: a new #CdkCursor.
 *
 * Since: 3.10
 */
CdkCursor *
cdk_cursor_new_from_surface (CdkDisplay      *display,
			     cairo_surface_t *surface,
			     gdouble          x,
			     gdouble          y)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (surface != NULL, NULL);
  g_return_val_if_fail (cairo_surface_get_type (surface) == CAIRO_SURFACE_TYPE_IMAGE, NULL);
  g_return_val_if_fail (0 <= x && x < cairo_image_surface_get_width (surface), NULL);
  g_return_val_if_fail (0 <= y && y < cairo_image_surface_get_height (surface), NULL);

  return CDK_DISPLAY_GET_CLASS (display)->get_cursor_for_surface (display,
								  surface, x, y);
}
/**
 * cdk_cursor_get_display:
 * @cursor: a #CdkCursor.
 *
 * Returns the display on which the #CdkCursor is defined.
 *
 * Returns: (transfer none): the #CdkDisplay associated to @cursor
 *
 * Since: 2.2
 */

CdkDisplay *
cdk_cursor_get_display (CdkCursor *cursor)
{
  g_return_val_if_fail (CDK_IS_CURSOR (cursor), NULL);

  return cursor->display;
}

/**
 * cdk_cursor_get_image:
 * @cursor: a #CdkCursor
 *
 * Returns a #GdkPixbuf with the image used to display the cursor.
 *
 * Note that depending on the capabilities of the windowing system and 
 * on the cursor, CDK may not be able to obtain the image data. In this 
 * case, %NULL is returned.
 *
 * Returns: (nullable) (transfer full): a #GdkPixbuf representing
 *   @cursor, or %NULL
 *
 * Since: 2.8
 */
GdkPixbuf*  
cdk_cursor_get_image (CdkCursor *cursor)
{
  int w, h;
  cairo_surface_t *surface;
  GdkPixbuf *pixbuf;
  gchar buf[32];
  double x_hot, y_hot;
  double x_scale, y_scale;

  g_return_val_if_fail (CDK_IS_CURSOR (cursor), NULL);

  surface = cdk_cursor_get_surface (cursor, &x_hot, &y_hot);
  if (surface == NULL)
    return NULL;

  w = cairo_image_surface_get_width (surface);
  h = cairo_image_surface_get_height (surface);

  x_scale = y_scale = 1;
  cairo_surface_get_device_scale (surface, &x_scale, &y_scale);

  pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0, w, h);
  cairo_surface_destroy (surface);

  if (x_scale != 1)
    {
      GdkPixbuf *old;

      old = pixbuf;
      pixbuf = gdk_pixbuf_scale_simple (old,
					w / x_scale, h / y_scale,
					GDK_INTERP_HYPER);
      g_object_unref (old);
    }

  
  g_snprintf (buf, 32, "%d", (int)x_hot);
  gdk_pixbuf_set_option (pixbuf, "x_hot", buf);

  g_snprintf (buf, 32, "%d", (int)y_hot);
  gdk_pixbuf_set_option (pixbuf, "y_hot", buf);

  return pixbuf;
}

/**
 * cdk_cursor_get_surface:
 * @cursor: a #CdkCursor
 * @x_hot: (optional) (out): Location to store the hotspot x position,
 *   or %NULL
 * @y_hot: (optional) (out): Location to store the hotspot y position,
 *   or %NULL
 *
 * Returns a cairo image surface with the image used to display the cursor.
 *
 * Note that depending on the capabilities of the windowing system and
 * on the cursor, CDK may not be able to obtain the image data. In this
 * case, %NULL is returned.
 *
 * Returns: (nullable) (transfer full): a #cairo_surface_t
 *   representing @cursor, or %NULL
 *
 * Since: 3.10
 */
cairo_surface_t *
cdk_cursor_get_surface (CdkCursor *cursor,
			gdouble   *x_hot,
			gdouble   *y_hot)
{
  g_return_val_if_fail (CDK_IS_CURSOR (cursor), NULL);

  return CDK_CURSOR_GET_CLASS (cursor)->get_surface (cursor, x_hot, y_hot);
}
