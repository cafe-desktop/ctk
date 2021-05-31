/* CTK - The GIMP Toolkit
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

#include "ctkseparator.h"

#include "ctkorientableprivate.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkrender.h"
#include "ctkwidgetprivate.h"
#include "ctkcsscustomgadgetprivate.h"

/**
 * SECTION:ctkseparator
 * @Short_description: A separator widget
 * @Title: CtkSeparator
 *
 * CtkSeparator is a horizontal or vertical separator widget, depending on the
 * value of the #CtkOrientable:orientation property, used to group the widgets
 * within a window. It displays a line with a shadow to make it appear sunken
 * into the interface.
 *
 * # CSS nodes
 *
 * CtkSeparator has a single CSS node with name separator. The node
 * gets one of the .horizontal or .vertical style classes.
 */


struct _CtkSeparatorPrivate
{
  CtkOrientation orientation;
  CtkCssGadget *gadget;
};


enum {
  PROP_0,
  PROP_ORIENTATION
};


G_DEFINE_TYPE_WITH_CODE (CtkSeparator, ctk_separator, CTK_TYPE_WIDGET,
                         G_ADD_PRIVATE (CtkSeparator)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ORIENTABLE, NULL))


static void
ctk_separator_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  CtkSeparator *separator = CTK_SEPARATOR (object);
  CtkSeparatorPrivate *private = separator->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      if (private->orientation != g_value_get_enum (value))
        {
          private->orientation = g_value_get_enum (value);
          _ctk_orientable_set_style_classes (CTK_ORIENTABLE (object));
          ctk_widget_queue_resize (CTK_WIDGET (object));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_separator_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  CtkSeparator *separator = CTK_SEPARATOR (object);
  CtkSeparatorPrivate *private = separator->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, private->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_separator_get_preferred_width (CtkWidget *widget,
                                   gint      *minimum,
                                   gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_SEPARATOR (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_separator_get_preferred_height (CtkWidget *widget,
                                    gint      *minimum,
                                    gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_SEPARATOR (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_separator_size_allocate (CtkWidget     *widget,
                             CtkAllocation *allocation)
{
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (CTK_SEPARATOR (widget)->priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static gboolean
ctk_separator_draw (CtkWidget *widget,
                    cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_SEPARATOR (widget)->priv->gadget, cr);

  return FALSE;
}

static void
ctk_separator_init (CtkSeparator *separator)
{
  CtkCssNode *widget_node;

  separator->priv = ctk_separator_get_instance_private (separator);
  separator->priv->orientation = CTK_ORIENTATION_HORIZONTAL;

  ctk_widget_set_has_window (CTK_WIDGET (separator), FALSE);

  _ctk_orientable_set_style_classes (CTK_ORIENTABLE (separator));

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (separator));
  separator->priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                                CTK_WIDGET (separator),
                                                                NULL, NULL, NULL,
                                                                NULL, NULL);
}

static void
ctk_separator_finalize (GObject *object)
{
  CtkSeparatorPrivate *priv = CTK_SEPARATOR (object)->priv;

  g_clear_object (&priv->gadget);


  G_OBJECT_CLASS (ctk_separator_parent_class)->finalize (object);
}

static void
ctk_separator_class_init (CtkSeparatorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  object_class->set_property = ctk_separator_set_property;
  object_class->get_property = ctk_separator_get_property;
  object_class->finalize = ctk_separator_finalize;

  widget_class->get_preferred_width = ctk_separator_get_preferred_width;
  widget_class->get_preferred_height = ctk_separator_get_preferred_height;
  widget_class->size_allocate = ctk_separator_size_allocate;

  widget_class->draw = ctk_separator_draw;

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_SEPARATOR);
  ctk_widget_class_set_css_name (widget_class, "separator");
}

/**
 * ctk_separator_new:
 * @orientation: the separator’s orientation.
 *
 * Creates a new #CtkSeparator with the given orientation.
 *
 * Returns: a new #CtkSeparator.
 *
 * Since: 3.0
 */
CtkWidget *
ctk_separator_new (CtkOrientation orientation)
{
  return g_object_new (CTK_TYPE_SEPARATOR,
                       "orientation", orientation,
                       NULL);
}
