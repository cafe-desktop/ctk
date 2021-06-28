/* ctktooltip.c
 *
 * Copyright (C) 2006-2007 Imendio AB
 * Contact: Kristian Rietveld <kris@imendio.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ctktooltip.h"
#include "ctktooltipprivate.h"

#include <math.h>
#include <string.h>

#include "ctkintl.h"
#include "ctkwindow.h"
#include "ctkmain.h"
#include "ctklabel.h"
#include "ctkimage.h"
#include "ctkbox.h"
#include "ctksettings.h"
#include "ctksizerequest.h"
#include "ctkstylecontext.h"
#include "ctktooltipwindowprivate.h"
#include "ctkwindowprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkaccessible.h"

#ifdef CDK_WINDOWING_WAYLAND
#include "wayland/cdkwayland.h"
#endif


/**
 * SECTION:ctktooltip
 * @Short_description: Add tips to your widgets
 * @Title: CtkTooltip
 *
 * Basic tooltips can be realized simply by using ctk_widget_set_tooltip_text()
 * or ctk_widget_set_tooltip_markup() without any explicit tooltip object.
 *
 * When you need a tooltip with a little more fancy contents, like adding an
 * image, or you want the tooltip to have different contents per #CtkTreeView
 * row or cell, you will have to do a little more work:
 * 
 * - Set the #CtkWidget:has-tooltip property to %TRUE, this will make CTK+
 *   monitor the widget for motion and related events which are needed to
 *   determine when and where to show a tooltip.
 *
 * - Connect to the #CtkWidget::query-tooltip signal.  This signal will be
 *   emitted when a tooltip is supposed to be shown. One of the arguments passed
 *   to the signal handler is a CtkTooltip object. This is the object that we
 *   are about to display as a tooltip, and can be manipulated in your callback
 *   using functions like ctk_tooltip_set_icon(). There are functions for setting
 *   the tooltip’s markup, setting an image from a named icon, or even putting in
 *   a custom widget.
 *
 *   Return %TRUE from your query-tooltip handler. This causes the tooltip to be
 *   show. If you return %FALSE, it will not be shown.
 *
 * In the probably rare case where you want to have even more control over the
 * tooltip that is about to be shown, you can set your own #CtkWindow which
 * will be used as tooltip window.  This works as follows:
 * 
 * - Set #CtkWidget:has-tooltip and connect to #CtkWidget::query-tooltip as before.
 *   Use ctk_widget_set_tooltip_window() to set a #CtkWindow created by you as
 *   tooltip window.
 *
 * - In the #CtkWidget::query-tooltip callback you can access your window using
 *   ctk_widget_get_tooltip_window() and manipulate as you wish. The semantics of
 *   the return value are exactly as before, return %TRUE to show the window,
 *   %FALSE to not show it.
 */


#define HOVER_TIMEOUT          500
#define BROWSE_TIMEOUT         60
#define BROWSE_DISABLE_TIMEOUT 500

#define CTK_TOOLTIP_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TOOLTIP, CtkTooltipClass))
#define CTK_IS_TOOLTIP_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TOOLTIP))
#define CTK_TOOLTIP_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TOOLTIP, CtkTooltipClass))

typedef struct _CtkTooltipClass   CtkTooltipClass;

struct _CtkTooltip
{
  GObject parent_instance;

  CtkWidget *window;
  CtkWidget *box;
  CtkWidget *image;
  CtkWidget *label;
  CtkWidget *custom_widget;

  CtkWindow *current_window;
  CtkWidget *keyboard_widget;

  CtkWidget *tooltip_widget;

  CdkWindow *last_window;

  guint timeout_id;
  guint browse_mode_timeout_id;

  CdkRectangle tip_area;

  guint browse_mode_enabled : 1;
  guint keyboard_mode_enabled : 1;
  guint tip_area_set : 1;
  guint custom_was_reset : 1;
};

struct _CtkTooltipClass
{
  GObjectClass parent_class;
};

#define CTK_TOOLTIP_VISIBLE(tooltip) ((tooltip)->current_window && ctk_widget_get_visible (CTK_WIDGET((tooltip)->current_window)))


static void       ctk_tooltip_class_init           (CtkTooltipClass *klass);
static void       ctk_tooltip_init                 (CtkTooltip      *tooltip);
static void       ctk_tooltip_dispose              (GObject         *object);

static void       ctk_tooltip_window_hide          (CtkWidget       *widget,
						    gpointer         user_data);
static void       ctk_tooltip_display_closed       (CdkDisplay      *display,
						    gboolean         was_error,
						    CtkTooltip      *tooltip);
static void       ctk_tooltip_set_last_window      (CtkTooltip      *tooltip,
						    CdkWindow       *window);

static void       ctk_tooltip_handle_event_internal (CdkEvent        *event);

static inline GQuark tooltip_quark (void)
{
  static GQuark quark;

  if G_UNLIKELY (quark == 0)
    quark = g_quark_from_static_string ("cdk-display-current-tooltip");
  return quark;
}

#define quark_current_tooltip tooltip_quark()

G_DEFINE_TYPE (CtkTooltip, ctk_tooltip, G_TYPE_OBJECT);

static void
ctk_tooltip_class_init (CtkTooltipClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ctk_tooltip_dispose;
}

