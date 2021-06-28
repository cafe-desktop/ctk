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

#include "cdkwindowimpl.h"

#include "cdkinternals.h"


G_DEFINE_TYPE (CdkWindowImpl, cdk_window_impl, G_TYPE_OBJECT);

static gboolean
cdk_window_impl_beep (CdkWindow *window)
{
  /* FALSE means windows can't beep, so the display will be
   * made to beep instead. */
  return FALSE;
}

static CdkDisplay *
get_display_for_window (CdkWindow *primary,
                        CdkWindow *secondary)
{
  CdkDisplay *display = cdk_window_get_display (primary);

  if (display)
    return display;

  display = cdk_window_get_display (secondary);

  if (display)
    return display;

  g_warning ("no display for window, using default");
  return cdk_display_get_default ();
}

static CdkMonitor *
get_monitor_for_rect (CdkDisplay         *display,
                      const CdkRectangle *rect)
{
  gint biggest_area = G_MININT;
  CdkMonitor *best_monitor = NULL;
  CdkMonitor *monitor;
  CdkRectangle workarea;
  CdkRectangle intersection;
  gint x;
  gint y;
  gint i;

  for (i = 0; i < cdk_display_get_n_monitors (display); i++)
    {
      monitor = cdk_display_get_monitor (display, i);
      cdk_monitor_get_workarea (monitor, &workarea);

      if (cdk_rectangle_intersect (&workarea, rect, &intersection))
        {
          if (intersection.width * intersection.height > biggest_area)
            {
              biggest_area = intersection.width * intersection.height;
              best_monitor = monitor;
            }
        }
    }

  if (best_monitor)
    return best_monitor;

  x = rect->x + rect->width / 2;
  y = rect->y + rect->height / 2;

  return cdk_display_get_monitor_at_point (display, x, y);
}

static gint
get_anchor_x_sign (CdkGravity anchor)
{
  switch (anchor)
    {
    case CDK_GRAVITY_STATIC:
    case CDK_GRAVITY_NORTH_WEST:
    case CDK_GRAVITY_WEST:
    case CDK_GRAVITY_SOUTH_WEST:
      return -1;

    default:
    case CDK_GRAVITY_NORTH:
    case CDK_GRAVITY_CENTER:
    case CDK_GRAVITY_SOUTH:
      return 0;

    case CDK_GRAVITY_NORTH_EAST:
    case CDK_GRAVITY_EAST:
    case CDK_GRAVITY_SOUTH_EAST:
      return 1;
    }
}

static gint
get_anchor_y_sign (CdkGravity anchor)
{
  switch (anchor)
    {
    case CDK_GRAVITY_STATIC:
    case CDK_GRAVITY_NORTH_WEST:
    case CDK_GRAVITY_NORTH:
    case CDK_GRAVITY_NORTH_EAST:
      return -1;

    default:
    case CDK_GRAVITY_WEST:
    case CDK_GRAVITY_CENTER:
    case CDK_GRAVITY_EAST:
      return 0;

    case CDK_GRAVITY_SOUTH_WEST:
    case CDK_GRAVITY_SOUTH:
    case CDK_GRAVITY_SOUTH_EAST:
      return 1;
    }
}

static gint
maybe_flip_position (gint      bounds_pos,
                     gint      bounds_size,
                     gint      rect_pos,
                     gint      rect_size,
                     gint      window_size,
                     gint      rect_sign,
                     gint      window_sign,
                     gint      offset,
                     gboolean  flip,
                     gboolean *flipped)
{
  gint primary;
  gint secondary;

  *flipped = FALSE;
  primary = rect_pos + (1 + rect_sign) * rect_size / 2 + offset - (1 + window_sign) * window_size / 2;

  if (!flip || (primary >= bounds_pos && primary + window_size <= bounds_pos + bounds_size))
    return primary;

  *flipped = TRUE;
  secondary = rect_pos + (1 - rect_sign) * rect_size / 2 - offset - (1 - window_sign) * window_size / 2;

  if (secondary >= bounds_pos && secondary + window_size <= bounds_pos + bounds_size)
    return secondary;

  *flipped = FALSE;
  return primary;
}

static CdkWindow *
traverse_to_toplevel (CdkWindow *window,
                      gint       x,
                      gint       y,
                      gint      *toplevel_x,
                      gint      *toplevel_y)
{
  CdkWindow *parent;
  gdouble xf = x;
  gdouble yf = y;

  while ((parent = cdk_window_get_effective_parent (window)) != NULL &&
         (cdk_window_get_window_type (parent) != CDK_WINDOW_ROOT))
    {
      cdk_window_coords_to_parent (window, xf, yf, &xf, &yf);
      window = parent;
    }

  *toplevel_x = (gint) xf;
  *toplevel_y = (gint) yf;
  return window;
}

