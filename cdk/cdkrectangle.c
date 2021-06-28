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

#include "cdkrectangle.h"
#include <cairo-gobject.h>


/**
 * SECTION:regions
 * @Short_description: Simple graphical data types
 * @Title: Points and Rectangles
 *
 * GDK provides the #CdkPoint and #CdkRectangle data types for representing pixels
 * and sets of pixels on the screen. Together with Cairo’s #cairo_region_t data
 * type, they make up the central types for representing graphical data.
 *
 * A #CdkPoint represents an x and y coordinate of a point.
 *
 * A #CdkRectangle represents the position and size of a rectangle.
 * The intersection of two rectangles can be computed with
 * cdk_rectangle_intersect(). To find the union of two rectangles use
 * cdk_rectangle_union().
 *
 * #cairo_region_t is usually used for managing clipping of graphical operations.
 */


/**
 * cdk_rectangle_union:
 * @src1: a #CdkRectangle
 * @src2: a #CdkRectangle
 * @dest: (out): return location for the union of @src1 and @src2
 *
 * Calculates the union of two rectangles.
 * The union of rectangles @src1 and @src2 is the smallest rectangle which
 * includes both @src1 and @src2 within it.
 * It is allowed for @dest to be the same as either @src1 or @src2.
 *
 * Note that this function does not ignore 'empty' rectangles (ie. with
 * zero width or height).
 */
void
cdk_rectangle_union (const CdkRectangle *src1,
		     const CdkRectangle *src2,
		     CdkRectangle       *dest)
{
  gint dest_x, dest_y;
  
  g_return_if_fail (src1 != NULL);
  g_return_if_fail (src2 != NULL);
  g_return_if_fail (dest != NULL);

  dest_x = MIN (src1->x, src2->x);
  dest_y = MIN (src1->y, src2->y);
  dest->width = MAX (src1->x + src1->width, src2->x + src2->width) - dest_x;
  dest->height = MAX (src1->y + src1->height, src2->y + src2->height) - dest_y;
  dest->x = dest_x;
  dest->y = dest_y;
}

/**
 * cdk_rectangle_intersect:
 * @src1: a #CdkRectangle
 * @src2: a #CdkRectangle
 * @dest: (out caller-allocates) (allow-none): return location for the
 * intersection of @src1 and @src2, or %NULL
 *
 * Calculates the intersection of two rectangles. It is allowed for
 * @dest to be the same as either @src1 or @src2. If the rectangles 
 * do not intersect, @dest’s width and height is set to 0 and its x 
 * and y values are undefined. If you are only interested in whether
 * the rectangles intersect, but not in the intersecting area itself,
 * pass %NULL for @dest.
 *
 * Returns: %TRUE if the rectangles intersect.
 */
gboolean
cdk_rectangle_intersect (const CdkRectangle *src1,
			 const CdkRectangle *src2,
			 CdkRectangle       *dest)
{
  gint dest_x, dest_y;
  gint dest_x2, dest_y2;
  gint return_val;

  g_return_val_if_fail (src1 != NULL, FALSE);
  g_return_val_if_fail (src2 != NULL, FALSE);

  return_val = FALSE;

  dest_x = MAX (src1->x, src2->x);
  dest_y = MAX (src1->y, src2->y);
  dest_x2 = MIN (src1->x + src1->width, src2->x + src2->width);
  dest_y2 = MIN (src1->y + src1->height, src2->y + src2->height);

  if (dest_x2 > dest_x && dest_y2 > dest_y)
    {
      if (dest)
        {
          dest->x = dest_x;
          dest->y = dest_y;
          dest->width = dest_x2 - dest_x;
          dest->height = dest_y2 - dest_y;
        }
      return_val = TRUE;
    }
  else if (dest)
    {
      dest->width = 0;
      dest->height = 0;
    }

  return return_val;
}

/**
 * cdk_rectangle_equal:
 * @rect1: a #CdkRectangle
 * @rect2: a #CdkRectangle
 *
 * Checks if the two given rectangles are equal.
 *
 * Returns: %TRUE if the rectangles are equal.
 *
 * Since: 3.20
 */
gboolean
cdk_rectangle_equal (const CdkRectangle *rect1,
                     const CdkRectangle *rect2)
{
  return rect1->x == rect2->x
      && rect1->y == rect2->y
      && rect1->width == rect2->width
      && rect1->height == rect2->height;
}

static CdkRectangle *
cdk_rectangle_copy (const CdkRectangle *rectangle)
{
  CdkRectangle *result = g_new (CdkRectangle, 1);
  *result = *rectangle;

  return result;
}

/* Transforms between identical boxed types.
 */
static void
cdk_rectangle_value_transform_rect (const GValue *src_value, GValue *dest_value)
{
  g_value_set_boxed (dest_value, g_value_get_boxed (src_value));
}

/* Allow GValue transformation between the identical structs
 * cairo_rectangle_int_t and CdkRectangle.
 */
static void
cdk_rectangle_register_value_transform_funcs (GType gtype_cdk_rectangle)
{
  /* This function is called from the first call to cdk_rectangle_get_type(),
   * before g_once_init_leave() has been called.
   * If cdk_rectangle_get_type() is called from here (e.g. via
   * GDK_TYPE_RECTANGLE), the program will wait indefinitely at
   * g_once_init_enter() in cdk_rectangle_get_type().
   */
  g_value_register_transform_func (CAIRO_GOBJECT_TYPE_RECTANGLE_INT,
                                   gtype_cdk_rectangle,
                                   cdk_rectangle_value_transform_rect);
  g_value_register_transform_func (gtype_cdk_rectangle,
                                   CAIRO_GOBJECT_TYPE_RECTANGLE_INT,
                                   cdk_rectangle_value_transform_rect);
}

G_DEFINE_BOXED_TYPE_WITH_CODE (CdkRectangle, cdk_rectangle,
                               cdk_rectangle_copy,
                               g_free,
                               cdk_rectangle_register_value_transform_funcs (g_define_type_id))