static void
ctk_tooltip_init (CtkTooltip *tooltip)
{
  tooltip->timeout_id = 0;
  tooltip->browse_mode_timeout_id = 0;

  tooltip->browse_mode_enabled = FALSE;
  tooltip->keyboard_mode_enabled = FALSE;

  tooltip->current_window = NULL;
  tooltip->keyboard_widget = NULL;

  tooltip->tooltip_widget = NULL;

  tooltip->last_window = NULL;

  tooltip->window = ctk_tooltip_window_new ();
  g_signal_connect (tooltip->window, "hide",
                    G_CALLBACK (ctk_tooltip_window_hide),
                    tooltip);
}

static void
ctk_tooltip_dispose (GObject *object)
{
  CtkTooltip *tooltip = CTK_TOOLTIP (object);

  if (tooltip->timeout_id)
    {
      g_source_remove (tooltip->timeout_id);
      tooltip->timeout_id = 0;
    }

  if (tooltip->browse_mode_timeout_id)
    {
      g_source_remove (tooltip->browse_mode_timeout_id);
      tooltip->browse_mode_timeout_id = 0;
    }

  ctk_tooltip_set_custom (tooltip, NULL);
  ctk_tooltip_set_last_window (tooltip, NULL);

  if (tooltip->window)
    {
      CdkDisplay *display;

      display = ctk_widget_get_display (tooltip->window);
      g_signal_handlers_disconnect_by_func (display,
					    ctk_tooltip_display_closed,
					    tooltip);
      ctk_widget_destroy (tooltip->window);
      tooltip->window = NULL;
    }

  G_OBJECT_CLASS (ctk_tooltip_parent_class)->dispose (object);
}

/* public API */

/**
 * ctk_tooltip_set_markup:
 * @tooltip: a #CtkTooltip
 * @markup: (allow-none): a markup string (see [Pango markup format][PangoMarkupFormat]) or %NULL
 *
 * Sets the text of the tooltip to be @markup, which is marked up
 * with the [Pango text markup language][PangoMarkupFormat].
 * If @markup is %NULL, the label will be hidden.
 *
 * Since: 2.12
 */
void
ctk_tooltip_set_markup (CtkTooltip  *tooltip,
			const gchar *markup)
{
  g_return_if_fail (CTK_IS_TOOLTIP (tooltip));

  ctk_tooltip_window_set_label_markup (CTK_TOOLTIP_WINDOW (tooltip->window), markup);
}

/**
 * ctk_tooltip_set_text:
 * @tooltip: a #CtkTooltip
 * @text: (allow-none): a text string or %NULL
 *
 * Sets the text of the tooltip to be @text. If @text is %NULL, the label
 * will be hidden. See also ctk_tooltip_set_markup().
 *
 * Since: 2.12
 */
void
ctk_tooltip_set_text (CtkTooltip  *tooltip,
                      const gchar *text)
{
  g_return_if_fail (CTK_IS_TOOLTIP (tooltip));

  ctk_tooltip_window_set_label_text (CTK_TOOLTIP_WINDOW (tooltip->window), text);
}

/**
 * ctk_tooltip_set_icon:
 * @tooltip: a #CtkTooltip
 * @pixbuf: (allow-none): a #CdkPixbuf, or %NULL
 *
 * Sets the icon of the tooltip (which is in front of the text) to be
 * @pixbuf.  If @pixbuf is %NULL, the image will be hidden.
 *
 * Since: 2.12
 */
void
ctk_tooltip_set_icon (CtkTooltip *tooltip,
		      CdkPixbuf  *pixbuf)
{
  g_return_if_fail (CTK_IS_TOOLTIP (tooltip));
  g_return_if_fail (pixbuf == NULL || CDK_IS_PIXBUF (pixbuf));

  ctk_tooltip_window_set_image_icon (CTK_TOOLTIP_WINDOW (tooltip->window), pixbuf);
}

/**
 * ctk_tooltip_set_icon_from_stock:
 * @tooltip: a #CtkTooltip
 * @stock_id: (allow-none): a stock id, or %NULL
 * @size: (type int): a stock icon size (#CtkIconSize)
 *
 * Sets the icon of the tooltip (which is in front of the text) to be
 * the stock item indicated by @stock_id with the size indicated
 * by @size.  If @stock_id is %NULL, the image will be hidden.
 *
 * Since: 2.12
 *
 * Deprecated: 3.10: Use ctk_tooltip_set_icon_from_icon_name() instead.
 */
void
ctk_tooltip_set_icon_from_stock (CtkTooltip  *tooltip,
				 const gchar *stock_id,
				 CtkIconSize  size)
{
  g_return_if_fail (CTK_IS_TOOLTIP (tooltip));

  ctk_tooltip_window_set_image_icon_from_stock (CTK_TOOLTIP_WINDOW (tooltip->window),
                                                stock_id,
                                                size);
}

/**
 * ctk_tooltip_set_icon_from_icon_name:
 * @tooltip: a #CtkTooltip
 * @icon_name: (allow-none): an icon name, or %NULL
 * @size: (type int): a stock icon size (#CtkIconSize)
 *
 * Sets the icon of the tooltip (which is in front of the text) to be
 * the icon indicated by @icon_name with the size indicated
 * by @size.  If @icon_name is %NULL, the image will be hidden.
 *
 * Since: 2.14
 */
void
ctk_tooltip_set_icon_from_icon_name (CtkTooltip  *tooltip,
				     const gchar *icon_name,
				     CtkIconSize  size)
{
  g_return_if_fail (CTK_IS_TOOLTIP (tooltip));

  ctk_tooltip_window_set_image_icon_from_name (CTK_TOOLTIP_WINDOW (tooltip->window),
                                               icon_name,
                                               size);
}

