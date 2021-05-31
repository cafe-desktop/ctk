/* GTK - The GIMP Toolkit
 * Copyright 2001 Sun Microsystems Inc.
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
#include <string.h>

#include "ctkwidget.h"
#include "ctkintl.h"
#include "ctkaccessible.h"

/**
 * SECTION:ctkaccessible
 * @Short_description: Accessibility support for widgets
 * @Title: CtkAccessible
 *
 * The #CtkAccessible class is the base class for accessible
 * implementations for #CtkWidget subclasses. It is a thin
 * wrapper around #AtkObject, which adds facilities for associating
 * a widget with its accessible object.
 *
 * An accessible implementation for a third-party widget should
 * derive from #CtkAccessible and implement the suitable interfaces
 * from ATK, such as #AtkText or #AtkSelection. To establish
 * the connection between the widget class and its corresponding
 * acccessible implementation, override the get_accessible vfunc
 * in #CtkWidgetClass.
 */

struct _CtkAccessiblePrivate
{
  CtkWidget *widget;
};

enum {
  PROP_0,
  PROP_WIDGET
};

static void ctk_accessible_real_connect_widget_destroyed (CtkAccessible *accessible);

G_DEFINE_TYPE_WITH_PRIVATE (CtkAccessible, ctk_accessible, ATK_TYPE_OBJECT)

static void
ctk_accessible_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  CtkAccessible *accessible = CTK_ACCESSIBLE (object);

  switch (prop_id)
    {
    case PROP_WIDGET:
      ctk_accessible_set_widget (accessible, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_accessible_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  CtkAccessible *accessible = CTK_ACCESSIBLE (object);
  CtkAccessiblePrivate *priv = accessible->priv;

  switch (prop_id)
    {
    case PROP_WIDGET:
      g_value_set_object (value, priv->widget);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_accessible_init (CtkAccessible *accessible)
{
  accessible->priv = ctk_accessible_get_instance_private (accessible);
}

static AtkStateSet *
ctk_accessible_ref_state_set (AtkObject *object)
{
  CtkAccessible *accessible = CTK_ACCESSIBLE (object);
  AtkStateSet *state_set;

  state_set = ATK_OBJECT_CLASS (ctk_accessible_parent_class)->ref_state_set (object);

  if (accessible->priv->widget == NULL)
    atk_state_set_add_state (state_set, ATK_STATE_DEFUNCT);

  return state_set;
}

static void
ctk_accessible_real_widget_set (CtkAccessible *accessible)
{
  atk_object_notify_state_change (ATK_OBJECT (accessible), ATK_STATE_DEFUNCT, FALSE);
}

static void
ctk_accessible_real_widget_unset (CtkAccessible *accessible)
{
  atk_object_notify_state_change (ATK_OBJECT (accessible), ATK_STATE_DEFUNCT, TRUE);
}

static void
ctk_accessible_dispose (GObject *object)
{
  CtkAccessible *accessible = CTK_ACCESSIBLE (object);
  
  ctk_accessible_set_widget (accessible, NULL);

  G_OBJECT_CLASS (ctk_accessible_parent_class)->dispose (object);
}

static void
ctk_accessible_class_init (CtkAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *atkobject_class = ATK_OBJECT_CLASS (klass);

  klass->connect_widget_destroyed = ctk_accessible_real_connect_widget_destroyed;
  klass->widget_set = ctk_accessible_real_widget_set;
  klass->widget_unset = ctk_accessible_real_widget_unset;

  atkobject_class->ref_state_set = ctk_accessible_ref_state_set;
  gobject_class->get_property = ctk_accessible_get_property;
  gobject_class->set_property = ctk_accessible_set_property;
  gobject_class->dispose = ctk_accessible_dispose;

  g_object_class_install_property (gobject_class,
				   PROP_WIDGET,
				   g_param_spec_object ("widget",
							P_("Widget"),
							P_("The widget referenced by this accessible."),
							CTK_TYPE_WIDGET,
							G_PARAM_READWRITE));
}

/**
 * ctk_accessible_set_widget:
 * @accessible: a #CtkAccessible
 * @widget: (allow-none): a #CtkWidget or %NULL to unset
 *
 * Sets the #CtkWidget corresponding to the #CtkAccessible.
 *
 * @accessible will not hold a reference to @widget.
 * It is the caller’s responsibility to ensure that when @widget
 * is destroyed, the widget is unset by calling this function
 * again with @widget set to %NULL.
 *
 * Since: 2.22
 */
void
ctk_accessible_set_widget (CtkAccessible *accessible,
                           CtkWidget     *widget)
{
  CtkAccessiblePrivate *priv;
  CtkAccessibleClass *klass;

  g_return_if_fail (CTK_IS_ACCESSIBLE (accessible));

  priv = accessible->priv;
  klass = CTK_ACCESSIBLE_GET_CLASS (accessible);

  if (priv->widget == widget)
    return;

  if (priv->widget)
    klass->widget_unset (accessible);

  priv->widget = widget;

  if (widget)
    klass->widget_set (accessible);

  g_object_notify (G_OBJECT (accessible), "widget");
}

/**
 * ctk_accessible_get_widget:
 * @accessible: a #CtkAccessible
 *
 * Gets the #CtkWidget corresponding to the #CtkAccessible.
 * The returned widget does not have a reference added, so
 * you do not need to unref it.
 *
 * Returns: (nullable) (transfer none): pointer to the #CtkWidget
 *     corresponding to the #CtkAccessible, or %NULL.
 *
 * Since: 2.22
 */
CtkWidget*
ctk_accessible_get_widget (CtkAccessible *accessible)
{
  g_return_val_if_fail (CTK_IS_ACCESSIBLE (accessible), NULL);

  return accessible->priv->widget;
}

/**
 * ctk_accessible_connect_widget_destroyed:
 * @accessible: a #CtkAccessible
 *
 * This function specifies the callback function to be called
 * when the widget corresponding to a CtkAccessible is destroyed.
 *
 * Deprecated: 3.4: Use ctk_accessible_set_widget() and its vfuncs.
 */
void
ctk_accessible_connect_widget_destroyed (CtkAccessible *accessible)
{
  CtkAccessibleClass *class;

  g_return_if_fail (CTK_IS_ACCESSIBLE (accessible));

  class = CTK_ACCESSIBLE_GET_CLASS (accessible);

  if (class->connect_widget_destroyed)
    class->connect_widget_destroyed (accessible);
}

static void
ctk_accessible_widget_destroyed (CtkWidget     *widget,
                                 CtkAccessible *accessible)
{
  ctk_accessible_set_widget (accessible, NULL);
}

static void
ctk_accessible_real_connect_widget_destroyed (CtkAccessible *accessible)
{
  CtkAccessiblePrivate *priv = accessible->priv;

  if (priv->widget)
    g_signal_connect (priv->widget, "destroy",
                      G_CALLBACK (ctk_accessible_widget_destroyed), accessible);
}
