/* GTK - The GIMP Toolkit
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

/**
 * SECTION:ctkbin
 * @Short_description: A container with just one child
 * @Title: CtkBin
 *
 * The #CtkBin widget is a container with just one child.
 * It is not very useful itself, but it is useful for deriving subclasses,
 * since it provides common code needed for handling a single child widget.
 *
 * Many GTK+ widgets are subclasses of #CtkBin, including #CtkWindow,
 * #CtkButton, #CtkFrame, #CtkHandleBox or #CtkScrolledWindow.
 */

#include "config.h"
#include "ctkbin.h"
#include "ctksizerequest.h"
#include "ctkintl.h"


struct _CtkBinPrivate
{
  CtkWidget *child;
};

static void ctk_bin_add         (CtkContainer   *container,
			         CtkWidget      *widget);
static void ctk_bin_remove      (CtkContainer   *container,
			         CtkWidget      *widget);
static void ctk_bin_forall      (CtkContainer   *container,
				 gboolean	include_internals,
				 CtkCallback     callback,
				 gpointer        callback_data);
static GType ctk_bin_child_type (CtkContainer   *container);

static void               ctk_bin_get_preferred_width             (CtkWidget           *widget,
                                                                   gint                *minimum_width,
                                                                   gint                *natural_width);
static void               ctk_bin_get_preferred_height            (CtkWidget           *widget,
                                                                   gint                *minimum_height,
                                                                   gint                *natural_height);
static void               ctk_bin_get_preferred_width_for_height  (CtkWidget           *widget,
                                                                   gint                 height,
                                                                   gint                *minimum_width,
                                                                   gint                *natural_width);
static void               ctk_bin_get_preferred_height_for_width  (CtkWidget           *widget,
                                                                   gint                 width,
                                                                   gint                *minimum_height,
                                                                   gint                *natural_height);
static void               ctk_bin_size_allocate                   (CtkWidget           *widget,
                                                                   CtkAllocation       *allocation);

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (CtkBin, ctk_bin, CTK_TYPE_CONTAINER)

static void
ctk_bin_class_init (CtkBinClass *class)
{
  CtkWidgetClass *widget_class = (CtkWidgetClass*) class;
  CtkContainerClass *container_class = (CtkContainerClass*) class;

  widget_class->get_preferred_width = ctk_bin_get_preferred_width;
  widget_class->get_preferred_height = ctk_bin_get_preferred_height;
  widget_class->get_preferred_width_for_height = ctk_bin_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ctk_bin_get_preferred_height_for_width;
  widget_class->size_allocate = ctk_bin_size_allocate;

  container_class->add = ctk_bin_add;
  container_class->remove = ctk_bin_remove;
  container_class->forall = ctk_bin_forall;
  container_class->child_type = ctk_bin_child_type;
}

static void
ctk_bin_init (CtkBin *bin)
{
  bin->priv = ctk_bin_get_instance_private (bin);

  ctk_widget_set_has_window (CTK_WIDGET (bin), FALSE);
}


static GType
ctk_bin_child_type (CtkContainer *container)
{
  CtkBinPrivate *priv = CTK_BIN (container)->priv;

  if (!priv->child)
    return CTK_TYPE_WIDGET;
  else
    return G_TYPE_NONE;
}

static void
ctk_bin_add (CtkContainer *container,
	     CtkWidget    *child)
{
  CtkBin *bin = CTK_BIN (container);
  CtkBinPrivate *priv = bin->priv;

  if (priv->child != NULL)
    {
      g_warning ("Attempting to add a widget with type %s to a %s, "
                 "but as a CtkBin subclass a %s can only contain one widget at a time; "
                 "it already contains a widget of type %s",
                 g_type_name (G_OBJECT_TYPE (child)),
                 g_type_name (G_OBJECT_TYPE (bin)),
                 g_type_name (G_OBJECT_TYPE (bin)),
                 g_type_name (G_OBJECT_TYPE (priv->child)));
      return;
    }

  ctk_widget_set_parent (child, CTK_WIDGET (bin));
  priv->child = child;
}

static void
ctk_bin_remove (CtkContainer *container,
		CtkWidget    *child)
{
  CtkBin *bin = CTK_BIN (container);
  CtkBinPrivate *priv = bin->priv;
  gboolean widget_was_visible;

  g_return_if_fail (priv->child == child);

  widget_was_visible = ctk_widget_get_visible (child);
  
  ctk_widget_unparent (child);
  priv->child = NULL;
  
  /* queue resize regardless of ctk_widget_get_visible (container),
   * since that's what is needed by toplevels, which derive from CtkBin.
   */
  if (widget_was_visible)
    ctk_widget_queue_resize (CTK_WIDGET (container));
}