/**
 * ctk_tooltip_set_icon_from_gicon:
 * @tooltip: a #CtkTooltip
 * @gicon: (allow-none): a #GIcon representing the icon, or %NULL
 * @size: (type int): a stock icon size (#CtkIconSize)
 *
 * Sets the icon of the tooltip (which is in front of the text)
 * to be the icon indicated by @gicon with the size indicated
 * by @size. If @gicon is %NULL, the image will be hidden.
 *
 * Since: 2.20
 */
void
ctk_tooltip_set_icon_from_gicon (CtkTooltip  *tooltip,
				 GIcon       *gicon,
				 CtkIconSize  size)
{
  g_return_if_fail (CTK_IS_TOOLTIP (tooltip));

  ctk_tooltip_window_set_image_icon_from_gicon (CTK_TOOLTIP_WINDOW (tooltip->window),
                                                gicon,
                                                size);
}

/**
 * ctk_tooltip_set_custom:
 * @tooltip: a #CtkTooltip
 * @custom_widget: (allow-none): a #CtkWidget, or %NULL to unset the old custom widget.
 *
 * Replaces the widget packed into the tooltip with
 * @custom_widget. @custom_widget does not get destroyed when the tooltip goes
 * away.
 * By default a box with a #CtkImage and #CtkLabel is embedded in 
 * the tooltip, which can be configured using ctk_tooltip_set_markup() 
 * and ctk_tooltip_set_icon().

 *
 * Since: 2.12
 */
void
ctk_tooltip_set_custom (CtkTooltip *tooltip,
			CtkWidget  *custom_widget)
{
  g_return_if_fail (CTK_IS_TOOLTIP (tooltip));
  g_return_if_fail (custom_widget == NULL || CTK_IS_WIDGET (custom_widget));

  /* The custom widget has been updated from the query-tooltip
   * callback, so we do not want to reset the custom widget later on.
   */
  tooltip->custom_was_reset = TRUE;

  ctk_tooltip_window_set_custom_widget (CTK_TOOLTIP_WINDOW (tooltip->window), custom_widget);
}

/**
 * ctk_tooltip_set_tip_area:
 * @tooltip: a #CtkTooltip
 * @rect: a #CdkRectangle
 *
 * Sets the area of the widget, where the contents of this tooltip apply,
 * to be @rect (in widget coordinates).  This is especially useful for
 * properly setting tooltips on #CtkTreeView rows and cells, #CtkIconViews,
 * etc.
 *
 * For setting tooltips on #CtkTreeView, please refer to the convenience
 * functions for this: ctk_tree_view_set_tooltip_row() and
 * ctk_tree_view_set_tooltip_cell().
 *
 * Since: 2.12
 */
void
ctk_tooltip_set_tip_area (CtkTooltip         *tooltip,
			  const CdkRectangle *rect)
{
  g_return_if_fail (CTK_IS_TOOLTIP (tooltip));

  if (!rect)
    tooltip->tip_area_set = FALSE;
  else
    {
      tooltip->tip_area_set = TRUE;
      tooltip->tip_area = *rect;
    }
}

/**
 * ctk_tooltip_trigger_tooltip_query:
 * @display: a #CdkDisplay
 *
 * Triggers a new tooltip query on @display, in order to update the current
 * visible tooltip, or to show/hide the current tooltip.  This function is
 * useful to call when, for example, the state of the widget changed by a
 * key press.
 *
 * Since: 2.12
 */
void
ctk_tooltip_trigger_tooltip_query (CdkDisplay *display)
{
  gint x, y;
  CdkWindow *window;
  CdkEvent event;
  CdkDevice *device;

  /* Trigger logic as if the mouse moved */
  device = cdk_seat_get_pointer (cdk_display_get_default_seat (display));
  window = cdk_device_get_window_at_position (device, &x, &y);
  if (!window)
    return;

  event.type = CDK_MOTION_NOTIFY;
  event.motion.window = window;
  event.motion.x = x;
  event.motion.y = y;
  event.motion.is_hint = FALSE;

  cdk_window_get_root_coords (window, x, y, &x, &y);
  event.motion.x_root = x;
  event.motion.y_root = y;

  ctk_tooltip_handle_event_internal (&event);
}

/* private functions */

static void
ctk_tooltip_reset (CtkTooltip *tooltip)
{
  ctk_tooltip_set_markup (tooltip, NULL);
  ctk_tooltip_set_icon (tooltip, NULL);
  ctk_tooltip_set_tip_area (tooltip, NULL);

  /* See if the custom widget is again set from the query-tooltip
   * callback.
   */
  tooltip->custom_was_reset = FALSE;
}

static void
ctk_tooltip_window_hide (CtkWidget *widget,
			 gpointer   user_data)
{
  CtkTooltip *tooltip = CTK_TOOLTIP (user_data);

  ctk_tooltip_set_custom (tooltip, NULL);
}

/* event handling, etc */

struct ChildLocation
{
  CtkWidget *child;
  CtkWidget *container;

  gint x;
  gint y;
};

static void
prepend_and_ref_widget (CtkWidget *widget,
                        gpointer   data)
{
  GSList **slist_p = data;

  *slist_p = g_slist_prepend (*slist_p, g_object_ref (widget));
}

