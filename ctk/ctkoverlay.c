/*
 * ctkoverlay.c
 * This file is part of ctk
 *
 * Copyright (C) 2011 - Ignacio Casal Quinteiro, Mike Krüger
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

#include "config.h"

#include "ctkoverlay.h"
#include "ctkbuildable.h"
#include "ctkscrolledwindow.h"
#include "ctkmarshalers.h"

#include "ctkprivate.h"
#include "ctkintl.h"

/**
 * SECTION:ctkoverlay
 * @short_description: A container which overlays widgets on top of each other
 * @title: CtkOverlay
 *
 * CtkOverlay is a container which contains a single main child, on top
 * of which it can place “overlay” widgets. The position of each overlay
 * widget is determined by its #CtkWidget:halign and #CtkWidget:valign
 * properties. E.g. a widget with both alignments set to %CTK_ALIGN_START
 * will be placed at the top left corner of the CtkOverlay container,
 * whereas an overlay with halign set to %CTK_ALIGN_CENTER and valign set
 * to %CTK_ALIGN_END will be placed a the bottom edge of the CtkOverlay,
 * horizontally centered. The position can be adjusted by setting the margin
 * properties of the child to non-zero values.
 *
 * More complicated placement of overlays is possible by connecting
 * to the #CtkOverlay::get-child-position signal.
 *
 * An overlay’s minimum and natural sizes are those of its main child. The sizes
 * of overlay children are not considered when measuring these preferred sizes.
 *
 * # CtkOverlay as CtkBuildable
 *
 * The CtkOverlay implementation of the CtkBuildable interface
 * supports placing a child as an overlay by specifying “overlay” as
 * the “type” attribute of a `<child>` element.
 *
 * # CSS nodes
 *
 * CtkOverlay has a single CSS node with the name “overlay”. Overlay children
 * whose alignments cause them to be positioned at an edge get the style classes
 * “.left”, “.right”, “.top”, and/or “.bottom” according to their position.
 */

struct _CtkOverlayPrivate
{
  GSList *children;
};

typedef struct _CtkOverlayChild CtkOverlayChild;

struct _CtkOverlayChild
{
  CtkWidget *widget;
  GdkWindow *window;
  gboolean pass_through;
};

enum {
  GET_CHILD_POSITION,
  LAST_SIGNAL
};

enum
{
  CHILD_PROP_0,
  CHILD_PROP_PASS_THROUGH,
  CHILD_PROP_INDEX
};

static guint signals[LAST_SIGNAL] = { 0 };

static void ctk_overlay_buildable_init (CtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkOverlay, ctk_overlay, CTK_TYPE_BIN,
                         G_ADD_PRIVATE (CtkOverlay)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_overlay_buildable_init))

static void
ctk_overlay_compute_child_allocation (CtkOverlay      *overlay,
				      CtkOverlayChild *child,
				      CtkAllocation *window_allocation,
				      CtkAllocation *widget_allocation)
{
  gint left, right, top, bottom;
  CtkAllocation allocation, overlay_allocation;
  gboolean result;

  g_signal_emit (overlay, signals[GET_CHILD_POSITION],
                 0, child->widget, &allocation, &result);

  ctk_widget_get_allocation (CTK_WIDGET (overlay), &overlay_allocation);

  allocation.x += overlay_allocation.x;
  allocation.y += overlay_allocation.y;

  /* put the margins outside the window; also arrange things
   * so that the adjusted child allocation still ends up at 0, 0
   */
  left = ctk_widget_get_margin_start (child->widget);
  right = ctk_widget_get_margin_end (child->widget);
  top = ctk_widget_get_margin_top (child->widget);
  bottom = ctk_widget_get_margin_bottom (child->widget);

  if (widget_allocation)
    {
      widget_allocation->x = - left;
      widget_allocation->y = - top;
      widget_allocation->width = allocation.width;
      widget_allocation->height = allocation.height;
    }

  if (window_allocation)
    {
      window_allocation->x = allocation.x + left;
      window_allocation->y = allocation.y + top;
      window_allocation->width = allocation.width - (left + right);
      window_allocation->height = allocation.height - (top + bottom);
    }
}

