/*
 * Copyright (c) 2008-2009  Christian Hammond
 * Copyright (c) 2008-2009  David Trowbridge
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "window.h"
#include "object-tree.h"

#include "ctkstack.h"
#include "ctkmain.h"
#include "ctkinvisible.h"

typedef struct
{
  gint x;
  gint y;
  gboolean found;
  gboolean first;
  CtkWidget *res_widget;
} FindWidgetData;

static void
find_widget (CtkWidget      *widget,
             FindWidgetData *data)
{
  CtkAllocation new_allocation;
  gint x_offset = 0;
  gint y_offset = 0;

  ctk_widget_get_allocation (widget, &new_allocation);

  if (data->found || !ctk_widget_get_mapped (widget))
    return;

  /* Note that in the following code, we only count the
   * position as being inside a WINDOW widget if it is inside
   * widget->window; points that are outside of widget->window
   * but within the allocation are not counted. This is consistent
   * with the way we highlight drag targets.
   */
  if (ctk_widget_get_has_window (widget))
    {
      new_allocation.x = 0;
      new_allocation.y = 0;
    }

  if (ctk_widget_get_parent (widget) && !data->first)
    {
      GdkWindow *window;

      window = ctk_widget_get_window (widget);
      while (window != ctk_widget_get_window (ctk_widget_get_parent (widget)))
        {
          gint tx, ty, twidth, theight;

          if (window == NULL)
            return;

          twidth = cdk_window_get_width (window);
          theight = cdk_window_get_height (window);

          if (new_allocation.x < 0)
            {
              new_allocation.width += new_allocation.x;
              new_allocation.x = 0;
            }
          if (new_allocation.y < 0)
            {
              new_allocation.height += new_allocation.y;
              new_allocation.y = 0;
            }
          if (new_allocation.x + new_allocation.width > twidth)
            new_allocation.width = twidth - new_allocation.x;
          if (new_allocation.y + new_allocation.height > theight)
            new_allocation.height = theight - new_allocation.y;

          cdk_window_get_position (window, &tx, &ty);
          new_allocation.x += tx;
          x_offset += tx;
          new_allocation.y += ty;
          y_offset += ty;

          window = cdk_window_get_parent (window);
        }
    }

  if ((data->x >= new_allocation.x) && (data->y >= new_allocation.y) &&
      (data->x < new_allocation.x + new_allocation.width) &&
      (data->y < new_allocation.y + new_allocation.height))
    {
      /* First, check if the drag is in a valid drop site in
       * one of our children 
       */
      if (CTK_IS_CONTAINER (widget))
        {
          FindWidgetData new_data = *data;

          new_data.x -= x_offset;
          new_data.y -= y_offset;
          new_data.found = FALSE;
          new_data.first = FALSE;

          ctk_container_forall (CTK_CONTAINER (widget),
                                (CtkCallback)find_widget,
                                &new_data);

          data->found = new_data.found;
          if (data->found)
            data->res_widget = new_data.res_widget;
        }

      /* If not, and this widget is registered as a drop site, check to
       * emit "drag_motion" to check if we are actually in
       * a drop site.
       */
      if (!data->found)
        {
          data->found = TRUE;
          data->res_widget = widget;
        }
    }
}

static CtkWidget *
find_widget_at_pointer (GdkDevice *device)
{
  CtkWidget *widget = NULL;
  GdkWindow *pointer_window;
  gint x, y;
  FindWidgetData data;

  pointer_window = cdk_device_get_window_at_position (device, NULL, NULL);

  if (pointer_window)
    {
      gpointer widget_ptr;

      cdk_window_get_user_data (pointer_window, &widget_ptr);
      widget = widget_ptr;
    }

  if (widget)
    {
      cdk_window_get_device_position (ctk_widget_get_window (widget),
                                      device, &x, &y, NULL);

      data.x = x;
      data.y = y;
      data.found = FALSE;
      data.first = TRUE;

      find_widget (widget, &data);
      if (data.found)
        return data.res_widget;

      return widget;
    }

  return NULL;
}

static gboolean draw_flash (CtkWidget          *widget,
                            cairo_t            *cr,
                            CtkInspectorWindow *iw);

static void
clear_flash (CtkInspectorWindow *iw)
{
  if (iw->flash_widget)
    {
      ctk_widget_queue_draw (iw->flash_widget);
      g_signal_handlers_disconnect_by_func (iw->flash_widget, draw_flash, iw);
      g_signal_handlers_disconnect_by_func (iw->flash_widget, clear_flash, iw);
      iw->flash_widget = NULL;
    }
}