static void
child_location_foreach (CtkWidget *child,
			gpointer   data)
{
  CtkAllocation child_allocation;
  gint x, y;
  struct ChildLocation *child_loc = data;

  /* Ignore invisible widgets */
  if (!ctk_widget_is_drawable (child))
    return;

  ctk_widget_get_allocation (child, &child_allocation);

  x = 0;
  y = 0;

  /* (child_loc->x, child_loc->y) are relative to
   * child_loc->container's allocation.
   */

  if (!child_loc->child &&
      ctk_widget_translate_coordinates (child_loc->container, child,
					child_loc->x, child_loc->y,
					&x, &y))
    {
      /* (x, y) relative to child's allocation. */
      if (x >= 0 && x < child_allocation.width
	  && y >= 0 && y < child_allocation.height)
        {
	  if (CTK_IS_CONTAINER (child))
	    {
	      struct ChildLocation tmp = { NULL, NULL, 0, 0 };
              GSList *children = NULL, *tmp_list;

	      /* Take (x, y) relative the child's allocation and
	       * recurse.
	       */
	      tmp.x = x;
	      tmp.y = y;
	      tmp.container = child;

	      ctk_container_forall (CTK_CONTAINER (child),
				    prepend_and_ref_widget, &children);

              for (tmp_list = children; tmp_list; tmp_list = tmp_list->next)
                {
                  child_location_foreach (tmp_list->data, &tmp);
                  g_object_unref (tmp_list->data);
                }

	      if (tmp.child)
		child_loc->child = tmp.child;
	      else
		child_loc->child = child;

              g_slist_free (children);
	    }
	  else
	    child_loc->child = child;
	}
    }
}

/* Translates coordinates from dest_widget->window relative (src_x, src_y),
 * to allocation relative (dest_x, dest_y) of dest_widget.
 */
static void
window_to_alloc (CtkWidget *dest_widget,
		 gint       src_x,
		 gint       src_y,
		 gint      *dest_x,
		 gint      *dest_y)
{
  CtkAllocation allocation;

  ctk_widget_get_allocation (dest_widget, &allocation);

  /* Translate from window relative to allocation relative */
  if (ctk_widget_get_has_window (dest_widget) &&
      ctk_widget_get_parent (dest_widget))
    {
      gint wx, wy;
      cdk_window_get_position (ctk_widget_get_window (dest_widget),
                               &wx, &wy);

      /* Offset coordinates if widget->window is smaller than
       * widget->allocation.
       */
      src_x += wx - allocation.x;
      src_y += wy - allocation.y;
    }
  else
    {
      src_x -= allocation.x;
      src_y -= allocation.y;
    }

  if (dest_x)
    *dest_x = src_x;
  if (dest_y)
    *dest_y = src_y;
}

/* Translates coordinates from window relative (x, y) to
 * allocation relative (x, y) of the returned widget.
 */
CtkWidget *
_ctk_widget_find_at_coords (CdkWindow *window,
                            gint       window_x,
                            gint       window_y,
                            gint      *widget_x,
                            gint      *widget_y)
{
  CtkWidget *event_widget;
  struct ChildLocation child_loc = { NULL, NULL, 0, 0 };

  g_return_val_if_fail (CDK_IS_WINDOW (window), NULL);

  cdk_window_get_user_data (window, (void **)&event_widget);

  if (!event_widget)
    return NULL;

  /* Coordinates are relative to event window */
  child_loc.x = window_x;
  child_loc.y = window_y;

  /* We go down the window hierarchy to the widget->window,
   * coordinates stay relative to the current window.
   * We end up with window == widget->window, coordinates relative to that.
   */
  while (window && window != ctk_widget_get_window (event_widget))
    {
      gdouble px, py;

      cdk_window_coords_to_parent (window,
                                   child_loc.x, child_loc.y,
                                   &px, &py);
      child_loc.x = px;
      child_loc.y = py;

      window = cdk_window_get_effective_parent (window);
    }

  /* Failing to find widget->window can happen for e.g. a detached handle box;
   * chaining ::query-tooltip up to its parent probably makes little sense,
   * and users better implement tooltips on handle_box->child.
   * so we simply ignore the event for tooltips here.
   */
  if (!window)
    return NULL;

  /* Convert the window relative coordinates to allocation
   * relative coordinates.
   */
  window_to_alloc (event_widget,
		   child_loc.x, child_loc.y,
		   &child_loc.x, &child_loc.y);

  if (CTK_IS_CONTAINER (event_widget))
    {
      CtkWidget *container = event_widget;

      child_loc.container = event_widget;
      child_loc.child = NULL;

      ctk_container_forall (CTK_CONTAINER (event_widget),
			    child_location_foreach, &child_loc);

      /* Here we have a widget, with coordinates relative to
       * child_loc.container's allocation.
       */

      if (child_loc.child)
	event_widget = child_loc.child;
      else if (child_loc.container)
	event_widget = child_loc.container;

      /* Translate to event_widget's allocation */
      ctk_widget_translate_coordinates (container, event_widget,
					child_loc.x, child_loc.y,
					&child_loc.x, &child_loc.y);
    }

  /* We return (x, y) relative to the allocation of event_widget. */
  if (widget_x)
    *widget_x = child_loc.x;
  if (widget_y)
    *widget_y = child_loc.y;

  return event_widget;
}

/* Ignores (x, y) on input, translates event coordinates to
 * allocation relative (x, y) of the returned widget.
 */