static GdkWindow *
ctk_overlay_create_child_window (CtkOverlay *overlay,
                                 CtkOverlayChild *child)
{
  CtkWidget *widget = CTK_WIDGET (overlay);
  CtkAllocation allocation;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  ctk_overlay_compute_child_allocation (overlay, child, &allocation, NULL);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
  attributes.event_mask = ctk_widget_get_events (widget);

  window = cdk_window_new (ctk_widget_get_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_register_window (widget, window);

  cdk_window_set_pass_through (window, child->pass_through);

  ctk_widget_set_parent_window (child->widget, window);
  
  return window;
}

static CtkAlign
effective_align (CtkAlign         align,
                 CtkTextDirection direction)
{
  switch (align)
    {
    case CTK_ALIGN_START:
      return direction == CTK_TEXT_DIR_RTL ? CTK_ALIGN_END : CTK_ALIGN_START;
    case CTK_ALIGN_END:
      return direction == CTK_TEXT_DIR_RTL ? CTK_ALIGN_START : CTK_ALIGN_END;
    default:
      return align;
    }
}

static void
ctk_overlay_get_main_widget_allocation (CtkOverlay *overlay,
                                        CtkAllocation *main_alloc_out)
{
  CtkWidget *main_widget;
  CtkAllocation main_alloc;

  main_widget = ctk_bin_get_child (CTK_BIN (overlay));

  /* special-case scrolled windows */
  if (CTK_IS_SCROLLED_WINDOW (main_widget))
    {
      CtkWidget *grandchild;
      gint x, y;
      gboolean res;

      grandchild = ctk_bin_get_child (CTK_BIN (main_widget));
      res = ctk_widget_translate_coordinates (grandchild, main_widget, 0, 0, &x, &y);

      if (res)
        {
          main_alloc.x = x;
          main_alloc.y = y;
        }
      else
        {
          main_alloc.x = 0;
          main_alloc.y = 0;
        }

      main_alloc.width = ctk_widget_get_allocated_width (grandchild);
      main_alloc.height = ctk_widget_get_allocated_height (grandchild);
    }
  else
    {
      main_alloc.x = 0;
      main_alloc.y = 0;
      main_alloc.width = ctk_widget_get_allocated_width (CTK_WIDGET (overlay));
      main_alloc.height = ctk_widget_get_allocated_height (CTK_WIDGET (overlay));
    }

  if (main_alloc_out)
    *main_alloc_out = main_alloc;
}

static void
ctk_overlay_child_update_style_classes (CtkOverlay *overlay,
                                        CtkWidget *child,
                                        CtkAllocation *child_allocation)
{
  CtkAllocation overlay_allocation, main_allocation;
  CtkAlign valign, halign;
  gboolean is_left, is_right, is_top, is_bottom;
  gboolean has_left, has_right, has_top, has_bottom;
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (child);
  has_left = ctk_style_context_has_class (context, CTK_STYLE_CLASS_LEFT);
  has_right = ctk_style_context_has_class (context, CTK_STYLE_CLASS_RIGHT);
  has_top = ctk_style_context_has_class (context, CTK_STYLE_CLASS_TOP);
  has_bottom = ctk_style_context_has_class (context, CTK_STYLE_CLASS_BOTTOM);

  is_left = is_right = is_top = is_bottom = FALSE;

  ctk_overlay_get_main_widget_allocation (overlay, &main_allocation);
  ctk_widget_get_allocation (CTK_WIDGET (overlay), &overlay_allocation);

  main_allocation.x += overlay_allocation.x;
  main_allocation.y += overlay_allocation.y;

  halign = effective_align (ctk_widget_get_halign (child),
                            ctk_widget_get_direction (child));

  if (halign == CTK_ALIGN_START)
    is_left = (child_allocation->x == main_allocation.x);
  else if (halign == CTK_ALIGN_END)
    is_right = (child_allocation->x + child_allocation->width ==
                main_allocation.x + main_allocation.width);

  valign = ctk_widget_get_valign (child);

  if (valign == CTK_ALIGN_START)
    is_top = (child_allocation->y == main_allocation.y);
  else if (valign == CTK_ALIGN_END)
    is_bottom = (child_allocation->y + child_allocation->height ==
                 main_allocation.y + main_allocation.height);

  if (has_left && !is_left)
    ctk_style_context_remove_class (context, CTK_STYLE_CLASS_LEFT);
  else if (!has_left && is_left)
    ctk_style_context_add_class (context, CTK_STYLE_CLASS_LEFT);

  if (has_right && !is_right)
    ctk_style_context_remove_class (context, CTK_STYLE_CLASS_RIGHT);
  else if (!has_right && is_right)
    ctk_style_context_add_class (context, CTK_STYLE_CLASS_RIGHT);

  if (has_top && !is_top)
    ctk_style_context_remove_class (context, CTK_STYLE_CLASS_TOP);
  else if (!has_top && is_top)
    ctk_style_context_add_class (context, CTK_STYLE_CLASS_TOP);

  if (has_bottom && !is_bottom)
    ctk_style_context_remove_class (context, CTK_STYLE_CLASS_BOTTOM);
  else if (!has_bottom && is_bottom)
    ctk_style_context_add_class (context, CTK_STYLE_CLASS_BOTTOM);
}

static void
ctk_overlay_child_allocate (CtkOverlay      *overlay,
                            CtkOverlayChild *child)
{
  CtkAllocation window_allocation, child_allocation;

  if (ctk_widget_get_mapped (CTK_WIDGET (overlay)))
    {
      /* Note: This calls show every size allocation, which makes
       * us keep the z-order of the chilren, as cdk_window_show()
       * does an implicit raise. */
      if (ctk_widget_get_visible (child->widget))
        cdk_window_show (child->window);
      else if (cdk_window_is_visible (child->window))
        cdk_window_hide (child->window);
    }

  if (!ctk_widget_get_visible (child->widget))
    return;

  ctk_overlay_compute_child_allocation (overlay, child, &window_allocation, &child_allocation);

  if (child->window)
    cdk_window_move_resize (child->window,
                            window_allocation.x, window_allocation.y,
                            window_allocation.width, window_allocation.height);

  ctk_overlay_child_update_style_classes (overlay, child->widget, &window_allocation);
  ctk_widget_size_allocate (child->widget, &child_allocation);
}

static void
ctk_overlay_size_allocate (CtkWidget     *widget,
                           CtkAllocation *allocation)
{
  CtkOverlay *overlay = CTK_OVERLAY (widget);
  CtkOverlayPrivate *priv = overlay->priv;
  GSList *children;
  CtkWidget *main_widget;

  CTK_WIDGET_CLASS (ctk_overlay_parent_class)->size_allocate (widget, allocation);

  main_widget = ctk_bin_get_child (CTK_BIN (overlay));
  if (main_widget && ctk_widget_get_visible (main_widget))
    ctk_widget_size_allocate (main_widget, allocation);

  for (children = priv->children; children; children = children->next)
    ctk_overlay_child_allocate (overlay, children->data);
}

static gboolean
ctk_overlay_get_child_position (CtkOverlay    *overlay,
                                CtkWidget     *widget,
                                CtkAllocation *alloc)
{
  CtkAllocation main_alloc;
  CtkRequisition min, req;
  CtkAlign halign;
  CtkTextDirection direction;

  ctk_overlay_get_main_widget_allocation (overlay, &main_alloc);
  ctk_widget_get_preferred_size (widget, &min, &req);

  alloc->x = main_alloc.x;
  alloc->width = MAX (min.width, MIN (main_alloc.width, req.width));

  direction = ctk_widget_get_direction (widget);

  halign = ctk_widget_get_halign (widget);
  switch (effective_align (halign, direction))
    {
    case CTK_ALIGN_START:
      /* nothing to do */
      break;
    case CTK_ALIGN_FILL:
      alloc->width = MAX (alloc->width, main_alloc.width);
      break;
    case CTK_ALIGN_CENTER:
      alloc->x += main_alloc.width / 2 - alloc->width / 2;
      break;
    case CTK_ALIGN_END:
      alloc->x += main_alloc.width - alloc->width;
      break;
    case CTK_ALIGN_BASELINE:
    default:
      g_assert_not_reached ();
      break;
    }

  alloc->y = main_alloc.y;
  alloc->height = MAX  (min.height, MIN (main_alloc.height, req.height));

  switch (ctk_widget_get_valign (widget))
    {
    case CTK_ALIGN_START:
      /* nothing to do */
      break;
    case CTK_ALIGN_FILL:
      alloc->height = MAX (alloc->height, main_alloc.height);
      break;
    case CTK_ALIGN_CENTER:
      alloc->y += main_alloc.height / 2 - alloc->height / 2;
      break;
    case CTK_ALIGN_END:
      alloc->y += main_alloc.height - alloc->height;
      break;
    case CTK_ALIGN_BASELINE:
    default:
      g_assert_not_reached ();
      break;
    }

  return TRUE;
}

static void
ctk_overlay_realize (CtkWidget *widget)
{
  CtkOverlay *overlay = CTK_OVERLAY (widget);
  CtkOverlayPrivate *priv = overlay->priv;
  CtkOverlayChild *child;
  GSList *children;

  CTK_WIDGET_CLASS (ctk_overlay_parent_class)->realize (widget);

  for (children = priv->children; children; children = children->next)
    {
      child = children->data;

      if (child->window == NULL)
	child->window = ctk_overlay_create_child_window (overlay, child);
    }
}

static void
ctk_overlay_unrealize (CtkWidget *widget)
{
  CtkOverlay *overlay = CTK_OVERLAY (widget);
  CtkOverlayPrivate *priv = overlay->priv;
  CtkOverlayChild *child;
  GSList *children;

  for (children = priv->children; children; children = children->next)
    {
      child = children->data;

      ctk_widget_set_parent_window (child->widget, NULL);
      ctk_widget_unregister_window (widget, child->window);
      cdk_window_destroy (child->window);
      child->window = NULL;
    }

  CTK_WIDGET_CLASS (ctk_overlay_parent_class)->unrealize (widget);
}

static void
ctk_overlay_map (CtkWidget *widget)
{
  CtkOverlay *overlay = CTK_OVERLAY (widget);
  CtkOverlayPrivate *priv = overlay->priv;
  CtkOverlayChild *child;
  GSList *children;

  CTK_WIDGET_CLASS (ctk_overlay_parent_class)->map (widget);

  for (children = priv->children; children; children = children->next)
    {
      child = children->data;

      if (child->window != NULL &&
          ctk_widget_get_visible (child->widget) &&
          ctk_widget_get_child_visible (child->widget))
        cdk_window_show (child->window);
    }
}

static void
ctk_overlay_unmap (CtkWidget *widget)
{
  CtkOverlay *overlay = CTK_OVERLAY (widget);
  CtkOverlayPrivate *priv = overlay->priv;
  CtkOverlayChild *child;
  GSList *children;

  for (children = priv->children; children; children = children->next)
    {
      child = children->data;

      if (child->window != NULL &&
          cdk_window_is_visible (child->window))
        cdk_window_hide (child->window);
    }

  CTK_WIDGET_CLASS (ctk_overlay_parent_class)->unmap (widget);
}

static void
ctk_overlay_remove (CtkContainer *container,
                    CtkWidget    *widget)
{
  CtkOverlayPrivate *priv = CTK_OVERLAY (container)->priv;
  CtkOverlayChild *child;
  GSList *children, *next;
  gboolean removed;

  removed = FALSE;
  for (children = priv->children; children; children = next)
    {
      child = children->data;
      next = children->next;

      if (child->widget == widget)
        {
          if (child->window != NULL)
            {
              ctk_widget_unregister_window (CTK_WIDGET (container), child->window);
              cdk_window_destroy (child->window);
            }

          ctk_widget_unparent (widget);

          priv->children = g_slist_delete_link (priv->children, children);
          g_slice_free (CtkOverlayChild, child);

          removed = TRUE;
        }
      else if (removed)
        ctk_widget_child_notify (child->widget, "index");
    }

  if (!removed)
    CTK_CONTAINER_CLASS (ctk_overlay_parent_class)->remove (container, widget);
}

/**
 * ctk_overlay_reorder_overlay:
 * @overlay: a #CtkOverlay
 * @child: the overlaid #CtkWidget to move
 * @index_: the new index for @child in the list of overlay children
 *   of @overlay, starting from 0. If negative, indicates the end of
 *   the list
 *
 * Moves @child to a new @index in the list of @overlay children.
 * The list contains overlays in the order that these were
 * added to @overlay by default. See also #CtkOverlay:index.
 *
 * A widget’s index in the @overlay children list determines which order
 * the children are drawn if they overlap. The first child is drawn at
 * the bottom. It also affects the default focus chain order.
 *
 * Since: 3.18
 */
void
ctk_overlay_reorder_overlay (CtkOverlay *overlay,
                             CtkWidget  *child,
                             int         index_)
{
  CtkOverlayPrivate *priv;
  GSList *old_link;
  GSList *new_link;
  GSList *l;
  CtkOverlayChild *child_info = NULL;
  gint old_index, i;
  gint index;

  g_return_if_fail (CTK_IS_OVERLAY (overlay));
  g_return_if_fail (CTK_IS_WIDGET (child));

  priv = CTK_OVERLAY (overlay)->priv;

  old_link = priv->children;
  old_index = 0;
  while (old_link)
    {
      child_info = old_link->data;
      if (child_info->widget == child)
	break;

      old_link = old_link->next;
      old_index++;
    }

  g_return_if_fail (old_link != NULL);

  if (index_ < 0)
    {
      new_link = NULL;
      index = g_slist_length (priv->children) - 1;
    }
  else
    {
      new_link = g_slist_nth (priv->children, index_);
      index = MIN (index_, g_slist_length (priv->children) - 1);
    }

  if (index == old_index)
    return;

  priv->children = g_slist_delete_link (priv->children, old_link);
  priv->children = g_slist_insert_before (priv->children, new_link, child_info);

  for (i = 0, l = priv->children; l != NULL; l = l->next, i++)
    {
      CtkOverlayChild *info = l->data;
      if ((i < index && i < old_index) ||
          (i > index && i > old_index))
        continue;
      ctk_widget_child_notify (info->widget, "index");
    }

  if (ctk_widget_get_visible (child) &&
      ctk_widget_get_visible (CTK_WIDGET (overlay)))
    ctk_widget_queue_resize (CTK_WIDGET (overlay));
}


static void
ctk_overlay_forall (CtkContainer *overlay,
                    gboolean      include_internals,
                    CtkCallback   callback,
                    gpointer      callback_data)
{
  CtkOverlayPrivate *priv = CTK_OVERLAY (overlay)->priv;
  CtkOverlayChild *child;
  GSList *children;
  CtkWidget *main_widget;

  main_widget = ctk_bin_get_child (CTK_BIN (overlay));
  if (main_widget)
    (* callback) (main_widget, callback_data);

  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child->widget, callback_data);
    }
}