static void
start_flash (CtkInspectorWindow *iw,
             CtkWidget          *widget)
{
  clear_flash (iw);

  iw->flash_count = 1;
  iw->flash_widget = widget;
  g_signal_connect_after (widget, "draw", G_CALLBACK (draw_flash), iw);
  g_signal_connect_swapped (widget, "unmap", G_CALLBACK (clear_flash), iw);
  ctk_widget_queue_draw (widget);
}

static void
select_widget (CtkInspectorWindow *iw,
               CtkWidget          *widget)
{
  CtkInspectorObjectTree *wt = CTK_INSPECTOR_OBJECT_TREE (iw->object_tree);

  iw->selected_widget = widget;

  if (!ctk_inspector_object_tree_select_object (wt, G_OBJECT (widget)))
    {
      ctk_inspector_object_tree_scan (wt, ctk_widget_get_toplevel (widget));
      ctk_inspector_object_tree_select_object (wt, G_OBJECT (widget));
    }
}

static void
on_inspect_widget (CtkWidget          *button,
                   GdkEvent           *event,
                   CtkInspectorWindow *iw)
{
  CtkWidget *widget;

  cdk_window_raise (ctk_widget_get_window (CTK_WIDGET (iw)));

  clear_flash (iw);

  widget = find_widget_at_pointer (cdk_event_get_device (event));

  if (widget)
    select_widget (iw, widget);
}

static void
on_highlight_widget (CtkWidget          *button,
                     GdkEvent           *event,
                     CtkInspectorWindow *iw)
{
  CtkWidget *widget;

  widget = find_widget_at_pointer (cdk_event_get_device (event));

  if (widget == NULL)
    {
      /* This window isn't in-process. Ignore it. */
      return;
    }

  if (ctk_widget_get_toplevel (widget) == CTK_WIDGET (iw))
    {
      /* Don't hilight things in the inspector window */
      return;
    }

  if (iw->flash_widget == widget)
    {
      /* Already selected */
      return;
    }

  clear_flash (iw);
  start_flash (iw, widget);
}

static void
deemphasize_window (CtkWidget *window)
{
  GdkScreen *screen;

  screen = ctk_widget_get_screen (window);
  if (cdk_screen_is_composited (screen) &&
      ctk_widget_get_visual (window) == cdk_screen_get_rgba_visual (screen))
    {
      cairo_rectangle_int_t rect;
      cairo_region_t *region;

      ctk_widget_set_opacity (window, 0.3);
      rect.x = rect.y = rect.width = rect.height = 0;
      region = cairo_region_create_rectangle (&rect);
      ctk_widget_input_shape_combine_region (window, region);
      cairo_region_destroy (region);
    }
  else
    cdk_window_lower (ctk_widget_get_window (window));
}

static void
reemphasize_window (CtkWidget *window)
{
  GdkScreen *screen;

  screen = ctk_widget_get_screen (window);
  if (cdk_screen_is_composited (screen) &&
      ctk_widget_get_visual (window) == cdk_screen_get_rgba_visual (screen))
    {
      ctk_widget_set_opacity (window, 1.0);
      ctk_widget_input_shape_combine_region (window, NULL);
    }
  else
    cdk_window_raise (ctk_widget_get_window (window));
}

static gboolean
property_query_event (CtkWidget *widget,
                      GdkEvent  *event,
                      gpointer   data)
{
  CtkInspectorWindow *iw = (CtkInspectorWindow *)data;

  if (event->type == GDK_BUTTON_RELEASE)
    {
      g_signal_handlers_disconnect_by_func (widget, property_query_event, data);
      ctk_grab_remove (widget);
      if (iw->grabbed)
        cdk_seat_ungrab (cdk_event_get_seat (event));
      reemphasize_window (CTK_WIDGET (iw));

      on_inspect_widget (widget, event, data);
    }
  else if (event->type == GDK_MOTION_NOTIFY)
    {
      on_highlight_widget (widget, event, data);
    }
  else if (event->type == GDK_KEY_PRESS)
    {
      GdkEventKey *ke = (GdkEventKey*)event;

      if (ke->keyval == GDK_KEY_Escape)
        {
          g_signal_handlers_disconnect_by_func (widget, property_query_event, data);
          ctk_grab_remove (widget);
          if (iw->grabbed)
            cdk_seat_ungrab (cdk_event_get_seat (event));
          reemphasize_window (CTK_WIDGET (iw));

          clear_flash (iw);
        }
    }

  return TRUE;
}