static CtkWidget *
find_topmost_widget_coords_from_event (CdkEvent *event,
				       gint     *x,
				       gint     *y)
{
  CtkAllocation allocation;
  gint tx, ty;
  gdouble dx, dy;
  CtkWidget *tmp;

  cdk_event_get_coords (event, &dx, &dy);

  /* Returns coordinates relative to tmp's allocation. */
  tmp = _ctk_widget_find_at_coords (event->any.window, dx, dy, &tx, &ty);

  if (!tmp)
    return NULL;

  /* Make sure the pointer can actually be on the widget returned. */
  ctk_widget_get_allocation (tmp, &allocation);
  allocation.x = 0;
  allocation.y = 0;
  if (CTK_IS_WINDOW (tmp))
    {
      CtkBorder border;
      _ctk_window_get_shadow_width (CTK_WINDOW (tmp), &border);
      allocation.x = border.left;
      allocation.y = border.top;
      allocation.width -= border.left + border.right;
      allocation.height -= border.top + border.bottom;
    }

  if (tx < allocation.x || tx >= allocation.width ||
      ty < allocation.y || ty >= allocation.height)
    return NULL;

  if (x)
    *x = tx;
  if (y)
    *y = ty;

  return tmp;
}

static gint
tooltip_browse_mode_expired (gpointer data)
{
  CtkTooltip *tooltip;
  CdkDisplay *display;

  tooltip = CTK_TOOLTIP (data);

  tooltip->browse_mode_enabled = FALSE;
  tooltip->browse_mode_timeout_id = 0;

  if (tooltip->timeout_id)
    {
      g_source_remove (tooltip->timeout_id);
      tooltip->timeout_id = 0;
    }

  /* destroy tooltip */
  display = ctk_widget_get_display (tooltip->window);
  g_object_set_qdata (G_OBJECT (display), quark_current_tooltip, NULL);

  return FALSE;
}

static void
ctk_tooltip_display_closed (CdkDisplay *display,
			    gboolean    was_error,
			    CtkTooltip *tooltip)
{
  if (tooltip->timeout_id)
    {
      g_source_remove (tooltip->timeout_id);
      tooltip->timeout_id = 0;
    }

  g_object_set_qdata (G_OBJECT (display), quark_current_tooltip, NULL);
}

static void
ctk_tooltip_set_last_window (CtkTooltip *tooltip,
			     CdkWindow  *window)
{
  CtkWidget *window_widget = NULL;

  if (tooltip->last_window == window)
    return;

  if (tooltip->last_window)
    g_object_remove_weak_pointer (G_OBJECT (tooltip->last_window),
				  (gpointer *) &tooltip->last_window);

  tooltip->last_window = window;

  if (tooltip->last_window)
    g_object_add_weak_pointer (G_OBJECT (tooltip->last_window),
			       (gpointer *) &tooltip->last_window);

  if (window)
    cdk_window_get_user_data (window, (gpointer *) &window_widget);

  if (window_widget)
    window_widget = ctk_widget_get_toplevel (window_widget);

  if (window_widget &&
      window_widget != tooltip->window &&
      ctk_widget_is_toplevel (window_widget) &&
      CTK_IS_WINDOW (window_widget))
    ctk_window_set_transient_for (CTK_WINDOW (tooltip->window),
                                  CTK_WINDOW (window_widget));
  else
    ctk_window_set_transient_for (CTK_WINDOW (tooltip->window), NULL);
}

static gboolean
ctk_tooltip_run_requery (CtkWidget  **widget,
			 CtkTooltip  *tooltip,
			 gint        *x,
			 gint        *y)
{
  gboolean has_tooltip = FALSE;
  gboolean return_value = FALSE;

  ctk_tooltip_reset (tooltip);

  do
    {
      has_tooltip = ctk_widget_get_has_tooltip (*widget);

      if (has_tooltip)
        return_value = ctk_widget_query_tooltip (*widget, *x, *y, tooltip->keyboard_mode_enabled, tooltip);

      if (!return_value)
        {
	  CtkWidget *parent = ctk_widget_get_parent (*widget);

	  if (parent)
	    ctk_widget_translate_coordinates (*widget, parent, *x, *y, x, y);

	  *widget = parent;
	}
      else
	break;
    }
  while (*widget);

  /* If the custom widget was not reset in the query-tooltip
   * callback, we clear it here.
   */
  if (!tooltip->custom_was_reset)
    ctk_tooltip_set_custom (tooltip, NULL);

  return return_value;
}

