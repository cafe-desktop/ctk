/*
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
 *
 * Authors: Cody Russell <crussell@canonical.com>
 *          Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"

#include "ctkoffscreenwindow.h"
#include "ctkwidgetprivate.h"
#include "ctkcontainerprivate.h"
#include "ctkprivate.h"

/**
 * SECTION:ctkoffscreenwindow
 * @short_description: A toplevel to manage offscreen rendering of child widgets
 * @title: CtkOffscreenWindow
 *
 * CtkOffscreenWindow is strictly intended to be used for obtaining
 * snapshots of widgets that are not part of a normal widget hierarchy.
 * Since #CtkOffscreenWindow is a toplevel widget you cannot obtain
 * snapshots of a full window with it since you cannot pack a toplevel
 * widget in another toplevel.
 *
 * The idea is to take a widget and manually set the state of it,
 * add it to a CtkOffscreenWindow and then retrieve the snapshot
 * as a #cairo_surface_t or #CdkPixbuf.
 *
 * CtkOffscreenWindow derives from #CtkWindow only as an implementation
 * detail.  Applications should not use any API specific to #CtkWindow
 * to operate on this object.  It should be treated as a #CtkBin that
 * has no parent widget.
 *
 * When contained offscreen widgets are redrawn, CtkOffscreenWindow
 * will emit a #CtkWidget::damage-event signal.
 */

G_DEFINE_TYPE (CtkOffscreenWindow, ctk_offscreen_window, CTK_TYPE_WINDOW);

static void
ctk_offscreen_window_get_preferred_width (CtkWidget *widget,
					  gint      *minimum,
					  gint      *natural)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkWidget *child;
  gint border_width;
  gint default_width;

  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));

  *minimum = border_width * 2;
  *natural = border_width * 2;

  child = ctk_bin_get_child (bin);

  if (child != NULL && ctk_widget_get_visible (child))
    {
      gint child_min, child_nat;

      ctk_widget_get_preferred_width (child, &child_min, &child_nat);

      *minimum += child_min;
      *natural += child_nat;
    }

  ctk_window_get_default_size (CTK_WINDOW (widget),
                               &default_width, NULL);

  *minimum = MAX (*minimum, default_width);
  *natural = MAX (*natural, default_width);
}

static void
ctk_offscreen_window_get_preferred_height (CtkWidget *widget,
					   gint      *minimum,
					   gint      *natural)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkWidget *child;
  gint border_width;
  gint default_height;

  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));

  *minimum = border_width * 2;
  *natural = border_width * 2;

  child = ctk_bin_get_child (bin);

  if (child != NULL && ctk_widget_get_visible (child))
    {
      gint child_min, child_nat;

      ctk_widget_get_preferred_height (child, &child_min, &child_nat);

      *minimum += child_min;
      *natural += child_nat;
    }

  ctk_window_get_default_size (CTK_WINDOW (widget),
                               NULL, &default_height);

  *minimum = MAX (*minimum, default_height);
  *natural = MAX (*natural, default_height);
}

static void
ctk_offscreen_window_size_allocate (CtkWidget *widget,
                                    CtkAllocation *allocation)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkWidget *child;
  gint border_width;

  ctk_widget_set_allocation (widget, allocation);

  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));

  if (ctk_widget_get_realized (widget))
    cdk_window_move_resize (ctk_widget_get_window (widget),
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);

  child = ctk_bin_get_child (bin);

  if (child != NULL && ctk_widget_get_visible (child))
    {
      CtkAllocation  child_alloc;

      child_alloc.x = border_width;
      child_alloc.y = border_width;
      child_alloc.width = allocation->width - 2 * border_width;
      child_alloc.height = allocation->height - 2 * border_width;

      ctk_widget_size_allocate (child, &child_alloc);
    }

  ctk_widget_queue_draw (widget);
}

static void
ctk_offscreen_window_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  CtkBin *bin;
  CtkWidget *child;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;

  bin = CTK_BIN (widget);

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = GDK_WINDOW_OFFSCREEN;
  attributes.event_mask = ctk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.wclass = GDK_INPUT_OUTPUT;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  window = cdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);
  ctk_widget_register_window (widget, window);

  child = ctk_bin_get_child (bin);
  if (child)
    ctk_widget_set_parent_window (child, window);
}

static void
ctk_offscreen_window_resize (CtkWidget *widget)
{
  CtkAllocation allocation = { 0, 0 };
  CtkRequisition requisition;

  ctk_widget_get_preferred_size (widget, &requisition, NULL);

  allocation.width  = requisition.width;
  allocation.height = requisition.height;
  ctk_widget_size_allocate (widget, &allocation);
}