static CtkOverlayChild *
ctk_overlay_get_overlay_child (CtkOverlay *overlay,
			       CtkWidget *child)
{
  CtkOverlayPrivate *priv = CTK_OVERLAY (overlay)->priv;
  CtkOverlayChild *child_info;
  GSList *children;

  for (children = priv->children; children; children = children->next)
    {
      child_info = children->data;

      if (child_info->widget == child)
	return child_info;
    }

  return NULL;
}

static void
ctk_overlay_set_child_property (CtkContainer *container,
				CtkWidget    *child,
				guint         property_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  CtkOverlay *overlay = CTK_OVERLAY (container);
  CtkOverlayChild *child_info;
  CtkWidget *main_widget;

  main_widget = ctk_bin_get_child (CTK_BIN (overlay));
  if (child == main_widget)
    child_info = NULL;
  else
    {
      child_info = ctk_overlay_get_overlay_child (overlay, child);
      if (child_info == NULL)
	{
	  CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
	  return;
	}
    }

  switch (property_id)
    {
    case CHILD_PROP_PASS_THROUGH:
      /* Ignore value on main child */
      if (child_info)
	{
	  if (g_value_get_boolean (value) != child_info->pass_through)
	    {
	      child_info->pass_through = g_value_get_boolean (value);
	      if (child_info->window)
		cdk_window_set_pass_through (child_info->window, child_info->pass_through);
	      ctk_container_child_notify (container, child, "pass-through");
	    }
	}
      break;
    case CHILD_PROP_INDEX:
      if (child_info != NULL)
	ctk_overlay_reorder_overlay (CTK_OVERLAY (container),
				     child,
				     g_value_get_int (value));
      break;

    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_overlay_get_child_property (CtkContainer *container,
				CtkWidget    *child,
				guint         property_id,
				GValue       *value,
				GParamSpec   *pspec)
{
  CtkOverlay *overlay = CTK_OVERLAY (container);
  CtkOverlayPrivate *priv = CTK_OVERLAY (overlay)->priv;
  CtkOverlayChild *child_info;
  CtkWidget *main_widget;

  main_widget = ctk_bin_get_child (CTK_BIN (overlay));
  if (child == main_widget)
    child_info = NULL;
  else
    {
      child_info = ctk_overlay_get_overlay_child (overlay, child);
      if (child_info == NULL)
	{
	  CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
	  return;
	}
    }

  switch (property_id)
    {
    case CHILD_PROP_PASS_THROUGH:
      if (child_info)
	g_value_set_boolean (value, child_info->pass_through);
      else
	g_value_set_boolean (value, FALSE);
      break;
    case CHILD_PROP_INDEX:
      g_value_set_int (value, g_slist_index (priv->children, child_info));
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}


static void
ctk_overlay_class_init (CtkOverlayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);

  widget_class->size_allocate = ctk_overlay_size_allocate;
  widget_class->realize = ctk_overlay_realize;
  widget_class->unrealize = ctk_overlay_unrealize;
  widget_class->map = ctk_overlay_map;
  widget_class->unmap = ctk_overlay_unmap;

  container_class->remove = ctk_overlay_remove;
  container_class->forall = ctk_overlay_forall;
  container_class->set_child_property = ctk_overlay_set_child_property;
  container_class->get_child_property = ctk_overlay_get_child_property;

  klass->get_child_position = ctk_overlay_get_child_position;

  /**
   * CtkOverlay:pass-through:
   *
   * Whether to pass input through the overlay child to the main child.
   * (Of course, this has no effect when set on the main child itself.)
   *
   * Since: 3.18
   */
  ctk_container_class_install_child_property (container_class, CHILD_PROP_PASS_THROUGH,
      g_param_spec_boolean ("pass-through", P_("Pass Through"), P_("Pass through input, does not affect main child"),
                            FALSE,
                            CTK_PARAM_READWRITE));

  /**
   * CtkOverlay:index:
   *
   * The index of the overlay child in the parent (or -1 for the main child).
   * See ctk_overlay_reorder_overlay().
   *
   * Since: 3.18
   */
  ctk_container_class_install_child_property (container_class, CHILD_PROP_INDEX,
					      g_param_spec_int ("index",
								P_("Index"),
								P_("The index of the overlay in the parent, -1 for the main child"),
								-1, G_MAXINT, 0,
								CTK_PARAM_READWRITE));

  /**
   * CtkOverlay::get-child-position:
   * @overlay: the #CtkOverlay
   * @widget: the child widget to position
   * @allocation: (type Gdk.Rectangle) (out caller-allocates): return
   *   location for the allocation
   *
   * The ::get-child-position signal is emitted to determine
   * the position and size of any overlay child widgets. A
   * handler for this signal should fill @allocation with
   * the desired position and size for @widget, relative to
   * the 'main' child of @overlay.
   *
   * The default handler for this signal uses the @widget's
   * halign and valign properties to determine the position
   * and gives the widget its natural size (except that an
   * alignment of %CTK_ALIGN_FILL will cause the overlay to
   * be full-width/height). If the main child is a
   * #CtkScrolledWindow, the overlays are placed relative
   * to its contents.
   *
   * Returns: %TRUE if the @allocation has been filled
   */
  signals[GET_CHILD_POSITION] =
    g_signal_new (I_("get-child-position"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkOverlayClass, get_child_position),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__OBJECT_BOXED,
                  G_TYPE_BOOLEAN, 2,
                  CTK_TYPE_WIDGET,
                  GDK_TYPE_RECTANGLE | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (signals[GET_CHILD_POSITION],
                              G_TYPE_FROM_CLASS (object_class),
                              _ctk_marshal_BOOLEAN__OBJECT_BOXEDv);

  ctk_widget_class_set_css_name (widget_class, "overlay");
}

static void
ctk_overlay_init (CtkOverlay *overlay)
{
  overlay->priv = ctk_overlay_get_instance_private (overlay);

  ctk_widget_set_has_window (CTK_WIDGET (overlay), FALSE);
}

static void
ctk_overlay_buildable_add_child (CtkBuildable *buildable,
                                 CtkBuilder   *builder,
                                 GObject      *child,
                                 const gchar  *type)
{
  if (type && strcmp (type, "overlay") == 0)
    ctk_overlay_add_overlay (CTK_OVERLAY (buildable), CTK_WIDGET (child));
  else if (!type)
    ctk_container_add (CTK_CONTAINER (buildable), CTK_WIDGET (child));
  else
    CTK_BUILDER_WARN_INVALID_CHILD_TYPE (buildable, type);
}

static void
ctk_overlay_buildable_init (CtkBuildableIface *iface)
{
  iface->add_child = ctk_overlay_buildable_add_child;
}

/**
 * ctk_overlay_new:
 *
 * Creates a new #CtkOverlay.
 *
 * Returns: a new #CtkOverlay object.
 *
 * Since: 3.2
 */
CtkWidget *
ctk_overlay_new (void)
{
  return g_object_new (CTK_TYPE_OVERLAY, NULL);
}

/**
 * ctk_overlay_add_overlay:
 * @overlay: a #CtkOverlay
 * @widget: a #CtkWidget to be added to the container
 *
 * Adds @widget to @overlay.
 *
 * The widget will be stacked on top of the main widget
 * added with ctk_container_add().
 *
 * The position at which @widget is placed is determined
 * from its #CtkWidget:halign and #CtkWidget:valign properties.
 *
 * Since: 3.2
 */
void
ctk_overlay_add_overlay (CtkOverlay *overlay,
                         CtkWidget  *widget)
{
  CtkOverlayPrivate *priv;
  CtkOverlayChild *child;

  g_return_if_fail (CTK_IS_OVERLAY (overlay));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = overlay->priv;
  child = g_slice_new0 (CtkOverlayChild);
  child->widget = widget;

  priv->children = g_slist_append (priv->children, child);

  if (ctk_widget_get_realized (CTK_WIDGET (overlay)))
    {
      child->window = ctk_overlay_create_child_window (overlay, child);
      ctk_widget_set_parent (widget, CTK_WIDGET (overlay));
    }
  else
    ctk_widget_set_parent (widget, CTK_WIDGET (overlay));

  ctk_widget_child_notify (widget, "index");
}

/**
 * ctk_overlay_set_overlay_pass_through:
 * @overlay: a #CtkOverlay
 * @widget: an overlay child of #CtkOverlay
 * @pass_through: whether the child should pass the input through
 *
 * Convenience function to set the value of the #CtkOverlay:pass-through
 * child property for @widget.
 *
 * Since: 3.18
 */
void
ctk_overlay_set_overlay_pass_through (CtkOverlay *overlay,
				      CtkWidget  *widget,
				      gboolean    pass_through)
{
  g_return_if_fail (CTK_IS_OVERLAY (overlay));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_container_child_set (CTK_CONTAINER (overlay), widget,
			   "pass-through", pass_through,
			   NULL);
}

/**
 * ctk_overlay_get_overlay_pass_through:
 * @overlay: a #CtkOverlay
 * @widget: an overlay child of #CtkOverlay
 *
 * Convenience function to get the value of the #CtkOverlay:pass-through
 * child property for @widget.
 *
 * Returns: whether the widget is a pass through child.
 *
 * Since: 3.18
 */
gboolean
ctk_overlay_get_overlay_pass_through (CtkOverlay *overlay,
				      CtkWidget  *widget)
{
  gboolean pass_through;

  g_return_val_if_fail (CTK_IS_OVERLAY (overlay), FALSE);
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  ctk_container_child_get (CTK_CONTAINER (overlay), widget,
			   "pass-through", &pass_through,
			   NULL);

  return pass_through;
}