static void
ctk_tooltip_position (CtkTooltip *tooltip,
		      CdkDisplay *display,
		      CtkWidget  *new_tooltip_widget,
                      CdkDevice  *device)
{
  CdkScreen *screen;
  CtkSettings *settings;
  CdkRectangle anchor_rect;
  CdkWindow *window;
  CdkWindow *widget_window;
  CdkWindow *effective_toplevel;
  CtkWidget *toplevel;
  int rect_anchor_dx = 0;
  int cursor_size;
  int anchor_rect_padding;

  ctk_widget_realize (CTK_WIDGET (tooltip->current_window));
  window = _ctk_widget_get_window (CTK_WIDGET (tooltip->current_window));

  tooltip->tooltip_widget = new_tooltip_widget;

  toplevel = _ctk_widget_get_toplevel (new_tooltip_widget);
  ctk_widget_translate_coordinates (new_tooltip_widget, toplevel,
                                    0, 0,
                                    &anchor_rect.x, &anchor_rect.y);

  anchor_rect.width = ctk_widget_get_allocated_width (new_tooltip_widget);
  anchor_rect.height = ctk_widget_get_allocated_height (new_tooltip_widget);

  screen = cdk_window_get_screen (window);
  settings = ctk_settings_get_for_screen (screen);
  g_object_get (settings,
                "ctk-cursor-theme-size", &cursor_size,
                NULL);

  if (cursor_size == 0)
    cursor_size = cdk_display_get_default_cursor_size (display);

  if (device)
    anchor_rect_padding = MAX (4, cursor_size - 32);
  else
    anchor_rect_padding = 4;

  anchor_rect.x -= anchor_rect_padding;
  anchor_rect.y -= anchor_rect_padding;
  anchor_rect.width += anchor_rect_padding * 2;
  anchor_rect.height += anchor_rect_padding * 2;

  if (device)
    {
      const int max_x_distance = 32;
      /* Max 48x48 icon + default padding */
      const int max_anchor_rect_height = 48 + 8;
      int pointer_x, pointer_y;

      /*
       * For pointer position triggered tooltips, implement the following
       * semantics:
       *
       * If the anchor rectangle is too tall (meaning if we'd be constrained
       * and flip, it'd flip too far away), rely only on the pointer position
       * to position the tooltip. The approximate pointer cursorrectangle is
       * used as a anchor rectantgle.
       *
       * If the anchor rectangle isn't to tall, make sure the tooltip isn't too
       * far away from the pointer position.
       */
      widget_window = _ctk_widget_get_window (new_tooltip_widget);
      effective_toplevel = cdk_window_get_effective_toplevel (widget_window);
      cdk_window_get_device_position (effective_toplevel,
                                      device,
                                      &pointer_x, &pointer_y, NULL);

      if (anchor_rect.height > max_anchor_rect_height)
        {
          anchor_rect.x = pointer_x - 4;
          anchor_rect.y = pointer_y - 4;
          anchor_rect.width = cursor_size;
          anchor_rect.height = cursor_size;
        }
      else
        {
          int anchor_point_x;
          int x_distance;

          anchor_point_x = anchor_rect.x + anchor_rect.width / 2;
          x_distance = pointer_x - anchor_point_x;

          if (x_distance > max_x_distance)
            rect_anchor_dx = x_distance - max_x_distance;
          else if (x_distance < -max_x_distance)
            rect_anchor_dx = x_distance + max_x_distance;
        }
    }

  ctk_window_set_transient_for (CTK_WINDOW (tooltip->current_window),
                                CTK_WINDOW (toplevel));

  cdk_window_move_to_rect (window,
                           &anchor_rect,
                           CDK_GRAVITY_SOUTH,
                           CDK_GRAVITY_NORTH,
                           CDK_ANCHOR_FLIP_Y | CDK_ANCHOR_SLIDE_X,
                           rect_anchor_dx, 0);
  ctk_widget_show (CTK_WIDGET (tooltip->current_window));
}

static void
ctk_tooltip_show_tooltip (CdkDisplay *display)
{
  gint x, y;
  CdkScreen *screen;
  CdkDevice *device;
  CdkWindow *window;
  CtkWidget *tooltip_widget;
  CtkTooltip *tooltip;
  gboolean return_value = FALSE;

  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  if (tooltip->keyboard_mode_enabled)
    {
      x = y = -1;
      tooltip_widget = tooltip->keyboard_widget;
      device = NULL;
    }
  else
    {
      gint tx, ty;

      window = tooltip->last_window;

      if (!CDK_IS_WINDOW (window))
        return;

      device = cdk_seat_get_pointer (cdk_display_get_default_seat (display));

      cdk_window_get_device_position (window, device, &x, &y, NULL);

      cdk_window_get_root_coords (window, x, y, &tx, &ty);

      tooltip_widget = _ctk_widget_find_at_coords (window, x, y, &x, &y);
    }

  if (!tooltip_widget)
    return;

  return_value = ctk_tooltip_run_requery (&tooltip_widget, tooltip, &x, &y);
  if (!return_value)
    return;

  if (!tooltip->current_window)
    {
      if (ctk_widget_get_tooltip_window (tooltip_widget))
        tooltip->current_window = ctk_widget_get_tooltip_window (tooltip_widget);
      else
        tooltip->current_window = CTK_WINDOW (CTK_TOOLTIP (tooltip)->window);
    }

  screen = ctk_widget_get_screen (tooltip_widget);

  /* FIXME: should use tooltip->current_window iso tooltip->window */
  if (screen != ctk_widget_get_screen (tooltip->window))
    {
      g_signal_handlers_disconnect_by_func (display,
                                            ctk_tooltip_display_closed,
                                            tooltip);

      ctk_window_set_screen (CTK_WINDOW (tooltip->window), screen);

      g_signal_connect (display, "closed",
                        G_CALLBACK (ctk_tooltip_display_closed), tooltip);
    }

  ctk_tooltip_position (tooltip, display, tooltip_widget, device);

  /* Now a tooltip is visible again on the display, make sure browse
   * mode is enabled.
   */
  tooltip->browse_mode_enabled = TRUE;
  if (tooltip->browse_mode_timeout_id)
    {
      g_source_remove (tooltip->browse_mode_timeout_id);
      tooltip->browse_mode_timeout_id = 0;
    }
}