static void
move_focus (CtkWidget       *widget,
            CtkDirectionType dir)
{
  ctk_widget_child_focus (widget, dir);

  if (!ctk_container_get_focus_child (CTK_CONTAINER (widget)))
    ctk_window_set_focus (CTK_WINDOW (widget), NULL);
}

static void
ctk_offscreen_window_show (CtkWidget *widget)
{
  gboolean need_resize;

  _ctk_widget_set_visible_flag (widget, TRUE);

  need_resize = _ctk_widget_get_alloc_needed (widget) || !ctk_widget_get_realized (widget);

  if (need_resize)
    ctk_offscreen_window_resize (widget);

  ctk_widget_map (widget);

  /* Try to make sure that we have some focused widget */
  if (!ctk_window_get_focus (CTK_WINDOW (widget)))
    move_focus (widget, CTK_DIR_TAB_FORWARD);
}

static void
ctk_offscreen_window_hide (CtkWidget *widget)
{
  _ctk_widget_set_visible_flag (widget, FALSE);
  ctk_widget_unmap (widget);
}

static void
ctk_offscreen_window_check_resize (CtkContainer *container)
{
  CtkWidget *widget = CTK_WIDGET (container);

  if (ctk_widget_get_visible (widget))
    ctk_offscreen_window_resize (widget);
}

static void
ctk_offscreen_window_class_init (CtkOffscreenWindowClass *class)
{
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;

  widget_class = CTK_WIDGET_CLASS (class);
  container_class = CTK_CONTAINER_CLASS (class);

  widget_class->realize = ctk_offscreen_window_realize;
  widget_class->show = ctk_offscreen_window_show;
  widget_class->hide = ctk_offscreen_window_hide;
  widget_class->get_preferred_width = ctk_offscreen_window_get_preferred_width;
  widget_class->get_preferred_height = ctk_offscreen_window_get_preferred_height;
  widget_class->size_allocate = ctk_offscreen_window_size_allocate;

  container_class->check_resize = ctk_offscreen_window_check_resize;
}

static void
ctk_offscreen_window_init (CtkOffscreenWindow *window)
{
}

/* --- functions --- */
/**
 * ctk_offscreen_window_new:
 *
 * Creates a toplevel container widget that is used to retrieve
 * snapshots of widgets without showing them on the screen.
 *
 * Returns: A pointer to a #CtkWidget
 *
 * Since: 2.20
 */
CtkWidget *
ctk_offscreen_window_new (void)
{
  return g_object_new (ctk_offscreen_window_get_type (), NULL);
}

/**
 * ctk_offscreen_window_get_surface:
 * @offscreen: the #CtkOffscreenWindow contained widget.
 *
 * Retrieves a snapshot of the contained widget in the form of
 * a #cairo_surface_t.  If you need to keep this around over window
 * resizes then you should add a reference to it.
 *
 * Returns: (nullable) (transfer none): A #cairo_surface_t pointer to the offscreen
 *     surface, or %NULL.
 *
 * Since: 2.20
 */
cairo_surface_t *
ctk_offscreen_window_get_surface (CtkOffscreenWindow *offscreen)
{
  g_return_val_if_fail (CTK_IS_OFFSCREEN_WINDOW (offscreen), NULL);

  return cdk_offscreen_window_get_surface (ctk_widget_get_window (CTK_WIDGET (offscreen)));
}

/**
 * ctk_offscreen_window_get_pixbuf:
 * @offscreen: the #CtkOffscreenWindow contained widget.
 *
 * Retrieves a snapshot of the contained widget in the form of
 * a #CdkPixbuf.  This is a new pixbuf with a reference count of 1,
 * and the application should unreference it once it is no longer
 * needed.
 *
 * Returns: (nullable) (transfer full): A #CdkPixbuf pointer, or %NULL.
 *
 * Since: 2.20
 */
CdkPixbuf *
ctk_offscreen_window_get_pixbuf (CtkOffscreenWindow *offscreen)
{
  cairo_surface_t *surface;
  CdkPixbuf *pixbuf = NULL;
  CdkWindow *window;

  g_return_val_if_fail (CTK_IS_OFFSCREEN_WINDOW (offscreen), NULL);

  window = ctk_widget_get_window (CTK_WIDGET (offscreen));
  surface = cdk_offscreen_window_get_surface (window);

  if (surface != NULL)
    {
      pixbuf = cdk_pixbuf_get_from_surface (surface,
                                            0, 0,
                                            cdk_window_get_width (window),
                                            cdk_window_get_height (window));
    }

  return pixbuf;
}