static void
ctk_bin_forall (CtkContainer *container,
		gboolean      include_internals,
		CtkCallback   callback,
		gpointer      callback_data)
{
  CtkBin *bin = CTK_BIN (container);
  CtkBinPrivate *priv = bin->priv;

  if (priv->child)
    (* callback) (priv->child, callback_data);
}

static int
ctk_bin_get_effective_border_width (CtkBin *bin)
{
  if (CTK_CONTAINER_CLASS (CTK_BIN_GET_CLASS (bin))->_handle_border_width)
    return 0;

  return ctk_container_get_border_width (CTK_CONTAINER (bin));
}

static void
ctk_bin_get_preferred_width (CtkWidget *widget,
                             gint      *minimum_width,
                             gint      *natural_width)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkBinPrivate *priv = bin->priv;
  gint border_width;

  *minimum_width = 0;
  *natural_width = 0;

  if (priv->child && ctk_widget_get_visible (priv->child))
    {
      gint child_min, child_nat;
      ctk_widget_get_preferred_width (priv->child,
                                      &child_min, &child_nat);
      *minimum_width = child_min;
      *natural_width = child_nat;
    }

  border_width = ctk_bin_get_effective_border_width (bin);
  *minimum_width += 2 * border_width;
  *natural_width += 2 * border_width;
}

static void
ctk_bin_get_preferred_height (CtkWidget *widget,
                              gint      *minimum_height,
                              gint      *natural_height)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkBinPrivate *priv = bin->priv;
  gint border_width;

  *minimum_height = 0;
  *natural_height = 0;

  if (priv->child && ctk_widget_get_visible (priv->child))
    {
      gint child_min, child_nat;
      ctk_widget_get_preferred_height (priv->child,
                                       &child_min, &child_nat);
      *minimum_height = child_min;
      *natural_height = child_nat;
    }

  border_width = ctk_bin_get_effective_border_width (bin);
  *minimum_height += 2 * border_width;
  *natural_height += 2 * border_width;
}

static void 
ctk_bin_get_preferred_width_for_height (CtkWidget *widget,
                                        gint       height,
                                        gint      *minimum_width,
                                        gint      *natural_width)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkBinPrivate *priv = bin->priv;
  gint border_width;

  *minimum_width = 0;
  *natural_width = 0;

  border_width = ctk_bin_get_effective_border_width (bin);

  if (priv->child && ctk_widget_get_visible (priv->child))
    {
      gint child_min, child_nat;
      ctk_widget_get_preferred_width_for_height (priv->child, height - 2 * border_width,
                                                 &child_min, &child_nat);

      *minimum_width = child_min;
      *natural_width = child_nat;
    }

  *minimum_width += 2 * border_width;
  *natural_width += 2 * border_width;
}

static void
ctk_bin_get_preferred_height_for_width  (CtkWidget *widget,
                                         gint       width,
                                         gint      *minimum_height,
                                         gint      *natural_height)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkBinPrivate *priv = bin->priv;
  gint border_width;

  *minimum_height = 0;
  *natural_height = 0;

  border_width = ctk_bin_get_effective_border_width (bin);

  if (priv->child && ctk_widget_get_visible (priv->child))
    {
      gint child_min, child_nat;
      ctk_widget_get_preferred_height_for_width (priv->child, width - 2 * border_width,
                                                 &child_min, &child_nat);

      *minimum_height = child_min;
      *natural_height = child_nat;
    }

  *minimum_height += 2 * border_width;
  *natural_height += 2 * border_width;
}

static void
ctk_bin_size_allocate (CtkWidget     *widget,
                       CtkAllocation *allocation)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkBinPrivate *priv = bin->priv;

  ctk_widget_set_allocation (widget, allocation);

  if (priv->child && ctk_widget_get_visible (priv->child))
    {
      CtkAllocation child_allocation;
      gint border_width = ctk_bin_get_effective_border_width (bin);

      child_allocation.x = allocation->x + border_width;
      child_allocation.y = allocation->y + border_width;
      child_allocation.width = allocation->width - 2 * border_width;
      child_allocation.height = allocation->height - 2 * border_width;

      ctk_widget_size_allocate (priv->child, &child_allocation);
    }
}

/**
 * ctk_bin_get_child:
 * @bin: a #CtkBin
 * 
 * Gets the child of the #CtkBin, or %NULL if the bin contains
 * no child widget. The returned widget does not have a reference
 * added, so you do not need to unref it.
 *
 * Returns: (transfer none) (nullable): the child of @bin, or %NULL if it does
 * not have a child.
 **/
CtkWidget*
ctk_bin_get_child (CtkBin *bin)
{
  g_return_val_if_fail (CTK_IS_BIN (bin), NULL);

  return bin->priv->child;
}

void
_ctk_bin_set_child (CtkBin    *bin,
                    CtkWidget *widget)
{
  bin->priv->child = widget;
}