static void
ctk_tooltip_hide_tooltip (CtkTooltip *tooltip)
{
  if (!tooltip)
    return;

  if (tooltip->timeout_id)
    {
      g_source_remove (tooltip->timeout_id);
      tooltip->timeout_id = 0;
    }

  if (!CTK_TOOLTIP_VISIBLE (tooltip))
    return;

  tooltip->tooltip_widget = NULL;

  if (!tooltip->keyboard_mode_enabled)
    {
      guint timeout = BROWSE_DISABLE_TIMEOUT;

      /* The tooltip is gone, after (by default, should be configurable) 500ms
       * we want to turn off browse mode
       */
      if (!tooltip->browse_mode_timeout_id)
        {
	  tooltip->browse_mode_timeout_id =
	    cdk_threads_add_timeout_full (0, timeout,
					  tooltip_browse_mode_expired,
					  g_object_ref (tooltip),
					  g_object_unref);
	  g_source_set_name_by_id (tooltip->browse_mode_timeout_id, "[ctk+] tooltip_browse_mode_expired");
	}
    }
  else
    {
      if (tooltip->browse_mode_timeout_id)
        {
	  g_source_remove (tooltip->browse_mode_timeout_id);
	  tooltip->browse_mode_timeout_id = 0;
	}
    }

  if (tooltip->current_window)
    {
      ctk_widget_hide (CTK_WIDGET (tooltip->current_window));
      tooltip->current_window = NULL;
    }
}

static gint
tooltip_popup_timeout (gpointer data)
{
  CdkDisplay *display;
  CtkTooltip *tooltip;

  display = CDK_DISPLAY (data);
  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  /* This usually does not happen.  However, it does occur in language
   * bindings were reference counting of objects behaves differently.
   */
  if (!tooltip)
    return FALSE;

  ctk_tooltip_show_tooltip (display);

  tooltip->timeout_id = 0;

  return FALSE;
}

static void
ctk_tooltip_start_delay (CdkDisplay *display)
{
  guint timeout;
  CtkTooltip *tooltip;

  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  if (!tooltip || CTK_TOOLTIP_VISIBLE (tooltip))
    return;

  if (tooltip->timeout_id)
    g_source_remove (tooltip->timeout_id);

  if (tooltip->browse_mode_enabled)
    timeout = BROWSE_TIMEOUT;
  else
    timeout = HOVER_TIMEOUT;

  tooltip->timeout_id = cdk_threads_add_timeout_full (0, timeout,
						      tooltip_popup_timeout,
						      g_object_ref (display),
						      g_object_unref);
  g_source_set_name_by_id (tooltip->timeout_id, "[ctk+] tooltip_popup_timeout");
}

void
_ctk_tooltip_focus_in (CtkWidget *widget)
{
  gint x, y;
  gboolean return_value = FALSE;
  CdkDisplay *display;
  CtkTooltip *tooltip;
  CdkDevice *device;

  /* Get current tooltip for this display */
  display = ctk_widget_get_display (widget);
  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  /* Check if keyboard mode is enabled at this moment */
  if (!tooltip || !tooltip->keyboard_mode_enabled)
    return;

  device = ctk_get_current_event_device ();

  if (device && cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD)
    device = cdk_device_get_associated_device (device);

  /* This function should be called by either a focus in event,
   * or a key binding. In either case there should be a device.
   */
  if (!device)
    return;

  if (tooltip->keyboard_widget)
    g_object_unref (tooltip->keyboard_widget);

  tooltip->keyboard_widget = g_object_ref (widget);

  cdk_window_get_device_position (ctk_widget_get_window (widget),
                                  device, &x, &y, NULL);

  return_value = ctk_tooltip_run_requery (&widget, tooltip, &x, &y);
  if (!return_value)
    {
      ctk_tooltip_hide_tooltip (tooltip);
      return;
    }

  if (!tooltip->current_window)
    {
      if (ctk_widget_get_tooltip_window (widget))
	tooltip->current_window = ctk_widget_get_tooltip_window (widget);
      else
	tooltip->current_window = CTK_WINDOW (CTK_TOOLTIP (tooltip)->window);
    }

  ctk_tooltip_show_tooltip (display);
}

void
_ctk_tooltip_focus_out (CtkWidget *widget)
{
  CdkDisplay *display;
  CtkTooltip *tooltip;

  /* Get current tooltip for this display */
  display = ctk_widget_get_display (widget);
  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  if (!tooltip || !tooltip->keyboard_mode_enabled)
    return;

  if (tooltip->keyboard_widget)
    {
      g_object_unref (tooltip->keyboard_widget);
      tooltip->keyboard_widget = NULL;
    }

  ctk_tooltip_hide_tooltip (tooltip);
}

void
_ctk_tooltip_toggle_keyboard_mode (CtkWidget *widget)
{
  CdkDisplay *display;
  CtkTooltip *tooltip;

  display = ctk_widget_get_display (widget);
  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  if (!tooltip)
    {
      tooltip = g_object_new (CTK_TYPE_TOOLTIP, NULL);
      g_object_set_qdata_full (G_OBJECT (display),
			       quark_current_tooltip,
			       tooltip,
                               g_object_unref);
      g_signal_connect (display, "closed",
			G_CALLBACK (ctk_tooltip_display_closed),
			tooltip);
    }

  tooltip->keyboard_mode_enabled ^= 1;

  if (tooltip->keyboard_mode_enabled)
    {
      tooltip->keyboard_widget = g_object_ref (widget);
      _ctk_tooltip_focus_in (widget);
    }
  else
    {
      if (tooltip->keyboard_widget)
        {
	  g_object_unref (tooltip->keyboard_widget);
	  tooltip->keyboard_widget = NULL;
	}

      ctk_tooltip_hide_tooltip (tooltip);
    }
}