void
ctk_inspector_on_inspect (CtkWidget          *button,
                          CtkInspectorWindow *iw)
{
  GdkDisplay *display;
  GdkCursor *cursor;
  GdkGrabStatus status;

  if (!iw->invisible)
    {
      iw->invisible = ctk_invisible_new_for_screen (cdk_screen_get_default ());
      ctk_widget_add_events (iw->invisible,
                             GDK_POINTER_MOTION_MASK |
                             GDK_BUTTON_PRESS_MASK |
                             GDK_BUTTON_RELEASE_MASK |
                             GDK_KEY_PRESS_MASK |
                             GDK_KEY_RELEASE_MASK);
      ctk_widget_realize (iw->invisible);
      ctk_widget_show (iw->invisible);
    }

  display = cdk_display_get_default ();
  cursor = cdk_cursor_new_from_name (display, "crosshair");
  status = cdk_seat_grab (cdk_display_get_default_seat (display),
                          ctk_widget_get_window (iw->invisible),
                          GDK_SEAT_CAPABILITY_ALL_POINTING, TRUE,
                          cursor, NULL, NULL, NULL);
  g_object_unref (cursor);
  iw->grabbed = status == GDK_GRAB_SUCCESS;

  g_signal_connect (iw->invisible, "event", G_CALLBACK (property_query_event), iw);

  ctk_grab_add (CTK_WIDGET (iw->invisible));
  deemphasize_window (CTK_WIDGET (iw));
}

static gboolean
draw_flash (CtkWidget          *widget,
            cairo_t            *cr,
            CtkInspectorWindow *iw)
{
  CtkAllocation alloc;

  if (iw && iw->flash_count % 2 == 0)
    return FALSE;

  if (CTK_IS_WINDOW (widget))
    {
      CtkWidget *child = ctk_bin_get_child (CTK_BIN (widget));
      /* We don't want to draw the drag highlight around the
       * CSD window decorations
       */
      if (child == NULL)
        return FALSE;

      ctk_widget_get_allocation (child, &alloc);
    }
  else
    {
      alloc.x = 0;
      alloc.y = 0;
      alloc.width = ctk_widget_get_allocated_width (widget);
      alloc.height = ctk_widget_get_allocated_height (widget);
    }

  cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 0.2);
  cairo_rectangle (cr,
                   alloc.x + 0.5, alloc.y + 0.5,
                   alloc.width - 1, alloc.height - 1);
  cairo_fill (cr);

  return FALSE;
}

static gboolean
on_flash_timeout (CtkInspectorWindow *iw)
{
  ctk_widget_queue_draw (iw->flash_widget);

  iw->flash_count++;

  if (iw->flash_count == 6)
    {
      g_signal_handlers_disconnect_by_func (iw->flash_widget, draw_flash, iw);
      g_signal_handlers_disconnect_by_func (iw->flash_widget, clear_flash, iw);
      iw->flash_widget = NULL;
      iw->flash_cnx = 0;

      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}

void
ctk_inspector_flash_widget (CtkInspectorWindow *iw,
                            CtkWidget          *widget)
{
  if (!ctk_widget_get_visible (widget) || !ctk_widget_get_mapped (widget))
    return;

  if (iw->flash_cnx != 0)
    {
      g_source_remove (iw->flash_cnx);
      iw->flash_cnx = 0;
    }

  start_flash (iw, widget);
  iw->flash_cnx = g_timeout_add (150, (GSourceFunc) on_flash_timeout, iw);
}

void
ctk_inspector_start_highlight (CtkWidget *widget)
{
  g_signal_connect_after (widget, "draw", G_CALLBACK (draw_flash), NULL);
  ctk_widget_queue_draw (widget);
}

void
ctk_inspector_stop_highlight (CtkWidget *widget)
{
  g_signal_handlers_disconnect_by_func (widget, draw_flash, NULL);
  g_signal_handlers_disconnect_by_func (widget, clear_flash, NULL);
  ctk_widget_queue_draw (widget);
}

void
ctk_inspector_window_select_widget_under_pointer (CtkInspectorWindow *iw)
{
  GdkDisplay *display;
  GdkDevice *device;
  CtkWidget *widget;

  display = cdk_display_get_default ();
  device = cdk_seat_get_pointer (cdk_display_get_default_seat (display));

  widget = find_widget_at_pointer (device);

  if (widget)
    select_widget (iw, widget);
}

/* vim: set et sw=2 ts=2: */