static void
cdk_window_impl_move_to_rect (CdkWindow          *window,
                              const CdkRectangle *rect,
                              CdkGravity          rect_anchor,
                              CdkGravity          window_anchor,
                              CdkAnchorHints      anchor_hints,
                              gint                rect_anchor_dx,
                              gint                rect_anchor_dy)
{
  CdkWindow *transient_for_toplevel;
  CdkDisplay *display;
  CdkMonitor *monitor;
  CdkRectangle bounds;
  CdkRectangle root_rect = *rect;
  CdkRectangle flipped_rect;
  CdkRectangle final_rect;
  gboolean flipped_x;
  gboolean flipped_y;

  /*
   * First translate the anchor rect to toplevel coordinates. This is needed
   * because not all backends will be able to get root coordinates for
   * non-toplevel windows.
   */
  transient_for_toplevel = traverse_to_toplevel (window->transient_for,
                                                 root_rect.x,
                                                 root_rect.y,
                                                 &root_rect.x,
                                                 &root_rect.y);

  cdk_window_get_root_coords (transient_for_toplevel,
                              root_rect.x,
                              root_rect.y,
                              &root_rect.x,
                              &root_rect.y);

  display = get_display_for_window (window, window->transient_for);
  monitor = get_monitor_for_rect (display, &root_rect);
  cdk_monitor_get_workarea (monitor, &bounds);

  flipped_rect.width = window->width - window->shadow_left - window->shadow_right;
  flipped_rect.height = window->height - window->shadow_top - window->shadow_bottom;
  flipped_rect.x = maybe_flip_position (bounds.x,
                                        bounds.width,
                                        root_rect.x,
                                        root_rect.width,
                                        flipped_rect.width,
                                        get_anchor_x_sign (rect_anchor),
                                        get_anchor_x_sign (window_anchor),
                                        rect_anchor_dx,
                                        anchor_hints & CDK_ANCHOR_FLIP_X,
                                        &flipped_x);
  flipped_rect.y = maybe_flip_position (bounds.y,
                                        bounds.height,
                                        root_rect.y,
                                        root_rect.height,
                                        flipped_rect.height,
                                        get_anchor_y_sign (rect_anchor),
                                        get_anchor_y_sign (window_anchor),
                                        rect_anchor_dy,
                                        anchor_hints & CDK_ANCHOR_FLIP_Y,
                                        &flipped_y);

  final_rect = flipped_rect;

  if (anchor_hints & CDK_ANCHOR_SLIDE_X)
    {
      if (final_rect.x + final_rect.width > bounds.x + bounds.width)
        final_rect.x = bounds.x + bounds.width - final_rect.width;

      if (final_rect.x < bounds.x)
        final_rect.x = bounds.x;
    }

  if (anchor_hints & CDK_ANCHOR_SLIDE_Y)
    {
      if (final_rect.y + final_rect.height > bounds.y + bounds.height)
        final_rect.y = bounds.y + bounds.height - final_rect.height;

      if (final_rect.y < bounds.y)
        final_rect.y = bounds.y;
    }

  if (anchor_hints & CDK_ANCHOR_RESIZE_X)
    {
      if (final_rect.x < bounds.x)
        {
          final_rect.width -= bounds.x - final_rect.x;
          final_rect.x = bounds.x;
        }

      if (final_rect.x + final_rect.width > bounds.x + bounds.width)
        final_rect.width = bounds.x + bounds.width - final_rect.x;
    }

  if (anchor_hints & CDK_ANCHOR_RESIZE_Y)
    {
      if (final_rect.y < bounds.y)
        {
          final_rect.height -= bounds.y - final_rect.y;
          final_rect.y = bounds.y;
        }

      if (final_rect.y + final_rect.height > bounds.y + bounds.height)
        final_rect.height = bounds.y + bounds.height - final_rect.y;
    }

  flipped_rect.x -= window->shadow_left;
  flipped_rect.y -= window->shadow_top;
  flipped_rect.width += window->shadow_left + window->shadow_right;
  flipped_rect.height += window->shadow_top + window->shadow_bottom;

  final_rect.x -= window->shadow_left;
  final_rect.y -= window->shadow_top;
  final_rect.width += window->shadow_left + window->shadow_right;
  final_rect.height += window->shadow_top + window->shadow_bottom;

  if (final_rect.width != window->width || final_rect.height != window->height)
    cdk_window_move_resize (window, final_rect.x, final_rect.y, final_rect.width, final_rect.height);
  else
    cdk_window_move (window, final_rect.x, final_rect.y);

  g_signal_emit_by_name (window,
                         "moved-to-rect",
                         &flipped_rect,
                         &final_rect,
                         flipped_x,
                         flipped_y);
}

static void
cdk_window_impl_process_updates_recurse (CdkWindow      *window,
                                         cairo_region_t *region)
{
  _cdk_window_process_updates_recurse (window, region);
}

static void
cdk_window_impl_class_init (CdkWindowImplClass *impl_class)
{
  impl_class->beep = cdk_window_impl_beep;
  impl_class->move_to_rect = cdk_window_impl_move_to_rect;
  impl_class->process_updates_recurse = cdk_window_impl_process_updates_recurse;
}

static void
cdk_window_impl_init (CdkWindowImpl *impl)
{
}