void
_ctk_tooltip_hide (CtkWidget *widget)
{
  CdkDisplay *display;
  CtkTooltip *tooltip;

  display = ctk_widget_get_display (widget);
  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  if (!tooltip || !CTK_TOOLTIP_VISIBLE (tooltip) || !tooltip->tooltip_widget)
    return;

  if (widget == tooltip->tooltip_widget)
    ctk_tooltip_hide_tooltip (tooltip);
}

void
_ctk_tooltip_hide_in_display (CdkDisplay *display)
{
  CtkTooltip *tooltip;

  if (!display)
    return;

  tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  if (!tooltip || !CTK_TOOLTIP_VISIBLE (tooltip))
    return;

  ctk_tooltip_hide_tooltip (tooltip);
}

static gboolean
tooltips_enabled (CdkEvent *event)
{
  CdkDevice *source_device;
  CdkInputSource source;

  source_device = cdk_event_get_source_device (event);

  if (!source_device)
    return FALSE;

  source = cdk_device_get_source (source_device);

  if (source != CDK_SOURCE_TOUCHSCREEN)
    return TRUE;

  return FALSE;
}

void
_ctk_tooltip_handle_event (CdkEvent *event)
{
  if (!tooltips_enabled (event))
    return;

  ctk_tooltip_handle_event_internal (event);
}

static void
ctk_tooltip_handle_event_internal (CdkEvent *event)
{
  gint x, y;
  CtkWidget *has_tooltip_widget;
  CdkDisplay *display;
  CtkTooltip *current_tooltip;

  /* Returns coordinates relative to has_tooltip_widget's allocation. */
  has_tooltip_widget = find_topmost_widget_coords_from_event (event, &x, &y);
  display = cdk_window_get_display (event->any.window);
  current_tooltip = g_object_get_qdata (G_OBJECT (display), quark_current_tooltip);

  if (current_tooltip)
    {
      ctk_tooltip_set_last_window (current_tooltip, event->any.window);
    }

  if (current_tooltip && current_tooltip->keyboard_mode_enabled)
    {
      gboolean return_value;

      has_tooltip_widget = current_tooltip->keyboard_widget;
      if (!has_tooltip_widget)
	return;

      return_value = ctk_tooltip_run_requery (&has_tooltip_widget,
					      current_tooltip,
					      &x, &y);

      if (!return_value)
	ctk_tooltip_hide_tooltip (current_tooltip);
      else
	ctk_tooltip_start_delay (display);

      return;
    }

  /* Always poll for a next motion event */
  cdk_event_request_motions (&event->motion);

  /* Hide the tooltip when there's no new tooltip widget */
  if (!has_tooltip_widget)
    {
      if (current_tooltip)
	ctk_tooltip_hide_tooltip (current_tooltip);

      return;
    }

  switch (event->type)
    {
      case CDK_BUTTON_PRESS:
      case CDK_2BUTTON_PRESS:
      case CDK_3BUTTON_PRESS:
      case CDK_KEY_PRESS:
      case CDK_DRAG_ENTER:
      case CDK_GRAB_BROKEN:
      case CDK_SCROLL:
	ctk_tooltip_hide_tooltip (current_tooltip);
	break;

      case CDK_MOTION_NOTIFY:
      case CDK_ENTER_NOTIFY:
      case CDK_LEAVE_NOTIFY:
	if (current_tooltip)
	  {
	    gboolean tip_area_set;
	    CdkRectangle tip_area;
	    gboolean hide_tooltip;

	    tip_area_set = current_tooltip->tip_area_set;
	    tip_area = current_tooltip->tip_area;

	    ctk_tooltip_run_requery (&has_tooltip_widget,
                                     current_tooltip,
                                     &x, &y);

	    /* Leave notify should override the query function */
	    hide_tooltip = (event->type == CDK_LEAVE_NOTIFY);

	    /* Is the pointer above another widget now? */
	    if (CTK_TOOLTIP_VISIBLE (current_tooltip))
	      hide_tooltip |= has_tooltip_widget != current_tooltip->tooltip_widget;

	    /* Did the pointer move out of the previous "context area"? */
	    if (tip_area_set)
	      hide_tooltip |= (x <= tip_area.x
			       || x >= tip_area.x + tip_area.width
			       || y <= tip_area.y
			       || y >= tip_area.y + tip_area.height);

	    if (hide_tooltip)
	      ctk_tooltip_hide_tooltip (current_tooltip);
	    else
	      ctk_tooltip_start_delay (display);
	  }
	else
	  {
	    /* Need a new tooltip for this display */
	    current_tooltip = g_object_new (CTK_TYPE_TOOLTIP, NULL);
	    g_object_set_qdata_full (G_OBJECT (display),
				     quark_current_tooltip,
				     current_tooltip,
                                     g_object_unref);
	    g_signal_connect (display, "closed",
			      G_CALLBACK (ctk_tooltip_display_closed),
			      current_tooltip);

	    ctk_tooltip_set_last_window (current_tooltip, event->any.window);

	    ctk_tooltip_start_delay (display);
	  }
	break;

      default:
	break;
    }
}
